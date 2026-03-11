#ifndef SINTACTICO_H
#define SINTACTICO_H

#include "lexico.h"
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

struct NodoAST {
	string etiqueta;
	vector<shared_ptr<NodoAST>> hijos;

	explicit NodoAST(const string& etiqueta) : etiqueta(etiqueta) {}

	void agregarHijo(const shared_ptr<NodoAST>& hijo) {
		if (hijo) {
			hijos.push_back(hijo);
		}
	}

	void imprimir(ostream& out, const string& prefijo, bool esUltimo) const;
};

class Sintactico {
public:
	explicit Sintactico(Lexico& lexico);

	void analizar();
	shared_ptr<NodoAST> getArbol() const;
	void imprimirArbol(ostream& out) const;

private:
	Lexico& lexico;
	token actual;
	shared_ptr<NodoAST> arbol;

	void avanzar();
	void coincidir(token::Tipo tipo);
	void coincidir(const string& lexema);
	token siguienteToken() const;
	shared_ptr<NodoAST> crearNodo(const string& etiqueta) const;
	shared_ptr<NodoAST> crearNodoToken(const string& etiqueta, const token& tok) const;
	shared_ptr<NodoAST> crearNodoBinario(const string& etiqueta, const shared_ptr<NodoAST>& izquierdo, const shared_ptr<NodoAST>& derecho) const;

	shared_ptr<NodoAST> programa();
	shared_ptr<NodoAST> directivaPreprocesador();
	shared_ptr<NodoAST> definicionFuncion();
	shared_ptr<NodoAST> listaParametros();
	shared_ptr<NodoAST> bloque();
	shared_ptr<NodoAST> sentencia();
	shared_ptr<NodoAST> declaracion();
	shared_ptr<NodoAST> declaracionSinPuntoComa();
	shared_ptr<NodoAST> sentenciaExpresion();
	shared_ptr<NodoAST> sentenciaReturn();
	shared_ptr<NodoAST> sentenciaIf();
	shared_ptr<NodoAST> sentenciaWhile();
	shared_ptr<NodoAST> sentenciaFor();
	shared_ptr<NodoAST> sentenciaDoWhile();
	shared_ptr<NodoAST> llamadaFuncion(bool requierePuntoComa);
	shared_ptr<NodoAST> listaArgumentos();
	shared_ptr<NodoAST> expresion();
	shared_ptr<NodoAST> expresionAsignacion();
	shared_ptr<NodoAST> expresionOR();
	shared_ptr<NodoAST> expresionAND();
	shared_ptr<NodoAST> expresionIgualdad();
	shared_ptr<NodoAST> expresionRelacional();
	shared_ptr<NodoAST> expresionAditiva();
	shared_ptr<NodoAST> expresionMultiplicativa();
	shared_ptr<NodoAST> expresionUnaria();
	shared_ptr<NodoAST> expresionPostfija();
	shared_ptr<NodoAST> primaria();
	shared_ptr<NodoAST> inicializador();

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
