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
        cout << "Token: " << t.tipoToString() << "\tLexema: " << t.getLexema() 
             << "\tLinea: " << t.getLinea() << "\tColumna: " << t.getColumna() << endl;
    } while (t.getTipo() != token::FIN);

    // Mostrar tabla de símbolos
    cout << "\n--- Tabla de Símbolos ---\n";
    const auto& ts = lexico.getTablaSimbolos();
    for (int i = 0; i < ts.getSize(); ++i) {
        cout << *ts.get(i) << endl;
    }

    // Mostrar errores
    const auto& err = lexico.getErrores();
    if (err.getSize() > 0) {
        cout << "\n--- Errores Léxicos ---\n";
        for (int i = 0; i < err.getSize(); ++i) {
            cout << *err.get(i) << endl;
        }
    }

    return 0;
}
