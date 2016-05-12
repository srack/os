#!/bin/bash

touch results.csv

increment=10
frames=2

./virtmem 100 $frames rand sort >> results.csv
./virtmem 100 $frames rand scan >> results.csv
./virtmem 100 $frames rand focus >> results.csv
./virtmem 100 $frames fifo sort >> results.csv
./virtmem 100 $frames fifo scan >> results.csv
./virtmem 100 $frames fifo focus >> results.csv
./virtmem 100 $frames custom sort >> results.csv
./virtmem 100 $frames custom scan >> results.csv
./virtmem 100 $frames custom focus >> results.csv

let frames=5

./virtmem 100 $frames rand sort >> results.csv
./virtmem 100 $frames rand scan >> results.csv
./virtmem 100 $frames rand focus >> results.csv
./virtmem 100 $frames fifo sort >> results.csv
./virtmem 100 $frames fifo scan >> results.csv
./virtmem 100 $frames fifo focus >> results.csv
./virtmem 100 $frames custom sort >> results.csv
./virtmem 100 $frames custom scan >> results.csv
./virtmem 100 $frames custom focus >> results.csv

let frames=10

while [ $frames -lt 101 ]
do
	./virtmem 100 $frames rand sort >> results.csv
	./virtmem 100 $frames rand scan >> results.csv
	./virtmem 100 $frames rand focus >> results.csv
	./virtmem 100 $frames fifo sort >> results.csv
	./virtmem 100 $frames fifo scan >> results.csv
	./virtmem 100 $frames fifo focus >> results.csv
	./virtmem 100 $frames custom sort >> results.csv
	./virtmem 100 $frames custom scan >> results.csv
	./virtmem 100 $frames custom focus >> results.csv

	let frames=$frames+$increment
done
