#!/usr/bin/env bash

TARGET="../comp1"
if [ ! -f $TARGET ]
then echo "Need to build $TARGET first!"; exit 1
fi

for i in input*
do if [ ! -f "out.$i" ]
   then echo "Can't run test on $i, no outputfile!"
   else
       echo -n $i
       $TARGET $i
       cc -o out out.s ../lib/printint.c
       ./out > trail.$i
       cmp -s "out.$i" "trail.$i"
       if [ "$?" -eq "1" ]
       then echo ": failed"
            diff -c "out.$i" "trail.$i"
            echo
       else echo ": OK"
       fi
       rm -f out out.s "trail.$i"
   fi
done
