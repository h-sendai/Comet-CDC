/*
SiTCP RBCP Read, Write function 

Usage: 

////////////////////////////////////////////////////////////////////////
// Read Registers
////////////////////////////////////////////////////////////////////////

SitcpRbcp rbcp;

int address = 0x10;
int length  = 4;
unsigned char buf[length];

if (rbcp.read_registers("192.168.0.16", address, length, buf) < 0) {
    std::cerr << "read_registers() fail" << std::endl;
}

read result is written in buf.
Return value: = 0 if success.
              < 0 if failure.

You may specify SiTCP RBCP header id as last argument:
read_registers("192.168.0.16", address, length, buf, 100);
Default id value is 1.

////////////////////////////////////////////////////////////////////////
// Write Registers
////////////////////////////////////////////////////////////////////////

SitcpRbcp rbcp;

int address = 0x20;
int length  = 4;
unsigned char buf[length];
// Prepare write contents
buf[0] = 0;
buf[1] = 1;
buf[2] = 2;
buf[3] = 3;
if (rbcp.write_registers("192.168.0.16", address, length, buf) < 0) {
    std::cerr << "write_registers() fail" << std::endl;
}
 
Return value: = 0 if success.
              < 0 if failure.

You may use set_verify_mode() to re-read the registers
and compare the original write contents.

rbcp.set_verify_mode();
rbcp.write_registers("192.168.0.16", address, length, buf);

Even if you do not specify set_verify_mode(), write_registers()
examine the Ack packet data (but this does not mean the write
was successfully done).

You may specify SiTCP RBCP header id as last argument:
write_registers("192.168.0.16", address, length, buf, 100);
Default id value is 1.
*/ 

#include <string>
#include <iostream>

#include <err.h>
#include <daqmw/Sock.h>

struct sitcp_rbcp_header {
    unsigned char ver_type;
    unsigned char cmd_flag;
    unsigned char id;
    unsigned char length;
    unsigned int  address;
};

class SitcpRbcp {

public:
    SitcpRbcp() :
    m_need_verify(0) {}
    ~SitcpRbcp() {}
    int read_registers(
        std::string ip_address,
        int address,
        int length,
        unsigned char *buf,
        int id = 1
    );
    int write_registers(
        std::string ip_address,
        int address,
        int length,
        unsigned char *buf,
        int id = 1
    );
    int set_verify_mode();
    int unset_verify_mode();

private:
    DAQMW::Sock m_sock;
    int pack_sitcp_rbcp_header(unsigned char *buf, struct sitcp_rbcp_header *header);
    int unpack_sitcp_rbcp_header(unsigned char *buf, struct sitcp_rbcp_header *header);
    int print_packet_error_message(int ret, std::string function_name, std::string ip_address);
    int send_recv_command_packet(int command, std::string ip_address, int address, int length, unsigned char *buf, int id);
    int m_need_verify;
    const static int SITCP_RBCP_HEADER_LEN    = 8;
    const static unsigned char ACK_MASK       = 0x08;
    const static unsigned char BUS_ERROR_MASK = 0x01;
    const static int SITCP_RBCP_PORT          = 4660;
    const static int READ                     = 1;
    const static int WRITE                    = 2;

};

int SitcpRbcp::set_verify_mode()
{
    m_need_verify = 1;
    return 0;
}

int SitcpRbcp::unset_verify_mode()
{
    m_need_verify = 0;
    return 0;
}

int SitcpRbcp::pack_sitcp_rbcp_header(unsigned char *buf, struct sitcp_rbcp_header *header)
{
    memcpy(&buf[0], &header->ver_type, sizeof(unsigned char));
    memcpy(&buf[1], &header->cmd_flag, sizeof(unsigned char));
    memcpy(&buf[2], &header->id,       sizeof(unsigned char));
    memcpy(&buf[3], &header->length,   sizeof(unsigned char));
    memcpy(&buf[4], &header->address,  sizeof(unsigned int ));

    return 0;
}

int SitcpRbcp::unpack_sitcp_rbcp_header(unsigned char *buf, struct sitcp_rbcp_header *header)
{
    header->ver_type = buf[0];
    header->cmd_flag = buf[1];
    header->id       = buf[2];
    header->length   = buf[3];
    unsigned int address;
    memcpy(&address, &buf[4], sizeof(unsigned int));
    header->address = ntohl(address);

    return 0;
}

int SitcpRbcp::print_packet_error_message(int ret, std::string function_name, std::string ip_address)
{
    if (ret == DAQMW::Sock::ERROR_TIMEOUT) {
        std::cerr << function_name << ": timeout on " << ip_address << std::endl;
    }
    else if (ret == DAQMW::Sock::ERROR_FATAL) {
        std::cerr << function_name << ": fatal error on " << ip_address << std::endl;
    }
    else {
        std::cerr << function_name << ": unknonw error on " << ip_address << std::endl;
    }
    
    return 0;
}

int SitcpRbcp::send_recv_command_packet(int command, std::string ip_address, int address, int length, unsigned char *buf, int id)
{
    int ret;
    struct sitcp_rbcp_header send_header, reply_header;
    std::string function_name;
    
    send_header.ver_type = (char) 0xff;
    if (command == READ) {
        send_header.cmd_flag = (char) 0xc0;
        function_name = "read_registers()";
    }
    else if (command == WRITE) {
        send_header.cmd_flag = (char) 0x80;
        function_name = "write_registers()";
    }
    else {
        std::cerr << "Unknown command" << std::endl;
    }

    send_header.id       = (char) id;
    send_header.length   = (char) length;
    send_header.address  = htonl(address);

    DAQMW::Sock m_sock(ip_address, SITCP_RBCP_PORT);
    m_sock.connectUDP();
    
    int send_buf_len = SITCP_RBCP_HEADER_LEN;
    if (command == WRITE) {
        send_buf_len += length;
    }
    unsigned char send_buf[send_buf_len];
    pack_sitcp_rbcp_header(send_buf, &send_header);
    if (command == WRITE) {
        memcpy(&send_buf[SITCP_RBCP_HEADER_LEN], buf, length);
    }

    ret = m_sock.writeTo(send_buf, send_buf_len);
    if (ret < 0) {
        print_packet_error_message(ret, function_name, ip_address);
        return -1;
    }

    int recv_buf_len = SITCP_RBCP_HEADER_LEN + length;
    unsigned char recv_buf[recv_buf_len];
    ret = m_sock.readFrom(recv_buf, recv_buf_len);
    if (ret < 0) {
        print_packet_error_message(ret, function_name, ip_address);
        return -1;
    }

    unpack_sitcp_rbcp_header(recv_buf, &reply_header);
    if ((reply_header.cmd_flag & BUS_ERROR_MASK) == BUS_ERROR_MASK) {
        std::cerr << "read_registers(): BUS Error on SiTCP equipment"
                  << std::endl;
        return -1;
    }
    if ((reply_header.cmd_flag & ACK_MASK) != ACK_MASK) {
        std::cerr << function_name
                  << ": Got reply packet, "
                  << "but ACK bit was not set"
                  << std::endl;
        return -1;
    }
    if (reply_header.id != send_header.id) {
        std::cerr << function_name 
                  << ": Got reply packet, "
                  << "but id did not match with send value. " 
                  << "send id: "  << send_header.id
                  << "reply id: " << reply_header.id
                  << std::endl;
        return -1;
    }
    if (reply_header.length != send_header.length) {
        std::cerr << function_name
                  << ": Got reply packet, "
                  << "but the length does not match with request length. "
                  << "send length: "  << send_header.length
                  << "reply length: " << reply_header.length
                  << std::endl;
    }

    if (command == READ) {
        memcpy(buf, &recv_buf[SITCP_RBCP_HEADER_LEN], length);
    }
    /* verify ack packet */
    if (command == WRITE) {
        for (int i = 0; i < length; i++) {
            if (buf[i] != recv_buf[SITCP_RBCP_HEADER_LEN + i]) {
                std::cerr << std::hex << std::showbase;
                std::cerr << function_name << ": Ack packet data is not same. "
                          "send: " << buf[i] << ", ack packet: " << recv_buf[SITCP_RBCP_HEADER_LEN + i]
                          << std::endl;
                std::cerr << std::dec;
                return -1;
            }
        }
    }

    return 0;
}

int SitcpRbcp::read_registers(std::string ip_address, int address, int length, unsigned char *buf, int id)
{
    int ret;

    ret = send_recv_command_packet(READ, ip_address, address, length, buf, id);
    
    return ret;
}

int SitcpRbcp::write_registers(std::string ip_address, int address, int length, unsigned char *buf, int id)
{
    int ret;

    ret = send_recv_command_packet(WRITE, ip_address, address, length, buf, id);
    if (ret < 0) {
        return ret;
    }
    
    if (m_need_verify == 1) {
        unsigned char re_read_buf[length];
        ret = send_recv_command_packet(READ, ip_address, address, length, re_read_buf, id);
        if (ret < 0) {
            std::cerr << "read for verification fail" << std::endl;
            return -1;
        }
        for (int i = 0; i < length; i++) {
            if (buf[i] != re_read_buf[i]) {
                std::cerr << std::hex << std::showbase;
                std::cerr << "write_registers(): re read fail. try to write " << buf[i] << ", but set " << re_read_buf[i] << std::endl;
                std::cerr << std::dec;
                ret = -1;
            }
        }
    }
    
    return ret;
}
