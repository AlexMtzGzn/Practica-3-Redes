#!/bin/bash
# Configurar las interfaces para cada red
ip addr add 192.168.1.254/24 dev eth0
ip addr add 192.168.2.254/24 dev eth1
ip link set dev eth0 up
ip link set dev eth1 up

# Habilitar el forwarding de IP
echo 1 > /proc/sys/net/ipv4/ip_forward