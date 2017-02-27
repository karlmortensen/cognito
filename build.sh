#!/bin/bash
#build.sh

gcc networkMonitor.cpp -o networkMonitor -lwiringPi -lpthread -lstdc++
