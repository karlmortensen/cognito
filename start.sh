#!/bin/bash
#start.sh

sudo ./killit.sh

#./build.sh
nodejs nodeUdpListener.js &
sudo ./cognito &
