#!/usr/bin/env python

import sys
import socket
import SitcpRbcp

def usage():
    print 'usage: regdump.py ip_address address length'

if len(sys.argv) < 4:
    usage()
    sys.exit(1)

ip_address   = sys.argv[1]
base_address = int(sys.argv[2], 0)
length       = int(sys.argv[3], 0)

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

n_col = 16
col = 0
for s in data:
    print '%02x' % (ord(s)),
    col += 1
    if col % n_col == 0:
        print
