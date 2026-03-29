# Project 2: File Similarity

## Authors
Neil Barot (nnb35)
Hriday Adani (hva62)

## Overview
This program compares text files by cosine similarity of word-frequency distributions.
It performs the required project tasks:
1. Reads files word-by-word.
2. Recursively traverses a directory.
3. Builds a data structure for word frequencies.

## Work Split (Neil + Hriday)
We split the project roughly in half by module so we could work in parallel and then integrate.

Neil worked mainly on:
1. Directory traversal and file collection logic.
2. Main program flow and pairwise comparison loop.
3. Output formatting and deterministic file ordering.

Hriday worked mainly on:
1. Word normalization and frequency-map construction.
2. Frequency lookup logic for sparse linked-list storage.
3. Cosine similarity computation.

Both of us reviewed final integration together and matched interfaces across files.

## Build and Run (Linux / iLab)
```bash
make
./p2compare <directory>
make clean
```

Example:
```bash
./p2compare ./sample_data
```

## Output
For each unique pair of `.txt` files:
```text
<similarity>  <fileA>  <fileB>
```

Similarity value is printed with 6 decimal places.

## Design

### `p2compare.c`
- Validates input arguments.
- Collects files recursively.
- Sorts file paths for deterministic output.
- Compares all file pairs and prints results.

### `dirwalk.c` / `dirwalk.h`
- Uses `opendir/readdir/stat` for recursive traversal.
- Collects only `.txt` files.
- Stores file paths in a dynamic array (`FileVector`).

### `freqmap.c` / `freqmap.h`
- Reads tokens using `fscanf("%s")`.
- Normalizes tokens by keeping only alphanumeric characters and converting to lowercase.
- Stores frequencies in a linked list (`WordNode`).

### `similarity.c` / `similarity.h`
- Computes cosine similarity:

$$
	ext{sim}(A, B) = \frac{A \cdot B}{\lVert A \rVert_2 \cdot \lVert B \rVert_2}
$$

- Returns `0.0` if either file has no valid tokens.

## Complexity
- Directory traversal: $O(D + F)$ where $D$ = directories and $F$ = files.
- Frequency build (linked list): worst-case $O(T \cdot U)$.
  Here $T$ = tokens and $U$ = unique tokens.
- Pair similarity: worst-case $O(U_A \cdot U_B)$ using linear lookup.

## Error Handling
- Invalid arguments: prints usage and exits.
- Unreadable directories/files: prints clear error.
- Allocation failures: exits safely via existing cleanup paths.
- Less than 2 text files: prints message and exits.

## Final Note
This code is intentionally written in a straightforward style for a systems programming class: simple modules, explicit memory management, and clear control flow.

## Files Included
- `AUTHOR`
- `Makefile`
- `README.md`
- `p2compare.c`
- `dirwalk.c`
- `dirwalk.h`
- `freqmap.c`
- `freqmap.h`
- `similarity.c`
- `similarity.h`
- `p2.pdf`
