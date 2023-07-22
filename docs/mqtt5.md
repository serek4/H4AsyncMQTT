# MQTT v5.0

The library provides a comprehensive support of MQTT 5.0 standard, including:
For short, the MQTT 5.0 comes with features I'd prefer to bundle in those categories:
- **Publish features:**
  The standard came out with a whole vector of improvements regarding publishes, which is Message Properties.
  The Properties are standardized ones that help describe the message and processing it. The standard highlighted those:
  - **Message expiry**: Runtime expiry interval setting per message publish, that will be discarded if expired.
  - ***Payload format and content type***: Fields checking the payload format and further Content Type. An automatic handling of Payload format (Binary/Text) when publishing in the help with language type provided (std::string, String, char). *the same applies for the receiving side; wherein the user can set a couple of callback messages (Binary/String).* Allow the user to specify MIME-style content type with publish options.
  -  **Request/Response**: Mimics the HTTP protocol which is Request/Response within MQTT Pub/Sub protocol, with the help of a custom property tells the Response Topic, and another property can hold correlation data.
   The library allows the user fetching the provided response topic and correlation data on the message callback, The server might have Response Information that can help in constructing the Response topic. The library allows getting the response information provided by the server (By `getServerOptions().response_information`). However, requesting response information needs to be activated within configuration (Request Response Information).
  -  **User properties**: Customized Properties as the application requires, which is a name-value string pair. The library allow for "User Properties" to be hooked automatically to most packets by three ways: 1. Static name-value user properties 2. Dynamic user properties 3. Supplied user properties (Subscribe and Publish).  
     -  Static user properties can hold static information as the device name, ID, version. The user can set different static properties to different types of packets.  
  	- Dynamic user properties can hold variable values, such as the time of publish, the user can assign callback functions to construct them accordingly.
	NOTE: At the moment, the supported packets that embed aforementioned user properties are Connect/(Un)Subscribe/Publish, soon for the rest of packets (AUTH,DISCONNECT,PUBACK,PUBREC,PUBREL,PUBCOMP).

	- Properties Parsing: The library does parse all properties.
	
- **Subscription features:**
  -  **Shared Subscription**: As some applications may need to balance loads to a set of clients, in which only one client receives a publish to the shared topic. The library allows for shared subscriptions to be made, and prevents trying to subscribe to a shared topic if the server doesn't support it.
  -  **Subscription ID**: Each subscription utilizes a custom ID, the corresponding received publish carries the ID, in which to direct it to the subscriber in interest. The library supports custom callbacks out of custom subscriptions by utilizing the Subscription ID. Note: Requires library configuration (Use Subscription Identifier) and server support.  
  	In combination between Shared subscription and Subscription ID, a user is able to specify a custom callback for such a shared subscription.
  -  **Subscription options**: More options that customizes the subscription beside the QoS, "No Local" option causes the publishes that originates from the client not to be received, "Retain As Published" option does keep the retain flag as it was published, else would be cleared, "Retain Handling" option handles the reception of matching retained publishes at the time of subscribe, `0` Sends upon subscribe, `1` Sends upon subscribe only if the subscription doesn't currently exist, `2` Doesn't send any matching retained publishes. The library allows the setting of subscription options in `subscribe()` call, with defaults `No Local = 0`, `Retain As Published = 1`, and `Retain Handling = 1`.

- **Server-Concerning features:**
  - **Session Expiry**: A configurable field tells the server when to remove the session upon a client goes offline. The client-side configuration is [configurable at compile-time](#configurations).
  - **Availability of Optional Server Features**: Runtime determination of Server's limits and features availability, such as: Maximum QoS, Retain, Wildcard Subscription, Subscription Identifier, and Shared Subscription. The library performs runtime checks of subscribe-related features, and manages the user publishes accordingly by decreasing of Publish QoS and Retain to the server's supplied option at CONNACK.
  - **Server disconnect**: The server now can issue disconnect packet to the client with optionally reason codes, strings, and other properties. The library handles with a notify to the user.
  - **Server reference**: Optionally the server can redirect the client to use another server, temporarily or permanently on CONNACK or DISCONNECT, and an optional mention of the Server reference. The library offers to the user to set a callback `onRedirect()` to [handle the redirection](#setting-callbacks-will-and-connect), the callback is optionally carries the server reference where supplied as an argument, until a good maturity level of utilizing this feature, the user has the responsibility of parsing and connecting with the other server.
  - **Server Keep Alive**: The server can override the client Keep Alive value. 

- **Overall Protocol improvements:**
  -  **Topic Alias**: Does contribute in shortening packets size by mapping a numeric value to a topic string. The library performs an automatic management of topic aliases with respect to `MQTT5_RX_TOPIC_ALIAS_MAXIMUM`, `MQTT5_TX_TOPIC_ALIAS_MAXIMUM` [configurations](#configurations), and server's maximum.
  - **Reason Codes** accompanied with ACK packets: CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBACK, UNSUBACK, DISCONNECT, and AUTH. *The library does handle or notify the user for such reason codes*.
  - **Optional Reason Strings** with ACK packets: String-based reasons made primarily for debugging. These strings are automatically printed to the Serial (Requires debug activated), user can receive them by setting a callback `onReason()`.
  -  **Flow control**: Both client and server can identify their own maximum inflight messages under ACK (for QoS>0 - which requires ACKs). The library performs *an automatic pause of QoS>0 inflight publish messages to the Receive Maximum value*.
  -  **Maximum packet size**: Both client and server can identify their own maximum received packet size. The library Controls outbound packet size due to the server's limit, and performs inbound packet size limitation based on `MQTT_CONNECT_MAX_PACKET_SIZE`.
  -  **Enhanced authentication**: For the client to authenticate against the server, and vise-versa. The library provides a base design for extended authentication methods.
  -  **Assigned ClientID**: User gets notified and can get the assigned clientId by the server.



## MQTT v3.1.1 API compatibility
To a good extent, both MQTT version have a compatible set of APIs.


## Usage MQTT v5.0
In the [configuration file](../src/h4amc_config.h), uncomment the line:
```cpp
// #define MQTT_VERSION       0x04 // 3.1.1
#define MQTT_VERSION       0x05 // 5.0
```

### Setting callbacks, will, and connect:
```cpp
H4AsyncMQTT mqttClient;
void printUserProperties(USER_PROPERTIES_MAP& up) {
	for (auto p : up) {
		Serial.printf("%s:%s\n", p.first.c_str(), p.second.c_str());
	}
	Serial.printf("\n");
}
void onMqttError(int e,int i){
  if(e < H4AMC_ERROR_BASE){
    Serial.printf("H4ASYNC ERROR %d [%s] info=%d[%p]\n",e,H4AsyncClient::errorstring(e).data(),i,i);
  }
  else {
    if(e < H4AMC_ERROR_MAX){
       Serial.printf("H4AsyncMQTT ERROR %d [%s] info=%d[%p]\n",e,H4AsyncMQTT::errorstring(e).data(),i,i);   
    }
    else Serial.printf("UNKNOWN ERROR: %u extra info %d[%p]\n",e,i,i);
  }
}

void onMqttConnect(H4AMC_ConnackParam params) {
  	Serial.printf("USER: Connected as %s MP=%d session %d\n",mqttClient.getClientId().data(),getMaxPayloadSize(), params.session);
	if (params.connack_props.size()){
		Serial.printf("CONNACK Properties:\n");
		printUserProperties(params.connack_props);
	}
}

void onMqttMessage(const char* topic, const uint8_t* payload, size_t len, H4AMC_MessageOptions opts) {
	Serial.printf("Receive: H=%u Message %s qos%d dup=%d retain=%d len=%d\n",_HAL_freeHeap(),topic,opts.qos,opts.dup,opts.retain,len);
	auto props=opts.getProperties();
	Serial.printf("Indicator%d Expiry%d Type \"%s\" Response topic \"%s\"\n", props.payload_format_indicator, props.message_expiry_interval, props.content_type.c_str(), props.response_topic.c_str());
	if (props.correlation_data.size()){
		Serial.printf("Correlation Data:\n");
		dump(&props.correlation_data[0], props.correlation_data.size());
	}
	if (props.user_properties.size()){
		Serial.printf("Message Properties:\n");
		printUserProperties(props.user_properties);
	}

	//dumphex(payload,len);
	//Serial.println();
}

void onMqttDisconnect() { }

void onMqttRedirect(const std::string& server_reference) {
	Serial.printf("Server Redirect %s\n", server_reference.c_str());
	// User handles redirection.
}

void onMqttReasonString(const std::string& reason) {
	Serial.printf("Reason String \"%s\"\n", reason.c_str());
}

void onMqttPublish(uint16_t id) { 
	// One might manage published qos>0 message by their ids. 
}
void setup() {
	mqttClient.onError(onMqttError);
	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.onMessage(onMqttMessage);
	mqttClient.onRedirect(onMqttRedirect);
	mqttClient.onReason(onMqttReasonString);
	mqttClient.onPublish(onMqttPublish);


	// mqttClient.setWill("DIED",2,"probably still some bugs",false);
	// OR .. To set will properties:
	H4AMC_WillOptions options(false);
	auto& props = options.getProperties();
	props.payload_format_indicator=H4AMC_PAYLOAD_FORMAT_STRING;
	// props.xxx=yyy
	mqttClient.setWill("DIED",2,"probably still some bugs",options);

	mqttClient.connect(MQTT_URL,mqAuth,mqPass);
}

```

### Publishing:
Please refer to [MQTT Payload handling](pl.md).
```cpp
void myPublish(const char* topic, const char* payload, uint8_t qos, bool retain, MQTT5PublishProperties& properties){
	H4AMC_PublishOptions opts(retain,properties);
	mqttClient.publish(topic, payload, strlen(payload), qos, opts);
}

{ // publish scope
	MQTT5PublishProperties props;
	props.message_expiry_interval=50;
  	// props.payload_format_indicator;
 	// props.message_expiry_interval;
 	// props.content_type;
 	// props.response_topic;
 	// props.correlation_data;
	USER_PROPERTIES_MAP user_props;
	user_props["key"]="value";

	props.user_properties=user_props;

	myPublish("myTopic", "myPayload", 2, false, props);
}
```
### Subscribe with Options and User Properties:
**Note:**: Although the following code snippets are out of context, it's highly recommended to subscribe within `onConnect()` callback function.

```cpp
uint8_t qos=2;
mqttClient.subscribe("topic/example/qos",qos); // MQTT v3.1.1 compatible API

USER_PROPERTIES_MAP subscribeProps{{"From","H4AsyncMQTT"}};
bool noLocal=1;
bool retainAsPublished=1;
uint8_t retainHandling=1;
H4AMC_SubscriptionOptions opts{qos,nullptr,noLocal,retainAsPublished,retainHandling,subscribeProps}
mqttClient.subscribe("topic/example/opts",opts);
```

### Subscription ID:
```cpp
void customCallback(const char* topic, const uint8_t* payload, size_t len, H4AMC_MessageOptions opts) {
	Serial.printf("customCallback: H=%u Message %s qos%d dup=%d retain=%d len=%d\n",_HAL_freeHeap(),topic,opts.qos,opts.dup,opts.retain,len);
	// ... As onMqttMessage()...
	//dumphex(payload,len);
	//Serial.println();
}

auto subId = mqttClient.subscribe("topic/example/customCallback", H4AMC_SubscriptionOptions{2,customCallback});
// ...
mqttClient.unsubscribe(subId);
```
### Shared Subscriptions:
This example utilizes Subscription ID to route the subscription to a custom callback function.
```cpp
void shareCallback(const char* topic, const uint8_t* payload, size_t len, H4AMC_MessageOptions opts) {
	Serial.printf("shareCallback: H=%u Message %s qos%d dup=%d retain=%d len=%d\n",_HAL_freeHeap(),topic,opts.qos,opts.dup,opts.retain,len);
	// ... As onMqttMessage()...
	//dumphex(payload,len);
	//Serial.println();
}

mqttClient.subscribe("$share/ShareName/filter", H4AMC_SubscriptionOptions{2,customCallback});
```
### Static and Dynamic Properties:
#### Static Properties:
```cpp
USER_PROPERTIES_MAP user_props{
	{"Client":"H4AsyncMQTT "H4AMC_VERSION},
	{"Device Serial":"123456"}
}
auto ret = mqttClient.addUserProp(PUBLISH, user_props); // returns false if invalid packet supplied.
// Add to multiple packets:
mqttClient.addUserProp({CONNECT,PUBLISH,SUBSCRIBE}, user_props);
```
#### Dynamic Properties:
```cpp
USER_PROPERTIES_MAP dynamicPropCB(PacketHeader header) {
	// switch(header) {...} // suitable if different headers gets different properties.
	USER_PROPERTIES_MAP user_props;
	if (header == PUBLISH) {
		user_props["time"]=millis(); // OR better: EPOCH time.
	}
	return user_props;
}

auto ret = mqttClient.addDynamicUserProp({PUBLISH,SUBSCRIBE}, dynamicPropCB);
```

### Authentication:
```cpp
SCRAM_Authenticator authenticator;
mqttClient.setAuthenticator(&SCRAM_Authenticator);

```
**Note: Unimplemented**
### Other APIs:
```cpp
Server_Options      getServerOptions(); // Gets the server options.
void                resetUserProps(); // Clears the Static and Dynamic maps.
```

### Configurations:
```cpp

#define MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT           1		// Comment or Set to zero if desired, this will reduce binary size and enhance performance a bit.
#define MQTT5_RX_TOPIC_ALIAS_MAXIMUM                    50 		// Receive Topic Alias Maximum 
#define MQTT5_TX_TOPIC_ALIAS_MAXIMUM                    50		// Transmit Topic Alias Maximum, to limit the server's Topic Alias Maximum if received a higher value.
#define MQTT5_RX_MAX_PACKET_SIZE                        5200	// RX Max Packet Size limit
#define MQTT5_RECEIVE_MAXIMUM                           25		// Receive Maximum limit
#define MQTT5_SESSION_EXPIRY_INTERVAL                   3600	// MQTT Session Expiry In seconds.



// CONNECT Properties / Options
#define MQTT_CONNECT_REQUEST_RESPONSE_INFORMATION       0
#define MQTT_CONNECT_REQUEST_PROBLEM_INFORMATION        0

// SUBSCRIBE Default Options
#define MQTT5_SUBSCRIPTION_OPTION_NO_LOCAL               0 // Default behaviour for MQTT v3.3, and it's protocol error to set it to 1 for Shared Subscriptions in MQTT v5.0
#define MQTT5_SUBSCRIPTION_OPTION_RETAIN_AS_PUBLISHED    1
#define MQTT5_SUBSCRIPTION_OPTION_RETAIN_HANDLING        1

```