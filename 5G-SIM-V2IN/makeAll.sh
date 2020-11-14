#!/bin/sh

cd inet
make makefiles
make MODE=release -j16
make MODE=debug -j16
cd ..
cd veins
make makefiles
make MODE=release -j16
make MODE=debug -j16
cd ..
cd veins_inet3
make makefiles
make MODE=release -j16
make MODE=debug -j16
cd ..
cd lteNR
make makefiles
make MODE=release -j16
make MODE=debug -j16
cd ..

