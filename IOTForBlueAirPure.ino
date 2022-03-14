
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include<PubSubClient.h>

#include "DictionaryMap.hpp"
#include "StartupTimer.hpp"


#include "ESP8266ConfigurationWizard.hpp"

#define DEBUG

#define DEVICE_VER "0.9.0"

#define BUTTON_PIN 12
#define SPECAKER_PIN 5
#define OUT_PIN 13

#define MODE_OFF 0
#define MODE_SLOW 1
#define MODE_NORMAL 2
#define MODE_FAST 3

#define MODE_DELAY 500
#define BUTTON_DELAY 1000
#define BUTTON_SETUP_IN_DELAY 10000
#define OUT_DELAY 50

#define FILTER_RESET_DELAY 5500

#define RESPONSE_STATUS_DELAY 100

#define TOPIC_REQUEST_STATE  "/qstate"
#define TOPIC_STATE  "/state"
#define TOPIC_MODE  "/mode"


ESP8266ConfigurationWizard _ESP8266ConfigurationWizard;
PubSubClient* _mqtt;


uint8_t _mode = MODE_OFF;
uint8_t _targetMode = MODE_OFF;

unsigned long _lastModeChanged = 0;
unsigned long _lastButtonPushed = 0;
unsigned long _lastOutTime = 0;
unsigned long _lastResponseTime = 0;
unsigned long _startFilterResetTime = 0;


bool _isPushButton = false;
bool _isFilterResetMode = false;
bool _isChildLock = false;




String _topicRoot = "";
String _topicState;
String _topicRequestState;
String _topicMode;

StartupTimer _startupTimer;

void onStatusCallback(int status);
const char* onFilterOption(const char* name, const char* value);
void callbackSubscribe(char* topic, byte* payload, unsigned int length);
void publishState();

void playSoundAtSuccess();
void playSoundAtError();

void setup() {

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

   pinMode(BUTTON_PIN, INPUT);
   pinMode(OUT_PIN, OUTPUT);

   Config* config = _ESP8266ConfigurationWizard.getConfigPt();
   config->setAPName("BlueAir_Pure211");
   config->addOption("Device_key","", false);
   config->setTimeZone(9);
   _ESP8266ConfigurationWizard.setOnFilterOption(onFilterOption);
   _ESP8266ConfigurationWizard.setOnStatusCallback(onStatusCallback);
   _mqtt = _ESP8266ConfigurationWizard.pubSubClient();
   _ESP8266ConfigurationWizard.connect();

   if(!_ESP8266ConfigurationWizard.available()) {
      playSoundAtError();
   }
    
   
    
    
  #ifdef DEBUG 
    config->printConfig();
  #endif
  _topicRoot = config->getOption("Device_key");

  #ifdef DEBUG
    Serial.print("Device_key: ");
    Serial.println(_topicRoot);
  #endif
  
  _topicState = _topicRoot + TOPIC_STATE;
  _topicRequestState = _topicRoot + TOPIC_REQUEST_STATE;
  _topicMode = _topicRoot + TOPIC_MODE;

  
}

void playSoundAtSuccess() {
    tone(SPECAKER_PIN, 392, 250);
    delay(700);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 330, 250);
    delay(400);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 330, 250);
    delay(400);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 392, 250);
    delay(400);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 330, 250);
    delay(400);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 262, 250);
    delay(400);
    noTone(SPECAKER_PIN);
}

void playSoundAtError() {
    tone(SPECAKER_PIN, 523, 250);
    delay(500);
    noTone(SPECAKER_PIN);
    tone(SPECAKER_PIN, 262, 250);
    delay(500);
    noTone(SPECAKER_PIN);
}


void onStatusCallback(int status) {
  #ifdef DEBUG
    if(status == STATUS_CONFIGURATION) {
      Serial.println("\n\nStart configuration mode");
    }
    else if(status == WIFI_CONNECT_TRY) {
      Serial.println("Try to connect to Wifi.");
    }
    else if(status == WIFI_ERROR) {
      Serial.println("WIFI connection error.");
    }
    else if(status == WIFI_CONNECTED) {
      Serial.println("WIFI connected.");
    }
    else if(status == NTP_CONNECT_TRY) {
      Serial.println("Try to connect to NTP Server.");
    }
    else if(status == NTP_ERROR) {
      Serial.println("NTP Server connection error.");
    }
    else if(status == NTP_CONNECTED) {
      Serial.println("NTP Server connected.");
    }
    else if(status == MQTT_CONNECT_TRY) {
      Serial.println("Try to connect to MQTT Server.");
    }
    else if(status == MQTT_ERROR) {
      Serial.println("MQTT Server connection error.");
    }
  #endif
  
  if(status == MQTT_CONNECTED) {
    
    #ifdef DEBUG
      Serial.println("MQTT Server connected.");
    #endif
    _mqtt->setCallback(callbackSubscribe);
    _mqtt->subscribe((_topicRequestState).c_str());
    _mqtt->subscribe((_topicMode).c_str());
  }
  else if(status == STATUS_OK) {
    playSoundAtSuccess();
    #ifdef DEBUG  
      Serial.println("All connections are fine.");
    #endif
    publishState();
  }

}

const char* onFilterOption(const char* name, const char* value) {
  if(strcmp(name, "Device_key") == 0) {
      int valueLen = strlen(value);
      if(valueLen > 16) {
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
  #ifdef DEBUG
    Serial.print("Subscribe topic: ");
    Serial.println(topic);
  #endif

  
  unsigned long current = millis();
  if(String(topic).equals(_topicRequestState) && (current < _lastResponseTime || current - _lastResponseTime > RESPONSE_STATUS_DELAY) ) {
    _lastResponseTime = current;
    publishState();
  }
  else if(String(topic).endsWith(_topicMode) && length >= 5) {
    char* buffer = new char[length + 1];
    for(int i = 0; i < length; ++i) {
      buffer[i] = payload[i];
    }
    buffer[length] = '\0';
  
    #ifdef DEBUG
      Serial.print("Subscribe message: ");
      Serial.println(buffer);
    #endif
      
      DictionaryMap reqMap;
      reqMap.parseFromQueryString(buffer);
      char* cmd = reqMap.get("cmd");
      char* targetMode = reqMap.get("tm");
      char* childLock = reqMap.get("cl");
      
      #ifdef DEBUG
         Serial.print("cmd: ");
         Serial.println(String(cmd));
         Serial.print("targetMode: ");
         Serial.println(String(targetMode));
      #endif
      if(strcmp(cmd, "fr") == 0) {
        _isFilterResetMode = true;
      }
      else if(strcmp(cmd, "cl") == 0 && childLock != nullptr) {
        _isChildLock = strcmp(childLock, "1") == 0;
        publishState();
      }
      else if(targetMode != nullptr && cmd != nullptr && strcmp(cmd, "mode") == 0) {
        uint8_t oldTargetMode = _targetMode;
        _targetMode =  String(targetMode).toInt();
        if(_targetMode > MODE_FAST) {
          _targetMode = MODE_OFF;
        }
        if(oldTargetMode != _targetMode) {
          publishState();
        }
     }
     
   }
}

void checkButton() {
  if(_isChildLock) return;
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
      playSoundAtError();
      _ESP8266ConfigurationWizard.startConfigurationMode();
      
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
  if(_isFilterResetMode) {
    if(_startFilterResetTime == 0) {
      #ifdef DEBUG
      Serial.println("Start FilterReset");
      #endif
      publishState();
      _startFilterResetTime = current;
      digitalWrite(OUT_PIN, HIGH);
    }
    
    if(current < _startFilterResetTime || current - _startFilterResetTime >= FILTER_RESET_DELAY) {
      digitalWrite(OUT_PIN, LOW);
      _isFilterResetMode = false; 
      #ifdef DEBUG
      Serial.println("END FilterReset");
      #endif
      _startFilterResetTime = 0;
      publishState();
      
    }
    return;
  }
  else if(_mode != _targetMode && (current < _lastModeChanged || current - _lastModeChanged > MODE_DELAY) ) {
      
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
  message = String("cmd") + "=state&tm=" + _targetMode  + "&cm=" + _mode + "&cl=" + (_isChildLock ? 1 :  0) +  "&fr=" + (_isFilterResetMode ? 1 :  0) + "&v=" + DEVICE_VER + "&ip=" + WiFi.localIP().toString() + "&ut=" + _startupTimer.uptimeMin();
  #ifdef DEBUG
    Serial.print("publish message: ");
    Serial.println(message);
  #endif
    
  _mqtt->publish(_topicState.c_str(), message.c_str());
}

void loop(){
  _ESP8266ConfigurationWizard.loop();
  _startupTimer.update();
  nextMode();
  checkButton();
  if(_ESP8266ConfigurationWizard.isConfigurationMode()) return;
}
