#include "lexico.h"
#include "expresiones.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

using namespace std;

namespace {
string quitarCadenas(const string& linea) {
    string limpia;
    bool enCadena = false;
    for (size_t i = 0; i < linea.size(); ++i) {
        char ch = linea[i];

        if (ch == '"' && (i == 0 || linea[i - 1] != '\\')) {
            enCadena = !enCadena;
            continue;
        }

        if (!enCadena) {
            limpia += ch;
        }
    }
    return limpia;
}

string trim(const string& texto) {
    size_t inicio = texto.find_first_not_of(" \t");
    if (inicio == string::npos) {
        return "";
    }
    size_t fin = texto.find_last_not_of(" \t");
    return texto.substr(inicio, fin - inicio + 1);
}

bool esExpresionNumericaValida(const string& expresion) {
    if (expresion.empty()) {
        return false;
    }

    bool tieneNumero = false;
    for (char ch : expresion) {
        if (isdigit(static_cast<unsigned char>(ch))) {
            tieneNumero = true;
            continue;
        }
        if (isspace(static_cast<unsigned char>(ch)) || ch == '.' || ch == '(' || ch == ')' ||
            ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
            continue;
        }
        return false;
    }

    return tieneNumero;
}

vector<string> extraerExpresiones(const string& lineaOriginal) {
    vector<string> expresiones;
    string linea = quitarCadenas(lineaOriginal);

    // Eliminar comentarios de linea
    size_t comentario = linea.find("//");
    if (comentario != string::npos) {
        linea = linea.substr(0, comentario);
    }

    // Ignorar directivas de preprocesador
    if (!linea.empty() && linea[0] == '#') {
        return expresiones;
    }

    // Caso comun: inicializaciones y asignaciones
    size_t igual = linea.find('=');
    while (igual != string::npos) {
        size_t inicio = igual + 1;
        size_t fin = linea.find(';', inicio);
        string candidata = trim(linea.substr(inicio, fin == string::npos ? string::npos : fin - inicio));

        if (esExpresionNumericaValida(candidata)) {
            expresiones.push_back(candidata);
        }

        if (fin == string::npos) {
            break;
        }
        igual = linea.find('=', fin + 1);
    }

    // Caso return con literal/expresion numerica
    regex returnRegex(R"(\breturn\s+([^;]+))");
    smatch match;
    if (regex_search(linea, match, returnRegex)) {
        string candidata = trim(match[1].str());
        if (esExpresionNumericaValida(candidata)) {
            expresiones.push_back(candidata);
        }
    }

    return expresiones;
}
} // namespace

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

    while (getline(archivo, linea)) {
        cout << "Linea " << numLinea++ << ": " << linea << endl;

        if (linea.empty()) {
            cout << "  → Linea vacia, ignorada.\n";
            continue;
        }

        vector<string> expresiones = extraerExpresiones(linea);
        bool encontrada = false;

        for (const string& expresionExtraida : expresiones) {
        try {
            double resultado = expr.evaluar(expresionExtraida);
            cout << "  → Expresion: '" << expresionExtraida << "' = " << resultado << endl;
            encontrada = true;
        } catch (const exception& e) {
            cout << "  → Error al evaluar '" << expresionExtraida << "': " << e.what() << endl;
        }
        }

        if (!encontrada) {
            cout << "  → No se encontro expresion aritmetica.\n";
        }
    }

    return 0;
}
