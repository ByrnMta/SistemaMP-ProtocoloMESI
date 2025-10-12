#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <queue>
#include <mutex>
#include <unordered_map>

// Declaraciones adelantadas
class Cache;                        // Representa la caché privada de un PE

// Enum para los tipos de mensaje MESI
enum class MessageType {
	READ,                           // Solicitud de lectura
	READ_RESP,                      // Respuesta de lectura
	WRITE,                          // Solicitud de escritura
	WRITE_RESP,                     // Respuesta de escritura
	INVALIDATE,                     // Solicitud de invalidación
	INV_ACK,                        // Acknowledge de invalidación (confirmación)
	BROADCAST_INVALIDATE,           // Difusión de invalidación
	INV_COMPLETE                    // Notificación de que la invalidación terminó
};

// Estructura básica para un mensaje MESI
struct Message {
	MessageType type;               // Tipo de mensaje
	uint8_t src_id;                 // PE/caché origen
	uint8_t dest_id;                // PE/caché destino
	uint64_t address;               // Dirección de memoria involucrada
	std::vector<uint8_t> data;      // Datos (si aplica)
	// Se pueden agregar campos para indicar el tiempo de simulación y el estado del mensaje (para debugging)
};

// Interconnect: Maneja la arbitraje, el paso de mensajes y la sincronización entre los PEs y la memoria
class Interconnect {
public:
	Interconnect(); // Constructor: inicializa el interconnect

    // Métodos para gestión de hilo propio
	void start();
	void stop();

	// Registra una caché (PE) en el interconnect
	void register_cache(uint8_t cache_id, Cache* cache);

	// Ingresa a la cola un mensaje desde un PE para ser procesado por el interconnect
	void enqueue_mssg(uint8_t pe_id, const Message& msg);

	// Procesa el siguiente mensaje en la cola (simula acceso al bus/memoria)
	void process_mssg();

	// Recibe un mensaje de un PE u otro componente
	void receive_mssg(const Message& msg);

private:
	// Puntero a la memoria principal (por integrar más adelante)
	// FileMemory* main_memory_;
    
    // Cola FIFO para mensajes pendientes
	std::queue<Message> mssg_queue; 
    
    // Mutex para sincronizar el acceso a la cola de mensajes
	std::mutex queue_mutex_;
    
    // Mapa de IDs de caché a punteros de caché (PEs registrados)                                
	std::unordered_map<uint8_t, Cache*> regs_cache;

    // Flag de control para el ciclo de vida
    bool running_ = false;
    
    // Hilo propio del Interconnect
	std::thread thread_;                

    // Método principal del ciclo de vida
	void run();
};

#endif // INTERCONNECT_H
