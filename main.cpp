#include "lexico.h"
#include <fstream>
#include <iostream>

using namespace std;

int main() {
    // Abrir el archivo de entrada
    ifstream archivo("entrada.txt");
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo de entrada." << endl;
        return 1;
    }

    // Crear una instancia de Lexico y cargar el archivo
    Lexico lexico(archivo);

    // Procesar los tokens
    token t;
    do {
        t = lexico.siguiente();
        cout << "Token: " << token::tipoToString(t.getTipo()) << "\tLexema: " << t.getLexema() 
             << "\tLinea: " << t.getLinea() << "\tColumna: " << t.getColumna() << endl;
    } while (t.getTipo() != token::FIN);

    // Mostrar tabla de símbolos
    cout << "\n--- Tabla de Simbolos ---\n";
    const auto& ts = lexico.getTablaSimbolos();
    for (int i = 0; i < ts.getSize(); ++i) {
        cout << *ts.get(i) << endl;
    }

    return 0;
}
