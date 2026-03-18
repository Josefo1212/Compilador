#include "sintactico.h"

// Dibuja el arbol en consola de forma recursiva.
void NodoAST::imprimir(ostream& out, const string& prefijo, bool esUltimo) const {
    out << prefijo << (esUltimo ? "`-- " : "|-- ") << etiqueta << '\n';
    string siguientePrefijo = prefijo + (esUltimo ? "    " : "|   ");
    for (size_t i = 0; i < hijos.size(); ++i) {
        hijos[i]->imprimir(out, siguientePrefijo, i + 1 == hijos.size());
    }
}

// Inicializa el parser y carga el primer token.
Sintactico::Sintactico(Lexico& lexico) : lexico(lexico) {
    avanzar();
}

// Inicia el analisis completo y guarda la raiz del arbol.
void Sintactico::analizar() {
    arbol = programa();
    if (actual.getTipo() != token::FIN) {
        throw errorSintactico("Tokens restantes después del analisis");
    }
}

// Devuelve la raiz del arbol sintactico.
shared_ptr<NodoAST> Sintactico::getArbol() const {
    return arbol;
}

// Imprime el arbol sintactico ya construido.
void Sintactico::imprimirArbol(ostream& out) const {
    if (!arbol) {
        out << "<arbol vacio>\n";
        return;
    }

    out << "Arbol Sintactico Abstracto\n";
    out << "========================\n";
    out << "`-- " << arbol->etiqueta << '\n';
    for (size_t i = 0; i < arbol->hijos.size(); ++i) {
        arbol->hijos[i]->imprimir(out, "    ", i + 1 == arbol->hijos.size());
    }
}

// Avanza al siguiente token del lexico.
void Sintactico::avanzar() {
    actual = lexico.siguiente();
}

// Mira el siguiente token sin consumirlo.
token Sintactico::siguienteToken() const {
    return const_cast<Lexico&>(lexico).peek();
}

// Crea un nodo simple del AST.
shared_ptr<NodoAST> Sintactico::crearNodo(const string& etiqueta) const {
    return make_shared<NodoAST>(etiqueta);
}

// Crea un nodo a partir de un token.
shared_ptr<NodoAST> Sintactico::crearNodoToken(const string& etiqueta, const token& tok) const {
    return crearNodo(etiqueta + ": " + tok.getLexema());
}

// Crea un nodo binario con hijo izquierdo y derecho.
shared_ptr<NodoAST> Sintactico::crearNodoBinario(const string& etiqueta, const shared_ptr<NodoAST>& izquierdo, const shared_ptr<NodoAST>& derecho) const {
    shared_ptr<NodoAST> nodo = crearNodo(etiqueta);
    nodo->agregarHijo(izquierdo);
    nodo->agregarHijo(derecho);
    return nodo;
}

// Verifica que el token actual sea del tipo esperado.
void Sintactico::coincidir(token::Tipo tipo) {
    if (actual.getTipo() != tipo) {
        throw errorSintactico("Se esperaba " + token::tipoToString(tipo) + ", se encontró " + token::tipoToString(actual.getTipo()));
    }
    avanzar();
}

// Verifica que el lexema actual sea el esperado.
void Sintactico::coincidir(const string& lexema) {
    if (actual.getLexema() != lexema) {
        throw errorSintactico("Se esperaba '" + lexema + "', se encontro '" + actual.getLexema() + "'");
    }
    avanzar();
}

// Analiza el programa completo.
shared_ptr<NodoAST> Sintactico::programa() {
    shared_ptr<NodoAST> nodoPrograma = crearNodo("Programa");
    while (actual.getTipo() != token::FIN) {
        if (actual.getLexema() == "#") {
            nodoPrograma->agregarHijo(directivaPreprocesador());
        } else if (esTipoDato(actual) && siguienteToken().getTipo() == token::IDENTIFICADOR) {
            nodoPrograma->agregarHijo(definicionFuncion());
        } else {
            nodoPrograma->agregarHijo(sentencia());
        }
    }
    return nodoPrograma;
}

// Analiza una definicion de funcion.
shared_ptr<NodoAST> Sintactico::definicionFuncion() {
    shared_ptr<NodoAST> nodoFuncion = crearNodo("Funcion");

    token tipo = actual;
    coincidir(token::PALABRA_RESERVADA);
    nodoFuncion->agregarHijo(crearNodoToken("Tipo", tipo));

    token identificador = actual;
    coincidir(token::IDENTIFICADOR);
    nodoFuncion->agregarHijo(crearNodoToken("Nombre", identificador));

    coincidir("(");
    nodoFuncion->agregarHijo(listaParametros());
    coincidir(")");
    nodoFuncion->agregarHijo(bloque());

    return nodoFuncion;
}

// Analiza una directiva simple de preprocesador.
shared_ptr<NodoAST> Sintactico::directivaPreprocesador() {
    shared_ptr<NodoAST> nodoInclude = crearNodo("DirectivaPreprocesador");
    coincidir("#");

    if (actual.getTipo() != token::IDENTIFICADOR || actual.getLexema() != "include") {
        throw errorSintactico("Directiva de preprocesador no soportada");
    }

    token directiva = actual;
    coincidir(token::IDENTIFICADOR);
    nodoInclude->agregarHijo(crearNodoToken("Directiva", directiva));

    if (actual.getLexema() == "<") {
        string encabezado = "<";
        coincidir("<");
        while (actual.getTipo() != token::FIN && actual.getLexema() != ">") {
            encabezado += actual.getLexema();
            avanzar();
        }
        coincidir(">");
        encabezado += ">";
        nodoInclude->agregarHijo(crearNodo("Encabezado: " + encabezado));
        return nodoInclude;
    }

    if (actual.getTipo() == token::CADENA) {
        token encabezado = actual;
        coincidir(token::CADENA);
        nodoInclude->agregarHijo(crearNodoToken("Encabezado", encabezado));
        return nodoInclude;
    }

    throw errorSintactico("Se esperaba un encabezado después de include");
}

// Decide que tipo de sentencia viene.
shared_ptr<NodoAST> Sintactico::sentencia() {
    if (actual.getLexema() == ";") {
        coincidir(";");
        return crearNodo("SentenciaVacia");
    } else if (actual.getLexema() == "{") {
        return bloque();
    } else if (esTipoDato(actual)) {
        return declaracion();
    } else if (esIf(actual)) {
        return sentenciaIf();
    } else if (esWhile(actual)) {
        return sentenciaWhile();
    } else if (esFor(actual)) {
        return sentenciaFor();
    } else if (esDo(actual)) {
        return sentenciaDoWhile();
    } else if (esReturn(actual)) {
        return sentenciaReturn();
    } else if (actual.getTipo() == token::IDENTIFICADOR) {
        return sentenciaExpresion();
    } else {
        throw errorSintactico("Sentencia invalida");
    }
}

// Analiza una declaracion terminada en punto y coma.
shared_ptr<NodoAST> Sintactico::declaracion() {
    shared_ptr<NodoAST> nodoDeclaracion = declaracionSinPuntoComa();
    coincidir(";");
    return nodoDeclaracion;
}

// Analiza una declaracion sin consumir el punto y coma final.
shared_ptr<NodoAST> Sintactico::declaracionSinPuntoComa() {
    shared_ptr<NodoAST> nodoDeclaracion = crearNodo("Declaracion");

    token tipo = actual;
    coincidir(token::PALABRA_RESERVADA);
    nodoDeclaracion->agregarHijo(crearNodoToken("Tipo", tipo));

    token identificador = actual;
    coincidir(token::IDENTIFICADOR);
    nodoDeclaracion->agregarHijo(crearNodoToken("Identificador", identificador));

    if (actual.getLexema() == "[") {
        coincidir("[");
        shared_ptr<NodoAST> nodoArreglo = crearNodo("Arreglo");
        if (actual.getTipo() == token::NUMERO) {
            token tamano = actual;
            coincidir(token::NUMERO);
            nodoArreglo->agregarHijo(crearNodoToken("Tamano", tamano));
        }
        coincidir("]");
        nodoDeclaracion->agregarHijo(nodoArreglo);
    }

    if (actual.getLexema() == "=") {
        coincidir("=");
        shared_ptr<NodoAST> nodoInit = crearNodo("Inicializador");
        nodoInit->agregarHijo(inicializador());
        nodoDeclaracion->agregarHijo(nodoInit);
    }

    return nodoDeclaracion;
}

// Analiza una sentencia basada en expresion.
shared_ptr<NodoAST> Sintactico::sentenciaExpresion() {
    shared_ptr<NodoAST> nodoSentencia = crearNodo("SentenciaExpresion");
    nodoSentencia->agregarHijo(expresion());
    coincidir(";");
    return nodoSentencia;
}

// Analiza una sentencia return.
shared_ptr<NodoAST> Sintactico::sentenciaReturn() {
    shared_ptr<NodoAST> nodoReturn = crearNodo("Return");
    coincidir(token::PALABRA_RESERVADA);
    if (actual.getLexema() != ";") {
        nodoReturn->agregarHijo(expresion());
    }
    coincidir(";");
    return nodoReturn;
}

// Analiza una sentencia if con else opcional.
shared_ptr<NodoAST> Sintactico::sentenciaIf() {
    shared_ptr<NodoAST> nodoIf = crearNodo("If");
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    shared_ptr<NodoAST> condicion = crearNodo("Condicion");
    condicion->agregarHijo(expresion());
    coincidir(")");
    nodoIf->agregarHijo(condicion);

    shared_ptr<NodoAST> entonces = crearNodo("Entonces");
    entonces->agregarHijo(sentencia());
    nodoIf->agregarHijo(entonces);

    if (esElse(actual)) {
        coincidir(token::PALABRA_RESERVADA);
        shared_ptr<NodoAST> sino = crearNodo("Sino");
        sino->agregarHijo(sentencia());
        nodoIf->agregarHijo(sino);
    }

    return nodoIf;
}

// Analiza una sentencia while.
shared_ptr<NodoAST> Sintactico::sentenciaWhile() {
    shared_ptr<NodoAST> nodoWhile = crearNodo("While");
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    shared_ptr<NodoAST> condicion = crearNodo("Condicion");
    condicion->agregarHijo(expresion());
    coincidir(")");
    nodoWhile->agregarHijo(condicion);

    shared_ptr<NodoAST> cuerpo = crearNodo("Cuerpo");
    cuerpo->agregarHijo(sentencia());
    nodoWhile->agregarHijo(cuerpo);

    return nodoWhile;
}

// Analiza una sentencia for.
shared_ptr<NodoAST> Sintactico::sentenciaFor() {
    shared_ptr<NodoAST> nodoFor = crearNodo("For");
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");

    shared_ptr<NodoAST> inicializacion = crearNodo("Inicializacion");
    if (esTipoDato(actual)) {
        inicializacion->agregarHijo(declaracionSinPuntoComa());
    } else if (actual.getLexema() != ";") {
        inicializacion->agregarHijo(expresion());
    }
    coincidir(";");
    nodoFor->agregarHijo(inicializacion);

    shared_ptr<NodoAST> condicion = crearNodo("Condicion");
    if (actual.getLexema() != ";") {
        condicion->agregarHijo(expresion());
    }
    coincidir(";");
    nodoFor->agregarHijo(condicion);

    shared_ptr<NodoAST> actualizacion = crearNodo("Actualizacion");
    if (actual.getLexema() != ")") {
        actualizacion->agregarHijo(expresion());
    }
    coincidir(")");
    nodoFor->agregarHijo(actualizacion);

    shared_ptr<NodoAST> cuerpo = crearNodo("Cuerpo");
    cuerpo->agregarHijo(sentencia());
    nodoFor->agregarHijo(cuerpo);

    return nodoFor;
}

// Analiza una sentencia do while.
shared_ptr<NodoAST> Sintactico::sentenciaDoWhile() {
    shared_ptr<NodoAST> nodoDoWhile = crearNodo("DoWhile");
    coincidir(token::PALABRA_RESERVADA);

    shared_ptr<NodoAST> cuerpo = crearNodo("Cuerpo");
    cuerpo->agregarHijo(sentencia());
    nodoDoWhile->agregarHijo(cuerpo);

    if (!esWhile(actual)) {
        throw errorSintactico("Se esperaba while despues de do");
    }

    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    shared_ptr<NodoAST> condicion = crearNodo("Condicion");
    condicion->agregarHijo(expresion());
    coincidir(")");
    coincidir(";");
    nodoDoWhile->agregarHijo(condicion);

    return nodoDoWhile;
}

// Analiza una llamada a funcion.
shared_ptr<NodoAST> Sintactico::llamadaFuncion(bool requierePuntoComa) {
    shared_ptr<NodoAST> nodoLlamada = crearNodo("LlamadaFuncion");

    token identificador = actual;
    coincidir(token::IDENTIFICADOR);
    nodoLlamada->agregarHijo(crearNodoToken("Nombre", identificador));

    coincidir("(");
    nodoLlamada->agregarHijo(listaArgumentos());
    coincidir(")");

    if (requierePuntoComa) {
        coincidir(";");
    }

    return nodoLlamada;
}

// Analiza la lista de argumentos de una llamada.
shared_ptr<NodoAST> Sintactico::listaArgumentos() {
    shared_ptr<NodoAST> nodoArgumentos = crearNodo("Argumentos");
    if (actual.getLexema() == ")") {
        return nodoArgumentos;
    }

    nodoArgumentos->agregarHijo(expresion());
    while (actual.getLexema() == ",") {
        coincidir(",");
        nodoArgumentos->agregarHijo(expresion());
    }

    return nodoArgumentos;
}

// Punto de entrada para analizar expresiones.
shared_ptr<NodoAST> Sintactico::expresion() {
    return expresionAsignacion();
}

// Analiza expresiones con operadores de asignacion.
shared_ptr<NodoAST> Sintactico::expresionAsignacion() {
    shared_ptr<NodoAST> izquierdo = expresionOR();
    if (esOperadorAsignacion(actual.getLexema())) {
        token operador = actual;
        coincidir(actual.getLexema());
        shared_ptr<NodoAST> derecho = expresionAsignacion();
        return crearNodoBinario("Asignacion: " + operador.getLexema(), izquierdo, derecho);
    }

    return izquierdo;
}

// Analiza expresiones con operador OR logico.
shared_ptr<NodoAST> Sintactico::expresionOR() {
    shared_ptr<NodoAST> izquierdo = expresionAND();
    while (actual.getLexema() == "||") {
        token operador = actual;
        coincidir("||");
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionAND());
    }

    return izquierdo;
}

// Analiza expresiones con operador AND logico.
shared_ptr<NodoAST> Sintactico::expresionAND() {
    shared_ptr<NodoAST> izquierdo = expresionIgualdad();
    while (actual.getLexema() == "&&") {
        token operador = actual;
        coincidir("&&");
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionIgualdad());
    }

    return izquierdo;
}

// Analiza expresiones de igualdad y diferencia.
shared_ptr<NodoAST> Sintactico::expresionIgualdad() {
    shared_ptr<NodoAST> izquierdo = expresionRelacional();
    while (actual.getLexema() == "==" || actual.getLexema() == "!=") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionRelacional());
    }

    return izquierdo;
}

// Analiza expresiones relacionales.
shared_ptr<NodoAST> Sintactico::expresionRelacional() {
    shared_ptr<NodoAST> izquierdo = expresionAditiva();
    while (actual.getLexema() == "<" || actual.getLexema() == ">" ||
           actual.getLexema() == "<=" || actual.getLexema() == ">=") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionAditiva());
    }

    return izquierdo;
}

// Analiza sumas y restas.
shared_ptr<NodoAST> Sintactico::expresionAditiva() {
    shared_ptr<NodoAST> izquierdo = expresionMultiplicativa();
    while (actual.getLexema() == "+" || actual.getLexema() == "-") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionMultiplicativa());
    }

    return izquierdo;
}

// Analiza multiplicaciones, divisiones y modulo.
shared_ptr<NodoAST> Sintactico::expresionMultiplicativa() {
    shared_ptr<NodoAST> izquierdo = expresionUnaria();
    while (actual.getLexema() == "*" || actual.getLexema() == "/" || actual.getLexema() == "%") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionUnaria());
    }

    return izquierdo;
}

// Analiza operadores unarios.
shared_ptr<NodoAST> Sintactico::expresionUnaria() {
    if (actual.getLexema() == "!" || actual.getLexema() == "+" || actual.getLexema() == "-" ||
        actual.getLexema() == "++" || actual.getLexema() == "--") {
        token operador = actual;
        coincidir(actual.getLexema());
        shared_ptr<NodoAST> nodo = crearNodo("Unario: " + operador.getLexema());
        nodo->agregarHijo(expresionUnaria());
        return nodo;
    }

    return expresionPostfija();
}

// Analiza operadores postfijos como ++ y --.
shared_ptr<NodoAST> Sintactico::expresionPostfija() {
    shared_ptr<NodoAST> nodo = primaria();

    while (actual.getLexema() == "++" || actual.getLexema() == "--") {
        token operador = actual;
        coincidir(actual.getLexema());
        shared_ptr<NodoAST> nodoPostfijo = crearNodo("Postfijo: " + operador.getLexema());
        nodoPostfijo->agregarHijo(nodo);
        nodo = nodoPostfijo;
    }

    return nodo;
}

// Analiza operandos basicos y expresiones entre parentesis.
shared_ptr<NodoAST> Sintactico::primaria() {
    if (actual.getTipo() == token::IDENTIFICADOR) {
        if (siguienteToken().getLexema() == "(") {
            return llamadaFuncion(false);
        }

        token identificador = actual;
        coincidir(token::IDENTIFICADOR);
        shared_ptr<NodoAST> nodo = crearNodoToken("Identificador", identificador);
        while (actual.getLexema() == "[") {
            coincidir("[");
            shared_ptr<NodoAST> indice = expresion();
            coincidir("]");
            shared_ptr<NodoAST> nodoIndice = crearNodo("Indice");
            nodoIndice->agregarHijo(nodo);
            nodoIndice->agregarHijo(indice);
            nodo = nodoIndice;
        }
        return nodo;
    } else if (actual.getTipo() == token::NUMERO || actual.getTipo() == token::CADENA || actual.getTipo() == token::CARACTER) {
        token literal = actual;
        avanzar();
        if (literal.getTipo() == token::NUMERO) return crearNodoToken("Numero", literal);
        if (literal.getTipo() == token::CARACTER) return crearNodoToken("Caracter", literal);
        return crearNodoToken("Cadena", literal);
    } else if (actual.getLexema() == "(") {
        coincidir("(");
        shared_ptr<NodoAST> nodo = expresion();
        coincidir(")");
        return nodo;
    } else {
        throw errorSintactico("Factor invalido");
    }
}

// Verifica si el token actual es un tipo de dato basico.
bool Sintactico::esTipoDato(const token& tok) const {
    if (tok.getTipo() == token::PALABRA_RESERVADA) {
        string lex = tok.getLexema();
        return lex == "int" || lex == "float" || lex == "char";
    }
    return false;
}

// Verifica si el token actual es return.
bool Sintactico::esReturn(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "return";
}

// Verifica si el token actual es if.
bool Sintactico::esIf(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "if";
}

// Verifica si el token actual es else.
bool Sintactico::esElse(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "else";
}

// Verifica si el token actual es while.
bool Sintactico::esWhile(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "while";
}

// Verifica si el token actual es for.
bool Sintactico::esFor(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "for";
}

// Verifica si el token actual es do.
bool Sintactico::esDo(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "do";
}

// Verifica si el lexema es un operador de asignacion.
bool Sintactico::esOperadorAsignacion(const string& lexema) const {
    return lexema == "=" || lexema == "+=" || lexema == "-=" ||
           lexema == "*=" || lexema == "/=" || lexema == "%=";
}

// Analiza la lista de parametros de una funcion.
shared_ptr<NodoAST> Sintactico::listaParametros() {
    shared_ptr<NodoAST> nodoParametros = crearNodo("Parametros");
    if (actual.getLexema() == ")") {
        return nodoParametros;
    }

    shared_ptr<NodoAST> parametro = crearNodo("Parametro");
    token tipo = actual;
    coincidir(token::PALABRA_RESERVADA);
    parametro->agregarHijo(crearNodoToken("Tipo", tipo));

    token nombre = actual;
    coincidir(token::IDENTIFICADOR);
    parametro->agregarHijo(crearNodoToken("Nombre", nombre));
    nodoParametros->agregarHijo(parametro);

    while (actual.getLexema() == ",") {
        coincidir(",");
        shared_ptr<NodoAST> siguienteParametro = crearNodo("Parametro");
        token siguienteTipo = actual;
        coincidir(token::PALABRA_RESERVADA);
        siguienteParametro->agregarHijo(crearNodoToken("Tipo", siguienteTipo));

        token siguienteNombre = actual;
        coincidir(token::IDENTIFICADOR);
        siguienteParametro->agregarHijo(crearNodoToken("Nombre", siguienteNombre));
        nodoParametros->agregarHijo(siguienteParametro);
    }

    return nodoParametros;
}

// Analiza un bloque delimitado por llaves.
shared_ptr<NodoAST> Sintactico::bloque() {
    shared_ptr<NodoAST> nodoBloque = crearNodo("Bloque");
    coincidir("{");
    while (actual.getTipo() != token::FIN && actual.getLexema() != "}") {
        nodoBloque->agregarHijo(sentencia());
    }
    coincidir("}");
    return nodoBloque;
}

// Analiza el valor inicial de una declaracion.
shared_ptr<NodoAST> Sintactico::inicializador() {
    if (actual.getTipo() == token::CADENA) {
        token cadena = actual;
        coincidir(token::CADENA);
        return crearNodoToken("Cadena", cadena);
    }
    if (actual.getTipo() == token::CARACTER) {
        token caracter = actual;
        coincidir(token::CARACTER);
        return crearNodoToken("Caracter", caracter);
    }

    return expresion();
}

// Construye el mensaje de error sintactico.
runtime_error Sintactico::errorSintactico(const string& mensaje) const {
    return runtime_error("Error sintactico en linea " + to_string(actual.getLinea()) + ", columna " + to_string(actual.getColumna()) + ": " + mensaje);
}
