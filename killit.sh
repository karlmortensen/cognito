#!/bin/bash
#killit.sh

sudo kill `ps -e | grep cognito | awk {'print $1'}`
sudo kill `ps -e | grep nodejs | awk {'print $1'}`
