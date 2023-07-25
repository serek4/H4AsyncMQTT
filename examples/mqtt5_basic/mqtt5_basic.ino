#include <Arduino.h>
#include <H4.h>
/* ENSURE the proper MQTT_VERSION is defined i <h4amc_config.h> file */
#include <H4AsyncMQTT.h>
#include <H4Tools.h>

H4 h4(115200);
H4AsyncMQTT mqttClient;

// #define MQTT_URL "https://192.168.1.21:8883"
#define MQTT_URL "192.168.1.34:1883"

// If using MQTT server authentication, fill in next two fields!
const char *mqAuth = "example";
const char *mqPass = "pangolin";

const char *pload = "Save the Pangolin!";
H4_TIMER sender;

void onMqttError(int e, int i){
	if (e < H4AMC_ERROR_BASE){
		Serial.printf("H4ASYNC ERROR %d [%s] info=%d[%p]\n", e, H4AsyncClient::errorstring(e).data(), i, i);
	}
	else{
		if (e < H4AMC_ERROR_MAX){
			Serial.printf("H4AsyncMQTT ERROR %d [%s] info=%d[%p]\n", e, H4AsyncMQTT::errorstring(e).data(), i, i);
		}
		else
			Serial.printf("UNKNOWN ERROR: %u extra info %d[%p]\n", e, i, i);
	}
}

void printProperties(MQTT5PublishProperties& props) {
	Serial.printf("Properties:\n");
	if (props.content_type.length()) Serial.printf("Content-Type: %s\n", props.content_type.c_str());
	if (props.response_topic.length()) Serial.printf("Response Topic: %s\n", props.response_topic.c_str());
	if (props.message_expiry_interval) Serial.printf("Message Expiry: %u\n", props.message_expiry_interval);
	if (props.payload_format_indicator) Serial.printf("Payload Format Indicator: %u\n", props.payload_format_indicator);
	if (props.correlation_data.size()){ Serial.printf("Correlation Data:\n"); dumphex(props.correlation_data.data(), props.correlation_data.size());}
	if (props.user_properties.size()) {
		Serial.printf("User Properties: (%d)\n", props.user_properties.size());
		for (auto& up:props.user_properties)
			Serial.printf("\"%s\":\"%s\"\n", up.first.c_str(), up.second.c_str());
	}
}

void onMqttConnect(H4AMC_ConnackParam params)
{
	Serial.printf("USER: Connected as %s MP=%d\n", mqttClient.getClientId().data(), getMaxPayloadSize());
	H4AMC_SubscriptionOptions subOpts;
	subOpts.setQos(2);
	subOpts.setNoLocal(true);
	// subOpts.setXXX();

	mqttClient.subscribe({"test", "multi2", "fully/compliant"}, subOpts);
	mqttClient.unsubscribe({"multi2", "fully/compliant"});

	sender = h4.every(5000, []
					  {
						Serial.printf("T=%u Publish:\n",millis());
						mqttClient.publish("test",pload,strlen(pload),2); // normal string publish
						
						MQTT5PublishProperties properties;
						properties.message_expiry_interval=10;
						properties.user_properties = H4AMC_USER_PROPERTIES{{"target","heap"}};
						H4AMC_PublishOptions options{false, properties};// false=no retain

						mqttClient.publish("test",_HAL_freeHeap(),"%u",2,options); 

						// OR JUST
						// mqttClient.publish("test",_HAL_freeHeap(),"%u",2,H4AMC_PublishOptions{false,MQTT5PublishProperties{0,10,"","",{},H4AMC_USER_PROPERTIES{{"target","heap"}}}}); 

					});
}

void onMqttMessage(const char *topic, const uint8_t *payload, size_t len, H4AMC_MessageOptions opts)
{
	Serial.printf("Receive: H=%u Message %s qos%d dup=%d retain=%d len=%d\n", _HAL_freeHeap(), topic, opts.qos, opts.dup, opts.retain, len);
	auto properties = opts.getProperties();
	printProperties(properties);
	// dumphex(payload,len);
}

void onMqttDisconnect()
{
	Serial.printf("USER: Disconnected from MQTT\n");
	h4.cancel(sender);
}

void h4setup()
{
	Serial.printf("\nH4AsyncMQTT v%s running @ debug level %d heap=%u\n", H4AMC_VERSION, H4AMC_DEBUG, _HAL_freeHeap());

	mqttClient.onError(onMqttError);
	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.onMessage(onMqttMessage);
	//  mqttClient.setServer(MQTT_URL,mqAuth,mqPass,cert);
	mqttClient.setWill("DIED", 2, "probably still some bugs", false);
	//  mqttClient.setKeepAlive(RECONNECT_DELAY_M *3); // very rarely need to change this (if ever)
	WiFi.begin("XXXXXXXX", "XXXXXXXX");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.printf("WIFI CONNECTED IP=%s\n", WiFi.localIP().toString().c_str());

	mqttClient.connect(MQTT_URL, mqAuth, mqPass);
}