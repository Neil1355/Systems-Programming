#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
typedef int socklen_t;
#define socket_close closesocket
#define socket_recv recv
#define socket_send send
#define socket_invalid INVALID_SOCKET
static void net_init(void) {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        perror("WSAStartup");
        exit(EXIT_FAILURE);
    }
}
static void net_cleanup(void) {
    WSACleanup();
}
static void set_nonblocking(socket_t fd) {
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
        perror("ioctlsocket");
        exit(EXIT_FAILURE);
    }
}
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
#define socket_close close
#define socket_recv recv
#define socket_send send
#define socket_invalid (-1)
static void net_init(void) {
}
static void net_cleanup(void) {
}
static void set_nonblocking(socket_t fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}
#endif

#define MAX_NAME 32
#define MAX_STATUS 64
#define MAX_MESSAGE 80

typedef struct buffer {
    char *data;
    size_t len;
    size_t cap;
} buffer_t;

typedef struct client {
    socket_t fd;
    bool logged_in;
    bool closing;
    char *name;
    char *status;
    buffer_t inbuf;
    buffer_t outbuf;
    struct client *next;
} client_t;

static client_t *clients = NULL;
static socket_t listen_fd = socket_invalid;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        die("malloc");
    }
    return ptr;
}

static char *xstrdup(const char *text) {
    char *copy = strdup(text);
    if (copy == NULL) {
        die("strdup");
    }
    return copy;
}

static void buffer_reserve(buffer_t *buf, size_t needed) {
    if (needed <= buf->cap) {
        return;
    }
    size_t cap = buf->cap == 0 ? 256 : buf->cap;
    while (cap < needed) {
        cap *= 2;
    }
    char *data = realloc(buf->data, cap);
    if (data == NULL) {
        die("realloc");
    }
    buf->data = data;
    buf->cap = cap;
}

static void buffer_append(buffer_t *buf, const char *data, size_t len) {
    buffer_reserve(buf, buf->len + len + 1);
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
}

static void buffer_consume(buffer_t *buf, size_t count) {
    if (count >= buf->len) {
        buf->len = 0;
        if (buf->data != NULL) {
            buf->data[0] = '\0';
        }
        return;
    }
    memmove(buf->data, buf->data + count, buf->len - count);
    buf->len -= count;
    buf->data[buf->len] = '\0';
}

static void queue_bytes(client_t *client, const char *data, size_t len) {
    if (client == NULL || client->fd == socket_invalid) {
        return;
    }
    buffer_append(&client->outbuf, data, len);
}

static void queue_protocol(client_t *client, const char *code, const char *body) {
    char header[64];
    size_t body_len = strlen(body);
    int header_len = snprintf(header, sizeof(header), "1|%s|%zu|", code, body_len);
    if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
        die("snprintf");
    }
    queue_bytes(client, header, (size_t)header_len);
    queue_bytes(client, body, body_len);
}

static void queue_err(client_t *client, int code, const char *why) {
    char body[256];
    int body_len = snprintf(body, sizeof(body), "%d|%s|", code, why);
    if (body_len < 0 || (size_t)body_len >= sizeof(body)) {
        die("snprintf");
    }
    queue_protocol(client, "ERR", body);
}

static void queue_msg(client_t *client, const char *sender, const char *recipient, const char *message) {
    size_t needed = strlen(sender) + strlen(recipient) + strlen(message) + 4;
    char *body = xmalloc(needed);
    snprintf(body, needed, "%s|%s|%s|", sender, recipient, message);
    queue_protocol(client, "MSG", body);
    free(body);
}

static void mark_close(client_t *client) {
    if (client != NULL) {
        client->closing = true;
    }
}

static void free_client(client_t *client) {
    if (client == NULL) {
        return;
    }
    if (client->fd != socket_invalid) {
        socket_close(client->fd);
    }
    free(client->name);
    free(client->status);
    free(client->inbuf.data);
    free(client->outbuf.data);
    free(client);
}

static void remove_client(client_t *client) {
    client_t **link = &clients;
    while (*link != NULL && *link != client) {
        link = &(*link)->next;
    }
    if (*link == client) {
        *link = client->next;
    }
    free_client(client);
}

static bool allowed_name_char(unsigned char ch) {
    return isalnum(ch) || ch == '-' || ch == '_';
}

static bool allowed_text_char(unsigned char ch) {
    return ch >= 32 && ch <= 126;
}

static bool is_valid_name(const char *text, size_t len) {
    if (len < 1 || len > MAX_NAME) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (!allowed_name_char((unsigned char)text[i])) {
            return false;
        }
    }
    return true;
}

static bool is_valid_status(const char *text, size_t len) {
    if (len > MAX_STATUS) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (!allowed_text_char((unsigned char)text[i])) {
            return false;
        }
    }
    return true;
}

static bool is_valid_message(const char *text, size_t len) {
    if (len < 1 || len > MAX_MESSAGE) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (!allowed_text_char((unsigned char)text[i])) {
            return false;
        }
    }
    return true;
}

static client_t *find_client_by_name(const char *name) {
    for (client_t *client = clients; client != NULL; client = client->next) {
        if (client->logged_in && client->name != NULL && strcmp(client->name, name) == 0) {
            return client;
        }
    }
    return NULL;
}

static char *duplicate_slice(const char *start, size_t len) {
    char *copy = xmalloc(len + 1);
    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

static size_t find_bar(const char *data, size_t limit, size_t start) {
    for (size_t i = start; i < limit; i++) {
        if (data[i] == '|') {
            return i;
        }
    }
    return limit;
}

static bool split_body(const char *body, size_t body_len, size_t field_count, char **fields) {
    if (body_len == 0 || body[body_len - 1] != '|') {
        return false;
    }
    size_t start = 0;
    for (size_t i = 0; i + 1 < field_count; i++) {
        size_t bar = find_bar(body, body_len - 1, start);
        if (bar == body_len - 1) {
            return false;
        }
        fields[i] = duplicate_slice(body + start, bar - start);
        start = bar + 1;
    }
    fields[field_count - 1] = duplicate_slice(body + start, body_len - 1 - start);
    return true;
}

static char *join_user_list(void) {
    size_t total = 1;
    size_t count = 0;
    for (client_t *client = clients; client != NULL; client = client->next) {
        if (!client->logged_in || client->name == NULL) {
            continue;
        }
        size_t line_len = strlen(client->name);
        if (client->status != NULL && client->status[0] != '\0') {
            line_len += 2 + strlen(client->status);
        }
        if (count > 0) {
            total += 1;
        }
        total += line_len;
        count++;
    }

    char *result = xmalloc(total);
    result[0] = '\0';
    bool first = true;
    for (client_t *client = clients; client != NULL; client = client->next) {
        if (!client->logged_in || client->name == NULL) {
            continue;
        }
        if (!first) {
            strcat(result, "\n");
        }
        first = false;
        strcat(result, client->name);
        if (client->status != NULL && client->status[0] != '\0') {
            strcat(result, ": ");
            strcat(result, client->status);
        }
    }
    return result;
}

static void announce_status_change(client_t *client, const char *status) {
    char body[256];
    int body_len = snprintf(body, sizeof(body), "%s is now \"%s\"", client->name, status);
    if (body_len < 0 || (size_t)body_len >= sizeof(body)) {
        return;
    }
    for (client_t *other = clients; other != NULL; other = other->next) {
        if (other->logged_in) {
            queue_msg(other, "#all", "#all", body);
        }
    }
}

static void process_name(client_t *client, const char *name) {
    size_t len = strlen(name);
    if (len > MAX_NAME) {
        queue_err(client, 4, "Too long");
        return;
    }
    if (!is_valid_name(name, len)) {
        queue_err(client, 3, "Illegal character");
        return;
    }
    if (find_client_by_name(name) != NULL) {
        queue_err(client, 1, "Name in use");
        return;
    }
    free(client->name);
    free(client->status);
    client->name = xstrdup(name);
    client->status = xstrdup("");
    client->logged_in = true;
    queue_msg(client, "#all", client->name, "Welcome to the chat!");
}

static void process_set(client_t *client, const char *status) {
    size_t len = strlen(status);
    if (len > MAX_STATUS) {
        queue_err(client, 4, "Too long");
        return;
    }
    if (!is_valid_status(status, len)) {
        queue_err(client, 3, "Illegal character");
        return;
    }
    free(client->status);
    client->status = xstrdup(status);
    if (status[0] != '\0') {
        announce_status_change(client, status);
    }
}

static void process_msg(client_t *client, const char *recipient, const char *message) {
    size_t len = strlen(message);
    if (len > MAX_MESSAGE) {
        queue_err(client, 4, "Too long");
        return;
    }
    if (!is_valid_message(message, len)) {
        queue_err(client, 3, "Illegal character");
        return;
    }
    if (strcmp(recipient, "#all") == 0) {
        for (client_t *other = clients; other != NULL; other = other->next) {
            if (other->logged_in) {
                queue_msg(other, client->name, "#all", message);
            }
        }
        return;
    }
    size_t rec_len = strlen(recipient);
    if (rec_len > MAX_NAME) {
        queue_err(client, 4, "Too long");
        return;
    }
    if (!is_valid_name(recipient, rec_len)) {
        queue_err(client, 3, "Illegal character");
        return;
    }
    client_t *target = find_client_by_name(recipient);
    if (target == NULL) {
        queue_err(client, 2, "Unknown recipient");
        return;
    }
    queue_msg(target, client->name, recipient, message);
}

static void process_who(client_t *client, const char *query) {
    if (strcmp(query, "#all") == 0) {
        char *list = join_user_list();
        queue_msg(client, "#all", client->name, list);
        free(list);
        return;
    }

    size_t len = strlen(query);
    if (len > MAX_NAME) {
        queue_err(client, 4, "Too long");
        return;
    }
    if (!is_valid_name(query, len)) {
        queue_err(client, 3, "Illegal character");
        return;
    }

    client_t *target = find_client_by_name(query);
    if (target == NULL) {
        queue_err(client, 2, "Unknown recipient");
        return;
    }

    char body[256];
    if (target->status != NULL && target->status[0] != '\0') {
        snprintf(body, sizeof(body), "%s: %s", target->name, target->status);
    } else {
        snprintf(body, sizeof(body), "%s: No status", target->name);
    }
    queue_msg(client, "#all", client->name, body);
}

static void handle_message(client_t *client, const char *code, const char *body, size_t body_len) {
    if (!client->logged_in && strcmp(code, "NAM") != 0) {
        queue_err(client, 0, "Unreadable");
        mark_close(client);
        return;
    }

    if (strcmp(code, "NAM") == 0) {
        char *fields[1] = { NULL };
        if (!split_body(body, body_len, 1, fields)) {
            queue_err(client, 0, "Unreadable");
            mark_close(client);
        } else {
            process_name(client, fields[0]);
        }
        free(fields[0]);
        return;
    }

    if (strcmp(code, "SET") == 0) {
        char *fields[1] = { NULL };
        if (!split_body(body, body_len, 1, fields)) {
            queue_err(client, 0, "Unreadable");
            mark_close(client);
        } else {
            process_set(client, fields[0]);
        }
        free(fields[0]);
        return;
    }

    if (strcmp(code, "MSG") == 0) {
        char *fields[3] = { NULL, NULL, NULL };
        if (!split_body(body, body_len, 3, fields)) {
            queue_err(client, 0, "Unreadable");
            mark_close(client);
        } else {
            process_msg(client, fields[1], fields[2]);
        }
        free(fields[0]);
        free(fields[1]);
        free(fields[2]);
        return;
    }

    if (strcmp(code, "WHO") == 0) {
        char *fields[1] = { NULL };
        if (!split_body(body, body_len, 1, fields)) {
            queue_err(client, 0, "Unreadable");
            mark_close(client);
        } else {
            process_who(client, fields[0]);
        }
        free(fields[0]);
        return;
    }

    queue_err(client, 0, "Unreadable");
    mark_close(client);
}

static size_t find_header_bar(const char *data, size_t len, size_t start) {
    for (size_t i = start; i < len; i++) {
        if (data[i] == '|') {
            return i;
        }
    }
    return len;
}

static bool parse_length(const char *text, size_t *value) {
    if (text[0] == '\0') {
        return false;
    }
    size_t result = 0;
    for (const char *p = text; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            return false;
        }
        result = result * 10 + (size_t)(*p - '0');
        if (result > 99999) {
            return false;
        }
    }
    *value = result;
    return true;
}

static bool read_header(const char *data, size_t len,
                        char **version, char **code, char **length_text,
                        size_t *body_start) {
    size_t first = find_header_bar(data, len, 0);
    if (first == len) {
        return false;
    }
    size_t second = find_header_bar(data, len, first + 1);
    if (second == len) {
        return false;
    }
    size_t third = find_header_bar(data, len, second + 1);
    if (third == len) {
        return false;
    }
    *version = duplicate_slice(data, first);
    *code = duplicate_slice(data + first + 1, second - first - 1);
    *length_text = duplicate_slice(data + second + 1, third - second - 1);
    *body_start = third + 1;
    return true;
}

static bool process_input(client_t *client) {
    while (client->inbuf.len > 0) {
        char *version = NULL;
        char *code = NULL;
        char *length_text = NULL;
        size_t body_start = 0;

        if (!read_header(client->inbuf.data, client->inbuf.len, &version, &code, &length_text, &body_start)) {
            break;
        }

        size_t body_len = 0;
        bool header_ok = strcmp(version, "1") == 0 && parse_length(length_text, &body_len);
        free(version);
        free(length_text);

        if (!header_ok) {
            free(code);
            queue_err(client, 0, "Unreadable");
            mark_close(client);
            return true;
        }

        size_t total_needed = body_start + body_len;
        if (client->inbuf.len < total_needed) {
            free(code);
            break;
        }

        if (body_len == 0 || client->inbuf.data[total_needed - 1] != '|') {
            free(code);
            queue_err(client, 0, "Unreadable");
            mark_close(client);
            return true;
        }

        char *body = duplicate_slice(client->inbuf.data + body_start, body_len);
        buffer_consume(&client->inbuf, total_needed);
        handle_message(client, code, body, body_len);
        free(code);
        free(body);

        if (client->closing) {
            break;
        }
    }
    return true;
}

static bool flush_output(client_t *client) {
    while (client->outbuf.len > 0) {
        ssize_t n = socket_send(client->fd, client->outbuf.data, (int)client->outbuf.len, 0);
        if (n > 0) {
            buffer_consume(&client->outbuf, (size_t)n);
            continue;
        }
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
            return true;
        }
#else
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        }
#endif
        remove_client(client);
        return false;
    }

    if (client->closing) {
        remove_client(client);
        return false;
    }
    return true;
}

static socket_t create_listener(const char *port_text) {
    char *end = NULL;
    long port = strtol(port_text, &end, 10);
    if (end == port_text || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "usage: chatd <port>\n");
        exit(EXIT_FAILURE);
    }

    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == socket_invalid) {
        die("socket");
    }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        die("bind");
    }
    if (listen(fd, 16) < 0) {
        die("listen");
    }
    set_nonblocking(fd);
    return fd;
}

static client_t *accept_client(void) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    socket_t fd = accept(listen_fd, (struct sockaddr *)&addr, &addr_len);
    if (fd == socket_invalid) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
            return NULL;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return NULL;
        }
#endif
        die("accept");
    }

    set_nonblocking(fd);
    client_t *client = xmalloc(sizeof(*client));
    memset(client, 0, sizeof(*client));
    client->fd = fd;
    client->next = clients;
    clients = client;
    return client;
}

static bool read_client(client_t *client) {
    char temp[4096];
    for (;;) {
        ssize_t n = socket_recv(client->fd, temp, sizeof(temp), 0);
        if (n > 0) {
            buffer_append(&client->inbuf, temp, (size_t)n);
            continue;
        }
        if (n == 0) {
            remove_client(client);
            return false;
        }
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
            break;
        }
#else
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
#endif
        remove_client(client);
        return false;
    }

    process_input(client);
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: chatd <port>\n");
        return EXIT_FAILURE;
    }

    net_init();
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    listen_fd = create_listener(argv[1]);

    while (1) {
        fd_set readfds;
        fd_set writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(listen_fd, &readfds);
        socket_t maxfd = listen_fd;

        for (client_t *client = clients; client != NULL; client = client->next) {
            if (client->fd == socket_invalid) {
                continue;
            }
            if (!client->closing) {
                FD_SET(client->fd, &readfds);
            }
            if (client->outbuf.len > 0) {
                FD_SET(client->fd, &writefds);
            }
            if (client->fd > maxfd) {
                maxfd = client->fd;
            }
        }

        int ready = select((int)maxfd + 1, &readfds, &writefds, NULL, NULL);
        if (ready < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEINTR) {
                continue;
            }
#else
            if (errno == EINTR) {
                continue;
            }
#endif
            die("select");
        }

        if (FD_ISSET(listen_fd, &readfds)) {
            while (accept_client() != NULL) {
            }
        }

        client_t *client = clients;
        while (client != NULL) {
            client_t *next = client->next;
            bool alive = true;

            if (client->fd != socket_invalid && !client->closing && FD_ISSET(client->fd, &readfds)) {
                alive = read_client(client);
            }
            if (alive && client->fd != socket_invalid && FD_ISSET(client->fd, &writefds)) {
                alive = flush_output(client);
            }
            if (alive && client->closing && client->outbuf.len == 0) {
                remove_client(client);
            }

            client = next;
        }
    }

    net_cleanup();
    return EXIT_SUCCESS;
}