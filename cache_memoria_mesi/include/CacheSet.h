#pragma once
#include "CacheLine.h"
#include <array>
#include <optional>

using namespace std;

class CacheSet {
public:
    static constexpr int NUM_WAYS = 2;  // Numero de líneas por set (2 vías)
    array<CacheLine, NUM_WAYS> lines; // Las líneas de caché en este set (2 líneas)

    // Busca una línea por tag, retorna índice o nullopt
    optional<uint8_t> find_line(uint8_t tag) const;

    // Encuentra línea a reemplazar (LRU)
    int get_replacement_index() const;

    // Actualiza LRU
    void update_lru(int accessed_idx);
};
