#ifndef TOKEN_H
#define TOKEN_H

#include <string>

using namespace std;

class token {
	public:
		enum Tipo {
			DESCONOCIDO,
			IDENTIFICADOR,
			NUMERO,
			CADENA,
			SIMBOLO,
			FIN
		};

		token();
		token(Tipo tipo, const string& lexema, int linea, int columna);

		Tipo getTipo() const;
		const string& getLexema() const;
		int getLinea() const;
		int getColumna() const;

		void setTipo(Tipo tipo);
		void setLexema(const string& lexema);
		void setLinea(int linea);
		void setColumna(int columna);

	private:
		Tipo tipo;
		string lexema;
		int linea;
		int columna;
};

inline token::token() : tipo(DESCONOCIDO), lexema(""), linea(1), columna(1) {}

inline token::token(Tipo tipo, const string& lexema, int linea, int columna)
	: tipo(tipo), lexema(lexema), linea(linea), columna(columna) {}

inline token::Tipo token::getTipo() const { return tipo; }
inline const string& token::getLexema() const { return lexema; }
inline int token::getLinea() const { return linea; }
inline int token::getColumna() const { return columna; }

inline void token::setTipo(Tipo tipo) { this->tipo = tipo; }
inline void token::setLexema(const string& lexema) { this->lexema = lexema; }
inline void token::setLinea(int linea) { this->linea = linea; }
inline void token::setColumna(int columna) { this->columna = columna; }

#endif