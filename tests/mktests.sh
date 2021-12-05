#!/usr/bin/env bash

if [ ! -f ../comp1 ]
then (cd ..; mke)
fi

for i in input*c
do if [ ! -f "out.$i" -a ! -f "err.$i" ]
   then
       echo -n $i
       ../comp1 $i  2> "err.$i"
       if [ ! -s "err.$i" ]
       then
	   rm -f "err.$i"
	   cc -o out $i ../lib/printint.c
	   ./out > "out.$i"
       fi
   fi
   rm -f out out.s
done
