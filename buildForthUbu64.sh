#!/bin/bash
pwd

make -C ForthLib -f Makefile.ubu64 all
rm ForthLinux/forth
make -C ForthLinux -f Makefile.ubu64 all

