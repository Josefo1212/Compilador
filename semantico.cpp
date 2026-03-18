#include "semantico.h"
#include <iostream>
#include <cctype>
#include <cmath>

using namespace std;

// Helpers usados por el tipado de expresiones
static bool esTipoNumericoVar(TipoVariable tipo);
static TipoVariable promoverTipoNumericoVar(TipoVariable izq, TipoVariable der);
static void verificarDivisionPorCeroSiLiteral(const shared_ptr<NodoAST>& rhs);

static bool esNodoWrapper(const string& etiqueta) {
    return etiqueta == "Condicion" || etiqueta == "Entonces" || etiqueta == "Sino" ||
           etiqueta == "Cuerpo" || etiqueta == "Inicializacion" || etiqueta == "Actualizacion" ||
           etiqueta == "Parametros" || etiqueta == "Argumentos" || etiqueta == "Inicializador";
}

static int longitudCadenaLiteral(const string& lexema) {
    // lexema esperado con comillas, ej: "Hola\n".
    if (lexema.size() < 2) return 0;
    size_t inicio = (lexema.front() == '"') ? 1 : 0;
    size_t fin = (lexema.back() == '"' && lexema.size() >= 2) ? lexema.size() - 1 : lexema.size();
    int longitud = 0;
    for (size_t i = inicio; i < fin; ++i) {
        if (lexema[i] == '\\' && i + 1 < fin) {
            ++longitud;
            ++i;
        } else {
            ++longitud;
        }
    }
    return longitud;
}

static int longitudCaracterLiteral(const string& lexema) {
    // lexema esperado con comillas simples, ej: 'a', '\n', '\x41', '10'
    if (lexema.size() < 2) return 0;
    size_t inicio = (lexema.front() == '\'') ? 1 : 0;
    size_t fin = (lexema.back() == '\'' && lexema.size() >= 2) ? lexema.size() - 1 : lexema.size();

    int longitud = 0;
    for (size_t i = inicio; i < fin; ++i) {
        if (lexema[i] != '\\') {
            ++longitud;
            continue;
        }

        // Escape
        if (i + 1 >= fin) {
            ++longitud;
            break;
        }

        char esc = lexema[i + 1];

        // Escape simple: \n, \t, \\, \' , etc.
        if (esc != 'x' && esc != 'u' && esc != 'U' && !(esc >= '0' && esc <= '7')) {
            ++longitud;
            i += 1;
            continue;
        }

        if (esc == 'x') {
            // \xHH... (al menos 1 hex)
            size_t j = i + 2;
            if (j < fin && isxdigit(static_cast<unsigned char>(lexema[j]))) {
                while (j < fin && isxdigit(static_cast<unsigned char>(lexema[j]))) ++j;
                ++longitud;
                i = j - 1;
                continue;
            }
            ++longitud;
            i += 1;
            continue;
        }

        if (esc == 'u') {
            // \uXXXX
            size_t j = i + 2;
            size_t count = 0;
            while (j < fin && count < 4 && isxdigit(static_cast<unsigned char>(lexema[j]))) {
                ++j;
                ++count;
            }
            ++longitud;
            i = (j > 0) ? (j - 1) : i;
            continue;
        }

        if (esc == 'U') {
            // \UXXXXXXXX
            size_t j = i + 2;
            size_t count = 0;
            while (j < fin && count < 8 && isxdigit(static_cast<unsigned char>(lexema[j]))) {
                ++j;
                ++count;
            }
            ++longitud;
            i = (j > 0) ? (j - 1) : i;
            continue;
        }

        // Octal: \[0-7]{1,3}
        size_t j = i + 1;
        size_t count = 0;
        while (j < fin && count < 3 && lexema[j] >= '0' && lexema[j] <= '7') {
            ++j;
            ++count;
        }
        ++longitud;
        i = j - 1;
    }

    return longitud;
}

static void verificarAsignacionNumericaEstrica(TipoNumerico destino, TipoNumerico origen, const string& contexto) {
    if (destino == TipoNumerico::NINGUNO || origen == TipoNumerico::NINGUNO) {
        throw IncompatibilidadTiposNumericosError("Asignacion no numerica" + (contexto.empty() ? "" : (" en " + contexto)));
    }
    // Regla estricta implementada en semantico.h: prohibir float -> int.
    if (destino == TipoNumerico::ENTERO && origen == TipoNumerico::FLOTANTE) {
        throw IncompatibilidadTiposNumericosError(
            "Asignacion de flotante a entero" +
            (contexto.empty() ? "." : (" (" + contexto + ")."))
        );
    }
}

// Constructor: inicia el ambito global
Semantico::Semantico(shared_ptr<NodoAST> raiz) : raiz(raiz) {
    entrarAmbito();   // ambito global
}

// Analizar: punto de entrada
void Semantico::analizar() {
    visitar(raiz);
}

// Manejo de ambitos
void Semantico::entrarAmbito() {
    tablaSimbolos.push_back({});
}

void Semantico::salirAmbito() {
    if (!tablaSimbolos.empty())
        tablaSimbolos.pop_back();
}

// Declarar variable en el ambito actual
void Semantico::declararVariable(const string& nombre, const Simbolo& sim) {
    if (tablaSimbolos.empty())
        throw runtime_error("Error interno: no hay ambito para declarar variable");
    auto& ambito = tablaSimbolos.back();
    if (ambito.find(nombre) != ambito.end())
        throw runtime_error("Error semantico: variable '" + nombre + "' ya declarada en este ambito");
    ambito[nombre] = sim;
}

// Buscar variable desde el ambito mas interno al mas externo
Semantico::Simbolo* Semantico::buscarVariable(const string& nombre) {
    if (tablaSimbolos.empty()) return nullptr;
    for (int i = static_cast<int>(tablaSimbolos.size()) - 1; i >= 0; --i) {
        auto it = tablaSimbolos[i].find(nombre);
        if (it != tablaSimbolos[i].end())
            return &it->second;
    }
    return nullptr;
}

// Obtener el tipo de una variable (lanza excepcion si no existe)
TipoVariable Semantico::obtenerTipoVariable(const string& nombre) {
    Simbolo* sim = buscarVariable(nombre);
    if (!sim)
        throw runtime_error("Error semantico: variable '" + nombre + "' no declarada");
    return sim->tipo;
}

// Conversiones de tipo
TipoNumerico Semantico::obtenerTipoNumericoDeLiteral(const string& lexema) {
    // Determina si el literal es entero o flotante
    for (char c : lexema) {
        if (c == '.' || c == 'e' || c == 'E')
            return TipoNumerico::FLOTANTE;
    }
    return TipoNumerico::ENTERO;
}

TipoNumerico Semantico::obtenerTipoNumericoDeTipoVariable(TipoVariable tv) {
    switch (tv) {
        case TipoVariable::ENTERO: return TipoNumerico::ENTERO;
        case TipoVariable::FLOTANTE: return TipoNumerico::FLOTANTE;
        default: return TipoNumerico::NINGUNO;
    }
}

TipoNumerico Semantico::obtenerTipoNumericoDeVariable(const string& nombre) {
    Simbolo* sim = buscarVariable(nombre);
    if (!sim)
        throw runtime_error("Error semantico: variable '" + nombre + "' no declarada");
    // Un arreglo puede usarse como argumento a una funcion (p.ej. printf),
    // pero no es un valor numerico util en operaciones aritmeticas.
    if (sim->esArreglo)
        return TipoNumerico::NINGUNO;
    return obtenerTipoNumericoDeTipoVariable(sim->tipo);
}

// Compatibilidad y tipo resultante en operaciones binarias
bool Semantico::sonCompatibles(TipoNumerico izq, TipoNumerico der, const string& operador) {
    // Asignacion: permitimos conversiones numericas (int<->float) por simplicidad.
    // El control estricto (p.ej. float->int) se puede endurecer mas adelante.
    if (operador == "=") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    // Para operadores aritmeticos, ambos deben ser numericos
    if (operador == "+" || operador == "-" || operador == "*" || operador == "/") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    // Modulo solo con enteros
    if (operador == "%") {
        return izq == TipoNumerico::ENTERO && der == TipoNumerico::ENTERO;
    }
    // Operadores relacionales y de igualdad: ambos numericos
    if (operador == "<" || operador == ">" || operador == "<=" || operador == ">=" ||
        operador == "==" || operador == "!=") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    // Operadores logicos: ambos numericos (se interpretan como booleanos)
    if (operador == "&&" || operador == "||") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    return false;
}

TipoNumerico Semantico::tipoResultante(TipoNumerico izq, TipoNumerico der, const string& operador) {
    if (operador == "+" || operador == "-" || operador == "*" || operador == "/") {
        // Promocion a flotante si alguno es flotante
        if (izq == TipoNumerico::FLOTANTE || der == TipoNumerico::FLOTANTE)
            return TipoNumerico::FLOTANTE;
        return TipoNumerico::ENTERO;
    }
    if (operador == "%") {
        return TipoNumerico::ENTERO;   // resultado entero
    }
    if (operador == "<" || operador == ">" || operador == "<=" || operador == ">=" ||
        operador == "==" || operador == "!=" || operador == "&&" || operador == "||") {
        // En C, las expresiones relacionales/logicas son de tipo int
        return TipoNumerico::ENTERO;
    }
    return TipoNumerico::NINGUNO;
}

// Verificacion de condicion (if, while, for)
void Semantico::verificarCondicion(shared_ptr<NodoAST> condNodo) {
    if (!condNodo) throw runtime_error("Error semantico: condicion nula");
    // El parser envuelve la condicion dentro de un nodo "Condicion".
    if (condNodo->etiqueta == "Condicion" && condNodo->hijos.size() == 1)
        condNodo = condNodo->hijos[0];
    TipoNumerico tipoCond = visitarExpresion(condNodo);
    if (tipoCond == TipoNumerico::NINGUNO)
        throw runtime_error("Error semantico: la condicion no es de tipo numerico");
}

// Visita general del AST
void Semantico::visitar(shared_ptr<NodoAST> nodo) {
    if (!nodo) return;

    string etiq = nodo->etiqueta;

    // Nodos wrapper generados por el parser: solo propagan la visita a sus hijos.
    if (esNodoWrapper(etiq)) {
        for (auto& hijo : nodo->hijos)
            visitar(hijo);
        return;
    }

    // Extraer el tipo de nodo según el prefijo de la etiqueta
    if (etiq == "Programa") {
        for (auto& hijo : nodo->hijos)
            visitar(hijo);
    }
    else if (etiq == "DirectivaPreprocesador") {
        // Ignorar por ahora
    }
    else if (etiq == "Funcion") {
        visitarFuncion(nodo);
    }
    else if (etiq == "Declaracion") {
        visitarDeclaracion(nodo);
    }
    else if (etiq == "SentenciaExpresion") {
        // La expresion puede ser asignacion, llamada, etc.
        if (!nodo->hijos.empty())
            visitarExpresionTipo(nodo->hijos[0]);
    }
    else if (etiq == "Return") {
        // Verificar tipo de retorno
        if (tipoFuncionActual.empty())
            throw runtime_error("Error semantico: return fuera de funcion");
        TipoVariable tipoEsperado = tipoFuncionActual.back();
        if (nodo->hijos.empty()) {
            // return sin expresion: solo valido en funciones void
            if (tipoEsperado != TipoVariable::DESCONOCIDO) // suponemos void como DESCONOCIDO? Mejor usar un tipo VOID
                throw runtime_error("Error semantico: return sin valor en funcion no void");
        } else {
            TipoVariable tipoRetVar = visitarExpresionTipo(nodo->hijos[0]);
            if (tipoEsperado == TipoVariable::CADENA) {
                if (tipoRetVar != TipoVariable::CADENA)
                    throw runtime_error("Error semantico: tipo de retorno incompatible (se esperaba CADENA)");
            } else {
                TipoNumerico tipoRet = obtenerTipoNumericoDeTipoVariable(tipoRetVar);
                TipoNumerico tipoEspNum = obtenerTipoNumericoDeTipoVariable(tipoEsperado);
                if (!sonCompatibles(tipoEspNum, tipoRet, "="))
                    throw IncompatibilidadTiposNumericosError("Tipo de retorno incompatible");
                verificarAsignacionNumericaEstrica(tipoEspNum, tipoRet, "return");
            }
        }
    }
    else if (etiq == "If") {
        // Hijos: [Condicion, Entonces, Sino?]
        if (nodo->hijos.size() < 2) throw runtime_error("Nodo If mal formado");
        verificarCondicion(nodo->hijos[0]);   // Condicion (wrapper)
        visitarSentencia(nodo->hijos[1]);     // Entonces (wrapper)
        if (nodo->hijos.size() == 3)
            visitarSentencia(nodo->hijos[2]); // Sino
    }
    else if (etiq == "While") {
        if (nodo->hijos.size() != 2) throw runtime_error("Nodo While mal formado");
        verificarCondicion(nodo->hijos[0]);
        visitarSentencia(nodo->hijos[1]);
    }
    else if (etiq == "For") {
        if (nodo->hijos.size() != 4) throw runtime_error("Nodo For mal formado");
        // En C, el for introduce un ambito para la inicializacion (p.ej. for(int i=0;...)).
        entrarAmbito();
        // Inicializacion: puede ser declaracion o expresion
        if (nodo->hijos[0]->hijos.size() > 0)
            visitar(nodo->hijos[0]->hijos[0]); // el hijo de Inicializacion
        // Condicion
        if (nodo->hijos[1]->hijos.size() > 0)
            verificarCondicion(nodo->hijos[1]->hijos[0]);
        // Actualizacion
        if (nodo->hijos[2]->hijos.size() > 0)
            visitarExpresionTipo(nodo->hijos[2]->hijos[0]);
        // Cuerpo
        visitarSentencia(nodo->hijos[3]);
        salirAmbito();
    }
    else if (etiq == "DoWhile") {
        if (nodo->hijos.size() != 2) throw runtime_error("Nodo DoWhile mal formado");
        visitarSentencia(nodo->hijos[0]); // cuerpo (wrapper)
        verificarCondicion(nodo->hijos[1]); // condicion (wrapper)
    }
    else if (etiq == "Bloque") {
        visitarBloque(nodo);
    }
    else if (etiq == "LlamadaFuncion") {
        visitarLlamadaFuncion(nodo);
    }
    else if (etiq == "SentenciaVacia") {
        // No hace nada.
    }
    else {
        // Si es otro tipo, podría ser una expresion suelta? Normalmente no.
        // Pero por si acaso, la tratamos como expresion
        visitarExpresionTipo(nodo);
    }
}

// Visita de sentencias (wrapper que maneja nodos de sentencia)
void Semantico::visitarSentencia(shared_ptr<NodoAST> nodo) {
    if (!nodo) return;
    // Las sentencias pueden ser bloques, declaraciones, etc.
    // Simplemente llamamos a visitar, que ya discrimina.
    visitar(nodo);
}

// Visita de bloque: nuevo ambito
void Semantico::visitarBloque(shared_ptr<NodoAST> nodo) {
    entrarAmbito();
    for (auto& hijo : nodo->hijos) {
        visitar(hijo);
    }
    salirAmbito();
}

// Visita de declaracion
void Semantico::visitarDeclaracion(shared_ptr<NodoAST> nodo) {
    // Nodo "Declaracion" tiene hijos: [Tipo, Identificador, (opcional Arreglo), (opcional Inicializador)]
    if (nodo->hijos.size() < 2) throw runtime_error("Declaracion mal formada");

    // Obtener tipo
    string tipoLex = nodo->hijos[0]->etiqueta; // "Tipo: int"
    size_t pos = tipoLex.find(": ");
    string tipoStr = (pos != string::npos) ? tipoLex.substr(pos + 2) : "";
    TipoVariable tipoVar;
    bool esChar = false;
    if (tipoStr == "int") tipoVar = TipoVariable::ENTERO;
    else if (tipoStr == "float") tipoVar = TipoVariable::FLOTANTE;
    else if (tipoStr == "char") { tipoVar = TipoVariable::ENTERO; esChar = true; } // char es entero en C
    else tipoVar = TipoVariable::DESCONOCIDO;

    // Obtener nombre
    string idLex = nodo->hijos[1]->etiqueta; // "Identificador: x"
    pos = idLex.find(": ");
    string nombre = (pos != string::npos) ? idLex.substr(pos + 2) : "";

    bool esArreglo = false;
    int tamArreglo = 0;
    size_t indice = 2;
    if (indice < nodo->hijos.size() && nodo->hijos[indice]->etiqueta == "Arreglo") {
        esArreglo = true;
        auto arrNodo = nodo->hijos[indice];
        if (!arrNodo->hijos.empty()) {
            // Hay tamaño específico
            string tamLex = arrNodo->hijos[0]->etiqueta; // "Tamano: 10"
            pos = tamLex.find(": ");
            string tamStr = (pos != string::npos) ? tamLex.substr(pos + 2) : "";
            tamArreglo = stoi(tamStr);
        }
        ++indice;
    }

    // Tratar char[] como CADENA.
    if (esArreglo && esChar) {
        tipoVar = TipoVariable::CADENA;
    }

    // Declarar variable
    Simbolo sim(tipoVar, esArreglo, tamArreglo);
    declararVariable(nombre, sim);

    // Si hay inicializador
    if (indice < nodo->hijos.size() && nodo->hijos[indice]->etiqueta == "Inicializador") {
        auto initNodo = nodo->hijos[indice];
        if (!initNodo->hijos.empty()) {
            // Caso especial: char[] = "..." (string literal)
            if (esArreglo && esChar && initNodo->hijos[0]->etiqueta.find("Cadena: ") == 0) {
                string lex = initNodo->hijos[0]->etiqueta.substr(8);
                int len = longitudCadenaLiteral(lex);
                int requerido = len + 1; // incluye '\0'

                if (tamArreglo == 0) {
                    auto& ambito = tablaSimbolos.back();
                    ambito[nombre].tamanoArreglo = requerido;
                } else if (tamArreglo < requerido) {
                    throw runtime_error("Error semantico: el literal de cadena no cabe en el arreglo '" + nombre + "'");
                }

                auto& ambito = tablaSimbolos.back();
                ambito[nombre].inicializado = true;
            } else {
                // Endurecer: char solo acepta literal de carácter, char[] solo literal de cadena
                if (tipoVar == TipoVariable::CADENA) {
                    // char[] = ...
                    TipoVariable tipoInitVar = visitarExpresionTipo(initNodo->hijos[0]);
                    if (tipoInitVar != TipoVariable::CADENA)
                        throw runtime_error("Error semantico: tipo de inicializador incompatible con la CADENA '" + nombre + "'");
                } else if (tipoVar == TipoVariable::ENTERO && esChar) {
                    // char = ...
                    auto hijo = initNodo->hijos[0];
                    if (hijo->etiqueta.find("Caracter: ") == 0) {
                        // En C estándar, una constante de carácter puede ser multi-carácter (ej. '10'),
                        // su valor es implementación-definido y normalmente el compilador emitiría un warning.
                        // Aquí lo aceptamos como inicializador válido de char.
                    } else {
                        throw runtime_error("Error semantico: un char debe inicializarse con un literal entre comillas simples");
                    }
                } else {
                    // int/float normales
                    TipoVariable tipoInitVar = visitarExpresionTipo(initNodo->hijos[0]);
                    TipoNumerico tipoInit = obtenerTipoNumericoDeTipoVariable(tipoInitVar);
                    TipoNumerico tipoVarNum = obtenerTipoNumericoDeTipoVariable(tipoVar);
                    if (!sonCompatibles(tipoVarNum, tipoInit, "="))
                        throw IncompatibilidadTiposNumericosError("Tipo de inicializador incompatible con la variable '" + nombre + "'");
                    verificarAsignacionNumericaEstrica(tipoVarNum, tipoInit, "inicializacion de '" + nombre + "'");
                }
                // Marcar como inicializada
                auto& ambito = tablaSimbolos.back();
                ambito[nombre].inicializado = true;
            }
        }
    }
}

// Visita de funcion
void Semantico::visitarFuncion(shared_ptr<NodoAST> nodo) {
    // Hijos: [Tipo, Nombre, Parametros, Bloque]
    if (nodo->hijos.size() != 4) throw runtime_error("Nodo Funcion mal formado");

    // Obtener tipo de retorno
    string tipoLex = nodo->hijos[0]->etiqueta; // "Tipo: int"
    size_t pos = tipoLex.find(": ");
    string tipoStr = (pos != string::npos) ? tipoLex.substr(pos + 2) : "";
    TipoVariable tipoRet;
    if (tipoStr == "int") tipoRet = TipoVariable::ENTERO;
    else if (tipoStr == "float") tipoRet = TipoVariable::FLOTANTE;
    else if (tipoStr == "char") tipoRet = TipoVariable::ENTERO;
    else tipoRet = TipoVariable::DESCONOCIDO; // void/no soportado

    // Obtener nombre (no lo usamos mucho por ahora)
    string nombreLex = nodo->hijos[1]->etiqueta;
    pos = nombreLex.find(": ");
    string nombreFunc = (pos != string::npos) ? nombreLex.substr(pos + 2) : "";

    // Entrar en nuevo ambito para la funcion (parametros y cuerpo)
    entrarAmbito();

    // Procesar parametros (si los hay)
    auto paramsNodo = nodo->hijos[2];
    for (auto& param : paramsNodo->hijos) {
        // Cada parametro es un nodo "Parametro" con hijos [Tipo, Nombre]
        if (param->hijos.size() != 2) throw runtime_error("Parametro mal formado");
        string tipoParamLex = param->hijos[0]->etiqueta;
        pos = tipoParamLex.find(": ");
        string tipoParamStr = (pos != string::npos) ? tipoParamLex.substr(pos + 2) : "";
        TipoVariable tipoParam;
        if (tipoParamStr == "int") tipoParam = TipoVariable::ENTERO;
        else if (tipoParamStr == "float") tipoParam = TipoVariable::FLOTANTE;
        else tipoParam = TipoVariable::DESCONOCIDO;

        string idParamLex = param->hijos[1]->etiqueta;
        pos = idParamLex.find(": ");
        string nombreParam = (pos != string::npos) ? idParamLex.substr(pos + 2) : "";

        Simbolo simParam(tipoParam, false, 0);
        declararVariable(nombreParam, simParam);
    }

    // Establecer tipo de retorno actual
    tipoFuncionActual.push_back(tipoRet);

    // Visitar cuerpo (bloque)
    visitarBloque(nodo->hijos[3]);

    // Salir de la funcion
    tipoFuncionActual.pop_back();
    salirAmbito();
}

// Visita de llamada a funcion
void Semantico::visitarLlamadaFuncion(shared_ptr<NodoAST> nodo) {
    // Hijos: [Nombre, Argumentos]
    if (nodo->hijos.size() < 2) throw runtime_error("Llamada a funcion mal formada");
    // Por ahora solo verificamos que los argumentos sean expresiones validas
    auto argsNodo = nodo->hijos[1];
    for (auto& arg : argsNodo->hijos) {
        visitarExpresionTipo(arg);
    }
    // En un analisis mas completo se verificaría que la funcion este declarada
    // y que los tipos de argumentos coincidan con los parametros.
}

// Visita de expresion (devuelve el tipo de la expresion)
TipoVariable Semantico::visitarExpresionTipo(shared_ptr<NodoAST> nodo, bool requiereInicializado) {
    if (!nodo) return TipoVariable::DESCONOCIDO;

    const string& etiq = nodo->etiqueta;

    if (etiq.find("Numero: ") == 0) {
        string lex = etiq.substr(8);
        return (obtenerTipoNumericoDeLiteral(lex) == TipoNumerico::FLOTANTE) ? TipoVariable::FLOTANTE : TipoVariable::ENTERO;
    }

    if (etiq.find("Cadena: ") == 0) {
        return TipoVariable::CADENA;
    }

    if (etiq.find("Caracter: ") == 0) {
        // En C, un literal de caracter es un entero, pero debe representar 1 caracter lógico.
        // (Ej válidos: 'a', '\\n', '\\x41'. Ej inválido: '10')
        string lex = etiq.substr(10);
        if (longitudCaracterLiteral(lex) != 1) {
            throw runtime_error("Error semantico: literal de caracter invalido; debe contener exactamente 1 caracter entre comillas simples");
        }
        return TipoVariable::ENTERO;
    }

    if (etiq.find("Identificador: ") == 0) {
        string nombre = etiq.substr(15);
        Simbolo* sim = buscarVariable(nombre);
        if (!sim)
            throw runtime_error("Error semantico: variable '" + nombre + "' no declarada");
        if (requiereInicializado && !sim->inicializado)
            throw ValorIndefinidoError("variable '" + nombre + "' usada sin inicializar.");
        return sim->tipo;
    }

    if (etiq == "LlamadaFuncion") {
        visitarLlamadaFuncion(nodo);
        return TipoVariable::ENTERO; // asumimos int
    }

    if (etiq == "Indice") {
        if (nodo->hijos.size() != 2) throw runtime_error("Nodo Índice mal formado");
        string baseEtiq = nodo->hijos[0]->etiqueta;
        if (baseEtiq.find("Identificador: ") != 0)
            throw runtime_error("Se esperaba un identificador de arreglo");
        string nombreArr = baseEtiq.substr(15);
        Simbolo* sim = buscarVariable(nombreArr);
        if (!sim)
            throw runtime_error("Error semantico: variable '" + nombreArr + "' no declarada");
        if (!sim->esArreglo)
            throw runtime_error("Error semantico: variable '" + nombreArr + "' no es un arreglo");
        if (requiereInicializado && !sim->inicializado)
            throw ValorIndefinidoError("arreglo '" + nombreArr + "' usado sin inicializar.");

        TipoVariable tipoIndice = visitarExpresionTipo(nodo->hijos[1]);
        if (tipoIndice != TipoVariable::ENTERO)
            throw runtime_error("Error semantico: el índice de un arreglo debe ser entero");

        // Indexar una CADENA (char[]) produce un ENTERO (char).
        if (sim->tipo == TipoVariable::CADENA)
            return TipoVariable::ENTERO;
        return sim->tipo;
    }

    if (etiq.find("Operador: ") == 0) {
        string op = etiq.substr(10);
        if (nodo->hijos.size() != 2) throw runtime_error("Operador binario mal formado");
        TipoVariable izq = visitarExpresionTipo(nodo->hijos[0]);
        TipoVariable der = visitarExpresionTipo(nodo->hijos[1]);

        if (op == "+") {
            if (izq == TipoVariable::CADENA || der == TipoVariable::CADENA) {
                if (izq == TipoVariable::CADENA && der == TipoVariable::CADENA)
                    return TipoVariable::CADENA;
                throw ConcatenacionError("no se puede concatenar CADENA con un tipo no cadena.");
            }
            if (!esTipoNumericoVar(izq) || !esTipoNumericoVar(der))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles para operador +");
            return promoverTipoNumericoVar(izq, der);
        }

        if (op == "-" || op == "*" || op == "/") {
            if (!esTipoNumericoVar(izq) || !esTipoNumericoVar(der))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles para operador " + op);
            if (op == "/") {
                verificarDivisionPorCeroSiLiteral(nodo->hijos[1]);
            }
            return promoverTipoNumericoVar(izq, der);
        }

        if (op == "%") {
            if (izq != TipoVariable::ENTERO || der != TipoVariable::ENTERO)
                throw IncompatibilidadTiposNumericosError("El operador % requiere operandos ENTERO");
            return TipoVariable::ENTERO;
        }

        if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=") {
            if (!esTipoNumericoVar(izq) || !esTipoNumericoVar(der))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles para operador relacional");
            return TipoVariable::ENTERO;
        }

        if (op == "&&" || op == "||") {
            if (!esTipoNumericoVar(izq) || !esTipoNumericoVar(der))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles para operador logico");
            return TipoVariable::ENTERO;
        }

        throw runtime_error("Operador binario no soportado: " + op);
    }

    if (etiq.find("Asignacion: ") == 0) {
        string op = etiq.substr(12);
        if (nodo->hijos.size() != 2) throw runtime_error("Asignacion mal formada");

        string izqEtiq = nodo->hijos[0]->etiqueta;
        bool lValido = (izqEtiq.find("Identificador: ") == 0) || (izqEtiq == "Indice");
        if (!lValido)
            throw runtime_error("Error semantico: el lado izquierdo de la asignacion no es modificable");

        TipoVariable tipoIzq = visitarExpresionTipo(nodo->hijos[0], false);
        TipoVariable tipoDer = visitarExpresionTipo(nodo->hijos[1], true);

        if (tipoIzq == TipoVariable::CADENA) {
            if (tipoDer != TipoVariable::CADENA)
                throw runtime_error("Error semantico: asignacion incompatible a CADENA");
        } else {
            TipoNumerico izqNum = obtenerTipoNumericoDeTipoVariable(tipoIzq);
            TipoNumerico derNum = obtenerTipoNumericoDeTipoVariable(tipoDer);
            if (!sonCompatibles(izqNum, derNum, "="))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles en asignacion");
            verificarAsignacionNumericaEstrica(izqNum, derNum, "asignacion");
            if (op != "=") {
                string opBase = op.substr(0, op.size() - 1);
                if (!sonCompatibles(izqNum, derNum, opBase))
                    throw IncompatibilidadTiposNumericosError("Tipos incompatibles en asignacion compuesta");
            }
        }

        if (izqEtiq.find("Identificador: ") == 0) {
            string nombre = izqEtiq.substr(15);
            Simbolo* sim = buscarVariable(nombre);
            if (sim) sim->inicializado = true;
        } else if (izqEtiq == "Indice" && !nodo->hijos.empty() && nodo->hijos[0]->hijos.size() >= 1) {
            string baseEtiq = nodo->hijos[0]->hijos[0]->etiqueta;
            if (baseEtiq.find("Identificador: ") == 0) {
                string nombre = baseEtiq.substr(15);
                Simbolo* sim = buscarVariable(nombre);
                if (sim) sim->inicializado = true;
            }
        }

        return tipoIzq;
    }

    if (etiq.find("Unario: ") == 0) {
        string op = etiq.substr(8);
        if (nodo->hijos.size() != 1) throw runtime_error("Operador unario mal formado");
        TipoVariable tipoOp = visitarExpresionTipo(nodo->hijos[0]);
        if (op == "!") {
            if (!esTipoNumericoVar(tipoOp))
                throw IncompatibilidadTiposNumericosError("El operador ! requiere tipo numerico");
            return TipoVariable::ENTERO;
        }
        if (op == "+" || op == "-") {
            if (!esTipoNumericoVar(tipoOp))
                throw IncompatibilidadTiposNumericosError("El operador unario requiere tipo numerico");
            return tipoOp;
        }
        // ++/-- prefijos: el parser los representa como Unario tambien.
        if (op == "++" || op == "--") {
            if (!esTipoNumericoVar(tipoOp))
                throw IncompatibilidadTiposNumericosError("++/-- requiere tipo numerico");
            return tipoOp;
        }
        throw runtime_error("Operador unario no soportado: " + op);
    }

    if (etiq.find("Postfijo: ") == 0) {
        if (nodo->hijos.size() != 1) throw runtime_error("Operador postfijo mal formado");
        string opEtiq = nodo->hijos[0]->etiqueta;
        bool lValido = (opEtiq.find("Identificador: ") == 0) || (opEtiq == "Indice");
        if (!lValido)
            throw runtime_error("Error semantico: el operando de ++/-- debe ser modificable");
        TipoVariable tipoOp = visitarExpresionTipo(nodo->hijos[0], false);
        if (!esTipoNumericoVar(tipoOp))
            throw IncompatibilidadTiposNumericosError("Operador postfijo requiere tipo numerico");
        if (opEtiq.find("Identificador: ") == 0) {
            string nombre = opEtiq.substr(15);
            Simbolo* sim = buscarVariable(nombre);
            if (sim) sim->inicializado = true;
        }
        return tipoOp;
    }

    if (etiq.empty() && nodo->hijos.size() == 1) {
        return visitarExpresionTipo(nodo->hijos[0], requiereInicializado);
    }

    throw runtime_error("Expresion no reconocida: " + etiq);
}

// Visita de expresion (devuelve el tipo numerico resultante)
TipoNumerico Semantico::visitarExpresion(shared_ptr<NodoAST> nodo) {
    TipoVariable tipoVar = visitarExpresionTipo(nodo);
    switch (tipoVar) {
        case TipoVariable::ENTERO: return TipoNumerico::ENTERO;
        case TipoVariable::FLOTANTE: return TipoNumerico::FLOTANTE;
        default: throw IncompatibilidadTiposNumericosError("Se esperaba una expresion numerica");
    }
}

static bool esTipoNumericoVar(TipoVariable tipo) {
    return tipo == TipoVariable::ENTERO || tipo == TipoVariable::FLOTANTE;
}

static TipoVariable promoverTipoNumericoVar(TipoVariable izq, TipoVariable der) {
    if (!esTipoNumericoVar(izq) || !esTipoNumericoVar(der)) return TipoVariable::DESCONOCIDO;
    if (izq == TipoVariable::FLOTANTE || der == TipoVariable::FLOTANTE) return TipoVariable::FLOTANTE;
    return TipoVariable::ENTERO;
}

static void verificarDivisionPorCeroSiLiteral(const shared_ptr<NodoAST>& rhs) {
    if (!rhs) return;
    if (rhs->etiqueta.find("Numero: ") != 0) return;
    string lex = rhs->etiqueta.substr(8);
    try {
        double v = stod(lex);
        if (v == 0.0) throw DivisionPorCeroError();
    } catch (const invalid_argument&) {
        // Ignorar: literal no parseable.
    } catch (const out_of_range&) {
        // Ignorar: fuera de rango.
    }
}