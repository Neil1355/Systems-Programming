# Project 2 - File Similarity (JSD)

## Partners
Neil Barot (nnb35)  
Hriday Adani (hva62)

## How to Build and Run
```bash
make
./compare <file-or-directory> [file-or-directory ...]
make clean
```

Example:
```bash
./compare sample/input.txt sample_dir
```

## What We Built
Our program takes a mix of files and directories, builds a word-frequency distribution for each collected file, and then prints the Jensen-Shannon distance for every unordered pair.

Main behavior matches the assignment spec:
- Any file argument is included directly.
- Any directory argument is traversed recursively and only `.txt` files from that traversal are included.
- Names starting with `.` are skipped during traversal.
- Pair results are sorted by decreasing combined word count.

## Work Split
We split this by module and then cross-tested each other’s code.

Neil focused mostly on:
- argument handling and collection phase flow,
- directory traversal + path collection,
- pair generation/sorting + output formatting.

Hriday focused mostly on:
- tokenization and word-frequency map construction,
- dynamic word storage and normalization,
- JSD math implementation and validation.

We reviewed integration together and tested edge cases jointly.

## Design Notes
Data structures used:
- `FileVector`: dynamic array of unique file paths.
- `WordFreq`: total word count + sorted linked list of `WordNode` (`word`, `count`).
- `Comparison`: pair of paths, combined word count, and JSD value.

Tokenization details:
- Uses POSIX `open()`, `read()`, `close()`.
- Reads buffered chunks; no fixed max file size assumption.
- Words are separated by whitespace.
- Valid word characters are letters, digits, and `-`.
- Other punctuation is ignored.
- Case-insensitive handling by lowercasing.
- Word memory is dynamically allocated (no hard max word length).

JSD details:
- For each pair, computes distributions $F_1$ and $F_2$, mean distribution $M$, then:
- $KLD(F_1 || M)$ and $KLD(F_2 || M)$
- $JSD(F_1 || F_2) = \sqrt{0.5 \cdot KLD(F_1 || M) + 0.5 \cdot KLD(F_2 || M)}$
- Uses simultaneous iteration over sorted word lists for the union of terms.

Error handling:
- `stat/open/read` failures are reported with `perror(path)` and processing continues.
- If any recoverable I/O errors occurred, program exits with `EXIT_FAILURE` after finishing.
- Allocation failures are treated as fatal and exit after cleanup.
- Fewer than two readable files -> error + `EXIT_FAILURE`.

## Testing Plan
All testing was done on iLab Linux.

1. Build checks
- `make clean && make`
- Verified clean compile with `-Wall -Wextra -Werror -std=c11`.

2. Interface checks
- No arguments prints usage and exits non-zero.
- Mixed inputs (file + directory + nested directory) run correctly.

3. Traversal checks
- Recursive `.txt` discovery works.
- Explicit non-`.txt` file args are still included.
- Hidden files and directories are skipped.
- Duplicate paths are not added twice.

4. Tokenization checks
- Case-insensitive normalization works.
- Hyphenated words stay intact.
- Punctuation is ignored according to spec.
- Numeric tokens are retained.

5. Correctness checks (JSD)
- Identical files produce near-zero distance.
- Different vocabularies produce higher distance.
- Checked sample-sized hand calculations for sanity.

6. Output ordering checks
- Comparisons sorted by decreasing combined word count.
- Tie ordering is deterministic by path compare.

7. Memory checks
- Ran valgrind leak/error checks.
- Confirmed no leaks and no invalid memory accesses in tested cases.
