#ifndef BUS_TYPES_H
#define BUS_TYPES_H

enum MessageType {
    READ_MISS,
    WRITE_MISS,
    INVALIDATE,
    WRITE_BACK,
    FLUSH,
    READ,
    WRITE,
};

struct BusMessage {
    int sender_id;
    MessageType type;
    size_t address;
    double data;
    array<double, 4> cache_line;
};

#endif
