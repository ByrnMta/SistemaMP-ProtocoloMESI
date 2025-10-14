#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <queue>
#include <mutex>
#include <unordered_map>
#include <thread>

// Enum para los tipos de mensaje
enum class MessageType {
	READ_LOCAL,           // Lectura iniciada por el propio PE
	READ_REMOTE,          // Lectura iniciada por otro PE (solicita línea - interconnect debe de generar este mensaje cuando otro PE hace READ_LOCAL)
	READ_RESP_SHARED,     // Respuesta de lectura (compartida)
	READ_RESP_EXCLUSIVE,  // Respuesta de lectura exclusiva (nadie más tiene la línea)
	WRITE_LOCAL,          // Escritura iniciada por el propio PE
	WRITE_REMOTE,         // Escritura iniciada por otro PE (solicita línea para modificar - interconnect debe de generar este mensaje cuando otro PE hace WRITE_LOCAL)
	WRITE_RESP_MODIFIED,  // Respuesta de escritura (modificada)

	NO_LINE,              // Indica que no se tiene la línea (miss, para respuestas del PE remoto al interconnect)
	
	INVALIDATE,           // Solicitud de invalidación
	INV_ACK,              // Acknowledge de invalidación (confirmación)
	BROADCAST_INVALIDATE, // Difusión de invalidación
	INV_COMPLETE          // Notificación de que la invalidación terminó
};

// Estructura para un mensaje 
struct Message {
	MessageType type;               // Tipo de mensaje
	uint8_t src_id;                 // PE/caché origen
	uint8_t dest_id;                // PE/caché destino
	uint8_t request_id;        		// ID del PE que inició la solicitud (para rastrear respuestas)
	uint64_t address;               // Dirección de memoria involucrada
	std::vector<uint8_t> data;
	std::vector<uint8_t> linea_cache;
	
};

class Interconnect {
public:
	Interconnect(size_t fifo_depth = 8);
	void register_cache(uint8_t cache_id, Cache* cache);
	bool enqueue_mssg(uint8_t pe_id, const Message& msg);
	void process_mssg();
	void receive_mssg(const Message& msg);

private:
	std::queue<Message> mssg_queue;
	size_t fifo_depth_ = 8;
	std::mutex queue_mutex_;
	std::unordered_map<uint8_t, Cache*> regs_cache_;
};

#endif // INTERCONNECT_H
