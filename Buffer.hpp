//
// Created by beom on 2022-02-14.
//

#ifndef BUFFER_HPP
#define BUFFER_HPP

#define DEFAULT_CAP 64
#include <string.h>

class Buffer {
private:
    unsigned char* _buf;
    int _capacity;
    int _size;
    int _pos;
    bool _isReleased;
public:
    Buffer();
    ~Buffer();
    Buffer(int capacity);
    void write(char data);
    void write(const char* str);
    void write(unsigned char* str, int len);
    void reset();
    void beginPos();
    int read();
    bool available();
    int next(unsigned char* buffer,int bufferLen, unsigned char split);
    int size();
    int pos();
    void release();
    bool isReleased();
    int read(unsigned char* buf, int start, int len);
    unsigned char* buf();
private:
    void copy(unsigned char* oldbuf,unsigned char* newbuf, int size);
    void expand(int newSize);
};

int Buffer::size() {
    return _size;
}

int Buffer::pos() {
    return _pos;
}

unsigned char* Buffer::buf() {
    return _buf;
}

void Buffer::release() {
    if(_isReleased) return;
    delete[] _buf;
    _isReleased = true;
}

bool  Buffer::isReleased() {
    return _isReleased;
}

Buffer::Buffer() : _capacity(DEFAULT_CAP), _isReleased(false), _size(0), _pos(0) {
    _buf = new unsigned char[_capacity];
    reset();
};

Buffer::Buffer(int capacity) : _capacity(capacity), _isReleased(false), _size(0), _pos(0) {
    _buf = new unsigned char[_capacity];
    reset();
};

Buffer::~Buffer() {
    release();
}

void Buffer::reset() {
    for(int i = 0; i < _capacity; ++i) {
        _buf[i] = '\0';
    }
    _size = 0;
}
void Buffer::expand(int newSize) {
    int newCapacity = newSize > _capacity ? newSize :  _capacity + (_capacity / 3);
    if(newCapacity == _capacity) newCapacity = _capacity * 2;
    unsigned char* newBuf = new unsigned char[newCapacity];
    for(int i = 0; i < newCapacity; ++i) {
        newBuf[i] = '\0';
    }
    copy(_buf, newBuf, _size);
    delete[] _buf;
    _buf = newBuf;
}

void Buffer::write(char data) {
    if(_size == _capacity) {
        expand(-1);
    }
    _buf[_size] = data;
    ++_size;
}

void Buffer::beginPos() {
    _pos = 0;
}

void Buffer::write(const char* data) {
    int len = strlen(data);
    if(_size + len <= _capacity) {
        expand(_size + len + _capacity);
    }
    for(int i = 0; i < len; ++i) {
        _buf[_size] = data[i];
        ++_size;
    }
}

void Buffer::write(unsigned char* data,int len) {
    if(_size + len <= _capacity) {
        expand(_size + len + _capacity);
    }
    for(int i = 0; i < len; ++i) {
        _buf[_size] = data[i];
        ++_size;
    }
}

int Buffer::read() {
     if(_pos == _size) return -1;
     unsigned char data = _buf[_pos];
     ++_pos;
     return (int)data;
}

int Buffer::read(unsigned char* buf, int start, int len) {
    len = len > _size ? _size : len;
    for(int i = 0; i < len; ++i) {
        int data = read();
        if(data == -1) return i;
        buf[i + start] = data;
    }
    return len;
}

bool Buffer::available() {
    return _pos < _size;
}

int Buffer::next(unsigned char* buffer, int bufferLen,unsigned char split) {
    if(_pos == _size) return -1;
    for(int bufferPos = 0, p = _pos; p < _size && bufferPos < bufferLen ; ++p, ++bufferPos) {
        if(_buf[p] == split) {
            _pos = p + 1;
            return bufferPos;
        }
        buffer[bufferPos] = _buf[p];
    }
    return -1;
}


void Buffer::copy(unsigned char* oldbuf,unsigned char* newbuf, int size) {
    for(int i = 0; i < size; ++i) {
        newbuf[i] = oldbuf[i];
    }
}


#endif //TIMER_SCHEDULEJOB_HPP
