#!/usr/bin/env bash

# Prints the help message.
./client -h
# Stores all files found in `test/data`.
./client -p -f /tmp/LSOfiletorage.sk -Z 200 -w test/data/text
# Read back all files.
./client -p -f /tmp/LSOfiletorage.sk -Z 200 -R -d test/data

exit 0
