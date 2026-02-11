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
        };

        Node* head;
        Node* tail;
        int size;

        Node* nodeAtOrNull(int index) const {
            if (index < 0 || index >= size) return nullptr;

            if (index <= (size / 2)) {
                Node* current = head;
                for (int i = 0; i < index; i++) {
                    current = current->next;
                }
                return current;
            }

            Node* current = tail;
            for (int i = size - 1; i > index; i--) {
                current = current->prev;
            }
            return current;
        }

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

    public:
        ArrayList() : head(nullptr), tail(nullptr), size(0) {}

        ArrayList(const ArrayList& other) : head(nullptr), tail(nullptr), size(0) {
            copyFrom(other);
        }

        ArrayList& operator=(const ArrayList& other) {
            if (this == &other) return *this;
            clearNodes();
            copyFrom(other);
            return *this;
        }

        ~ArrayList() {
            clearNodes();
        }

        int getSize() const { return size; }
        bool isEmpty() const { return size == 0; }

        T* first() { return head ? &head->value : nullptr; }
        const T* first() const { return head ? &head->value : nullptr; }

        T* last() { return tail ? &tail->value : nullptr; }
        const T* last() const { return tail ? &tail->value : nullptr; }

        void clear() { clearNodes(); }

        T* get(int index) {
            Node* n = nodeAtOrNull(index);
            // Explicación de la línea: return (n == nullptr) ? nullptr : &n->value;
            // Esta línea usa el operador ternario (condición ? valor_si_verdadero : valor_si_falso)
            // - Si n es nullptr (nulo), devuelve nullptr directamente
            // - Si n NO es nullptr, devuelve la dirección de memoria del valor almacenado (&n->value)
            // Esto evita acceder a un puntero nulo y causar un error de segmentación
            return (n == nullptr) ? nullptr : &n->value;
        }

        const T* get(int index) const {
            Node* n = nodeAtOrNull(index);
            // Explicación de la línea: return (n == nullptr) ? nullptr : &n->value;
            // Esta línea usa el operador ternario (condición ? valor_si_verdadero : valor_si_falso)
            // - Si n es nullptr (nulo), devuelve nullptr directamente
            // - Si n NO es nullptr, devuelve la dirección de memoria del valor almacenado (&n->value)
            // Esto evita acceder a un puntero nulo y causar un error de segmentación
            return (n == nullptr) ? nullptr : &n->value;
        }

        bool remove(int index, T& removedValue) {
            Node* target = nodeAtOrNull(index);
            if (target == nullptr) return false;

            if (target->prev != nullptr) {
                target->prev->next = target->next;
            } else {
                head = target->next;
            }

            if (target->next != nullptr) {
                target->next->prev = target->prev;
            } else {
                tail = target->prev;
            }

            removedValue = target->value;
            delete target;
            --size;
            return true;
        }

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

        T* next(int index) {
            if (index < 0 || index >= size - 1) return nullptr;
            
            Node* current = nodeAtOrNull(index);
            if (current == nullptr || current->next == nullptr) {
                return nullptr;
            }
            
            return &(current->next->value);
        }

        const T* next(int index) const {
            if (index < 0 || index >= size - 1) return nullptr;
            
            Node* current = nodeAtOrNull(index);
            if (current == nullptr || current->next == nullptr) {
                return nullptr;
            }
            
            return &(current->next->value);
        }

        T* prios(int index) {
            if (index <= 0 || index >= size) return nullptr;
            
            Node* current = nodeAtOrNull(index);
            if (current == nullptr || current->prev == nullptr) {
                return nullptr;
            }
            
            return &(current->prev->value);
        }
        
        const T* prios(int index) const {
            if (index <= 0 || index >= size) return nullptr;
            
            Node* current = nodeAtOrNull(index);
            if (current == nullptr || current->prev == nullptr) {
                return nullptr;
            }
            
            return &(current->prev->value);
        }
};
#endif