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



cd ../services/ai_requester_service/build/
./ai_requester_service &

OTHER_PID=$!
wait $OTHER_PID