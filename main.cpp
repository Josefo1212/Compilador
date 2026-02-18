#include "lexico.h"
#include "expresiones.h"
#include <fstream>
#include <iostream>
#include <regex>

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

    // Mostrar tabla de simbolos
    cout << "\n--- Tabla de Simbolos ---\n";
    const auto& ts = lexico.getTablaSimbolos();
    for (int i = 0; i < ts.getSize(); ++i) {
        cout << *ts.get(i) << endl;
    }

    // Reposicionar el archivo para evaluar expresiones
    archivo.clear();
    archivo.seekg(0, ios::beg);

    // Evaluar expresiones desde el archivo
    cout << "\n--- Evaluacion de Expresiones ---\n";
    Expresiones expr;
    string linea;
    regex expresionRegex("^\\s*[0-9()+\\-*/^\\s.]+\\s*$", regex_constants::ECMAScript); // Corregido: guion escapado correctamente

    while (getline(archivo, linea)) {
        // Ignorar líneas vacías
        if (linea.empty()) {
            cout << "Linea ignorada: " << linea << endl; // Depuración
            continue;
        }

        smatch match;
        if (regex_match(linea, match, expresionRegex)) {
            string expresion = match[0]; // Extraer la expresión completa
            cout << "Procesando expresion: " << expresion << endl; // Depuración
            try {
                double resultado = expr.evaluar(expresion);
                cout << "Expresion: " << expresion << " = " << resultado << endl;
            } catch (const exception& e) {
                cerr << "Error al evaluar expresion: " << e.what() << endl;
            }
        } else {
            cout << "No se encontro una expresion valida en la linea: " << linea << endl; // Depuración
        }
    }

    return 0;
}
