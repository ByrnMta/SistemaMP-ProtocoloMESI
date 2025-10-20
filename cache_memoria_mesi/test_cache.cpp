#include "include/Cache.h"
#include <iostream>

using namespace std;

int main() {
    Cache cache;
    uint16_t address1 = 0x4; // Ejemplo de dirección
    uint16_t address2 = 0x24; // Ejemplo de dirección
    array<double, 4> linea_cache_from_memoria1= {1.1, 2.2, 3.3, 4.4}; // Ejemplo de línea de caché desde memoria
    array<double, 4> linea_cache_from_memoria2 = {5.5, 6.6, 7.7, 8.8}; // Ejemplo de línea de caché a escribir
    double data1 = 50.5; // Ejemplo de dato a escribir
    double data2 = 60.6; // Otro dato a escribir

    // Escritura de linea de caché completa
    if(cache.write_linea_cache(address1, linea_cache_from_memoria1) != nullopt){
        cout << "Se necesita hacer write-back\n" << endl;
    }
    else{
        cout << "No se necesita hacer write-back\n" << endl;
    }
    cache.write_linea_cache(address2, linea_cache_from_memoria2);

    // Escritura de datos individuales en la caché
    cache.write_data_linea_cache(address1, data1);
    cache.write_data_linea_cache(address2, data2);

    // Lectura de dato individual
    auto result = cache.read_data_linea_cache(address2);
    if (result) {
        cout << "Lectura de dato exitosa en la dirección " << address2 << ": " << *result << "\n"<< endl;
    } else {
        cout << "Miss de caché en lectura" << endl;
    }
    
    // Lectura de linea completa
    auto result_linea = cache.read_linea_cache(address2);
    if (result_linea) {
        cout << "Lectura de linea exitosa: ";
        for (const auto& dato : *result_linea) {
            cout << dato << " ";
        }
        cout << endl;
    } else {
        cout << "Miss de caché en lectura" << endl;
    }  
    
    // Reemplazo de linea de cache
    uint16_t address3 = 0x44; // Ejemplo de dirección
    array<double, 4> linea_cache_from_memoria3= {9.9, 10.1, 11.1, 12.1}; // Ejemplo de línea de caché desde memoria
    optional<CacheLine> linea_para_write_back = cache.write_linea_cache(address3, linea_cache_from_memoria3); // en este punto la linea de la dirección adress1 es la menos usada recientemente, entonces se reemplaza

    // Lectura de linea nueva
    result_linea = cache.read_linea_cache(address3);
    if (result_linea) {
        cout << "Lectura de linea nueva escrita: ";
        for (const auto& dato : *result_linea) {
            cout << dato << " ";
        }
        cout << endl;
    } else {
        cout << "Miss de caché en lectura" << endl;
    }  

    // Linea a la que se le debe hacer write-back
    if (linea_para_write_back) {
        cout << "Linea para write-back en la dirección " << linea_para_write_back->direccion_bloque << ": ";
        for (const auto& dato : linea_para_write_back->linea_cache) {
            cout << dato << " ";
        }
        cout << endl;
    }
    else
    {
        cout << "No hay línea para write-back" << endl;
    }

    // Imprimir el estado de la caché
    //cache.print_cache_content();

    return 0;
}

// g++ Cache.cpp CacheSet.cpp test_cache.cpp -o cache_sim
