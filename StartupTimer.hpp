

#ifndef STARTUPTIMER_HPP
#define STARTUPTIMER_HPP

unsigned long current = 0;



class StartupTimer {
private:
    unsigned long _lastMillis;
    unsigned long _passed;
    unsigned long _startupMin;
public:
    StartupTimer();
    StartupTimer& operator=(StartupTimer& startupTimer);
    void update();
    int uptimeMin();
    int uptimeDays();
};

StartupTimer::StartupTimer() : _lastMillis(0), _passed(0), _startupMin(0) {}

void StartupTimer::update() {
    unsigned long current = millis();
    if(current == _lastMillis) return;
    else if(current < _lastMillis) {
        _passed += 4294967295 - _lastMillis;
        _passed += current;
    } else {
        _passed += current - _lastMillis;
    }
    _lastMillis = current;

    if(_passed >= 60000) {
        unsigned long min = _passed / 60000;
        _startupMin += min;
        _passed = _passed % 60000;
    }
}

StartupTimer& StartupTimer::operator=(StartupTimer& startupTimer) {
    this->_lastMillis = startupTimer._lastMillis;
    this->_startupMin = startupTimer._startupMin;
    this->_passed = startupTimer._passed;
    return *this;
}


int StartupTimer::uptimeMin() {
    return _startupMin;
}
int StartupTimer::uptimeDays() {
    return _startupMin / 1440;

}







#endif
