#include "expresiones.h"
#include <sstream>
#include <cctype>
#include <cmath>
#include <stdexcept>

using namespace std;

int Expresiones::precedencia(char operador) {
    switch (operador) {
        case '+': case '-': return 1;
        case '*': case '/': return 2;
        case '^': return 3;
        default: return 0;
    }
}

bool Expresiones::esOperador(char ch) {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^';
}

string Expresiones::infijaAPostfija(const string& expresion) {
    stack<char> pila;
    string resultado;
    size_t i = 0;

    while (i < expresion.size()) {
        char ch = expresion[i];
        if (isspace(ch)) {
            i++;
            continue;
        }
        if (isdigit(ch) || ch == '.') {
            // Leer número completo (varios dígitos y punto decimal)
            string numero;
            while (i < expresion.size() && (isdigit(expresion[i]) || expresion[i] == '.')) {
                numero += expresion[i];
                i++;
            }
            resultado += numero + " ";
        } else if (ch == '(') {
            pila.push(ch);
            i++;
        } else if (ch == ')') {
            while (!pila.empty() && pila.top() != '(') {
                resultado += pila.top();
                resultado += " ";
                pila.pop();
            }
            if (!pila.empty() && pila.top() == '(') {
                pila.pop();
            } else {
                throw runtime_error("Parentesis desbalanceados");
            }
            i++;
        } else if (esOperador(ch)) {
            while (!pila.empty() && precedencia(pila.top()) >= precedencia(ch)) {
                resultado += pila.top();
                resultado += " ";
                pila.pop();
            }
            pila.push(ch);
            i++;
        } else {
            throw runtime_error("Caracter invalido en la expresion: " + string(1, ch));
        }
    }

    while (!pila.empty()) {
        if (pila.top() == '(') {
            throw runtime_error("Parentesis desbalanceados");
        }
        resultado += pila.top();
        resultado += " ";
        pila.pop();
    }

    return resultado;
}

double Expresiones::evaluarPostfija(const string& expresionPostfija) {
    stack<double> pila;
    istringstream tokens(expresionPostfija);
    string token;

    while (tokens >> token) {
        if (isdigit(token[0]) || (token[0] == '.' && token.size() > 1)) {
            pila.push(stod(token));
        } else if (token.size() == 1 && esOperador(token[0])) {
            if (pila.size() < 2) {
                throw runtime_error("Expresion invalida");
            }
            double b = pila.top(); pila.pop();
            double a = pila.top(); pila.pop();
            char op = token[0];
            switch (op) {
                case '+': pila.push(a + b); break;
                case '-': pila.push(a - b); break;
                case '*': pila.push(a * b); break;
                case '/':
                    if (b == 0) throw runtime_error("Division por cero");
                    pila.push(a / b);
                    break;
                case '^': pila.push(pow(a, b)); break;
                default: throw runtime_error("Operador desconocido");
            }
        } else {
            throw runtime_error("Token inválido en postfija");
        }
    }

    if (pila.size() != 1) {
        throw runtime_error("Expresion invalida");
    }

    return pila.top();
}

double Expresiones::evaluar(const string& expresion) {
    string postfija = infijaAPostfija(expresion);
    return evaluarPostfija(postfija);
}