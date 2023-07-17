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
#include<H4AsyncTCP.h>
#include"h4amc_common.h"
#include"Properties.h"


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
#define H4AMC_SESSION_CLEAN_START CLEAN_START
#else
#define H4AMC_SESSION_CLEAN_START CLEAN_SESSION
#endif

#if MQTT5
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
};
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT

struct SubscriptionResource {
    H4AMC_cbMessage cb;
    std::set<std::string> topix;
    // SubscriptionResource():cb(nullptr) {}
    // SubscriptionResource(H4AMC_cbMessage cb, std::set<std::string> topics) : cb(cb), topix(topics) {}
};
#endif
#endif
class H4AMC_SubscriptionOptions {
    uint8_t qos;
#if MQTT5
    bool nl;
    bool rap;
    uint8_t rh;
    H4AMC_cbMessage cb;
    USER_PROPERTIES_MAP user_properties;
#endif
public:
    // H4AMC_SubscriptionOptions() {}
    H4AMC_SubscriptionOptions(int qos=0) : qos(qos) {}
#if MQTT5
    H4AMC_SubscriptionOptions(uint8_t QoS,
                              H4AMC_cbMessage callback=nullptr,
                              bool no_local = MQTT5_SUBSCRIPTION_OPTION_NO_LOCAL,
                              bool retain_as_published = MQTT5_SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED,
                              uint8_t retain_handling = MQTT5_SUBSCRIPTION_OPTION_RETAIN_HANDLING) 
                              : qos(QoS), cb(callback), nl(no_local), rap(retain_as_published), rh(retain_handling)
    {
    }
            bool                getNoLocal(){ return nl; }
            bool                getRetainAsPublished(){ return rap; }
            uint8_t             getRetainHandling(){ return rh; }
            H4AMC_cbMessage     getCallback() { return cb; }
            USER_PROPERTIES_MAP& getUserProperties() { return user_properties; }
#endif
            uint8_t             getQos(){ return qos; }

};


class H4AMC_PublishOptions {
    bool retained;

#if MQTT5
    MQTT5PublishProperties props;
#endif
public:
    H4AMC_PublishOptions(bool retained = false) : retained(retained) {}

    bool getRetained() { return retained; }
#if MQTT5
    MQTT5PublishProperties& getProperties() { return props; }
#endif
};

class H4AMC_WillOptions {
    bool retained;

#if MQTT5
    MQTT5WillProperties props;
#endif
public:
    H4AMC_WillOptions(bool retained = false) : retained(retained) {}

    bool getRetained() { return retained; }
#if MQTT5
    MQTT5WillProperties& getProperties() { return props; }
#endif
};

class mqttTraits {             
                std::string     _decodestring(uint8_t** p);
        inline  uint16_t        _peek16(uint8_t* p){ return (*(p+1))|(*p << 8); }
        inline  uint16_t        _parse16(uint8_t** p) { auto r=_peek16(*p);  return *p+=2, r;}
        inline  uint8_t         _parse8(uint8_t** p) { return *(*p)++;}
        
    public:
                uint8_t         conackflags=0;
                uint32_t        remlen=0;
                uint8_t         offset=0;
                uint8_t         flags=0;

                uint8_t*        protocolpayload=nullptr;
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
                std::vector<uint8_t> subreasons;
                uint8_t         reasoncode=0;
#if MQTT5
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                uint32_t        subscription_id; // Subscription ID
#endif
                std::shared_ptr<MQTT_Properties> properties;
                // MQTT_Properties*  properties = nullptr;     
                MQTT_PROP_PARSERET     initiateProperties(uint8_t* start) { properties=std::make_shared<MQTT_Properties>();
                    auto ret = properties->parseProperties(start);
                    malformed_packet=ret.first;
                    if (malformed_packet){
                        H4AMC_PRINT1("ERROR: Malformed Packet %d\n", ret.first);
                    }
                    return ret;
                }
#endif
                
#if H4AMC_DEBUG
#if MQTT5
        static std::map<H4AMC_MQTT_ReasonCode,char*> rcnames;
        static std::map<H4AMC_MQTT5_Property,char*> propnames;
#else
        static  std::map<uint8_t,char*> connacknames;
        static  std::map<uint8_t,char*> subacknames;
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


class Packet;
class ConnectPacket;
class PublishPacket;

enum {
    H4AMC_DISCONNECTED,
    H4AMC_CONNECTING,
    H4AMC_RUNNING,
    H4AMC_FATAL
};

struct WillMessage {
    std::string         payload;
    uint8_t             qos=0;
    bool                retain=false;
    std::string         topic;
#if MQTT5
    MQTT5WillProperties props;
#endif
} ;

class H4Authenticator;
enum NetworkState : uint8_t {
    H4AMC_NETWORK_DISCONNECTED,
    H4AMC_NETWORK_CONNECTED
};
class H4AsyncMQTT {
        friend class Packet;
        friend class ConnectPacket;
        friend class PublishPacket;
        friend class mqttTraits;
                NetworkState        _networkState=H4AMC_NETWORK_CONNECTED;
                uint32_t            _state=H4AMC_DISCONNECTED;
#if MQTT5
                H4AMC_cbProperties  _cbMQTTConnect=nullptr;
                bool                _cleanStart=true;
                Server_Options*     _serverOptions=nullptr;
                std::map<PacketHeader, std::shared_ptr<USER_PROPERTIES_MAP>> _user_static_props;                
                std::map<PacketHeader, H4AMC_FN_DYN_PROPS> _user_dynamic_props;
                MQTT5WillProperties _willProperties;
                H4AMC_FN_STRING     _cbRedirect;
                H4AMC_FN_STRING     _cbReason;
                H4Authenticator*    _authenticator=nullptr;
                std::map<uint16_t, std::string> _rx_topic_alias;
                // std::map<std::string, uint16_t> _tx_topic_alias;
                std::vector<std::string> _tx_topic_alias; // vector<string> because we manage it.
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                
                H4AMC_SUBRES_MAP    _subsResources;
                H4AMC_PK_SUBIDS_MAP _packSubIds;
                uint32_t            subId = 0; // VBI.

                void                _proposeDeletion(uint32_t packetId, uint32_t subId) {_packSubIds[packetId]=subId;};
                void                _confirmDeletion(uint32_t packId) { _subsResources.erase(_packSubIds[packId]); _packSubIds.erase(packId); }
#endif

                
#else // MQTT5
                H4AMC_FN_VOID       _cbMQTTConnect=nullptr;
                bool                _cleanSession=true;
#endif // MQTT5
                H4AMC_FN_VOID       _cbMQTTDisconnect=nullptr;
                H4AMC_cbError       _cbMQTTError=nullptr;
                H4AMC_cbMessage     _cbMessage=nullptr;
                H4AMC_cbPublish     _cbPublish=nullptr;
                std::string         _clientId;
        static  H4_INT_MAP          _errorNames;
        // static  H4AMC_PACKET_MAP    _inbound;
        static  std::set<int>       _inbound;
                uint32_t            _keepalive;
                size_t              _nextId=1000;
        static  H4AMC_PACKET_MAP    _outbound;
                std::string         _password;
                std::string         _url;
                std::string         _username;
                WillMessage         _will;
#if H4AT_TLS
                std::vector<uint8_t> _caCert;
                std::vector<uint8_t> _privkey;
                std::vector<uint8_t> _privkeyPass;
                std::vector<uint8_t> _clientCert;
#endif
               
                void                _ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout); // inout true=INBOUND false=OUTBOUND
                void                _ACKoutbound(uint16_t id){ _ACK(&_outbound,id,false); }

                void                _startClean();
                void                _clearQQ(H4AMC_PACKET_MAP* m);
                void                _startPinging(uint32_t keepalive=KEEP_ALIVE_INTERVAL);
                void                _connect();
                void                _destroyClient();
#if MQTT5
                void                _protocolError(H4AMC_MQTT_ReasonCode reason);
                void                _handleConnackProps(MQTT_Properties& props);
                void                _redirect(MQTT_Properties& props);
                void                _addUserProp(PacketHeader header, std::shared_ptr<USER_PROPERTIES_MAP> nv) {
                                        _user_static_props[header]=nv;
                                    }
                void                _addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f) {
                                        _user_dynamic_props[header]=f;
                                    }
                void                _handleReasonString(MQTT_Properties& props) {
                                        if (_cbReason && props.isAvailable(PROPERTY_REASON_STRING))
                                            _cbReason(props.getStringProperty(PROPERTY_REASON_STRING));
                                    }
#endif
                void                _handlePacket(uint8_t* data, size_t len, int n_handled=0);
                void                _handlePublish(mqttTraits T);
        inline  void                _hpDespatch(mqttTraits T);
        // inline  void                _hpDespatch(uint16_t id);
                void                _resendPartialTxns(bool availableSession);
        inline  void                _runGuard(H4AMC_FN_VOID f);
        template<typename T>
        inline  T                   _runGuard(H4_FN_COUNT f,T dummy) { // For numerics only.
                                        if(_state==H4AMC_RUNNING) return f();
                                        else _notify(0,H4AMC_USER_LOGIC_ERROR);
                                        return 0;
                                    }
        inline  void                _startReconnector();
    public:
                H4AsyncClient*      _h4atClient;
        H4AsyncMQTT();
                void                connect(const char* url,const char* auth="",const char* pass="",const char* clientId="",bool clean=true);
                void                disconnect(H4AMC_MQTT_ReasonCode reason=REASON_NORMAL_DISCONNECTION);
        static  std::string         errorstring(int e);
                std::string         getClientId(){ return _clientId; }
                void                onDisconnect(H4AMC_FN_VOID callback){ _cbMQTTDisconnect=callback; }
#if MQTT5
                void                onConnect(H4AMC_cbProperties callback){ _cbMQTTConnect=callback; }
                void                onRedirect(H4AMC_FN_STRING f) { _cbRedirect=f; }
                void                onReason(H4AMC_FN_STRING f) { _cbReason=f; }
                void                setAuthenticator(H4Authenticator* authenticator) { _authenticator = authenticator; }
                bool                availableTXAlias(const std::string &topic) { auto& ta=_tx_topic_alias; return std::find(ta.begin(), ta.end(), topic) != ta.end(); } // For any fresh start, _tx_topic_alias is empty
                // bool                availableTXAlias(const std::string &topic) { return _tx_topic_alias.count(topic); } // For any fresh start, _tx_topic_alias is empty
                bool                availableTXAliasSpace()
                                    {
                                        if (_serverOptions)
                                            return _serverOptions->topic_alias_max > _tx_topic_alias.size();
                                        _notify(0,H4AMC_NO_SERVEROPTIONS);
                                        return false;
                                    }
                uint16_t            assignTXAlias(const std::string& topic) { _tx_topic_alias.push_back(topic); return _tx_topic_alias.size()+1; }
                uint16_t            getTXAlias(const std::string& topic) { // [ ] Test operations.
                                        auto& ta=_tx_topic_alias; 
                                        auto it = std::find(ta.begin(), ta.end(), topic);
                                        return std::distance(ta.begin(),it)+1;
                                    }
                void                clearAliases() { _tx_topic_alias.clear(), _rx_topic_alias.clear(); }
#else
                void                onConnect(H4AMC_FN_VOID callback){ _cbMQTTConnect=callback; }
#endif

                /* secureTLS
                    Make sure activating H4AT_TLS and passing a valid secure url "https", NOT "http" to connect securely.
                    All keys/certificates must contain the null terminator if PEM encoded
                 */
                bool                secureTLS(const u8_t *ca, size_t ca_len, const u8_t *privkey = nullptr, size_t privkey_len=0,
                                            const u8_t *privkey_pass = nullptr, size_t privkey_pass_len = 0,
                                            const u8_t *cert = nullptr, size_t cert_len = 0);
                void                informNetworkState(NetworkState state) { _networkState = state; }
                void                onError(H4AMC_cbError callback){ _cbMQTTError=callback; }
                void                onMessage(H4AMC_cbMessage callback){ _cbMessage=callback; }
                void                onPublish(H4AMC_cbPublish callback){ _cbPublish=callback; }
                uint16_t            publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0,  H4AMC_PublishOptions opts={});
                uint16_t            publish(const char* topic,const char* payload, size_t length, uint8_t qos=0,  H4AMC_PublishOptions opts={});
                template<typename T>
                uint16_t publish(const char* topic,T v,const char* fmt="%d",uint8_t qos=0, H4AMC_PublishOptions opts={}){
                    char buf[16];
                    sprintf(buf,fmt,v);
                    return publish(topic, reinterpret_cast<const uint8_t*>(buf), strlen(buf), qos, opts);
                }
//              Coalesce templates when C++17 available (if constexpr (x))
                uint16_t xPublish(const char* topic,const char* value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const uint8_t*>(value),strlen(value),qos,opts);
                }
                uint16_t xPublish(const char* topic,String value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.length(),qos,opts);
                }
                uint16_t xPublish(const char* topic,std::string value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<const uint8_t*>(value.c_str()),value.size(),qos,opts);
                }
                template<typename T>
                uint16_t xPublish(const char* topic,T value, uint8_t qos=0, H4AMC_PublishOptions opts={}) {
                    return publish(topic,reinterpret_cast<uint8_t*>(&value),sizeof(T),qos,opts);
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
                void               setWill(const char* topic, uint8_t qos, const char* payload = nullptr, H4AMC_WillOptions opts_retain={});
                /**! subscribe
                 *              For MQTT v3.3, one can pass QoS value directly.
                 * \return 
                */
                uint32_t           subscribe(const char* topic, H4AMC_SubscriptionOptions opts_qos={});
                uint32_t           subscribe(std::initializer_list<const char*> topix, H4AMC_SubscriptionOptions opts_qos={});
                void               unsubscribe(const char* topic);
                void               unsubscribe(std::initializer_list<const char*> topix);
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
                void               unsubscribe(uint32_t subscription_id);
#endif
#if MQTT5
                // Valid for CONNECT, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, UNSUBSCRIBE, DISCONNECT, AUTH

                bool addUserProp(PacketHeader header, USER_PROPERTIES_MAP user_properties){
                    if (header == CONNACK || header == SUBACK || header == UNSUBACK)
                        return false;
                    // _user_static_props[header].push_back(std::pair<std::string, std::string>(key, value));
                    // _user_static_props[header] = std::make_shared<std::pair<std::string, std::string>>(key, value);
                    _addUserProp(header, std::make_shared<USER_PROPERTIES_MAP>(user_properties));
                }
                bool addUserProp(std::initializer_list<PacketHeader> headers, USER_PROPERTIES_MAP user_properties){
                    for (auto header : headers)
                        if (header == CONNACK || header == SUBACK || header == UNSUBACK)
                            return false;
                    
                    auto shared = std::make_shared<USER_PROPERTIES_MAP>(user_properties);
                    for(auto header : headers)
                        _addUserProp(header, shared);
                    return true;
                }

                bool addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f){
                    if (header == CONNACK || header == SUBACK || header == UNSUBACK)
                        return false;
                    _addDynamicUserProp(header, f);
                }
                bool addDynamicUserProp(std::initializer_list<PacketHeader> headers, H4AMC_FN_DYN_PROPS f){
                    for (auto header : headers)
                        if (header == CONNACK || header == SUBACK || header == UNSUBACK)
                            return false;
                    
                    for(auto header : headers)
                        _addDynamicUserProp(header, f);
                    return true;
                }

                
                void resetUserProps() { _user_static_props.clear(); _user_dynamic_props.clear(); }
#endif
//
//              DO NOT CALL ANY FUNCTION STARTING WITH UNDERSCORE!!! _
//
                void               _notify(int e,int info=0);
                void               dump(); // null if no debug
};

class H4Authenticator {
    enum H4AMC_AuthState : uint8_t {
        AUTH_STATE_CONTINUE,
        AUTH_STATE_SUCCESS
    };
            std::string     _method;
            int             _state=0;

    virtual H4AMC_AuthState         handle(std::vector<uint8_t> data)=0;
};