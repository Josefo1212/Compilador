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
	token siguienteToken() const;

	void programa();
	void directivaPreprocesador();
	void definicionFuncion();
	void listaParametros();
	void bloque();
	void sentencia();
	void declaracion();
	void asignacion();
	void sentenciaReturn();
	void llamadaFuncion(bool requierePuntoComa);
	void listaArgumentos();
	void expresion();
	void termino();
	void factor();
	void inicializador();

	bool esTipoDato(const token& tok) const;
	bool esReturn(const token& tok) const;
	runtime_error errorSintactico(const string& mensaje) const;
};

#endif
