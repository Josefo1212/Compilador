#include "sintactico.h"

void NodoAST::imprimir(ostream& out, const string& prefijo, bool esUltimo) const {
    out << prefijo << (esUltimo ? "`-- " : "|-- ") << etiqueta << '\n';
    string siguientePrefijo = prefijo + (esUltimo ? "    " : "|   ");
    for (size_t i = 0; i < hijos.size(); ++i) {
        hijos[i]->imprimir(out, siguientePrefijo, i + 1 == hijos.size());
    }
}

Sintactico::Sintactico(Lexico& lexico) : lexico(lexico) {
    avanzar();
}

void Sintactico::analizar() {
    arbol = programa();
    if (actual.getTipo() != token::FIN) {
        throw errorSintactico("Tokens restantes después del analisis");
    }
}

shared_ptr<NodoAST> Sintactico::getArbol() const {
    return arbol;
}

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

void Sintactico::avanzar() {
    actual = lexico.siguiente();
}

token Sintactico::siguienteToken() const {
    return const_cast<Lexico&>(lexico).peek();
}

shared_ptr<NodoAST> Sintactico::crearNodo(const string& etiqueta) const {
    return make_shared<NodoAST>(etiqueta);
}

shared_ptr<NodoAST> Sintactico::crearNodoToken(const string& etiqueta, const token& tok) const {
    return crearNodo(etiqueta + ": " + tok.getLexema());
}

shared_ptr<NodoAST> Sintactico::crearNodoBinario(const string& etiqueta, const shared_ptr<NodoAST>& izquierdo, const shared_ptr<NodoAST>& derecho) const {
    shared_ptr<NodoAST> nodo = crearNodo(etiqueta);
    nodo->agregarHijo(izquierdo);
    nodo->agregarHijo(derecho);
    return nodo;
}

void Sintactico::coincidir(token::Tipo tipo) {
    if (actual.getTipo() != tipo) {
        throw errorSintactico("Se esperaba " + token::tipoToString(tipo) + ", se encontró " + token::tipoToString(actual.getTipo()));
    }
    avanzar();
}

void Sintactico::coincidir(const string& lexema) {
    if (actual.getLexema() != lexema) {
        throw errorSintactico("Se esperaba '" + lexema + "', se encontro '" + actual.getLexema() + "'");
    }
    avanzar();
}

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

shared_ptr<NodoAST> Sintactico::declaracion() {
    shared_ptr<NodoAST> nodoDeclaracion = declaracionSinPuntoComa();
    coincidir(";");
    return nodoDeclaracion;
}

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

shared_ptr<NodoAST> Sintactico::sentenciaExpresion() {
    shared_ptr<NodoAST> nodoSentencia = crearNodo("SentenciaExpresion");
    nodoSentencia->agregarHijo(expresion());
    coincidir(";");
    return nodoSentencia;
}

shared_ptr<NodoAST> Sintactico::sentenciaReturn() {
    shared_ptr<NodoAST> nodoReturn = crearNodo("Return");
    coincidir(token::PALABRA_RESERVADA);
    if (actual.getLexema() != ";") {
        nodoReturn->agregarHijo(expresion());
    }
    coincidir(";");
    return nodoReturn;
}

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

shared_ptr<NodoAST> Sintactico::expresion() {
    return expresionAsignacion();
}

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

shared_ptr<NodoAST> Sintactico::expresionOR() {
    shared_ptr<NodoAST> izquierdo = expresionAND();
    while (actual.getLexema() == "||") {
        token operador = actual;
        coincidir("||");
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionAND());
    }

    return izquierdo;
}

shared_ptr<NodoAST> Sintactico::expresionAND() {
    shared_ptr<NodoAST> izquierdo = expresionIgualdad();
    while (actual.getLexema() == "&&") {
        token operador = actual;
        coincidir("&&");
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionIgualdad());
    }

    return izquierdo;
}

shared_ptr<NodoAST> Sintactico::expresionIgualdad() {
    shared_ptr<NodoAST> izquierdo = expresionRelacional();
    while (actual.getLexema() == "==" || actual.getLexema() == "!=") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionRelacional());
    }

    return izquierdo;
}

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

shared_ptr<NodoAST> Sintactico::expresionAditiva() {
    shared_ptr<NodoAST> izquierdo = expresionMultiplicativa();
    while (actual.getLexema() == "+" || actual.getLexema() == "-") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionMultiplicativa());
    }

    return izquierdo;
}

shared_ptr<NodoAST> Sintactico::expresionMultiplicativa() {
    shared_ptr<NodoAST> izquierdo = expresionUnaria();
    while (actual.getLexema() == "*" || actual.getLexema() == "/" || actual.getLexema() == "%") {
        token operador = actual;
        coincidir(actual.getLexema());
        izquierdo = crearNodoBinario("Operador: " + operador.getLexema(), izquierdo, expresionUnaria());
    }

    return izquierdo;
}

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
    } else if (actual.getTipo() == token::NUMERO || actual.getTipo() == token::CADENA) {
        token literal = actual;
        avanzar();
        return crearNodoToken(literal.getTipo() == token::NUMERO ? "Numero" : "Cadena", literal);
    } else if (actual.getLexema() == "(") {
        coincidir("(");
        shared_ptr<NodoAST> nodo = expresion();
        coincidir(")");
        return nodo;
    } else {
        throw errorSintactico("Factor invalido");
    }
}

bool Sintactico::esTipoDato(const token& tok) const {
    if (tok.getTipo() == token::PALABRA_RESERVADA) {
        string lex = tok.getLexema();
        return lex == "int" || lex == "float" || lex == "char";
    }
    return false;
}

bool Sintactico::esReturn(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "return";
}

bool Sintactico::esIf(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "if";
}

bool Sintactico::esElse(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "else";
}

bool Sintactico::esWhile(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "while";
}

bool Sintactico::esFor(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "for";
}

bool Sintactico::esDo(const token& tok) const {
    return tok.getTipo() == token::PALABRA_RESERVADA && tok.getLexema() == "do";
}

bool Sintactico::esOperadorAsignacion(const string& lexema) const {
    return lexema == "=" || lexema == "+=" || lexema == "-=" ||
           lexema == "*=" || lexema == "/=" || lexema == "%=";
}

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

shared_ptr<NodoAST> Sintactico::bloque() {
    shared_ptr<NodoAST> nodoBloque = crearNodo("Bloque");
    coincidir("{");
    while (actual.getTipo() != token::FIN && actual.getLexema() != "}") {
        nodoBloque->agregarHijo(sentencia());
    }
    coincidir("}");
    return nodoBloque;
}

shared_ptr<NodoAST> Sintactico::inicializador() {
    if (actual.getTipo() == token::CADENA) {
        token cadena = actual;
        coincidir(token::CADENA);
        return crearNodoToken("Cadena", cadena);
    }

    return expresion();
}

runtime_error Sintactico::errorSintactico(const string& mensaje) const {
    return runtime_error("Error sintactico en linea " + to_string(actual.getLinea()) + ", columna " + to_string(actual.getColumna()) + ": " + mensaje);
}
