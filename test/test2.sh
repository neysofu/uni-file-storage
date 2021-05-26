#!/usr/bin/env bash

./server --test/test2-config.toml &
SERVER_PID=$!

echo $SERVER_PID

kill -s SIGHUP $SERVER_PID

exit 0
