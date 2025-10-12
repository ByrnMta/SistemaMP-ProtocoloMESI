#pragma once
#include "CacheSet.h"
#include <array>
#include <cstdint>
#include <vector>

using namespace std;

class Cache {
public:
    static constexpr int NUM_SETS = 8;
    static constexpr int BLOCK_SIZE = 32; // bytes
    static constexpr int NUM_BLOCKS = 16;
    static constexpr int NUM_WAYS = 2;

    // Sets de la caché (2-ways asociativa)
    array<CacheSet, NUM_SETS> sets;

    // Lee datos de la caché
    vector<uint8_t> read(uint32_t address, MESIState& state_out);

    // Escribe datos en la caché
    optional<uint8_t> write(uint32_t address, const vector<double>& data);

    // Métodos auxiliares
    uint8_t get_tag(uint32_t address) const;
    uint8_t get_set_index(uint32_t address) const;
    uint8_t get_block_offset(uint32_t address) const;
};
