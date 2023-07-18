# MQTT v5.0

The library provides a comprehensive support of MQTT 5.0 standard, including:
- Session Expiry: Despite the default behavior is to connect with a clean session, Session Expiry field in CONNECT packet is configurable at compile-time.
- Message expiry: Allow an expiry interval to be set when a message is published via Publish Options.
- Handled (or) Exposed the received Reason Codes in: CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBACK, UNSUBACK, DISCONNECT, and AUTH
- Handled and Exposed Reason Strings on all received ACKs: It's automatically printed to the Serial with debug ON, and the user can set a callback to hook to the received reason strings.
- Handled Server disconnect, with a notify to the user with the reason.
- Payload format and content type: *An automatic handling of Payload format (Binary/Text) when publishing in the help with language type provided (vector<uint8_t> / string), the same for the receiving side; wherein the user can set a couple of callback messages (Binary/String).* Allow the user to specify MIME-style content type with publish options.
-  Request/Response: The library allows the user fetching the provided response topic and correlation data on the message callback, *also allows the user read response information provided by the server (if available)* that might help in response topic construction, this needs to be activated within configuration (Request Response Information).
-  Shared Subscription: The library allows for shared subscriptions to be made, and prevents trying to subscribe to a shared topic if the server doesn't support it.
-  Subscription ID: Supports custom callbacks out of custom subscriptions by utilizing the Subscription ID, Note: Both library configuration (Use Subscription Identifier) and the server support it.  
	In combination between Shared subscription and Subscription ID, a user is able to specify a custom callback for such a shared subscription.
-  Topic Alias: Automatic management of topic aliases that can reduce packets size, with respect to `MQTT5_RX_TOPIC_ALIAS_MAXIMUM`, `MQTT5_TX_TOPIC_ALIAS_MAXIMUM` configurations, and server's maximum.
-  Flow control: *Automatic pause of QoS>0 inflight publish messages to the Receive Maximum value*.
-  User properties: Allow user properties to be attached to most packets by three ways: 1. Static user properties 2. Dynamic user properties 3. Supplied user properties (Subscribe and Publish).  
   -  Static user properties can hold static information as the device name, ID, version. The user can set different static properties to different types of packets.  
	- Dynamic user properties can hold different values based on time as the user decides, as the EPOCH time of publish, user can assign them by setting a runtime callback function that will be called for the needed packet type.

	- At the moment, the supported packets that embed aforementioned user properties are Connect/(Un)Subscribe/Publish, soon for the rest of packets (AUTH,DISCONNECT,PUBACK,PUBREC,PUBREL,PUBCOMP).

	- Properties parsing are active for all packet types.
-  Maximum packet size: Controlled outbound packet size due to the server's configuration, inbound packet size limitation due to `MQTT_CONNECT_MAX_PACKET_SIZE`.
-  Proper handling based on the availability of server optional features: Maximum QoS, Retain, Wildcard Subscription, Subscription Identifier, and Shared Subscription. Runtime checks of subscribe-related features, and automatic management of publish ones, by decreasing of Publish QoS and Retain to the server's supplied option.
-  Enhanced authentication: A base design for extended authentication methods.
-  Subscription options: Allows the setting of subscription options in subscribe() call.
-  Handled Server Keep Alive.
-  Assigned ClientID: User gets notified and can get the assigned value by the server.
-  Server reference: A Reason Code by server's CONNACK or DISCONNECT indicating the usage of another server (temporarily or permenantly), with an optional Server reference. The library offers the user to set a callback onRedirect() with the server reference supplied as an argument, if received.




## MQTT v3.1.1 API compatibility
To a good extent, both MQTT version have a compatible set of APIs.


## Usage MQTT v5.0
In the [configuration file](../src/h4amc_config.h), uncomment the line:
```cpp
// #define MQTT_VERSION       0x04 // 3.1.1
#define MQTT_VERSION       0x05 // 5.0
```
