#include "include/Cache.h"
#include <cstring>
#include <cassert>
#include <iostream>

using namespace std;

// Metodo para obtener la etiqueta de una dirección 
// (128 bloques en memoria principal, donde en cada set hay 2 bloques en un momento, de 16 posibilidades 
// (tag = num_bloque_actual/8)), num_bloque_actual va de 0 a 127
uint8_t Cache::get_tag(uint16_t address) {
    return address / BLOCK_SIZE / NUM_SETS;
}

// Metodo para obtener el índice del conjunto (set) de una dirección (puede ir  de 0 a 7, pues son 8 sets)
uint8_t Cache::get_set_index(uint16_t address) {
    return (address / BLOCK_SIZE) % NUM_SETS;
}

// Metodo para obtener el offset (índice de dato) dentro del bloque de una dirección
uint8_t Cache::get_block_offset(uint16_t address) {
    // Cada dato es de 8 bytes, bloque de 4 datos = 32 bytes
    // Offset = índice del dato dentro del bloque (0 a 3)
    return (address >> 3) & 0x3;
}

// Método para leer datos de la caché
optional<double> Cache::read(uint16_t address) {
    // Se obtiene el índice del set al que corresponde la dirección
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección
    uint8_t tag = get_tag(address);

    // Se obtiene el offset dentro del bloque
    uint8_t offset = get_block_offset(address);

    // Acceder al conjunto correspondiente
    auto& set = sets[set_index];

    // Buscar la línea en el conjunto
    auto indice_linea_encontrado = set.find_line(tag);

    if (indice_linea_encontrado != nullopt) {
        // Hit
        set.update_lru(*indice_linea_encontrado); // se incrementa el lru_counter de la linea en cuestión (pues se acaba de acceder a ella)
        auto dato_leido = set.lines[*indice_linea_encontrado].linea_cache[offset];

        return dato_leido; // Se retorna el dato leído

    } else {
        // Se retorna un nullopt si no se pudo encontrar la línea (miss) o si la línea no es válida para leer
        return nullopt;
    }
}

// Método para reemplazar una línea de caché completa (localidad espacial y write-allocate)
optional<uint8_t> Cache::write_linea_cache(uint16_t address, array<double,4>& linea_cache_from_memoria) {
    // Se obtiene el índice del set al que corresponde la dirección
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección
    uint8_t tag = get_tag(address);

    // Se obtiene el set correspondiente a esa dirección de memoria
    auto& set = sets[set_index];

    // Se obtiene el índice de la línea a reemplazar (0 o 1)
    uint8_t indice_linea_reemplazar = set.get_replacement_index();

    // Se actualiza la línea de caché en el set
    set.lines[indice_linea_reemplazar].linea_cache = linea_cache_from_memoria;

    // se actualizan los valores de la línea de caché (pues se volvió a cargar a la caché)
    set.lines[indice_linea_reemplazar].tag = tag;
    set.lines[indice_linea_reemplazar].valid = true;
    set.lines[indice_linea_reemplazar].dirty = false;
    set.lines[indice_linea_reemplazar].lru_counter = 0;

    return indice_linea_reemplazar; // Se retorna el índice de la línea usada (0 o 1)
}

// Método para escribir datos en la caché
optional<uint8_t> Cache::write_data_linea_cache(uint16_t address, double data_to_write) {
    // Se obtiene el indice del set al que corresponde la dirección
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección
    uint8_t tag = get_tag(address);

    // Obtener el offset dentro del bloque
    uint8_t offset = get_block_offset(address);

    // Se obtiene el set correspondiente a esa dirección de memoria
    auto& set = sets[set_index];

    // Buscar la línea en el set (se obtiene un indice (0 o 1) o nullopt)
    auto indice_linea_encontrada = set.find_line(tag);

    // En caso de que se haya encontrado la línea (hit)
    if (indice_linea_encontrada != nullopt) {
        // Hit
        set.lines[*indice_linea_encontrada].linea_cache[offset] = data_to_write;  // se escribe el dato en la línea de caché
        set.lines[*indice_linea_encontrada].dirty = true; //Linea sucia (no es igual que en memoria principal)
        set.update_lru(*indice_linea_encontrada); // se incrementa el lru_counter de la linea en cuestión (pues se acaba de acceder a ella)
        
        return *indice_linea_encontrada; // Se retorna el índice de la línea usada (0 o 1)

    } else {
        // En caso de que no se haya encontrado la línea (miss)
        // se retorna un nullopt si no se pudo encontrar la línea (miss)
        return nullopt;
    }
}

// Método para verificar si existe una línea en la caché dado una dirección
optional<uint8_t> Cache::find_line(uint16_t address) {
    // Obtener el índice del conjunto, la etiqueta y el offset
    uint8_t set_index = get_set_index(address);

    // Obtener la etiqueta de la dirección
    uint8_t tag = get_tag(address);
   
    auto& set = sets[set_index];
    return set.find_line(tag);  // regresa el índice de la línea (0 o 1) o nullopt
}

// Metodo para imprimir el contenido de la cache (para debug)
void Cache::print_cache_content() {
    for (int i = 0; i < NUM_SETS; ++i) {
        cout << "Set " << i << ":\n";
        for (int j = 0; j < NUM_WAYS; ++j) {
            auto& line = sets[i].lines[j];
            cout << "  Linea " << j << ":";
            for (auto& val : line.linea_cache) {
                cout << val << " ";
            }
            cout << "\n" << endl;
        }
    }
}
