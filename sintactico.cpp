#include "sintactico.h"

Sintactico::Sintactico(Lexico& lexico) : lexico(lexico) {
    avanzar();
}

void Sintactico::analizar() {
    programa();
    if (actual.getTipo() != token::FIN) {
        throw errorSintactico("Tokens restantes después del análisis");
    }
}

void Sintactico::avanzar() {
    actual = lexico.siguiente();
}

void Sintactico::coincidir(token::Tipo tipo) {
    if (actual.getTipo() != tipo) {
        throw errorSintactico("Se esperaba " + token::tipoToString(tipo) + ", se encontró " + token::tipoToString(actual.getTipo()));
    }
    avanzar();
}

void Sintactico::coincidir(const string& lexema) {
    if (actual.getLexema() != lexema) {
        throw errorSintactico("Se esperaba '" + lexema + "', se encontró '" + actual.getLexema() + "'");
    }
    avanzar();
}

void Sintactico::programa() {
    while (actual.getTipo() != token::FIN) {
        sentencia();
    }
}

void Sintactico::sentencia() {
    if (esTipoDato(actual)) {
        declaracion();
    } else if (actual.getTipo() == token::IDENTIFICADOR) {
        asignacion();
    } else {
        throw errorSintactico("Sentencia inválida");
    }
}

void Sintactico::declaracion() {
    // Asumir que el tipo es PALABRA_RESERVADA
    coincidir(token::PALABRA_RESERVADA);
    coincidir(token::IDENTIFICADOR);
    coincidir(";");
}

void Sintactico::asignacion() {
    coincidir(token::IDENTIFICADOR);
    coincidir("=");
    expresion();
    coincidir(";");
}

void Sintactico::expresion() {
    termino();
    while (actual.getLexema() == "+" || actual.getLexema() == "-") {
        coincidir(actual.getLexema());
        termino();
    }
}

void Sintactico::termino() {
    factor();
    while (actual.getLexema() == "*" || actual.getLexema() == "/") {
        coincidir(actual.getLexema());
        factor();
    }
}

void Sintactico::factor() {
    if (actual.getTipo() == token::IDENTIFICADOR || actual.getTipo() == token::NUMERO) {
        avanzar();
    } else if (actual.getLexema() == "(") {
        coincidir("(");
        expresion();
        coincidir(")");
    } else {
        throw errorSintactico("Factor inválido");
    }
}

bool Sintactico::esTipoDato(const token& tok) const {
    if (tok.getTipo() == token::PALABRA_RESERVADA) {
        string lex = tok.getLexema();
        return lex == "int" || lex == "float" || lex == "char";
    }
    return false;
}

runtime_error Sintactico::errorSintactico(const string& mensaje) const {
    return runtime_error("Error sintáctico en línea " + to_string(actual.getLinea()) + ", columna " + to_string(actual.getColumna()) + ": " + mensaje);
}
