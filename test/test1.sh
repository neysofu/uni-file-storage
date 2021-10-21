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

# TEST -h

./client -h
echo ""

# TEST -W (single file, looping)

for file in $PARENT_PATH/data/Imgur/80s/*; do
   echo "$file"
   ./client -p warn -f /tmp/LSOfiletorage.sk -W "$file" -D "$PARENT_PATH/data/target/evicted" -z 1
done

echo "Written $(ls -1q $PARENT_PATH/data/Imgur/80s | wc -l) files."

for file in $PARENT_PATH/data/Imgur/80s/*; do
   echo "$file"
   ./client -p warn -f /tmp/LSOfiletorage.sk -r "$file" -d "$PARENT_PATH/data/target/80s" -z 1
done

echo "Got back $(ls -1q $PARENT_PATH/data/target/80s | wc -l) files."

exit 0
