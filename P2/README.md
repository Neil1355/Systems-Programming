# Project 2: File Similarity

## Authors
Neil Barot (nnb35)
Hriday Adani (hva62)

## Project Goal
This project compares text files by similarity based on each file's word-frequency distribution.
The program does three major tasks that match the assignment scope:
1. Reads text files word-by-word.
2. Recursively traverses a directory to discover candidate input files.
3. Builds a data structure representing each file's word-frequency distribution and compares distributions using cosine similarity.

## Current Collaboration State
This repository is intentionally split for parallel work:
1. Neil-owned traversal/orchestration code is active.
2. Hriday-owned parsing/similarity code is currently left as TODO placeholders (`freqmap.c`, `similarity.c`) and will be filled on Hriday's branch.

## High-Level Program Flow
1. Start in `main()` inside `p2compare.c`.
2. Recursively collect `.txt` files under the provided directory path.
3. For each unique file pair `(i, j)` where `i < j`:
	 - Build frequency map for file `i`.
	 - Build frequency map for file `j`.
	 - Compute cosine similarity.
	 - Print result.
4. Free all allocated memory before exiting.

## Build and Run

### Linux / iLab
```bash
make
./p2compare ./sample_data
make clean
```

### Expected Command Usage
```bash
./p2compare <directory-containing-text-files>
```

If fewer than two `.txt` files are found, the program prints an error and exits.

## Output Format
Current output line format per pair is:
```text
<similarity>  <fileA>  <fileB>
```

Where `<similarity>` is a floating-point cosine similarity in the range `[0.0, 1.0]` for non-negative term frequencies.

## File-by-File Design

### `p2compare.c`
Responsibilities:
- Validates command-line arguments.
- Coordinates file discovery and pairwise comparison.
- Handles top-level error reporting.

Important details:
- Uses `FileVector` from `dirwalk.h` for discovered files.
- Calls `freq_build_from_file()` and `cosine_similarity()` per pair.
- Frees each frequency map after use.

### `dirwalk.h` / `dirwalk.c`
Responsibilities:
- Defines a dynamic vector type (`FileVector`) for storing file paths.
- Recursively traverses the directory tree using POSIX directory APIs.
- Filters only files ending in `.txt`.

Implementation notes:
- `collect_text_files_recursive()` calls a static helper to recurse.
- Uses `stat()` to distinguish directories vs regular files.
- Grows vector capacity geometrically (`8, 16, 32, ...`) for amortized efficient appends.

### `freqmap.h` / `freqmap.c`
Responsibilities:
- Defines linked-list node structure for word counts.
- Reads each file token-by-token using `fscanf("%s")`.
- Normalizes tokens to lowercase.
- Inserts new words or increments existing counts.

Implementation notes:
- Uses singly linked list for clarity and straightforward manual memory management.
- `freq_get_count()` performs linear lookup by word.
- `freq_free()` releases node and word allocations.

### `similarity.h` / `similarity.c`
Responsibilities:
- Computes cosine similarity of two sparse frequency distributions.

Math:
If vectors are $A$ and $B$, cosine similarity is:

$$
	ext{sim}(A, B) = \frac{A \cdot B}{\lVert A \rVert_2 \cdot \lVert B \rVert_2}
$$

In implementation:
- Dot product is accumulated by iterating words in map A and looking up counts in map B.
- Magnitudes are computed as square roots of sum of squared counts.
- If either magnitude is zero, similarity is defined as `0.0`.

## Data Structures and Complexity

### File collection
- Structure: dynamic array of `char*` paths.
- Time complexity: $O(F + D)$ for tree traversal overhead, where $F$ is files and $D$ is directories.

### Frequency map (current implementation)
- Structure: singly linked list of `(word, count)` nodes.
- Insert/update per token: worst-case $O(U)$ where $U$ is number of unique words in that file.
- Build map over $T$ tokens: worst-case $O(T \cdot U)$.

### Similarity
- Dot product currently does repeated linear lookups: worst-case $O(U_A \cdot U_B)$.
- Magnitudes: $O(U_A + U_B)$.

### Practical note
This implementation prioritizes correctness and readability for systems programming fundamentals.
If performance optimization is required, replace list-based map with a hash table to reduce expected lookup to $O(1)$ average.

## Memory Management Strategy
- Every dynamically allocated path pushed into `FileVector` is freed in `filevec_free()`.
- Every word node allocates:
	- node struct
	- heap copy of token string
- `freq_free()` releases both pieces for every node.
- Error paths attempt to free already-allocated resources before returning.

## Error Handling
The program explicitly handles:
1. Missing or incorrect command-line arguments.
2. Directory traversal/open failures.
3. File read/open failures.
4. Allocation failures in vectors or frequency maps.
5. Degenerate comparison cases (zero-vector norms).

## Assumptions and Scope
Current assumptions in this starter:
1. Input files of interest end with `.txt`.
2. Tokens are split by whitespace (`fscanf("%s")`).
3. Normalization only lowercases tokens; punctuation is not yet stripped.

If assignment requirements specify extra normalization rules (punctuation removal, stemming, stop-word filtering, sorting constraints, minimum word length), those rules should be implemented exactly before final submission.

## Testing Plan

### Functional tests
1. Single directory with two small known files to verify manual cosine values.
2. Nested directories to verify recursive traversal.
3. Mixture of `.txt` and non-`.txt` files to verify filtering.
4. Empty directory and single-file directory for expected error handling.
5. File with repeated words and mixed case (`Word word WORD`) for normalization behavior.

### Robustness tests
1. Nonexistent root path.
2. Permission-restricted subdirectory (if available on iLab).
3. Large text file for stress testing memory and runtime.

### Example manual validation
For files:
- A: `cat cat dog`
- B: `cat dog dog`

Vectors on vocabulary `[cat, dog]`:
- $A = [2,1]$
- $B = [1,2]$

Similarity:
$$
\frac{2\cdot1 + 1\cdot2}{\sqrt{2^2+1^2}\sqrt{1^2+2^2}} = \frac{4}{5} = 0.8
$$

Program output for this pair should be close to `0.800000`.

## Collaboration and Work Split
Detailed ownership and merge contract are in `WORK_SPLIT.md`.

Summary:
- Neil owns directory traversal and top-level orchestration.
- Hriday owns token/frequency/similarity internals.
- Shared headers are treated as API contracts to avoid merge churn.

## Build Artifacts and Repository Hygiene
- `Makefile` builds `p2compare` from module sources.
- `.gitignore` excludes binaries and object files.
- `docs/CHANGELOG.md` tracks significant project updates.

## Known Limitations (Current Starter)
1. Frequency map uses linear linked-list lookup (not optimized for large corpora).
2. Tokenization is whitespace-based and currently keeps punctuation attached.
3. Output ordering is pair-generation order from discovered path sequence.

These are acceptable for initial integration and can be tightened based on final grading rubric requirements from the assignment PDF.

## Submission Checklist
Before final submit:
1. Verify output format exactly matches the professor PDF examples.
2. Confirm token normalization rules match assignment text.
3. Run tests on iLab Linux using the final branch.
4. Remove debug prints and ensure clean `-Wall -Wextra -Werror` build.
5. Ensure README and changelog reflect final behavior.
