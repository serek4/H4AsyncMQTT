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
H4AMC_PACKET_MAP        H4AsyncMQTT::_inbound;
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

H4AsyncMQTT::H4AsyncMQTT(){

}

void H4AsyncMQTT::_ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout){ /// refakta?
    if(m->count(id)){
        uint8_t* data=((*m)[id]).data;
        mbx::clear(data);
        m->erase(id);
    } else _notify(inout ? H4AMC_INBOUND_QOS_ACK_FAIL:H4AMC_OUTBOUND_QOS_ACK_FAIL,id); //H4AMC_PRINT("WHO TF IS %d???\n",id);
}

void H4AsyncMQTT::_cleanStart(){
    H4AMC_PRINT4("_cleanStart clrQQ inbound\n");
    _clearQQ(&_inbound);
    H4AMC_PRINT4("_cleanStart clrQQ outbound\n");
    _clearQQ(&_outbound);
}

void H4AsyncMQTT::_clearQQ(H4AMC_PACKET_MAP* m){
    for(auto &i:*m) mbx::clear(i.second.data);
    m->clear();
}

void H4AsyncMQTT::_connect(){
    static uint8_t PING[]={PINGREQ,0};    
    
    H4AMC_PRINT1("_connect %s _state=%d\n",_url.data(),_state);
    _h4atClient->onConnect([=](){
        H4AMC_PRINT1("on TCP Connect\n");
        _h4atClient->nagle(true);
        h4.cancelSingleton(H4AMC_RCX_ID);
        H4AMC_PRINT1("KA = %d\n",_keepalive - H4AMC_HEADROOM);
        h4.every(_keepalive - H4AMC_HEADROOM,[=]{
            if(_state==H4AMC_RUNNING){//} && ((millis() - _h4atClient->_lastSeen) > _keepalive)){ // 100 = headroom
                H4AMC_PRINT1("MQTT PINGREQ\n");
                _h4atClient->TX(PING,2,false); /// optimise
            } //else Serial.printf("No ping: activity %d mS ago\n",(millis() - _h4atClient->_lastSeen));
        },nullptr,H4AMC_KA_ID,true);
        h4.queueFunction([=]{ ConnectPacket cp{this}; }); // offload required for esp32 to get off tcpip thread
    });

    _h4atClient->onDisconnect([=]{
        H4AMC_PRINT1("onDisconnect - start reconnector\n");
        if(_state!=H4AMC_DISCONNECTED) if(_cbMQTTDisconnect) _cbMQTTDisconnect();
        _state=H4AMC_DISCONNECTED;
//        Serial.printf("CREATE NEW CLIENT!\n");
        _h4atClient=new H4AsyncClient;
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
                Serial.printf("session : %s\n",_cleanSession ? "clean":"dirty");
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

void H4AsyncMQTT::_hpDespatch(uint16_t id){ _hpDespatch(_inbound[id]); }

void H4AsyncMQTT::_handlePacket(uint8_t* data, size_t len, int n_handled){
    H4AMC_DUMP4(data,len);
    if(data[0]==PINGRESP || data[0]==UNSUBACK){ // [ ] _ACKoutbound of UNSUBACK packet?
        H4AMC_PRINT1("MQTT %s\n",mqttTraits::pktnames[data[0]]);
        return; // early bath
    }
    mqttTraits traits(data,len);
    auto i=traits.start();
    uint16_t id=traits.id;

    switch (traits.type){
        case CONNACK:
            if(i[1]) _notify(H4AMC_CONNECT_FAIL,i[1]);
            else {
                _state=H4AMC_RUNNING;
                bool session=i[0] & 0x01;
                _ACKoutbound(0); // ACK connect to clear it from POOL.
                _resendPartialTxns();
                H4AMC_PRINT1("CONNECTED FH=%u MaxPL=%u SESSION %s\n",_HAL_maxHeapBlock(),getMaxPayloadSize(),session ? "DIRTY":"CLEAN");
#if H4AMC_DEBUG
                SubscribePacket pango(this,"pango",0); // internal info during beta...will be moved back inside debug #ifdef
#endif
                if(_cbMQTTConnect) _cbMQTTConnect();
            }
            break;
        case SUBACK:
            if(i[2] & 0x80) _notify(H4AMC_SUBSCRIBE_FAIL,id);
            else _ACKoutbound(id);  // MAX retries applies??
            break;
        case PUBACK:
        case PUBCOMP:
            _ACKoutbound(id);
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
                    _hpDespatch(_inbound[id]);
                    _ACK(&_inbound,id,true); // true = inbound
                } else _notify(H4AMC_INBOUND_QOS_ACK_FAIL,id);
                PubcompPacket pcp(this,id); // pubrel
            }
            break;
       default:
            if(traits.isPublish()) _handlePublish(traits);
            else {
                _notify(H4AMC_BOGUS_PACKET,data[0]);
                H4AMC_DUMP3(data,len);
            }
            break;
    }
    if(traits.next.second){
        H4AMC_PRINT4("Let's go round again! %p %d\n",traits.next.first,traits.next.second);
        if (n_handled < 10)
            _handlePacket(traits.next.first,traits.next.second, n_handled + 1);
        else {  // Relay off to another call tree to avoid memory exhaustion / stack overflow / watchdog reset
            H4AMC_PRINT4("Too many packets to handle, saving stack to another call tree\n");
            h4.queueFunction([=]() { _handlePacket(traits.next.first,traits.next.second, 0); });
        }
    }
}

void H4AsyncMQTT::_handlePublish(mqttTraits P){
    uint8_t qos=P.qos;
    uint16_t id=P.id;
    H4AMC_PRINT4("_handlePublish %s id=%d @ QoS%d R=%s DUP=%d PL@%08X PLEN=%d\n",P.topic.data(),id,qos,P.retain ? "true":"false",P.dup,P.payload,P.plen);
    switch(qos){
        case 0:
            _hpDespatch(P);
            break;
        case 1:
            { 
                _hpDespatch(P);
                PubackPacket pap(this,id);
            }
            break;
        case 2:
        //  MQTT Spec. "method A"
            {
                PublishPacket pub(this,P.topic.data(),qos,P.retain,P.payload,P.plen,0,id); // build and HOLD until PUBREL force dup=0
                PubrecPacket pcpthis(this,id);
            }
            break;
    }
}

void H4AsyncMQTT::_notify(int e,int info){ 
    H4AMC_PRINT1("NOTIFY e=%d inf=%d\n",e,info);
    if(_cbMQTTError) _cbMQTTError(e,info);
}

void H4AsyncMQTT::_resendPartialTxns(){
    // Check whether the messages are outdated...
    // Or regularly resend them...?
    std::vector<uint16_t> morituri;
    for(auto const& o:_outbound){
        mqttTraits m=o.second;
        if(--(m.retries)){
            if(m.pubrec){
                H4AMC_PRINT4("WE ARE PUBREC'D ATTEMPT @ QOS2: SEND %d PUBREL\n",m.id);
                PubrelPacket prp(this,m.id);
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
}

void H4AsyncMQTT::_runGuard(H4_FN_VOID f){
    if(_state==H4AMC_RUNNING) f();
    else _notify(0,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::_startReconnector(){ h4.every(5000,[=]{ _connect(); },nullptr,H4AMC_RCX_ID,true); }
//
//      PUBLIC
//
void H4AsyncMQTT::connect(const char* url,const char* auth,const char* pass,const char* clientId,bool session){
    H4AMC_PRINT1("H4AsyncMQTT::connect(%s,%s,%s,%s,%d)\n",url,auth,pass,clientId,session);
    if(_state==H4AMC_DISCONNECTED){
        _h4atClient=new H4AsyncClient;
        _url=url;
        _username = auth;
        _password = pass;
        _cleanSession = session;
        _clientId = "" ? clientId:_HAL_uniqueName("H4AMC" H4AMC_VERSION);
        _connect();
        H4AsyncClient::_scavenge();
    } else if(_cbMQTTError) _cbMQTTError(ERR_ISCONN,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::disconnect() {
    static uint8_t  G[]={DISCONNECT,0};
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

void H4AsyncMQTT::publish(const char* topic, const uint8_t* payload, size_t length, uint8_t qos, bool retain) { _runGuard([=]{ PublishPacket pub(this,topic,qos,retain,payload,length,0,0); }); }

void H4AsyncMQTT::publish(const char* topic, const char* payload, size_t length, uint8_t qos, bool retain) { 
    _runGuard([=]{ publish(topic, reinterpret_cast<const uint8_t*>(payload), length, qos, retain); });
}

void H4AsyncMQTT::setWill(const char* topic, uint8_t qos, bool retain, const char* payload) {
    H4AMC_PRINT2("setWill *%s* q=%d r=%d *%s*\n",topic,qos,retain,payload);
    _willTopic = topic;
    _willQos = qos;
    _willRetain = retain;
    _willPayload = payload;
}

void H4AsyncMQTT::subscribe(const char* topic, uint8_t qos) { _runGuard([=]{ SubscribePacket sub(this,topic,qos); }); }

void H4AsyncMQTT::subscribe(std::initializer_list<const char*> topix, uint8_t qos) { _runGuard([=]{ SubscribePacket sub(this,topix,qos); }); }

void H4AsyncMQTT::unsubscribe(const char* topic) {_runGuard([=]{ UnsubscribePacket usp(this,topic); }); }

void H4AsyncMQTT::unsubscribe(std::initializer_list<const char*> topix) {_runGuard([=]{  UnsubscribePacket usp(this,topix); }); }
//
//
//
void H4AsyncMQTT::dump(){
#if H4AMC_DEBUG

    H4AMC_PRINT4("DUMP ALL %d PACKETS OUTBOUND\n",_outbound.size());
    for(auto & p:_outbound) p.second.dump();

    H4AMC_PRINT4("DUMP ALL %d PACKETS INBOUND\n",_inbound.size());
    for(auto & p:_inbound) p.second.dump();

    H4AMC_PRINT4("\n");
#endif
}