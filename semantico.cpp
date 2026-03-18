#include "semantico.h"
#include <iostream>
#include <cctype>
#include <cmath>

using namespace std;

// Constructor: inicia el ámbito global
Semantico::Semantico(shared_ptr<NodoAST> raiz) : raiz(raiz) {
    entrarAmbito();   // ámbito global
}

// Analizar: punto de entrada
void Semantico::analizar() {
    visitar(raiz);
}

// Manejo de ámbitos
void Semantico::entrarAmbito() {
    tablaSimbolos.push_back({});
}

void Semantico::salirAmbito() {
    if (!tablaSimbolos.empty())
        tablaSimbolos.pop_back();
}

// Declarar variable en el ámbito actual
void Semantico::declararVariable(const string& nombre, const Simbolo& sim) {
    if (tablaSimbolos.empty())
        throw runtime_error("Error interno: no hay ámbito para declarar variable");
    auto& ambito = tablaSimbolos.back();
    if (ambito.find(nombre) != ambito.end())
        throw runtime_error("Error semántico: variable '" + nombre + "' ya declarada en este ámbito");
    ambito[nombre] = sim;
}

// Buscar variable desde el ámbito más interno al más externo
Semantico::Simbolo* Semantico::buscarVariable(const string& nombre) {
    for (int i = tablaSimbolos.size() - 1; i >= 0; --i) {
        auto it = tablaSimbolos[i].find(nombre);
        if (it != tablaSimbolos[i].end())
            return &it->second;
    }
    return nullptr;
}

// Obtener el tipo de una variable (lanza excepción si no existe)
TipoVariable Semantico::obtenerTipoVariable(const string& nombre) {
    Simbolo* sim = buscarVariable(nombre);
    if (!sim)
        throw runtime_error("Error semántico: variable '" + nombre + "' no declarada");
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
        throw runtime_error("Error semántico: variable '" + nombre + "' no declarada");
    if (sim->esArreglo)
        throw runtime_error("Error semántico: variable '" + nombre + "' es un arreglo y se requiere índice");
    return obtenerTipoNumericoDeTipoVariable(sim->tipo);
}

// Compatibilidad y tipo resultante en operaciones binarias
bool Semantico::sonCompatibles(TipoNumerico izq, TipoNumerico der, const string& operador) {
    // Para operadores aritméticos, ambos deben ser numéricos
    if (operador == "+" || operador == "-" || operador == "*" || operador == "/") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    // Módulo solo con enteros
    if (operador == "%") {
        return izq == TipoNumerico::ENTERO && der == TipoNumerico::ENTERO;
    }
    // Operadores relacionales y de igualdad: ambos numéricos
    if (operador == "<" || operador == ">" || operador == "<=" || operador == ">=" ||
        operador == "==" || operador == "!=") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    // Operadores lógicos: ambos numéricos (se interpretan como booleanos)
    if (operador == "&&" || operador == "||") {
        return izq != TipoNumerico::NINGUNO && der != TipoNumerico::NINGUNO;
    }
    return false;
}

TipoNumerico Semantico::tipoResultante(TipoNumerico izq, TipoNumerico der, const string& operador) {
    if (operador == "+" || operador == "-" || operador == "*" || operador == "/") {
        // Promoción a flotante si alguno es flotante
        if (izq == TipoNumerico::FLOTANTE || der == TipoNumerico::FLOTANTE)
            return TipoNumerico::FLOTANTE;
        return TipoNumerico::ENTERO;
    }
    if (operador == "%") {
        return TipoNumerico::ENTERO;   // resultado entero
    }
    if (operador == "<" || operador == ">" || operador == "<=" || operador == ">=" ||
        operador == "==" || operador == "!=" || operador == "&&" || operador == "||") {
        // En C, las expresiones relacionales/lógicas son de tipo int
        return TipoNumerico::ENTERO;
    }
    return TipoNumerico::NINGUNO;
}

// Verificación de condición (if, while, for)
void Semantico::verificarCondicion(shared_ptr<NodoAST> condNodo) {
    TipoNumerico tipoCond = visitarExpresion(condNodo);
    if (tipoCond == TipoNumerico::NINGUNO)
        throw runtime_error("Error semántico: la condición no es de tipo numérico");
}

// Visita general del AST
void Semantico::visitar(shared_ptr<NodoAST> nodo) {
    if (!nodo) return;

    string etiq = nodo->etiqueta;

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
        // La expresión puede ser asignación, llamada, etc.
        if (!nodo->hijos.empty())
            visitarExpresion(nodo->hijos[0]);
    }
    else if (etiq == "Return") {
        // Verificar tipo de retorno
        if (tipoFuncionActual.empty())
            throw runtime_error("Error semántico: return fuera de función");
        TipoVariable tipoEsperado = tipoFuncionActual.back();
        if (nodo->hijos.empty()) {
            // return sin expresión: solo válido en funciones void
            if (tipoEsperado != TipoVariable::DESCONOCIDO) // suponemos void como DESCONOCIDO? Mejor usar un tipo VOID
                throw runtime_error("Error semántico: return sin valor en función no void");
        } else {
            TipoNumerico tipoRet = visitarExpresion(nodo->hijos[0]);
            TipoNumerico tipoEspNum = obtenerTipoNumericoDeTipoVariable(tipoEsperado);
            if (!sonCompatibles(tipoEspNum, tipoRet, "="))
                throw IncompatibilidadTiposNumericosError("Tipo de retorno incompatible");
        }
    }
    else if (etiq == "If") {
        // Hijos: [Condicion, Entonces, Sino?]
        if (nodo->hijos.size() < 2) throw runtime_error("Nodo If mal formado");
        verificarCondicion(nodo->hijos[0]);   // Condición
        visitarSentencia(nodo->hijos[1]);     // Entonces
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
        // Inicialización: puede ser declaración o expresión
        if (nodo->hijos[0]->hijos.size() > 0)
            visitar(nodo->hijos[0]->hijos[0]); // el hijo de Inicializacion
        // Condición
        if (nodo->hijos[1]->hijos.size() > 0)
            verificarCondicion(nodo->hijos[1]->hijos[0]);
        // Actualización
        if (nodo->hijos[2]->hijos.size() > 0)
            visitarExpresion(nodo->hijos[2]->hijos[0]);
        // Cuerpo
        visitarSentencia(nodo->hijos[3]);
    }
    else if (etiq == "DoWhile") {
        if (nodo->hijos.size() != 2) throw runtime_error("Nodo DoWhile mal formado");
        visitarSentencia(nodo->hijos[0]); // cuerpo
        verificarCondicion(nodo->hijos[1]); // condición
    }
    else if (etiq == "Bloque") {
        visitarBloque(nodo);
    }
    else if (etiq == "LlamadaFuncion") {
        visitarLlamadaFuncion(nodo);
    }
    else {
        // Si es otro tipo, podría ser una expresión suelta? Normalmente no.
        // Pero por si acaso, la tratamos como expresión
        visitarExpresion(nodo);
    }
}

// Visita de sentencias (wrapper que maneja nodos de sentencia)
void Semantico::visitarSentencia(shared_ptr<NodoAST> nodo) {
    if (!nodo) return;
    // Las sentencias pueden ser bloques, declaraciones, etc.
    // Simplemente llamamos a visitar, que ya discrimina.
    visitar(nodo);
}

// Visita de bloque: nuevo ámbito
void Semantico::visitarBloque(shared_ptr<NodoAST> nodo) {
    entrarAmbito();
    for (auto& hijo : nodo->hijos) {
        visitar(hijo);
    }
    salirAmbito();
}

// Visita de declaración
void Semantico::visitarDeclaracion(shared_ptr<NodoAST> nodo) {
    // Nodo "Declaracion" tiene hijos: [Tipo, Identificador, (opcional Arreglo), (opcional Inicializador)]
    if (nodo->hijos.size() < 2) throw runtime_error("Declaración mal formada");

    // Obtener tipo
    string tipoLex = nodo->hijos[0]->etiqueta; // "Tipo: int"
    size_t pos = tipoLex.find(": ");
    string tipoStr = (pos != string::npos) ? tipoLex.substr(pos + 2) : "";
    TipoVariable tipoVar;
    if (tipoStr == "int") tipoVar = TipoVariable::ENTERO;
    else if (tipoStr == "float") tipoVar = TipoVariable::FLOTANTE;
    else if (tipoStr == "char") tipoVar = TipoVariable::CADENA; // char como cadena simple
    else tipoVar = TipoVariable::DESCONOCIDO;

    // Obtener nombre
    string idLex = nodo->hijos[1]->etiqueta; // "Identificador: x"
    pos = idLex.find(": ");
    string nombre = (pos != string::npos) ? idLex.substr(pos + 2) : "";

    bool esArreglo = false;
    int tamArreglo = 0;
    int indice = 2;
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
        indice++;
    }

    // Declarar variable
    Simbolo sim(tipoVar, esArreglo, tamArreglo);
    declararVariable(nombre, sim);

    // Si hay inicializador
    if (indice < nodo->hijos.size() && nodo->hijos[indice]->etiqueta == "Inicializador") {
        auto initNodo = nodo->hijos[indice];
        if (!initNodo->hijos.empty()) {
            TipoNumerico tipoInit = visitarExpresion(initNodo->hijos[0]);
            TipoNumerico tipoVarNum = obtenerTipoNumericoDeTipoVariable(tipoVar);
            if (!sonCompatibles(tipoVarNum, tipoInit, "="))
                throw IncompatibilidadTiposNumericosError("Tipo de inicializador incompatible con la variable '" + nombre + "'");
            // Marcar como inicializada
            sim.inicializado = true;
            // Actualizar el símbolo en la tabla (ya que sim es copia, necesitamos acceso directo)
            auto& ambito = tablaSimbolos.back();
            ambito[nombre].inicializado = true;
        }
    }
}

// Visita de función
void Semantico::visitarFuncion(shared_ptr<NodoAST> nodo) {
    // Hijos: [Tipo, Nombre, Parametros, Bloque]
    if (nodo->hijos.size() != 4) throw runtime_error("Nodo Función mal formado");

    // Obtener tipo de retorno
    string tipoLex = nodo->hijos[0]->etiqueta; // "Tipo: int"
    size_t pos = tipoLex.find(": ");
    string tipoStr = (pos != string::npos) ? tipoLex.substr(pos + 2) : "";
    TipoVariable tipoRet;
    if (tipoStr == "int") tipoRet = TipoVariable::ENTERO;
    else if (tipoStr == "float") tipoRet = TipoVariable::FLOTANTE;
    else if (tipoStr == "char") tipoRet = TipoVariable::CADENA;
    else tipoRet = TipoVariable::DESCONOCIDO; // void

    // Obtener nombre (no lo usamos mucho por ahora)
    string nombreLex = nodo->hijos[1]->etiqueta;
    pos = nombreLex.find(": ");
    string nombreFunc = (pos != string::npos) ? nombreLex.substr(pos + 2) : "";

    // Entrar en nuevo ámbito para la función (parámetros y cuerpo)
    entrarAmbito();

    // Procesar parámetros (si los hay)
    auto paramsNodo = nodo->hijos[2];
    for (auto& param : paramsNodo->hijos) {
        // Cada parámetro es un nodo "Parametro" con hijos [Tipo, Nombre]
        if (param->hijos.size() != 2) throw runtime_error("Parámetro mal formado");
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

    // Salir de la función
    tipoFuncionActual.pop_back();
    salirAmbito();
}

// Visita de llamada a función
void Semantico::visitarLlamadaFuncion(shared_ptr<NodoAST> nodo) {
    // Hijos: [Nombre, Argumentos]
    if (nodo->hijos.size() < 2) throw runtime_error("Llamada a función mal formada");
    // Por ahora solo verificamos que los argumentos sean expresiones válidas
    auto argsNodo = nodo->hijos[1];
    for (auto& arg : argsNodo->hijos) {
        visitarExpresion(arg);
    }
    // En un análisis más completo se verificaría que la función esté declarada
    // y que los tipos de argumentos coincidan con los parámetros.
}

// Visita de expresión (devuelve el tipo numérico resultante)
TipoNumerico Semantico::visitarExpresion(shared_ptr<NodoAST> nodo) {
    if (!nodo) return TipoNumerico::NINGUNO;

    string etiq = nodo->etiqueta;

    // Literal numérico
    if (etiq.find("Numero: ") == 0) {
        string lex = etiq.substr(8); // después de "Numero: "
        return obtenerTipoNumericoDeLiteral(lex);
    }
    // Literal cadena (no numérico)
    if (etiq.find("Cadena: ") == 0) {
        return TipoNumerico::NINGUNO; // las cadenas no son numéricas
    }
    // Identificador
    if (etiq.find("Identificador: ") == 0) {
        string nombre = etiq.substr(15);
        return obtenerTipoNumericoDeVariable(nombre);
    }
    // Acceso a arreglo: nodo "Indice" con hijos [base, indice]
    if (etiq == "Indice") {
        if (nodo->hijos.size() != 2) throw runtime_error("Nodo Índice mal formado");
        // El primer hijo es el identificador del arreglo
        string baseEtiq = nodo->hijos[0]->etiqueta;
        if (baseEtiq.find("Identificador: ") != 0)
            throw runtime_error("Se esperaba un identificador de arreglo");
        string nombreArr = baseEtiq.substr(15);
        Simbolo* sim = buscarVariable(nombreArr);
        if (!sim)
            throw runtime_error("Error semántico: variable '" + nombreArr + "' no declarada");
        if (!sim->esArreglo)
            throw runtime_error("Error semántico: variable '" + nombreArr + "' no es un arreglo");
        // Verificar que el índice sea entero
        TipoNumerico tipoIndice = visitarExpresion(nodo->hijos[1]);
        if (tipoIndice != TipoNumerico::ENTERO)
            throw runtime_error("Error semántico: el índice de un arreglo debe ser entero");
        // El tipo del elemento es el tipo base del arreglo
        return obtenerTipoNumericoDeTipoVariable(sim->tipo);
    }
    // Operadores binarios
    if (etiq.find("Operador: ") == 0) {
        string op = etiq.substr(10);
        if (nodo->hijos.size() != 2) throw runtime_error("Operador binario mal formado");
        TipoNumerico izq = visitarExpresion(nodo->hijos[0]);
        TipoNumerico der = visitarExpresion(nodo->hijos[1]);
        if (!sonCompatibles(izq, der, op))
            throw IncompatibilidadTiposNumericosError("Tipos incompatibles para operador " + op);
        return tipoResultante(izq, der, op);
    }
    // Asignación
    if (etiq.find("Asignacion: ") == 0) {
        string op = etiq.substr(12); // puede ser "=", "+=", etc.
        if (nodo->hijos.size() != 2) throw runtime_error("Asignación mal formada");
        // El izquierdo debe ser una expresión que pueda estar a la izquierda (L-value)
        // Por simplicidad, verificamos que sea un identificador o índice
        string izqEtiq = nodo->hijos[0]->etiqueta;
        bool lValido = (izqEtiq.find("Identificador: ") == 0) || (izqEtiq == "Indice");
        if (!lValido)
            throw runtime_error("Error semántico: el lado izquierdo de la asignación no es modificable");
        TipoNumerico tipoIzq = visitarExpresion(nodo->hijos[0]); // esto ya verifica existencia
        TipoNumerico tipoDer = visitarExpresion(nodo->hijos[1]);
        if (!sonCompatibles(tipoIzq, tipoDer, "="))
            throw IncompatibilidadTiposNumericosError("Tipos incompatibles en asignación");
        // Si es asignación compuesta, verificar además que el operador sea compatible
        if (op != "=") {
            // Para +=, etc., necesitamos que el tipo resultante de la operación sea compatible con el izquierdo
            // Simulamos la operación: izq op der
            if (!sonCompatibles(tipoIzq, tipoDer, op.substr(0, op.size()-1)))
                throw IncompatibilidadTiposNumericosError("Tipos incompatibles en asignación compuesta");
        }
        // Marcar la variable como inicializada si es una asignación simple a identificador
        if (op == "=" && izqEtiq.find("Identificador: ") == 0) {
            string nombre = izqEtiq.substr(15);
            Simbolo* sim = buscarVariable(nombre);
            if (sim) sim->inicializado = true;
        }
        return tipoIzq; // la asignación devuelve el tipo del izquierdo
    }
    // Operadores unarios
    if (etiq.find("Unario: ") == 0) {
        string op = etiq.substr(8);
        if (nodo->hijos.size() != 1) throw runtime_error("Operador unario mal formado");
        TipoNumerico tipoOp = visitarExpresion(nodo->hijos[0]);
        if (op == "!" ) {
            // ! requiere tipo numérico, devuelve entero
            if (tipoOp == TipoNumerico::NINGUNO)
                throw runtime_error("Error semántico: operador '!' requiere argumento numérico");
            return TipoNumerico::ENTERO;
        } else if (op == "+" || op == "-") {
            // + y - unarios preservan el tipo (promociones a entero? en C se conserva)
            if (tipoOp == TipoNumerico::NINGUNO)
                throw runtime_error("Error semántico: operador unario requiere argumento numérico");
            return tipoOp;
        } else {
            throw runtime_error("Operador unario no soportado: " + op);
        }
    }
    // Operadores postfijos (++ y --)
    if (etiq.find("Postfijo: ") == 0) {
        string op = etiq.substr(10);
        if (nodo->hijos.size() != 1) throw runtime_error("Operador postfijo mal formado");
        // El operando debe ser modificable (L-value)
        string opEtiq = nodo->hijos[0]->etiqueta;
        bool lValido = (opEtiq.find("Identificador: ") == 0) || (opEtiq == "Indice");
        if (!lValido)
            throw runtime_error("Error semántico: el operando de ++/-- debe ser modificable");
        TipoNumerico tipoOp = visitarExpresion(nodo->hijos[0]);
        if (tipoOp == TipoNumerico::NINGUNO)
            throw runtime_error("Error semántico: operador postfijo requiere argumento numérico");
        // Marcar como inicializado si es identificador
        if (opEtiq.find("Identificador: ") == 0) {
            string nombre = opEtiq.substr(15);
            Simbolo* sim = buscarVariable(nombre);
            if (sim) sim->inicializado = true;
        }
        return tipoOp; // el resultado es el mismo tipo
    }
    // Expresiones entre paréntesis: se representan como un nodo con un solo hijo (la expresión interna)
    if (etiq.empty() && nodo->hijos.size() == 1) {
        return visitarExpresion(nodo->hijos[0]);
    }

    throw runtime_error("Expresión no reconocida: " + etiq);
}