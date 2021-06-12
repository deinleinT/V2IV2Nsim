#!/bin/sh


./run.sh VideoDL 0 omnetppUR.ini &
#PID1=$!

sleep 2

./run.sh VideoDL 1 omnetppUR.ini &
#PID2=$!

sleep 2

./run.sh VideoDL 2 omnetppUR.ini &
#PID3=$!

sleep 2


####
./run.sh VideoUL 0 omnetppUR.ini &
#PID3=$!

sleep 2

./run.sh VideoUL 1 omnetppUR.ini &
#PID4=$!

sleep 2

./run.sh VideoUL 2 omnetppUR.ini &
#PID5=$!

sleep 2


####
./run.sh DL4Applications 0 omnetppUR.ini &
#PID1=$!

sleep 2

./run.sh DL4Applications 1 omnetppUR.ini &
#PID2=$!

sleep 2

./run.sh DL4Applications 2 omnetppUR.ini &
#PID3=$!

sleep 2


./run.sh UL4Applications 0 omnetppUR.ini &
#PID1=$!

sleep 2

./run.sh UL4Applications 1 omnetppUR.ini &
#PID2=$!

sleep 2

./run.sh UL4Applications 2 omnetppUR.ini &
#PID3=$!

sleep 2