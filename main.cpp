#include <iostream>

#include "arrayList.h"

int main() {
    ArrayList<int> a;

    for (int i = 0; i < 25; i++) {
        a.add(i);
    }

    cout << "len = " << a.length() << ", cap = " << a.getCapacity() << "\n";
    return 0;
}
