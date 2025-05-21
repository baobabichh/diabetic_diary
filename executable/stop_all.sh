#!/bin/bash

current_dir=$(pwd)

stop_service() {
cd "$current_dir"
  local script_name=$1

  local pid=$(pgrep -f "./$script_name")

  if [ -n "$pid" ]; then
    echo "Stopping $script_name with PID: $pid"
    sudo kill "$pid"
  else
    echo "No running instance of $script_name found."
  fi
}

stop_service "ai_requester_service.sh"
stop_service "web_server.sh"
