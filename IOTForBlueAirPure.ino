
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include<PubSubClient.h>

#include "DictionaryMap.hpp"
#include "StartupTimer.hpp"

#define HEAVY_BUILD
//#define LIGHT_BUILD

#ifdef HEAVY_BUILD
#include "ESP8266ConfigurationWizard.hpp"
#endif

#define VER 1

#define BUTTON_PIN 12
#define OUT_PIN 13

#define MODE_OFF 0
#define MODE_SLOW 1
#define MODE_NORMAL 2
#define MODE_FAST 3

#define MODE_DELAY 500
#define BUTTON_DELAY 500
#define BUTTON_SETUP_IN_DELAY 30000
#define OUT_DELAY 50

#define RESPONSE_STATUS_DELAY 100

#define TOPIC_REQUEST_STATE  "/qstate"
#define TOPIC_STATE  "/state"
#define TOPIC_MODE  "/mode"



/////////////////////////////////////////////////
#ifdef LIGHT_BUILD

const char* ssid = "...";
const char* password = "...";
const char* mqtt_server = "broker.hivemq.com";
String _clientID = "client12354";

WiFiClient espClient;
PubSubClient client(espClient);
PubSubClient* _mqtt = &client;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);

  while (!client.connected()) {
    client.connect(_clientID.c_str());
    delay(1000);
    Serial.print(".");
  }
  Serial.println("connected");
}

#endif
///////////////////////////////////////////////

#ifdef HEAVY_BUILD
WiFiUDP _udp;
WiFiClient _wifiClient; 
ESP8266ConfigurationWizard _ESP8266ConfigurationWizard(_udp, _wifiClient);
PubSubClient* _mqtt;
#endif


uint8_t _mode = MODE_OFF;
uint8_t _targetMode = MODE_OFF;

unsigned long _lastModeChanged = 0;
unsigned long _lastButtonPushed = 0;
unsigned long _lastOutTime = 0;
unsigned long _lastResponseTime = 0;

bool _isPushButton = false;


String _topicRoot = "ibap";

String _topicState;
String _topicRequestState;
String _topicMode;

StartupTimer _startupTimer;

void onStatusCallback(int status);
const char* onFilterOption(const char* name, const char* value);
void callbackSubscribe(char* topic, byte* payload, unsigned int length);
void publishState();



void setup() {

  Serial.begin(115200);

  #ifdef LIGHT_BUILD
   setup_wifi();
  #endif

  #ifdef HEAVY_BUILD
  ESP.wdtDisable();
   Config* config = _ESP8266ConfigurationWizard.getConfigPt();
   config->setAPName("BlueAir_Pure211");
   config->addOption("Device_key","", false);
   //config->setTimeZone(9);
   _ESP8266ConfigurationWizard.setOnFilterOption(onFilterOption);
   _ESP8266ConfigurationWizard.setOnStatusCallback(onStatusCallback);
   _ESP8266ConfigurationWizard.connect();
   _mqtt = _ESP8266ConfigurationWizard.pubSubClient();
      //config->addOption("Device key","", false);

  #endif

  pinMode(BUTTON_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);


  Serial.println(_topicRequestState.c_str());
  Serial.println(_topicMode.c_str());

  _topicState = _topicRoot + TOPIC_STATE;
  _topicRequestState = _topicRoot + TOPIC_REQUEST_STATE;
  _topicMode = _topicRoot + TOPIC_STATE;

  _mqtt->setCallback(callbackSubscribe);
  _mqtt->subscribe((_topicRequestState).c_str());
  _mqtt->subscribe((_topicMode).c_str());


  if(_mqtt->connected()) {
     publishState();
  }
  
}


void onStatusCallback(int status) {
  if(status == MQTT_CONNECTED) {
    Serial.println("MQTT Server connected.");
    publishState();
  }
  

}

const char* onFilterOption(const char* name, const char* value) {
  if(strcmp(name, "Device_key") == 0) {
      int valueLen = strlen(value);
      if(valueLen > 32) {
        return "Please enter 16 characters or less.";
      }      
      for(int i = 0; i < valueLen; ++i) {
        if( !(('0' <= value[i] && value[i] <= '9') || 
              ('A' <= value[i] && value[i] <= 'Z') || 
              ('a' <= value[i] && value[i] <= 'z') ) ) 
        {
          return "Only numbers or English alphabets can be entered.";
        }
      }
  }
  return "";
}


void callbackSubscribe(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  if(String(topic).equals(_topicRequestState) && (current < _lastResponseTime || current - _lastResponseTime > RESPONSE_STATUS_DELAY) ) {
    _lastResponseTime = current;
    publishState();
  }
  else if(String(topic).endsWith(_topicMode) && length > 10) {
    char* buffer = new char[length + 1];
    for(int i = 0; i < length; ++i) {
      buffer[i] = payload[i];
    }
    buffer[length] = '\0';
    Serial.println(length);
    Serial.println(buffer);
    DictionaryMap reqMap;
    reqMap.parseFromQueryString(buffer);
    char* cmd = reqMap.get("cmd");
    char* targetMode = reqMap.get("targetMode");
    Serial.print("cmd: ");
    Serial.println(String(cmd));
    Serial.print("targetMode: ");
    Serial.println(String(targetMode));
      

    
    if(targetMode != nullptr && cmd != nullptr && strcmp(cmd, "mode") == 0) {
      _targetMode =  String(targetMode).toInt();
      Serial.print("targetMode: ");
      Serial.println(String(_targetMode));
      if(_targetMode > MODE_FAST) {
        _targetMode = MODE_OFF;
      }
      publishState();
    }
    delete[] buffer;
  }
}

void checkButton() {
  unsigned long current = millis();
  int value = digitalRead(BUTTON_PIN);
  // 버튼 눌렀을 때
  if(value == LOW && !_isPushButton) {
      _isPushButton = true;
      _lastButtonPushed = current;
  }
  // 버튼에서 손을 뗐을 때. 
  else if(value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_SETUP_IN_DELAY)) {
      _lastButtonPushed = current;
      _isPushButton = false;
      // 설정모드 진입.
      #ifdef HEAVY_BUILD
      _ESP8266ConfigurationWizard.startConfigurationMode();
      #endif
  }
  else if(value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_DELAY)) {
      _lastButtonPushed = current;
      _isPushButton = false;
      _targetMode = _mode;
      ++_targetMode;
      if(_targetMode > MODE_FAST) {
        _targetMode = MODE_OFF;
      }      
      publishState();
  }
}

void nextMode() {
  unsigned long current = millis();
  if(_mode != _targetMode && (current < _lastModeChanged || current - _lastModeChanged > MODE_DELAY) ) {
      
      _lastModeChanged = current;
      ++_mode;
      digitalWrite(OUT_PIN, HIGH);
      _lastOutTime = current;
      if(_mode > MODE_FAST) {
         _mode = MODE_OFF;
      }
      publishState();
  }
  current = millis();
  if(current < _lastOutTime || current - _lastOutTime > OUT_DELAY) {
    digitalWrite(OUT_PIN, LOW);
  }
}

void publishState() {
  if(!_mqtt->connected()) return;
  String message;
  message = String("cmd") + "=state&targetMode=" + _targetMode +"&currentMode=" + _mode + "&ver=" + VER + "&addr=" + WiFi.localIP().toString() + "&startup=" + _startupTimer.startupMin();
  Serial.println(message);
  _mqtt->publish(_topicState.c_str(), message.c_str());
}

void loop(){
  #ifdef LIGHT_BUILD 
    _mqtt->loop();
  #endif
  #ifdef HEAVY_BUILD
  _ESP8266ConfigurationWizard.loop();
  if(_ESP8266ConfigurationWizard.isConfigurationMode()) return;
  #endif
  
  _startupTimer.update();
  nextMode();
  checkButton();
}
