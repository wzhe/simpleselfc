#!/usr/bin/env bash

TARGET="../ssc"
if [ ! -f $TARGET ]
then echo "Need to build $TARGET first!"; exit 1
fi

for i in input*c
do if [ ! -f "out.$i" -a ! -f "err.$i" ]
   then echo "Can't run test on $i, no outputfile!"
   else if [ -f "out.$i" ]
	then
	    echo -n $i
	    $TARGET -o out $i
	    ./out > trail.$i
	    cmp -s "out.$i" "trail.$i"
	    if [ "$?" -eq "1" ]
	    then echo ": failed"
		 diff -c "out.$i" "trail.$i"
		 echo
	    else echo ": OK"
	    fi
	else if [ -f "err.$i" ]
	     then
		 echo -n $i
		 $TARGET -o out $i 2> "trail.$i"
		 cmp -s "err.$i" "trail.$i"
		 if [ "$?" -eq "1" ]
		 then echo ": failed"
		      diff -c "err.$i" "trail.$i"
		      echo
		 else echo ": OK"
		 fi
	     fi
	fi
   fi
   rm -f out out.s "trail.$i"
done
