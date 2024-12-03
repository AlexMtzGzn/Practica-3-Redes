#!/bin/bash
# Configurar bridge
ip link add name br0 type bridge
ip link set dev eth0 master br0
ip link set dev eth1 master br0
ip link set dev eth2 master br0
ip link set dev eth3 master br0
ip link set dev eth4 master br0
ip link set dev br0 up