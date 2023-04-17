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
#include <H4AsyncMQTT.h>

using H4AMC_BLOCK_Q        = std::queue<mbx>;

class Packet {
    protected:
                H4AsyncMQTT*     _parent;
                uint16_t         _id=0; 
                bool             _hasId=false;
                uint8_t          _controlcode;
                H4AMC_BLOCK_Q    _blox;
                uint32_t         _bs=0;
                H4AMC_FN_VOID    _begin=[]{};   // begin to store properties?
                H4AMC_FN_U8PTRU8 _middle=[](uint8_t* p){ return p; }; // middle to serialize properties? or at _build()?
                H4AMC_FN_U8PTR   _end=[](uint8_t* p,uint8_t* base){}; // lose this?

                void	         _build(bool hold=false);
                void             _idGarbage(uint16_t id);
                void             _initId();
                void             _multiTopic(std::initializer_list<const char*> topix,uint8_t qos=0);
                uint8_t*         _poke16(uint8_t* p,uint16_t u);
                void             _stringblock(const std::string& s);
    public:
        Packet(H4AsyncMQTT* p,uint8_t controlcode,bool hasid=false): _parent(p),_controlcode(controlcode),_hasId(hasid){}
};

class ConnectPacket: public Packet {

            uint8_t  protocol[8]={0x0,0x4,'M','Q','T','T',H4AMC_VERSION,0};
    public:
        ConnectPacket(H4AsyncMQTT* p);
};

struct PubackPacket: public Packet {
    public:
        PubackPacket(H4AsyncMQTT* p,uint16_t id): Packet(p,PUBACK) { _idGarbage(id); }
};
class PubrecPacket: public Packet {
    public:
        PubrecPacket(H4AsyncMQTT* p,uint16_t id): Packet(p,PUBREC) { _idGarbage(id); }
};
class PubrelPacket: public Packet {
    public:
        PubrelPacket(H4AsyncMQTT* p,uint16_t id): Packet(p,PUBREL) { _idGarbage(id); }
};
class PubcompPacket: public Packet {
    public:
        PubcompPacket(H4AsyncMQTT* p,uint16_t id): Packet(p,PUBCOMP) { _idGarbage(id); }  
};
class SubscribePacket: public Packet {
    public:
        SubscribePacket(H4AsyncMQTT* p,const std::string& topic,uint8_t qos): Packet(p,SUBSCRIBE,true) { _multiTopic({topic.data()},qos); }
        SubscribePacket(H4AsyncMQTT* p,std::initializer_list<const char*> topix,uint8_t qos): Packet(p,SUBSCRIBE,true) { _multiTopic(topix,qos); }
};

class UnsubscribePacket: public Packet {
    public:
        UnsubscribePacket(H4AsyncMQTT* p,const std::string& topic): Packet(p,UNSUBSCRIBE,true) { _multiTopic({topic.data()}); }
        UnsubscribePacket(H4AsyncMQTT* p,std::initializer_list<const char*> topix): Packet(p,UNSUBSCRIBE,true) { _multiTopic(topix); }
};

class PublishPacket: public Packet {
        std::string     _topic;
        uint8_t         _qos;
        bool            _retain;
        size_t          _length;
        bool            _dup;
        uint16_t        _givenId=0;
    public:
        PublishPacket(H4AsyncMQTT* p,const char* topic, uint8_t qos, bool retain, const uint8_t* payload, size_t length, bool dup,uint16_t givenId=0);
};
