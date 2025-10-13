#pragma once
#include <array>
#include <cstdint>

using namespace std;

// Estados MESI
enum class MESIState : uint8_t {
    INVALID = 0,
    SHARED = 1,
    EXCLUSIVE = 2,
    MODIFIED = 3
};

// Estructura que representa una línea de caché
struct CacheLine {
    array<double,  4> linea_cache; // 32 bytes por línea (4 datos de 8 bytes)
    uint8_t tag = 0;             // Etiqueta
    MESIState mesi = MESIState::INVALID;  // cada linea de caché comienza con un estado inválido
    bool dirty = false;           // Write-back
    bool valid = false;           // Línea válida
    uint16_t lru_counter = 0;      // Para LRU
};
