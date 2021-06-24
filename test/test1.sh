#!/usr/bin/env bash

valgrind --leak-check=full ./server --test/test1-config.toml
SERVER_PID=$!

echo "Server PID is $SERVER_PID."

kill -s SIGHUP $SERVER_PID

exit 0
