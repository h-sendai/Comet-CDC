#!/usr/bin/env python

import sys
import socket
import struct
import SitcpRbcp

def usage():
    print 'Usage: ./set-packet-id.py ip_address module_num'

def main():
    if (len(sys.argv) < 3):
        usage()
        sys.exit(1)

    rbcp = SitcpRbcp.SitcpRbcp()
    rbcp.set_verify_mode()
    rbcp.set_timeout(2.0)
    ip_address = sys.argv[1]
    module_num = int(sys.argv[2])
    packet_id_data = struct.pack('>BBB', 0x00, 0x00, module_num)

    try:
        rbcp.write_registers(ip_address, address = 0x0a, length = len(packet_id_data), id = 10, data = packet_id_data)
    except socket.error, e:
        sys.exit(e)
    except Exception, e:
        sys.exit(e)
    else:
        print '%s packet_id: 0x %02x %02x %02x' % (ip_address, 0, 0, module_num)

if __name__ == '__main__':
    main()
