#!/usr/bin/env python

import sys
import socket
import struct
import SitcpRbcp

def usage():
    print 'Usage: ./set-window-size.py ip_address'

def main():
    if len(sys.argv) != 2:
        usage()
        sys.exit(1)

    window_size = 29
    rbcp = SitcpRbcp.SitcpRbcp()
    rbcp.set_verify_mode()
    rbcp.set_timeout(2.0)
    ip_address = sys.argv[1]
    window_data = struct.pack('>B', window_size)

    try:
        rbcp.write_registers(ip_address, address = 0x06, length = len(window_data), id = 10, data = window_data)
    except socket.error, e:
        sys.exit(e)
    except Exception, e:
        sys.exit(e)
    else:
        print "window size data write done: %d (in register value)" % (window_size)

if __name__ == '__main__':
    main()
