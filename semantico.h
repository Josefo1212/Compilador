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
