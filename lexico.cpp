#include "lexico.h"
#include <istream>
#include <sstream>
#include <cctype>
#include <unordered_set>

using namespace std;

unordered_set<string> Lexico::palabrasReservadas = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
};

Lexico::Lexico() : pos(0), linea(1), columna(1), hasPeek(false) {}

Lexico::Lexico(istream& in) : pos(0), linea(1), columna(1), hasPeek(false) {
	load(in);
}

void Lexico::reset() {
	pos = 0;
	linea = 1;
	columna = 1;
	hasPeek = false;
	tablaSimbolos.clear();
	errores.clear();
}

token Lexico::siguiente() {
	if (hasPeek) {
		hasPeek = false;
		return peekToken;
	}
	return scanToken(pos, linea, columna);
}

token Lexico::peek() {
	if (!hasPeek) {
		size_t p = pos;
		int l = linea;
		int c = columna;
		peekToken = scanToken(p, l, c);
		hasPeek = true;
	}
	return peekToken;
}

void Lexico::load(istream& in) {
	ostringstream ss;
	ss << in.rdbuf();
	source = ss.str();
	reset();
}

char Lexico::currentChar(size_t p) const {
	return (p < source.size()) ? source[p] : '\0';
}

char Lexico::advance(size_t& p, int& l, int& c) {
	if (p >= source.size()) return '\0';
	char ch = source[p++];
	if (ch == '\n') {
		++l;
		c = 1;
	} else {
		++c;
	}
	return ch;
}

void Lexico::skipWhitespaceAndComments(size_t& p, int& l, int& c) {
	for (;;) {
		char ch = currentChar(p);
		if (ch == '\0') return;

		if (isspace(static_cast<unsigned char>(ch))) {
			advance(p, l, c);
			continue;
		}

		if (ch == '/' && currentChar(p + 1) == '/') {
			while (currentChar(p) != '\0' && currentChar(p) != '\n') {
				advance(p, l, c);
			}
			continue;
		}

		if (ch == '/' && currentChar(p + 1) == '*') {
			advance(p, l, c);
			advance(p, l, c);
			while (currentChar(p) != '\0') {
				if (currentChar(p) == '*' && currentChar(p + 1) == '/') {
					advance(p, l, c);
					advance(p, l, c);
					break;
				}
				advance(p, l, c);
			}
			continue;
		}

		return;
	}
}

bool Lexico::isIdentifierStart(char ch) {
	return isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexico::isIdentifierPart(char ch) {
	return isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool Lexico::isTwoCharSymbol(const string& s) {
	return s == "==" || s == "!=" || s == "<=" || s == ">=" ||
	       s == "&&" || s == "||" || s == "++" || s == "--" ||
	       s == "+=" || s == "-=" || s == "*=" || s == "/=" ||
	       s == "%=" || s == "->";
}

bool Lexico::esPalabraReservada(const string& lex) {
    return palabrasReservadas.find(lex) != palabrasReservadas.end();
}

void Lexico::reportarError(const string& mensaje, int linea, int columna) {
    string error = "Línea " + to_string(linea) + ", columna " + to_string(columna) + ": " + mensaje;
    errores.add(error);
}

token Lexico::scanToken(size_t& p, int& l, int& c) {
	skipWhitespaceAndComments(p, l, c);

	if (p >= source.size()) {
		return token(token::FIN, "", l, c);
	}

	int startLine = l;
	int startCol = c;
	char ch = currentChar(p);

	if (isIdentifierStart(ch)) {
		string lex;
		while (isIdentifierPart(currentChar(p))) {
			lex.push_back(advance(p, l, c));
		}
		if (esPalabraReservada(lex)) {
            return token(token::PALABRA_RESERVADA, lex, startLine, startCol);
        } else {
            bool encontrado = false;
            for (int i = 0; i < tablaSimbolos.getSize(); ++i) {
                if (*tablaSimbolos.get(i) == lex) {
                    encontrado = true;
                    break;
                }
            }
            if (!encontrado) {
                tablaSimbolos.add(lex);
            }
			return token(token::IDENTIFICADOR, lex, startLine, startCol);

		}
	}

	if (isdigit(static_cast<unsigned char>(ch))) {
		string lex;
		if (ch == '0' && (currentChar(p+1) == 'x' || currentChar(p+1) == 'X')) { // Hexadecimal
            lex.push_back(advance(p, l, c)); // '0'
            lex.push_back(advance(p, l, c)); // 'x'
            while (isxdigit(static_cast<unsigned char>(currentChar(p)))) {
                lex.push_back(advance(p, l, c));
            }
        } else {
		while (isdigit(static_cast<unsigned char>(currentChar(p)))) { // Entero
			lex.push_back(advance(p, l, c));
		}
		if (currentChar(p) == '.' && isdigit(static_cast<unsigned char>(currentChar(p + 1)))) { // Fraccionario
			lex.push_back(advance(p, l, c));
			while (isdigit(static_cast<unsigned char>(currentChar(p)))) {
				lex.push_back(advance(p, l, c));
			}
		}
		if ((currentChar(p) == 'e' || currentChar(p) == 'E') && // Notación Científica
                (isdigit(static_cast<unsigned char>(currentChar(p+1))) ||
                 currentChar(p+1) == '+' || currentChar(p+1) == '-')) {
                lex.push_back(advance(p, l, c));
                if (currentChar(p) == '+' || currentChar(p) == '-') {
                    lex.push_back(advance(p, l, c));
                }
                while (isdigit(static_cast<unsigned char>(currentChar(p)))) {
                    lex.push_back(advance(p, l, c));
                }
            }
        }
		return token(token::NUMERO, lex, startLine, startCol);
	}

	if (ch == '"' || ch == '\'') {
		char quote = ch;
		string lex;
		lex.push_back(advance(p, l, c));

		bool closed = false;
		while (currentChar(p) != '\0') {
			char c1 = advance(p, l, c);
			lex.push_back(c1);
			if (c1 == '\\' && currentChar(p) != '\0') {
				lex.push_back(advance(p, l, c));
				continue;
			}
			if (c1 == quote) {
				closed = true;
				break;
			}
		}

		if (!closed) {
			return token(token::DESCONOCIDO, lex, startLine, startCol);
		}

		return token(token::CADENA, lex, startLine, startCol);
	}

	if (p + 1 < source.size()) {
		string two;
		two.push_back(currentChar(p));
		two.push_back(currentChar(p + 1));
		if (isTwoCharSymbol(two)) {
			advance(p, l, c);
			advance(p, l, c);
			return token(token::SIMBOLO, two, startLine, startCol);
		}
	}

	string one;
	one.push_back(advance(p, l, c));
	return token(token::SIMBOLO, one, startLine, startCol);
}
