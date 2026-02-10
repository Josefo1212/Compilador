#ifndef LEXICO_H
#define LEXICO_H

#include "token.h"
#include <cctype>
#include <istream>
#include <sstream>
#include <string>

class Lexico {
	public:
		Lexico() : pos(0), linea(1), columna(1), hasPeek(false) {}

		explicit Lexico(istream& in) : pos(0), linea(1), columna(1), hasPeek(false) {
			load(in);
		}

		void reset() {
			pos = 0;
			linea = 1;
			columna = 1;
			hasPeek = false;
		}

		token siguiente() {
			if (hasPeek) {
				hasPeek = false;
				return peekToken;
			}
			return scanToken(pos, linea, columna);
		}

		token peek() {
			if (!hasPeek) {
				size_t p = pos;
				int l = linea;
				int c = columna;
				peekToken = scanToken(p, l, c);
				hasPeek = true;
			}
			return peekToken;
		}

	private:
		string source;
		size_t pos;
		int linea;
		int columna;
		bool hasPeek;
		token peekToken;

		void load(istream& in) {
			ostringstream ss;
			ss << in.rdbuf();
			source = ss.str();
			reset();
		}

		char currentChar(size_t p) const {
			return (p < source.size()) ? source[p] : '\0';
		}

		char advance(size_t& p, int& l, int& c) {
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

		void skipWhitespaceAndComments(size_t& p, int& l, int& c) {
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

		static bool isIdentifierStart(char ch) {
			return isalpha(static_cast<unsigned char>(ch)) || ch == '_';
		}

		static bool isIdentifierPart(char ch) {
			return isalnum(static_cast<unsigned char>(ch)) || ch == '_';
		}

		static bool isTwoCharSymbol(const string& s) {
			return s == "==" || s == "!=" || s == "<=" || s == ">=" ||
				   s == "&&" || s == "||" || s == "++" || s == "--" ||
				   s == "+=" || s == "-=" || s == "*=" || s == "/=" ||
				   s == "%=" || s == "->";
		}

		token scanToken(size_t& p, int& l, int& c) {
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
				return token(token::IDENTIFICADOR, lex, startLine, startCol);
			}

			if (isdigit(static_cast<unsigned char>(ch))) {
				string lex;
				while (isdigit(static_cast<unsigned char>(currentChar(p)))) {
					lex.push_back(advance(p, l, c));
				}
				if (currentChar(p) == '.' && isdigit(static_cast<unsigned char>(currentChar(p + 1)))) {
					lex.push_back(advance(p, l, c));
					while (isdigit(static_cast<unsigned char>(currentChar(p)))) {
						lex.push_back(advance(p, l, c));
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
};

#endif