#include "include/memoria.h"

#include <stdexcept>
#include <iostream>

using namespace std;

int main() {
    Memoria memoria;

    size_t direccion = 0x8;
    array<double, 4> bloque_leer = memoria.read_bloque(direccion);

    cout << "Memoria inicial en la dirección 0x" << direccion << ": ";
    for (double valor : bloque_leer) {
        cout << valor << " ";
    }
    cout << endl;

    array<double, 4> bloque_escribir = {1.1, 2.2, 3.3, 4.4};

    memoria.write_bloque(direccion, bloque_escribir);
    cout << "Bloque escrito en la dirección 0x" << direccion << ": ";
    for (double valor : bloque_escribir) {
        cout << valor << " ";
    }
    cout << endl;

    bloque_leer = memoria.read_bloque(direccion);

    cout << "Bloque leído de la dirección 0x" << direccion << ": ";
    for (double valor : bloque_leer) {
        cout << valor << " ";
    }
    cout << endl;
    return 0;
}

// g++ memoria.cpp test_memoria.cpp -o test_memoria