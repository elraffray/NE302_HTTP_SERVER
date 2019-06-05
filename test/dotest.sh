#!/bin/sh
IP=127.0.0.1
PORT=8080

if [ ! -d results ] ; then
	echo "results directory does not exist."
	exit 1
fi

if [ ! -d tests ] ; then
	echo "tests directory does not exist."
	exit 1
fi

for f in $(find ./tests -type f) ; do
	echo "Input file for test $f"
	res="./results/$(basename $f)"
	echo "./test -p $PORT -s $IP -i $f -o $res"
	./test -p $PORT -s $IP -i $f -o $res
done
