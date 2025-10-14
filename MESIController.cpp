#include "MESIController.h"
#include <iostream>

using namespace std;

MESIController::MESIController(Cache* cache, Interconnect* interconnect)
    : cache_(cache), interconnect_(interconnect) {}


/* Para solicitudes de lectura desde el PE 
    1. Intenta leer directamente de la caché
    2. Si hit, retorna el dato
    3. Si miss, genera solicitud READ_LOCAL al Interconnect
    4. 
*/
double read(uint16_t address, uint8_t pe_id) {
    
    auto data_result = cache_->read(address);

    if (data_result.has_value()) {
        return data_result.value();
    } else {

        Message msg;
        msg.type = MessageType::READ_LOCAL;
        msg.address = address;
        msg.src_id = pe_id;

        // Envia el mensaje al Interconnect
        interconnect_->enqueue_mssg(pe_id, msg);

        // Esta acción de read debe de devolver el dato, entonces debe de esperar a la respuesta
    
        return 0.0; // Valor de retorno momentaneo, mientras se discute como se maneja la espera
    }
}


/* Para solicitudes de escritura desde el PE
    1. Intenta escribir directamente en la caché
    2. Si hit, actualiza el dato y el estado MESI a MODIFIED, notifica al Interconnect
    3. Si miss, genera solicitud WRITE_LOCAL al Interconnect para traer la línea, invalidar en otras caches y luego escribir
    4. 
*/
void write(uint16_t address, double dato, uint8_t pe_id) {

    auto resultado = cache_->write_data_linea_cache(address, dato);

    if (resultado.has_value()) {

        cache_->setMESIState(address, MESIState::MODIFIED);

        Message msg;
        msg.type = MessageType::INVALIDATE; // envia una invalidacion directa por que ya tiene la linea
        msg.address = address;
        msg.src_id = pe_id;

        interconnect_->enqueue_mssg(pe_id, msg);

        return;
    
    } else { // Es necesario realizar un caso para miss? Que pasa si hay un miss en escritura?

        Message msg;
        msg.type = MessageType::WRITE_LOCAL; // envia una solicitud de escritura para traer la linea, invalidar y luego escribir
        msg.address = address;
        msg.src_id = pe_id;

        interconnect_->enqueue_mssg(pe_id, msg);

        // La escritura real se pueede realizar tras la respuesta (en processResponse)
    }
}


/* Para procesar la respuesta a la solicitud de lectura o escritura
    1. Escribe la línea completa en la caché
    2. Actualiza el estado MESI según el tipo de respuesta
*/
void processResponse(uint16_t address, array<double,4>& linea_cache, MessageType responseType) {
    cache_->write_linea_cache(address, linea_cache); // escribe la línea completa en la caché

    if (responseType == MessageType::READ_RESP_SHARED) {
        cache_->setMESIState(address, MESIState::SHARED);

    } else if (responseType == MessageType::READ_RESP_EXCLUSIVE) {
        cache_->setMESIState(address, MESIState::EXCLUSIVE);

    } else if (responseType == MessageType::WRITE_RESP_MODIFIED) {
        cache_->setMESIState(address, MESIState::MODIFIED);

        // podria escribir aqui el dato, pues ya la caché tiene la línea completa
        cache_->write_data_linea_cache(address, linea_cache[0]);
    }
}


/* Para procesar solicitudes remotas de otros PEs
    1. Si es READ_REMOTE y tengo la línea, cambio su estado a SHARED y la comparto
    2. Si es INVALIDATE y tengo la línea, cambio su estado a INVALID
    3. Si es WRITE_REMOTE y tengo la línea, cambio su estado a INVALID y la comparto

    4. En cualquier caso de mensaje sino no la tengo, no hago nada (el Interconnect se encarga de buscar en otras caches o en memoria principal)

    A los mensajes agregar el request_id del PE que inició la solicitud para rastrear respuestas
    Y revisar si hace falta un dst_id para los mensajes que van al Interconnect
    Tambien si hace falta llenar algo mas en los mensajes que van al Interconnect
*/
void processRemoteRequest(uint16_t address, MessageType remoteType, uint8_t src_id) {
    switch (remoteType) {
        case MessageType::READ_REMOTE:
            
            auto line_result = cache_->read_line(address);          // cambiar read_line por el metodo real
            
            if (line_result.has_value()) {
                
                cache_->setMESIState(address, MESIState::SHARED);   // cambia el estado de la linea de la cache actual
                
                Message resp_msg;                                   // genera el mensaje de respuesta con la línea
                resp_msg.type = MessageType::READ_RESP_SHARED;
                resp_msg.address = address;
                resp_msg.src_id = this->getPEId();                  // id del PE que responde (implementar getPEId)
                resp_msg.dst_id = src_id;                           // id del PE que solicitó la línea
                resp_msg.linea_cache = line_result.value();         // la línea de datos

                interconnect_->enqueue_mssg(src_id, resp_msg);      // envía el mensaje al Interconnect

            } else {

                // No tengo la línea en mi caché, envio NO_LINE para que el Interconnect busque en otras caches o en memoria principal
                Message resp_msg;
                resp_msg.type = MessageType::NO_LINE;               // Tipo especial para indicar ausencia de línea
                resp_msg.address = address;
                resp_msg.src_id = this->getPEId();                  // id del PE que responde (implementar getPEId)
                resp_msg.dst_id = 255;                              // id temporal para el Interconnect

                interconnect_->enqueue_mssg(src_id, resp_msg);      // Envía el mensaje al Interconnect

            }
            break;

        case MessageType::INVALIDATE:
            
            auto line_result = cache_->read_line(address);          // cambiar read_line por el metodo real
            
            if (line_result.has_value()) {

                cache_->setMESIState(address, MESIState::INVALID);

            } else {
                
                // No tengo la línea en mi caché, envio NO_LINE para que el Interconnect busque en otras caches o en memoria principal
                Message resp_msg;
                resp_msg.type = MessageType::NO_LINE;               // Tipo especial para indicar ausencia de línea
                resp_msg.address = address;
                resp_msg.src_id = this->getPEId();                  // id del PE que responde (implementar getPEId)
                resp_msg.dst_id = 255;                              // id temporal para el Interconnect

                interconnect_->enqueue_mssg(src_id, resp_msg);      // Envía el mensaje al Interconnect

            }   
            break;
        
        case MessageType::WRITE_REMOTE:

            auto line_result = cache_->read_line(address);          // cambiar read_line por el metodo real
            if (line_result.has_value()) {
                cache_->setMESIState(address, MESIState::INVALID);

                Message resp_msg;                                   // genera el mensaje de respuesta con la línea
                resp_msg.type = MessageType::READ_RESP_SHARED;
                resp_msg.address = address;
                resp_msg.src_id = this->getPEId();                  // id del PE que responde (implementar getPEId)
                resp_msg.dst_id = src_id;                           // id del PE que solicitó la línea
                resp_msg.linea_cache = line_result.value();         // la línea de datos

                interconnect_->enqueue_mssg(src_id, resp_msg);      // envía el mensaje al Interconnect

            } else {
                
                // No tengo la línea en mi caché, envio NO_LINE para que el Interconnect busque en otras caches o en memoria principal
                Message resp_msg;
                resp_msg.type = MessageType::NO_LINE;               // Tipo especial para indicar ausencia de línea
                resp_msg.address = address;
                resp_msg.src_id = this->getPEId();                  // id del PE que responde (implementar getPEId)
                resp_msg.dst_id = 255;                              // id temporal para el Interconnect

                interconnect_->enqueue_mssg(src_id, resp_msg);      // Envía el mensaje al Interconnect

            }
            break;
            
        default:
            break;
    }
}