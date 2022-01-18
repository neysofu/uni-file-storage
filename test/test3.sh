#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."
echo ""

./client -h

echo ""

loop_end=$((SECONDS+3))

while [ $SECONDS -lt $end ]; do
	./client -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/1 - cG6STRJ.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
	sleep 0.05
	:
done
