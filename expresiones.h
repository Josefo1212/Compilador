#ifndef EXPRESIONES_H
#define EXPRESIONES_H

#include <string>
#include <stack>
#include <unordered_map>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <cmath>

using namespace std;

class Expresiones {
public:
    Expresiones() {}
    double evaluar(const string& expresion);

private:
    int precedencia(char operador);
    bool esOperador(char ch);
    string infijaAPostfija(const string& expresion);
    double evaluarPostfija(const string& expresionPostfija);
};

#endif // EXPRESIONES_H