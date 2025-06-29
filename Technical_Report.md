# ResilioZ - Linux Backup and Recovery System Technical Report

**Project:** ResilioZ  
**Version:** 0.1  
**Date:** December 2024  
**Author:** Technical Analysis Team  

---

## Executive Summary

ResilioZ is a Linux-based backup and recovery system implemented in C++17. The system provides both command-line interface (CLI) and graphical user interface (GUI) for managing backup operations across multiple repository types including local filesystem, Network File System (NFS), and remote SSH/SFTP storage.

The system implements content-defined chunking for data deduplication, Zstandard compression for storage optimization, and automated scheduling capabilities. ResilioZ is designed for enterprise environments requiring reliable data protection with support for full, incremental, and differential backup strategies.

---

## 1. System Overview

### 1.1 Architecture

ResilioZ follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│  CLI Interface  │  GUI Interface  │  Scheduler Service      │
├─────────────────────────────────────────────────────────────┤
│                    Business Logic Layer                     │
├─────────────────────────────────────────────────────────────┤
│  Backup System  │  Restore System │  Services System        │
├─────────────────────────────────────────────────────────────┤
│                    Core Components                          │
├─────────────────────────────────────────────────────────────┤
│  Chunker        │  Compression   │  Metadata Management     │
├─────────────────────────────────────────────────────────────┤
│                    Repository Layer                         │
├─────────────────────────────────────────────────────────────┤
│  Local Storage  │  NFS Storage   │  Remote Storage          │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Core Components

- **Backup System**: Orchestrates backup operations and manages backup types
- **Restore System**: Handles data restoration with location flexibility
- **Services System**: Manages repository and scheduling services
- **Repository Layer**: Abstract interface for multiple storage backends
- **Chunker**: Content-defined chunking for deduplication
- **Compression**: Zstandard compression for storage optimization

---

## 2. Installation and Setup

### 2.1 Prerequisites

#### 2.1.1 System Requirements
- **Operating System**: Linux (Ubuntu 18.04+, RHEL 7+, SUSE 12+)
- **Architecture**: x86_64 or ARM64
- **Memory**: 2GB RAM minimum, 4GB recommended
- **Storage**: Variable based on data volume

#### 2.1.2 Required Dependencies
Based on the CMakeLists.txt configuration, the following dependencies are required:

**System Packages:**
```bash
# Ubuntu/Debian
sudo apt-get install libssh-dev libzstd-dev zlib1g-dev pkg-config libnfs-dev
sudo apt-get install qt6-base-dev qtcreator libxkbcommon-dev

# RHEL/CentOS
sudo yum install libssh-devel libzstd-devel zlib-devel pkgconfig libnfs-devel
sudo yum install qt6-qtbase-devel qt6-qtcreator libxkbcommon-devel
```

**External Libraries:**
- **Qt6**: GUI framework (Core, Widgets, Concurrent)
- **OpenSSL**: Cryptographic operations and SSH support
- **ZSTD**: Compression library
- **libssh**: SSH/SFTP client functionality
- **libnfs**: NFS client support
- **libcron**: Cron expression parsing and scheduling
- **nlohmann/json**: JSON parsing and serialization

### 2.2 Build Process

#### 2.2.1 CMake Configuration
```cmake
cmake_minimum_required(VERSION 3.24...4.0.2)
project(HPE_Backup_Recovery_System VERSION 0.1)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
add_compile_options(-O3 -Wall)
```

#### 2.2.2 Build Instructions
```bash
# Clone repository
git clone <repository-url>
cd Linux-Backup-and-Recovery

# Configure and build
mkdir build && cd build
cmake ..
make

# Alternative: Use automated build script
chmod +x run.sh
sudo ./run.sh
```

#### 2.2.3 Build Targets
- **Main Application**: CLI and GUI combined executable (`main`)
- **Scheduler Service**: Standalone scheduling daemon (`scheduler`)
- **Utils Library**: Shared utility functions (`utils`)

---

## 3. System Functionality

### 3.1 Backup Types

ResilioZ supports three primary backup strategies:

#### 3.1.1 Full Backup
- Complete filesystem snapshot
- Baseline for incremental and differential backups
- Stores all files regardless of previous backup state

#### 3.1.2 Incremental Backup
- Stores only changes since the last backup (any type)
- Requires previous backup metadata
- Optimized for storage efficiency

#### 3.1.3 Differential Backup
- Stores changes since the last full backup
- Requires full backup as baseline
- Balance between storage efficiency and restore speed

### 3.2 Content-Defined Chunking

The system implements content-defined chunking for data deduplication:

#### 3.2.1 Chunking Algorithm
```cpp
struct Chunk {
  std::vector<uint8_t> data;
  size_t size;
  std::string hash;  // SHA-256 hash
};
```

#### 3.2.2 Chunking Process
- **Variable Chunk Sizes**: Configurable average size (default: 1MB)
- **Hash-Based Identification**: SHA-256 cryptographic hashing
- **Deduplication**: Only unique chunks stored in repository
- **Streaming Support**: Memory-efficient processing for large files

### 3.3 Compression

#### 3.3.1 Zstandard Integration
```cpp
Chunk Backup::CompressChunk(const Chunk& original_chunk) {
  size_t compressed_size = ZSTD_compressBound(original_chunk.size);
  std::vector<uint8_t> compressed_data(compressed_size);
  
  size_t actual_size = ZSTD_compress(compressed_data.data(), compressed_size,
                                    original_chunk.data.data(), original_chunk.size,
                                    ZSTD_CLEVEL_DEFAULT);
}
```

#### 3.3.2 Compression Levels
- **Level 1**: Fastest compression, lower ratio
- **Level 3**: Balanced speed and compression
- **Level 6**: Default setting, optimal balance
- **Level 9**: Maximum compression, slower speed

---

## 4. Repository Management

### 4.1 Repository Types

#### 4.1.1 Local Repository
```cpp
class LocalRepository : public Repository {
  // Direct filesystem access
  // Password-protected storage
  // Local path configuration
};
```

**Features:**
- Direct filesystem access
- Password protection with SHA-256 hashing
- Local path configuration
- Automatic directory structure creation

#### 4.1.2 NFS Repository
```cpp
class NFSRepository : public Repository {
  // Network File System support
  // Server IP and export path configuration
  // libnfs integration
};
```

**Features:**
- Network File System (NFS) support
- Server IP and export path configuration
- libnfs library integration
- Automatic mount point management

#### 4.1.3 Remote Repository
```cpp
class RemoteRepository : public Repository {
  // SSH/SFTP support
  // User@host:path format
  // libssh integration
};
```

**Features:**
- SSH/SFTP protocol support
- User@host:path connection format
- libssh library integration
- Public key authentication support

### 4.2 Repository Structure

All repository types follow a consistent structure:

```
repository/
├── backup/                    # Backup metadata
│   ├── backup_20241201_120000.json
│   └── backup_20241202_120000.json
├── chunks/                    # Compressed data chunks
│   ├── 00/                    # Hash-based subdirectories
│   ├── 01/
│   └── ff/
└── config.json               # Repository configuration
```

### 4.3 Repository Configuration

#### 4.3.1 Configuration Format
```json
{
  "name": "repository_name",
  "type": "local|nfs|remote",
  "path": "repository_path",
  "created_at": "2024-12-01T12:00:00Z",
  "password_hash": "sha256_hash"
}
```

#### 4.3.2 Password Security
- SHA-256 password hashing
- Secure credential storage
- Repository-level access control

---

## 5. SSH Setup and Configuration

### 5.1 SSH Client Setup

#### 5.1.1 Key Generation
```bash
# Generate SSH key pair
ssh-keygen -t rsa -b 4096 -C "resilioz@example.com"

# Copy public key to remote server
ssh-copy-id user@remote-host
```

#### 5.1.2 SSH Configuration
```bash
# SSH config file (~/.ssh/config)
Host backup-server
    HostName remote-host.example.com
    User backup-user
    IdentityFile ~/.ssh/id_rsa
    Port 22
```

### 5.2 SSH Server Setup

#### 5.2.1 Server Configuration
```bash
# Install SSH server
sudo apt-get install openssh-server

# Configure SSH daemon
sudo nano /etc/ssh/sshd_config

# Key settings for backup operations
PubkeyAuthentication yes
PasswordAuthentication no
PermitRootLogin no
```

#### 5.2.2 User Setup
```bash
# Create backup user
sudo useradd -m -s /bin/bash backup-user

# Set up backup directory
sudo mkdir -p /home/backup-user/backups
sudo chown backup-user:backup-user /home/backup-user/backups
```

### 5.3 Repository Path Configuration

#### 5.3.1 Remote Repository Format
```
user@host:/path/to/backup/directory
```

**Example:**
```
backup-user@192.168.1.100:/home/backup-user/backups
```

#### 5.3.2 NFS Repository Format
```
server-ip:/export/path
```

**Example:**
```
192.168.1.100:/backups
```

---

## 6. NFS Mount Configuration

### 6.1 NFS Server Setup

#### 6.1.1 Server Installation
```bash
# Install NFS server
sudo apt-get install nfs-kernel-server

# Create export directory
sudo mkdir -p /backups
sudo chown nobody:nogroup /backups
```

#### 6.1.2 Export Configuration
```bash
# Edit exports file
sudo nano /etc/exports

# Add export entry
/backups 192.168.1.0/24(rw,sync,no_subtree_check)
```

#### 6.1.3 Start NFS Service
```bash
# Export file systems
sudo exportfs -a

# Start NFS service
sudo systemctl start nfs-kernel-server
sudo systemctl enable nfs-kernel-server
```

### 6.2 NFS Client Setup

#### 6.2.1 Client Installation
```bash
# Install NFS client
sudo apt-get install nfs-common
```

#### 6.2.2 Mount Configuration
```bash
# Manual mount
sudo mount 192.168.1.100:/backups /mnt/backups

# Permanent mount (/etc/fstab)
192.168.1.100:/backups /mnt/backups nfs defaults 0 0
```

---

## 7. System Components

### 7.1 Backup System

#### 7.1.1 Core Functionality
```cpp
class BackupSystem {
public:
  void CreateBackup();
  void ListBackups();
  void CompareBackups();
  void ScheduleBackup();
};
```

#### 7.1.2 Backup Process
1. **Repository Selection**: Choose or create repository
2. **Source Path**: Specify backup source directory
3. **Backup Type**: Select full, incremental, or differential
4. **File Processing**: Content-defined chunking and compression
5. **Metadata Storage**: JSON-based backup metadata
6. **Scheduling**: Optional automated scheduling

### 7.2 Restore System

#### 7.2.1 Core Functionality
```cpp
class RestoreSystem {
public:
  void RestoreFromBackup();
  void ListBackups();
  void CompareBackups();
};
```

#### 7.2.2 Restore Process
1. **Repository Selection**: Choose source repository
2. **Backup Selection**: Select backup to restore
3. **Location Choice**: Original or custom location
4. **File Reconstruction**: Chunk retrieval and assembly
5. **Metadata Restoration**: File attributes and permissions

### 7.3 Services System

#### 7.3.1 Repository Service
- Repository creation and management
- Repository listing and selection
- Repository deletion and cleanup

#### 7.3.2 Scheduler Service
- Cron-based scheduling
- HTTP API integration
- Background task execution

---

## 8. User Interface

### 8.1 Command-Line Interface

#### 8.1.1 CLI Structure
```
ResilioZ (HPE) - Backup and Recovery System in Linux...

=== SYSTEMS ===
    Backup System
    Restore System
    Others (Services System)
    EXIT...
```

#### 8.1.2 Navigation
- Arrow key navigation
- Enter key selection
- ESC key to return
- Hierarchical menu system

### 8.2 Graphical User Interface

#### 8.2.1 Qt6 Integration
```cpp
int RunGUI(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
```

#### 8.2.2 GUI Components
- **Main Window**: Primary application interface
- **Backup Tab**: Backup configuration and execution
- **Restore Tab**: Restore operations and file selection
- **Services Tab**: Repository and schedule management

---

## 9. Metadata Management

### 9.1 Backup Metadata

#### 9.1.1 Metadata Structure
```cpp
struct BackupMetadata {
  BackupType type;
  std::chrono::system_clock::time_point timestamp;
  std::string original_path;
  std::string previous_backup;
  std::string remarks;
  std::map<std::string, FileMetadata> files;
};
```

#### 9.1.2 File Metadata
```cpp
struct FileMetadata {
  std::string original_filename;
  std::vector<std::string> chunk_hashes;
  uint64_t total_size;
  fs::file_time_type mtime;
  bool is_symlink = false;
  std::string symlink_target;
};
```

### 9.2 Metadata Storage

#### 9.2.1 JSON Format
```json
{
  "type": 0,
  "timestamp": 1701446400,
  "original_path": "/path/to/source",
  "previous_backup": "backup_20241201_120000",
  "remarks": "Daily backup",
  "files": {
    "/path/to/file.txt": {
      "original_filename": "file.txt",
      "chunk_hashes": ["abc123", "def456"],
      "total_size": 1024,
      "mtime": 1701446400,
      "is_symlink": false
    }
  }
}
```

---

## 10. Logging and Error Handling

### 10.1 Logging System

#### 10.1.1 Log Levels
```cpp
enum class LogLevel { INFO, WARNING, ERROR };
```

#### 10.1.2 Log Storage
```
~/.logs/
└── sys.log    # System log file
```

#### 10.1.3 Log Format
```
2024-12-01T12:00:00Z - [INFO ] Repository: backup_repo created at location: /backups
2024-12-01T12:01:00Z - [INFO ] Starting backup of /path/to/file.txt (1024 bytes)
2024-12-01T12:01:01Z - [INFO ] Completed backup of /path/to/file.txt
```

### 10.2 Error Handling

#### 10.2.1 Error Categories
- **Validation Errors**: Input validation and parameter checking
- **File System Errors**: File access and permission issues
- **Network Errors**: Connection and transfer failures
- **Repository Errors**: Storage backend issues

#### 10.2.2 Error Recovery
- Automatic retry mechanisms
- Rollback support for failed operations
- Graceful degradation for partial failures
- User notification with recovery suggestions

---

## 11. Performance Characteristics

### 11.1 Compression Performance

The system uses Zstandard compression with configurable levels:

- **Level 1**: Fastest compression, suitable for real-time operations
- **Level 3**: Balanced speed and compression ratio
- **Level 6**: Default setting, optimal for most use cases
- **Level 9**: Maximum compression, suitable for archival storage

### 11.2 Deduplication Efficiency

Content-defined chunking provides deduplication benefits:

- **Source Code**: High deduplication due to repeated patterns
- **Virtual Machine Images**: Significant space savings
- **Database Backups**: Moderate deduplication depending on data type
- **System Logs**: Variable deduplication based on log content

### 11.3 Scalability

- **File Size Support**: Up to 2^64 bytes (16 exabytes)
- **Repository Size**: Limited only by storage capacity
- **Concurrent Operations**: Multi-threaded backup and restore
- **Network Optimization**: Streaming operations for large files

---

## 12. Security Features

### 12.1 Authentication

#### 12.1.1 Password Security
- SHA-256 password hashing for repository access
- Secure credential storage in configuration files
- Repository-level access control

#### 12.1.2 SSH Authentication
- Public key authentication for remote repositories
- SSH key management and validation
- Secure connection establishment

### 12.2 Data Protection

#### 12.2.1 Transport Security
- SSH/SFTP for remote repository access
- Encrypted communication channels
- Certificate validation

#### 12.2.2 Storage Security
- Repository-level access permissions
- Secure file permissions
- Isolated storage environments

---

## 13. Scheduling and Automation

### 13.1 Scheduler Architecture

#### 13.1.1 Scheduler Components
```cpp
class Scheduler {
private:
  libcron::Cron cron;                    // Cron expression parser
  std::map<std::string, std::string> schedules;  // Schedule metadata
  std::map<std::string, Repository*> repo_data;  // Repository instances
  struct sockaddr_in address;            // Network configuration
  int conn_id;                          // Connection counter
};
```

#### 13.1.2 HTTP API Endpoints
- **Add Schedule**: POST request to create new backup schedules
- **View Schedules**: GET request to list all configured schedules
- **Remove Schedule**: DELETE request to remove existing schedules

### 13.2 Automation Features

#### 13.2.1 Cron Integration
- Standard cron expression support
- Flexible scheduling options
- Background task execution

#### 13.2.2 Error Handling
- Automatic retry mechanisms
- Failure notification
- Comprehensive logging

---

## 14. Conclusion

ResilioZ represents a comprehensive backup and recovery solution designed for Linux environments. The system's architecture demonstrates solid engineering principles with:

### 14.1 Technical Strengths
- **Modular Design**: Clear separation of concerns across components
- **Multiple Storage Backends**: Support for local, NFS, and remote storage
- **Content-Defined Chunking**: Efficient deduplication through intelligent chunking
- **Compression Integration**: Zstandard compression for storage optimization
- **Dual Interface**: CLI and GUI for different user preferences

### 14.2 Enterprise Features
- **Repository Management**: Comprehensive repository lifecycle management
- **Scheduling Capabilities**: Automated backup scheduling with cron integration
- **Security Integration**: SSH/SFTP support with public key authentication
- **Error Handling**: Robust error handling and recovery mechanisms
- **Logging System**: Comprehensive logging for monitoring and debugging

### 14.3 Implementation Quality
- **C++17 Compliance**: Modern C++ features and best practices
- **Memory Management**: RAII principles and proper resource management
- **Exception Safety**: Strong exception safety guarantees
- **Cross-Platform Support**: Linux compatibility with multiple distributions

The system provides a solid foundation for enterprise data protection while maintaining flexibility for various deployment scenarios. The modular architecture allows for future enhancements and customizations to meet specific organizational requirements.

---

## Appendices

### Appendix A: Build Configuration
Complete CMake configuration and dependency management details.

### Appendix B: Repository Configuration
Detailed repository setup and configuration examples for all supported types.

### Appendix C: API Documentation
Complete HTTP API documentation for the scheduler service.

### Appendix D: Troubleshooting Guide
Common issues and resolution procedures for system deployment and operation.

---

**Document Version:** 2.0  
**Last Updated:** December 2024  
**Next Review:** March 2025 