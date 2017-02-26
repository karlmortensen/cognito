#!/bin/bash
#build.sh

gcc wifiMonitor.cpp -o wifiMonitor -lwiringPi -lpthread
gcc udpListener.cpp -o udpListener -lstdc++

