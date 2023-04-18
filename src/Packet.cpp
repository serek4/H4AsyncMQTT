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
#include<H4AsyncMQTT.h>
#include<Packet.h>

void Packet::_build(bool hold){
    uint8_t* virgin;
    _begin();
    if(_hasId) _bs+=2;
    // calc rl
    uint32_t X=_bs;
    std::vector<uint8_t> rl;
    uint8_t encodedByte;
    do{
        encodedByte = X % 128;
        X = X / 128;
        if ( X > 0 ) encodedByte = encodedByte | 128;
        rl.push_back(encodedByte);
    } while ( X > 0 );
    _bs+=1+rl.size();
    uint8_t* snd_buf=virgin=mbx::getMemory(_bs);
    H4AMC_PRINT4("PACKET CREATED @ %p len=%d\n",snd_buf,_bs);
    if(snd_buf){
        *snd_buf++=_controlcode;
        for(auto const& r:rl) *snd_buf++=r;
        snd_buf=_varHeader(snd_buf);
        _protocolPayload(snd_buf,virgin);
        if(!hold) {
#if H4AMC_DEBUG
            if((_controlcode & 0xf0) != PUBLISH) mqttTraits traits(virgin,_bs);
#endif
            _parent->_h4atClient->TX(virgin,_bs,false);
        }
    } else _parent->_notify(ERR_MEM,_bs);
}

void Packet::_idGarbage(uint16_t id){
    uint8_t hi=(id & 0xff00) >> 8;
    uint8_t lo=id & 0xff;
    uint8_t  G[]={_controlcode,2,hi,lo};
#if H4AMC_DEBUG
    mqttTraits(&G[0],4);
#endif
    _parent->_h4atClient->TX(&G[0],4,true);
}

void Packet::_multiTopic(std::initializer_list<const char*> topix,uint8_t qos){
    static std::vector<std::string> topics;
    _id=++_parent->_nextId;
    _begin=[=]{
        for(auto &t:topix){
            topics.push_back(t);
            _bs+=(_controlcode==SUBSCRIBE ? 3:2)+strlen(t); // 3 because of subscription options/QoS (3.1.1).
        }
        // [ ] Add the properties to the length (property length) + store them
    };
    _varHeader = [=](uint8_t *p)
    {
        p = _poke16(p, _id);
#if MQTT5
        // Embed the properties:
        // property length, then the properties
#endif
        return p;
    };

    _protocolPayload=[=](uint8_t* p,uint8_t* base){
        for(auto const& t:topics){
            size_t n=t.size();
            p=_poke16(p,n);
            memcpy(p,t.data(),n);
            p+=n;
            if(_controlcode==SUBSCRIBE) *p++=qos;// [ ] TODO: Subscription options
        }
        mqttTraits T(base,_bs);
        H4AsyncMQTT::_outbound[_id]=T;  // To be released on SUBACK
    };

    _build();
    topics.clear();
    topics.shrink_to_fit();
}

uint8_t* Packet::_poke16(uint8_t* p,uint16_t u){
    *p++=(u & 0xff00) >> 8;
    *p++=u & 0xff;
    return p;
}

void Packet::_stringblock(const std::string& s){ 
    size_t sz=s.size();
    _bs+=sz+2;
    _blox.push(mbx((uint8_t*) s.data(),sz,true));
}

ConnectPacket::ConnectPacket(H4AsyncMQTT* p): Packet(p,CONNECT){
    _bs=10;
    _begin=[=]{
#if MQTT5
        if(_parent->_cleanStart) protocol[7]|=CLEAN_START;
#else
        if(_parent->_cleanSession) protocol[7]|=CLEAN_SESSION;
#endif
        _stringblock(_parent->_clientId);
        // clientID --> Will properties --> wil lTopic --> willPayload --> username --> password
        if(_parent->_willTopic.size()){
            if(_parent->_willRetain) protocol[7]|=WILL_RETAIN;
            if(_parent->_willQos) protocol[7]|=(_parent->_willQos==1) ? WILL_QOS1:WILL_QOS2;
            // _stringblock(""_parent->_willProperties"");
            _stringblock(_parent->_willTopic);
            _stringblock(_parent->_willPayload);
            protocol[7]|=WILL;
        }
        if(_parent->_username.size()){
            _stringblock(_parent->_username);
            protocol[7]|=USERNAME;
        }
        if(_parent->_password.size()){
            _stringblock(_parent->_password);
            protocol[7]|=PASSWORD;
        }
        // [ ] Add the properties to the length (property length) + store them
    };
    _varHeader=[=](uint8_t* p){
        memcpy(p,&protocol,8);p+=8;
        return _poke16(p,_parent->_keepalive);
    };
    _protocolPayload=[=](uint8_t* p_pos,uint8_t* base){
        // [ClientID] --> ((Will properties)) --> [willTopic --> willPayload --> username --> password]
        while(!_blox.empty()){ // [ ] TODO: Will properties...
            mbx tmp=_blox.front();
            uint16_t n=tmp.len;
            uint8_t* p=tmp.data;
            p_pos=_poke16(p_pos,n);
            memcpy(p_pos,p,n);
            p_pos+=n;
            tmp.clear();
            _blox.pop();
        }
        mqttTraits T(base,_bs);
        H4AsyncMQTT::_outbound[0]=T;  // To be released on CONNACK
    };
    _build();
}

PublishPacket::PublishPacket(H4AsyncMQTT* p,const char* topic, uint8_t qos, bool retain, const uint8_t* payload, size_t length, bool dup,uint16_t givenId):
    _topic(topic),_qos(qos),_retain(retain),_length(length),_dup(dup),_givenId(givenId),Packet(p,PUBLISH,_qos) {

        if(length < _parent->getMaxPayloadSize()){
            _begin=[this]{ 
                _stringblock(_topic);
                _bs+=_length;
                byte flags=_retain;
                flags|=(_dup << 3);
            //
                if(_qos) {
                    _id=_givenId ? _givenId:(++_parent->_nextId);
                    flags|=(_qos << 1);
                    // _bs+=2; // because Packet id will be added (Is done through the constructor by passing _qos to _hasId parameter)
                    // _hasId = true;
                }
                _controlcode|=flags;
            // [ ] Add the properties to the length (property length) + store them

            };
            _varHeader=[this](uint8_t* p_pos){ // To embed the Topic name.
                mbx tmp=_blox.front();
                uint16_t n=tmp.len;
                uint8_t* p=tmp.data;
                p_pos=_poke16(p_pos,n);
                memcpy(p_pos,p,n);
                p_pos+=n;
                tmp.clear();
                _blox.pop();
                if(_hasId) p_pos=_poke16(p_pos,_id);
                // Properties...
                return p_pos;
            };

            _protocolPayload=[=](uint8_t* p,uint8_t* base){ 
                memcpy(p,payload,_length);
                mqttTraits T(base,_bs);
                if(_givenId) H4AsyncMQTT::_inbound[_id]=T;
                else if(_qos) H4AsyncMQTT::_outbound[_id]=T;
                else
                    h4.once(10000U /* H4AT_WRITE_TIMEOUT */,[=](){mbx::clear(base);}); // Initial fix to QoS 0 memory leak.
            };
            _build(_givenId);
        } else {
            H4AMC_PRINT1("PUB %d MPL=%d: NO CAN DO\n",length,_parent->getMaxPayloadSize());
            _parent->_notify(_givenId ? H4AMC_INBOUND_PUB_TOO_BIG:H4AMC_OUTBOUND_PUB_TOO_BIG,length);
        }
}