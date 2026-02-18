#ifndef LEXICO_H
#define LEXICO_H

#include "token.h"
#include "arraylist.h"
#include <cctype>
#include <istream>
#include <sstream>
#include <string>
#include <unordered_set>

class Lexico {
	public:
		Lexico();
		explicit Lexico(istream& in);
		void reset();
		token siguiente();
		token peek();

		const ArrayList<string>& getTablaSimbolos() const { return tablaSimbolos; }

	private:
		string source;
		size_t pos;
		int linea;
		int columna;
		bool hasPeek;
		token peekToken;

		ArrayList<string> tablaSimbolos;   // Almacena los identificadores (simplificado)
    	static unordered_set<string> palabrasReservadas; 

		void load(istream& in);
		char currentChar(size_t p) const;
		char advance(size_t& p, int& l, int& c);
		void skipWhitespaceAndComments(size_t& p, int& l, int& c);
		static bool isIdentifierStart(char ch);
		static bool isIdentifierPart(char ch);
		static bool isTwoCharSymbol(const string& s);
		token scanToken(size_t& p, int& l, int& c);
		token scanNumber(size_t& p, int& l, int& c, int startLine, int startCol);
		void skipNonExpressionContent(size_t& p, int& l, int& c); // Declaración de la función

		static bool esPalabraReservada(const string& lex);
};

#endif