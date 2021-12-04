#!/usr/bin/env bash

for i in input*
do if [ ! -f "out.$i" ]
   then
       cc -o out $i ../lib/printint.c
       ./out > out.$i
       rm -f out
   fi
done
