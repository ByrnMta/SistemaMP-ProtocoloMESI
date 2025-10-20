#include <iostream>

#include "MESIController.h"
#include "../Interconnect/bus_types.h"
#include "../Interconnect/interconnect.h"

using namespace std;

MESIController::MESIController(Cache* cache, Interconnect* interconnect, int pe_id)
    : cache_(cache), interconnect_(interconnect), pe_id_(pe_id) {}

// ==================================================================================== METODOS PRINCIPALES ===


// Lee un dato de la caché, si no está, envía mensaje de read miss al interconnect para obtenerlo
optional<double> MESIController::read(uint16_t address) { 
    
    auto result = cache_->read_data_linea_cache(address);                               // Intenta leer de caché privada

    if (result.has_value()) {                                                           // Si está en caché lo retorna, sino envia solicitud al bus
        cout << "[VERIF-MESI] PE " << pe_id_ << " hit en caché privada para dirección " << address << endl;
        return result;                                                                  // Retorna el dato leído
    } else {
        cout << "[VERIF-MESI] PE " << pe_id_ << " miss en caché privada para dirección " << address << endl;
        return request_line_from_bus(address, READ_MISS, 0.0, MESIState::SHARED);       // Solicita línea al bus
    }
}


// Escribe un dato en la caché, si no está, envía mensaje de write miss al interconnect para obtenerlo
void MESIController::write(uint16_t address, double value) {

    auto result = cache_->write_data_linea_cache(address, value);                       // Intenta escribir en caché privada
    
    if (result.has_value()) {                                                           // Si está en caché lo escribe directamente, sino envia solicitud al bus
        cout << "[VERIF-MESI] PE " << pe_id_ << " escribió dato en caché privada para dirección " << address << endl;
        cache_->update_linea_cache_mesi(address, MESIState::MODIFIED);                  // Actualiza estado a MODIFIED
        cout << "[VERIF-MESI] PE " << pe_id_ << " actualizó estado de línea (a MODIFIED). Enviando invalidación a otros PEs..." << endl;
        send_invalidate_to_others(address);                                             // Notifica a otros PEs para invalidar la línea
        cout << "[VERIF-MESI] PE " << pe_id_ << " finalizó flujo de invalidación directa para dirección " << address << endl;
    } else {
        cout << "[VERIF-MESI] PE " << pe_id_ << " MISS al escribir en caché para dirección " << address << endl;
        request_line_from_bus(address, WRITE_MISS, value, MESIState::MODIFIED);         // Solicita línea al bus
    }
}


// ==================================================================================== FUNCIONES AUXILIARES ===


// Maneja la solicitud de una línea al interconnect para READ_MISS y WRITE_MISS
optional<double> MESIController::request_line_from_bus(uint16_t address, MessageType type, double value, MESIState new_state) {
    
    if (!interconnect_) {                                                               // Verifica que el interconnect esté disponible
        cout << "[VERIF-MESI] ERROR: No hay interconnect disponible para PE " << pe_id_ << endl;
        return nullopt;
    }

    BusMessage msg;                                                                     // Prepara el mensaje para el interconnect
    msg.sender_id = pe_id_;
    msg.type = type;
    msg.address = address;
    msg.data = value;

    interconnect_->send_message(msg);                                                   // Envía el mensaje al interconnect
    auto interconnect_response = interconnect_->process_messages(msg);                  // Procesa el mensaje y espera la respuesta

    if (interconnect_response.has_value()) {                                            // Si recibió una línea válida

        const auto& response = interconnect_response.value();                           // Extrae la respuesta
        cout << "[VERIF-MESI] PE " << pe_id_ << " recibió línea para dirección " << address << " y la escribe en caché" << endl;

        auto write_back_line = cache_->write_linea_cache(address, const_cast<array<double,4>&>(response.linea)); // Escribe la línea en caché
        
        MESIState next_state;                                                           // Determina el siguiente estado de la línea
        if (type == READ_MISS) {                                                        // Si es READ_MISS
            if (response.from_memory) {                                                 // Si vino de memoria principal, pasa a EXCLUSIVE, sino a SHARED porque vino de otro caché
                next_state = MESIState::EXCLUSIVE;
            } else {    
                next_state = MESIState::SHARED;
            }
        } else if (type == WRITE_MISS) {                                                // Si es WRITE_MISS, pasa a MODIFIED
            next_state = MESIState::MODIFIED;
        }
        
        cache_->update_linea_cache_mesi(address, next_state);                           // Actualiza el estado de la línea en caché

        if (write_back_line.has_value()) {                                              // Si hubo write-back, envia el mensaje al interconnect
            cout << "[VERIF-MESI] PE " << pe_id_ << " genera WRITE_BACK para dirección " << write_back_line->direccion_bloque << endl;
            request_write_back(write_back_line->direccion_bloque, write_back_line->linea_cache);
        }

        if (type == WRITE_MISS) {                                                       // Si es WRITE_MISS, escribe el dato solicitado, sino lee y retorna
            cache_->write_data_linea_cache(address, value);                             // Escribe el dato solicitado
            return nullopt;                                                             // Este es ignorado por write
        } else {
            auto final_result = cache_->read_data_linea_cache(address);                 // Lee el dato solicitado 
            if (final_result.has_value()) {                                             // Si pudo leer el dato
                cout << "[VERIF-MESI] PE " << pe_id_ << " accedió al dato tras recibir línea para dirección " << address << endl;
                return final_result;                                                    // Retorna el dato leído
            } else {
                cout << "[VERIF-MESI] ERROR: PE " << pe_id_ << " no pudo acceder al dato tras recibir línea para dirección " << address << endl;
                return nullopt;                                                         
            }
        }
    } else {
        cout << "[VERIF-MESI] ERROR: PE " << pe_id_ << " no recibió línea válida para dirección " << address << endl;
        return nullopt;
    }
}


// Maneja el envío de un mensaje WRITE_BACK al interconnect (parametros correctos en el mensaje para write_bloque?)
void MESIController::request_write_back(uint16_t address, const array<double,4>& linea_cache) {
    if (!interconnect_) {                                                               // Verifica que el interconnect esté disponible
        cout << "[VERIF-MESI] ERROR: No hay interconnect disponible para PE " << pe_id_ << endl;
        return;
    }

    BusMessage msg;                                                                     // Prepara el mensaje para el interconnect
    msg.sender_id = pe_id_;
    msg.type = WRITE_BACK;
    msg.address = address;
    msg.cache_line = linea_cache;                                                       // Adjunta la línea de caché para write-back
    interconnect_->send_message(msg);                                                   // Envía el mensaje al interconnect
    interconnect_->process_messages(msg);                                               // Procesa el mensaje
}


// Maneja el envío de un mensaje INVALIDATE a los otros PEs a través del interconnect
void MESIController::send_invalidate_to_others(uint16_t address) {
    if (interconnect_) {                                                                // Verifica que el interconnect esté disponible
        
        cout << "[VERIF-MESI] PE " << pe_id_ << " envía mensaje INVALIDATE al interconnect para dirección " << address << endl;
        
        BusMessage msg;                                                                 // Prepara el mensaje para el interconnect
        msg.sender_id = pe_id_;
        msg.type = INVALIDATE;
        msg.address = address;
        interconnect_->send_message(msg);                                               // Envía el mensaje al interconnect
        interconnect_->process_messages(msg);                                           // Procesa el mensaje
        
        cout << "[VERIF-MESI] PE " << pe_id_ << " mensaje INVALIDATE enviado al interconnect para dirección " << address << endl;
    } else {
        cout << "[VERIF-MESI] ERROR: No hay interconnect disponible para PE " << pe_id_ << endl;
    }
}


// ==================================================================================== PARA MENSAJES DEL EXTERIOR ===


// Procesa un mensaje recibido del bus (Interconnect) y retorna la línea si la tiene
optional<array<double,4>> MESIController::process_bus_message(const BusMessage& msg) {
    switch (msg.type) {
        case READ_MISS:
            return handle_cache_miss_bus(msg);                                          // Maneja READ_MISS
        case WRITE_MISS:
            return handle_cache_miss_bus(msg);                                          // Maneja WRITE_MISS
        case INVALIDATE:
            handle_invalidate_bus(msg);                                                 // Maneja INVALIDATE
            break;
        default: {
            cerr << "[VERIF-MESI] PE " << pe_id_ << " recibió mensaje desconocido tipo " << msg.type << endl;
            return nullopt;
        }
    }
    return nullopt;
}


// Maneja el mensaje de cache miss (READ_MISS o WRITE_MISS) recibido del bus
optional<array<double,4>> MESIController::handle_cache_miss_bus(const BusMessage& msg) {
    
    auto line = cache_->read_linea_cache(msg.address);                                  // Verifica si tiene la línea en caché
    
    if (line.has_value()) {
        if (msg.type == READ_MISS) {                                                    // Si es READ_MISS
            cout << "[VERIF-MESI] PE " << pe_id_ << " tiene línea en caché para dirección " << msg.address << " (READ_MISS), actualizando estado a SHARED y devolviendo línea..." << endl;
            //cache_->update_linea_cache_mesi(msg.address, MESIState::SHARED);            // Actualiza estado a SHARED
        } else if (msg.type == WRITE_MISS) {
            cout << "[VERIF-MESI] PE " << pe_id_ << " tiene línea en caché para dirección " << msg.address << " (WRITE_MISS), invalidando y devolviendo línea..." << endl;
            //cache_->update_linea_cache_mesi(msg.address, MESIState::INVALID);           // Actualiza estado a INVALID
        }
        return line;                                                                    // Retorna la línea encontrada en caché
    } else {
        cout << "[VERIF-MESI] PE " << pe_id_ << " NO tiene línea en caché para dirección " << msg.address << " (MISS)" << endl;
        return nullopt;                                                             
    }
}


// Maneja el mensaje INVALIDATE recibido del bus
void MESIController::handle_invalidate_bus(const BusMessage& msg) {
    cout << "[VERIF-MESI] PE " << pe_id_ << " recibe mensaje INVALIDATE desde bus para dirección " << msg.address << " (solicitado por PE " << msg.sender_id << ")" << endl;
    auto line = cache_->read_linea_cache(msg.address);                                  // Verifica si tiene la línea en caché
    
    if (line.has_value()) {
        cout << "[VERIF-MESI] PE " << pe_id_ << " tiene línea en caché para dirección " << msg.address << ", invalidando (cambiando estado a INVALID, si se descomenta)..." << endl;
        //cache_->update_linea_cache_mesi(msg.address, MESIState::INVALID);               // Invalida la línea en caché
    } else {
        cout << "[VERIF-MESI] PE " << pe_id_ << " NO tiene línea en caché para dirección " << msg.address << ", nada que invalidar." << endl;
    }
}





