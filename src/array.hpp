#pragma once
#include "common.hpp"
#include <stdio.h>

template<typename T>
struct Array {
    T* data = nullptr;
    int count = 0;
    int capacity = 0;

    T& operator[](size_t index) {
        verify((int)index < count);
        return data[index];
    }

    const T& operator[](size_t index) const {
        verify((int)index < count);
        return data[index];
    }

    void pop() {
        verify(count > 0);
        --count;
    }

    T& last() {
        verify(count > 0);
        return data[count - 1];
    }

    T* begin() { return data ? &data[0] : nullptr; }
    T* end() { return data ? &data[count] : nullptr; }
    const T* begin() const { return data ? &data[0] : nullptr; }
    const T* end() const { return data ? &data[count] : nullptr; }

    void reserve(int newCapacity) {
        if (capacity >= newCapacity) {
            return;
        }
        T* newData = new T[newCapacity];
        memcpy(newData, this->data, sizeof(T) * this->count);
        delete[] this->data;
        this->data = newData;
        this->capacity = newCapacity;
    }

    void grow(int increment) {
        int neededCount = this->count + increment;
        if (neededCount > this->capacity) {
            int newCapacity = this->capacity < 4 ? 4 : this->capacity * 2;
            if (newCapacity < neededCount) {
                newCapacity = neededCount;
            }
            T* newData = new T[newCapacity];
            memcpy(newData, this->data, sizeof(T) * this->count);
            delete[] this->data;
            this->data = newData;
            this->capacity = newCapacity;
        }
    }

    void push(const T& value) {
        grow(1);
        this->data[this->count++] = value;
    }

    void pushMultiple(const T* values, size_t numValues) {
        if (numValues) {
            verify(values);
            grow((int)numValues);
            memcpy(&data[count], values, numValues * sizeof(T));
            count += (int)numValues;
        }
    }

    void destroy() {
        delete[] this->data;
        this->data = nullptr;
        this->count = 0;
        this->capacity = 0;
    }

    void removeAt(int index) {
        verify(index >= 0 && index < this->count);
        memmove(&this->data[index], &this->data[index + 1], sizeof(T) * (size_t)(this->count - index));
        --this->count;
    }

    void removeManyAt(int index, size_t numValues) {
        verify(index >= 0 && index <= this->count);
        verify((size_t)index + numValues <= (size_t)this->count);
        if (numValues) {
            int right = this->count - index;
            memmove(&this->data[index], &this->data[index + numValues], sizeof(T) * right);
            this->count -= (int)numValues;
        }
    }

    void insertAt(int index, const T& value) {
        verify(index >= 0 && index <= this->count);
        grow(1);
        for (int i = count; i > index; --i) {
            data[i] = data[i - 1];
        }
        this->data[index] = value;
        ++this->count;
    }

    void insertMultipleAt(int index, const T* values, size_t numValues) {
        verify(index >= 0 && index <= this->count);
        if (numValues) {
            grow((int)numValues);
            int right = (this->count - index);
            memmove(&this->data[index + numValues], &this->data[index], sizeof(T) * right);
            memcpy(&this->data[index], values, sizeof(T) * numValues);
            this->count += (int)numValues;
        }
    }

    void remove(const T& value) {
        for (int i = 0; i < this->count; ++i) {
            if (this->data[i] == value) {
                this->removeAt(i);
                return;
            }
        }
        verify(false); // Item not found in array.
    }

    int findIndex(const T& value) {
        for (int i = 0; i < count; ++i) {
            if (data[i] == value) {
                return i;
            }
        }
        return -1;
    }

    bool contains(const T& value) {
        for (int i = 0; i < count; ++i) {
            if (data[i] == value) {
                return true;
            }
        }
        return false;
    }

    void reverse() {
        for (int i = 0; i < count / 2; ++i) {
            T tmp = data[i];
            data[i] = data[count - i - 1];
            data[count - i - 1] = tmp;
        }
    }
};
