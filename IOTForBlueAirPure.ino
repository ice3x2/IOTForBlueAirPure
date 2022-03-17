
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

// 각 모드 전환 딜레이. 예를 들어 MODE_SLOW 에서 MODE_NORMAL 로 바뀌는데 500ms 딜레이 발생.
#define MODE_DELAY 500
// 버튼 동작 딜레이. 지정된 시간(1초) 이전에 버튼을 누를 경우 동작되지 않음.
#define BUTTON_DELAY 1000
// 설정된 시간(MS) 이상으로 버튼을 누르고 있으면 셋업 모드로 들어간다.
#define BUTTON_PRESS_TIME_TO_SETUP 10000
// 지정된 시간(MS)만큼 OUT_PIN 에서 HIGH 신호 발생
#define OUTPUT_DELAY 50
// 기본 5.5초 이상으로 OUT_PIN 에서 HIGH 신호를 발생시켜 필터 초기화를 진행한다.
#define FILTER_RESET_DELAY 5500
// topic /qstate 이 지정된 시간 안에 연속으로 발생한다면 무시한다.
#define QSTATE_DELAY 100

#define HEARTBEAT_INTERVAL 5000

// 상태값 callback
#define CALLBACK_PUBLISH_TOPIC_STATE  "/qstate"
// 모드 callback
#define CALLBACK_TOPIC_MODE  "/mode"

// 상태값 전달
#define PUBLISH_TOPIC_STATE  "/state"
#define PUBLISH_TOPIC_HEARTBEAT  "/heartbeat"


ESP8266ConfigurationWizard _ESP8266ConfigurationWizard;
PubSubClient* _mqtt;

// 현재 상태
uint8_t _mode = MODE_OFF;
// 목표상태
uint8_t _targetMode = MODE_OFF;
// 현재 상태가 마지막으로 변경된 시간
unsigned long _lastModeChanged = 0;
// 버튼이 마지막으로 눌린 시간
unsigned long _lastButtonPushed = 0;
// 마지막으로 모드 변경을 위해서 OUT_PIN 을 통한 HIGH 가 발생된 시간
unsigned long _lastOutTime = 0;
// 마지막으로 /qstate 요청이 들어온 시간
unsigned long _lastRequestStateTime = 0;
// 마지막으로 필터 리셋을 위해서 OUT_PIN 을 통한 HIGH 가 발생된 시간
unsigned long _startFilterResetTime = 0;
// 마지막으로 heartbeat 가 전송된 시간
unsigned long _lastHeartbeat = 0;

bool _isPushButton = false;
bool _isFilterResetMode = false;
bool _isChildLock = false;




String _topicRoot = "";
String _topicState;
String _topicRequestState;
String _topicMode;
String _topicHeartbeat;

StartupTimer _startupTimer;
void initMqtt();
void onStatusCallback(int status);
const char* onFilterOption(const char* name, const char* value);
void callbackSubscribe(char* topic, byte* payload, unsigned int length);
void publishState();
void publishHeartbeat();

void playSuccessBeep();
void playAlertBeep();

void setup() {

#ifdef DEBUG
  Serial.begin(115200);
#endif

  pinMode(BUTTON_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);

  Config* config = _ESP8266ConfigurationWizard.getConfigPt();
  config->setAPName("BlueAir_Pure211");
  config->addOption("Device_key", "", false);
  config->setTimeZone(9);
  _ESP8266ConfigurationWizard.setOnFilterOption(onFilterOption);
  _ESP8266ConfigurationWizard.setOnStatusCallback(onStatusCallback);
  _mqtt = _ESP8266ConfigurationWizard.pubSubClient();




#ifdef DEBUG
  Serial.print("Device_key: ");
  Serial.println(_topicRoot);
#endif

  _ESP8266ConfigurationWizard.connect();

    #ifdef DEBUG
  config->printConfig();
  #endif

  if (!_ESP8266ConfigurationWizard.available()) {
    playAlertBeep();
  }
  


}

void playSuccessBeep() {
  /* 산토끼...
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
  noTone(SPECAKER_PIN);*/
  tone(SPECAKER_PIN, 523, 100);
  delay(100);
  tone(SPECAKER_PIN, 623, 100);
  delay(100);
  tone(SPECAKER_PIN, 823, 250);
  delay(250);
  noTone(SPECAKER_PIN);

  
}

void playAlertBeep() {
  /*tone(SPECAKER_PIN, 523, 250);
  delay(500);
  noTone(SPECAKER_PIN);
  tone(SPECAKER_PIN, 262, 250);
  delay(500);
  noTone(SPECAKER_PIN);*/
  tone(SPECAKER_PIN, 392, 250);
  delay(1000);
  noTone(SPECAKER_PIN);
  tone(SPECAKER_PIN, 392, 250);
  delay(1000);
  noTone(SPECAKER_PIN);
  tone(SPECAKER_PIN, 392, 250);
  delay(1000);
  noTone(SPECAKER_PIN);
  
}


void onStatusCallback(int status) {
#ifdef DEBUG
  if (status == STATUS_CONFIGURATION) {
    Serial.println("\n\nStart configuration mode");
  }
  else if (status == WIFI_CONNECT_TRY) {
    Serial.println("Try to connect to Wifi.");
  }
  else if (status == WIFI_ERROR) {
    Serial.println("WIFI connection error.");
  }
  else if (status == WIFI_CONNECTED) {
    Serial.println("WIFI connected.");
  }
  else if (status == NTP_CONNECT_TRY) {
    Serial.println("Try to connect to NTP Server.");
  }
  else if (status == NTP_ERROR) {
    Serial.println("NTP Server connection error.");
  }
  else if (status == NTP_CONNECTED) {
    Serial.println("NTP Server connected.");
  }
  else if (status == MQTT_CONNECT_TRY) {
    Serial.println("Try to connect to MQTT Server.");
  }
  else if (status == MQTT_ERROR) {
    Serial.println("MQTT Server connection error.");
  }
#endif

  if (status == MQTT_CONNECTED) {

      #ifdef DEBUG
       Serial.println("MQTT Server connected.");
      #endif
      initMqtt();
    
    }
  else if (status == STATUS_OK) {
    playSuccessBeep();
#ifdef DEBUG
    Serial.println("All connections are fine.");
#endif
    publishState();
  }
}

void initMqtt() {
   Config* config = _ESP8266ConfigurationWizard.getConfigPt();
  _topicRoot = config->getOption("Device_key");
  _topicState = _topicRoot + PUBLISH_TOPIC_STATE;
  _topicRequestState = _topicRoot + CALLBACK_PUBLISH_TOPIC_STATE;
  _topicMode = _topicRoot + CALLBACK_TOPIC_MODE;
  _topicHeartbeat = _topicRoot + PUBLISH_TOPIC_HEARTBEAT;
  _mqtt->setCallback(callbackSubscribe);
  _mqtt->subscribe((_topicRequestState).c_str());
  _mqtt->subscribe((_topicMode).c_str());
  
}


const char* onFilterOption(const char* name, const char* value) {
  if (strcmp(name, "Device_key") == 0) {
    int valueLen = strlen(value);
    if (valueLen > 16) {
      return "Please enter 16 characters or less.";
    }
    for (int i = 0; i < valueLen; ++i) {
      if ( !(('0' <= value[i] && value[i] <= '9') ||
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
  if (String(topic).equals(_topicRequestState) && (current < _lastRequestStateTime || current - _lastRequestStateTime > QSTATE_DELAY) ) {
    _lastRequestStateTime = current;
    publishState();
  }
  else if (String(topic).endsWith(_topicMode) && length >= 5) {
    char* buffer = new char[length + 1];
    for (int i = 0; i < length; ++i) {
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
    if (strcmp(cmd, "fr") == 0) {
      _isFilterResetMode = true;
    }
    else if (strcmp(cmd, "cl") == 0 && childLock != nullptr) {
      _isChildLock = strcmp(childLock, "1") == 0;
      publishState();
    }
    else if (targetMode != nullptr && cmd != nullptr && strcmp(cmd, "mode") == 0) {
      uint8_t oldTargetMode = _targetMode;
      _targetMode =  String(targetMode).toInt();
      if (_targetMode > MODE_FAST) {
        _targetMode = MODE_OFF;
      }
      if (oldTargetMode != _targetMode) {
        publishState();
      }
    }
    delete[] buffer;

  }
}

void checkButton() {
  if (_isChildLock) return;
  unsigned long current = millis();
  int value = digitalRead(BUTTON_PIN);
  // 버튼 눌렀을 때
  if (value == LOW && !_isPushButton) {
    _isPushButton = true;
    _lastButtonPushed = current;
  }
  // 버튼에서 손을 뗐을 때.
  else if (value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_PRESS_TIME_TO_SETUP)) {
    _lastButtonPushed = current;
    _isPushButton = false;
    // 설정모드 진입.
    playAlertBeep();
    _ESP8266ConfigurationWizard.startConfigurationMode();

  }
  else if (value == HIGH && _isPushButton && (current < _lastButtonPushed || current - _lastButtonPushed > BUTTON_DELAY)) {
    _lastButtonPushed = current;
    _isPushButton = false;
    _targetMode = _mode;
    ++_targetMode;
    if (_targetMode > MODE_FAST) {
      _targetMode = MODE_OFF;
    }
    publishState();
  }
}

void nextMode() {

  unsigned long current = millis();
  if (_isFilterResetMode) {
    if (_startFilterResetTime == 0) {
#ifdef DEBUG
      Serial.println("Start FilterReset");
#endif
      publishState();
      _startFilterResetTime = current;
      digitalWrite(OUT_PIN, HIGH);
    }

    if (current < _startFilterResetTime || current - _startFilterResetTime >= FILTER_RESET_DELAY) {
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
  else if (_mode != _targetMode && (current < _lastModeChanged || current - _lastModeChanged > MODE_DELAY) ) {

    _lastModeChanged = current;
    ++_mode;
    digitalWrite(OUT_PIN, HIGH);
    _lastOutTime = current;
    if (_mode > MODE_FAST) {
      _mode = MODE_OFF;
    }
    publishState();
  }

  current = millis();
  if (current < _lastOutTime || current - _lastOutTime > OUTPUT_DELAY) {
    digitalWrite(OUT_PIN, LOW);
  }
}

void publishHeartbeat() {
  unsigned long current = millis();
  if(current - _lastHeartbeat < HEARTBEAT_INTERVAL) {
     return;
  }
  if (!_mqtt->connected()) return;
  _lastHeartbeat = current;
  String message("cmd=hb&t=");
  message += _ESP8266ConfigurationWizard.getEpochTime();
  _mqtt->publish(_topicHeartbeat.c_str(), message.c_str());
  #ifdef DEBUG
  Serial.println(ESP.getFreeHeap());
  #endif
}

void publishState() {
  if (!_mqtt->connected()) return;
  String message;
  message = String("cmd") + "=state&tm=" + _targetMode  + "&cm=" + _mode + "&cl=" + (_isChildLock ? 1 :  0) +  "&fr=" + (_isFilterResetMode ? 1 :  0) + "&v=" + DEVICE_VER + "&ip=" + WiFi.localIP().toString() + "&ut=" + _startupTimer.uptimeMin();
#ifdef DEBUG
  Serial.print(_topicState);
  Serial.print("  publish message: ");
  Serial.println(message);
#endif

  _mqtt->publish(_topicState.c_str(), message.c_str());
}

void loop() {
  _ESP8266ConfigurationWizard.loop();
  _startupTimer.update();
  nextMode();
  checkButton();
  if (_ESP8266ConfigurationWizard.isConfigurationMode()) return;
  publishHeartbeat();
}
