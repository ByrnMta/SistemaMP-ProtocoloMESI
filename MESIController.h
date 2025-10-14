#ifndef MESICONTROLLER_H
#define MESICONTROLLER_H

#include "include/Cache.h" // Ajustar la ruta cuando se integren los archivos
#include "interconnect.h"  // Ajustar la ruta cuando se integren los archivos
#include <cstdint>

class MESIController {
public:
    MESIController(Cache* cache, Interconnect* interconnect);

    double read(uint16_t address, uint8_t pe_id);
    void write(uint16_t address, double dato, uint8_t pe_id);
    void processResponse(uint16_t address, MessageType responseType, MESIState newState);
    void processRemoteRequest(uint16_t address, MessageType remoteType, uint8_t src_id);

private:
    Cache* cache_;
    Interconnect* interconnect_;
};

#endif // MESICONTROLLER_H
