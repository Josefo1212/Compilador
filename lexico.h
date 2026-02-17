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

		const ArrayList<std::string>& getTablaSimbolos() const { return tablaSimbolos; }

	private:
		std::string source;
		size_t pos;
		int linea;
		int columna;
		bool hasPeek;
		token peekToken;

		ArrayList<std::string> tablaSimbolos;   // Almacena los identificadores (simplificado)
    	static std::unordered_set<std::string> palabrasReservadas; 

		void load(istream& in);
		char currentChar(size_t p) const;
		char advance(size_t& p, int& l, int& c);
		void skipWhitespaceAndComments(size_t& p, int& l, int& c);
		static bool isIdentifierStart(char ch);
		static bool isIdentifierPart(char ch);
		static bool isTwoCharSymbol(const std::string& s);
		token scanToken(size_t& p, int& l, int& c);
		token scanNumber(size_t& p, int& l, int& c, int startLine, int startCol);

		static bool esPalabraReservada(const std::string& lex);
};

#endif