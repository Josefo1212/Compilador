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
	void declaracionSinPuntoComa();
	void sentenciaExpresion();
	void sentenciaReturn();
	void sentenciaIf();
	void sentenciaWhile();
	void sentenciaFor();
	void sentenciaDoWhile();
	void llamadaFuncion(bool requierePuntoComa);
	void listaArgumentos();
	void expresion();
	void expresionAsignacion();
	void expresionOR();
	void expresionAND();
	void expresionIgualdad();
	void expresionRelacional();
	void expresionAditiva();
	void expresionMultiplicativa();
	void expresionUnaria();
	void expresionPostfija();
	void primaria();
	void inicializador();

	bool esTipoDato(const token& tok) const;
	bool esReturn(const token& tok) const;
	bool esIf(const token& tok) const;
	bool esElse(const token& tok) const;
	bool esWhile(const token& tok) const;
	bool esFor(const token& tok) const;
	bool esDo(const token& tok) const;
	bool esOperadorAsignacion(const string& lexema) const;
	runtime_error errorSintactico(const string& mensaje) const;
};

#endif
