
#include <ESP8266WiFi.h>
#include<PubSubClient.h>

#include "DictionaryMap.hpp"

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

#define TOPIC_REQUEST_STATE  "/qstate"
#define TOPIC_STATE  "/state"
#define TOPIC_MODE  "/mode"

/////////////////////////////////////////////////
const char* ssid = "...";
const char* password = "...";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
PubSubClient* _mqtt = &client;

///////////////////////////////////////////////

uint8_t _mode = MODE_OFF;
uint8_t _targetMode = MODE_OFF;

unsigned long _lastModeChanged = 0;
unsigned long _lastButtonPushed = 0;
unsigned long _lastOutTime = 0;

bool _isPushButton = false;

String _clientID = "client12354";
String _topicRoot = "ibap";
String _topicState;
String _topicRequestState;
String _topicMode;

void callbackSubscribe(char* topic, byte* payload, unsigned int length);
void publishState();

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
}


void setup() {

  Serial.begin(115200);


  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callbackSubscribe);

  while (!client.connected()) {
    client.connect(_clientID.c_str());
    delay(1000);
    Serial.print(".");
  }
  Serial.println("connected");
  

  ////////////

  _topicRequestState = _topicRoot + TOPIC_REQUEST_STATE;
  _topicMode = _topicRoot + TOPIC_MODE;
  _topicState = _topicRoot + TOPIC_STATE;

  pinMode(BUTTON_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);


  Serial.println(_topicRequestState.c_str());
  Serial.println(_topicMode.c_str());

  _mqtt->setCallback(callbackSubscribe);
  _mqtt->subscribe((_topicRequestState).c_str());
  _mqtt->subscribe((_topicMode).c_str());
  
}


void callbackSubscribe(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  if(String(topic).equals(_topicRequestState)) {
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
  if(value == LOW && !_isPushButton) {
      _isPushButton = true;
      _lastButtonPushed = current;
  }
  else if(value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_SETUP_IN_DELAY)) {
      _lastButtonPushed = current;
  }
  else if(value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_DELAY)) {
      _lastButtonPushed = current;
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
  String message;
  message = String("cmd") + "=state&targetMode=" + _targetMode +"&currentMode=" + _mode + "&ver=" + VER;
  Serial.println(message);
  _mqtt->publish(_topicState.c_str(), message.c_str());
}

void loop(){
  nextMode();
  checkButton();
  _mqtt->loop();
  

  
}
