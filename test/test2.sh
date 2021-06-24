#!/usr/bin/env bash

# Hide output! We only care about clients.
./server test/test2-config.toml > /dev/null 2>&1 &
SERVER_PID=$!

echo "Server PID is $SERVER_PID."
echo "Launching clients."

./client -f /tmp/LSOfiletorage.sk -p -W Makefile,README.md,.clang-format
sleep 2

kill -s SIGHUP $SERVER_PID

exit 0
