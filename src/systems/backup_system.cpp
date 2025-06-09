#include "systems/backup_system.h"

#include "utils/utils.h"

BackupSystem::BackupSystem() {}

void BackupSystem::Start() { Logger::Log("Starting Backup System..."); }

void BackupSystem::Shutdown() { Logger::Log("Shutting down Backup System..."); }

void BackupSystem::Log() {
  Logger::TerminalLog("Backup system is up and running... \n");
}
