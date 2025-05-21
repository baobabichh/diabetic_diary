#!/bin/bash
current_dir=$(pwd)

start_service() {
cd "$current_dir"
  local script_name=$1
  local log_file="${script_name%.*}.log"

  [ -f "$log_file" ] && rm "$log_file"
  nohup ./"$script_name" > "$log_file" 2>&1 &
  echo $!
}

start_service "ai_requester_service.sh"
start_service "web_server.sh"