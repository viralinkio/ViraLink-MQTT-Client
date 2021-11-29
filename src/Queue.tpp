#ifndef VIRALINK_QUEUE_TPP
#define VIRALINK_QUEUE_TPP

#include <Arduino.h>

using namespace std;

template<class T>
class Queue {
private:
    vector<T> list;
    uint16_t size;
public:
    Queue(uint16_t size_t) {
        this->size = size_t;
    }

    ~Queue() {
        clear();
    }

    bool isFull();

    bool isEmpty();

    uint16_t getCounts();

    bool push(const T &item);

    T peek();

    void clear();

    bool removeLastPeek();
};

template<class T>
bool Queue<T>::removeLastPeek() {
//    std::next(list.begin(), 3)
    if (isEmpty()) return false;

    list.erase(list.begin());
    return true;
}

template<class T>
bool Queue<T>::isFull() {
    return list.size() == size;
}

template<class T>
bool Queue<T>::isEmpty() {
    return list.empty();
}

template<class T>
uint16_t Queue<T>::getCounts() {
    return list.size();
}

template<class T>
bool Queue<T>::push(const T &item) {
    if (isFull()) return false;

    list.push_back(item);
    return true;
}

template<class T>
T Queue<T>::peek() {
    if (isEmpty()) return T(); // Returns empty
    return list.front();
}

template<class T>
void Queue<T>::clear() {
    list.clear();
}

#endif //VIRALINK_QUEUE_TPP
