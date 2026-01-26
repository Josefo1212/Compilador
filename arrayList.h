#ifndef ARRAYLIST_H
#define ARRAYLIST_H

using namespace std;

template <typename T>
class ArrayList {
    private:
        T* data;
        int capacity;
        int size;

        void resize(int newCapacity) {
            if (newCapacity < size) {
                newCapacity = size;
            }
            T* newData = new T[newCapacity];
            for (int i = 0; i < size; i++) {
                newData[i] = data[i];
            }
            delete[] data;
            data = newData;
            capacity = newCapacity;
            }

        void ensureCapacityForOneMore() {
            if (size < capacity) return;
            int newCapacity = (capacity <= 0) ? 1 : (capacity * 2);
            resize(newCapacity);
        }

    public:
        ArrayList() : data(nullptr), capacity(0), size(0) {
            resize(10);
        }

        explicit ArrayList(int initialCapacity) : data(nullptr), capacity(0), size(0) {
            if (initialCapacity <= 0) initialCapacity = 10;
            resize(initialCapacity);
        }

        ~ArrayList() {
            delete[] data;
        }

        int length() const { return size; }
        int getCapacity() const { return capacity; }
        bool isEmpty() const { return size == 0; }

        void clear() { size = 0; }

        void add(const T& value) {
            ensureCapacityForOneMore();
            data[size++] = value;
        }
};
#endif