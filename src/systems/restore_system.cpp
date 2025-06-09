#include "systems/restore_system.h"

#include "utils/utils.h"

RestoreSystem::RestoreSystem() {}

void RestoreSystem::Start() { Logger::Log("Starting Restore System..."); }

void RestoreSystem::Shutdown() {
  Logger::Log("Shutting down Restore System...");
}

void RestoreSystem::Log() {
  Logger::TerminalLog("Restore system is up and running... \n");
}
