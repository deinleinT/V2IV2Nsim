#!/bin/sh

cd inet
make makefiles
make -j $(nproc) MODE=release
make -j $(nproc) MODE=debug
cd ..
cd veins
make makefiles
make -j $(nproc) MODE=release
make -j $(nproc) MODE=debug
cd ..
cd veins_inet3
make makefiles
make -j $(nproc) MODE=release
make -j $(nproc) MODE=debug
cd ..
cd lteNR
make makefiles
make -j $(nproc) MODE=release
make -j $(nproc) MODE=debug
cd ..

