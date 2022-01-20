#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."
echo ""

rm -rf "$PARENT_PATH/data/target"
mkdir -p "$PARENT_PATH/data/target/evicted"

# 796Ki
./client -p trace -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/1 - cG6STRJ.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
echo "Evicted $(ls -1q $PARENT_PATH/data/target/evicted | wc -l) files (0 expected)."

# 856Ki
./client -p trace -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/2 - lHjYzzm.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
echo "Evicted $(ls -1q $PARENT_PATH/data/target/evicted | wc -l) files (1 expected)."

# 972Ki
./client -p trace -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/3 - Cpx97PW.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
echo "Evicted $(ls -1q $PARENT_PATH/data/target/evicted | wc -l) files (2 expected)."

# 410Ki
./client -p trace -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/5 - N1V9PES.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
echo "Evicted $(ls -1q $PARENT_PATH/data/target/evicted | wc -l) files (3 expected)."

# The last file should fit and not cause any evictions.
# 336Ki
./client -p trace -f /tmp/LSOfiletorage.sk -W "$PARENT_PATH/data/Imgur/80s/7 - L1KaXjH.jpg" -D "$PARENT_PATH/data/target/evicted" -z 1
echo "Evicted $(ls -1q $PARENT_PATH/data/target/evicted | wc -l) files (3 expected)."

kill -s SIGHUP "$(head -n 1 server.pid)"
exit 0
