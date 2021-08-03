#include <H4AsyncMQTT.h>

H4 h4(115200);

H4AsyncMQTT mqttClient;

//H4_TIMER PT1;
//
// if using TLS / https, edit h4amcconfig.h and #define ASYNC_TCP_SSL_ENABLED 1
// do the same in async_config.h of the PATCHED ESPAsyncTCP library!! 

//#define MQTT_URL "https://192.168.1.21:8883"
#define MQTT_URL "http://192.168.1.20:1883"
const uint8_t* cert=nullptr;

// if you provide a valid certificate when connecting, it will be checked and fail on no match
// if you do not provide one, H4AsyncMQTT will continue insecurely with a warning
// this one is MY local mosquitto server... it ain't gonna work, so either don't use one, or set your own!!!
//const uint8_t cert[20] = { 0x9a, 0xf1, 0x39, 0x79,0x95,0x26,0x78,0x61,0xad,0x1d,0xb1,0xa5,0x97,0xba,0x65,0x8c,0x20,0x5a,0x9c,0xfa };
//#define MQTT_URL "https://robot.local:8883"

// If using MQTT server authentication, fill in next two fields!
const char* mqAuth="example";
const char* mqPass="pangolin";
//
//  Some sketches will require you to set START_WITH_CLEAN_SESSION to false
//  For THIS sketch, leave it at false
//
#define START_WITH_CLEAN_SESSION   true

#define BIG_SIZE 5000

const char* pload0="multi-line payload hex dumper which should split over several lines, with some left over";
const char* pload1="PAYLOAD QOS1";
const char* pload2="Save the Pangolin!";
char big[BIG_SIZE];

void onMqttError(int e,int i){
  if(e < H4AMC_ERROR_BASE){
    Serial.printf("H4ASYNC ERROR %d [%s] info=%d[0x%08x]\n",e,H4AsyncClient::errorstring(e).data(),i,i);
  }
  else {
    if(e < H4AMC_ERROR_MAX){
       Serial.printf("H4AsyncMQTT ERROR %d [%s] info=%d[0x%08x]\n",e,H4AsyncMQTT::errorstring(e).data(),i,i);   
    }
    else Serial.printf("UNKNOWN ERROR: %u extra info %d[0x%08x]\n",e,i,i);
  }
}

void onMqttConnect(bool session) {
  Serial.printf("USER: Connected as %s session=%d max payload size=%d\n",mqttClient.getClientId().data(),session,mqttClient.getMaxPayloadSize());

  Serial.println("USER: Subscribing at QoS 2");
//  mqttClient.subscribe({"test","multi2","fully/compliant"}, 0);
  mqttClient.subscribe({"test"}, 0);
//  Serial.printf("USER: T=%u Publishing at QoS 0\n",millis());
  mqttClient.publish("test",big,BIG_SIZE,0);
//  Serial.printf("USER: T=%u Publishing at QoS 1\n",millis());
//  mqttClient.publish("test",pload1,strlen(pload1),1); 
//  Serial.printf("USER: T=%u Publishing at QoS 2\n",millis());
//  mqttClient.publish("test",pload2,strlen(pload2),2);
/*
  h4.every(10000,[]{
    // simple way to publish int types  as strings using printf format
    mqttClient.publish("test",_HAL_freeHeap(),"%u"); 
    mqttClient.publish("test",-33); 
  });
*/
  //mqttClient.unsubscribe({"multi2","fully/compliant"});
}

void onMqttMessage(const char* topic, const uint8_t* payload, size_t len,uint8_t qos,bool retain,bool dup) {
  Serial.printf("\nUSER: H=%u Message %s qos%d dup=%d retain=%d len=%d\n",_HAL_freeHeap(),topic,qos,dup,retain,len);
  //dumphex(payload,len);
  //Serial.println();
}

void onMqttDisconnect(int8_t reason) {
  Serial.printf("USER: Disconnected from MQTT reason=%d\n",reason);
}

void h4setup(){
  for(auto i=0;i<BIG_SIZE;i++) big[i]=i;
  Serial.begin(115200);
  delay(250); //why???
  Serial.printf("\nH4AsyncMQTT v%s running @ debug level %d heap=%u\n",H4AMC_VERSION,H4AMC_DEBUG,_HAL_freeHeap()); 

  mqttClient.onMqttError(onMqttError);
  mqttClient.onMqttConnect(onMqttConnect);
  mqttClient.onMqttDisconnect(onMqttDisconnect);
  mqttClient.onMqttMessage(onMqttMessage);
  mqttClient.setServer(MQTT_URL,mqAuth,mqPass,cert);
/*    mqttClient.setWill("DIED",2,false,"probably still some bugs");
//  mqttClient.setKeepAlive(RECONNECT_DELAY_M *3); // very rarely need to change this (if ever)
*/
  WiFi.begin("XXXXXXXX","XXXXXXXX");
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  
  Serial.printf("WIFI CONNECTED IP=%s\n",WiFi.localIP().toString().c_str());

  mqttClient.connectMqtt("Pangolin_101",START_WITH_CLEAN_SESSION);
}