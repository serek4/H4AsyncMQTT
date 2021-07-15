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
#include <PangolinMQTT.h>
#include <h4async_config.h>
#include "Packet.h"
#include "mqTraits.h"

H4AMC_PACKET_MAP        PangolinMQTT::_inbound;
H4AMC_PACKET_MAP        PangolinMQTT::_outbound;

PangolinMQTT*           H4AMCV3;

H4_INT_MAP PangolinMQTT::_errorNames={
#if H4AMC_DEBUG
    {H4AMC_DISCONNECTED,"NOT CONNECTED"},
    {H4AMC_SERVER_UNAVAILABLE," MQTT SERVER UNAVAILABLE"},
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
#endif
};

PangolinMQTT::PangolinMQTT(): H4AsyncTCP(){
    H4AMCV3=this;
    onConnect([=](){
        setNoDelay(true);
        if(!_connected) ConnectPacket cp{};
    });
    onDisconnect([=](int r){ H4AMC_PRINT1("TCP CHOPPED US! %d\n",r); _onDisconnect(H4AMC_DISCONNECTED); });
    onError([=](int error,int info){
        H4AMC_PRINT1("onError %d info=%d\n",error,info); 
        _notify(error,info);
        if(error < H4AMC_ERROR_BASE) disconnect();
    });
    onPoll([=]{ _onPoll(); });

    rx([=](const uint8_t* data,size_t len){ _handlePacket((uint8_t*) data,len); });
}

void PangolinMQTT::setServer(const char* url,const char* username, const char* password,const uint8_t* fingerprint){
    _username = username;
    _password = password;
    TCPurl(url,fingerprint);
}

void PangolinMQTT::setWill(const std::string& topic, uint8_t qos, bool retain, const std::string& payload) {
    _willTopic = topic;
    _willQos = qos;
    _willRetain = retain;
    _willPayload = payload;
}

void PangolinMQTT::_ACK(H4AMC_PACKET_MAP* m,uint16_t id,bool inout){ /// refakta?
    if(m->count(id)){
        uint8_t* data=((*m)[id]).data;
        mbx::clear(data);
        m->erase(id);
    } else _notify(inout ? H4AMC_INBOUND_QOS_ACK_FAIL:H4AMC_OUTBOUND_QOS_ACK_FAIL,id); //H4AMC_PRINT("WHO TF IS %d???\n",id);
}

void PangolinMQTT::_cleanStart(){
    H4AMC_PRINT4("_cleanStart clrQQ inbound\n");
    _clearQQ(&_inbound);
    H4AMC_PRINT4("_cleanStart clrQQ outbound\n");
    _clearQQ(&_outbound);
    Packet::_nextId=1000; // SO much easier to differentiate client vs server IDs in Wireshark log :)
}

void PangolinMQTT::_clearQQ(H4AMC_PACKET_MAP* m){
    for(auto &i:*m) mbx::clear(i.second.data);
    m->clear();
}
void PangolinMQTT::_cnxGuard(H4_FN_VOID f){
    if(_connected) f();
    else _notify(H4AMC_DISCONNECTED);
}

void PangolinMQTT::_hpDespatch(mqttTraits P){ 
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
                Serial.printf("POLLRATE: %d\n",H4AMC_POLL_RATE);
                Serial.printf("NRETRIES: %d\n",H4AMC_MAX_RETRIES);

                Serial.printf("session : %s\n",_cleanSession ? "clean":"dirty");
                Serial.printf("clientID: %s\n",_clientId.data());
                Serial.printf("keepaliv: %d => %ds\n",_keepalive,_keepalive/H4AMC_POLL_RATE);
                Serial.printf("poll Tix: %d\n",_nPollTicks);
                Serial.printf("srv  Tix: %d\n",_nSrvTicks);
            }
            else if(pl=="dump"){ dump(); }
        }

        else
#endif
        _cbMessage(P.topic.data(), P.payload, P.plen, P.qos, P.retain, P.dup);
    }
}

void PangolinMQTT::_hpDespatch(uint16_t id){ _hpDespatch(_inbound[id]); }

void PangolinMQTT::_handlePacket(uint8_t* data, size_t len){
    _nSrvTicks=0;
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
                _connected=true;
                bool session=i[0] & 0x01;
                _resendPartialTxns();
                _nPollTicks=_nSrvTicks=0;
                H4AMC_PRINT1("CONNECTED FH=%u MaxPL=%u SESSION %s\n",_HAL_maxHeapBlock(),getMaxPayloadSize(),session ? "DIRTY":"CLEAN");
#if H4AMC_DEBUG
                SubscribePacket pango("pango",0); // internal info during beta...will be moved back inside debug #ifdef
#endif
                if(_cbConnect) _cbConnect(session);
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

void PangolinMQTT::_handlePublish(mqttTraits P){
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

void PangolinMQTT::_notify(uint8_t e,int info){ 
    H4AMC_PRINT1("NOTIFY e=%d inf=%d\n",e,info);
    if(_cbError) _cbError(e,info);
}

void PangolinMQTT::_onDisconnect(int8_t r) {
    _connected=false;
    if(_cbDisconnect) _cbDisconnect(r);
}

void PangolinMQTT::_onPoll() {
    static uint8_t  G[]={PINGREQ,0};
    if(_connected){
        ++_nPollTicks;
        ++_nSrvTicks;

        if(_nSrvTicks > ((_keepalive * 3) / 2)) _onDisconnect(H4AMC_SERVER_UNAVAILABLE);
        else {
            if(_nPollTicks > _keepalive){
                H4AMC_PRINT1("T=%d PINGREQ\n",millis());
                txdata(G,2,false); // static ping
                _nPollTicks=0;
            }
        }
    }
}

void PangolinMQTT::_resendPartialTxns(){
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
//
//      PUBLIC
//
void PangolinMQTT::connectMqtt(std::string client,bool session){ 
    _cleanSession = session;
    _clientId = client.size() ? client:_HAL_uniqueName("H4AMC"H4AMC_VERSION);
    connect();
}

void PangolinMQTT::disconnect() {
    static uint8_t  G[]={DISCONNECT,0};
    H4AMC_PRINT1("USER DCX\n");
    _cnxGuard([=]{ txdata(G,2,false); });
}

std::string PangolinMQTT::errorstring(int e){
    #ifdef H4AMC_DEBUG
        if(_errorNames.count(e)) return _errorNames[e];
        else return stringFromInt(e); 
    #else
        return stringFromInt(e); 
    #endif
}

void PangolinMQTT::publish(const char* topic, const uint8_t* payload, size_t length, uint8_t qos, bool retain) { _cnxGuard([=]{ PublishPacket pub(topic,qos,retain,payload,length,0,0); }); }

void PangolinMQTT::publish(const char* topic, const char* payload, size_t length, uint8_t qos, bool retain) { 
    _cnxGuard([=]{ publish(topic, reinterpret_cast<const uint8_t*>(payload), length, qos, retain); });
}

void PangolinMQTT::subscribe(const char* topic, uint8_t qos) { _cnxGuard([=]{ SubscribePacket sub(topic,qos); }); }

void PangolinMQTT::subscribe(std::initializer_list<const char*> topix, uint8_t qos) { _cnxGuard([=]{ SubscribePacket sub(topix,qos); }); }

void PangolinMQTT::unsubscribe(const char* topic) {_cnxGuard([=]{ UnsubscribePacket usp(topic); }); }

void PangolinMQTT::unsubscribe(std::initializer_list<const char*> topix) {_cnxGuard([=]{  UnsubscribePacket usp(topix); }); }
//
//
//
#if H4AMC_DEBUG
void PangolinMQTT::dump(){
#if H4AT_DEBUG
    H4AsyncTCP::dump();
#endif
    H4AMC_PRINT4("DUMP ALL %d PACKETS OUTBOUND\n",_outbound.size());
    for(auto & p:_outbound) p.second.dump();

    H4AMC_PRINT4("DUMP ALL %d PACKETS INBOUND\n",_inbound.size());
    for(auto & p:_inbound) p.second.dump();

    H4AMC_PRINT4("\n");
}
#endif