#pragma once
#include "CacheSet.h"
#include <array>
#include <cstdint>
#include <vector>

using namespace std;

class Cache {
public:
    static constexpr int NUM_SETS = 8;
    static constexpr int BLOCK_SIZE = 4; // 4 datos por bloque (8 bytes cada dato)
    static constexpr int NUM_BLOCKS = 16;
    static constexpr int NUM_WAYS = 2;

    // Sets de la caché (2-ways asociativa)
    array<CacheSet, NUM_SETS> sets;
    
    // Lee una línea completa de la caché
    optional<array<double, 4>> read_linea_cache(uint16_t address);
    // Lee un dato individual en la caché
    optional<double> read_data_linea_cache(uint16_t address);

    // Método para reemplazar una linea de caché y escribir un dato individual en la caché
    optional<CacheLine> write_linea_cache(uint16_t address, array<double,4>& linea_cache_from_memoria);
    optional<uint8_t> write_data_linea_cache(uint16_t address, double data_to_write);

    // Métodos para cambiar el estado MESI de una línea
    bool update_linea_cache_mesi(uint16_t address, MESIState new_state);

    // Métodos auxiliares
    uint8_t get_tag(uint16_t address);
    uint8_t get_set_index(uint16_t address);
    uint8_t get_block_offset(uint16_t address);
    optional<uint8_t> find_line(uint16_t address);

    // metodo para imprimir el estado de la cache (para debug)
    void print_cache_content();
};
