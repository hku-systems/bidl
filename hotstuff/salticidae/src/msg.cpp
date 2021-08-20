#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/msg.h"

extern "C" {

msg_t *msg_new_moved_from_bytearray(_opcode_t opcode, bytearray_t *_moved_payload) {
    return new msg_t(opcode, std::move(*_moved_payload));
}

void msg_free(msg_t *msg) { delete msg; }

datastream_t *msg_consume_payload(const msg_t *msg) {
    return new datastream_t(msg->get_payload());
}

_opcode_t msg_get_opcode(const msg_t *self) { return self->get_opcode(); }

uint32_t msg_get_magic(const msg_t *self) { return self->get_magic(); }

void msg_set_magic(msg_t *self, uint32_t magic) { return self->set_magic(magic); }

}

#endif
