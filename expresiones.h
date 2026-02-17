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
    // Constructor
    Expresiones() {}

    // Método para evaluar una expresión en notación infija
    double evaluar(const string& expresion) {
        string expresionPostfija = infijaAPostfija(expresion);
        return evaluarPostfija(expresionPostfija);
    }

private:
    // Métodos auxiliares
    int precedencia(char operador) {
        switch (operador) {
            case '+': case '-': return 1;
            case '*': case '/': return 2;
            case '^': return 3;
            default: return 0;
        }
    }

    bool esOperador(char ch) {
        return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^';
    }

    string infijaAPostfija(const string& expresion) {
        stack<char> pila;
        string resultado;

        for (char ch : expresion) {
            if (isdigit(ch) || isalpha(ch)) {
                resultado += ch;
            } else if (ch == '(') {
                pila.push(ch);
            } else if (ch == ')') {
                while (!pila.empty() && pila.top() != '(') {
                    resultado += pila.top();
                    pila.pop();
                }
                if (!pila.empty() && pila.top() == '(') {
                    pila.pop();
                }
            } else if (esOperador(ch)) {
                while (!pila.empty() && precedencia(pila.top()) >= precedencia(ch)) {
                    resultado += pila.top();
                    pila.pop();
                }
                pila.push(ch);
            }
        }

        while (!pila.empty()) {
            resultado += pila.top();
            pila.pop();
        }

        return resultado;
    }

    double evaluarPostfija(const string& expresionPostfija) {
        stack<double> pila;

        for (char ch : expresionPostfija) {
            if (isdigit(ch)) {
                pila.push(ch - '0');
            } else if (esOperador(ch)) {
                if (pila.size() < 2) {
                    throw runtime_error("Expresión inválida");
                }
                double b = pila.top(); pila.pop();
                double a = pila.top(); pila.pop();

                switch (ch) {
                    case '+': pila.push(a + b); break;
                    case '-': pila.push(a - b); break;
                    case '*': pila.push(a * b); break;
                    case '/': 
                        if (b == 0) throw runtime_error("División por cero");
                        pila.push(a / b); 
                        break;
                    case '^': pila.push(pow(a, b)); break;
                }
            }
        }

        if (pila.size() != 1) {
            throw runtime_error("Expresión inválida");
        }

        return pila.top();
    }
};

#endif // EXPRESIONES_H