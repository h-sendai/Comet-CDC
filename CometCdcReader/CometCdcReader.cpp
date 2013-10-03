// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "CometCdcReader.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;
using DAQMW::FatalType::USER_DEFINED_ERROR2;

// Module specification
// Change following items to suit your component's spec.
static const char* cometcdcreader_spec[] =
{
    "implementation_id", "CometCdcReader",
    "type_name",         "CometCdcReader",
    "description",       "CometCdcReader component",
    "version",           "1.0",
    "vendor",            "Kazuo Nakayoshi, KEK",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    ""
};

CometCdcReader::CometCdcReader(RTC::Manager* manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("cometcdcreader_out", m_out_data),
      m_sock(0),
      m_recv_byte_size(0),
      m_window_size(30),
      m_read_byte_size(0),
      m_epfd(-1),
      m_ev_ret(NULL),
      m_set_registers(true),
      m_out_status(BUF_SUCCESS),

      m_debug(false)
{
    // Registration: InPort/OutPort/Service

    // Set OutPort buffers
    registerOutPort("cometcdcreader_out", m_OutPort);

    init_command_port();
    init_state_table();
    set_comp_name("COMETCDCREADER");
}

CometCdcReader::~CometCdcReader()
{
}

RTC::ReturnCode_t CometCdcReader::onInitialize()
{
    if (m_debug) {
        std::cerr << "CometCdcReader::onInitialize()" << std::endl;
    }

    return RTC::RTC_OK;
}

RTC::ReturnCode_t CometCdcReader::onExecute(RTC::UniqueId ec_id)
{
    daq_do();

    return RTC::RTC_OK;
}

int CometCdcReader::daq_dummy()
{
    return 0;
}

int CometCdcReader::daq_configure()
{
    std::cerr << "*** CometCdcReader::configure" << std::endl;

    ::NVList* paramList;
    paramList = m_daq_service0.getCompParams();
    parse_params(paramList);

    if (m_set_registers) {
        for (unsigned int i = 0; i < m_module_list.size(); i++) {
            if (set_window_size(m_module_list[i].ip_address, m_window_size) < 0) {
                fatal_error_report(USER_DEFINED_ERROR1, "CANNOT SET WINDOW SIZE");
            }
            if (set_packet_id(m_module_list[i].ip_address, m_module_list[i].module_num) < 0) {
                fatal_error_report(USER_DEFINED_ERROR1, "CANNOT SET MODULE NUM");
            }
        }
    }

    return 0;
}

int CometCdcReader::parse_params(::NVList* list)
{
    std::cerr << "param list length:" << (*list).length() << std::endl;
    std::vector<std::string> ip_addresses;
    std::vector<int>         ports;

    int len = (*list).length();
    for (int i = 0; i < len; i+=2) {
        std::string sname  = (std::string)(*list)[i].value;
        std::string svalue = (std::string)(*list)[i+1].value;

        std::cerr << "sname: " << sname << "  ";
        std::cerr << "value: " << svalue << std::endl;

        if ( sname == "srcAddr" ) {
            if (m_debug) {
                std::cerr << "source addr: " << svalue << std::endl;
            }
            ip_addresses.push_back(svalue);
        }
        if ( sname == "srcPort" ) {
            if (m_debug) {
                std::cerr << "source port: " << svalue << std::endl;
            }
            char* offset;
            m_srcPort = (int)strtol(svalue.c_str(), &offset, 10);
            ports.push_back(m_srcPort);
        }
        if (sname == "windowSize") {
            if (m_debug) {
                std::cerr << "window_size: " << svalue << std::endl;
            }
            char* offset;
            m_window_size = (int)strtol(svalue.c_str(), &offset, 10);
        }
        if (sname == "setRegisters") {
            if (m_debug) {
                std::cerr << "setRegisters: " << svalue << std::endl;
            }
            if (svalue == "yes" || svalue == "Yes" || svalue == "YES") {
                m_set_registers = true;
            }
            else {
                m_set_registers = false;
            }
        }
    }

    if (ip_addresses.size() != ports.size()) {
        fatal_error_report(USER_DEFINED_ERROR1, "NUM OF IP ADDR/PORT MISMATCH");
    }

    module_info mi;
    for (unsigned int i = 0; i < ip_addresses.size(); i++) {
        mi.ip_address = ip_addresses[i];
        mi.port       = ports[i];
        mi.module_num = i;
        memset(&(mi.buf), 0, sizeof(mi.buf));
        m_module_list.push_back(mi);
    }

    m_read_byte_size =  COMET_CDC_HEADER_BYTE_SIZE
                      + COMET_CDC_N_CHANNEL*COMET_CDC_ONE_EVENT_BYTE_SIZE*m_window_size*2;
    
    for (unsigned int i = 0; i < m_module_list.size(); i++) {
        std::cerr << "ip_address: " << m_module_list[i].ip_address << std::endl;
        std::cerr << "port:       " << m_module_list[i].port       << std::endl;
        std::cerr << "module_num: " << m_module_list[i].module_num << std::endl;
    }
    std::cerr << "m_window_size:    " << m_window_size    << std::endl;
    std::cerr << "m_read_byte_size: " << m_read_byte_size << std::endl;

    return 0;
}

int CometCdcReader::daq_unconfigure()
{
    std::cerr << "*** CometCdcReader::unconfigure" << std::endl;
    m_module_list.clear();

    return 0;
}

int CometCdcReader::daq_start()
{
    std::cerr << "*** CometCdcReader::start" << std::endl;

    m_out_status = BUF_SUCCESS;

    // Create socket (not yet connected)
    try {
        for (unsigned int i = 0; i < m_module_list.size(); i++) {
            m_module_list[i].Sock = DAQMW::Sock(m_module_list[i].ip_address, m_module_list[i].port);
            m_module_list[i].Sock.createTCP();
            m_module_list[i].Sock.setOptRecvBuf(16*1024*1024);
        }
    } catch (DAQMW::SockException& e) {
        std::cerr << "Sock Fatal Error : " << e.what() << std::endl;
        fatal_error_report(USER_DEFINED_ERROR1, "SOCKET FATAL ERROR");
    } catch (...) {
        std::cerr << "Sock Fatal Error : Unknown" << std::endl;
        fatal_error_report(USER_DEFINED_ERROR1, "SOCKET FATAL ERROR");
    }

    // debug
    for (unsigned int i = 0; i < m_module_list.size(); i++) {
        std::cerr << "sockfd: " << m_module_list[i].ip_address << " "
                  << m_module_list[i].Sock.getSockFd() << std::endl;
    }

    // Epoll handler
    m_epfd = epoll_create(m_module_list.size());
    if (m_epfd < 0) {
        warn("epoll_create: ");
        fatal_error_report(USER_DEFINED_ERROR1, "EPOLL FD CREATION ERROR");
    }
    // debug
    std::cerr << "m_epfd: " << m_epfd << std::endl;

    // Epoll: register the watch socket
    struct epoll_event ev;
    for (unsigned int i = 0; i < m_module_list.size(); i++) {
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.ptr = &(m_module_list[i]);
        int sockfd = m_module_list[i].Sock.getSockFd();
        std::cerr << "sockfd: " << sockfd << std::endl;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_module_list[i].Sock.getSockFd(), &ev) < 0) {
            warn("epoll_ctl");
            fatal_error_report(USER_DEFINED_ERROR1, "EPOLL CTL ADD ERROR");
        }
    }
    // Prepare for epoll_wait return data
    m_ev_ret = (struct epoll_event *)malloc(sizeof(struct epoll_event)*m_module_list.size());
    if (m_ev_ret == NULL) {
        warn("malloc for m_ev_ret");
        fatal_error_report(USER_DEFINED_ERROR1, "MALLOC FOR m_ev_ret");
    }

    // Connect
    for (unsigned int i = 0; i < m_module_list.size(); i++) {
        int status = m_module_list[i].Sock.connectTCP();
        if (status != DAQMW::Sock::SUCCESS) {
            if (status == DAQMW::Sock::ERROR_TIMEOUT) {
                std::cerr << "Connect timeout for " << m_module_list[i].ip_address << std::endl;;
            }
            if (status == DAQMW::Sock::ERROR_FATAL) {
                std::cerr << "Fatal eror for " << m_module_list[i].ip_address << std::endl;;
            }
            fatal_error_report(USER_DEFINED_ERROR1, "CONNECTION ERROR");
        }
    }
        
    // Check data port connections
    bool outport_conn = check_dataPort_connections( m_OutPort );
    if (!outport_conn) {
        std::cerr << "### NO Connection" << std::endl;
        fatal_error_report(DATAPATH_DISCONNECTED);
    }

    //debug
    std::cerr << "m_module_list size: " << m_module_list.size() << std::endl;

    return 0;
}

int CometCdcReader::daq_stop()
{
    std::cerr << "*** CometCdcReader::stop" << std::endl;

    // epoll: remove the watch socket
    for (unsigned int i = 0; i < m_module_list.size(); i++) {
        if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_module_list[i].Sock.getSockFd(), NULL) < 0) {
            warn("epoll_ctl DEL for %s", m_module_list[i].ip_address.c_str());
        }
        m_module_list[i].Sock.disconnect();
    }
    // epoll: destroy epoll handler
    if (close(m_epfd) < 0) {
        warn("close for epoll file descriptor");
        fatal_error_report(USER_DEFINED_ERROR1, "CLOSE ERROR FOR EPOLL FD");
    }
    // epoll: remove epoll_wait return data buffer
    if (m_ev_ret != NULL) {
        free(m_ev_ret);
    }

    return 0;
}

int CometCdcReader::daq_pause()
{
    std::cerr << "*** CometCdcReader::pause" << std::endl;

    return 0;
}

int CometCdcReader::daq_resume()
{
    std::cerr << "*** CometCdcReader::resume" << std::endl;

    return 0;
}

int CometCdcReader::set_data(unsigned char *read_buf, unsigned int data_byte_size)
{
    unsigned char header[8];
    unsigned char footer[8];

    set_header(&header[0], data_byte_size);
    set_footer(&footer[0]);

    ///set OutPort buffer length
    m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
    memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
    memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &read_buf[0], data_byte_size);
    memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
           FOOTER_BYTE_SIZE);

    return 0;
}

int CometCdcReader::write_OutPort()
{
    ////////////////// send data from OutPort  //////////////////
    bool ret = m_OutPort.write();

    //////////////////// check write status /////////////////////
    if (ret == false) {  // TIMEOUT or FATAL
        m_out_status  = check_outPort_status(m_OutPort);
        if (m_out_status == BUF_FATAL) {   // Fatal error
            fatal_error_report(OUTPORT_ERROR);
        }
        if (m_out_status == BUF_TIMEOUT) { // Timeout
            std::cerr << "BUF_TIMEOUT" << std::endl;
            return -1;
        }
    }
    else {
        m_out_status = BUF_SUCCESS; // successfully done
    }

    return 0;
}

int CometCdcReader::daq_run()
{
    if (m_debug) {
        std::cerr << "*** CometCdcReader::run" << std::endl;
    }

    if (check_trans_lock()) {  // check if stop command has come
        set_trans_unlock();    // transit to CONFIGURED state
        return 0;
    }

//    if (m_out_status == BUF_SUCCESS) {   // previous OutPort.write() successfully done
//        int ret = read_data_from_detectors();
//        if (ret > 0) {
//            m_recv_byte_size = ret;
//            set_data(m_recv_byte_size); // set data to OutPort Buffer
//        }
//    }
//
//    if (write_OutPort() < 0) {
//        ;     // Timeout. do nothing.
//    }
//    else {    // OutPort write successfully done
//        inc_sequence_num();                     // increase sequence num.
//        inc_total_data_size(m_recv_byte_size);  // increase total data byte size
//    }

    // Wait for packet.  Timeout is 2000 ms (2 seconds)
    unsigned int n_readable = epoll_wait(m_epfd, m_ev_ret, m_module_list.size(), 2000);
    module_info *mi;
    int status;
    for (unsigned int i = 0; i < n_readable; i++) {
        mi = (module_info *)m_ev_ret[i].data.ptr;
        status = mi->Sock.readAll(mi->buf, m_read_byte_size);
        if (status != DAQMW::Sock::SUCCESS) {
            if (status == DAQMW::Sock::ERROR_TIMEOUT) {
                std::cerr << "readAll() timeout for " << mi->ip_address << std::endl;
                fatal_error_report(USER_DEFINED_ERROR1, "READLL() TIMEOUT");
            }
            else if (status == DAQMW::Sock::ERROR_FATAL) {
                std::cerr << "readAll() fatal error for  " << mi->ip_address << std::endl;
                fatal_error_report(USER_DEFINED_ERROR1, "READLL() FATAL ERROR");
            }
        }

        // Read from one socket done.  Now trying to write outport
        set_data(m_module_list[i].buf, m_read_byte_size);
        unsigned int max_retry = 1024;
        for (unsigned int retry = 0; retry < max_retry; retry ++) {
            if (write_OutPort() == 0) { // write success
                break; // exit retry loop
            }
            else if (retry == (max_retry - 1)) {
                fprintfwt(stderr, "BUF_TIMEOUT retry: %d for %s\n", retry, mi->ip_address.c_str());
                fprintfwt(stderr, "retry count due to BUF_TIMEOUT exceeds max_retry: %s\n",
                    mi->ip_address.c_str());
                fatal_error_report(USER_DEFINED_ERROR1, "BUF_TIMEOUT");
            }
            else {
                fprintfwt(stderr, "BUF_TIMEOUT retry: %d for %s\n", retry, mi->ip_address.c_str());
            }
        }
        // data was written to OutPort successfully.
        inc_sequence_num();
        inc_total_data_size(m_read_byte_size);
    } // epoll readable loop

    return 0;
}

extern "C"
{
    void CometCdcReaderInit(RTC::Manager* manager)
    {
        RTC::Properties profile(cometcdcreader_spec);
        manager->registerFactory(profile,
                    RTC::Create<CometCdcReader>,
                    RTC::Delete<CometCdcReader>);
    }
};
