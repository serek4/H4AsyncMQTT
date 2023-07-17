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
#include <H4AsyncMQTT.h>
#include "Packet.h"

H4AMC_MEM_POOL          mbx::pool;
std::set<int>           H4AsyncMQTT::_inbound;
H4AMC_PACKET_MAP        H4AsyncMQTT::_outbound;

H4_INT_MAP H4AsyncMQTT::_errorNames={
#if H4AMC_DEBUG
    {H4AMC_CONNECT_FAIL,"CONNECT FAIL"},
    {H4AMC_BAD_FINGERPRINT,"H4AMC_BAD_FINGERPRINT"},
    {H4AMC_NO_FINGERPRINT,""},
    {H4AMC_NO_SSL,"H4AMC_NO_SSL"},
    {H4AMC_UNWANTED_FINGERPRINT,"H4AMC_UNWANTED_FINGERPRINT"},
    {H4AMC_SUBSCRIBE_FAIL,"H4AMC_SUBSCRIBE_FAIL"},
    {H4AMC_INBOUND_QOS_ACK_FAIL,"H4AMC_INBOUND_QOS_ACK_FAIL"},
    {H4AMC_OUTBOUND_QOS_ACK_FAIL,"H4AMC_OUTBOUND_QOS_ACK_FAIL"},
    {H4AMC_INBOUND_PUB_TOO_BIG,"H4AMC_INBOUND_PUB_TOO_BIG"},
    {H4AMC_OUTBOUND_PUB_TOO_BIG,"H4AMC_OUTBOUND_PUB_TOO_BIG"},
    {H4AMC_BOGUS_PACKET,"H4AMC_BOGUS_PACKET"},
    {H4AMC_X_INVALID_LENGTH,"H4AMC_X_INVALID_LENGTH"},
//    {H4AMC_KEEPALIVE_TOO_LONG,"KEEPALIVE TOO LONG: MUST BE < H4AS_SCAVENGE_FREQ"},
    {H4AMC_USER_LOGIC_ERROR,"USER LOGIC ERROR"},
#endif
};
#if MQTT5
namespace H4AMC_Helpers {
    uint8_t* poke16(uint8_t* p,uint16_t u){
        *p++=(u & 0xff00) >> 8;
        *p++=u & 0xff;
        return p;
    }
    uint16_t peek16(uint8_t* p){ return (*(p+1))|(*p << 8); }
    
    uint8_t* encodestring(uint8_t* p,const std::string& s){
        p=poke16(p,s.size());
        memcpy(p,s.data(),s.size());
        return p+s.size();
    }

    std::string decodestring(uint8_t** p){
        size_t tlen=peek16(*p);//payload+=2;
        std::string rv((const char*) *(p)+2,tlen);
        *p+=2+tlen;
        return rv;
    }
    uint8_t* encodeBinary(uint8_t* p, const std::vector<uint8_t>& data)
    {
        p=poke16(p,data.size());
        std::copy_n(data.begin(), data.size(), p);
        return p+data.size();
    }
    std::vector<uint8_t> decodeBinary(uint8_t** p)
    {
        uint16_t len=peek16(*p);
        *p+=2;
        std::vector<uint8_t> rv;
        rv.reserve(len);
        std::copy_n(*p, len, std::back_inserter(rv));
        *p+=len;
        return rv;
    }
    uint32_t decodeVariableByteInteger(uint8_t** p) {
        uint32_t multiplier = 1;
        uint32_t value = 0;
        uint8_t encodedByte;
        do {
            encodedByte = *(*p)++;  // Ensure this is the right pointer dealing.
            value += (encodedByte & 127) * multiplier;
            multiplier *= 128;
            if (multiplier > 128 * 128 * 128)
                return UINT32_MAX; // Malformed
        } while ((encodedByte & 128) != 0);
        return value;
    }
    uint8_t* encodeVariableByteInteger(uint8_t* p, uint32_t value) {
        do {
            uint8_t encodedByte = value % 128;
            value /= 128;
            // if there are more data to encode, set the top bit of this byte
            if (value > 0)
                encodedByte |= 128;
            *p++=encodedByte;
        } while (value > 0);
        return p;
    }
    uint8_t varBytesLength(uint32_t value){
        if (value > 268435455) return 0; // Error. 4 bytes max
        uint8_t len=1;
        while(value>127){
            value>>=7;
            len++;
        }
        return len;
    }
}
#endif
H4AsyncMQTT::H4AsyncMQTT(){

}

void H4AsyncMQTT::_ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout){ /// refakta?
    if(m->count(id)){
        uint8_t* data=((*m)[id]).data;
        mbx::clear(data);
        m->erase(id);
    } else _notify(inout ? H4AMC_INBOUND_QOS_ACK_FAIL:H4AMC_OUTBOUND_QOS_ACK_FAIL,id); //H4AMC_PRINT("WHO TF IS %d???\n",id);
    mbx::dump();
}

void H4AsyncMQTT::_startClean(){
    H4AMC_PRINT4("_startClean clrQQ inbound\n");
    _inbound.clear();
    H4AMC_PRINT4("_startClean clrQQ outbound\n");
    _clearQQ(&_outbound);
}

void H4AsyncMQTT::_clearQQ(H4AMC_PACKET_MAP* m){
    for(auto &i:*m) mbx::clear(i.second.data);
    m->clear();
}

void H4AsyncMQTT::_startPinging(uint32_t keepalive)
{
    _keepalive = keepalive;
    H4AMC_PRINT1("KA = %d\n",_keepalive - H4AMC_HEADROOM);
    static uint8_t PING[]={PINGREQ,0};    
    h4.every(_keepalive - H4AMC_HEADROOM,[=]{ // [ ] Change rate if _keepalive changes on CONNACK.
        if(_state==H4AMC_RUNNING){//} && ((millis() - _h4atClient->_lastSeen) > _keepalive)){ // 100 = headroom
            H4AMC_PRINT1("MQTT PINGREQ\n");
            _h4atClient->TX(PING,2,false); /// optimise
        } //else Serial.printf("No ping: activity %d mS ago\n",(millis() - _h4atClient->_lastSeen));
    },nullptr,H4AMC_KA_ID,true);
}

void H4AsyncMQTT::_redirect(MQTT_Properties& props)
{
    std::string reference;
    if (props.isAvailable(PROPERTY_SERVER_REFERENCE)) {
        H4AMC_PRINT1("Server Reference=%s\n", props.getStringProperty(PROPERTY_SERVER_REFERENCE).c_str());
        reference=props.getStringProperty(PROPERTY_SERVER_REFERENCE);
    } else {
        H4AMC_PRINT1("Server Reference not available\n");
    }
    if (_cbRedirect) (_cbRedirect(reference));
}

void H4AsyncMQTT::_connect(){
    
    H4AMC_PRINT1("_connect %s _state=%d\n",_url.data(),_state);
    _h4atClient->onConnect([=](){
        H4AMC_PRINT1("on TCP Connect\n");
        _h4atClient->nagle(true);
        h4.cancelSingleton(H4AMC_RCX_ID);
        _startPinging();
        h4.queueFunction([=]{ ConnectPacket cp{this}; }); // offload required for esp32 to get off tcpip thread
    });

    _h4atClient->onDisconnect([=]{
        H4AMC_PRINT1("onDisconnect - start reconnector\n");
        if(_state!=H4AMC_DISCONNECTED) if(_cbMQTTDisconnect) _cbMQTTDisconnect();
        _state=H4AMC_DISCONNECTED;
//        Serial.printf("CREATE NEW CLIENT!\n");
        _h4atClient=new H4AsyncClient;
#if MQTT5
        clearAliases(); // valid per network session
#endif
        _startReconnector();
    });

    _h4atClient->onError([=](int error,int info){
        H4AMC_PRINT1("onError %d info=%d\n",error,info);
        /*
        if(_state!=H4AMC_DISCONNECTED){
            if(error==ERR_OK || error==ERR_RST || error==ERR_ABRT){
//                Serial.printf("IGNORING OK RST ABRT e=%d\n",error);
                _h4atClient->_shutdown();
            } else _notify(error,info);
        } //else Serial.printf("NOT CONNECTED!\n");
        */
        return true;
    });
    _h4atClient->onConnectFail([=](){
        H4AMC_PRINT1("onConnectFail - start reconnector\n");
        _state = H4AMC_DISCONNECTED;
        _h4atClient=new H4AsyncClient;
        _startReconnector();
    });

    _h4atClient->onRX([=](const uint8_t* data,size_t len){ _handlePacket((uint8_t*) data,len); });
    _h4atClient->connect(_url);
    _startReconnector();
}

void H4AsyncMQTT::_destroyClient() {
    H4AMC_PRINT1("DESTROY CLIENT\n");
    if (_h4atClient && _h4atClient->connected()) {
        _h4atClient->close();
    }
    _h4atClient=nullptr; // OR the callback?
}
void H4AsyncMQTT::_hpDespatch(mqttTraits P){ 
    if(_cbMessage) {
#if H4AMC_DEBUG
        if(P.topic=="pango"){
            H4AMC_PRINT1("H4AMC INTERCEPTED\n");
            std::string pl((const char*) P.payload,P.plen);
            if(pl=="mem"){
                Serial.printf("FH: %u\nMB: %u\nMP: %d\n",_HAL_freeHeap(),_HAL_maxHeapBlock(),getMaxPayloadSize());
            } 
            else if(pl=="info"){ 
                Serial.printf("H4AT  Vn: %s\n",H4AT_VERSION);
//                Serial.printf("CHECK FP: %d\n",H4AT_CHECK_FINGERPRINT);
//                Serial.printf("SAFEHEAP: %d\n",H4T_HEAP_SAFETY);
                Serial.printf("H4AMC Vn: %s\n",H4AMC_VERSION);
                Serial.printf("NRETRIES: %d\n",H4AMC_MAX_RETRIES);
#if MQTT5
                Serial.printf("start : %s\n",_cleanStart ? "clean":"dirty");
#else
                Serial.printf("session : %s\n",_cleanSession ? "clean":"dirty");
#endif
                Serial.printf("clientID: %s\n",_clientId.data());
                Serial.printf("keepaliv: %d\n",_keepalive);
            }
            else if(pl=="dump"){ dump(); }
        }
        else
#endif
        _cbMessage(P.topic.data(), P.payload, P.plen, P.qos, P.retain, P.dup);
    }
}


void H4AsyncMQTT::_handlePacket(uint8_t* data, size_t len, int n_handled){
    H4AMC_DUMP4(data,len);
    if(data[0]==PINGRESP){ // [ ] _ACKoutbound of UNSUBACK packet?
        H4AMC_PRINT1("MQTT %s\n",mqttTraits::pktnames[data[0]]);
        return; // early bath
    }
    mqttTraits traits(data,len);
    if(traits.malformed_packet)
    {
        H4AMC_PRINT1("Malformed packet! DISCONNECT\n");
        // [ ] MAY Send a DISCONNECT packet to the server with a Reason Code of 0x81 (Malformed Packet)
        // _disconnect(MALFORMED_PACKET);
        _destroyClient();
        return;
    }
    auto i=traits.start();
    uint16_t id=traits.id;
#if MQTT5
    auto props = traits.properties ? *(traits.properties) : MQTT_Properties();
#endif
    auto rcode = traits.reasoncode;
    switch (traits.type)
    {
    case CONNACK:
    {
        switch (rcode)
        {
#if MQTT5
        case REASON_SUCCESS:
            if (!traits.properties) {
                H4AMC_PRINT1("INCOMPLIAT SERVER!\n");
                return;
            }
            _handleConnackProps(*(traits.properties));
#else
        case 0x00:
#endif
        {
                
            _state=H4AMC_RUNNING;
            bool session=i[0] & 0x01; // Check CONNACK session flag, and reflect on _resendPartialTxns()

            _ACKoutbound(0); // ACK connect to clear it from POOL.
            if (!session){
                _startClean();
            }
            _resendPartialTxns(session);
            H4AMC_PRINT1("CONNECTED FH=%u MaxPL=%u SESSION %s\n",_HAL_maxHeapBlock(),getMaxPayloadSize(),session ? "DIRTY":"CLEAN");
#if H4AMC_DEBUG
            SubscribePacket pango(this,"pango",0); // internal info during beta...will be moved back inside debug #ifdef
#endif
#if MQTT5
            if(_cbMQTTConnect) _cbMQTTConnect(traits.properties->getUserProperties());
#else
            if(_cbMQTTConnect) _cbMQTTConnect();
#endif
        }
                break;
#if MQTT5
        case REASON_SERVER_MOVED:
        case REASON_USE_ANOTHER_SERVER:
            _redirect(*(traits.properties));
            break;
        default:
            H4AMC_PRINT1("CONNACK %s\n",mqttTraits::rcnames[static_cast<H4AMC_MQTT_ReasonCode>(rcode)]);
            // [ ] Inform the user of the CONNACK packet Reason Code when a failure occurs. (Not needed)
            // if (_cbProtocolEvent) _cbProtocolEvent(CONNECT_FAIL, static_cast<H4AMC_MQTT_ReasonCode>(rcode), traits.props);
                
#else
        default: 
            H4AMC_PRINT1("CONNACK %s\n",mqttTraits::connacknames[rcode]);
#endif
            _notify(H4AMC_CONNECT_FAIL, rcode);
            break;
        }
    }
        break;
    case PUBACK:
    case PUBCOMP:
        if (_cbPublish) _cbPublish(id);
    case UNSUBACK:
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
        if (_packSubIds.count(id)) {
            _confirmDeletion(id);
        }
#endif
    case SUBACK:
    {
        bool badSub=false;
        for (auto &rc : traits.subreasons)
            if (rc>=0x80) badSub=true;

        if (badSub)
            _notify(traits.type==SUBACK?H4AMC_SUBSCRIBE_FAIL:H4AMC_UNSUBSCRIBE_FAIL, id);
    }
        _ACKoutbound(id); // MAX retries applies??
        break;
    case PUBREC:
        {
            _outbound[id].pubrec=true;
            PubrelPacket prp(this,id);
        }
        break;
    case PUBREL:
        {
            if(_inbound.count(id)) {
                _inbound.erase(id);
            } else _notify(H4AMC_INBOUND_QOS_ACK_FAIL,id);
            PubcompPacket pcp(this,id); // pubrel
        }
        break;
#if MQTT5
    case DISCONNECT: // Server disconnect.
    {
        switch(rcode) {
            case REASON_SERVER_MOVED:
            case REASON_USE_ANOTHER_SERVER:
                _redirect(props);
            break;
            default:
                H4AMC_PRINT1("DISCONNECT %s\n", mqttTraits::rcnames[static_cast<H4AMC_MQTT_ReasonCode>(rcode)]);
                _notify(H4AMC_SERVER_DISCONNECT, rcode);
        }
        _handleReasonString(props);
        _state = H4AMC_DISCONNECTED;
        if (_cbMQTTDisconnect)
            _cbMQTTDisconnect();
        _destroyClient();
    }
        break;
    case AUTH:
        //[ ]  Handle Auth request ...
        // ...

        break;
#endif // MQTT5
    default:
        if(traits.isPublish()) _handlePublish(traits);
        else {
            _notify(H4AMC_BOGUS_PACKET,data[0]);
            H4AMC_DUMP3(data,len);
        }
        break;
    }
#if MQTT5
        //_handleReasonString();
#endif
        if (traits.next.second)
        {
            H4AMC_PRINT4("Let's go round again! %p %d\n", traits.next.first, traits.next.second);
            if (n_handled < 10)
                _handlePacket(traits.next.first, traits.next.second, n_handled + 1);
            else
            { // Relay off to another call tree to avoid memory exhaustion / stack overflow / watchdog reset
                H4AMC_PRINT4("Too many packets to handle, saving stack to another call tree\n");
                h4.queueFunction([=]()
                                    { _handlePacket(traits.next.first, traits.next.second, 0); });
            }
        }
}

#if MQTT5
void H4AsyncMQTT::_protocolError(H4AMC_MQTT_ReasonCode reason)
{
    H4AMC_PRINT1("Protocol Error %u:%s\n", reason, mqttTraits::rcnames[reason]);
    disconnect(reason);
    _destroyClient();
}
void H4AsyncMQTT::_handleConnackProps(MQTT_Properties& props)
{
    if (_serverOptions) 
        delete _serverOptions;
    _serverOptions = new Server_Options;
    for (auto &p : props.available_properties) {
        switch (p) {
        // case PROPERTY_SESSION_EXPIRY_INTERVAL:
        //     _serverOptions->session_expiry_interval = props.getNumericProperty(p);
        //     break;
        case PROPERTY_RECEIVE_MAXIMUM:
            _serverOptions->receive_max = props.getNumericProperty(p);
            break;
        case PROPERTY_MAXIMUM_QOS:
            _serverOptions->maximum_qos = props.getNumericProperty(p);
            break;
        case PROPERTY_RETAIN_AVAILABLE:
            _serverOptions->retain_available = props.getNumericProperty(p);
            break;
        case PROPERTY_MAXIMUM_PACKET_SIZE:
            _serverOptions->maximum_packet_size = props.getNumericProperty(p);
            break;
        case PROPERTY_ASSIGNED_CLIENT_IDENTIFIER: // [ ] Any action?
            H4AMC_PRINT1("Assigned ClientID=%s\n", props.getStringProperty(p).c_str());
            break;
        case PROPERTY_TOPIC_ALIAS_MAXIMUM:
            _serverOptions->topic_alias_max = props.getNumericProperty(p);
            break;
        case PROPERTY_REASON_STRING:
            H4AMC_PRINT1("Reason String=%s\n", props.getStringProperty(p).c_str());
            break;
        case PROPERTY_WILDCARD_SUBSCRIPTION_AVAILABLE:
            _serverOptions->wildcard_subscription_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SUBSCRIPTION_IDENTIFIER_AVAILABLE:
            _serverOptions->subscriptions_identifiers_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SHARED_SUBSCRIPTION_AVAILABLE:
            _serverOptions->shared_subscription_available = props.getNumericProperty(p);
            break;
        case PROPERTY_SERVER_KEEP_ALIVE:
        {
            auto v = props.getNumericProperty(p);
            if (v < _keepalive) {
                // [x] Change the PINGREQ timer to the new value.
                _startPinging(v);
            }
            break;
        }
        break;
        case PROPERTY_RESPONSE_INFORMATION:
            _serverOptions->response_information = props.getStringProperty(p);
            break;
            // [ ] Handle Authentication Method and Data...
/*                     case PROPERTY_AUTHENTICATION_METHOD:
            _serverOptions->authentication_method = props.getStringProperty(p);
            break;
        case PROPERTY_AUTHENTICATION_DATA:
            _serverOptions->authentication_data = props.getStringProperty(p);
            break; */
        default:
            break;
        }
    }
// #if H4AMC_DEBUG
//     props.dump();
// #endif
}
#endif

void H4AsyncMQTT::_handlePublish(mqttTraits P){
    uint8_t qos=P.qos;
    uint16_t id=P.id;
    H4AMC_PRINT4("_handlePublish %s id=%d @ QoS%d R=%s DUP=%d PL@%08X PLEN=%d\n",P.topic.data(),id,qos,P.retain ? "true":"false",P.dup,P.payload,P.plen);
#if MQTT5
    if (P.properties && P.properties->isAvailable(PROPERTY_TOPIC_ALIAS)) {
        // Register/Update topic alias
        uint16_t alias = P.properties->getNumericProperty(PROPERTY_TOPIC_ALIAS);
        H4AMC_PRINT2("RECEIVED TOPIC ALIAS %d EXIST %d\n", alias, _rx_topic_alias.count(alias));

        if (_rx_topic_alias.count(alias) && !P.topic.length()) 
        {
            H4AMC_PRINT2("FETCHING TOPIC %s\n", _rx_topic_alias[alias].c_str());
            P.topic = _rx_topic_alias[alias];
        } 
        else if (alias && alias < MQTT5_RX_TOPIC_ALIAS_MAXIMUM && P.topic.length()) // There's Topic and a valid alias
        { 
            H4AMC_PRINT2("ASSIGNING TOPIC ALIAS %d to %s\n", alias, P.topic.c_str());
            _rx_topic_alias[alias] = P.topic;
        } 
        else if (P.topic.length()) // It's an invalid alias (alias==0 OR Topic Alias Exceeded)
        {
            H4AMC_PRINT2("VALID TOPIC - INVALID ALIAS\n");
            _protocolError(REASON_TOPIC_ALIAS_INVALID);
            return;
        } // else NO Topic Alias Mapping found (P.topic.length()==0)
    }
    if (!P.topic.length()) {
        H4AMC_PRINT1("NO Topic Alias Mapping Exist!\n");
        _protocolError(REASON_PROTOCOL_ERROR);
        return;
    }
#endif
    if (qos<2 || !_inbound.count(id)); // For QoS2, Only dispatch to the user once; _inbound holds the id of publishes under PUBREL from us to the server.
        _hpDespatch(P);

    switch(qos){
        // case 0:
        //     _hpDespatch(P);
        //     break;
        case 1:
            { 
                // _hpDespatch(P);
                PubackPacket pap(this,id);
            }
            break;
        case 2:
            {
                _inbound.insert(id);
                // PublishPacket pub(this,P.topic.data(),qos,P.retain,P.payload,P.plen,0,id); // build and HOLD until PUBREL force dup=0
                PubrecPacket pcpthis(this,id);
            }
            break;
    }
}

void H4AsyncMQTT::_notify(int e,int info){ 
    H4AMC_PRINT1("NOTIFY e=%d inf=%d\n",e,info);
    if(_cbMQTTError) _cbMQTTError(e,info);
}

void H4AsyncMQTT::_resendPartialTxns(bool availSession){ // [ ] Rename to handleSession
    // Check whether the messages are outdated...
    // OR regularly resend them...?
    std::vector<uint16_t> morituri;
    for(auto const& o:_outbound){
        mqttTraits m=o.second;
        if(--(m.retries)){
            if(m.pubrec){
                if (!availSession) morituri.push_back(m.id);
                else {
                    H4AMC_PRINT4("WE ARE PUBREC'D ATTEMPT @ QOS2: SEND %d PUBREL\n",m.id);
                    PubrelPacket prp(this,m.id);
                }
            }
            else {
                H4AMC_PRINT4("SET DUP & RESEND %d\n",m.id);
                m.data[0]|=0x08; // set dup & resend
                _h4atClient->TX(m.data,m.len,false);
            }
        }
        else {
            H4AMC_PRINT4("NO JOY AFTER %d ATTEMPTS: QOS FAIL\n",H4AMC_MAX_RETRIES);
            morituri.push_back(m.id); // all hope exhausted TODO: reconnect?
        }
    }
    for(auto const& i:morituri) _ACKoutbound(i);

#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
    if (!availSession) {
        // [x] Clean subscription id maps.
        _subsResources.clear();
        _packSubIds.clear();
    }
#endif
}

void H4AsyncMQTT::_runGuard(H4AMC_FN_VOID f){
    if(_state==H4AMC_RUNNING) f();
    else _notify(0,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::_startReconnector(){ h4.every(5000,[=]{ _connect(); },nullptr,H4AMC_RCX_ID,true); }
//
//      PUBLIC
//
void H4AsyncMQTT::connect(const char* url,const char* auth,const char* pass,const char* clientId,bool clean){
    H4AMC_PRINT1("H4AsyncMQTT::connect(%s,%s,%s,%s,%d)\n",url,auth,pass,clientId,clean);
    if(_state==H4AMC_DISCONNECTED){
        _h4atClient=new H4AsyncClient;
        _url=url;
        _username = auth;
        _password = pass;
#if MQTT5
        _cleanStart = clean;
#else
        _cleanSession = clean;
#endif
        _clientId = "" ? clientId:_HAL_uniqueName("H4AMC" H4AMC_VERSION);
        _connect();
        H4AsyncClient::_scavenge();
    } else if(_cbMQTTError) _cbMQTTError(ERR_ISCONN,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::disconnect(H4AMC_MQTT_ReasonCode reason) {
    static uint8_t G[] = {DISCONNECT, reason}; 
    // [ ] Properties...
        // Session expiry interval
        // Reason String
        // User Property
    H4AMC_PRINT1("USER DCX\n");
    if(_state==H4AMC_RUNNING) _h4atClient->TX(G,2,false);
    else _h4atClient->_cbError(ERR_CONN,H4AMC_USER_LOGIC_ERROR);
}

std::string H4AsyncMQTT::errorstring(int e){
    #ifdef H4AMC_DEBUG
        if(_errorNames.count(e)) return _errorNames[e];
        else return stringFromInt(e); 
    #else
        return stringFromInt(e); 
    #endif
}

uint16_t H4AsyncMQTT::publish(const char* topic, const uint8_t* payload, size_t length, uint8_t qos, H4AMC_PublishOptions opts_retain) { return _runGuard([=]{ PublishPacket pub(this,topic,qos,payload,length,opts_retain); return pub.getId(); }, (uint16_t)0); }

uint16_t H4AsyncMQTT::publish(const char* topic, const char* payload, size_t length, uint8_t qos, H4AMC_PublishOptions opts_retain) { 
    return publish(topic, reinterpret_cast<const uint8_t*>(payload), length, qos, opts_retain);
}

void H4AsyncMQTT::setWill(const char* topic, uint8_t qos, bool retain, const char* payload) {
    H4AMC_PRINT2("setWill *%s* q=%d r=%d *%s*\n",topic,qos,retain,payload);
    _willTopic = topic;
    _willQos = qos;
    _willRetain = retain;
    _willPayload = payload;
}

uint32_t H4AsyncMQTT::subscribe(const char* topic, H4AMC_SubscriptionOptions opts_qos) { return _runGuard([=](void){ SubscribePacket sub(this,topic,opts_qos); return sub.getId(); }, (uint32_t)0); }

uint32_t H4AsyncMQTT::subscribe(std::initializer_list<const char*> topix, H4AMC_SubscriptionOptions opts_qos) { _runGuard([=]{ SubscribePacket sub(this,topix,opts_qos); return sub.getId(); }, (uint32_t)0); }

void H4AsyncMQTT::unsubscribe(const char* topic) {_runGuard([=]{ UnsubscribePacket usp(this,topic); }); }

void H4AsyncMQTT::unsubscribe(std::initializer_list<const char*> topix) {_runGuard([=]{  UnsubscribePacket usp(this,topix); }); }
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
void H4AsyncMQTT::unsubscribe(uint32_t subscription_id) {
    if (_subsResources.count(subscription_id)) { 
        _runGuard([=]{ UnsubscribePacket usp(this, _subsResources[subscription_id].topix); });
    } else {
        _notify(0,H4AMC_USER_LOGIC_ERROR);
    }
}
#endif
//
//
//
void H4AsyncMQTT::dump(){
#if H4AMC_DEBUG

    H4AMC_PRINT4("DUMP ALL %d PACKETS OUTBOUND\n",_outbound.size());
    for(auto & p:_outbound) p.second.dump();

    H4AMC_PRINT4("DUMP ALL %d PACKETS INBOUND\n",_inbound.size());
    for(auto & p:_inbound) H4AMC_PRINT4("%d\t",p);

    H4AMC_PRINT4("\n");
#endif
}