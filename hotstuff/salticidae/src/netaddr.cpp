#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/netaddr.h"

using namespace salticidae;

extern "C" {

netaddr_t *netaddr_new() { return new netaddr_t(); }

void netaddr_free(const netaddr_t *self) { delete self; }

netaddr_t *netaddr_new_from_ip_port(uint32_t ip, uint16_t port, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new netaddr_t(ip, port);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

netaddr_t *netaddr_new_from_sip_port(const char *ip, uint16_t port, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new netaddr_t(ip, port);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

netaddr_t *netaddr_new_from_sipport(const char *ip_port_addr, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new netaddr_t(ip_port_addr);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

netaddr_t *netaddr_copy(const netaddr_t *self) {
    return new netaddr_t(*self);
}

bool netaddr_is_eq(const netaddr_t *a, const netaddr_t *b) {
    return *a == *b;
}

bool netaddr_is_null(const netaddr_t *self) { return self->is_null(); }

uint32_t netaddr_get_ip(const netaddr_t *self) { return self->ip; }

uint16_t netaddr_get_port(const netaddr_t *self) { return self->port; }

netaddr_array_t *netaddr_array_new() { return new netaddr_array_t(); }
netaddr_array_t *netaddr_array_new_from_addrs(const netaddr_t * const *addrs, size_t naddrs) {
    auto res = new netaddr_array_t();
    res->resize(naddrs);
    for (size_t i = 0; i < naddrs; i++)
        (*res)[i] = *addrs[i];
    return res;
}

void netaddr_array_free(netaddr_array_t *self) { delete self; }

}

#endif
