#ifndef PZEM_H
#define PZEM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float voltage;
    float current;
    float power;
    uint32_t energy;
} pzem_data_t;

bool pzem_read(pzem_data_t *data);

#endif // PZEM_H