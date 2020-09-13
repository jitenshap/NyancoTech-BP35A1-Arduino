#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdlib.h>
#include <WString.h>
#include "bp35a1.h"

//#define BP35A1_DEBUG

HardwareSerial BP35A1::*_serial;
char _addr_ipv6[128];
char _addr[32];
char _panId[8];
char _channel[8];

//_serialのバッファクリア
void BP35A1::discard_buf()
{
    delay(1000);
    while(_serial->available())
    {
        _serial->read();
    }
}

//レスポンスからOKを探す
bool BP35A1::read_res()
{
    _serial->flush();  //BP35A1へ送信したコマンドの送信完了を待つ
    int ttimeout = 0;
    while(ttimeout < 50)
    {
        #ifdef BP35A1_DEBUG
        #endif                
        char tmp[128];
        char * tmp_p = tmp;
        tmp[0] = '\0';
        long timeout = millis() + 1000;
        while(_serial->available())
        {
            *tmp_p = _serial->read();       
            if(*tmp_p == '\n')
            {
                tmp_p += sizeof(char);
                *tmp_p = '\0';
                #ifdef BP35A1_DEBUG
                Serial.printf("read_res():Received [%s]\r\n", tmp);
                #endif
                break;
            }
            if(timeout - millis() < 0)
            {
                #ifdef BP35A1_DEBUG
                Serial.println("read_res(): UART timedout");
                #endif
                tmp_p = tmp;
                break;
            }
            tmp_p += sizeof(char);
        }
        #ifdef BP35A1_DEBUG
        #endif
        if(strstr(tmp ,"FAIL ER") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("error response receiverd"); 
            #endif
            discard_buf();
            return false;
        }
        else if(strstr(tmp, "OK") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("read_res(): OK");
            #endif
            return true;
        }
        delay(100);
        ttimeout ++;
    }
    discard_buf();
    #ifdef BP35A1_DEBUG
    Serial.println("read_res(): timedout");
    Serial.flush();
    #endif
    return false;
}

//スキャン結果のパース
bool BP35A1::get_scan_result(int dur)
{
    _serial->flush();
    int ttimeout = 0;
    while(ttimeout < (dur * 30 + 50))
    {
        char tmp[512];
        tmp[0]  = '\0';
        char * tmp_p = tmp;
        long timeout = millis() + 5000;
        while(_serial->available())
        {
            *tmp_p = _serial->read();       
            if(*tmp_p == '\n')
            {

                tmp_p += sizeof(char);
                *tmp_p = '\0';
                break;
            }
            if(timeout - millis() < 0)
            {
                #ifdef BP35A1_DEBUG
                Serial.println("get_scan_result(): _serial timedout");
                #endif
                tmp_p = tmp;
                break;
            }
            tmp_p += sizeof(char);
        }
        if(strstr(tmp, "Channel:") != NULL)
        {
            char *start = strstr(tmp ,"Channel:");
            start += (sizeof(char) * 8);
            char * end = strstr(start, "\n");
            strncpy(_channel, start, (end - start) / sizeof(char));
        }
        else if(strstr(tmp, "Pan ID:") != NULL)
        {
            char *start = strstr(tmp ,"Pan ID:");
            start += (sizeof(char) * 7);
            char * end = strstr(start, "\n");
            strncpy(_panId, start, (end - start) / sizeof(char));
        }
        else if(strstr(tmp, "Addr:") != NULL)
        {
            char *start = strstr(tmp ,"Addr:");
            start += (sizeof(char) * 5);
            char * end = strstr(start, "\n");
            strncpy(_addr, start, (end - start) / sizeof(char));
            return true;
        }
        else if(strstr(tmp ,"EVENT 22 ") != NULL)
        {
            return false;
        }
        ttimeout ++;
        delay(100);
    }
    #ifdef BP35A1_DEBUG
    Serial.println("get_scan_result(): timedout");
    #endif
    discard_buf();
    return false;
}

//スキャン結果のメーターアドレスをIPv6アドレス形式に変換
bool BP35A1::get_ipv6_addr()
{
    _serial->flush();
    int ttimeout = 0;
    while(ttimeout < 50)
    {
        char tmp[128];
        char * tmp_p = tmp;
        tmp[0]  = '\0';
        long timeout = millis() + 5000;
        while(_serial->available())
        {
            *tmp_p = _serial->read();
            if(*tmp_p == '\n')
            {
                strncpy(_addr_ipv6, tmp, 39);
                #ifdef BP35A1_DEBUG
                Serial.printf("get_ipv6_addr(): IPv6Addr = [%s]\r\n", _addr_ipv6);
                #endif
                discard_buf();
                return true;
            }
            if(timeout - millis() < 0)
            {
                #ifdef BP35A1_DEBUG
                Serial.printf("get_ipv6_addr(): UART Timed out. [%s]\r\n", tmp);
                #endif
                tmp[0]  = '\0';
                tmp_p = tmp;
                break;
            }
            tmp_p += sizeof(char);
        }
        ttimeout ++;
        delay(100);
    }
    discard_buf();
    return false;
}

//EVENT 25が来るのを待つ
bool BP35A1::get_connecting_status()
{
    _serial->flush();
    long ttimeout = millis() + 30000;
    while(ttimeout - millis() > 0)
    {
        char tmp[128];
        char * tmp_p = tmp;
        tmp[0] = '\0';
        long timeout = millis() + 5000;
        while(_serial->available())
        {
            if(timeout - millis() < 0)
            {
                tmp[0] = '\0';
                tmp_p = tmp;
                #ifdef BP35A1_DEBUG
                Serial.printf("get_connecting_status():UART timed out. %s\r\n", tmp);
                #endif
                break;
            }
            *tmp_p = _serial->read();       
            if(*tmp_p == '\n')
            {
                tmp_p += sizeof(char);
                *tmp_p = '\0';
                break;
            }
            tmp_p += sizeof(char);
        }
        if(strstr(tmp, "EVENT 25") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("get_connecting_status(): Connected!");
            #endif
            discard_buf();
            return true;
        }
        else if(strstr(tmp, "EVENT 24") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("get_connecting_status(): Connection failed");
            #endif
            discard_buf();
            return false;
        }
        else if(strstr(tmp, "EVENT 21") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("get_connecting_status(): ...");
            #endif
        }
    }
    discard_buf();
    return false;
}

//瞬時電力レスポンスを受信してパース
long BP35A1::get_and_parse_inst_data()
{
    int timeout = 0;
    while(timeout < 110)
    {
        char tmp[512];
        char * tmp_p = tmp;
        tmp[0] = '\0';
        long timeout = millis() + 5000;
        while(_serial->available())
        {
            if(timeout - millis() < 0)
            {
                tmp[0] = '\0';
                tmp_p = tmp;
                #ifdef BP35A1_DEBUG
                Serial.printf("get_and_parse_inst_data(): UART timed out. %s\r\n", tmp);
                #endif
                break;
            }
            *tmp_p = _serial->read();       
            if(*tmp_p == '\n')
            {
                tmp_p += sizeof(char);
                *tmp_p = '\0';
                break;
            }
            tmp_p += sizeof(char);
        }
        if(strstr(tmp, "ERXUDP ") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("get_and_parse_inst_data(): Got UDP Response");
            Serial.printf("get_and_parse_inst_data(): len = %ld\r\n", strlen(tmp));
            Serial.printf("[%s]\r\n", tmp);
            #endif
            char pwr_char[16];
            tmp_p = tmp + sizeof(char) * (strlen(tmp) - 9);
            strcpy(pwr_char, tmp_p);
            long power = strtol(pwr_char, NULL, 16);
            #ifdef BP35A1_DEBUG
            Serial.println(pwr_char);
            Serial.printf("get_and_parse_inst_data(): Power = %ld [W]\r\n", power);
            #endif
            return power;
        }
        else if(strstr(tmp, "FAIL ER04") != NULL)
        {
            #ifdef BP35A1_DEBUG
            Serial.println("get_and_parse_inst_data(): ERROR");
            #endif
            return -1;
        }
        timeout ++;
        delay(100);
    }
    discard_buf();
    return -1;  
}

//瞬時電力をリクエストして取得する
long BP35A1::get_power()
{
    char sendBuf[] = {0x10, 0x81, 0x00, 0x01, 0x05, 0xFF, 0x01, 0x02, 0x88, 0x01, 0x62, 0x01, 0xE7, 0x00};
    #ifdef BP35A1_DEBUG
    Serial.println("Send request");
    Serial.printf("SKSENDTO 1 %s 0E1A 1 000E ", _addr_ipv6);
    for(int i = 0; i < sizeof(sendBuf); i ++)
    {
        Serial.write(sendBuf[i]);
    }
    Serial.print("\r\n");
    #endif
    _serial->printf("SKSENDTO 1 %s 0E1A 1 000E ", _addr_ipv6);
    for(int i = 0; i < sizeof(sendBuf); i ++)
    {
        _serial->write(sendBuf[i]);
    }
    _serial->print("\r\n");
    if(!read_res())
    {
        #ifdef BP35A1_DEBUG
        Serial.println("err");
        #endif
        return -1;
    }
    long power = get_and_parse_inst_data();
    return power;
}

bool BP35A1::connect(const char * _bid, const char * _bpwd)
{
    delay(1000);
    discard_buf();
    #ifdef BP35A1_DEBUG
    Serial.println("Disabling echo");
    Serial.print("SKSREG SFE 0\r\n");
    #endif
    _serial->print("SKSREG SFE 0\r\n");
    delay(500);
    discard_buf();
    #ifdef BP35A1_DEBUG
    Serial.println("Checking connection to BP35A1");
    Serial.print("SKVER\r\n");
    #endif
    discard_buf();
    _serial->print("SKVER\r\n");
    if(!read_res())
    {
        #ifdef BP35A1_DEBUG
        Serial.println("err");
        #endif 
    }
    #ifdef BP35A1_DEBUG
    Serial.println("Force disconnecting");
    Serial.print("SKTERM\r\n");
    #endif
    discard_buf();
    _serial->print("SKTERM\r\n");
    delay(100);
    discard_buf();
    #ifdef BP35A1_DEBUG
    Serial.println("Setting password");
    Serial.printf("SKSETPWD C %s\r\n",_bpwd);
    #endif
    discard_buf();
    _serial->printf("SKSETPWD C %s\r\n",_bpwd);
    if(!read_res())
    {
        return false;
    }
    #ifdef BP35A1_DEBUG
    Serial.println("Setting ID");
    Serial.printf("SKSETRBID %s\r\n", _bid);
    #endif
    discard_buf();
    _serial->printf("SKSETRBID %s\r\n", _bid);
    if(!read_res())
    {
        return false;
    }
    #ifdef BP35A1_DEBUG
    Serial.println("Scanning the meter");
    #endif
    for(int i = 3; i <= 6; i ++)
    {
        #ifdef BP35A1_DEBUG
        Serial.printf("SKSCAN 3 FFFFFFFF %d\r\n", i);
        #endif
        discard_buf();
        _serial->printf("SKSCAN 3 FFFFFFFF %d\r\n", i);
        if(!read_res())
        {
            return false;
        }
        if(!get_scan_result(i))
        {
            if(i > 6)
            {
                return false;
            }
            #ifdef BP35A1_DEBUG
            Serial.println("retry");
            #endif
            delay(1000);
        }
    }
    #ifdef BP35A1_DEBUG
    Serial.printf("Scan result\r\nAddr: [%s], Channel: [%s], Pan ID: [%s]\r\n", _addr, _channel, _panId);
    Serial.println("converting address format");
    Serial.printf("SKLL64 %s\r\n", _addr);
    #endif
    discard_buf();
    _serial->printf("SKLL64 %s\r\n", _addr);
    get_ipv6_addr();
    if(_addr == "NG")
    {
        return false;
    }
    #ifdef BP35A1_DEBUG
    Serial.printf("IPv6 Addr: [%s]\r\n", _addr_ipv6);
    Serial.println("Setting channel");
    Serial.printf("SKSREG S2 %s\r\n",_channel);
    #endif
    _serial->printf("SKSREG S2 %s\r\n", _channel);
    if(!read_res())
    {
        return false;
    }
    #ifdef BP35A1_DEBUG
    Serial.println("Setting pan ID");
    Serial.printf("SKSREG S3 %s\r\n", _panId);
    #endif
    _serial->printf("SKSREG S3 %s\r\n",_panId);
    if(!read_res())
    {
        return false;
    }
    #ifdef BP35A1_DEBUG
    Serial.println("Connecting");
    Serial.printf("SKJOIN %s\r\n", _addr_ipv6);
    #endif
    _serial->printf("SKJOIN %s\r\n", _addr_ipv6);
    if(!read_res())
    {
        return false;
    } 
    if(!get_connecting_status())
    {
        return false;    
    }
    else
    {
        return true;
    }
    return false;
}

void BP35A1::begin(HardwareSerial * s)
{
    _serial = s;
}
