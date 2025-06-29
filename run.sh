#!/bin/bash
mkdir -p build

# Clear Previous Build Targets
cmake --build build --target clean

# Build with Options
cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel $(($(nproc) - 1)) --target all # Parallelize make for faster builds


# Config for scheduler as systemd
SERVICE_NAME="resilioz-backup-scheduler.service" #For systemd commands
UNIT_FILE="/etc/systemd/system/$SERVICE_NAME"
EXEC_PATH="/usr/local/bin/scheduler"
BUILD_PATH="./build/scheduler"

## IMPORTANT - Change this is you want logs to go to sys.log, currenly scheduler logs are a seperate file
LOG_FILE="/var/log/scheduler.log" 

echo "Checking if $SERVICE_NAME is already running..."
if systemctl is-active --quiet "$SERVICE_NAME"; then
  echo "$SERVICE_NAME is already running. Skipping binary copy."
else
  echo "Moving scheduler binary to $EXEC_PATH..."
  if [[ ! -f "$BUILD_PATH" ]]; then
    echo "Error: Scheduler binary not found at $BUILD_PATH"
    exit 1
  fi

  # Moving files
  sudo cp "$BUILD_PATH" "$EXEC_PATH"
  sudo chmod +x "$EXEC_PATH"
fi

# Systemd unit file creation
if [[ ! -f "$UNIT_FILE" ]]; then
  echo "Creating systemd unit file at $UNIT_FILE..."
  sudo tee "$UNIT_FILE" > /dev/null <<EOF
[Unit]
Description=ResilioZ Backup Scheduler
After=network.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=$EXEC_PATH
StandardOutput=append:$LOG_FILE
StandardError=inherit
Restart=on-failure
RestartSec=5s

TimeoutStopSec=15
KillMode=mixed

[Install]
WantedBy=multi-user.target
EOF
  sudo chmod 644 "$UNIT_FILE"
else
  echo "Systemd unit already exists. Skipping creation."
fi

echo "Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

echo "Enabling $SERVICE_NAME to run at boot..."
sudo systemctl enable "$SERVICE_NAME"

if systemctl is-active --quiet "$SERVICE_NAME"; then
  echo "$SERVICE_NAME is already running."
else
  echo "Starting $SERVICE_NAME..."
  sudo systemctl start "$SERVICE_NAME"
fi

echo "Done. Logs will be written to $LOG_FILE"

# Run the Binary
cd build

# CLI
sudo -E ./main --cli

# GUI
sudo -E ./main --gui
