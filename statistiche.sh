#!/usr/bin/env bash

LOG_FILE=$1

echo
echo "Analyzing '$LOG_FILE' for server stats..."

NUM_NEW_CONNECTIONS=`grep -r "Adding a new connection" $1 | wc -l`
NUM_LOCKS=`grep -r "New API request \\\`lockFile\\\`" $1 | wc -l`
NUM_UNLOCKS=`grep -r "New API request \\\`unlockFile\\\`" $1 | wc -l`
NUM_LOCKED_OPENS=`grep -r "New API request \\\`openFile\\\`, create = 1, lock = 1" $1 | wc -l`
NUM_READS=`grep -r "New API request \\\`read" $1 | wc -l`
NUM_WRITES=`grep -r "New API request \\\`writeFile\\\`" $1 | wc -l`
NUM_OPENS=`grep -r "New API request \\\`openFile\\\`, create = 1, lock = 0." $1 | wc -l`
NUM_CLOSES=`grep -r "New API request \\\`closeFile\\\`" $1 | wc -l`
NUM_BYTES_READ='0'
NUM_OPERATIONS_BY_WORKER_ID=(0)
NUM_BYTES_WRITTEN='0'
MAX_NUM_CONCURRENT_CONNECTIONS='0'
NUM_CONNECTIONS=`grep -r "Adding a new connection" $1 | wc -l`
NUM_EVICTIONS='0'
NUM_EVICTED_FILES='0'
NUM_WORKERS='0'

while read LINE; do
	if [[ $LINE == *"The latest message is"* ]]; then
		WORKER_ID=`grep -oP '(?<=n\.)[0-9]+(?=\])' <<< $LINE`
		# FIXME
		if [ "${WORKER_ID}" -ge "${NUM_WORKERS}" ]; then
			NUM_OPERATIONS_BY_WORKER_ID+=(1)
			let NUM_WORKERS+=1
		else
			NUM_OPERATIONS_BY_WORKER_ID[$WORKER_ID]=$(( $WORKER_ID+1 ))
		fi
	elif [[ $LINE == *"This read operation consists of"* ]]; then
		NUM_BYTES=`grep -oP '(?<=consists of )[0-9]+(?= bytes)' <<< $LINE`
		NUM_BYTES_READ=$[$NUM_BYTES_READ + $NUM_BYTES]
	elif [[ $LINE == *"This write operation consists of"* ]]; then
		NUM_BYTES=`grep -oP '(?<=consists of )[0-9]+(?= bytes)' <<< $LINE`
		NUM_BYTES_WRITTEN=$[$NUM_BYTES_WRITTEN + $NUM_BYTES]
	elif [[ $LINE == *"evicted files"* ]]; then
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

if [ $NUM_READS -ne "0" ]; then
	NUM_BYTES_READ=$[$NUM_BYTES_READ / $NUM_READS]
fi

if [ $NUM_WRITES -ne "0" ]; then
	NUM_BYTES_WRITTEN=$[$NUM_BYTES_WRITTEN / $NUM_WRITES]
fi

echo "Num. of new connections: $NUM_CONNECTIONS"
echo "Num. of read operations: $NUM_READS"
echo "Num. of write operations: $NUM_WRITES"
echo "Num. of lock operations: $NUM_LOCKS"
echo "Num. of unlock operations: $NUM_UNLOCKS"
echo "Num. of \"locked open\" operations: $NUM_LOCKED_OPENS"
echo "Num. of open operations: $NUM_OPENS"
echo "Num. of close operations: $NUM_CLOSES"
echo "Num. of times the eviction algorithm ran: $NUM_EVICTIONS"
echo "Num. of evicted files: $NUM_EVICTED_FILES"
echo "Max. number of concurrent connections: $MAX_NUM_CONCURRENT_CONNECTIONS"
echo "Avg. size of read operations in bytes: $NUM_BYTES_READ"
echo "Avg. size of write operations in bytes: $NUM_BYTES_WRITTEN"

for (( i=0; i<${NUM_WORKERS}; i++ )); do
	echo "Num. of operations handled by worker n.$i: ${NUM_OPERATIONS_BY_WORKER_ID[$i]}"
done

exit 0