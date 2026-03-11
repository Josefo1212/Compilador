#include "sintactico.h"

Sintactico::Sintactico(Lexico& lexico) : lexico(lexico) {
    avanzar();
}

void Sintactico::analizar() {
    programa();
    if (actual.getTipo() != token::FIN) {
        throw errorSintactico("Tokens restantes después del analisis");
    }
}

void Sintactico::avanzar() {
    actual = lexico.siguiente();
}

token Sintactico::siguienteToken() const {
    return const_cast<Lexico&>(lexico).peek();
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

void Sintactico::programa() {
    while (actual.getTipo() != token::FIN) {
        if (actual.getLexema() == "#") {
            directivaPreprocesador();
        } else if (esTipoDato(actual) && siguienteToken().getTipo() == token::IDENTIFICADOR) {
            definicionFuncion();
        } else {
            sentencia();
        }
    }
}

void Sintactico::definicionFuncion() {
    coincidir(token::PALABRA_RESERVADA);
    coincidir(token::IDENTIFICADOR);
    coincidir("(");
    listaParametros();
    coincidir(")");
    bloque();
}

void Sintactico::directivaPreprocesador() {
    coincidir("#");

    if (actual.getTipo() != token::IDENTIFICADOR || actual.getLexema() != "include") {
        throw errorSintactico("Directiva de preprocesador no soportada");
    }

    coincidir(token::IDENTIFICADOR);

    if (actual.getLexema() == "<") {
        coincidir("<");
        while (actual.getTipo() != token::FIN && actual.getLexema() != ">") {
            avanzar();
        }
        coincidir(">");
        return;
    }

    if (actual.getTipo() == token::CADENA) {
        coincidir(token::CADENA);
        return;
    }

    throw errorSintactico("Se esperaba un encabezado después de include");
}

void Sintactico::sentencia() {
    if (actual.getLexema() == ";") {
        coincidir(";");
    } else if (actual.getLexema() == "{") {
        bloque();
    } else if (esTipoDato(actual)) {
        declaracion();
    } else if (esIf(actual)) {
        sentenciaIf();
    } else if (esWhile(actual)) {
        sentenciaWhile();
    } else if (esFor(actual)) {
        sentenciaFor();
    } else if (esDo(actual)) {
        sentenciaDoWhile();
    } else if (esReturn(actual)) {
        sentenciaReturn();
    } else if (actual.getTipo() == token::IDENTIFICADOR) {
        sentenciaExpresion();
    } else {
        throw errorSintactico("Sentencia invalida");
    }
}

void Sintactico::declaracion() {
    declaracionSinPuntoComa();
    coincidir(";");
}

void Sintactico::declaracionSinPuntoComa() {
    coincidir(token::PALABRA_RESERVADA);
    coincidir(token::IDENTIFICADOR);

    if (actual.getLexema() == "[") {
        coincidir("[");
        if (actual.getTipo() == token::NUMERO) {
            coincidir(token::NUMERO);
        }
        coincidir("]");
    }

    if (actual.getLexema() == "=") {
        coincidir("=");
        inicializador();
    }
}

void Sintactico::sentenciaExpresion() {
    expresion();
    coincidir(";");
}

void Sintactico::sentenciaReturn() {
    coincidir(token::PALABRA_RESERVADA);
    if (actual.getLexema() != ";") {
        expresion();
    }
    coincidir(";");
}

void Sintactico::sentenciaIf() {
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    expresion();
    coincidir(")");
    sentencia();

    if (esElse(actual)) {
        coincidir(token::PALABRA_RESERVADA);
        sentencia();
    }
}

void Sintactico::sentenciaWhile() {
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    expresion();
    coincidir(")");
    sentencia();
}

void Sintactico::sentenciaFor() {
    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");

    if (esTipoDato(actual)) {
        declaracionSinPuntoComa();
    } else if (actual.getLexema() != ";") {
        expresion();
    }
    coincidir(";");

    if (actual.getLexema() != ";") {
        expresion();
    }
    coincidir(";");

    if (actual.getLexema() != ")") {
        expresion();
    }
    coincidir(")");
    sentencia();
}

void Sintactico::sentenciaDoWhile() {
    coincidir(token::PALABRA_RESERVADA);
    sentencia();

    if (!esWhile(actual)) {
        throw errorSintactico("Se esperaba while despues de do");
    }

    coincidir(token::PALABRA_RESERVADA);
    coincidir("(");
    expresion();
    coincidir(")");
    coincidir(";");
}

void Sintactico::llamadaFuncion(bool requierePuntoComa) {
    coincidir(token::IDENTIFICADOR);
    coincidir("(");
    listaArgumentos();
    coincidir(")");

    if (requierePuntoComa) {
        coincidir(";");
    }
}

void Sintactico::listaArgumentos() {
    if (actual.getLexema() == ")") {
        return;
    }

    expresion();
    while (actual.getLexema() == ",") {
        coincidir(",");
        expresion();
    }
}

void Sintactico::expresion() {
    expresionAsignacion();
}

void Sintactico::expresionAsignacion() {
    expresionOR();
    if (esOperadorAsignacion(actual.getLexema())) {
        coincidir(actual.getLexema());
        expresionAsignacion();
    }
}

void Sintactico::expresionOR() {
    expresionAND();
    while (actual.getLexema() == "||") {
        coincidir("||");
        expresionAND();
    }
}

void Sintactico::expresionAND() {
    expresionIgualdad();
    while (actual.getLexema() == "&&") {
        coincidir("&&");
        expresionIgualdad();
    }
}

void Sintactico::expresionIgualdad() {
    expresionRelacional();
    while (actual.getLexema() == "==" || actual.getLexema() == "!=") {
        coincidir(actual.getLexema());
        expresionRelacional();
    }
}

void Sintactico::expresionRelacional() {
    expresionAditiva();
    while (actual.getLexema() == "<" || actual.getLexema() == ">" ||
           actual.getLexema() == "<=" || actual.getLexema() == ">=") {
        coincidir(actual.getLexema());
        expresionAditiva();
    }
}

void Sintactico::expresionAditiva() {
    expresionMultiplicativa();
    while (actual.getLexema() == "+" || actual.getLexema() == "-") {
        coincidir(actual.getLexema());
        expresionMultiplicativa();
    }
}

void Sintactico::expresionMultiplicativa() {
    expresionUnaria();
    while (actual.getLexema() == "*" || actual.getLexema() == "/" || actual.getLexema() == "%") {
        coincidir(actual.getLexema());
        expresionUnaria();
    }
}

void Sintactico::expresionUnaria() {
    if (actual.getLexema() == "!" || actual.getLexema() == "+" || actual.getLexema() == "-" ||
        actual.getLexema() == "++" || actual.getLexema() == "--") {
        coincidir(actual.getLexema());
        expresionUnaria();
        return;
    }

    expresionPostfija();
}

void Sintactico::expresionPostfija() {
    primaria();

    while (actual.getLexema() == "++" || actual.getLexema() == "--") {
        coincidir(actual.getLexema());
    }
}

void Sintactico::primaria() {
    if (actual.getTipo() == token::IDENTIFICADOR) {
        if (siguienteToken().getLexema() == "(") {
            llamadaFuncion(false);
            return;
        }

        coincidir(token::IDENTIFICADOR);
        while (actual.getLexema() == "[") {
            coincidir("[");
            expresion();
            coincidir("]");
        }
    } else if (actual.getTipo() == token::NUMERO || actual.getTipo() == token::CADENA) {
        avanzar();
    } else if (actual.getLexema() == "(") {
        coincidir("(");
        expresion();
        coincidir(")");
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

void Sintactico::listaParametros() {
    if (actual.getLexema() == ")") {
        return;
    }

    coincidir(token::PALABRA_RESERVADA);
    coincidir(token::IDENTIFICADOR);

    while (actual.getLexema() == ",") {
        coincidir(",");
        coincidir(token::PALABRA_RESERVADA);
        coincidir(token::IDENTIFICADOR);
    }
}

void Sintactico::bloque() {
    coincidir("{");
    while (actual.getTipo() != token::FIN && actual.getLexema() != "}") {
        sentencia();
    }
    coincidir("}");
}

void Sintactico::inicializador() {
    if (actual.getTipo() == token::CADENA) {
        coincidir(token::CADENA);
        return;
    }

    expresion();
}

runtime_error Sintactico::errorSintactico(const string& mensaje) const {
    return runtime_error("Error sintactico en linea " + to_string(actual.getLinea()) + ", columna " + to_string(actual.getColumna()) + ": " + mensaje);
}
