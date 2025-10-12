#include "CacheSet.h"
#include <limits>
#include <optional>

using namespace std;

optional<uint8_t> CacheSet::find_line(uint8_t tag) const {
    // Busca la línea con el tag dado (solo hay dos lineas por set en este momento de 16 bloques posibles)
    for (int i = 0; i < NUM_WAYS; ++i) {
        if (lines[i].valid && lines[i].tag == tag) {
            return i; // regresa el indice en el set de la linea encontrada si es válido
        }
    }
    return nullopt;
}

int CacheSet::get_replacement_index() const {
    // Busca línea inválida primero
    for (int i = 0; i < NUM_WAYS; ++i) {
        if (!lines[i].valid) return i;
    }
    // Si no hay, usa LRU (menor lru_counter)
    int lru_idx = 0;
    uint8_t min_lru = numeric_limits<uint8_t>::max();
    for (int i = 0; i < NUM_WAYS; ++i) {
        if (lines[i].lru_counter < min_lru) {
            min_lru = lines[i].lru_counter;
            lru_idx = i;
        }
    }
    return lru_idx;
}

void CacheSet::update_lru(int accessed_idx) {
    uint8_t max_lru = 0;
    for (const auto& line : lines) {
        if (line.lru_counter > max_lru) max_lru = line.lru_counter;
    }
    lines[accessed_idx].lru_counter = max_lru + 1;
}
