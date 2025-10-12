#include "include/memoria.h"

#include <stdexcept>
#include <iostream>

using namespace std;

int main() {
    Memoria memoria;

    // Comprobar la memoria despues de la inicialización
    for (size_t i = 0; i < 22; ++i) {
        try {
            double valor = memoria.leer(i);
            cout << "Direccion " << i << ": " << valor << endl;
        } catch (const out_of_range& e) {
            cerr << e.what() << endl;
        }
    }
    return 0;
}