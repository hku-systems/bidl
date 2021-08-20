#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/type.h"

extern "C" {

bytearray_t *bytearray_new() { return new bytearray_t(); }

void bytearray_free(bytearray_t *arr) { delete arr; }

uint8_t *bytearray_data(bytearray_t *arr) { return &(*arr)[0]; }

size_t bytearray_size(bytearray_t *arr) { return arr->size(); }

}

#endif
