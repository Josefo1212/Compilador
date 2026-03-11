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
            token siguiente = siguienteToken();
            coincidir(token::PALABRA_RESERVADA);
            coincidir(token::IDENTIFICADOR);

            if (actual.getLexema() == "(") {
                coincidir("(");
                listaParametros();
                coincidir(")");
                bloque();
            } else {
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
                coincidir(";");
            }
        } else {
            sentencia();
        }
    }
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
    if (actual.getLexema() == "{") {
        bloque();
    } else if (esTipoDato(actual)) {
        declaracion();
    } else if (esReturn(actual)) {
        sentenciaReturn();
    } else if (actual.getTipo() == token::IDENTIFICADOR) {
        token siguiente = siguienteToken();
        if (siguiente.getLexema() == "=") {
            asignacion();
        } else if (siguiente.getLexema() == "(") {
            llamadaFuncion(true);
        } else {
            throw errorSintactico("Sentencia con identificador no soportada");
        }
    } else {
        throw errorSintactico("Sentencia invalida");
    }
}

void Sintactico::declaracion() {
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

    coincidir(";");
}

void Sintactico::asignacion() {
    coincidir(token::IDENTIFICADOR);
    coincidir("=");
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
    if (actual.getTipo() == token::IDENTIFICADOR) {
        if (siguienteToken().getLexema() == "(") {
            llamadaFuncion(false);
        } else {
            avanzar();
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
