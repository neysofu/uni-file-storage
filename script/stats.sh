#!/usr/bin/env bash

LOG_FILE=$1

NUM_NEW_CONNECTIONS=`grep -r "Adding a new connection" $1 | wc -l`
NUM_LOCKS=`grep -r "Lock" $1 | wc -l`
NUM_UNLOCKS=`grep -r "Unlock" $1 | wc -l`
NUM_CLOSES=`grep -r "Close" $1 | wc -l`
NUM_EVICTIONS=`grep -r "Evict" $1 | wc -l`

let MAX_NUM_CONNECTIONS=0
let NUM_CONNECTIONS=0

cat $1 | while read LINE 
do
	if [[ $LINE == *"Adding a new connection"* ]]; then
		let NUM_CONNECTIONS++
		if (( NUM_CONNECTIONS > MAX_NUM_CONNECTIONS )); then
			let MAX_NUM_CONNECTIONS=NUM_CONNECTIONS
		fi
	else
		let NUM_CONNECTIONS--
	fi
done

echo "Num. of new connections: $NUM_NEW_CONNECTIONS"
echo "Max. number of concurrent connections: $NUM_NEW_CONNECTIONS"
echo "Num. of lock operatoins: $NUM_LOCKS"
echo "Num. of unlock operatoins: $NUM_UNLOCKS"
echo "Num. of close operatoins: $NUM_CLOSES"
echo "Num. of evictions: $NUM_EVICTIONS"

exit 0