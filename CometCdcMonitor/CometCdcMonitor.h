// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef COMETCDCMONITOR_H
#define COMETCDCMONITOR_H

#include "DaqComponentBase.h"

#include <arpa/inet.h> // for ntohl()

////////// ROOT Include files //////////
#include "TH1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TApplication.h"

#include "CometCdcData.h"

using namespace RTC;

class CometCdcMonitor
    : public DAQMW::DaqComponentBase
{
public:
    CometCdcMonitor(RTC::Manager* manager);
    ~CometCdcMonitor();

    // The initialize action (on CREATED->ALIVE transition)
    // former rtc_init_entry()
    virtual RTC::ReturnCode_t onInitialize();

    // The execution action that is invoked periodically
    // former rtc_active_do()
    virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
    TimedOctetSeq          m_in_data;
    InPort<TimedOctetSeq>  m_InPort;

private:
    int daq_dummy();
    int daq_configure();
    int daq_unconfigure();
    int daq_start();
    int daq_run();
    int daq_stop();
    int daq_pause();
    int daq_resume();

    int parse_params(::NVList* list);
    int reset_InPort();

    unsigned int read_InPort();
    //int online_analyze();
    int decode_data(const unsigned char* mydata);
    int fill_data(const unsigned char* mydata, const int size);

    BufferStatus m_in_status;

    ////////// ROOT Histogram //////////
    TCanvas *m_canvas;
    TH1F    *m_hist;
    int      m_bin;
    double   m_min;
    double   m_max;
    int      m_monitor_update_rate;
    unsigned char m_recv_data[4096];
    unsigned int  m_event_byte_size;
    struct sampleData m_sampleData;

    bool m_debug;
};


extern "C"
{
    void CometCdcMonitorInit(RTC::Manager* manager);
};

#endif // COMETCDCMONITOR_H
