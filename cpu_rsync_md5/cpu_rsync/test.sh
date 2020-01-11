#!/bin/bash
for chunk_size in 64 128 256 512 1024 2048
do
    total=0
    for((i = 1; i <= 20; i++))
    do
    ./server -s $chunk_size 
      if [ $i -gt 10 ]
        then
        result=`./server -s $chunk_size | tr -cd [0-9]`
        let total+=result
        #echo "gpu time:" $total "threads num:" $t_num "chunk size:" $chunk_size
      fi
    done
    ((total/=10))
    echo "cpu time:" $total "chunk size:" $chunk_size
done
echo -n "time is:" date
