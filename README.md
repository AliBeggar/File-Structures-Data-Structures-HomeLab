# File Structures & Data Structures Homelab (SFSD)

A C implementation of block-based secondary-storage file organization, buffered I/O, binary record serialization, metadata inspection, and multi-criteria file fragmentation with explicit physical I/O cost tracking. Built for the **SFSD (Structures de Fichiers et Structures de Données)** module at **ESI (École nationale Supérieure d'Informatique, Algiers)**, taught by **Prof. Hidouci Walid Khaled**.

## Overview

Designing efficient algorithms for handling large volumes of data on external memory requires a deep understanding of physical **input/output (I/O) operations**, data organization across primary and secondary memory, and algorithmic complexity.

This homelab (`main.c`) implements an interactive CLI **File Management System** operating on fixed-size blocks (`BLOCK_SIZE = 100`) with a dedicated file header (`theader`). Using `algeria_cities_geocoded.csv` as input, it builds a contiguous binary file (`cities_binary_file.bin`), inspects file characteristics, and partitions records across multiple output files while counting every block read and write.

## Storage Model & Memory Layout

| Structure | Fields | Size | Role |
|---|---|---|---|
| `record` | `id[5]`, `commune[25]`, `daira[22]`, `wilaya_code[3]`, `wilaya_name[21]` | 76 bytes | Fixed-size binary record |
| `tblock` | `record tab[100]` (`BLOCK_SIZE = 100`) | 7,600 bytes (~7.42 KB) | Physical I/O transfer unit (buffer/block) |
| `theader` | `last_block`, `nb`, `nb_records` (`unsigned int`) | 12 bytes | Header block stored at byte offset `0` |
| `file` | `FILE *F`, `theader h` | Handle struct | Application-level file abstraction wrapping C streams |

Block `i` is addressed directly via `fseek(F, i * sizeof(tblock) + sizeof(theader), SEEK_SET)` in `read_block()` and `write_block()`.

## Implemented Features

1. **Initial CSV-to-Binary Loading (`load_file`)**: Parses `algeria_cities_geocoded.csv`, buffers records into `tblock`, flushes full blocks to `cities_binary_file.bin`, and persists `theader` metadata.
2. **Header & Metadata Inspection (`show_header`, `read_and_display_metadata`)**: Displays `last_block`, `nb`, `nb_records`, file size (bytes/KB), total block count, and minimum/maximum record IDs.
3. **Formatted Record Display (`show_file`, `show_file_by_name`)**: Reads blocks sequentially and prints tabular output for `cities_binary_file.bin` or any generated fragment file.
4. **Fragmentation by First Letter of Wilaya (`fragmentate_by_firstLetter_Wilaya`)**: Partitions records into `A.bin`–`Z.bin` by `wilaya_name[0]`, removes empty files (`nb_records == 0`), and reports block I/O cost.
5. **Fragmentation by Wilaya Code (`fragmentate_by_wilaya_code`)**: Distributes records across `nb_files` (`file_1.bin`–`file_99.bin`) over wilaya codes `1..58` using buffered writes and tracks I/O cost.
6. **Fragmentation by ID Range (`fragmentate_by_id`)**: Partitions records into `file_0001.bin`–`file_1541.bin` using a user-defined `step` over IDs `1..1541` and tracks I/O cost.
7. **Cleanup Routines (`delete_by_first_letter`, `delete_by_willaya_code`, `delete_by_id_range`, `delete_all_files`)**: Removes generated binary fragments individually by mode or all at once.

## Performance Metrics & I/O Cost Analysis

Let $B_{\text{in}} = \text{last\_block} + 1$ be the number of blocks in `cities_binary_file.bin`, $B_{\text{out}}$ be the total number of blocks written across output fragments, and $N$ be the total number of records.

| Operation | Block I/O Cost | Memory Footprint | Bottleneck / Notes |
|---|---|---|---|
| **Initial Load (`load_file`)** | $B_{\text{in}}$ writes + 1 header write | 1 `tblock` (~7.6 KB) | Single sequential pass over CSV |
| **Metadata Scan (`read_and_display_metadata`)** | $B_{\text{in}}$ reads | 1 `tblock` (~7.6 KB) | Full scan required only to compute `min_id` and `max_id` |
| **Fragment by ID (`fragmentate_by_id`)** | $B_{\text{in}} + B_{\text{out}}$ | `nb_files` × 7.6 KB (heap) | Single-pass I/O; $O(\text{nb\_files})$ linear bucket lookup per record; opens up to 1,541 FDs when `step = 1` |
| **Fragment by Wilaya Code (`fragmentate_by_wilaya_code`)** | $B_{\text{in}} + B_{\text{out}}$ | `nb_files` × 7.6 KB (stack VLA) | Single-pass I/O; $O(\text{nb\_files})$ linear bucket lookup per record |
| **Fragment by First Letter (`fragmentate_by_firstLetter_Wilaya`)** | $26 \cdot B_{\text{in}} + B_{\text{out}}$ (+ 26 header opens) | 2 `tblock`s (~15.2 KB) | Multi-pass: re-reads entire input file once per letter `'A'`–`'Z'` |

### Optimization Opportunities

- **Single-Pass Alphabetical Partitioning**: Replace the 26-pass loop in `fragmentate_by_firstLetter_Wilaya` with 26 in-memory output buffers (~198 KB total) to drop input I/O from $26 \cdot B_{\text{in}}$ to $B_{\text{in}}$ (a **26× reduction** in disk reads) and eliminate the secondary open/remove pass.
- **$O(1)$ Direct Bucket Indexing**: Replace the inner `for (k = 0; k < nb_files; k++)` range-search loops in `fragmentate_by_id` and `fragmentate_by_wilaya_code` with direct calculation (`k = (id - 1) / step` and `k = (wilaya_code - 1) / step`).
- **Header-Cached Extremes**: Store `min_id` and `max_id` directly inside `theader` so `read_and_display_metadata()` runs in $O(1)$ (1 header read instead of $B_{\text{in}}$ block reads).
- **Binary Numeric Fields & Higher Blocking Factor**: Store `id` (`uint16_t`) and `wilaya_code` (`uint8_t`) as binary integers rather than ASCII strings to remove repeated `atoi()` calls and pack more records per physical block.
- **Memory & File Descriptor Hygiene**: Free the heap-allocated `file` struct inside `close_file()` (and change its return type to `void`), replace the stack VLA in `fragmentate_by_wilaya_code` with `malloc`, and guard against OS open-file-descriptor limits (`ulimit -n`) when `nb_files` is large.

## Full SFSD Course Syllabus & Resources

Course homepage: [Prof. Hidouci Walid Khaled — 3. File Structures (ESI Algiers)](https://sites.google.com/site/hidouciwk/cours/file-structures)

- **Full Course Handout (PDF)**: [Complete Course Document](https://drive.google.com/file/d/1GKRQcoNrZ3hkPZnhiCjGFMfc2WCthwjK/view?usp=sharing)
- **0. Course Presentation**: [Slides](https://drive.google.com/file/d/11Y0Qc9MyISaaShiQSA1wl_cfGJn8m75O/view?usp=sharing)
- **1. General Concepts** ([Slides](https://drive.google.com/file/d/1jnFQLEPOHecZ543uarI91g3MhZEWrLtu/view?usp=sharing)):
  - Notion of files, buffered I/O, physical I/O, buffers, blocks, cache/buffer zones, secondary memory
  - Files at application level, C Streams
  - Physical-level file structure model, complexity and cost of file operations
  - Header block and file characteristics
- **2. Simple File Structures** ([Slides](https://drive.google.com/file/d/1k3W142TUfYx5R4ATxx-CodOYOwZzKyVC/view?usp=sharing)):
  - Sequential access structures, contiguous blocks, chained (linked-list) blocks
  - Sorted files, external binary search, initial loading, periodic reorganizations, file merging
  - Variable-size records and handling of interblock overlaps
- **3. Indexed Files** ([Slides](https://drive.google.com/file/d/1btSby-mC6tO1RyJPrVVNTcoWUH0jLYC6/view?usp=sharing)):
  - Main-memory indexes, primary index, secondary index, loading operation, index backup
  - Secondary-storage indexes, inverted lists, multi-key access, multi-level index, bitmap index
- **4. Trees in Secondary Storage**:
  - M-ary search trees in secondary memory ([Slides](https://drive.google.com/file/d/1Y8Jmy7412QI1Z4sXFhGNV1u0kcvpt-D4/view?usp=sharing))
  - B-Trees ([Slides](https://drive.google.com/file/d/1hEDocWhC-3D_NwhkEfMPHhohZSkkK55L/view?usp=sharing))
  - Variants of B-Trees: B+, B*, Prefixed B+ ([Slides](https://drive.google.com/file/d/1VE0GoAPFH2n70PhS135eOLJFKj_N9daf/view?usp=sharing))
  - C Program Examples: [B-tree file structure](https://sites.google.com/site/hidouciwk/cours/programmes#h.p_ID_54)
- **5. Hashing in Secondary Storage** ([Slides](https://drive.google.com/file/d/1C4SvGi3fBbOaYTPCbeYp7p7jzmj5_zOU/view?usp=sharing)):
  - Static hashing methods
  - Dynamic hashing methods (Linear Hashing)
- **6. High-Level Operations on Files**:
  - Sorting files ([Slides](https://drive.google.com/file/d/1Q0y0NJACErgrbrtUPCPeLIttYuBCnLJV/view?usp=sharing)): Multi-Files Merging Algorithm, External Sort Algorithm, File Fragmentation
  - Join algorithms ([Slides](https://drive.google.com/file/d/1_q_VisUqCUp4_9aX2f2_dJBejLCbPM6B/view?usp=sharing)): Nested-Loops Join, Sort-Merge Join, Hash Join
  - Parallelism in high-level operations ([Slides](https://drive.google.com/file/d/1nfMzXXySvcKBJibM4hIydUYgn7uDQDWy/view?usp=sharing)): Parallel merging and parallel sorting
  - C Program Examples: [External sort, Multi-Files merging, and Parallel sorting for clusters](https://sites.google.com/site/hidouciwk/cours/programmes#h.p_ID_54)
