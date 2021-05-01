#pragma once

#include <endian.hpp>

class Buffer {
public:
    Buffer(char* b, size_t capacity) : capacity(capacity) {
        buf = (char*)malloc(sizeof(char)*capacity);
        memcpy(buf, b, capacity);
        offset = 0;
    }
    Buffer(size_t capacity) : capacity(capacity) {
        buf = (char*)malloc(sizeof(char)*capacity);
        offset = 0;
    }
    Buffer(std::string str) {
        capacity = str.length();
        buf = (char*)malloc(sizeof(char)*capacity);
        memcpy(buf, str.c_str(), capacity);
        offset = 0;
    }
    ~Buffer() {
        free(buf);
    }
    char& operator[](size_t idx) {
        return buf[offset + idx];
    }
    Buffer& operator+=(const size_t offs) {
        offset += offs;
        return *this;
    }
    Buffer& operator-=(const size_t offs) {
        offset -= offs;
        return *this;
    }
    Buffer& append(Buffer& other) {
        size_t pcap = capacity;
        setCapacity(capacity + other.getLength());
        memcpy(buf + pcap, &other, other.getLength());
        return *this;
    }
    void* data() {
        return buf + offset;
    }
    size_t getLength() {
        return capacity - offset;
    }
    size_t getCapacity() {
        return capacity;
    }
    void setCapacity(size_t nwcap) {
        capacity = nwcap;
        buf = (char*)realloc(buf, sizeof(char)*capacity);
    }
    bool isEmpty() {
        return getLength() == 0;
    }
    void rewind() {
        offset = 0;
    }
    template <typename T>
    T LEGet() {
        if (sizeof(T) > getLength()) { throw "type too long"; }
        T res = Endian::LittleEndian::get<T>(data());
        offset += sizeof(T);
        return res;
    }
    template <typename T>
    size_t LEPut(T bu) {
        if (sizeof(T) > getLength()) { throw "type too long"; }
        size_t off = Endian::LittleEndian::put(data(), &bu);
        offset += off;
        return off;
    }
    std::string get9PString() {
        uint16_t len = LEGet<uint16_t>();
        if (len > getLength()) { throw "string too long"; }
        std::string res((char*)data(), len);
        offset += len;
        return res;
    }

private:
    char* buf;
    size_t capacity;
    size_t offset;
};