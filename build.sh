#!/bin/bash
#build.sh

gcc cognito.cpp -o cognito -lwiringPi -lpthread -lstdc++
make
