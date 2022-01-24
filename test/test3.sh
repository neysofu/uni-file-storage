#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."
echo ""

./client -h

echo ""

START=`date +%s`

while [ $(( $(date +%s) - 30 )) -lt $START ]; do
	./client -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/1 - cG6STRJ.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
	sleep 0.05
	:
done

kill -s SIGINT "$(head -n 1 server.pid)"
./statistiche.sh server.log

exit 0
