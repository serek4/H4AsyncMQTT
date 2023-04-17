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

#if H4AMC_DEBUG
    std::map<uint8_t,char*> mqttTraits::pktnames={
        {0x10,"CONNECT"},
        {0x20,"CONNACK"},
        {0x30,"PUBLISH"},
        {0x40,"PUBACK"},
        {0x50,"PUBREC"},
        {0x60,"PUBREL"},
        {0x70,"PUBCOMP"},
        {0x80,"SUBSCRIBE"},
        {0x90,"SUBACK"},
        {0xa0,"UNSUBSCRIBE"},
        {0xb0,"UNSUBACK"},
        {0xc0,"PINGREQ"},
        {0xd0,"PINGRESP"},
        {0xe0,"DISCONNECT"}
    };
#endif

std::string mqttTraits::_decodestring(uint8_t** p){
    size_t tlen=_peek16(*p);//payload+=2;
    std::string rv((const char*) *(p)+2,tlen);
    *p+=2+tlen;
    return rv;
}

mqttTraits::mqttTraits(uint8_t* p,size_t s): data(p){
    type=data[0];
    flags=(data[0] & 0xf);
//  CALCULATE RL
//  [ ] DO "HAMZA'S" FIX
    uint32_t multiplier = 1;
    uint8_t encodedByte;//,rl=0;
    uint8_t* pp=&data[1];
    do{
        encodedByte = *pp++;
        offset++;
        remlen += (encodedByte & 0x7f) * multiplier;
        multiplier <<= 7;// multiplier *= 128;
        if (multiplier > 128*128*128)
        {
            H4AMC_PRINT1("MaMalformed Variable Byte Integer %d\n",remlen);
            // remlen=0;
            // Malformed packet....
            // [ ] v3.1.1 #4.8 "If the Client or Server encounters a Transient Error while processing an inbound Control Packet it MUST close the Network Connection on which it received that Control Packet"
        }
    } while ((encodedByte & 0x80) != 0);
    len=1+offset+remlen;
    H4AMC_PRINT4("PACKET SIZE=%d s=%d\n",len,s);
    if(s > len){
        H4AMC_PRINT4("STUFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED PACKET!!! %d > %d\n",s,len);
        next.first=p+len;
        next.second=s-len;
    }
/*
#if H4AMC_DEBUG
    if(s!=1+offset+remlen) H4AMC_PRINT3("SANITY FAIL! s=%d RL=%d offset=%d L=%d\n",s,remlen,offset,1+offset+remlen);
    else H4AMC_PRINT3("LL s=%d RL=%d offset=%d L=%d\n",s,remlen,offset,1+offset+remlen);
#endif
*/
    switch(type){
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
        case SUBSCRIBE:
        case SUBACK:
        case UNSUBSCRIBE:
        case UNSUBACK:
            id=_peek16(start());
            break;
        default:
            {
                if(isPublish()){
                    retain=flags & 0x01;
                    dup=(flags & 0x8) >> 3;
                    qos=(flags & 0x6) >> 1;
                    payload=start();
                    topic=_decodestring(&payload);
                    if(qos){ id=_peek16(payload);payload+=2; }
                    plen=data+len-payload;
                }
            }
            break;
    }

#if H4AMC_DEBUG
    H4AMC_PRINT1("MQTT %s\n",getPktName().data());
    H4AMC_PRINT2(" Data @ %p len=%d RL=%d\n",data,len,remlen);
    H4AMC_DUMP4(data,len);
    switch(type){
        case PUBACK:
        case PUBREC:
        case PUBREL:
        case PUBCOMP:
        case SUBACK:
        case UNSUBACK:
            H4AMC_PRINT3("  id: %d\n",id);
            break;
        case CONNECT:
            {
                uint8_t cf=data[9];
                H4AMC_PRINT3("  Protocol: %s\n",data[8]==4 ? "3.1.1":stringFromInt(data[8],"0x%02x").data());
                H4AMC_PRINT4("  Flags: 0x%02x\n",cf);
#if MQTT5
                H4AMC_PRINT3("  Session: %s\n",((cf & CLEAN_START) >> 1) ? "Clean":"Dirty");
#else
                H4AMC_PRINT3("  Session: %s\n",((cf & CLEAN_SESSION) >> 1) ? "Clean":"Dirty");
#endif
                H4AMC_PRINT3("  Keepalive: %d\n",_peek16(&data[10]));
                uint8_t* sp=&data[12];
                H4AMC_PRINT3("  ClientId: %p %s\n",sp,_decodestring(&sp).data());
                if(cf & WILL){
                    H4AMC_PRINT3("  Will Topic: %p %s\n",sp,_decodestring(&sp).data());
                    if(cf & WILL_RETAIN) H4AMC_PRINT3("  Will: RETAIN\n");
                    H4AMC_PRINT3("  Will QoS: %d\n",(cf >> 3) &0x3);
                    H4AMC_PRINT3("  Will Message: %p %s\n",sp,_decodestring(&sp).data());
                } 
                if(cf & USERNAME) H4AMC_PRINT3("  Username: %s\n",_decodestring(&sp).data());
                if(cf & PASSWORD) H4AMC_PRINT3("  Password: %s\n",_decodestring(&sp).data());
                break;
            }
        case CONNACK:
            {
                switch(data[3]){
                    case 0: // 0x00 Connection Accepted
                        H4AMC_PRINT3("  Session: %s\n",((data[2]) & 1) ? "Present":"None");
                        break;
                    case 1: // 0x01 Connection Refused, unacceptable protocol version
                        H4AMC_PRINT3("  Error: %s\n","unacceptable protocol version");
                        break;
                    case 2: // 0x02 Connection Refused, identifier rejected
                        H4AMC_PRINT3("  Error: %s\n","client identifier rejected");
                        break;
                    case 3: // 0x03 Connection Refused, Server unavailable
                        H4AMC_PRINT3("  Error: %s\n","Server unavailable");
                        break;
                    case 4: // 0x04 Connection Refused, bad user name or password
                        H4AMC_PRINT3("  Error: %s\n","bad user name or password");
                        break;
                    case 5: // 0x05 Connection Refused, not authorized
                        H4AMC_PRINT3("  Error: %s\n","not authorized");
                        break;
                    default: // ??????
                        H4AMC_PRINT3("  SOMETHING NASTY IN THE WOODSHED!!\n");
                        break;
                }
            }
            break;
        case SUBSCRIBE:
        case UNSUBSCRIBE:
            {
                H4AMC_PRINT3("  id: %d\n",id);
                uint8_t* payload=start()+2;
                do {
                    uint16_t len=_peek16(payload);
                    payload+=2;
                    std::string topic((const char*) payload,len);
                    payload+=len;
                    if(type==SUBSCRIBE) {
                        uint8_t qos=*payload++;
                        H4AMC_PRINT3("  Topic: QoS%d %s\n",qos,topic.data());
                    } 
                    else H4AMC_PRINT3("  Topic: %s\n",topic.data());
                } while (payload < (data + len));
            }
            break;
        default:
            {
                if(isPublish()){
                    if(qos) H4AMC_PRINT3("  id: %d\n",id);
                    H4AMC_PRINT3("  qos: %d\n",qos);
                    if(dup) H4AMC_PRINT3("  DUP\n");
                    if(retain) H4AMC_PRINT3("  RETAIN\n");
                    H4AMC_PRINT3("  Topic: %s\n",topic.data());
                    H4AMC_PRINT3("  Payload size: %d\n",plen);
                }
                else Serial.printf("WTF99999999999999999999999?\n");
            }
            break;
    }
}

void mqttTraits::dump(){
    Serial.printf("PKTDUMP %s @ %p len=%d RL=%d off=%d flags=0x%02x\n",getPktName().data(),data,len,remlen,offset,flags);
    Serial.printf("PKTDUMP %s id=%d qos=%d dup=%d ret=%d PR=%d PL=%p L=%d \n",topic.data(),id,qos,dup,retain,pubrec,payload,plen);
    dumphex(data,len);
}
#else
}
#endif