# HPE Linux Backup and Recovery System

A comprehensive backup and recovery system tailored for Linux environments, developed to ensure data integrity, optimize storage, and minimize recovery times. This system offers automated scheduling, deduplication, compression, and optional encryption, providing a robust solution for secure and efficient data backup and restoration.

---

## Features

- **Backup Types:** Full, Incremental, and Differential backups to balance storage usage and backup duration.
- **Scheduling:** Automated backups using CRON for unattended, periodic backups.
- **Data Deduplication:** Efficient content-defined chunking (CDC) with Rabin Fingerprinting to eliminate redundant data.
- **Compression:** Zstandard (ZSTD) for fast and efficient data compression.
- **Encryption:** Optional AES-256 encryption to protect sensitive data during backup and transfer.
- **Integrity Checks:** Metadata-driven verification to ensure data consistency and reliability.
- **Cloud Integration:** Seamless synchronization with AWS, Azure, and GCP for off-site data storage.

---

## System Workflow

1. **Configuration:**
   - User configures backup preferences (Full, Incremental, Differential), scheduling options, and encryption settings via CLI.

2. **Data Preparation:**
   - The system scans and processes the data, generating metadata for incremental and differential backups.

3. **Deduplication:**
   - Data is split into chunks using CDC and Rabin Fingerprinting, and only unique chunks are stored.

4. **Compression:**
   - Data chunks are compressed using Zstandard (ZSTD) to reduce storage space.

5. **Encryption (Optional):**
   - Compressed data is encrypted using AES-256 for added security.

6. **Backup Storage:**
   - Data is stored locally or synchronized with cloud storage (AWS, Azure, GCP).

7. **Recovery:**
   - Data is restored using metadata to accurately reconstruct files, decrypting and decompressing as needed.

---

## Diagrams

### **Class Diagram:**
![ClassDiagram](https://github.com/user-attachments/assets/6a7e3326-154a-4e4e-9c30-ca38c749fbdf)


### **Backup and Recovery Workflow:**
![SystemWorkflow](https://github.com/user-attachments/assets/5b709981-ae1f-45be-9deb-576f25a976f2)

---

## Dependencies

- GCC or Clang (C/C++ Compiler)
- Zstandard (ZSTD) Library
- OpenSSL (for AES-256 encryption)
- SQLite (for metadata storage)
- AWS CLI / Azure CLI / GCP CLI (for cloud synchronization)


