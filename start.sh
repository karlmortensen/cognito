#!/bin/bash
#start.sh

./build.sh
nodejs nodeUdpListener.js &
sudo ./cognito &