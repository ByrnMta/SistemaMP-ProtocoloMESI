#include "include/Cache.h"
#include <iostream>

using namespace std;

int main() {
    Cache cache;
    uint16_t address1 = 0x0; // Ejemplo de dirección
    uint16_t address2 = 0x100; // Ejemplo de dirección
    array<double, 4> linea_cache_from_memoria1= {1.1, 2.2, 3.3, 4.4}; // Ejemplo de línea de caché desde memoria
    array<double, 4> linea_cache_from_memoria2 = {5.5, 6.6, 7.7, 8.8}; // Ejemplo de línea de caché a escribir
    double data1 = 50.5; // Ejemplo de dato a escribir
    double data2 = 60.6; // Otro dato a escribir

    // Escritura de linea de caché completa
    cache.write_linea_cache(address1, linea_cache_from_memoria1);
    cache.write_linea_cache(address2, linea_cache_from_memoria2);
    cout << "Se reemplaza una línea de caché que contiene la dirección: " << address1 << endl;
    cout << "Se reemplaza una línea de caché que contiene la dirección: " << address2 << endl;

    // Escritura de dato en la caché
    cache.write_data_linea_cache(address1, data1);
    cache.write_data_linea_cache(address2, data2);

    // Lectura
    auto result = cache.read(address2);
    if (result) {
        cout << "Lectura exitosa: " << *result << "\n"<< endl;
    } else {
        cout << "Miss de caché en lectura" << endl;
    }

    // Imprimir el estado de la caché
    cache.print_cache_content();

    return 0;
}

// g++ Cache.cpp CacheSet.cpp test_cache.cpp -o cache_sim