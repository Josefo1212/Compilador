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

    // Reposicionar el archivo para leer línea por línea
    archivo.clear();
    archivo.seekg(0, ios::beg);

    cout << "\n--- Evaluacion de Expresiones (linea por linea) ---\n";
    Expresiones expr;
    string linea;
    int numLinea = 1;

    // Expresión regular para encontrar números o expresiones simples
    regex exprRegex(R"(\d+(?:\.\d+)?|\d+\s*[+\-*/^()]\s*\d+|\(\s*\d+(?:\s*[+\-*/^]\s*\d+)+\s*\))");

    while (getline(archivo, linea)) {
        cout << "Linea " << numLinea++ << ": " << linea << endl;

        if (linea.empty()) {
            cout << "  → Linea vacia, ignorada.\n";
            continue;
        }

        smatch match;
        string resto = linea;
        bool encontrada = false;

        while (regex_search(resto, match, exprRegex)) {
        string expresionExtraida = match[0];
        // Limpiar espacios
        size_t first = expresionExtraida.find_first_not_of(" \t");
        size_t last = expresionExtraida.find_last_not_of(" \t");
        if (first != string::npos && last != string::npos) {
            expresionExtraida = expresionExtraida.substr(first, last - first + 1);
        }
        try {
            double resultado = expr.evaluar(expresionExtraida);
            cout << "  → Expresion: '" << expresionExtraida << "' = " << resultado << endl;
            encontrada = true;
        } catch (const exception& e) {
            cout << "  → Error al evaluar '" << expresionExtraida << "': " << e.what() << endl;
        }
        resto = match.suffix();
    }

        if (!encontrada) {
            cout << "  → No se encontro expresion aritmetica.\n";
        }
    }

    return 0;
}
