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

enum PacketHeader :uint8_t {
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
#if MQTT5
    ,
    AUTH        = 0xf0
#endif
};

enum H4AMC_MQTT_CNX_FLAG : uint8_t {
    USERNAME      = 0x80,
    PASSWORD      = 0x40,
    WILL_RETAIN   = 0x20,
    WILL_QOS1     = 0x08,
    WILL_QOS2     = 0x10,
    WILL          = 0x04,
#if MQTT5
    CLEAN_START   = 0x02
#else
    CLEAN_SESSION = 0x02
#endif
};


#if MQTT5
using MQTT_PROP_STRPAIR = std::pair<std::string,std::string>;

namespace H4AMC_Helpers {
    uint8_t* poke16(uint8_t* p,uint16_t u);
    uint16_t peek16(uint8_t* p);
    uint8_t* encodestring(uint8_t* p,const std::string& s);
    std::string decodestring(uint8_t** p);
    uint8_t* encodeBinary(uint8_t* p, const std::vector<uint8_t>&);
    std::vector<uint8_t> decodeBinary(uint8_t** p);
    uint32_t decodeVariableByteInteger(uint8_t** p);
    uint8_t* encodeVariableByteInteger(uint8_t* p, uint32_t value);
    uint8_t varBytesLength(uint32_t value);
}

enum H4AMC_MQTT5_ReasonCode : uint8_t {
    REASON_SUCCESS                          = 0x00,
    REASON_NORMAL_DISCONNECTION             = 0x00,
    REASON_GRANTED_QOS_0                    = 0x00,
    REASON_GRANTED_QOS_1                    = 0x01,
    REASON_GRANTED_QOS_2                    = 0x02,
    REASON_DISCONNECT_WITH_WILL_MESSAGE     = 0x04,
    REASON_NO_MATCHING_SUBSCRIBERS          = 0x10,
    REASON_NO_SUBSCRIPTION_EXISTED          = 0x11,
    REASON_CONTINUE_AUTHENTICATION          = 0x18,
    REASON_RE_AUTHENTICATE                  = 0x19,
    REASON_UNSPECIFIED_ERROR                = 0x80,
    REASON_MALFORMED_PACKET                 = 0x81,
    REASON_PROTOCOL_ERROR                   = 0x82,
    REASON_IMPLEMENTATION_SPECIFIC_ERROR    = 0x83,
    REASON_UNSUPPORTED_PROTOCOL_VERSION     = 0x84,
    REASON_CLIENT_IDENTIFIER_NOT_VALID      = 0x85,
    REASON_BAD_USER_NAME_OR_PASSWORD        = 0x86,
    REASON_NOT_AUTHORIZED                   = 0x87,
    REASON_SERVER_UNAVAILABLE               = 0x88,
    REASON_SERVER_BUSY                      = 0x89,
    REASON_BANNED                           = 0x8A,
    REASON_SERVER_SHUTTING_DOWN             = 0x8B,
    REASON_BAD_AUTHENTICATION_METHOD        = 0x8C,
    REASON_KEEP_ALIVE_TIMEOUT               = 0x8D,
    REASON_SESSION_TAKEN_OVER               = 0x8E,
    REASON_TOPIC_FILTER_INVALID             = 0x8F,
    REASON_TOPIC_NAME_INVALID               = 0x90,
    REASON_PACKET_IDENTIFIER_IN_USE         = 0x91,
    REASON_PACKET_IDENTIFIER_NOT_FOUND      = 0x92,
    REASON_RECEIVE_MAXIMUM_EXCEEDED         = 0x93,
    REASON_TOPIC_ALIAS_INVALID              = 0x94,
    REASON_PACKET_TOO_LARGE                 = 0x95,
    REASON_MESSAGE_RATE_TOO_HIGH            = 0x96,
    REASON_QUOTA_EXCEEDED                   = 0x97,
    REASON_ADMINISTRATIVE_ACTION            = 0x98,
    REASON_PAYLOAD_FORMAT_INVALID           = 0x99,
    REASON_RETAIN_NOT_SUPPORTED             = 0x9A,
    REASON_QOS_NOT_SUPPORTED                = 0x9B,
    REASON_USE_ANOTHER_SERVER               = 0x9C,
    REASON_SERVER_MOVED                     = 0x9D,
    REASON_SHARED_SUBSCRIPTIONS_NOT_SUPPORTED = 0x9E,
    REASON_CONNECTION_RATE_EXCEEDED         = 0x9F,
    REASON_MAXIMUM_CONNECT_TIME             = 0xA0,
    REASON_SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 0xA1,
    REASON_WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED = 0xA2
};
/* 
    Although the Property Identifier is defined as a Variable Byte Integer, in this version of the
    500
    specification all of the Property Identifiers are one byte long.
    -MQTT v5 specs 
*/
enum H4AMC_MQTT5_Property : uint8_t {
    PROPERTY_INVALID                            = 0x00,
    PROPERTY_PAYLOAD_FORMAT_INDICATOR           = 0x01,     // BYTE
    PROPERTY_MESSAGE_EXPIRY_INTERVAL            = 0x02,     // 4 BYTE INT
    PROPERTY_CONTENT_TYPE                       = 0x03,     // UTF-8 STRING
    PROPERTY_RESPONSE_TOPIC                     = 0x08,     // UTF-8 STRING
    PROPERTY_CORRELATION_DATA                   = 0x09,     // BINARY DATA
    PROPERTY_SUBSCRIPTION_IDENTIFIER            = 0x0B,     // VAR BYTE INT
    PROPERTY_SESSION_EXPIRY_INTERVAL            = 0x11,     // 4 BYTE INT
    PROPERTY_ASSIGNED_CLIENT_IDENTIFIER         = 0x12,     // UTF-8 STRING
    PROPERTY_SERVER_KEEP_ALIVE                  = 0x13,     // 2 BYTE INT
    PROPERTY_AUTHENTICATION_METHOD              = 0x15,     // UTF-8 STRING
    PROPERTY_AUTHENTICATION_DATA                = 0x16,     // BINARY DATA
    PROPERTY_REQUEST_PROBLEM_INFORMATION        = 0x17,     // BYTE
    PROPERTY_WILL_DELAY_INTERVAL                = 0x18,     // 4 BYTE INT
    PROPERTY_REQUEST_RESPONSE_INFORMATION       = 0x19,     // BYTE
    PROPERTY_RESPONSE_INFORMATION               = 0x1A,     // UTF-8 STRING
    PROPERTY_SERVER_REFERENCE                   = 0x1C,     // UTF-8 STRING
    PROPERTY_REASON_STRING                      = 0x1F,     // UTF-8 STRING
    PROPERTY_RECEIVE_MAXIMUM                    = 0x21,     // 2 BYTE INT
    PROPERTY_TOPIC_ALIAS_MAXIMUM                = 0x22,     // 2 BYTE INT
    PROPERTY_TOPIC_ALIAS                        = 0x23,     // 2 BYTE INT
    PROPERTY_MAXIMUM_QOS                        = 0x24,     // BYTE
    PROPERTY_RETAIN_AVAILABLE                   = 0x25,     // BYTE
    PROPERTY_USER_PROPERTY                      = 0x26,     // UTF-8 STRING PAIR
    PROPERTY_MAXIMUM_PACKET_SIZE                = 0x27,     // 4 BYTE INT
    PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE    = 0x28,     // BYTE
    PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE  = 0x29,     // BYTE
    PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE      = 0x2A      // BYTE
};

enum Subscription_Options {
    SUBSCRIPTION_OPTION_QoS_SHIFT                     = 0,
    SUBSCRIPTION_OPTION_NO_LOCAL_SHIFT                = 2,
    SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED_SHIFT     = 3,
    SUBSCRIPTION_OPTION_RETAIN_HANDLING_SHIFT         = 4
};
struct Server_Options {
    // uint32_t session_expiry_interval=MQTT_CONNECT_SESSION_EXPRITY_INTERVAL; // Until being supported in the library.
    uint16_t receive_max= MQTT_CONNECT_RECEIVE_MAXIMUM;
    uint16_t topic_alias_max=0;
    uint32_t maximum_packet_size= MQTT_CONNECT_MAX_PACKET_SIZE;
    bool retain_available=true;
    bool wildcard_subscription_available=true;
    bool subscriptions_identifiers_available=true;
    bool shared_subscription_available=true;
    uint8_t maximum_qos=2;
    std::string response_information;

    uint8_t subId = 0; // Maximum 127.. because of the 7 bit encoding, as VBI.
    std::map<uint16_t, std::string> rec_topic_alias;
    std::map<std::string, uint16_t> pub_topic_alias;
    std::map<uint16_t, std::vector<std::string>> subId2topic;

};

template<typename T>
struct MQTT_Property {
    H4AMC_MQTT5_Property id;
    virtual uint8_t* parse (uint8_t* data) = 0;
    virtual uint8_t* serialize(uint8_t* data, T value) = 0;
    virtual uint8_t* serialize(uint8_t* data) = 0; // Serializes the ID.
    virtual bool is_malformed() { return false; }
    virtual void print() {}
    MQTT_Property(H4AMC_MQTT5_Property i):id(i){}
};
struct MQTT_Property_Numeric : public MQTT_Property<uint32_t> {
    uint32_t value=0;
    void print() override;
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, uint32_t value) override;
    uint8_t* serialize(uint8_t* data) override;

    MQTT_Property_Numeric(H4AMC_MQTT5_Property i):MQTT_Property(i){}
};

struct MQTT_Property_Numeric_1B : public MQTT_Property_Numeric {
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, uint32_t value) override;
    uint8_t* serialize (uint8_t* data) override;
    bool is_malformed() override { return value > 0xff; }
    MQTT_Property_Numeric_1B(H4AMC_MQTT5_Property i):MQTT_Property_Numeric(i){}
};
struct MQTT_Property_Numeric_2B : public MQTT_Property_Numeric {
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, uint32_t value) override;
    uint8_t* serialize (uint8_t* data) override;
    bool is_malformed() override { return value > 0xffff; }
    MQTT_Property_Numeric_2B(H4AMC_MQTT5_Property i):MQTT_Property_Numeric(i){}
};

/* struct MQTT_Property_Numeric_4B : public MQTT_Property_Numeric {
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, uint32_t value) override;
    uint8_t* serialize(uint8_t* data) override;
    MQTT_Property_Numeric_4B(H4AMC_MQTT5_Property i):MQTT_Property_Numeric(i){}
}; */
struct MQTT_Property_Numeric_VBI : public MQTT_Property_Numeric { // Variable Byte Integer
    bool malformed_packet;
    int length;
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, uint32_t value) override;
    uint8_t* serialize(uint8_t* data) override;
    bool is_malformed() override { return malformed_packet; }
    MQTT_Property_Numeric_VBI(H4AMC_MQTT5_Property i):MQTT_Property_Numeric(i){}
};
struct MQTT_Property_Bool : public MQTT_Property_Numeric_1B {
    bool is_malformed() override { return value > 1; }
    void print() override;
    MQTT_Property_Bool(H4AMC_MQTT5_Property i):MQTT_Property_Numeric_1B(i){}
};


//** Perhaps this is better to use the shared memory??? (Or contiguous memory)
/* struct MQTT_Property_Binary : public MQTT_Property<std::pair<uint8_t*,uint32_t>> {
    std::pair<uint8_t*,uint32_t> value;
    uint8_t* parse (uint8_t* data) override{
        value = H4AMC_Helpers::decodeBinary(&data);
    }
    uint8_t* serialize(uint8_t* data, std::pair<uint8_t*,uint32_t> value) override {
        this->value = value;
        return serialize(data);
    }
    uint8_t* serialize(uint8_t* data) override {
        
    }
    void print() override;
    MQTT_Property_Binary(H4AMC_MQTT5_Property i):MQTT_Property(i){}

}; */
struct MQTT_Property_Binary : public MQTT_Property<std::vector<uint8_t>> {
    std::vector<uint8_t> value;
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, std::vector<uint8_t> value) override;
    uint8_t* serialize(uint8_t* data) override;
    void print() override;
    MQTT_Property_Binary(H4AMC_MQTT5_Property i):MQTT_Property(i){}

};
struct MQTT_Property_String : public MQTT_Property<std::string> {
    std::string value;
    void print() override;
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, std::string value) override;
    uint8_t* serialize(uint8_t* data) override;

    MQTT_Property_String(H4AMC_MQTT5_Property i):MQTT_Property(i){}
};

struct MQTT_Property_StringPair : public MQTT_Property<MQTT_PROP_STRPAIR> {
    MQTT_PROP_STRPAIR value;
    uint8_t* parse (uint8_t* data) override;
    uint8_t* serialize(uint8_t* data, MQTT_PROP_STRPAIR value) override;
    uint8_t* serialize(uint8_t* data) override;

    void print() override;
    MQTT_Property_StringPair():MQTT_Property(PROPERTY_USER_PROPERTY){}
};
using USER_PROPERTIES_MAP = std::map<std::string,std::string>;
struct MQTT_Properties {
/* //    uint8_t payload_format_indicator;
//   uint32_t message_expiry_interval;
//    std::string content_type;
//    std::string response_topic;

//    std::string correlation_data;
//    uint32_t subscription_identifier;
//    uint32_t session_expiry_interval;
//    std::string assigned_client_identifier;
//    uint16_t server_keep_alive;
//    std::string authentication_method;
//    std::string authentication_data;
//    bool request_problem_information;

//    uint32_t will_delay_interval;
//    bool request_response_information;
//    std::string response_information;
//    std::string server_reference;

//    std::string reason_string;
//    uint16_t receive_maximum;
//    uint16_t topic_alias_maximum;
//    uint16_t topic_alias;
//    uint8_t maximum_qos;
//    bool retain_available;
//    uint32_t maximum_packet_size;
//    bool wildcard_subscription_available;
//    bool subscription_identifiers_available;
//    bool shared_subscription_available; */
    std::vector<std::unique_ptr<MQTT_Property_Numeric>> numeric_props;
    std::vector<MQTT_Property_String> string_props;
    std::vector<MQTT_Property_Binary> binary_props;
    USER_PROPERTIES_MAP user_properties;
    std::vector<H4AMC_MQTT5_Property> available_properties;

    // For the user.
    std::string operator[](const std::string& s) { return user_properties.count(s) ? user_properties[s] : std::string{}; }
    USER_PROPERTIES_MAP getUserProperties() { return user_properties; }

    bool isAvailable(H4AMC_MQTT5_Property p);
    // Make a template for this??
    static uint8_t* serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, uint32_t value);
    static uint8_t* serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, std::string value);
    static uint8_t* serializeProperty(H4AMC_MQTT5_Property p, uint8_t* data, std::vector<uint8_t> value);
    static uint8_t* serializeUserProperty(uint8_t *data, MQTT_PROP_STRPAIR value);

    std::pair<H4AMC_MQTT5_ReasonCode,uint8_t*> parseProperties(uint8_t* data);

    std::string getStringProperty(H4AMC_MQTT5_Property p);

    uint32_t getNumericProperty(H4AMC_MQTT5_Property p);

};

struct MQTT5MessageProperties {
    uint8_t payload_format_indicator;
    uint32_t message_expiry_interval;
    std::string content_type;
    std::string response_topic;
    std::vector<uint8_t> correlation_data;
    std::map<std::string,std::string> user_properties;
};
struct MQTT5WillProperties : public MQTT5MessageProperties {
    uint32_t will_delay_interval;
};
struct MQTT5PublishProperties : public MQTT5MessageProperties {
    uint16_t topic_alias;
};
#endif

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
                bool            malformed_packet=false;

#if MQTT5
                MQTT_Properties*  properties = nullptr;                
#endif
                
#if H4AMC_DEBUG
#if MQTT5
        static std::map<H4AMC_MQTT5_ReasonCode,char*> rcnames;
        static std::map<H4AMC_MQTT5_Property,char*> propnames;
#endif
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
using H4AMC_cbProperties    =std::function<void(USER_PROPERTIES_MAP)>; // in MQTT5
using H4AMC_MEM_POOL        =std::unordered_set<uint8_t*>;

class Packet;
class ConnectPacket;
class PublishPacket;

enum {
    H4AMC_DISCONNECTED,
    H4AMC_RUNNING,
    H4AMC_FATAL
};

class mbx {
    public:
        static  H4AMC_MEM_POOL  pool;
                bool            managed;
                int             len=0;
                uint8_t*        data=nullptr;
                uint8_t         flags=0;
        mbx(){}
        mbx(uint8_t* p,size_t s,bool copy=true,uint8_t f=0);
        // 
         ~mbx(){} // absolutely do not never nohow free the data pointer here! It must stay alive till it is ACKed

                void            clear();
        static  void            clear(uint8_t*);
        static  uint8_t*        getMemory(size_t size);
        static  void            dump(size_t slice=32){
#if H4AMC_DEBUG
            H4AMC_PRINT1("Memory POOL DUMP s=%d\n", pool.size());
            for (auto &p:pool) Serial.printf("%p\t",p);
            Serial.println();
            for(auto &p:pool) dumphex(p,slice);
#endif
        }
};

class H4AsyncMQTT {
        friend class Packet;
        friend class ConnectPacket;
        friend class PublishPacket;
        friend class mqttTraits;
                uint32_t            _state=H4AMC_DISCONNECTED;
#if MQTT5
                H4AMC_cbProperties  _cbMQTTConnect=nullptr;
#else
                H4_FN_VOID          _cbMQTTConnect=nullptr;
#endif
                H4_FN_VOID          _cbMQTTDisconnect=nullptr;
                H4AMC_cbError       _cbMQTTError=nullptr;

                H4AMC_cbMessage     _cbMessage=nullptr;
#ifdef MQTT5
                bool                _cleanStart=true;
                Server_Options*     _serverOptions=nullptr;
                std::map<PacketHeader, std::vector<std::shared_ptr<std::pair<std::string, std::string>>>> _user_props;                


#else // MQTT5
                bool                _cleanSession=true;
#endif // MQTT5
                std::string         _clientId;
        static  H4_INT_MAP          _errorNames;
        static  H4AMC_PACKET_MAP    _inbound;
                uint32_t            _keepalive=KEEP_ALIVE_INTERVAL;
                size_t              _nextId=1000;
        static  H4AMC_PACKET_MAP    _outbound;
                std::string         _password;
                std::string         _url;
                std::string         _username;
#if MQTT5
                MQTT5WillProperties _willProperties;
#endif
                std::string         _willPayload;
                uint8_t             _willQos=0;
                bool                _willRetain=false;
                std::string         _willTopic;
               
                void                _ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout); // inout true=INBOUND false=OUTBOUND
                void                _ACKoutbound(uint16_t id){ _ACK(&_outbound,id,false); }

                void                _startClean();
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
                size_t inline       getMaxPayloadSize(){ return (_HAL_maxHeapBlock() - H4T_HEAP_SAFETY) / 2 ; }
#if MQTT5
                void                onConnect(H4AMC_cbProperties callback){ _cbMQTTConnect=callback; }
#else
                void                onConnect(H4_FN_VOID callback){ _cbMQTTConnect=callback; }
#endif
                void                onDisconnect(H4_FN_VOID callback){ _cbMQTTDisconnect=callback; }
                void                onError(H4AMC_cbError callback){ _cbMQTTError=callback; }
                void                onMessage(H4AMC_cbMessage callback){ _cbMessage=callback; }
                void                publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H);
                void                publish(const char* topic,const char* payload, size_t length, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H);
                template<typename T>
                void publish(const char* topic,T v,const char* fmt="%d",uint8_t qos=0,bool retain=false MQTTPUBLISHPROPERTIES_API_H){
                    char buf[16];
                    sprintf(buf,fmt,v);
                    publish(topic, reinterpret_cast<const uint8_t*>(buf), strlen(buf), qos, retain);
                }
//              Coalesce templates when C++17 available (if constexpr (x))
                void xPublish(const char* topic,const char* value, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value),strlen(value),qos,retain MQTTPUBLISHPROPERTIES_CALL);
                }
                void xPublish(const char* topic,String value, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.length(),qos,retain MQTTPUBLISHPROPERTIES_CALL);
                }
                void xPublish(const char* topic,std::string value, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H) {
                    publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.size(),qos,retain MQTTPUBLISHPROPERTIES_CALL);
                }
                template<typename T>
                void xPublish(const char* topic,T value, uint8_t qos=0,  bool retain=false MQTTPUBLISHPROPERTIES_API_H) {
                    publish(topic,reinterpret_cast<uint8_t*>(&value),sizeof(T),qos,retain MQTTPUBLISHPROPERTIES_CALL);
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
#if MQTT5
                // Valid for CONNECT, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, UNSUBSCRIBE, DISCONNECT, AUTH
                bool addUserProp(PacketHeader header, std::string key, std::string value, bool checked=false){
                    if (!checked && (header == CONNACK || header == SUBACK || header == UNSUBACK))
                        return false;
                    _user_props[header].push_back(std::make_shared<std::pair<std::string, std::string>>(key, value));
                    // _user_props[header] = std::make_shared<std::pair<std::string, std::string>>(key, value);
                }
                bool addUserProp(std::initializer_list<PacketHeader> headers, std::string key, std::string value){
                    for (auto header : headers)
                        if (header == CONNACK || header == SUBACK || header == UNSUBACK)
                            return false;
                    for(auto header : headers)
                        addUserProp(header, key, value, true);
                    return true;
                }
                void resetUserProps() { _user_props.clear(); }
#endif
//
//              DO NOT CALL ANY FUNCTION STARTING WITH UNDERSCORE!!! _
//
                void               _notify(int e,int info=0);
                void               dump(); // null if no debug
};