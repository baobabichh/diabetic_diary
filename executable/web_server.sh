#!/bin/bash

OTHER_PID=0

cleanup() {
  echo "SIGTERM received. Performing cleanup..."
  kill $OTHER_PID
  kill 0
  exit 0
}
trap cleanup SIGTERM SIGINT

current_dir=$(pwd)
export ex_cfg_path="$current_dir/../credentials.json"

cd ../services/web_server/build/
./web_server &

OTHER_PID=$!
wait $OTHER_PID