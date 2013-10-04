#!/usr/bin/env python

import sys
import socket
import SitcpRbcp

def usage():
    print 'usage: ./get-packet-id.py ip_address'

if len(sys.argv) < 2:
    usage()
    sys.exit(1)

ip_address   = sys.argv[1]
base_address = 0x0a
length       = 3

# validate ip address
try:
    socket.inet_aton(ip_address)
except socket.error, e:
    sys.exit(e)

rbcp = SitcpRbcp.SitcpRbcp()
rbcp.set_timeout(1)
try:
    data = rbcp.read_registers(ip_address = ip_address, address = base_address, length = length)
except Exception, e:
    sys.exit(e)

print '%s packet_id: 0x' % (ip_address),
n_col = 16
col = 0
for s in data:
    print '%02x' % (ord(s)),
    col += 1
    if col % n_col == 0:
        print
