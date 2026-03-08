# OS HW5 – xv6 Filesystem Image Tool

Operating Systems course (TAU) – Homework 5.

## Overview

A user-space tool that reads and manipulates an xv6 filesystem image by memory-mapping it directly. Parses the on-disk xv6 filesystem structures (superblock, inodes, data blocks) without mounting the image.

**Supported commands:**
- `ls` – list files in the root directory with their inode numbers and sizes
- `cp <xv6-file> <linux-file>` – copy a file out of the xv6 image to the host filesystem

## Files

| File | Description |
|------|-------------|
| `hw5.c` | Main tool: `mmap`-based image parser, `ls` and `cp` commands |
| `fs.h` | xv6 filesystem on-disk structures (`superblock`, `dinode`, `dirent`, etc.) |
| `stat.h` | xv6 `stat` struct |
| `types.h` | xv6 primitive type aliases |
| `Makefile` | Build system |
| `solution.pdf` | Written solution document |

## Build & Run

```bash
make
./hw5 <fs.img> ls
./hw5 <fs.img> cp <filename> <output>
```

Example:
```bash
./hw5 fs.img ls
./hw5 fs.img cp README README_out.txt
```
