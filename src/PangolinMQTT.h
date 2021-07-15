/*
MIT License

Copyright (c) 2020 Phil Bowles with huge thanks to Adam Sharp http://threeorbs.co.uk
for testing, debugging, moral support and permanent good humour.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include"pango_config.h"
#include"pango_common.h"

#include<Arduino.h>

#include<functional>
#include<string>
#include<map>
#include<queue>

#ifdef ARDUINO_ARCH_ESP8266
    #if ASYNC_TCP_SSL_ENABLED
        #include <tcp_axtls.h>
    #endif
#endif

#include<H4AsyncTCP.h>

enum H4AMC_FAILURE {
    H4AMC_DISCONNECTED = H4AMC_ERROR_BASE,
    H4AMC_SERVER_UNAVAILABLE,
    H4AMC_CONNECT_FAIL,
    H4AMC_BAD_FINGERPRINT,
    H4AMC_NO_FINGERPRINT,
    H4AMC_NO_SSL,
    H4AMC_UNWANTED_FINGERPRINT,
    H4AMC_SUBSCRIBE_FAIL,
    H4AMC_INBOUND_QOS_ACK_FAIL,
    H4AMC_OUTBOUND_QOS_ACK_FAIL,
    H4AMC_INBOUND_PUB_TOO_BIG,
    H4AMC_OUTBOUND_PUB_TOO_BIG,
    H4AMC_BOGUS_PACKET,
    H4AMC_X_INVALID_LENGTH,
    H4AMC_NOT_ENOUGH_MEMORY,
    H4AMC_ERROR_MAX
};

#include<mqTraits.h>

using H4AMC_FN_VOID        = std::function<void(void)>;
using H4AMC_FN_U8PTR       = std::function<void(uint8_t*,uint8_t* base)>;
using H4AMC_FN_U8PTRU8     = std::function<uint8_t*(uint8_t*)>;

using H4AMC_PACKET_MAP      =std::map<uint16_t,mqttTraits>; // indexed by messageId
using H4AMC_cbConnect       =std::function<void(bool)>;
using H4AMC_cbDisconnect    =std::function<void(int8_t)>;
using H4AMC_cbError         =std::function<void(int, int)>;
using H4AMC_cbMessage       =std::function<void(const char* topic, const uint8_t* payload, size_t len,uint8_t qos,bool retain,bool dup)>;

class Packet;
class ConnectPacket;
class PublishPacket;

class PangolinMQTT: public H4AsyncTCP {
        friend class Packet;
        friend class ConnectPacket;
        friend class PublishPacket;
        friend class mqttTraits;

               H4AMC_cbConnect     _cbConnect=nullptr;
               H4AMC_cbDisconnect  _cbDisconnect=nullptr;
               H4AMC_cbError       _cbError=nullptr;
               H4AMC_cbMessage     _cbMessage=nullptr;
               bool                _cleanSession=true;
               std::string         _clientId;
               bool                _connected=false;
        static H4_INT_MAP          _errorNames;
        static H4AMC_PACKET_MAP    _inbound;
               uint16_t            _keepalive=15 * H4AMC_POLL_RATE;
               uint32_t            _nPollTicks;
               uint32_t            _nSrvTicks;
        static H4AMC_PACKET_MAP    _outbound;
               std::string         _password;
               std::string         _username;
               std::string         _willPayload;
               uint8_t             _willQos;
               bool                _willRetain;
               std::string         _willTopic;
               
               void                _ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout); // inout true=INBOUND false=OUTBOUND
               void                _ACKoutbound(uint16_t id){ _ACK(&_outbound,id,false); }

               void                _cleanStart();
               void                _clearQQ(H4AMC_PACKET_MAP* m);
               void                _cnxGuard(H4_FN_VOID f);
               void                _destroyClient();
               void                _handlePacket(uint8_t* data, size_t len);
               void                _handlePublish(mqttTraits T);
        inline void                _hpDespatch(mqttTraits T);
        inline void                _hpDespatch(uint16_t id);
               void                _onDisconnect(int8_t r);
               void                _onPoll();
               void                _resendPartialTxns();
    public:
        PangolinMQTT();
                void               connectMqtt(std::string clientId="",bool session=true);
                void               disconnect();
        static  std::string        errorstring(int e);
                std::string        getClientId(){ return _clientId; }
                size_t inline      getMaxPayloadSize(){ return (_HAL_maxHeapBlock() - H4AMC_HEAP_SAFETY) /2 ; }
                bool               mqttConnected(){ return _connected; }
                void               onMqttConnect(H4AMC_cbConnect callback){ _cbConnect=callback; }
                void               onMqttDisconnect(H4AMC_cbDisconnect callback){ _cbDisconnect=callback; }
                void               onMqttError(H4AMC_cbError callback){ _cbError=callback; }
                void               onMqttMessage(H4AMC_cbMessage callback){ _cbMessage=callback; }
                void               publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0,  bool retain=false);
                void               publish(const char* topic,const char* payload, size_t length, uint8_t qos=0,  bool retain=false);
                template<typename T>
                void publish(const char* topic,T v,const char* fmt="%d",uint8_t qos=0,bool retain=false){
                    char buf[16];
                    sprintf(buf,fmt,v);
                    publish(topic, reinterpret_cast<const uint8_t*>(buf), strlen(buf), qos, retain);
                }
//              Coalesce templates when C++17 available (if constexpr (x))
                void xPublish(const char* topic,const char* value, uint8_t qos=0,  bool retain=false) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value),strlen(value),qos,retain);
                }
                void xPublish(const char* topic,String value, uint8_t qos=0,  bool retain=false) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.length(),qos,retain);
                }
                void xPublish(const char* topic,std::string value, uint8_t qos=0,  bool retain=false) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.size(),qos,retain);
                }
                template<typename T>
                void xPublish(const char* topic,T value, uint8_t qos=0,  bool retain=false) {
                    publish(topic,reinterpret_cast<uint8_t*>(&value),sizeof(T),qos,retain);
                }
                void xPayload(const uint8_t* payload,size_t len,char*& cp) {
                    char* p=reinterpret_cast<char*>(malloc(len+1));
                    memcpy(p,payload,len);
                    p[len]='\0';
                    cp=p;
                }
                void xPayload(const uint8_t* payload,size_t len,std::string& ss) {
                    char* cp;
                    xPayload(payload,len,cp);
                    ss.assign(cp,strlen(cp));
                    ::free(cp);
                }
                void xPayload(const uint8_t* payload,size_t len,String& duino) {
                    char* cp;
                    xPayload(payload,len,cp);
                    duino+=cp;
                    ::free(cp);
                }
                template<typename T>
                void xPayload(const uint8_t* payload,size_t len,T& value) {
                    if(len==sizeof(T)) memcpy(reinterpret_cast<T*>(&value),payload,sizeof(T));
                    else _notify(H4AMC_X_INVALID_LENGTH,len);
                }
                void               setKeepAlive(uint16_t keepAlive){ _keepalive = H4AMC_POLL_RATE * keepAlive; }
                void               setServer(const char* url,const char* username="", const char* password = "",const uint8_t* fingerprint=nullptr);
                void               setWill(const std::string& topic, uint8_t qos, bool retain, const std::string& payload = nullptr);
                void               subscribe(const char* topic, uint8_t qos=0);
                void               subscribe(std::initializer_list<const char*> topix, uint8_t qos=0);
                void               unsubscribe(const char* topic);
                void               unsubscribe(std::initializer_list<const char*> topix);
//
//              DO NOT CALL ANY FUNCTION STARTING WITH UNDERSCORE!!! _
//
                void               _notify(uint8_t e,int info=0);
#if H4AMC_DEBUG
                void               dump(); // null if no debug
#endif
};

extern PangolinMQTT*        H4AMCV3;