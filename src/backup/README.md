# Linux Backup and Recovery System

This directory contains the core backup and recovery functionality of the system. The backup system implements a chunk-based, deduplicated backup solution with support for full, incremental, and differential backups.

## System Architecture

The backup system consists of several key components:

1. **Chunker (`chunker.cpp`)**: 
   - Splits files into variable-sized chunks using content-defined chunking
   - Implements streaming chunking for efficient memory usage
   - Generates unique hashes for each chunk

2. **Backup (`backup.cpp`)**:
   - Handles the backup process for files and directories
   - Supports three backup types:
     - Full: Complete backup of all files
     - Incremental: Only backs up changed files since last backup
     - Differential: Backs up changes since last full backup
   - Compresses chunks using ZSTD compression
   - Maintains metadata about backed-up files

3. **Restore (`restore.cpp`)**:
   - Restores files from backup
   - Handles both single file and full system restore
   - Decompresses chunks during restore
   - Preserves file metadata (timestamps, etc.)

4. **Progress Tracking (`progress.cpp`)**:
   - Provides real-time progress updates during backup/restore
   - Shows transfer speed and completion percentage

## Backup Storage Structure

The backup system creates the following directory structure:

```
backup_root/
├── backup/           # Contains backup metadata files
│   └── YYYYMMDD_HHMMSS  # Backup metadata (JSON format)
└── chunks/           # Contains compressed data chunks
    └── XX/           # Subdirectories based on chunk hash
        └── hash.chunk # Compressed chunk files
```

## Using the Backup UI

The backup-ui program provides a command-line interface to interact with the backup system. Here are the available commands:

### Backup Commands

1. Create a backup:
```bash
./backup-ui backup --type [full | incremental | differential]  --input-path <INPUT_PATH> --output-path <OUTPUT_PATH>  --remarks <REMARKs>
```
DEFAULT : type - full, remark - null

2. Restore a specific backup:
```bash
./backup-ui restore file --input-path <INPUT_PATH> --output-path <OUTPUT_PATH> 
--backup-name <BACKUP_NAME>
```

3. List available backups:
```bash
./backup-ui list --input-path <INPUT_PATH> 
```

4. Compare 2 backups:
```bash
./backup-ui compare <BACKUP_NAME_1> <BACKUP_NAME_2> --input-path <INPUT_PATH> 

```
## Example Usage

1. Create a backup:
```bash
./backup-ui backup --input-path "/media/mukundh/New Volume/tokens" --output-path "/media/mukundh/New Volume/big2" 
```

2. Restore a specific backup:
```bash
./backup-ui restore --output-path "/media/mukundh/New Volume/test22r" --input-path "/media/mukundh/New Volume/big2"  --backup-name 20250609_112320
```

3. List available backups:
```bash
./backup-ui list --input-path "/media/mukundh/New Volume/big2"
```

4. Compare 2 backups:
```bash
./backup-ui compare 20250609_122538 20250609_112320 --input-path "/media/mukundh/New Volume/big2"

```

## Technical Details

- Uses ZSTD compression for efficient storage
- Implements content-defined chunking for deduplication
- Preserves file metadata (timestamps, permissions)
- Supports streaming operations for memory efficiency
- Uses SHA-256 for chunk identification
- JSON-based metadata storage 