#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/network.h"

using namespace salticidae;

extern "C" {

// MsgNetwork

msgnetwork_config_t *msgnetwork_config_new() {
    return new msgnetwork_config_t();
}

void msgnetwork_config_free(const msgnetwork_config_t *self) { delete self; }

msgnetwork_t *msgnetwork_new(const eventcontext_t *ec, const msgnetwork_config_t *config, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new msgnetwork_t(*ec, *config);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

void msgnetwork_free(const msgnetwork_t *self) { delete self; }

void msgnetwork_config_max_msg_size(msgnetwork_config_t *self, size_t size) {
    self->max_msg_size(size);
}

void msgnetwork_config_max_msg_queue_size(msgnetwork_config_t *self, size_t size) {
    self->max_msg_queue_size(size);
}

void msgnetwork_config_burst_size(msgnetwork_config_t *self, size_t burst_size) {
    self->burst_size(burst_size);
}

void msgnetwork_config_max_listen_backlog(msgnetwork_config_t *self, int backlog) {
    self->max_listen_backlog(backlog);
}

void msgnetwork_config_conn_server_timeout(msgnetwork_config_t *self, double timeout) {
    self->conn_server_timeout(timeout);
}

void msgnetwork_config_recv_chunk_size(msgnetwork_config_t *self, size_t size) {
    self->recv_chunk_size(size);
}

void msgnetwork_config_nworker(msgnetwork_config_t *self, size_t nworker) {
    self->nworker(nworker);
}

void msgnetwork_config_max_recv_buff_size(msgnetwork_config_t *self, size_t size) {
    self->max_recv_buff_size(size);
}

void msgnetwork_config_max_send_buff_size(msgnetwork_config_t *self, size_t size) {
    self->max_send_buff_size(size);
}

void msgnetwork_config_enable_tls(msgnetwork_config_t *self, bool enabled) {
    self->enable_tls(enabled);
}

void msgnetwork_config_tls_key_file(msgnetwork_config_t *self, const char *pem_fname) {
    self->tls_key_file(pem_fname);
}

void msgnetwork_config_tls_cert_file(msgnetwork_config_t *self, const char *pem_fname) {
    self->tls_cert_file(pem_fname);
}

void msgnetwork_config_tls_key_by_move(msgnetwork_config_t *self, pkey_t *key) {
    self->tls_key(new pkey_t(std::move(*key)));
}

void msgnetwork_config_tls_cert_by_move(msgnetwork_config_t *self, x509_t *cert) {
    self->tls_cert(new x509_t(std::move(*cert)));
}

bool msgnetwork_send_msg(msgnetwork_t *self, const msg_t *msg, const msgnetwork_conn_t *conn) {
    return self->_send_msg(*msg, *conn);
}

int32_t msgnetwork_send_msg_deferred_by_move(msgnetwork_t *self,
                                        msg_t *_moved_msg, const msgnetwork_conn_t *conn) {
    return self->_send_msg_deferred(std::move(*_moved_msg), *conn);
}

msgnetwork_conn_t *msgnetwork_connect_sync(msgnetwork_t *self, const netaddr_t *addr, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new msgnetwork_conn_t(self->connect_sync(*addr));
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

int32_t msgnetwork_connect(msgnetwork_t *self, const netaddr_t *addr) {
    return self->connect(*addr);
}

msgnetwork_conn_t *msgnetwork_conn_copy(const msgnetwork_conn_t *self) {
    return new msgnetwork_conn_t(*self);
}


void msgnetwork_conn_free(const msgnetwork_conn_t *self) { delete self; }

void msgnetwork_listen(msgnetwork_t *self, const netaddr_t *listen_addr, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    self->listen(*listen_addr);
    SALTICIDAE_CERROR_CATCH(cerror)
}

void msgnetwork_start(msgnetwork_t *self) { self->start(); }

void msgnetwork_stop(msgnetwork_t *self) { self->stop(); }

void msgnetwork_terminate(msgnetwork_t *self, const msgnetwork_conn_t *conn) {
    self->terminate(*conn);
}

void msgnetwork_reg_handler(msgnetwork_t *self,
                            _opcode_t opcode,
                            msgnetwork_msg_callback_t cb,
                            void *userdata) {
    self->set_handler(opcode,
        [=](const msgnetwork_t::Msg &msg, const msgnetwork_conn_t &conn) {
            cb(&msg, &conn, userdata);
        });
}

void msgnetwork_reg_conn_handler(msgnetwork_t *self,
                                msgnetwork_conn_callback_t cb,
                                void *userdata) {
    self->reg_conn_handler([=](const ConnPool::conn_t &_conn, bool connected) {
        auto conn = salticidae::static_pointer_cast<msgnetwork_t::Conn>(_conn);
        return cb(&conn, connected, userdata);
    });
}

void msgnetwork_reg_error_handler(msgnetwork_t *self,
                                msgnetwork_error_callback_t cb,
                                void *userdata) {
    self->reg_error_handler([=](const std::exception_ptr _err, bool fatal, int32_t async_id) {
        SalticidaeCError cerror;
        try {
            std::rethrow_exception(_err);
        } catch (SalticidaeError &err) {
            cerror = err.get_cerr();
        } catch (...) {
            cerror = salticidae_cerror_unknown();
        }
        cb(&cerror, fatal, async_id, userdata);
    });
}

msgnetwork_t *msgnetwork_conn_get_net(const msgnetwork_conn_t *conn) {
    return (*conn)->get_net();
}

msgnetwork_conn_mode_t msgnetwork_conn_get_mode(const msgnetwork_conn_t *conn) {
    return (msgnetwork_conn_mode_t)(*conn)->get_mode();
}

const netaddr_t *msgnetwork_conn_get_addr(const msgnetwork_conn_t *conn) {
    return &(*conn)->get_addr();
}

const x509_t *msgnetwork_conn_get_peer_cert(const msgnetwork_conn_t *conn) {
    return (*conn)->get_peer_cert();
}

bool msgnetwork_conn_is_terminated(const msgnetwork_conn_t *conn) {
    return (*conn)->is_terminated();
}

// PeerNetwork

void peerid_free(const peerid_t *self) { delete self; }
peerid_t *peerid_new_from_netaddr(const netaddr_t *addr) {
    return new peerid_t(*addr);
}

peerid_t *peerid_new_from_x509(const x509_t *cert) {
    return new peerid_t(*cert);
}

peerid_t *peerid_new_moved_from_uint256(uint256_t *_moved_rawid) {
    return new peerid_t(std::move(*_moved_rawid));
}

peerid_t *peerid_new_copied_from_uint256(const uint256_t *rawid) {
    return new peerid_t(*rawid);
}

peerid_array_t *peerid_array_new() { return new peerid_array_t(); }
peerid_array_t *peerid_array_new_from_peers(const peerid_t * const *peers, size_t npeers) {
    auto res = new peerid_array_t();
    res->resize(npeers);
    for (size_t i = 0; i < npeers; i++)
        (*res)[i] = *peers[i];
    return res;
}

const uint256_t *peerid_as_uint256(const peerid_t *self) { return self; }

void peerid_array_free(const peerid_array_t *self) { delete self; }

peernetwork_config_t *peernetwork_config_new() {
    return new peernetwork_config_t();
}

void peernetwork_config_free(const peernetwork_config_t *self) { delete self; }

void peernetwork_config_ping_period(peernetwork_config_t *self, double t) {
    self->ping_period(t);
}

void peernetwork_config_conn_timeout(peernetwork_config_t *self, double t) {
    self->conn_timeout(t);
}

void peernetwork_config_id_mode(peernetwork_config_t *self, peernetwork_id_mode_t mode) {
    self->id_mode(peernetwork_t::IdentityMode(mode));
}

msgnetwork_config_t *peernetwork_config_as_msgnetwork_config(peernetwork_config_t *self) { return self; }

peernetwork_t *peernetwork_new(const eventcontext_t *ec, const peernetwork_config_t *config, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new peernetwork_t(*ec, *config);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

void peernetwork_free(const peernetwork_t *self) { delete self; }

int32_t peernetwork_add_peer(peernetwork_t *self, const peerid_t *peer) {
    return self->add_peer(*peer);
}

int32_t peernetwork_del_peer(peernetwork_t *self, const peerid_t *peer) {
    return self->del_peer(*peer);
}

int32_t peernetwork_conn_peer(peernetwork_t *self, const peerid_t *peer, int32_t ntry, double retry_delay) {
    return self->conn_peer(*peer, ntry, retry_delay);
}

bool peernetwork_has_peer(const peernetwork_t *self, const peerid_t *peer) {
    return self->has_peer(*peer);
}

const peernetwork_conn_t *peernetwork_get_peer_conn(const peernetwork_t *self,
                                                    const peerid_t *peer,
                                                    SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new peernetwork_conn_t(self->get_peer_conn(*peer));
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

int32_t peernetwork_set_peer_addr(peernetwork_t *self, const peerid_t *peer, const netaddr_t *addr) {
    return self->set_peer_addr(*peer, *addr);
}

msgnetwork_t *peernetwork_as_msgnetwork(peernetwork_t *self) { return self; }

peernetwork_t *msgnetwork_as_peernetwork_unsafe(msgnetwork_t *self) {
    return static_cast<peernetwork_t *>(self);
}

msgnetwork_conn_t *msgnetwork_conn_new_from_peernetwork_conn(const peernetwork_conn_t *conn) {
    return new msgnetwork_conn_t(*conn);
}

peernetwork_conn_t *peernetwork_conn_new_from_msgnetwork_conn_unsafe(const msgnetwork_conn_t *conn) {
    return new peernetwork_conn_t(salticidae::static_pointer_cast<peernetwork_t::Conn>(*conn));
}

peernetwork_conn_t *peernetwork_conn_copy(const peernetwork_conn_t *self) {
    return new peernetwork_conn_t(*self);
}

netaddr_t *peernetwork_conn_get_peer_addr(const peernetwork_conn_t *self) {
    return new netaddr_t((*self)->get_peer_addr());
}

peerid_t *peernetwork_conn_get_peer_id(const peernetwork_conn_t *self) {
    return new peerid_t((*self)->get_peer_id());
}

void peernetwork_conn_free(const peernetwork_conn_t *self) { delete self; }

bool peernetwork_send_msg(peernetwork_t *self, const msg_t * msg, const peerid_t *peer) {
    try {
        self->_send_msg(*msg, *peer);
        return true;
    } catch (...) {
        return false;
    }
}

int32_t peernetwork_send_msg_deferred_by_move(peernetwork_t *self,
                                        msg_t *_moved_msg, const peerid_t *peer) {
    return self->_send_msg_deferred(std::move(*_moved_msg), *peer);
}

int32_t peernetwork_multicast_msg_by_move(peernetwork_t *self,
                                    msg_t *_moved_msg, const peerid_array_t *peers) {
    return self->_multicast_msg(std::move(*_moved_msg), *peers);
}

void peernetwork_listen(peernetwork_t *self, const netaddr_t *listen_addr, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    self->listen(*listen_addr);
    SALTICIDAE_CERROR_CATCH(cerror)
}

void peernetwork_reg_peer_handler(peernetwork_t *self,
                                peernetwork_peer_callback_t cb,
                                void *userdata) {
    self->reg_peer_handler([=](const peernetwork_conn_t &conn, bool connected) {
        cb(&conn, connected, userdata);
    });
}

void peernetwork_reg_unknown_peer_handler(peernetwork_t *self,
                                        peernetwork_unknown_peer_callback_t cb,
                                        void *userdata) {
    self->reg_unknown_peer_handler([=](const NetAddr &claimed_addr, const salticidae::X509 *cert) {
        cb(&claimed_addr, cert, userdata);
    });
}

clientnetwork_t *clientnetwork_new(const eventcontext_t *ec, const msgnetwork_config_t *config, SalticidaeCError *cerror) {
    SALTICIDAE_CERROR_TRY(cerror)
    return new clientnetwork_t(*ec, *config);
    SALTICIDAE_CERROR_CATCH(cerror)
    return nullptr;
}

void clientnetwork_free(const clientnetwork_t *self) { delete self; }

msgnetwork_t *clientnetwork_as_msgnetwork(clientnetwork_t *self) { return self; }

clientnetwork_t *msgnetwork_as_clientnetwork_unsafe(msgnetwork_t *self) {
    return static_cast<clientnetwork_t *>(self);
}

msgnetwork_conn_t *msgnetwork_conn_new_from_clientnetwork_conn(const clientnetwork_conn_t *conn) {
    return new msgnetwork_conn_t(*conn);
}

clientnetwork_conn_t *clientnetwork_conn_new_from_msgnetwork_conn_unsafe(const msgnetwork_conn_t *conn) {
    return new clientnetwork_conn_t(salticidae::static_pointer_cast<clientnetwork_t::Conn>(*conn));
}

clientnetwork_conn_t *clientnetwork_conn_copy(const clientnetwork_conn_t *self) {
    return new clientnetwork_conn_t(*self);
}

void clientnetwork_conn_free(const clientnetwork_conn_t *self) { delete self; }

bool clientnetwork_send_msg(clientnetwork_t *self, const msg_t * msg, const netaddr_t *addr) {
    try {
        self->_send_msg(*msg, *addr);
        return true;
    } catch (...) {
        return false;
    }
}

int32_t clientnetwork_send_msg_deferred_by_move(clientnetwork_t *self, msg_t *_moved_msg, const netaddr_t *addr) {
    return self->_send_msg_deferred(std::move(*_moved_msg), *addr);
}

}

#endif
