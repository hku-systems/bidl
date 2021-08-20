/**
 * Copyright (c) 2019 Ava Labs, Inc.
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

#include <cstdio>
#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>
#include <signal.h>

#include "salticidae/msg.h"
#include "salticidae/event.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"

using salticidae::NetAddr;
using salticidae::DataStream;
using salticidae::htole;
using salticidae::letoh;
using salticidae::EventContext;
using salticidae::ThreadCall;
using salticidae::PKey;
using std::placeholders::_1;
using std::placeholders::_2;

using PeerNetwork = salticidae::PeerNetwork<uint8_t>;

struct MsgText {
    static const uint8_t opcode = 0x0;
    DataStream serialized;
    uint64_t id;
    std::string text;

    MsgText(uint64_t id, const std::string &text) {
        serialized << salticidae::htole(id) << salticidae::htole((uint32_t)text.length()) << text;
    }

    MsgText(DataStream &&s) {
        uint32_t len;
        s >> id;
        id = salticidae::letoh(id);
        s >> len;
        len = salticidae::letoh(len);
        text = std::string((const char *)s.get_data_inplace(len), len);
    }
};

const uint8_t MsgText::opcode;

void masksigs() {
	sigset_t mask;
	sigemptyset(&mask);
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

std::unordered_set<uint256_t> valid_certs;

struct Net {
    uint64_t id;
    EventContext ec;
    ThreadCall tc;
    std::thread th;
    PeerNetwork *net;
    const std::string listen_addr;
    uint256_t cert;

    Net(uint64_t id, uint16_t port): id(id), tc(ec), listen_addr("127.0.0.1:"+ std::to_string(port)) {
        auto tls_key = new PKey(PKey::create_privkey_rsa(2048));
        auto tls_cert = new salticidae::X509(salticidae::X509::create_self_signed_from_pubkey(*tls_key));
        tls_key->save_privkey_to_file(std::to_string(port) + "_pkey.pem");
        tls_cert->save_to_file(std::to_string(port) + ".pem");
        cert = salticidae::get_hash(tls_cert->get_der());
        valid_certs.insert(cert);
        net = new PeerNetwork(ec, PeerNetwork::Config(salticidae::ConnPool::Config()
                    .enable_tls(true)
                    .tls_key(tls_key)
                    .tls_cert(tls_cert)
                ).conn_timeout(5)
                .ping_period(2)
                .id_mode(PeerNetwork::IdentityMode::ADDR_BASED));
        net->reg_handler([this](const MsgText &msg, const PeerNetwork::conn_t &) {
            fprintf(stdout, "net %lu: peer %lu says %s\n", this->id, msg.id, msg.text.c_str());
        });
        net->reg_conn_handler([this](const salticidae::ConnPool::conn_t &conn, bool connected) {
            if (connected)
            {
                auto cert_hash = salticidae::get_hash(conn->get_peer_cert()->get_der());
                return valid_certs.count(cert_hash) > 0;
            }
            return true;
        });
        net->reg_error_handler([this](const std::exception_ptr _err, bool fatal, int32_t async_id) {
            try {
                std::rethrow_exception(_err);
            } catch (const std::exception &err) {
                fprintf(stdout, "net %lu: captured %s error during an async call %d: %s\n",
                        this->id, fatal ? "fatal" : "recoverable", async_id, err.what());
            }
        });
        net->reg_peer_handler([this](const PeerNetwork::conn_t &conn, bool connected) {
            fprintf(stdout, "net %lu: %s peer %s (cert: %s)\n", this->id,
                    connected ? "connected to" : "disconnected from",
                    std::string(conn->get_peer_addr()).c_str(),
                    salticidae::get_hash(conn->get_peer_cert()->get_der()).to_hex().c_str());
        });
        net->reg_unknown_peer_handler([this](const NetAddr &claimed_addr, const salticidae::X509 *) {
            fprintf(stdout, "net %lu: unknown peer %s attempts to connnect\n",
                    this->id, std::string(claimed_addr).c_str());
        });
        th = std::thread([=](){
            masksigs();
            try {
                net->start();
                net->listen(NetAddr(listen_addr));
                fprintf(stdout, "net %lu: listen to %s\n", id, listen_addr.c_str());
                ec.dispatch();
            } catch (std::exception &err) {
                fprintf(stdout, "net %lu: got error during a sync call: %s\n", id, err.what());
            }
            fprintf(stdout, "net %lu: main loop ended\n", id);
        });
    }

    void add_peer(const std::string &listen_addr) {
        try {
            net->add_peer(NetAddr(listen_addr));
        } catch (std::exception &err) {
            fprintf(stdout, "net %lu: got error during a sync call: %s\n", id, err.what());
        }
    }

    void set_peer_addr(const NetAddr &addr) {
        try {
            net->set_peer_addr(addr, addr);
        } catch (std::exception &err) {
            fprintf(stdout, "net %lu: got error during a sync call: %s\n", id, err.what());
        }
    }

    void conn_peer(const NetAddr &addr) {
        try {
            net->conn_peer(addr);
        } catch (std::exception &err) {
            fprintf(stdout, "net %lu: got error during a sync call: %s\n", id, err.what());
        }
    }

    void del_peer(const std::string &listen_addr) {
        try {
            net->del_peer(NetAddr(listen_addr));
        } catch (std::exception &err) {
            fprintf(stdout, "net %lu: got error during a sync call: %s\n", id, err.what());
        }
    }

    void stop_join() {
        tc.async_call([ec=this->ec](ThreadCall::Handle &) { ec.stop(); });
        th.join();
    }

    ~Net() {
        valid_certs.erase(cert);
        delete net;
    }
};

std::unordered_map<uint64_t, Net *> nets;
std::unordered_map<std::string, std::function<void(char *)> > cmd_map;

int read_int(char *buff) {
    scanf("%64s", buff);
    try {
        int t = std::stoi(buff);
        if (t < 0) throw std::invalid_argument("negative");
        return t;
    } catch (std::invalid_argument) {
        fprintf(stdout, "expect a non-negative integer\n");
        return -1;
    }
}

int main(int argc, char **argv) {
    int i;
    fprintf(stdout, "p2p network library playground (type help for more info)\n");
    fprintf(stdout, "========================================================\n");

    auto cmd_exit = [](char *) {
        for (auto &p: nets)
        {
            p.second->stop_join();
            delete p.second;
        }
        exit(0);
    };

    auto cmd_add = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        if (nets.count(id))
        {
            fprintf(stdout, "net id already exists\n");
            return;
        }
        int port = read_int(buff);
        if (port < 0) return;
        if (port >= 65536)
        {
            fprintf(stdout, "port should be < 65536\n");
            return;
        }
        nets.insert(std::make_pair(id, new Net(id, port)));
    };

    auto cmd_ls = [](char *) {
        for (auto &p: nets)
            fprintf(stdout, "%d -> %s\n", p.first, p.second->listen_addr.c_str());
    };

    auto cmd_del = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        it->second->stop_join();
        delete it->second;
        nets.erase(it);
    };

    auto cmd_addpeer = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        int id2 = read_int(buff);
        if (id2 < 0) return;
        auto it2 = nets.find(id2);
        if (it2 == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        it->second->add_peer(it2->second->listen_addr);
    };

    auto cmd_delpeer = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        int id2 = read_int(buff);
        if (id2 < 0) return;
        auto it2 = nets.find(id2);
        if (it2 == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        it->second->del_peer(it2->second->listen_addr);
    };

    auto cmd_setpeeraddr = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        int id2 = read_int(buff);
        if (id2 < 0) return;
        auto it2 = nets.find(id2);
        if (it2 == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        it->second->set_peer_addr(it2->second->listen_addr);
    };

    auto cmd_connpeer = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        int id2 = read_int(buff);
        if (id2 < 0) return;
        auto it2 = nets.find(id2);
        if (it2 == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        it->second->conn_peer(it2->second->listen_addr);
    };

    auto cmd_msg = [](char *buff) {
        int id = read_int(buff);
        if (id < 0) return;
        auto it = nets.find(id);
        if (it == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        int id2 = read_int(buff);
        if (id2 < 0) return;
        auto it2 = nets.find(id2);
        if (it2 == nets.end())
        {
            fprintf(stdout, "net id does not exist\n");
            return;
        }
        scanf("%64s", buff);
        it->second->net->send_msg(MsgText(id, buff), NetAddr(it2->second->listen_addr));
    };

    auto cmd_sleep = [](char *buff) {
        int sec = read_int(buff);
        if (sec < 0) return;
        sleep(sec);
    };

    auto cmd_help = [](char *) {
        fprintf(stdout,
            "add <node-id> <port> -- start a node (create a PeerNetwork instance)\n"
            "addpeer <node-id> <peer-id> -- add a peer to a given node\n"
            "setpeeraddr <node-id> <peer-id> -- set the peer addr\n"
            "connpeer <node-id> <peer-id> -- try to connect to the peer\n"
            "delpeer <node-id> <peer-id> -- add a peer to a given node\n"
            "del <node-id> -- remove a node (destroy a PeerNetwork instance)\n"
            "msg <node-id> <peer-id> <msg> -- send a text message to a node\n"
            "ls -- list all node ids\n"
            "sleep <sec> -- wait for some seconds\n"
            "exit -- quit the program\n"
            "help -- show this info\n"
        );
    };

    cmd_map.insert(std::make_pair("add", cmd_add));
    cmd_map.insert(std::make_pair("addpeer", cmd_addpeer));
    cmd_map.insert(std::make_pair("setpeeraddr", cmd_setpeeraddr));
    cmd_map.insert(std::make_pair("connpeer", cmd_connpeer));
    cmd_map.insert(std::make_pair("del", cmd_del));
    cmd_map.insert(std::make_pair("delpeer", cmd_delpeer));
    cmd_map.insert(std::make_pair("msg", cmd_msg));
    cmd_map.insert(std::make_pair("ls", cmd_ls));
    cmd_map.insert(std::make_pair("sleep", cmd_sleep));
    cmd_map.insert(std::make_pair("exit", cmd_exit));
    cmd_map.insert(std::make_pair("help", cmd_help));

    bool is_tty = isatty(0);
    for (;;)
    {
        if (is_tty) fprintf(stdout, "> ");
        char buff[128];
        if (scanf("%64s", buff) == EOF) break;
        auto it = cmd_map.find(buff);
        if (it == cmd_map.end())
            fprintf(stdout, "invalid comand \"%s\"\n", buff);
        else
            (it->second)(buff);
    }

    return 0;
}
