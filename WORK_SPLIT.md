# P2 Team Split

## Goal
Minimize merge conflicts by splitting by module boundary, then integrate with one shared output format.

## Neil (Local Windows Workspace)
1. Own directory traversal and program orchestration.
2. Files:
   - `dirwalk.c`
   - `dirwalk.h`
   - `p2compare.c`
   - Makefile updates tied to integration
3. Responsibilities:
   - Recursive walk correctness
   - File filtering logic (what counts as an input text file)
   - Pair generation + final print format
   - Integration testing with partner branch

## Hriday (Linux iLab)
1. Own parsing, frequency distribution, and similarity math.
2. Files:
   - `freqmap.c`
   - `freqmap.h`
   - `similarity.c`
   - `similarity.h`
3. Responsibilities:
   - Token normalization rules from PDF
   - Data structure correctness for word counts
   - Similarity formula and edge cases
   - Unit-style tests with small known files

## Merge Contract (Do This First)
1. Do not rename public functions in headers without agreement.
2. Keep these function signatures stable:
   - `collect_text_files_recursive(...)`
   - `freq_build_from_file(...)`
   - `cosine_similarity(...)`
3. If one side must change API, open a short issue in chat and update both header and caller in same commit.

## Suggested Branches
1. `neil-dirwalk-main`
2. `hriday-freq-sim`
3. `integration-final`

## Suggested Timeline
1. Day 1: each person completes own module with sample files.
2. Day 2: merge to integration branch and fix compile/runtime issues.
3. Day 3: align with PDF output requirements and write final README notes.
