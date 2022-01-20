#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."
echo ""

rm -rf "$PARENT_PATH/data/target"
mkdir -p "$PARENT_PATH/data/target/80s"
mkdir -p "$PARENT_PATH/data/target/Gangsta"
mkdir -p "$PARENT_PATH/data/target/Pokemon"
mkdir -p "$PARENT_PATH/data/target/Winter"
mkdir -p "$PARENT_PATH/data/target/evicted"
mkdir -p "$PARENT_PATH/data/target/others"

# TEST -h

./client -h
echo ""

# TEST -W (single file, looping)

for file in $PARENT_PATH/data/Imgur/80s/*; do
   echo "$file"
   ./client -p trace -f /tmp/LSOfiletorage.sk -W "$file" -D "$PARENT_PATH/data/target/evicted" -z 1
done

echo "Written $(ls -1q $PARENT_PATH/data/Imgur/80s | wc -l) files."

for file in $PARENT_PATH/data/Imgur/80s/*; do
   echo "$file"
   ./client -p trace -f /tmp/LSOfiletorage.sk -r "$file" -d "$PARENT_PATH/data/target/80s" -z 1
done

echo "Got back $(ls -1q $PARENT_PATH/data/target/80s | wc -l) files."

for file in $PARENT_PATH/data/Imgur/Pokemon/*; do
   echo "$file"
   ./client -p trace -f /tmp/LSOfiletorage.sk -W "$file" -D "$PARENT_PATH/data/target/evicted" -z 1
done

echo "Written $(ls -1q $PARENT_PATH/data/Imgur/Pokemon | wc -l) files."

for file in $PARENT_PATH/data/Imgur/Pokemon/*; do
   echo "$file"
   ./client -p trace -f /tmp/LSOfiletorage.sk -r "$file" -d "$PARENT_PATH/data/target/Pokemon" -z 1
done

echo "Got back $(ls -1q $PARENT_PATH/data/target/Pokemon | wc -l) files."

./client -p trace -f /tmp/LSOfiletorage.sk -w "$PARENT_PATH/data/Imgur/Winter,n=1" -D "$PARENT_PATH/data/target/evicted" -z 1
./client -p trace -f /tmp/LSOfiletorage.sk -R n=0 -d "$PARENT_PATH/data/target/others" -z 1

./client -p trace -f /tmp/LSOfiletorage.sk -w "$PARENT_PATH/data/Imgur/Gangsta,n=0" -D "$PARENT_PATH/data/target/evicted" -z 1
./client -p trace -f /tmp/LSOfiletorage.sk -w "$PARENT_PATH/data/Imgur/Nested,n=0" -D "$PARENT_PATH/data/target/evicted" -z 1

kill -s SIGHUP "$(head -n 1 server.pid)"
exit 0
