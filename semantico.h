#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include "sintactico.h"

using namespace std;

// Excepcion para division entre cero
class DivisionPorCeroError : public runtime_error {
public:
	DivisionPorCeroError() : runtime_error("Error: division por cero.") {}
};

// Enum para tipos de variables
enum class TipoVariable {
	DESCONOCIDO,
	ENTERO,
	FLOTANTE,
	CADENA,
	BOOLEANO
};

// Enum para tipos numericos
enum class TipoNumerico {
	NINGUNO,
	ENTERO,
	FLOTANTE
};

// Excepcion para incompatibilidad de tipos numericos
class IncompatibilidadTiposNumericosError : public runtime_error {
public:
	IncompatibilidadTiposNumericosError(const string& detalle)
		: runtime_error("Error semantico: incompatibilidad de tipos numericos. " + detalle) {}
};

// Funcion para verificar compatibilidad de tipos numericos en asignaciones
inline void verificarCompatibilidadNumerica(TipoNumerico destino, TipoNumerico origen) {
	// Ejemplo: asignar flotante a entero
	if (destino == TipoNumerico::ENTERO && origen == TipoNumerico::FLOTANTE) {
		throw IncompatibilidadTiposNumericosError("Asignacion de flotante a entero puede causar perdida de informacion.");
	}
}

// Utilidad para convertir tipo numerico a string
inline string tipoNumericoToString(TipoNumerico tipo) {
	switch (tipo) {
		case TipoNumerico::NINGUNO: return "NINGUNO";
		case TipoNumerico::ENTERO: return "ENTERO";
		case TipoNumerico::FLOTANTE: return "FLOTANTE";
		default: return "INVALIDO";
	}
}

// Excepcion para errores de concatenacion
class ConcatenacionError : public runtime_error {
public:
    explicit ConcatenacionError(const string& detalle = "")
        : runtime_error(detalle.empty() ? "Error: concatenacion invalida." : ("Error: concatenacion invalida. " + detalle)) {}
};

// Excepcion para valor indefinido
class ValorIndefinidoError : public runtime_error {
public:
    explicit ValorIndefinidoError(const string& detalle = "")
        : runtime_error(detalle.empty() ? "Error: valor indefinido." : ("Error: valor indefinido. " + detalle)) {}
};

// Utilidad para convertir tipo a string
inline string tipoVariableToString(TipoVariable tipo) {
	switch (tipo) {
		case TipoVariable::DESCONOCIDO: return "DESCONOCIDO";
		case TipoVariable::ENTERO: return "ENTERO";
		case TipoVariable::FLOTANTE: return "FLOTANTE";
		case TipoVariable::CADENA: return "CADENA";
		case TipoVariable::BOOLEANO: return "BOOLEANO";
		default: return "INVALIDO";
	}
}

// Clase Semantico para el analisis semantico
class Semantico {
public:
    Semantico(shared_ptr<NodoAST> raiz);
    void analizar();

private:
    shared_ptr<NodoAST> raiz;

    struct Simbolo {
        TipoVariable tipo;
        bool esArreglo;
        int tamanoArreglo;
        bool inicializado;

        Simbolo(TipoVariable t = TipoVariable::DESCONOCIDO, bool arr = false, int tam = 0)
            : tipo(t), esArreglo(arr), tamanoArreglo(tam), inicializado(false) {}
    };

    // Pila de ambitos: cada ambito es un mapa de nombre de variable a símbolo
    vector<unordered_map<string, Simbolo>> tablaSimbolos;
    // Pila para el tipo de retorno de la funcion actual (para verificar return)
    vector<TipoVariable> tipoFuncionActual;

    void entrarAmbito();
    void salirAmbito();
    void declararVariable(const string& nombre, const Simbolo& sim);
    Simbolo* buscarVariable(const string& nombre);
    TipoVariable obtenerTipoVariable(const string& nombre);

    // Metodos de visita
    void visitar(shared_ptr<NodoAST> nodo);
    TipoNumerico visitarExpresion(shared_ptr<NodoAST> nodo);
    TipoVariable visitarExpresionTipo(shared_ptr<NodoAST> nodo, bool requiereInicializado = true);
    void visitarSentencia(shared_ptr<NodoAST> nodo);
    void visitarDeclaracion(shared_ptr<NodoAST> nodo);
    void visitarBloque(shared_ptr<NodoAST> nodo);
    void visitarFuncion(shared_ptr<NodoAST> nodo);
    void visitarAsignacion(const string& op, shared_ptr<NodoAST> izquierdo, shared_ptr<NodoAST> derecho);
    void visitarLlamadaFuncion(shared_ptr<NodoAST> nodo);

    // Utilidades de tipos
    TipoNumerico obtenerTipoNumericoDeLiteral(const string& lexema);
    TipoNumerico obtenerTipoNumericoDeVariable(const string& nombre);
    TipoNumerico obtenerTipoNumericoDeTipoVariable(TipoVariable tv);
    bool sonCompatibles(TipoNumerico izquierdo, TipoNumerico derecho, const string& operador);
    TipoNumerico tipoResultante(TipoNumerico izq, TipoNumerico der, const string& operador);
    void verificarCondicion(shared_ptr<NodoAST> condNodo);
};

#endif
