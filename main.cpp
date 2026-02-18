#include "lexico.h"
#include "expresiones.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include <map>

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

bool esExpresionValida(const string& expresion) {
    if (expresion.empty()) {
        return false;
    }
    // Permite letras, números, operadores y paréntesis
    for (char ch : expresion) {
        if (isalnum(static_cast<unsigned char>(ch)) || ch == '_' ||
            isspace(static_cast<unsigned char>(ch)) || ch == '.' || ch == '(' || ch == ')' ||
            ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
            continue;
        }
        return false;
    }
    return true;
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

        if (esExpresionValida(candidata)) {
            expresiones.push_back(candidata);
        }

        if (fin == string::npos) {
            break;
        }
        igual = linea.find('=', fin + 1);
    }

    // Caso return con literal/expresion
    regex returnRegex(R"(\breturn\s+([^;]+))");
    smatch match;
    if (regex_search(linea, match, returnRegex)) {
        string candidata = trim(match[1].str());
        if (esExpresionValida(candidata)) {
            expresiones.push_back(candidata);
        }
    }

    return expresiones;
}

// Busca el valor de una variable en la tabla de símbolos y un mapa de valores
bool obtenerValorVariable(const string& var, const map<string, double>& valores, double& valor) {
    auto it = valores.find(var);
    if (it != valores.end()) {
        valor = it->second;
        return true;
    }
    return false;
}

// Reemplaza variables por su valor en la expresión, si es posible
string reemplazarVariables(const string& expr, const map<string, double>& valores) {
    string resultado;
    string var;
    for (size_t i = 0; i < expr.size(); ++i) {
        char ch = expr[i];
        if (isalpha(ch) || ch == '_') {
            var += ch;
        } else {
            if (!var.empty()) {
                auto it = valores.find(var);
                if (it != valores.end()) {
                    resultado += to_string(it->second);
                } else {
                    resultado += var;
                }
                var.clear();
            }
            resultado += ch;
        }
    }
    if (!var.empty()) {
        auto it = valores.find(var);
        if (it != valores.end()) {
            resultado += to_string(it->second);
        } else {
            resultado += var;
        }
    }
    return resultado;
}

// Verifica si una cadena es un número válido (entero o decimal)
bool esNumero(const string& s) {
    if (s.empty()) return false;
    char* endptr = nullptr;
    strtod(s.c_str(), &endptr);
    return (*endptr == '\0');
}

// Extrae las asignaciones simples de variables a valores numéricos
map<string, double> extraerValoresVariables(const string& archivoPath) {
    map<string, double> valores;
    ifstream archivo(archivoPath);
    string linea;
    regex declVar(R"((int|float|double|long|short|unsigned|signed|char)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([-]?[0-9]+(\.[0-9]+)?))");
    smatch match;
    while (getline(archivo, linea)) {
        string sinCadena = quitarCadenas(linea);
        // Buscar todas las declaraciones de variables con asignación numérica
        string::const_iterator searchStart(sinCadena.cbegin());
        while (regex_search(searchStart, sinCadena.cend(), match, declVar)) {
            string nombre = match[2].str();
            string valor = match[3].str();
            if (esNumero(valor)) {
                valores[nombre] = stod(valor);
            }
            searchStart = match.suffix().first;
        }
    }
    return valores;
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
    // Extraer valores de variables inicializadas
    map<string, double> valores = extraerValoresVariables("entrada.txt");

    while (getline(archivo, linea)) {
        vector<string> expresiones = extraerExpresiones(linea);

        for (const string& expresionExtraida : expresiones) {
            string exprReemplazada = reemplazarVariables(expresionExtraida, valores);
            try {
                double resultado = expr.evaluar(exprReemplazada);
                cout << "Expresion: '" << expresionExtraida << "' = " << resultado << endl;
            } catch (const exception& e) {
                cout << "No se puede evaluar: '" << expresionExtraida << "' (" << e.what() << ")" << endl;
            }
        }
    }

    return 0;
}
