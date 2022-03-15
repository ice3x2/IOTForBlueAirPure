//
// Created by beom on 2022-02-21.
//

#ifndef DICTIONARY_H
#define DICTIONARY_H
#include <string.h>
#include "LinkedMap.hpp"
#include "Buffer.hpp"

bool _onMatchKey(char* keyA, char* keyB) {
    int keyALen = strlen(keyA);
    int keyBLen = strlen(keyB);
    if(keyALen != keyBLen) return false;
    for(int i = 0; i < keyALen; ++i) {
        if(keyA[i] != keyB[i]) {
            return false;
        }
    }
    return true;
}

void _onRemoveNode(char* key, char* value) {
    if(key != nullptr) delete[] key;
    if(value != nullptr) delete[] value;
}


class DictionaryMap {
private:
    LinkedMap<char*,char*>* _linkedMap;

public:
    DictionaryMap();
    ~DictionaryMap();
    void put(char* key, char* value);
    bool remove(char* key);
    bool contains(char* key);
    Enumeration<char*,char*>  enumeration();
    void clear();
    char* get(char* key);
    bool onMatch(char* keyA, char* keyB);
    void toQueryString(Buffer* buffer);
    bool parseFromQueryString(char* queryString);
private:
    char* decodeURL(char* str);
    char* encodeURL(char* str);
    char* readBuffer(Buffer* buffer);
    unsigned char h2int(char c);
};

DictionaryMap::DictionaryMap() {
    _linkedMap = new LinkedMap<char*,char*>(nullptr);
    _linkedMap->setOnMatchKey(_onMatchKey);
    _linkedMap->setOnRemoveNode(_onRemoveNode);
}

DictionaryMap::~DictionaryMap() {
    delete _linkedMap;
}

void DictionaryMap::clear() {
    if(_linkedMap->size() == 0) {
        return;
    }
    delete _linkedMap;
    _linkedMap = new LinkedMap<char*,char*>(nullptr);
    _linkedMap->setOnMatchKey(_onMatchKey);
    _linkedMap->setOnRemoveNode(_onRemoveNode);
}



void DictionaryMap::put(char* key, char* value) {
    remove(key);
    int keyLen = strlen(key);
    int valueLen = value == nullptr || value == NULL ? 0 : strlen(value);
    char* dKey = new char [keyLen + 1];
    char* dValue = new char [valueLen + 1];
    strcpy(dKey,(const char*)key);
    if(valueLen > 0) {
        strcpy(dValue, (const char *) value);
    } else {
        dValue[0] = '\0';
    }
    _linkedMap->put(dKey, dValue);
}

bool DictionaryMap::remove(char* key) {
    if(_linkedMap->remove(key)) {
        return true;
    }
    return false;
}



bool DictionaryMap::contains(char* key) {
    return _linkedMap->contains(key);
}
Enumeration<char*,char*> DictionaryMap::enumeration() {
    return _linkedMap->enumeration();
}

char* DictionaryMap::get(char* key) {
    return _linkedMap->get(key);
}

bool DictionaryMap::parseFromQueryString(char *queryString) {
    clear();
    Buffer tmpBuffer;
    char* key = nullptr;
    char* value = nullptr;
    bool readKey = true;
    int len = strlen(queryString);
    for(int i = 0; i < len; ++i) {
        char c = queryString[i];

        if(readKey) {
            if(c == '=') {
                tmpBuffer.beginPos();
                key = readBuffer(&tmpBuffer);
                tmpBuffer.reset();
                readKey = false;
            }
            else if(c == '&') {
                if(key != nullptr) delete[] key;
                if(value != nullptr) delete[] value;
                return false;
            }
            else {
                tmpBuffer.write(c);
            }
        } else {
            if(c == '&') {
                value = readBuffer(&tmpBuffer);
                char* decodedKey = decodeURL(key);
                char* decodedValue = decodeURL(value);
                _linkedMap->put(decodedKey, decodedValue);
                delete[] key;
                delete[] value;
                key = nullptr;
                value = nullptr;
                tmpBuffer.reset();
                readKey = true;
            }
            else if(c == '=') {
                if(key != nullptr) delete[] key;
                if(value != nullptr) delete[] value;
                return false;
            }
            else {
                tmpBuffer.write(c);
            }
        }

    }

    if(!readKey && key != nullptr) {
        value = readBuffer(&tmpBuffer);

        char* decodedKey = decodeURL(key);
        char* decodedValue = decodeURL(value);
        _linkedMap->put(decodedKey, decodedValue);

    }
    if(key != nullptr) delete[] key;
    if(value != nullptr) delete[] value;



    return true;
}

void DictionaryMap::toQueryString(Buffer* buffer) {
    Enumeration<char*,char*> enums = enumeration();
    while(enums.hasMoreNodes()) {
        Node<char*,char*> node = enums.nextNode();
        char* key = node.getKey();
        char* value = node.getValue();
        key = encodeURL(key);
        value = encodeURL(value);
        buffer->write(key);
        buffer->write("=");
        buffer->write(value);

        delete[] key;
        delete[] value;

        if(enums.hasMoreNodes()) {
            buffer->write("&");
        }
    }
    //char* result = readBuffer(&buffer);
    //return result;

}

char* DictionaryMap::readBuffer(Buffer* buffer) {
    int size = buffer->size();
    char* result = new char[size + 1];
    buffer->beginPos();
    buffer->read((unsigned  char*)result, 0, size);
    result[size] = '\0';
    return result;
}


char* DictionaryMap::encodeURL(char* str)
{
    if(str == NULL || str == nullptr) return new char[1]{'\0'};
    Buffer buffer;
    int len = strlen(str);
    char c;
    char code0;
    char code1;
    for (int i =0; i < len; i++){
        c= str[i];
        if (c == ' '){
            buffer.write("%20");
        } else if ( ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') ){
            buffer.write(c);
        } else{
            code1=(c & 0xf)+'0';
            if ((c & 0xf) >9){
                code1=(c & 0xf) - 10 + 'A';
            }
            c=(c>>4)&0xf;
            code0=c+'0';
            if (c > 9){
                code0=c - 10 + 'A';
            }
            buffer.write('%');
            buffer.write(code0);
            buffer.write(code1);
        }
    }
    char* result = readBuffer(&buffer);
    return result;

}


char* DictionaryMap::decodeURL(char* str)
{
    if(str == NULL || str == nullptr) return new char[1]{'\0'};
    Buffer buffer;
    int len = strlen(str);
    char c;
    char code0;
    char code1;
    for (int i =0; i < len; i++){
        c= str[i];
        if (c == '+'){
            buffer.write(' ');
        }else if (c == '%') {
            i++;
            code0=str[i];
            i++;
            code1=str[i];
            c = (h2int(code0) << 4) | h2int(code1);
            buffer.write(c);
        } else{
            buffer.write(c);
        }

    }
    char* result = readBuffer(&buffer);
    return result;
}



unsigned char DictionaryMap::h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}



#endif
