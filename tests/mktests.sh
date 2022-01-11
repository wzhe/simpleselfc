#!/usr/bin/env bash

if [ ! -f ../ssc ]
then (cd ..; make)
fi

for i in $(ls input*c)
do if [ ! -f "out.$i" -a ! -f "err.$i" ]
   then
   echo "$i"
   ../ssc $i  2> "err.$i"
   if [ ! -s "err.$i" ]
   then
   rm -f "err.$i"
   cc -o out $i
   ./out > "out.$i"
   fi
   fi
   rm -f out out.s
done
