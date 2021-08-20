#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/stream.h"

extern "C" {

uint256_t *uint256_new() { return new uint256_t(); }
uint256_t *uint256_new_from_bytes(const uint8_t *arr) {
    try {
        return new uint256_t(arr);
    } catch (...) {
        return nullptr;
    }
}

uint256_t *uint256_new_from_bytearray(const bytearray_t *bytes) {
    try {
        return new uint256_t(*bytes);
    } catch (...) {
        return nullptr;
    }
}

void uint256_free(const uint256_t *self) { delete self; }

bool uint256_is_null(const uint256_t *self) { return self->is_null(); }
bool uint256_is_eq(const uint256_t *a, const uint256_t *b) {
    return *a == *b;
}

void uint256_serialize(const uint256_t *self, datastream_t *s) {
    self->serialize(*s);
}

void uint256_unserialize(uint256_t *self, datastream_t *s) {
    self->unserialize(*s);
}

datastream_t *datastream_new() {
    try {
        return new datastream_t();
    } catch (...) {
        return nullptr;
    }
}

datastream_t *datastream_new_from_bytes(const uint8_t *base, size_t size) {
    try {
        return new datastream_t(base, base + size);
    } catch (...) {
        return nullptr;
    }
}

datastream_t *datastream_new_from_bytearray(const bytearray_t *bytes) {
    try {
        return new datastream_t(*bytes);
    } catch (...) {
        return nullptr;
    }
}

datastream_t *datastream_new_moved_from_bytearray(bytearray_t *bytes) {
    try {
        return new datastream_t(std::move(*bytes));
    } catch (...) {
        return nullptr;
    }
}

void datastream_free(const datastream_t *self) { delete self; }

datastream_t *datastream_copy(const datastream_t *self) {
    try {
        return new datastream_t(*self);
    } catch (...) { return nullptr; }
}

uint8_t *datastream_data(datastream_t *self) { return self->data(); }

void datastream_clear(datastream_t *self) { self->clear(); }

size_t datastream_size(const datastream_t *self) { return self->size(); }

bool datastream_put_u8(datastream_t *self, uint8_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_u16(datastream_t *self, uint16_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_u32(datastream_t *self, uint32_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_u64(datastream_t *self, uint64_t val) { try {*self << val; } catch (...) { return false; } return true; }

bool datastream_put_i8(datastream_t *self, int8_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_i16(datastream_t *self, int16_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_i32(datastream_t *self, int32_t val) { try {*self << val; } catch (...) { return false; } return true; }
bool datastream_put_i64(datastream_t *self, int64_t val) { try {*self << val; } catch (...) { return false; } return true; }

bool datastream_put_data(datastream_t *self, const uint8_t *base, size_t size) {
    try {
        self->put_data(base, base + size);
    } catch (...) { return false; }
    return true;
}

uint8_t datastream_get_u8(datastream_t *self, bool *succ) { uint8_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
uint16_t datastream_get_u16(datastream_t *self, bool *succ) { uint16_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
uint32_t datastream_get_u32(datastream_t *self, bool *succ) { uint32_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
uint64_t datastream_get_u64(datastream_t *self, bool *succ) { uint64_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }

int8_t datastream_get_i8(datastream_t *self, bool *succ) {int8_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
int16_t datastream_get_i16(datastream_t *self, bool *succ) {int16_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
int32_t datastream_get_i32(datastream_t *self, bool *succ) {int32_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }
int64_t datastream_get_i64(datastream_t *self, bool *succ) {int64_t val = 0; try {*self >> val;} catch (...) {*succ = false;} *succ = true; return val; }

const uint8_t *datastream_get_data_inplace(datastream_t *self, size_t len) {
    try {
        return self->get_data_inplace(len);
    } catch (...) {
        return nullptr;
    }
}

uint256_t *datastream_get_hash(const datastream_t *self) {
    try {
        return new uint256_t(self->get_hash());
    } catch (...) {
        return nullptr;
    }
}

bytearray_t *bytearray_new_moved_from_datastream(datastream_t *_moved_src) {
    try {
        return new bytearray_t(std::move(*_moved_src));
    } catch (...) {
        return nullptr;
    }
}

bytearray_t *bytearray_new_copied_from_datastream(datastream_t *src) {
    try {
        return new bytearray_t(*src);
    } catch (...) {
        return nullptr;
    }
}

bytearray_t *bytearray_new_from_hex(const char *hex_str) {
    try {
        return new bytearray_t(salticidae::from_hex(hex_str));
    } catch (...) {
        return nullptr;
    }
}

bytearray_t *bytearray_new_from_bytes(const uint8_t *arr, size_t len) {
    try {
        return new bytearray_t(arr, arr + len);
    } catch (...) {
        return nullptr;
    }
}

char *datastream_get_hex(datastream_t *self) {
    std::string tmp = self->get_hex();
    auto res = (char *)malloc(tmp.length() + 1);
    memmove(res, tmp.c_str(), tmp.length());
    res[tmp.length()] = 0;
    return res;
}

}

#endif
