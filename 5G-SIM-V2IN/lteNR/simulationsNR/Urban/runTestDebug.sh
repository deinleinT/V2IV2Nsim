#!/bin/sh


./runDebug.sh VideoDL 0 omnetppMW.ini &
#PID1=$!

sleep 2

./runDebug.sh VideoDL 1 omnetppMW.ini &
#PID2=$!

sleep 2

./runDebug.sh VideoDL 2 omnetppMW.ini &
#PID3=$!

sleep 2


####
./runDebug.sh VideoUL 0 omnetppMW.ini &
#PID3=$!

sleep 2

./runDebug.sh VideoUL 1 omnetppMW.ini &
#PID4=$!

sleep 2

./runDebug.sh VideoUL 2 omnetppMW.ini &
#PID5=$!

sleep 2


####
./runDebug.sh DL4Applications 0 omnetppMW.ini &
#PID1=$!

sleep 2

./runDebug.sh DL4Applications 1 omnetppMW.ini &
#PID2=$!

sleep 2

./runDebug.sh DL4Applications 2 omnetppMW.ini &
#PID3=$!

sleep 2


./runDebug.sh UL4Applications 0 omnetppMW.ini &
#PID1=$!

sleep 2

./runDebug.sh UL4Applications 1 omnetppMW.ini &
#PID2=$!

sleep 2

./runDebug.sh UL4Applications 2 omnetppMW.ini &
#PID3=$!

sleep 2