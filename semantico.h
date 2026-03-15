#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <unordered_map>
#include <stdexcept>

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

#endif
