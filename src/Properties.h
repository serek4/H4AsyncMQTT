#pragma once
#include "h4amc_config.h"
#if MQTT5
#include "h4amc_common.h"
#include <vector>
#include <memory>

using MQTT_PROP_STRPAIR = std::pair<std::string,std::string>;
using MQTT_PROP_PARSERET= std::pair<H4AMC_MQTT_ReasonCode,uint8_t*>;
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
class MQTT_Properties {
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
    
	using H4AMC_NUMERIC_PROPS = std::vector<MQTT_Property_Numeric>;
	using H4AMC_STRING_PROPS = std::vector<MQTT_Property_String>;
	using H4AMC_BINARY_PROPS = std::vector<MQTT_Property_Binary>;
		H4AMC_NUMERIC_PROPS 				numeric_props;
		H4AMC_STRING_PROPS 					string_props;
		H4AMC_BINARY_PROPS 					binary_props;
		USER_PROPERTIES_MAP 				user_properties;
		
public:
		std::vector<H4AMC_MQTT5_Property> 	available_properties;
    			// For the user.
    			std::string 				operator[](const std::string& s) {
												auto& up=user_properties;
												return up.count(s) ? up[s] : std::string{};
											}

				MQTT_PROP_PARSERET 			parseProperties(uint8_t* data);

				USER_PROPERTIES_MAP 		getUserProperties()
											{
												return user_properties;
											}
				std::string 				getStringProperty(H4AMC_MQTT5_Property p);
    			uint32_t 					getNumericProperty(H4AMC_MQTT5_Property p);
                std::vector<uint8_t>        getBinaryProperty(H4AMC_MQTT5_Property p);

				bool 						isAvailable(H4AMC_MQTT5_Property p);

    // Make a template for this??
		static 	uint8_t* 					serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, uint32_t value);
		static 	uint8_t* 					serializeProperty(H4AMC_MQTT5_Property p, uint8_t *data, std::string value);
		static 	uint8_t* 					serializeProperty(H4AMC_MQTT5_Property p, uint8_t* data, std::vector<uint8_t> value);

		static 	uint8_t* 					serializeUserProperty(uint8_t *data, MQTT_PROP_STRPAIR& value);
		static 	uint8_t* 					serializeUserProperties(uint8_t *data, USER_PROPERTIES_MAP& map);


	// MQTT_Properties(uint8_t* start) { parseProperties(start); }
#if H4AMC_DEBUG
	void dump(){
		for (auto& p:numeric_props)
			p.print();
		for (auto& p:string_props)
			p.print();
		for (auto& p:binary_props)
			p.print();
		if (user_properties.size()){
			H4AMC_PRINT1("USER PROPERTIES %d:\n", user_properties.size());
			for (auto& p:user_properties)
				H4AMC_PRINT1("\"%s\":\"%s\"\n", p.first.c_str(), p.second.c_str());
		}
	}
#endif
};

struct MQTT5MessageProperties {
    	uint8_t 				payload_format_indicator;
    	uint32_t 				message_expiry_interval;
    	std::string 			content_type;
    	std::string 			response_topic;
    	std::vector<uint8_t> 	correlation_data;
    	H4T_NVP_MAP				user_properties;
};
struct MQTT5WillProperties : public MQTT5MessageProperties {
    uint32_t will_delay_interval;
};
struct MQTT5PublishProperties : public MQTT5MessageProperties {
private:
	friend class PublishPacket;
    uint16_t topic_alias=0;
};
#endif