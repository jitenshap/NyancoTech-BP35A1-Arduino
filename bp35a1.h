#include <Arduino.h>
#include <HardwareSerial.h>
#ifndef _BP35A1_h
#define _BP35A1_h

class BP35A1
{
    private:
        HardwareSerial *_serial;
        void discard_buf();
        bool read_res();
        bool get_scan_result(int dur);
        bool get_ipv6_addr();
        bool get_connecting_status();
        long get_and_parse_inst_data();

    public:
        long get_power();
        bool connect(const char * bid, const char * bpwd);
        void begin(HardwareSerial * s);
};

#endif