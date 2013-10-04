#!/usr/bin/env python

import sys
import socket
import SitcpRbcp

def usage():
    print 'usage: ./get-window-size.py ip_address'

if len(sys.argv) < 2:
    usage()
    sys.exit(1)

ip_address   = sys.argv[1]
base_address = 0x06
length       = 1

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

print '%s window size: 0x%02x (hex) %d (dec) in register value' % (ip_address, ord(data), ord(data))
