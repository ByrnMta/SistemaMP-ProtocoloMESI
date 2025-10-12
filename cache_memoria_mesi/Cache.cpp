#include "Cache.h"
#include <cstring>
#include <cassert>

using namespace std;

// Metodo para obtener la etiqueta de una dirección 
// (128 bloques en memoria principal, donde en cada set hay 2 bloques en un momento, de 16 posibilidades 
// (tag = num_bloque_actual/8)), num_bloque_actual va de 0 a 127
uint8_t Cache::get_tag(uint32_t address) const {
    return address / BLOCK_SIZE / NUM_SETS;
}

// Metodo para obtener el índice del conjunto (set) de una dirección (puede ir  de 0 a 7, pues son 8 sets)
uint8_t Cache::get_set_index(uint32_t address) const {
    return (address / BLOCK_SIZE) % NUM_SETS;
}

// Metodo para obtener el offset (índice de dato) dentro del bloque de una dirección
uint8_t Cache::get_block_offset(uint32_t address) const {
    // Cada dato es de 8 bytes, bloque de 4 datos = 32 bytes
    // Offset = índice del dato dentro del bloque (0 a 3)
    return (address >> 3) & 0x3;
}

// Método para leer datos de la caché
vector<uint8_t> Cache::read(uint32_t address, MESIState& state_out) {
    // Obtener el índice del conjunto, la etiqueta y el offset
    uint8_t set_index = get_set_index(address);

    // Obtener la etiqueta de la dirección
    uint8_t tag = get_tag(address);

    // Obtener el offset dentro del bloque
    uint8_t offset = get_block_offset(address);

    // Acceder al conjunto correspondiente
    auto& set = sets[set_index];

    // Buscar la línea en el conjunto
    auto indice_linea_set_encontrada = set.find_line(tag);

    if (indice_linea_set_encontrada) {
        // Hit
        set.update_lru(*indice_linea_set_encontrada);
        state_out = set.lines[*indice_linea_set_encontrada].mesi;
        vector<uint8_t> data(1);
    data[0] = set.lines[*indice_linea_set_encontrada].linea_cache[offset];
        return data;
    } else {
        // Miss: aquí se debería cargar el bloque desde memoria principal
        // (Simulación: se marca como inválido y se retorna vacío)
        state_out = MESIState::INVALID;
        return {};
    }
}

// Método para escribir datos en la caché
optional<uint8_t> Cache::write(uint32_t address, const vector<double>& data) {
    // Obtener el índice del conjunto, la etiqueta y el offset
    uint8_t set_index = get_set_index(address);

    // Obtener la etiqueta de la dirección
    uint8_t tag = get_tag(address);

    // Obtener el offset dentro del bloque
    uint8_t offset = get_block_offset(address);

    // Acceder al set correspondiente
    auto& set = sets[set_index];

    // Buscar la línea en el set (se obtiene un indice (0 o 1) o nullopt)
    auto indice_linea_set_encontrada = set.find_line(tag);

    int indice_linea_set; // Índice de la línea a usar (dentro del set, 0 o 1)

    if (indice_linea_set_encontrada != nullopt) {
        // Hit
        indice_linea_set = *indice_linea_set_encontrada;

        set.lines[indice_linea_set].linea_cache[offset] = data[0];
        set.lines[indice_linea_set].dirty = true;
        set.lines[indice_linea_set].mesi = MESIState::MODIFIED;
        set.update_lru(indice_linea_set);
        
        return indice_linea_set; // Se retorna el índice de la línea usada (0 o 1)

    } else {
        // Miss: write-allocate, reemplazo
        indice_linea_set = set.get_replacement_index(); // Se obtiene el indice de la linea a reemplazar (0 o 1), menos usada recientemente

        /*
        if (set.lines[indice_linea_set].valid && set.lines[indice_linea_set].dirty) {
            // Aquí se debería escribir a memoria principal (write-back)

        }
        */

        set.lines[indice_linea_set].tag = tag;
        set.lines[indice_linea_set].valid = true;
        set.lines[indice_linea_set].dirty = false;
        //set.lines[indice_linea_set].mesi = MESIState::EXCLUSIVE; // Por defecto

        // se limpia o se inicializa la linea de caché
        for (auto& elemento_en_linea : set.lines[indice_linea_set].linea_cache) {
            elemento_en_linea = 0;
        }
        
        // se retorna un nullopt si no se pudo encontrar la línea (miss)
        return nullopt;
    }
}
