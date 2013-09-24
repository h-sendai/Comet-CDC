// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "CometCdcMonitor.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* cometcdcmonitor_spec[] =
{
    "implementation_id", "CometCdcMonitor",
    "type_name",         "CometCdcMonitor",
    "description",       "CometCdcMonitor component",
    "version",           "1.0",
    "vendor",            "Kazuo Nakayoshi, KEK",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    ""
};

CometCdcMonitor::CometCdcMonitor(RTC::Manager* manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("cometcdcmonitor_in",   m_in_data),
      m_in_status(BUF_SUCCESS),
      m_canvas(0),
      m_hist(0),
      m_bin(0),
      m_min(0),
      m_max(0),
      m_monitor_update_rate(30),
      m_event_byte_size(0),
      m_debug(false)
{
    // Registration: InPort/OutPort/Service

    // Set InPort buffers
    registerInPort ("cometcdcmonitor_in",  m_InPort);

    init_command_port();
    init_state_table();
    set_comp_name("COMETCDCMONITOR");
}

CometCdcMonitor::~CometCdcMonitor()
{
}

RTC::ReturnCode_t CometCdcMonitor::onInitialize()
{
    if (m_debug) {
        std::cerr << "CometCdcMonitor::onInitialize()" << std::endl;
    }

    return RTC::RTC_OK;
}

RTC::ReturnCode_t CometCdcMonitor::onExecute(RTC::UniqueId ec_id)
{
    daq_do();

    return RTC::RTC_OK;
}

int CometCdcMonitor::daq_dummy()
{
    if (m_canvas) {
        m_canvas->Update();
        // daq_dummy() will be invoked again after 10 msec.
        // This sleep reduces X servers' load.
        sleep(1);
    }

    return 0;
}

int CometCdcMonitor::daq_configure()
{
    std::cerr << "*** CometCdcMonitor::configure" << std::endl;

    ::NVList* paramList;
    paramList = m_daq_service0.getCompParams();
    parse_params(paramList);

    return 0;
}

int CometCdcMonitor::parse_params(::NVList* list)
{

    std::cerr << "param list length:" << (*list).length() << std::endl;

    int len = (*list).length();
    for (int i = 0; i < len; i+=2) {
        std::string sname  = (std::string)(*list)[i].value;
        std::string svalue = (std::string)(*list)[i+1].value;

        std::cerr << "sname: " << sname << "  ";
        std::cerr << "value: " << svalue << std::endl;
    }

    return 0;
}

int CometCdcMonitor::daq_unconfigure()
{
    std::cerr << "*** CometCdcMonitor::unconfigure" << std::endl;
    if (m_canvas) {
        delete m_canvas;
        m_canvas = 0;
    }

    if (m_hist) {
        delete m_hist;
        m_hist = 0;
    }
    return 0;
}

int CometCdcMonitor::daq_start()
{
    std::cerr << "*** CometCdcMonitor::start" << std::endl;

    m_in_status  = BUF_SUCCESS;

    //////////////// CANVAS FOR HISTOS ///////////////////
    if (m_canvas) {
        delete m_canvas;
        m_canvas = 0;
    }
    m_canvas = new TCanvas("c1", "histos", 0, 0, 600, 400);

    ////////////////       HISTOS      ///////////////////
    if (m_hist) {
        delete m_hist;
        m_hist = 0;
    }

    int m_hist_bin = 100;
    double m_hist_min = 0.0;
    double m_hist_max = 1000.0;

    gStyle->SetStatW(0.4);
    gStyle->SetStatH(0.2);
    gStyle->SetOptStat("em");

    m_hist = new TH1F("hist", "hist", m_hist_bin, m_hist_min, m_hist_max);
    m_hist->GetXaxis()->SetNdivisions(5);
    m_hist->GetYaxis()->SetNdivisions(4);
    m_hist->GetXaxis()->SetLabelSize(0.07);
    m_hist->GetYaxis()->SetLabelSize(0.06);

    return 0;
}

int CometCdcMonitor::daq_stop()
{
    std::cerr << "*** CometCdcMonitor::stop" << std::endl;

    m_hist->Draw();
    m_canvas->Update();

    reset_InPort();

    return 0;
}

int CometCdcMonitor::daq_pause()
{
    std::cerr << "*** CometCdcMonitor::pause" << std::endl;

    return 0;
}

int CometCdcMonitor::daq_resume()
{
    std::cerr << "*** CometCdcMonitor::resume" << std::endl;

    return 0;
}

int CometCdcMonitor::reset_InPort()
{
    int ret = true;
    while(ret == true) {
        ret = m_InPort.read();
    }

    return 0;
}

int CometCdcMonitor::decode_data(const unsigned char* mydata)
{
    m_sampleData.magic      = mydata[0];
    m_sampleData.format_ver = mydata[1];
    m_sampleData.module_num = mydata[2];
    m_sampleData.reserved   = mydata[3];
    unsigned int netdata    = *(unsigned int*)&mydata[4];
    m_sampleData.data       = ntohl(netdata);

    if (m_debug) {
        std::cerr << "magic: "      << std::hex << (int)m_sampleData.magic      << std::endl;
        std::cerr << "format_ver: " << std::hex << (int)m_sampleData.format_ver << std::endl;
        std::cerr << "module_num: " << std::hex << (int)m_sampleData.module_num << std::endl;
        std::cerr << "reserved: "   << std::hex << (int)m_sampleData.reserved   << std::endl;
        std::cerr << "data: "       << std::dec << (int)m_sampleData.data       << std::endl;
    }

    return 0;
}

int CometCdcMonitor::fill_data(const unsigned char* mydata, const int size)
{
    for (int i = 0; i < size/(int)ONE_EVENT_SIZE; i++) {
        decode_data(mydata);
        float fdata = m_sampleData.data/1000.0; // 1000 times value is received
        m_hist->Fill(fdata);

        mydata+=ONE_EVENT_SIZE;
    }
    return 0;
}

unsigned int CometCdcMonitor::read_InPort()
{
    /////////////// read data from InPort Buffer ///////////////
    unsigned int recv_byte_size = 0;
    bool ret = m_InPort.read();

    //////////////////// check read status /////////////////////
    if (ret == false) { // false: TIMEOUT or FATAL
        m_in_status = check_inPort_status(m_InPort);
        if (m_in_status == BUF_TIMEOUT) { // Buffer empty.
            if (check_trans_lock()) {     // Check if stop command has come.
                set_trans_unlock();       // Transit to CONFIGURE state.
            }
        }
        else if (m_in_status == BUF_FATAL) { // Fatal error
            fatal_error_report(INPORT_ERROR);
        }
    }
    else {
        recv_byte_size = m_in_data.data.length();
    }

    if (m_debug) {
        std::cerr << "m_in_data.data.length():" << recv_byte_size
                  << std::endl;
    }

    return recv_byte_size;
}

int CometCdcMonitor::daq_run()
{
    if (m_debug) {
        std::cerr << "*** CometCdcMonitor::run" << std::endl;
    }

    unsigned int recv_byte_size = read_InPort();
    if (recv_byte_size == 0) { // Timeout
        return 0;
    }

    check_header_footer(m_in_data, recv_byte_size); // check header and footer
    m_event_byte_size = get_event_size(recv_byte_size);

    /////////////  Write component main logic here. /////////////
    memcpy(&m_recv_data[0], &m_in_data.data[HEADER_BYTE_SIZE], m_event_byte_size);

    fill_data(&m_recv_data[0], m_event_byte_size);

    if (m_monitor_update_rate == 0) {
        m_monitor_update_rate = 1000;
    }

    unsigned long sequence_num = get_sequence_num();
    if ((sequence_num % m_monitor_update_rate) == 0) {
        m_hist->Draw();
        m_canvas->Update();
    }
    /////////////////////////////////////////////////////////////
    inc_sequence_num();                      // increase sequence num.
    inc_total_data_size(m_event_byte_size);  // increase total data byte size

    return 0;
}

extern "C"
{
    void CometCdcMonitorInit(RTC::Manager* manager)
    {
        RTC::Properties profile(cometcdcmonitor_spec);
        manager->registerFactory(profile,
                    RTC::Create<CometCdcMonitor>,
                    RTC::Delete<CometCdcMonitor>);
    }
};
