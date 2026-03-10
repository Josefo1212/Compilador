#ifndef SINTACTICO_H
#define SINTACTICO_H

#include "lexico.h"
#include <stdexcept>
#include <string>

class Sintactico {
public:
	explicit Sintactico(Lexico& lexico);

	void analizar();

private:
	Lexico& lexico;
	token actual;

	void avanzar();
	void coincidir(token::Tipo tipo);
	void coincidir(const string& lexema);

	void programa();
	void sentencia();
	void declaracion();
	void asignacion();
	void expresion();
	void termino();
	void factor();

	bool esTipoDato(const token& tok) const;
	runtime_error errorSintactico(const string& mensaje) const;
};

#endif
