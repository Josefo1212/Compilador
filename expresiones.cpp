#include "expresiones.h"
#include "lexico.h"
#include <sstream>
#include <iostream>

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
    cout << "Convirtiendo a postfija: " << expresion << endl; // Depuración
    istringstream flujo(expresion);
    Lexico lexico(flujo);
    stack<string> pila;
    string resultado;

    token t;
    int balanceParentesis = 0; // Contador para verificar balance de paréntesis

    while ((t = lexico.siguiente()).getTipo() != token::FIN) {
        string lexema = t.getLexema();
        cout << "Token procesado: " << lexema << endl; // Depuración

        if (t.getTipo() == token::NUMERO) {
            resultado += lexema + " ";
        } else if (lexema == "(") {
            pila.push(lexema);
            balanceParentesis++;
        } else if (lexema == ")") {
            balanceParentesis--;
            while (!pila.empty() && pila.top() != "(") {
                resultado += pila.top() + " ";
                pila.pop();
            }
            if (!pila.empty() && pila.top() == "(") {
                pila.pop();
            } else {
                throw runtime_error("Paréntesis desbalanceados");
            }
        } else if (esOperador(lexema[0])) {
            while (!pila.empty() && precedencia(pila.top()[0]) >= precedencia(lexema[0])) {
                resultado += pila.top() + " ";
                pila.pop();
            }
            pila.push(lexema);
        } else {
            throw runtime_error("Token inválido en la expresión: " + lexema);
        }
    }

    while (!pila.empty()) {
        if (pila.top() == "(") {
            throw runtime_error("Paréntesis desbalanceados");
        }
        resultado += pila.top() + " ";
        pila.pop();
    }

    if (balanceParentesis != 0) {
        throw runtime_error("Paréntesis desbalanceados en la expresión");
    }

    cout << "Postfija: " << resultado << endl; // Depuración
    return resultado;
}

double Expresiones::evaluarPostfija(const string& expresionPostfija) {
    cout << "Evaluando postfija: " << expresionPostfija << endl; // Depuración
    stack<double> pila;
    istringstream tokens(expresionPostfija);
    string token;

    while (tokens >> token) {
        cout << "Procesando token: " << token << endl; // Depuración
        if (isdigit(token[0])) {
            pila.push(stod(token));
        } else if (esOperador(token[0])) {
            if (pila.size() < 2) {
                throw runtime_error("Expresión inválida");
            }
            double b = pila.top(); pila.pop();
            double a = pila.top(); pila.pop();

            switch (token[0]) {
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
        throw runtime_error("Expresion invalida");
    }

    cout << "Resultado: " << pila.top() << endl; // Depuración
    return pila.top();
}

double Expresiones::evaluar(const string& expresion) {
    string expresionPostfija = infijaAPostfija(expresion);
    return evaluarPostfija(expresionPostfija);
}