#ifndef BACKUP_BACKUP_H_
#define BACKUP_BACKUP_H_

#include <string>

namespace backup {

    class Backup {
    public:
    virtual ~Backup() = default;

    virtual void PerformBackup(const std::string& source_directory_path, const std::string& destination_path)  = 0;



    };
    
}       // namespace backup

#endif // BACKUP_BACKUP_H_