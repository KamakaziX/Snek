#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   exit 1
fi

apt install -y libncurses5-dev libsdl2-dev libsdl2-mixer-dev cmake libboost-log-dev libboost-system-dev libboost-program-options-dev
