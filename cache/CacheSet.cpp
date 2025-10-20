#include "include/CacheSet.h"
#include <limits>
#include <optional>
#include <iostream>

using namespace std;

optional<uint8_t> CacheSet::find_line(uint8_t tag) {
    // Se busca la línea con el tag dado (solo hay dos lineas por set en este momento de 16 bloques posibles)
    for (int i = 0; i < NUM_WAYS; ++i) {
        // Se verifica si la linea buscada es válida y si la etiqueta coincide
        if (lines[i].valid && lines[i].tag == tag) {
            return i; // regresa el indice en el set de la linea encontrada si es válida
        }
    }
    return nullopt;
}

uint8_t CacheSet::get_replacement_index() {
    // Se busca la línea inválida primero
    for (int i = 0; i < NUM_WAYS; ++i) {
        if (!lines[i].valid) {
            return i;
        }
    }
    // Si no hay, usa LRU (menor lru_counter)
    int lru_linea_index = 0;

    uint16_t min_lru = lines[0].lru_counter; // valor inicial para min_lru

    // Encontrar la línea con el contador LRU más bajo
    for (int i = 1; i < NUM_WAYS; ++i) {
        if (lines[i].lru_counter < min_lru) {
            min_lru = lines[i].lru_counter; // actualizar el mínimo
            lru_linea_index = i; // actualizar el índice de la línea con menor LRU (menos usada recientemente)
        }
    }
    return lru_linea_index;
}

// Actualiza el contador LRU para la línea accedida
void CacheSet::update_lru(uint8_t accessed_idx) {
    uint16_t max_lru = 0;
    for (auto& line : lines) {
        if (line.lru_counter > max_lru) {
            max_lru = line.lru_counter;
        }
    }
    // Se coloca como la línea más recientemente usada (por eso se suma 1 al máximo)
    lines[accessed_idx].lru_counter = max_lru + 1;
}
