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
