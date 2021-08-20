/**
 * Copyright (c) 2018 Cornell University.
 *
 * Author: Ted Yin <tederminant@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SALTICIDAE_STREAM_H
#define _SALTICIDAE_STREAM_H

#ifdef __cplusplus
#include "salticidae/type.h"
#include "salticidae/ref.h"
#include "salticidae/crypto.h"

namespace salticidae {

template<size_t N, typename T> class Blob;
using uint256_t = Blob<256, uint64_t>;

class DataStream {
    bytearray_t buffer;
    size_t offset;

    public:
    DataStream(): offset(0) {}
    DataStream(const uint8_t *begin, const uint8_t *end): buffer(begin, end), offset(0) {}
    DataStream(bytearray_t &&data): buffer(std::move(data)), offset(0) {}
    DataStream(const bytearray_t &data): buffer(data), offset(0) {}
    DataStream(const std::string &data):
        DataStream((uint8_t *)data.data(),
                    (uint8_t *)data.data() + data.size()) {}

    DataStream(DataStream &&other):
            buffer(std::move(other.buffer)),
            offset(other.offset) {}

    DataStream(const DataStream &other):
        buffer(other.buffer),
        offset(other.offset) {}

    void swap(DataStream &other) {
        std::swap(buffer, other.buffer);
        std::swap(offset, other.offset);
    }

    DataStream &operator=(const DataStream &other) {
        if (this != &other)
        {
            DataStream tmp(other);
            tmp.swap(*this);
        }
        return *this;
    }

    DataStream &operator=(DataStream &&other) {
        if (this != &other)
        {
            DataStream tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    uint8_t *data() { return &buffer[offset]; }

    void clear() {
        buffer.clear();
        offset = 0;
    }

    size_t size() const {
        return buffer.size() - offset;
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, DataStream &>::type
    operator<<(T d) {
        buffer.resize(buffer.size() + sizeof(T));
        *(reinterpret_cast<T *>(&*buffer.end() - sizeof(T))) = d;
        return *this;
    }

    template<typename T>
    typename std::enable_if<is_ranged<T>::value>::type
    put_data(const T &d) {
        buffer.insert(buffer.end(), d.begin(), d.end());
    }

    DataStream &operator<<(const std::string &d) { put_data(d); return *this; }
    DataStream &operator<<(const bytearray_t &d) { put_data(d); return *this; }

    void put_data(const uint8_t *begin, const uint8_t *end) {
        size_t len = end - begin;
        buffer.resize(buffer.size() + len);
        memmove(&*buffer.end() - len, begin, len);
    }

    const uint8_t *get_data_inplace(size_t len) {
        auto res = (uint8_t *)&*(buffer.begin() + offset);
        offset += len;
#ifndef SALTICIDAE_NOCHECK
        if (offset > buffer.size())
            throw std::ios_base::failure("insufficient buffer");
#endif
        return res;
    }

    template<typename T>
    typename std::enable_if<!std::is_integral<T>::value, DataStream &>::type
    operator<<(const T &obj) {
        obj.serialize(*this);
        return *this;
    }

    DataStream &operator<<(const char *cstr) {
        put_data((uint8_t *)cstr, (uint8_t *)cstr + strlen(cstr));
        return *this;
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, DataStream &>::type
    operator>>(T &d) {
#ifndef SALTICIDAE_NOCHECK
        if (offset + sizeof(T) > buffer.size())
            throw std::ios_base::failure("insufficient buffer");
#endif
        d = *(reinterpret_cast<T *>(&buffer[offset]));
        offset += sizeof(T);
        return *this;
    }

    template<typename T>
    typename std::enable_if<!std::is_integral<T>::value, DataStream &>::type
    operator>>(T &obj) {
        obj.unserialize(*this);
        return *this;
    }

    std::string get_hex() const {
        char buf[3];
        DataStream s;
        for (auto it = buffer.begin() + offset; it != buffer.end(); it++)
        {
            sprintf(buf, "%02x", *it);
            s.put_data((uint8_t *)buf, (uint8_t *)buf + 2);
        }
        return std::string(s.buffer.begin(), s.buffer.end());
    }

    void load_hex(const std::string &hex_str) {
        size_t len = hex_str.size();
        const char *p;
        uint8_t *bp;
        unsigned int tmp;
        if (len & 1)
            throw std::invalid_argument("not a valid hex string");
        buffer.resize(len >> 1);
        offset = 0;
        for (p = hex_str.data(), bp = &*buffer.begin();
            p < hex_str.data() + len; p += 2, bp++)
        {
            if (sscanf(p, "%02x", &tmp) != 1)
                throw std::invalid_argument("not a valid hex string");
            *bp = tmp;
        }
    }

    operator bytearray_t () const & {
        return bytearray_t(buffer.begin() + offset, buffer.end());
    }

    operator bytearray_t () && {
        return buffer;
    }

    operator std::string () const & {
        return std::string(buffer.begin() + offset, buffer.end());
    }

    inline uint256_t get_hash() const;
};

class Serializable {
    public:
    virtual ~Serializable() = default;
    virtual void serialize(DataStream &s) const = 0;
    virtual void unserialize(DataStream &s) = 0;

    virtual void from_bytes(const bytearray_t &raw_bytes) {
        DataStream s(raw_bytes);
        s >> *this;
    }

    virtual void from_bytes(bytearray_t &&raw_bytes) {
        DataStream s(std::move(raw_bytes));
        s >> *this;
    }


    virtual void from_hex(const std::string &hex_str) {
        DataStream s;
        s.load_hex(hex_str);
        s >> *this;
    }

    bytearray_t to_bytes() const {
        DataStream s;
        s << *this;
        return bytearray_t(std::move(s));
    }

    inline std::string to_hex() const;
};

template<size_t N, typename T = uint64_t>
class Blob: public Serializable {
    using _impl_type = T;
    static const size_t bit_per_datum = sizeof(_impl_type) * 8;
    static_assert(!(N % bit_per_datum), "N must be divisible by bit_per_datum");
    static const auto _len = N / bit_per_datum;
    _impl_type data[_len];
    bool loaded;

    public:

    Blob(): loaded(false) { memset(data, 0, sizeof(data)); }
    Blob(const bytearray_t &arr) {
        if (arr.size() != N / 8)
            throw std::invalid_argument("incorrect Blob size");
        load(&*arr.begin());
    }

    Blob(const uint8_t *arr) { load(arr); }

    void load(const uint8_t *arr) {
        arr += N / 8;
        for (_impl_type *ptr = data + _len; ptr > data;)
        {
            _impl_type x = 0;
            for (unsigned j = 0; j < sizeof(_impl_type); j++)
                x = (x << 8) | *(--arr);
            *(--ptr) = x;
        }
        loaded = true;
    }

    bool is_null() const { return !loaded; }

    bool operator==(const Blob<N> &other) const {
        for (size_t i = 0; i < _len; i++)
            if (data[i] != other.data[i])
                return false;
        return true;
    }

    bool operator!=(const Blob<N> &other) const {
        return !(*this == other);
    }

    bool operator<(const Blob<N> &other) const {
        for (size_t i = _len - 1; i > 0; i--)
            if (data[i] != other.data[i])
                return data[i] < other.data[i];
        return data[0] < other.data[0];
    }

    size_t cheap_hash() const { return *data; }

    void serialize(DataStream &s) const override {
        if (loaded)
        {
            for (const _impl_type *ptr = data; ptr < data + _len; ptr++)
                s << htole(*ptr);
        }
        else
        {
            for (const _impl_type *ptr = data; ptr < data + _len; ptr++)
                s << htole((_impl_type)0);
        }
    }

    void unserialize(DataStream &s) override {
        for (_impl_type *ptr = data; ptr < data + _len; ptr++)
        {
            _impl_type x;
            s >> x;
            *ptr = letoh(x);
        }
        loaded = true;
    }

    operator bytearray_t () const & {
        DataStream s;
        s << *this;
        return bytearray_t(std::move(s));
    }
};

template<typename T = uint64_t>
class _Bits {
    using _impl_type = T;
    static const uint32_t bit_per_datum = sizeof(_impl_type) * 8;
    static const uint32_t shift_per_datum = log2<bit_per_datum>::value;
    static_assert(bit_per_datum == 1 << shift_per_datum, "int type must have 2^n bits");
    BoxObj<_impl_type[]> data;
    uint32_t nbits;
    uint32_t ndata;

    public:

    _Bits(): data(nullptr) {}
    _Bits(const bytearray_t &arr) {
        load(&*arr.begin(), arr.size());
    }

    _Bits(const _Bits &other): nbits(other.nbits), ndata(other.ndata) {
        data = new _impl_type[ndata];
        memmove(data.get(), other.data.get(), ndata * sizeof(_impl_type));
    }

    _Bits(const uint8_t *arr, uint32_t len) { load(arr, len); }
    _Bits(uint32_t nbits): nbits(nbits) {
        ndata = (nbits + bit_per_datum - 1) / bit_per_datum;
        data = new _impl_type[ndata];
    }

    ~_Bits() {}

    void load(const uint8_t *arr, uint32_t len) {
        nbits = len * 8;
        ndata = (nbits + bit_per_datum - 1) / bit_per_datum;
        data = new _impl_type[ndata];

        const uint8_t *end = arr + len;
        for (_impl_type *ptr = data.get(); ptr < data.get() + ndata;)
        {
            _impl_type x = 0;
            for (unsigned j = 0, k = 0; j < sizeof(_impl_type); j++, k += 8)
                if (arr < end) x |= *(arr++) << k;
            *(ptr++) = x;
        }
    }

    bool is_null() const { return data == nullptr; }

    size_t cheap_hash() const { return *data; }

    void serialize(DataStream &s) const {
        s << htole(nbits);
        if (data)
        {
            for (const _impl_type *ptr = data.get(); ptr < data.get() + ndata; ptr++)
                s << htole(*ptr);
        }
        else
        {
            for (const _impl_type *ptr = data.get(); ptr < data.get() + ndata; ptr++)
                s << htole((_impl_type)0);
        }
    }

    void unserialize(DataStream &s) {
        s >> nbits;
        nbits = letoh(nbits);
        ndata = (nbits + bit_per_datum - 1) / bit_per_datum;
        data = new _impl_type[ndata];
        for (_impl_type *ptr = data.get(); ptr < data.get() + ndata; ptr++)
        {
            _impl_type x;
            s >> x;
            *ptr = letoh(x);
        }
    }

    operator bytearray_t () const & {
        DataStream s;
        s << *this;
        return bytearray_t(std::move(s));
    }

    uint8_t get(uint32_t idx) const {
        return (data[idx >> shift_per_datum] >>
                    (idx & (bit_per_datum - 1))) & 1;
    }

    void set(uint32_t idx) {
        auto i = idx >> shift_per_datum;
        auto pos = idx & (bit_per_datum - 1);
        data[i] ^= (((data[i] >> pos) & 1) ^ 1) << pos;
    }

    void unset(uint32_t idx) {
        auto i = idx >> shift_per_datum;
        auto pos = idx & (bit_per_datum - 1);
        data[i] ^= ((data[i] >> pos) & 1) << pos;
    }

    void flip(uint32_t idx) {
        auto i = idx >> shift_per_datum;
        auto pos = idx & (bit_per_datum - 1);
        data[i] ^= ((_impl_type)1) << pos;
    }

    void clear() {
        memset(data.get(), 0, ndata * sizeof(_impl_type));
    }
   
    uint8_t operator[](uint32_t idx) const { return get(idx); }

    uint32_t size() const { return nbits; }
};

using Bits = _Bits<>;

const size_t ENT_HASH_LENGTH = 256 / 8;

uint256_t DataStream::get_hash() const {
    class SHA256 d;
    d.update(buffer.begin() + offset, size());
    return d.digest();
}

template<typename T> inline uint256_t get_hash(const T &x) {
    DataStream s;
    s << x;
    return s.get_hash();
}

template<typename T> inline std::string get_hex(const T &x) {
    DataStream s;
    s << x;
    return s.get_hex();
}

inline bytearray_t from_hex(const std::string &hex_str) {
    DataStream s;
    s.load_hex(hex_str);
    return bytearray_t(std::move(s));
}

inline std::string Serializable::to_hex() const { return get_hex(*this); }

}

namespace std {
    template <>
    struct hash<salticidae::uint256_t> {
        size_t operator()(const salticidae::uint256_t &k) const {
            return (size_t)k.cheap_hash();
        }
    };

    template <>
    struct hash<const salticidae::uint256_t> {
        size_t operator()(const salticidae::uint256_t &k) const {
            return (size_t)k.cheap_hash();
        }
    };
}

#ifdef SALTICIDAE_CBINDINGS
using uint256_t = salticidae::uint256_t;
using datastream_t = salticidae::DataStream;
#endif

#else

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "type.h"

#ifdef SALTICIDAE_CBINDINGS
typedef struct datastream_t datastream_t;
typedef struct uint256_t uint256_t;
#endif

#endif

#ifdef SALTICIDAE_CBINDINGS
#ifdef __cplusplus
extern "C" {
#endif

uint256_t *uint256_new();
uint256_t *uint256_new_from_bytes(const uint8_t *arr);
uint256_t *uint256_new_from_bytearray(const bytearray_t *bytes);
void uint256_free(const uint256_t *self);

bool uint256_is_null(const uint256_t *self);
bool uint256_is_eq(const uint256_t *a, const uint256_t *b);
void uint256_serialize(const uint256_t *self, datastream_t *s);
void uint256_unserialize(uint256_t *self, datastream_t *s);

datastream_t *datastream_new();
datastream_t *datastream_new_from_bytes(const uint8_t *base, size_t size);
datastream_t *datastream_new_from_bytearray(const bytearray_t *bytes);
datastream_t *datastream_new_moved_from_bytearray(bytearray_t *bytes);
void datastream_free(const datastream_t *self);

datastream_t *datastream_copy(const datastream_t *self);
uint8_t *datastream_data(datastream_t *self);
void datastream_clear(datastream_t *self);
size_t datastream_size(const datastream_t *self);

bool datastream_put_u8(datastream_t *self, uint8_t val);
bool datastream_put_u16(datastream_t *self, uint16_t val);
bool datastream_put_u32(datastream_t *self, uint32_t val);
bool datastream_put_u64(datastream_t *self, uint64_t val);

bool datastream_put_i8(datastream_t *self, int8_t val);
bool datastream_put_i16(datastream_t *self, int16_t val);
bool datastream_put_i32(datastream_t *self, int32_t val);
bool datastream_put_i64(datastream_t *self, int64_t val);
bool datastream_put_data(datastream_t *self, const uint8_t *base, size_t size);

uint8_t datastream_get_u8(datastream_t *self, bool *succ);
uint16_t datastream_get_u16(datastream_t *self, bool *succ);
uint32_t datastream_get_u32(datastream_t *self, bool *succ);
uint64_t datastream_get_u64(datastream_t *self, bool *succ);

int8_t datastream_get_i8(datastream_t *self, bool *succ);
int16_t datastream_get_i16(datastream_t *self, bool *succ);
int32_t datastream_get_i32(datastream_t *self, bool *succ);
int64_t datastream_get_i64(datastream_t *self, bool *succ);

const uint8_t *datastream_get_data_inplace(datastream_t *self, size_t len);
uint256_t *datastream_get_hash(const datastream_t *self);
bytearray_t *bytearray_new_moved_from_datastream(datastream_t *_moved_self);
bytearray_t *bytearray_new_copied_from_datastream(datastream_t *src);
bytearray_t *bytearray_new_from_hex(const char *hex_str);
bytearray_t *bytearray_new_from_bytes(const uint8_t *arr, size_t len);

char *datastream_get_hex(datastream_t *self);

#ifdef __cplusplus
}
#endif
#endif

#endif
