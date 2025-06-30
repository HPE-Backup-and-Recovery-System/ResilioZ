#!/bin/bash

# === CONFIGURATION ===
SERVICE_NAME="resilioz-backup-scheduler.service"
UNIT_FILE="/etc/systemd/system/$SERVICE_NAME"
EXEC_PATH="/usr/local/bin/scheduler"

echo "Stopping $SERVICE_NAME if running..."
sudo systemctl stop "$SERVICE_NAME"

echo "Disabling $SERVICE_NAME from starting at boot..."
sudo systemctl disable "$SERVICE_NAME"

if [[ -f "$UNIT_FILE" ]]; then
  echo "Removing systemd unit file: $UNIT_FILE"
  sudo rm "$UNIT_FILE"
else
  echo "Systemd unit file not found at $UNIT_FILE (already removed?)"
fi

echo "Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

if [[ -f "$EXEC_PATH" ]]; then
  echo "Removing scheduler binary at $EXEC_PATH"
  sudo rm "$EXEC_PATH"
else
  echo "Scheduler binary not found at $EXEC_PATH"
fi

echo "Uninstall complete. Scheduler service has been removed."