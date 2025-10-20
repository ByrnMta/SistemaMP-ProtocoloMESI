#include "Cache.h"
#include <iostream>

using namespace std;

int main() {
    Cache cache;
    MESIState state;
    uint32_t address = 0x10; // Ejemplo de dirección
    vector<uint8_t> data = {42};

    // Escritura
    cache.write(address, data);
    cout << "Se escribe en la dirección: " << address << endl;

    // Lectura
    auto result = cache.read(address, state);
    if (!result.empty()) {
        cout << "Lectura exitosa: " << (int)result[0] << ", estado MESI: " << (int)state << endl;
    } else {
        cout << "Miss de caché en lectura" << endl;
    }
    return 0;
}
