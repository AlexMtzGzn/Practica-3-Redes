#!/bin/bash
# Habilitar las interfaces de red
ifconfig eth0 up
ifconfig eth1 up

# Crear un puente entre las interfaces
brctl addbr bridge0         # Crear el puente llamado 'bridge0'
brctl addif bridge0 eth0    # Agregar eth0 al puente
brctl addif bridge0 eth1    # Agregar eth1 al puente
ifconfig bridge0 up         # Habilitar el puente
