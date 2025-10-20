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
    // Cada dato es de 8 bytes (double), bloque de 4 datos
    // Offset = índice del dato dentro del bloque (0 a 3)
    return address % BLOCK_SIZE;
}

// Método para leer una linea de caché completa
optional<array<double, 4>> Cache::read_linea_cache(uint16_t address) {

    // Se obtiene el índice del set al que corresponde la dirección (de 0 a 7)
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección (de 0 a 15 (solo tiene sentido dentro del set))
    uint8_t tag = get_tag(address);

    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    // Se busca la línea en el set (se obtiene un indice (0 o 1) o nullopt)
    auto indice_linea_encontrada = set.find_line(tag);

    if (indice_linea_encontrada != nullopt) {
        // Hit
        set.update_lru(*indice_linea_encontrada); // se incrementa el lru_counter de la linea en cuestión (pues se acaba de acceder a ella)
        auto linea_leida = set.lines[*indice_linea_encontrada].linea_cache;

        return linea_leida; // Se retorna la línea leída

    } else {
        // Se retorna un nullopt si no se pudo encontrar la línea (miss) o si la línea no es válida para leer
        return nullopt;
    }
}

// Método para leer un dato individual en una linea de caché
optional<double> Cache::read_data_linea_cache(uint16_t address) {
    // Se obtiene el índice del set al que corresponde la dirección (de 0 a 7)
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección (de 0 a 15 (solo tiene sentido dentro del set))
    uint8_t tag = get_tag(address);

    // Se obtiene el offset dentro del bloque (offset puede ir de 0 a 3)
    uint8_t offset = get_block_offset(address);

    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    // Se busca la línea en el set (se obtiene un indice (0 o 1) o nullopt)
    auto indice_linea_encontrada = set.find_line(tag);

    if (indice_linea_encontrada != nullopt) {
        // Hit
        set.update_lru(*indice_linea_encontrada); // se incrementa el lru_counter de la linea en cuestión (pues se acaba de acceder a ella)
        auto dato_leido = set.lines[*indice_linea_encontrada].linea_cache[offset];

        return dato_leido; // Se retorna el dato leído

    } else {
        // Se retorna un nullopt si no se pudo encontrar la línea (miss) o si la línea no es válida para leer
        return nullopt;
    }
}

// Método para reemplazar una línea de caché completa (localidad espacial y write-allocate)
optional<CacheLine> Cache::write_linea_cache(uint16_t address, array<double,4>& linea_cache_from_memoria) {
    // Se obtiene el índice del set al que corresponde la dirección
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección
    uint8_t tag = get_tag(address);

    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    // Se obtiene el índice de la línea a reemplazar dentro del set actual (0 o 1)
    uint8_t indice_linea_reemplazar = set.get_replacement_index();

    // objeto CacheLine (se hace una copia pues se va hacer write-back)
    auto linea_write_back = set.lines[indice_linea_reemplazar];

    // Se actualiza o se reemplaza la línea de caché en el set (la nueva linea)
    set.lines[indice_linea_reemplazar].linea_cache = linea_cache_from_memoria;

    // se actualizan los valores de la línea de caché nueva (pues se volvió a cargar a la caché)
    set.lines[indice_linea_reemplazar].tag = tag;
    set.lines[indice_linea_reemplazar].valid = true;
    set.lines[indice_linea_reemplazar].dirty = false; // es igual que en memoria principal
    set.lines[indice_linea_reemplazar].direccion_bloque = address; // se actualiza la dirección en memoria principal del bloque
    set.update_lru(indice_linea_reemplazar); // se incrementa el lru_counter de la linea, acaba de entrar

    // Se retorna la línea que se reemplaza si es que está válida y sucia (para hacer write-back)
    if (linea_write_back.valid && linea_write_back.dirty) {
        return linea_write_back;  // objeto CacheLine (se necesita la dirección y la linea de datos para hacer write-back)
    }
    else {
        return nullopt; // No se retorna nada si la línea que se reemplaza es inválida o no está sucia
    }
}

// Método para escribir un dato individual en una linea de caché
optional<uint8_t> Cache::write_data_linea_cache(uint16_t address, double data_to_write) {

    // Se obtiene el indice del set al que corresponde la dirección (de 0 a 7)
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección (de 0 a 15 (solo tiene sentido dentro del set))
    uint8_t tag = get_tag(address);

    // Obtener el offset dentro del bloque (offset puede ir de 0 a 3)
    uint8_t offset = get_block_offset(address);

    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    // Se busca la línea en el set (se obtiene un indice (0 o 1) o nullopt)
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
        // se retorna un nullopt si no se pudo encontrar la línea
        return nullopt;
    }
}

// Método para verificar si existe una línea en la caché dado una dirección
optional<uint8_t> Cache::find_line(uint16_t address) {
    // Obtener el índice del conjunto, la etiqueta y el offset
    uint8_t set_index = get_set_index(address);

    // Obtener la etiqueta de la dirección
    uint8_t tag = get_tag(address);
    
    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    return set.find_line(tag);  // regresa el índice de la línea (0 o 1) o nullopt
}

// Método para actualizar el estado MESI de una línea de caché dada una dirección
bool Cache::update_linea_cache_mesi(uint16_t address, MESIState new_state){

    /// Se obtiene el indice del set al que corresponde la dirección (de 0 a 7)
    uint8_t set_index = get_set_index(address);

    // Se obtiene la etiqueta del bloque al que corresponde la dirección (de 0 a 15 (solo tiene sentido dentro del set))
    uint8_t tag = get_tag(address);

    // Se obtiene el set correspondiente (objeto CacheSet) a esa dirección de memoria
    auto& set = sets[set_index];

    // Se busca la línea en el set (se obtiene un indice (0 o 1) o nullopt)
    auto indice_linea_encontrada = set.find_line(tag);

    if (indice_linea_encontrada != nullopt) {

        if (new_state == MESIState::INVALID) {
            // Si el nuevo estado es INVALID, marcar la línea como inválida
            set.lines[*indice_linea_encontrada].valid = false;
            set.lines[*indice_linea_encontrada].dirty = false; // También se puede considerar que ya no está sucia
        }

        cout << "[Cache] Estado Actual de la linea: " << int(set.lines[*indice_linea_encontrada].mesi) << endl;

        // Se actualiza el estado MESI de la línea de caché
        set.lines[*indice_linea_encontrada].mesi = new_state;

        cout << "[Cache] Estado actualizado: " << int(set.lines[*indice_linea_encontrada].mesi) << endl;

        return true; // Estado actualizado exitosamente

    } else {
        // Si no se encuentra la línea, se retorna false
        return false; // Línea no encontrada
    }
}

// Método para imprimir el contenido de la cache (para debug)
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
