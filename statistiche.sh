#!/usr/bin/env bash

LOG_FILE=$1

echo
echo "Analyzing '$LOG_FILE' for server stats..."

NUM_NEW_CONNECTIONS=`grep -r "Adding a new connection" $1 | wc -l`
NUM_LOCKS=`grep -r "New API request \\\`lockFile\\\`" $1 | wc -l`
NUM_UNLOCKS=`grep -r "New API request \\\`unlockFile\\\`" $1 | wc -l`
NUM_LOCKED_OPENS=`grep -r "New API request \\\`openFile\\\`, create = 1, lock = 1" $1 | wc -l`
NUM_OPENS=`grep -r "New API request \\\`openFile\\\`, create = 1, lock = 0." $1 | wc -l`
NUM_CLOSES=`grep -r "New API request \\\`closeFile\\\`" $1 | wc -l`
MAX_NUM_CONCURRENT_CONNECTIONS='0'
NUM_CONNECTIONS=`grep -r "Adding a new connection" $1 | wc -l`
NUM_EVICTIONS='0'
NUM_EVICTED_FILES='0'

while read LINE; do
	if [[ $LINE == *"evicted files"* ]]; then
		NUM_EVICTIONS=$[$NUM_EVICTIONS + 1]
		NUM_FILES=`grep -oP '(?<=over )[0-9]+(?= evicted files)' <<< $LINE`
		NUM_EVICTED_FILES=$[$NUM_EVICTED_FILES + $NUM_FILES]
	elif [[ $LINE == *"New iteration in the polling loop"* ]]; then
		NUM=`grep -oP '(?<=with )[0-9]+(?= connection)' <<< $LINE`
		if (( NUM > MAX_NUM_CONCURRENT_CONNECTIONS )); then
			MAX_NUM_CONCURRENT_CONNECTIONS=$[$NUM]
		fi
	fi
done < "$LOG_FILE"

echo "Num. of new connections: $NUM_CONNECTIONS"
echo "Max. number of concurrent connections: $MAX_NUM_CONCURRENT_CONNECTIONS"
echo "Num. of lock operations: $NUM_LOCKS"
echo "Num. of unlock operations: $NUM_UNLOCKS"
echo "Num. of \"locked open\" operations: $NUM_LOCKED_OPENS"
echo "Num. of open operations: $NUM_OPENS"
echo "Num. of close operations: $NUM_CLOSES"
echo "Num. of times the eviction algorithm ran: $NUM_EVICTIONS"
echo "Num. of evicted files: $NUM_EVICTED_FILES"

exit 0