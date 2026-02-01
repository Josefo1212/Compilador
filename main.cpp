#include <iostream>

#include "arrayList.h"

int main() {
    ArrayList<int> a;

    for (int i = 0; i < 25; i++) {
        a.add(i);
    }

    cout << "longitud = " << a.length() << ", capacidad = " << a.getCapacity() << "\n";

        if (int* p0 = a.get(0)) cout << "obtener(0) = " << *p0 << "\n";
        if (int* p10 = a.get(10)) cout << "obtener(10) = " << *p10 << "\n";
        if (int* plast = a.get(a.length() - 1)) cout << "obtener(ultimo) = " << *plast << "\n";

        int removedFirst = 0;
        int removedMiddle = 0;
        int removedLast = 0;
        bool okFirst = a.remove(0, removedFirst);
        bool okMiddle = a.remove(10, removedMiddle);
        bool okLast = a.remove(a.length() - 1, removedLast);
        cout << "eliminado primero=" << (okFirst ? removedFirst : -1)
            << ", medio=" << (okMiddle ? removedMiddle : -1)
            << ", ultimo=" << (okLast ? removedLast : -1) << "\n";

    cout << "despues de eliminar: longitud = " << a.length() << ", capacidad = " << a.getCapacity() << "\n";
    int* newFirst = a.get(0);
    int* newLast = a.get(a.length() - 1);
    if (newFirst && newLast) {
        cout << "ahora primero = " << *newFirst << ", ultimo = " << *newLast << "\n";
    }
    return 0;
}
