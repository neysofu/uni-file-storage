#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."

mkdir $PARENT_PATH/data/target

./client -h

for file in "$PARENT_PATH/data/Imgur/80s/*"; do
   ./client -p debug -f /tmp/LSOfiletorage.sk -W $file
done

for file in "$PARENT_PATH/data/Imgur/80s/*"; do
   ./client -p debug -f /tmp/LSOfiletorage.sk -r $file -d $PARENT_PATH/data/target/ -z 1
done

find "$PARENT_PATH/data/target/80s" -type f -exec md5sum {} + | sort -k 2 > $PARENT_PATH/data/target/md5.txt
find "$PARENT_PATH/data/Imgur/80s" -type f -exec md5sum {} + | sort -k 2 > $PARENT_PATH/data/target/original.txt

# Compare the original directory and the one we got from the server's contents.
diff -u $PARENT_PATH/data/target/md5.txt $PARENT_PATH/data/target/original.txt

rm -rf $PARENT_PATH/data/target

exit 0
