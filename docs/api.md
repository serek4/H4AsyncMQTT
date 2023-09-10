![plainhdr](../assets/pangoplain.jpg)

# Main API Contents

- [Callbacks](#callbacks)
- [API](#api)
  - [URL defintion](#url-defintion)
    - [Valid examples](#valid-examples)
  - [Using TLS](#using-tls)
  - [Functions](#functions)
  - [Running MQTT v5.0](#running-mqtt-v50)
- [Advanced Topics](#advanced-topics)
- [Find me daily in these FB groups](#find-me-daily-in-these-fb-groups)

---

## Callbacks

```cpp
// M A N D A T O R Y ! ! !
void cbConnect(H4AMC_ConnackParam params); // for MQTT 3.1.1, params holds session = whether connection has started with a dirty session
// For MQTT 5.0, it holds also the CONNACK Properties (member connack_props)
// M A N D A T O R Y  if subscribing:
/* For MQTT 3.1.1 opts holds qos,retain, and dup values
For MQTT 5.0 it also includes properties.
can be fetched by opts.getProperties();
 */
void cbMessage(const char* topic, const uint8_t* payload, size_t len,H4AMC_MessageOptions opts);

// O P T I O N A L
void cbDisconnect();
void cbPublish(PacketID id);// For Publishes > 0, the user can know the publish is fully acked by receiving its id via this callback.
void cbRedirect(const std::string& reference);// For MQTT5 a server redirect is handled by a callback.
void cbReason(const std::string& reason);// For MQTT5 a reason string might be received, this callback catches it.

/*
LwIP produces [error codes](https://www.nongnu.org/lwip/2_0_x/group__infrastructure__errors.html) with negative value and these get fed back up through H4AsyncTCP eventually to this library, which also has a few of its own valid reasons. How to tell the difference? All of Pangolin's are +ve, but if you get a rare underlying TCP error which will help in diagnosing problems, you will also get told that (-ve) reason code
*/
void cbError(int error,int info); // info is additional information about the error whose code is one of:
/*
H4AMC_SUBSCRIBE_FAIL, // invalid topic provided
H4AMC_UNSUBSCRIBE_FAIL, // invalid topic provided / No subscription
H4AMC_INBOUND_QOS_ACK_FAIL, // an ID has been provided by the server that is no longer held by us (usually after crash/reboot with open session)
H4AMC_OUTBOUND_QOS_ACK_FAIL,// an ID has been provided by the server that is no longer held by us (usually after crash/reboot with open session)
H4AMC_OUTBOUND_QOS_ACK_FAIL, // someone sent you a waaaaaay too big message
H4AMC_OUTBOUND_PUB_TOO_BIG, // you tried to send out a a waaaaaay too big message
H4AMC_BOGUS_PACKET, // should never happen - server sent malformed / unrecognised packet - SERIOUS PROBLEM
H4AMC_X_INVALID_LENGTH, // length of payload does not match expected data type in x functions - server sent malformed message - SERIOUS PROBLEM
H4AMC_USER_LOGIC_ERROR,  // Calling any API that requires a connection to the server
H4AMC_NO_SSL, // Tried to call secureTLS() but you've not built correctly, refer to tls document
H4AMC_NOT_RUNNING, // The library isn't connected to the server at the moment!
// FOR MQTT5 ONLY:
H4AMC_SERVER_DISCONNECT, // the server sends Disconnect.
H4AMC_NO_SERVEROPTIONS, // No server options found!
H4AMC_PUBACK_FAIL,  // Got Reason Code >0x00 for the publish, check the provided reason code held in info and checkout MQTT5 reason codes.
H4AMC_PUBREC_FAIL, // Same as PUBACK
H4AMC_PUBREL_FAIL, // SAME as prev
H4AMC_PUBCOMP_FAIL, // SAME as prev
H4AMC_SERVER_RETAIN_UNAVAILABLE, // If you tried to publish with retain wherein the server doesn't support or permit it.
H4AMC_SHARED_SUBS_UNAVAILABLE, // If you tried to subscribe a shared topic while the server doesn't support/permit it.
H4AMC_BAD_SHARED_TOPIC, // You supplied a bad shared topic.
H4AMC_BAD_TOPIC, // You're trying to publish to bad topic (as one containing a wild card)
H4AMC_WILDCARD_UNAVAILABLE, // If you're tring to subscribe using a wildcard wherein the server doesn't support/permit it.
H4AMC_ASSIGNED_CLIENTID,// The server just assigned you a clientID
H4AMC_NO_AUTHENTICATOR, // The library failed to find an authenticator assigned.
H4AMC_INVALID_AUTH_METHOD, // The Authenticator packet to send doesn't hold a method.
H4AMC_SUBID_NOT_FOUND,     // Invalid subscription-id which tried to unsubscribe with
H4AMC_PROTOCOL_ERROR, // Protocol Error is called internally, disconnecting to the server
H4AMC_INCOMPLETE_PACKET,  // An incomplete message is received (Remaining Length is larger than received). This happens usually when the server attaches PUSH TCP Flag to all of its packets (each MQTT Segment) to the client, which results in H4AMC receiving a chunk of the message from the TCP, which is an incomplete message!

*/
```

---

## API

### URL defintion

The url must be specified in the following general form. The extended path and query portions are optional, as is the port. If the port is omitted it will default to 80 for URLs starting `http` and 443 for those starting `https`

`http://hostname:port/path/to/resource?a=b&c=d"`

or

`https://hostname:port/path/to/resource?a=b&c=d"`

The `hostname` portion my be specified as a "dotted quad" IP address e.g. "172.103.22.14" or a publicly resolvable DNS name e.g. `somehost.co.uk`

#### Valid examples

- `http://192.168.1.15` // defaults to port 80
- `https://myremotehost.com/api?userid=123456` // default to port 443
- `https://mosquitto.local:8883`
- `http://insecure.remote.ru:12345/long/resource/path/?data=123&moredata=456`

### Using TLS

Refer to [TLS document](tls.md) for more information.

Note that this will significantly increase the size of the compiled app. Unless you absolutely need it, do not compile in TLS!

### Functions

```cpp
void connect(const char* server, const char* auth, const char* psk, std::string clientId=""); // CHANGED IN V3
void disconnect(); // CHANGED IN V3

std::string getClientId(); // CHANGED IN V3
size_t getMaxPayloadSize();

void onMqttConnect(H4AMC_cbConnect callback); // mandatory: set connect handler V3: REPLACES onConnect
void onMqttDisconnect(H4AMC_cbDisconnect callback);// optional: set disconnect handler V3: REPLACES onDisconnect
void onMqttError(H4AMC_cbError callback);// optional: set error handler  V3: REPLACES onError
void onMqttMessage(H4AMC_cbMessage callback); // // mandatory if subscribing: set topic handler  V3: REPLACES onMessage
std::string getClientId(); // Gets the clientID, reasonable for MQTT5

/* API to secure with TLS  */
bool secureTLS(const u8_t *ca, size_t ca_len, const u8_t *privkey = nullptr, size_t privkey_len=0,
                            const u8_t *privkey_pass = nullptr, size_t privkey_pass_len = 0,
                            const u8_t *cert = nullptr, size_t cert_len = 0);

void informNetworkState(NetworkState state); // If WiFi Disconnected, you might inform with network awareness, BUT if you informed with network disconnection, you MUSH either inform with network connected, or call the connect() API
void publish(const char* topic,const uint8_t* payload, size_t length, uint8_t qos=0,  H4AMC_PublishOptions opts={}); // opts holds retain flag for MQTT 3.1.1, and further holds MQTT5PublishProperties
void publish(const char* topic,const char* payload, size_t length, uint8_t qos=0,  H4AMC_PublishOptions opts={});
template<typename T>
void publish(const char* topic,T v,const char* fmt="%d",uint8_t qos=0,H4AMC_PublishOptions opts={});

void setKeepAlive(uint16_t keepAlive); // probably best left alone... note actual rate is H4AMC_POLL_RATE * keepAlive; and depends on your LwIP
void setServer(const char* url,const char* username="", const char* password = "",const uint8_t* fingerprint=nullptr); // V3: CHANGED
void setWill(const char* topic, uint8_t qos, bool retain, const char* payload = nullptr); // optional

// opts_qos can hold Only QoS for MQTT 3.1.1, one can just set the QoS value in. For MQTT5 it can be supplied with further parameters regarding a subscription: such as Subscription Options, callback function, and user properties. 
// Returns the Subscription ID (For MQTT 5.0) if any. Otherwise 0.
uint32_t subscribe(const char* topic, H4AMC_SubscriptionOptions opts_qos={});
uint32_t subscribe(std::initializer_list<const char*> topix, H4AMC_SubscriptionOptions opts_qos={});
void unsubscribe(const char* topic);
void unsubscribe(std::initializer_list<const char*> topix);
#if MQTT_SUBSCRIPTION_IDENTIFIERS_SUPPORT
// This fetches the topics out of the stored map, then unsubscribes them all once. and remove the resources.
void unsubscribe(uint32_t subscription_id);
#endif


void xPublish(const char* topic,const char* value, uint8_t qos=0,  H4AMC_PublishOptions opts={});
void xPublish(const char* topic,String value, uint8_t qos=0,  H4AMC_PublishOptions opts={});
void xPublish(const char* topic,std::string value, uint8_t qos=0,  H4AMC_PublishOptions opts={});
template<typename T>
void xPublish(const char* topic,T value, uint8_t qos=0,  H4AMC_PublishOptions opts={})

void xPayload(const uint8_t* payload,size_t len,char*& cp); // YOU MUST FREE THE POINTER CREATED BY THIS CALL!!!
void xPayload(const uint8_t* payload,size_t len,std::string& ss);
void xPayload(const uint8_t* payload,size_t len,String& duino);
template<typename T>
void xPayload(const uint8_t* payload,size_t len,T& value);


// ONLY FOR MQTT5:
static void printUserProperty(H4AMC_USER_PROPERTIES& props); // A helping function to pring User Properties
void onRedirect(H4AMC_FN_STRING f); // Sets server redirect callback
void onReason(H4AMC_FN_STRING f); // Sets Reason String callback
void setAuthenticator(H4Authenticator* authenticator); // Sets an authenticator
Server_Options getServerOptions(); // Gets the server options (Valid Only if CONNECTED)

 // Valid for CONNECT, PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, UNSUBSCRIBE, DISCONNECT, AUTH
bool addStaticUserProp(PacketHeader header, H4AMC_USER_PROPERTIES user_properties); // Adds a static User Property to certain packet header.
bool addStaticUserProp(std::initializer_list<PacketHeader> headers, H4AMC_USER_PROPERTIES user_properties); // Adds a static User Property to several certain packet headers.
bool addDynamicUserProp(PacketHeader header, H4AMC_FN_DYN_PROPS f); // Sets a callback function that must return User Properties for a certain packet header.
bool addDynamicUserProp(std::initializer_list<PacketHeader> headers, H4AMC_FN_DYN_PROPS f); // Sets a callback function for several packet headers
void resetUserProps(); // Clears the maps holding properties info (State/Dynamic).


```

---

### Running MQTT v5.0

MQTT standard version 5.0 comes with good pack of features. Refer to [MQTT5 Document](mqtt5.md) for more information and how to use it.

## Advanced Topics

H4AsyncMQTT comes with some built-in diagnostics. These can be controlled by setting the debug level in `config.h`

```cpp
/*
    Debug levels: 
    0 - No debug messages
    1 - connection / disconnection messages
    2 - level 1 + MQTT packet types
    3 - level 2 + MQTT packet data
    4 - everything
*/

#define H4AMC_DEBUG 1
```

You can also include your own using `H4AMC_PRINTx` functions (where x is 1-4).
These operate just like `printf` but will only be compiled-in if the H4AMC_DEBUG level is x or greater

```cpp
H4AMC_PRINT3("FATAL ERROR %d\n",errorcode); 
```

Will only be compiled if user sets H4AMC_DEBUG to 3 or above

---

## Find me daily in these FB groups

- [Pangolin Support](https://www.facebook.com/groups/H4AsyncMQTT/)
- [ESP8266 & ESP32 Microcontrollers](https://www.facebook.com/groups/2125820374390340/)
- [ESP Developers](https://www.facebook.com/groups/ESP8266/)
- [H4/Plugins support](https://www.facebook.com/groups/h4plugins)

I am always grateful for any $upport on [Patreon](https://www.patreon.com/esparto) :)

(C) 2020 Phil Bowles
