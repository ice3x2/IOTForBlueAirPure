//
// Created by beom on 2022-02-21.
//

#ifndef LINKEDMAP_H
#define LINKEDMAP_H



template<typename K, typename V>
class Node {
private:
    Node<K,V>* _next;
    Node<K,V>* _prev;
    K _key;
    V _value;

public:
    Node() {}
    Node(K key, V value) {
        _key = key;
        _value = value;
        _next = nullptr;
        _prev = nullptr;
    }
    inline void setNext(Node* next) {
        _next = next;
    }
    inline Node* getNext() {
        return _next;
    }
    inline void setPrev(Node* prev) {
        _prev = prev;
    }
    inline  Node* getPrev() {
        return _prev;
    }
    inline const K& getKey() {
        return _key;
    }
    inline const K& getValue() {
        return _value;
    }
    Node operator=(Node &ref);
    bool isEmptyNode(Node<K,V> node);
};

template<typename K, typename V>
class Enumeration {
private:
    Node<K,V>* _current;
    Node<K,V>* _head;
    Node<K,V>* _end;
    Node<K,V> _emptyNode;
public:
    Enumeration( Node<K,V>* head, Node<K,V>* end);
    Node<K,V> nextNode();
    bool hasMoreNodes();
    Enumeration operator=(Enumeration &ref);

};


template<typename K, typename V>
class LinkedMap {
public:
    typedef bool (*OnMatchKey)(K keyA, K keyB);
    typedef void (*OnRemoveNode)(K key, V value);
    Node<K,V>* _current;
    Node<K,V>* _head;
    Node<K,V>* _end;
    int _size;
    V _emptyValue;
    OnMatchKey _onMatchKey;
    OnRemoveNode _onRemoveNode;
public:
    LinkedMap();
    LinkedMap(V emptyValue);
    ~LinkedMap();
    void setOnMatchKey(OnMatchKey);
    void setOnRemoveNode(OnRemoveNode);
    void put(K key, V value);
    bool remove(K key);
    bool contains(K key);
    void moveNextNode();
    bool isEmpty();
    int size();
    Enumeration<K, V>  enumeration();
    V get(K key);
    V getEmptyValue();


private:
    bool onMatchKeyDef(K keyA, K keyB);
    void onRemoveNode(K key, V value);
    void init();
};

template<typename K, typename V>
Node<K,V> Node<K,V>::operator=(Node<K, V> &ref) {
    this->_key = ref._key;
    this->_value = ref._value;
    this->_prev = ref._prev;
    this->_next = ref._next;
    return *this;
}

template<typename K, typename V>
bool Node<K,V>::isEmptyNode(Node<K, V> node) {
    return node.getNext() == nullptr && node.getPrev() == nullptr;
}

template<typename K, typename V>
Enumeration<K,V>::Enumeration(Node<K,V>* head,Node<K,V>* end) {
    _head = head;
    _current = head;
    _end = end;
}





template<typename K, typename V>
Node<K,V> Enumeration<K,V>::nextNode() {
    if(_current == _end) return _emptyNode;
    _current = _current->getNext();
    return *_current;
}

template<typename K, typename V>
bool Enumeration<K,V>::hasMoreNodes() {
    return _current->getNext() != _end;
}

template<typename K, typename V>
Enumeration<K,V> Enumeration<K,V>::operator=(Enumeration<K, V> &ref) {
    _current = ref._current;
    _head = ref._head;
    _end = ref._end;
    return *this;
}

template<typename K, typename V>
LinkedMap<K,V>::LinkedMap(V emptyValue)  {
    _current = _head = new Node<K,V>();
    _end = new Node<K,V>();
    _emptyValue = emptyValue;
    init();
}


template<typename K, typename V>
LinkedMap<K,V>::LinkedMap()  {
    _current = _head = new Node<K,V>();
    _end = new Node<K,V>();
    init();
}


template<typename K, typename V>
void LinkedMap<K, V>::init() {
    _onMatchKey = nullptr;
    _head->setNext(_end);
    _head->setPrev(nullptr);
    _end->setPrev(_head);
    _end->setNext(nullptr);
    _size = 0;
}

template<typename K, typename V>
LinkedMap<K,V>::~LinkedMap() {
    Node<K,V>* currentNode = (Node<K,V>*)_head->getNext();
    do {
        Node<K,V>* node = currentNode;
        Node<K,V>* nextNode = node->getNext();;
        Node<K,V>* endNode = _end;
        if(node != endNode) {
            onRemoveNode(currentNode->getKey(), currentNode->getValue());
            delete currentNode;
        }
        currentNode = nextNode;

    } while(currentNode != _end && currentNode != nullptr);
    delete _head;
    delete _end;
}


template<typename K, typename V>
void LinkedMap<K,V>::setOnRemoveNode(OnRemoveNode onRemoveNode) {
    _onRemoveNode = onRemoveNode;

}

template<typename K, typename V>
void LinkedMap<K,V>::setOnMatchKey(OnMatchKey onMatchKey) {
    _onMatchKey = onMatchKey;

}

template<typename K, typename V>
bool LinkedMap<K, V>::isEmpty() {
    return _size == 0;
}

template<typename K, typename V>
int LinkedMap<K, V>::size() {
    return _size;
}

template<typename K, typename V>
void LinkedMap<K, V>::put(K key, V value) {
    remove(key);
    Node<K,V>* lastNode = _end->getPrev();
    Node<K,V>* newNode = new Node<K, V>(key, value);
    lastNode->setNext(newNode);
    newNode->setPrev(lastNode);
    newNode->setNext(_end);
    _end->setPrev(newNode);
    _size++;
}


template<typename K, typename V>
bool LinkedMap<K, V>::remove(K key) {
    Node<K,V>* currentNode = _head;
    while(currentNode != _end) {
        if(currentNode == _head) {
            currentNode = currentNode->getNext();
            continue;
        }
        K k = currentNode->getKey();
        if(onMatchKeyDef(key, k)) {
            Node<K,V>* prevNode = currentNode->getPrev();
            Node<K,V>* nextNode = currentNode->getNext();
            prevNode->setNext(nextNode);
            nextNode->setPrev(prevNode);
            if(currentNode == _current) {
                _current = _current->getPrev();
            }
            onRemoveNode(currentNode->getKey(), currentNode->getValue());
            delete currentNode;
            --_size;
            return true;
        }
        currentNode = currentNode->getNext();
    }
    return false;
}
template<typename K, typename V>
Enumeration<K, V>  LinkedMap<K, V>::enumeration() {
    return Enumeration<K, V>(_head, _end);
}


template<typename K, typename V>
V LinkedMap<K, V>::get(K key) {
    if(_current == _head) {
        moveNextNode();
    }
    if(_current == _head) {
        return _emptyValue;
    }
    Node<K,V>* endNode = _current;
    do {
        K k = _current->getKey();
        V v = _current->getValue();
        moveNextNode();
        if(onMatchKeyDef(key, k)) {
            return v;
        }

    } while(_current != endNode);
    return _emptyValue;
}

template<typename K, typename V>
void LinkedMap<K, V>::moveNextNode() {
    _current = _current->getNext();
    if(_current == _end) {
        _current = _head->getNext();
    }
    if(_current == _end) {
        _current = _head;
    }
}

template<typename K, typename V>
V LinkedMap<K, V>::getEmptyValue() {
    return _emptyValue;
}

template<typename K, typename V>
bool LinkedMap<K, V>::contains(K key) {
    Node<K,V>* currentNode = _head;
    while(currentNode != _end) {
        if(currentNode == _head) {
            currentNode = currentNode->getNext();
            continue;
        }
        K k = currentNode->getKey();
        if(onMatchKeyDef(key, k)) {
            return true;
        }
        currentNode = currentNode->getNext();
    }
    return false;
}


template<typename K, typename V>
void LinkedMap<K, V>::onRemoveNode(K key, V value) {
    if(_onRemoveNode != nullptr) _onRemoveNode(key, value);
}

template<typename K, typename V>
bool LinkedMap<K, V>::onMatchKeyDef(K keyA, K keyB) {
    if(keyA == nullptr || keyB == nullptr) return false;
    if(_onMatchKey != nullptr) return _onMatchKey(keyA, keyB);
    return keyA == keyB;
}



#endif //TIMER_LINKEDMAP_H
