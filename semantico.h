#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include "sintactico.h"

using namespace std;

// Excepción para división entre cero
class DivisionPorCeroError : public runtime_error {
public:
	DivisionPorCeroError() : runtime_error("Error: división por cero.") {}
};

// Enum para tipos de variables
enum class TipoVariable {
	DESCONOCIDO,
	ENTERO,
	FLOTANTE,
	CADENA,
	BOOLEANO
};

// Enum para tipos numéricos
enum class TipoNumerico {
	NINGUNO,
	ENTERO,
	FLOTANTE
};

// Excepción para incompatibilidad de tipos numéricos
class IncompatibilidadTiposNumericosError : public runtime_error {
public:
	IncompatibilidadTiposNumericosError(const string& detalle)
		: runtime_error("Error semántico: incompatibilidad de tipos numéricos. " + detalle) {}
};

// Función para verificar compatibilidad de tipos numéricos en asignaciones
inline void verificarCompatibilidadNumerica(TipoNumerico destino, TipoNumerico origen) {
	// Ejemplo: asignar flotante a entero
	if (destino == TipoNumerico::ENTERO && origen == TipoNumerico::FLOTANTE) {
		throw IncompatibilidadTiposNumericosError("Asignación de flotante a entero puede causar pérdida de información.");
	}
}

// Utilidad para convertir tipo numérico a string
inline string tipoNumericoToString(TipoNumerico tipo) {
	switch (tipo) {
		case TipoNumerico::NINGUNO: return "NINGUNO";
		case TipoNumerico::ENTERO: return "ENTERO";
		case TipoNumerico::FLOTANTE: return "FLOTANTE";
		default: return "INVALIDO";
	}
}

// Excepción para errores de concatenación
class ConcatenacionError : public runtime_error {
public:
	ConcatenacionError() : runtime_error("Error: concatenación inválida.") {}
};

// Excepción para valor indefinido
class ValorIndefinidoError : public runtime_error {
public:
	ValorIndefinidoError() : runtime_error("Error: valor indefinido.") {}
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

// Clase Semantico para el análisis semántico
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

    // Pila de ámbitos: cada ámbito es un mapa de nombre de variable a símbolo
    vector<unordered_map<string, Simbolo>> tablaSimbolos;
    // Pila para el tipo de retorno de la función actual (para verificar return)
    vector<TipoVariable> tipoFuncionActual;

    void entrarAmbito();
    void salirAmbito();
    void declararVariable(const string& nombre, const Simbolo& sim);
    Simbolo* buscarVariable(const string& nombre);
    TipoVariable obtenerTipoVariable(const string& nombre);

    // Métodos de visita
    void visitar(shared_ptr<NodoAST> nodo);
    TipoNumerico visitarExpresion(shared_ptr<NodoAST> nodo);
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
