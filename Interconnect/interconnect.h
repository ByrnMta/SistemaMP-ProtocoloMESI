#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "../cache/include/Cache.h"
#include "../cache/include/memoria.h"
#include "../PE/PE.h"
#include "Interconnect/bus_types.h"

using namespace std;

// Hacer InterconnectResponse visible globalmente
struct InterconnectResponse {
    array<double,4> linea;
    bool from_memory;
};

class MESIController;

class Interconnect {

public:
    Interconnect(int num_pes, Memoria* memoria, size_t queue_depth = 16);

    // Configuración y gestión de PEs y controladores MESI
    void attach_mesi_controller(MESIController* mesi, int pe_id);

    // Envío y procesamiento de mensajes del bus
    void send_message(const BusMessage& msg);
    optional<InterconnectResponse> process_messages(const BusMessage& msg);

    // Estado y métricas del bus
    int get_bus_traffic() const;
    void print_bus_state() const;
    size_t get_queue_depth() const { return queue_depth_; }

private:
    Memoria *main_memory_;

    int current_active_pe_id = -1;
    int num_pes;
    vector<MESIController*> mesi_controllers;

    queue<BusMessage> bus_queue;

    mutex bus_mutex;
    condition_variable bus_cv;

    int bus_traffic = 0;
    size_t queue_depth_ = 16;

    InterconnectResponse handle_cache_miss(const BusMessage& msg);
    void handle_invalidate(const BusMessage& msg);
    void handle_write_back(const BusMessage& msg);
};

#endif // INTERCONNECT_H

// AGREGAR METODOS FALTANTES DE LA CLASE (interconnect.cpp)