#!/bin/sh

cd inet
make makefiles
make -j $(nproc) MODE=release
cd ..
cd veins
make makefiles
make -j $(nproc) MODE=release
cd ..
cd veins_inet3
make makefiles
make -j $(nproc) MODE=release
cd ..
cd lteNR
make makefiles
make -j $(nproc) MODE=release
cd ..

