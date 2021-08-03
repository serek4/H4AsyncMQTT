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
#include <H4AsyncMQTT.h>
#include <h4async_config.h>
#include "Packet.h"
#include "mqTraits.h"

H4AMC_PACKET_MAP        H4AsyncMQTT::_inbound;
H4AMC_PACKET_MAP        H4AsyncMQTT::_outbound;

H4AsyncMQTT*           H4AMCV3;

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
    {H4AMC_KEEPALIVE_TOO_LONG,"KEEPALIVE TOO LONG: MUST BE < H4AS_SCAVENGE_FREQ"},
    {H4AMC_USER_LOGIC_ERROR,"USER LOGIC ERROR"},
#endif
};

H4AsyncMQTT::H4AsyncMQTT(): H4AsyncClient(){
    static uint8_t PING[]={PINGREQ,0};
    H4AMCV3=this;
    onConnect([=](){
        H4AMC_PRINT1("on TCP Connect\n");
        //setNoDelay(true);
        h4.cancelSingleton(H4AMC_RCX_ID);
        h4.every(_keepalive,[=]{
            if(!connected()){ 
                Serial.printf("Server gone away - keep client alive!\n");
                _lastSeen=millis();
            } else txdata(PING,2,false); 
        },nullptr,H4AMC_KA_ID,true); // keepalive even if remote server gone away
        ConnectPacket cp{};
    });

    onDisconnect([=]{ 
        _state=H4AMC_DISCONNECTED;
        if(_cbMQTTDisconnect) _cbMQTTDisconnect();
        Serial.printf("Disconnected - start reconnector\n");
        h4.every(10000,[=]{ Serial.printf("Attempt reconnection\n"); connect(); },[]{ Serial.printf("RCX stopped\n"); },H4AMC_RCX_ID,true);
    });

    onError([=](int error,int info){
        H4AMC_PRINT1("onError %d info=%d\n",error,info);
        _notify(error,info);
//        if(error == ERR_RST) _cbDisconnect();
        if(error > ERR_ABRT) {
            Serial.printf("UNRECOVERABLE cancel reconnector! %d\n",error);
            h4.cancelSingleton(H4AMC_RCX_ID);
            _state=H4AMC_FATAL;
        } 
        else {
            Serial.printf("Allow error! %d\n",error);
            // _cbDisconnect();
            //h4.queueFunction([=]{ Serial.printf("QF _raw_close %d\n",error); _raw_close(this); });
            _raw_close(this);
        }
    });

    _rxfn=[=](const uint8_t* data,size_t len){ _handlePacket((uint8_t*) data,len); };
}

void H4AsyncMQTT::setServer(const char* url,const char* username, const char* password,const uint8_t* fingerprint){
    _username = username;
    _password = password;
    TCPurl(url,fingerprint);
}

void H4AsyncMQTT::setWill(const std::string& topic, uint8_t qos, bool retain, const std::string& payload) {
    _willTopic = topic;
    _willQos = qos;
    _willRetain = retain;
    _willPayload = payload;
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
    Packet::_nextId=1000; // SO much easier to differentiate client vs server IDs in Wireshark log :)
}

void H4AsyncMQTT::_clearQQ(H4AMC_PACKET_MAP* m){
    for(auto &i:*m) mbx::clear(i.second.data);
    m->clear();
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
                Serial.printf("H4AT  Vn: %s\n",H4ASYNC_VERSION);
                Serial.printf("CHECK FP: %d\n",H4AT_CHECK_FINGERPRINT);
//                Serial.printf("SAFEHEAP: %d\n",H4AT_HEAP_SAFETY);
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

void H4AsyncMQTT::_handlePacket(uint8_t* data, size_t len){
    if(data[0]==PINGRESP || data[0]==UNSUBACK){
        H4AMC_PRINT1("T=%d %s\n",millis(),mqttTraits::pktnames[data[0]]);
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
                _resendPartialTxns();
                H4AMC_PRINT1("CONNECTED FH=%u MaxPL=%u SESSION %s\n",_HAL_maxHeapBlock(),getMaxPayloadSize(),session ? "DIRTY":"CLEAN");
#if H4AMC_DEBUG
                SubscribePacket pango("pango",0); // internal info during beta...will be moved back inside debug #ifdef
#endif
                if(_cbMQTTConnect) _cbMQTTConnect(session);
            }
            break;
        case SUBACK:
            if(i[2] & 0x80) _notify(H4AMC_SUBSCRIBE_FAIL,id);
            break;
        case PUBACK:
        case PUBCOMP:
            _ACKoutbound(id);
            break;
        case PUBREC:
            {
                _outbound[id].pubrec=true;
                PubrelPacket prp(id);
            }
            break;
        case PUBREL:
            {
                if(_inbound.count(id)) {
                    _hpDespatch(_inbound[id]);
                    _ACK(&_inbound,id,true); // true = inbound
                } else _notify(H4AMC_INBOUND_QOS_ACK_FAIL,id);
                PubcompPacket pcp(id); // pubrel
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
                PubackPacket pap(id);
            }
            break;
        case 2:
        //  MQTT Spec. "method A"
            {
                PublishPacket pub(P.topic.data(),qos,P.retain,P.payload,P.plen,0,id); // build and HOLD until PUBREL force dup=0
                PubrecPacket pcp(id);
            }
            break;
    }
}

void H4AsyncMQTT::_notify(int e,int info){ 
    H4AMC_PRINT1("NOTIFY e=%d inf=%d\n",e,info);
    if(_cbMQTTError) _cbMQTTError(e,info);
}

void H4AsyncMQTT::_resendPartialTxns(){
    std::vector<uint16_t> morituri;
    for(auto const& o:_outbound){
        mqttTraits m=o.second;
        if(--(m.retries)){
            if(m.pubrec){
                H4AMC_PRINT4("WE ARE PUBREC'D ATTEMPT @ QOS2: SEND %d PUBREL\n",m.id);
                PubrelPacket prp(m.id);
            }
            else {
                H4AMC_PRINT4("SET DUP & RESEND %d\n",m.id);
                m.data[0]|=0x08; // set dup & resend
                txdata(m.data,m.len,false);
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
    else Serial.print("not running - ignored\n");
}
//
//      PUBLIC
//
void H4AsyncMQTT::connectMqtt(std::string client,bool session){
    if(!connected()){
        _cleanSession = session;
        _clientId = client.size() ? client:_HAL_uniqueName("H4AMC"H4AMC_VERSION);
        connect();
        Serial.printf("INITAL CONNECT...start timer anyway\n");
        h4.every(10000,[=]{ Serial.printf("Attempt reconnection\n"); connect(); },[]{ Serial.printf("RCX stopped\n"); },H4AMC_RCX_ID,true);
    } else XDISPATCH_V(Error,ERR_ISCONN,H4AMC_USER_LOGIC_ERROR);
}

void H4AsyncMQTT::disconnect() {
    static uint8_t  G[]={DISCONNECT,0};
    H4AMC_PRINT1("USER DCX\n");
    if(_state==H4AMC_RUNNING) txdata(G,2,false);
    else XDISPATCH_V(Error,ERR_CONN,H4AMC_USER_LOGIC_ERROR);
}

std::string H4AsyncMQTT::errorstring(int e){
    #ifdef H4AMC_DEBUG
        if(_errorNames.count(e)) return _errorNames[e];
        else return stringFromInt(e); 
    #else
        return stringFromInt(e); 
    #endif
}

void H4AsyncMQTT::publish(const char* topic, const uint8_t* payload, size_t length, uint8_t qos, bool retain) { _runGuard([=]{ PublishPacket pub(topic,qos,retain,payload,length,0,0); }); }

void H4AsyncMQTT::publish(const char* topic, const char* payload, size_t length, uint8_t qos, bool retain) { 
    _runGuard([=]{ publish(topic, reinterpret_cast<const uint8_t*>(payload), length, qos, retain); });
}

void H4AsyncMQTT::setKeepAlive(uint16_t keepAlive){
    if(keepAlive < (H4AS_SCAVENGE_FREQ - 1000)) _keepalive=keepAlive;
    else XDISPATCH_V(Error,H4AMC_KEEPALIVE_TOO_LONG,(H4AS_SCAVENGE_FREQ - 1000));
}

void H4AsyncMQTT::subscribe(const char* topic, uint8_t qos) { _runGuard([=]{ SubscribePacket sub(topic,qos); }); }

void H4AsyncMQTT::subscribe(std::initializer_list<const char*> topix, uint8_t qos) { _runGuard([=]{ SubscribePacket sub(topix,qos); }); }

void H4AsyncMQTT::unsubscribe(const char* topic) {_runGuard([=]{ UnsubscribePacket usp(topic); }); }

void H4AsyncMQTT::unsubscribe(std::initializer_list<const char*> topix) {_runGuard([=]{  UnsubscribePacket usp(topix); }); }
//
//
//
#if H4AMC_DEBUG
void H4AsyncMQTT::dump(){
#if H4AT_DEBUG
    H4AsyncClient::dump();
#endif
    H4AMC_PRINT4("DUMP ALL %d PACKETS OUTBOUND\n",_outbound.size());
    for(auto & p:_outbound) p.second.dump();

    H4AMC_PRINT4("DUMP ALL %d PACKETS INBOUND\n",_inbound.size());
    for(auto & p:_inbound) p.second.dump();

    H4AMC_PRINT4("\n");
}
#endif