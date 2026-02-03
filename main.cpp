#include <iostream>

#include "arrayList.h"

int main() {
    ArrayList<int> a;

    for (int i = 0; i < 25; i++) {
        a.add(i);
    }

    cout << "longitud = " << a.getSize() << ", capacidad = " << a.getCapacity() << "\n";

        if (int* p0 = a.first()) cout << "obtener(0) = " << *p0 << "\n";
        if (int* p10 = a.get(10)) cout << "obtener(10) = " << *p10 << "\n";
        if (int* plast = a.last()) cout << "obtener(ultimo) = " << *plast << "\n";

        int removedFirst = 0;
        int removedMiddle = 0;
        int removedLast = 0;
        bool okFirst = a.remove(0, removedFirst);
        bool okMiddle = a.remove(10, removedMiddle);
        bool okLast = a.remove(a.getSize() - 1, removedLast);
        cout << "eliminado primero = " << (okFirst ? removedFirst : -1)
            << ", medio = " << (okMiddle ? removedMiddle : -1)
            << ", ultimo = " << (okLast ? removedLast : -1) << "\n";

    cout << "despues de eliminar: longitud = " << a.getSize() << ", capacidad = " << a.getCapacity() << "\n";
    int* newFirst = a.first();
    int* newLast = a.last();
    if (newFirst && newLast) {
        cout << "ahora primero = " << *newFirst << ", ultimo = " << *newLast << "\n";
    }
    return 0;
}
