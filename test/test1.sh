#!/usr/bin/env bash

PARENT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P)
echo "The parent path of this test is $PARENT_PATH."

./client -h

for file in "$PARENT_PATH/data/Imgur/80s/*"; do
   ./client -p -f /tmp/LSOfiletorage.sk -w $file -z 1
done

for file in "$PARENT_PATH/data/Imgur/80s/*"; do
   ./client -p -f /tmp/LSOfiletorage.sk -r $file -d $PARENT_PATH/data/target/ -z 1
done

find "data/target/80s" -type f -exec md5sum {} + | sort -k 2 > $PARENT_PATH/data/target/md5.txt
find "data/Imgur/80s" -type f -exec md5sum {} + | sort -k 2 > $PARENT_PATH/data/target/original.txt

# Compare the original directory and the one we got from the server's contents.
diff -u $PARENT_PATH/data/target/md5.txt $PARENT_PATH/data/target/original.txt

exit 0
