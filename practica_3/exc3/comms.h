#ifndef COMMS_H
#define COMMS_H

#include <sys/types.h>

#define KEY_PATHNAME "key_file"
#define KEY_ID 'K'

struct comms_data {
    pid_t pid_b;      // Proceso B escribirá su PID aquí
    char mensaje[256]; // Proceso A escribirá su mensaje aquí
};

#endif
