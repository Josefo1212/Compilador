#ifndef ARRAYLIST_H
#define ARRAYLIST_H

using namespace std;

template <typename T>
class ArrayList {
    private:
        struct Node {
            T value;
            Node* prev;
            Node* next;

            explicit Node(const T& v) : value(v), prev(nullptr), next(nullptr) {}
            explicit Node(T&& v) : value(static_cast<T&&>(v)), prev(nullptr), next(nullptr) {}
        };

        Node* head;
        Node* tail;
        int size;

        void clearNodes() {
            Node* current = head;
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
            head = nullptr;
            tail = nullptr;
            size = 0;
        }

        void copyFrom(const ArrayList& other) {
            for (Node* n = other.head; n != nullptr; n = n->next) {
                add(n->value);
            }
        }

        void stealFrom(ArrayList&& other) noexcept {
            head = other.head;
            tail = other.tail;
            size = other.size;
            other.head = nullptr;
            other.tail = nullptr;
            other.size = 0;
        }

    public:
        ArrayList() : head(nullptr), tail(nullptr), size(0) {}

        explicit ArrayList(int) : head(nullptr), tail(nullptr), size(0) {}

        ArrayList(const ArrayList& other) : head(nullptr), tail(nullptr), size(0) {
            copyFrom(other);
        }

        ArrayList& operator=(const ArrayList& other) {
            if (this == &other) return *this;
            clearNodes();
            copyFrom(other);
            return *this;
        }

        ArrayList(ArrayList&& other) noexcept : head(nullptr), tail(nullptr), size(0) {
            stealFrom(static_cast<ArrayList&&>(other));
        }

        ArrayList& operator=(ArrayList&& other) noexcept {
            if (this == &other) return *this;
            clearNodes();
            stealFrom(static_cast<ArrayList&&>(other));
            return *this;
        }

        ~ArrayList() {
            clearNodes();
        }

        int length() const { return size; }
        int getCapacity() const { return size; }
        bool isEmpty() const { return size == 0; }

        void clear() { clearNodes(); }

        void add(const T& value) {
            Node* n = new Node(value);
            n->prev = tail;
            n->next = nullptr;
            if (tail != nullptr) {
                tail->next = n;
            } else {
                head = n;
            }
            tail = n;
            ++size;
        }

        void add(T&& value) {
            Node* n = new Node(static_cast<T&&>(value));
            n->prev = tail;
            n->next = nullptr;
            if (tail != nullptr) {
                tail->next = n;
            } else {
                head = n;
            }
            tail = n;
            ++size;
        }
};
#endif