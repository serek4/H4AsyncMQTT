/*
Creative Commons: Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

You are free to:

Share — copy and redistribute the material in any medium or format
Adapt — remix, transform, and build upon the material

The licensor cannot revoke these freedoms as long as you follow the license terms. Under the following terms:

Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

NonCommercial — You may not use the material for commercial purposes.

ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions 
under the same license as the original.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others 
from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use is 
permitted by an applicable exception or limitation. To discuss an exception, contact the author:

philbowles2012@gmail.com

No warranties are given. The license may not give you all of the permissions necessary for your intended use. 
For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.
*/
#pragma once

#include<Arduino.h>
#include"h4amc_config.h"

#include<functional>
#include<string>
#include<map>
#include<queue>

#include<H4AsyncTCP.h>

#if H4AMC_DEBUG
    template<int I, typename... Args>
    void H4AMC_PRINT(const char* fmt, Args... args) {
        if (H4AMC_DEBUG >= I) Serial.printf(std::string(std::string("H4AMC:%d: H=%u M=%u ")+fmt).c_str(),I,_HAL_freeHeap(),_HAL_maxHeapBlock(),args...);
    }
    #define H4AMC_PRINT1(...) H4AMC_PRINT<1>(__VA_ARGS__)
    #define H4AMC_PRINT2(...) H4AMC_PRINT<2>(__VA_ARGS__)
    #define H4AMC_PRINT3(...) H4AMC_PRINT<3>(__VA_ARGS__)
    #define H4AMC_PRINT4(...) H4AMC_PRINT<4>(__VA_ARGS__)

    template<int I>
    void h4amcdump(const uint8_t* p, size_t len) { if (H4AMC_DEBUG >= I) dumphex(p,len); }
    #define H4AMC_DUMP3(p,l) h4amcdump<3>((p),l)
    #define H4AMC_DUMP4(p,l) h4amcdump<4>((p),l)
#else
    #define H4AMC_PRINT1(...)
    #define H4AMC_PRINT2(...)
    #define H4AMC_PRINT3(...)
    #define H4AMC_PRINT4(...)

    #define H4AMC_DUMP3(...)
    #define H4AMC_DUMP4(...)
#endif

enum H4AMC_FAILURE {
    H4AMC_CONNECT_FAIL= H4AMC_ERROR_BASE,
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
    H4AMC_USER_LOGIC_ERROR,
//    H4AMC_KEEPALIVE_TOO_LONG,
    H4AMC_ERROR_MAX
};

enum :uint8_t {
    CONNECT     = 0x10, // x
    CONNACK     = 0x20, // x
    PUBLISH     = 0x30, // x
    PUBACK      = 0x40, // x
    PUBREC      = 0x50, 
    PUBREL      = 0x62,
    PUBCOMP     = 0x70,
    SUBSCRIBE   = 0x82, // x
    SUBACK      = 0x90, // x
    UNSUBSCRIBE = 0xa2, // x
    UNSUBACK    = 0xb0, // x
    PINGREQ     = 0xc0, // x
    PINGRESP    = 0xd0, // x
    DISCONNECT  = 0xe0
};

enum H4AMC_MQTT_CNX_FLAG : uint8_t {
    USERNAME      = 0x80,
    PASSWORD      = 0x40,
    WILL_RETAIN   = 0x20,
    WILL_QOS1     = 0x08,
    WILL_QOS2     = 0x10,
    WILL          = 0x04,
    CLEAN_SESSION = 0x02
};
class mqttTraits {             
                std::string     _decodestring(uint8_t** p);
        inline  uint16_t        _peek16(uint8_t* p){ return (*(p+1))|(*p << 8); }
        
    public:
                uint32_t        remlen=0;
                uint8_t         offset=0;
                uint8_t         flags=0;

                uint8_t*        data;
                size_t          len;
                uint8_t         type;
                uint16_t        id=0;
                uint8_t         qos=0;
                bool            dup;
                bool            retain;
                std::string     topic;
                uint8_t*        payload;
                size_t          plen;
                bool            pubrec=0;
                size_t          retries=H4AMC_MAX_RETRIES;
                std::pair<uint8_t*,size_t> next;
                
#if H4AMC_DEBUG
        static std::map<uint8_t,char*> pktnames;
        std::string             getPktName(){
            uint8_t t=type&0xf0;
            if(pktnames.count(t)) return pktnames[t];
            else return stringFromInt(type,"02X");
        }
                void            dump();
#else
        std::string             getPktName(){ return stringFromInt(type,"02X"); }
#endif
        mqttTraits(){};
        mqttTraits(uint8_t* p,size_t s);

        inline  bool            isPublish() { return (type & 0xf0) == PUBLISH; }
        inline  uint8_t*        start() { return data+1+offset; }
};

using H4AMC_FN_VOID        = std::function<void(void)>;
using H4AMC_FN_U8PTR       = std::function<void(uint8_t*,uint8_t* base)>;
using H4AMC_FN_U8PTRU8     = std::function<uint8_t*(uint8_t*)>;

using H4AMC_PACKET_MAP      =std::map<uint16_t,mqttTraits>; // indexed by messageId
using H4AMC_cbError         =std::function<void(int, int)>;
using H4AMC_cbMessage       =std::function<void(const char* topic, const uint8_t* payload, size_t len,uint8_t qos,bool retain,bool dup)>;

class Packet;
class ConnectPacket;
class PublishPacket;

enum {
    H4AMC_DISCONNECTED,
    H4AMC_RUNNING,
    H4AMC_FATAL
};

class H4AsyncMQTT {
        friend class Packet;
        friend class ConnectPacket;
        friend class PublishPacket;
        friend class mqttTraits;
                uint32_t            _state=H4AMC_DISCONNECTED;
                H4_FN_VOID          _cbMQTTConnect=nullptr;
                H4_FN_VOID          _cbMQTTDisconnect=nullptr;
                H4AMC_cbError       _cbMQTTError=nullptr;

                H4AMC_cbMessage     _cbMessage=nullptr;
                bool                _cleanSession=true;
                std::string         _clientId;
        static  H4_INT_MAP          _errorNames;
        static  H4AMC_PACKET_MAP    _inbound;
                uint32_t            _keepalive=H4AS_SCAVENGE_FREQ - H4AMC_HEADROOM;
                size_t              _nextId=1000;
        static  H4AMC_PACKET_MAP    _outbound;
                std::string         _password;
                std::string         _url;
                std::string         _username;
                std::string         _willPayload;
                uint8_t             _willQos;
                bool                _willRetain;
                std::string         _willTopic;
               
                void                _ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout); // inout true=INBOUND false=OUTBOUND
                void                _ACKoutbound(uint16_t id){ _ACK(&_outbound,id,false); }

                void                _cleanStart();
                void                _clearQQ(H4AMC_PACKET_MAP* m);
                void                _connect();
                void                _destroyClient();
                void                _handlePacket(uint8_t* data, size_t len, int n_handled=0);
                void                _handlePublish(mqttTraits T);
        inline  void                _hpDespatch(mqttTraits T);
        inline  void                _hpDespatch(uint16_t id);
                void                _resendPartialTxns();
        inline  void                _runGuard(H4_FN_VOID f);
        inline  void                _startReconnector();
    public:
                H4AsyncClient*      _h4atClient;
        H4AsyncMQTT();
                void                connect(const char* url,const char* auth="",const char* pass="",const char* clientId="",bool clean=true);
                void                disconnect();
        static  std::string         errorstring(int e);
                std::string         getClientId(){ return _clientId; }
                void                onConnect(H4_FN_VOID callback){ _cbMQTTConnect=callback; }
                void                onDisconnect(H4_FN_VOID callback){ _cbMQTTDisconnect=callback; }
                void                onError(H4AMC_cbError callback){ _cbMQTTError=callback; }
                void                onMessage(H4AMC_cbMessage callback){ _cbMessage=callback; }
                void                publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0,  bool retain=false);
                void                publish(const char* topic,const char* payload, size_t length, uint8_t qos=0,  bool retain=false);
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
//                void               setKeepAlive(uint16_t keepAlive){ _keepalive=keepAlive; }
                void               setWill(const char* topic, uint8_t qos, bool retain, const char* payload = nullptr);
                void               subscribe(const char* topic, uint8_t qos=0);
                void               subscribe(std::initializer_list<const char*> topix, uint8_t qos=0);
                void               unsubscribe(const char* topic);
                void               unsubscribe(std::initializer_list<const char*> topix);
//
//              DO NOT CALL ANY FUNCTION STARTING WITH UNDERSCORE!!! _
//
                void               _notify(int e,int info=0);
                void               dump(); // null if no debug
};