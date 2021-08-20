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

#include <cstdio>
#include <string>
#include <functional>
#include <openssl/rand.h>

#include "salticidae/msg.h"
#include "salticidae/event.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"

using salticidae::NetAddr;
using salticidae::DataStream;
using salticidae::MsgNetwork;
using salticidae::ConnPool;
using salticidae::TimerEvent;
using salticidae::EventContext;
using salticidae::htole;
using salticidae::letoh;
using salticidae::bytearray_t;
using salticidae::uint256_t;
using salticidae::static_pointer_cast;
using salticidae::Config;
using salticidae::ThreadCall;
using salticidae::BoxObj;
using salticidae::PKey;
using std::placeholders::_1;
using std::placeholders::_2;

struct MsgRand {
    static const uint8_t opcode = 0x0;
    DataStream serialized;
    uint32_t view;
    bytearray_t bytes;
    uint256_t hash;
    MsgRand(uint32_t _view, size_t size) {
        bytearray_t bytes;
        bytes.resize(size);
        RAND_bytes(&bytes[0], size);
        hash = salticidae::get_hash(bytes);
        serialized << htole(_view) << std::move(bytes);
    }
    MsgRand(DataStream &&s) {
        s >> view;
        view = letoh(view);
        bytes = s;
    }
};

struct MsgAck {
    static const uint8_t opcode = 0x1;
    DataStream serialized;
    uint32_t view;
    uint256_t hash;
    MsgAck(uint32_t _view, const uint256_t &hash) {
        serialized << htole(_view) << hash;
    }
    MsgAck(DataStream &&s) {
        s >> view >> hash;
        view = letoh(view);
    }
};

const uint8_t MsgRand::opcode;
const uint8_t MsgAck::opcode;

using MyNet = salticidae::PeerNetwork<uint8_t>;

bool use_tls;
std::unordered_set<uint256_t> valid_certs;
std::vector<NetAddr> addrs;

struct TestContext {
    TimerEvent timer;
    int state;
    uint32_t view;
    uint256_t hash;
    size_t ncompleted;
    TestContext(): view(0), ncompleted(0) {}
};

struct AppContext {
    NetAddr addr;
    EventContext ec;
    BoxObj<MyNet> net;
    BoxObj<ThreadCall> tcall;
    std::unordered_map<NetAddr, TestContext> tc;
};

void install_proto(AppContext &app, const size_t &recv_chunk_size) {
    auto &ec = app.ec;
    auto &net = *app.net;
    auto send_rand = [&](int size, const MyNet::conn_t &conn, TestContext &tc) {
        MsgRand msg(++tc.view, size);
        tc.hash = msg.hash;
        net.send_msg(std::move(msg), conn);
    };
    net.reg_conn_handler([](const ConnPool::conn_t &conn, bool connected) {
        if (connected && use_tls)
        {
            auto cert_hash = salticidae::get_hash(conn->get_peer_cert()->get_der());
            return valid_certs.count(cert_hash) > 0;
        }
        return true;
    });
    net.reg_peer_handler([&, send_rand](const MyNet::conn_t &conn, bool connected) {
        if (connected)
        {
            auto addr = conn->get_peer_addr();
            if (addr.is_null()) return;
            auto &tc = app.tc[addr];
            tc.state = 1;
            send_rand(tc.state, conn, tc);
        }
    });
    net.reg_error_handler([ec](const std::exception_ptr _err, bool fatal, int32_t async_id) {
        try {
            std::rethrow_exception(_err);
        } catch (const std::exception & err) {
            SALTICIDAE_LOG_WARN("captured %s error during async call %d: %s",
                fatal ? "fatal" : "recoverable", async_id, err.what());
        }
    });
    net.reg_handler([&](MsgRand &&msg, const MyNet::conn_t &conn) {
        uint256_t hash = salticidae::get_hash(msg.bytes);
        net.send_msg(MsgAck(msg.view, hash), conn);
    });
    net.reg_handler([&, send_rand](MsgAck &&msg, const MyNet::conn_t &conn) {
        auto addr = conn->get_peer_addr();
        if (addr.is_null()) return;
        auto &tc = app.tc[addr];
        if (msg.view != tc.view)
        {
            SALTICIDAE_LOG_WARN("dropping MsgAck from a stale view");
            return;
        }
        if (msg.hash != tc.hash)
        {
            SALTICIDAE_LOG_ERROR("%s corrupted I/O: from=%s view=%d state=%d",
                std::string(app.addr).c_str(),
                std::string(addr).c_str(), msg.view, tc.state);
            exit(1);
        }

        if (tc.state == recv_chunk_size * 2)
        {
            send_rand(tc.state, conn, tc);
            tc.state = -1;
            tc.timer = TimerEvent(ec, [&, conn](TimerEvent &) {
                tc.ncompleted++;
                net.terminate(conn);
                std::string s;
                for (const auto &p: app.tc)
                    s += salticidae::stringprintf(" %d(%d)", ntohs(p.first.port), p.second.ncompleted);
                std::string id_hex = salticidae::get_hex10(net.get_peer_id());
                SALTICIDAE_LOG_INFO("%s(%d) completed:%s", id_hex.c_str(), ntohs(app.addr.port), s.c_str());
                SALTICIDAE_LOG_INFO("%s(%d) npending: %lu", id_hex.c_str(), ntohs(app.addr.port), net.get_npending());
            });
            double t = rand() / (double)RAND_MAX * 10;
            tc.timer.add(t);
            SALTICIDAE_LOG_INFO("rand-bomboard phase, ending in %.2f secs", t);
        }
        else if (tc.state == -1)
            send_rand(rand() % (recv_chunk_size * 10), conn, tc);
        else
            send_rand(++tc.state, conn, tc);
    });
}

void masksigs() {
	sigset_t mask;
	sigemptyset(&mask);
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

int main(int argc, char **argv) {
    Config config;
    auto opt_no_msg = Config::OptValFlag::create(false);
    auto opt_npeers = Config::OptValInt::create(5);
    auto opt_recv_chunk_size = Config::OptValInt::create(4096);
    auto opt_nworker = Config::OptValInt::create(2);
    auto opt_conn_timeout = Config::OptValDouble::create(5);
    auto opt_ping_peroid = Config::OptValDouble::create(2);
    auto opt_tls = Config::OptValFlag::create(false);
    auto opt_help = Config::OptValFlag::create(false);
    config.add_opt("no-msg", opt_no_msg, Config::SWITCH_ON);
    config.add_opt("npeers", opt_npeers, Config::SET_VAL);
    config.add_opt("seg-buff-size", opt_recv_chunk_size, Config::SET_VAL);
    config.add_opt("nworker", opt_nworker, Config::SET_VAL);
    config.add_opt("conn-timeout", opt_conn_timeout, Config::SET_VAL);
    config.add_opt("ping-period", opt_ping_peroid, Config::SET_VAL);
    config.add_opt("tls", opt_tls, Config::SWITCH_ON, 't');
    config.add_opt("help", opt_help, Config::SWITCH_ON, 'h', "show this help info");
    config.parse(argc, argv);
    if (opt_help->get())
    {
        config.print_help();
        exit(0);
    }
    size_t recv_chunk_size = opt_recv_chunk_size->get();
    for (int i = 0; i < opt_npeers->get(); i++)
        addrs.push_back(NetAddr("127.0.0.1:" + std::to_string(12345 + i)));
    std::vector<AppContext> apps;
    std::vector<std::thread> threads;
    use_tls = opt_tls->get();
    apps.resize(addrs.size());
    for (size_t i = 0; i < apps.size(); i++)
    {
        auto &a = apps[i];
        a.addr = addrs[i];
        salticidae::ConnPool::Config cfg{};
        if (use_tls)
        {
            auto tls_key = new PKey(PKey::create_privkey_rsa(2048));
            auto tls_cert = new salticidae::X509(salticidae::X509::create_self_signed_from_pubkey(*tls_key));
            cfg.enable_tls(true)
                .tls_key(tls_key)
                .tls_cert(tls_cert);
            valid_certs.insert(salticidae::get_hash(tls_cert->get_der()));
        }
        else cfg.enable_tls(false);
        a.net = new MyNet(a.ec, MyNet::Config(cfg
                    .nworker(opt_nworker->get())
                    .recv_chunk_size(recv_chunk_size))
                        .conn_timeout(opt_conn_timeout->get())
                        .ping_period(opt_ping_peroid->get())
                        .max_msg_size(65536));
        a.tcall = new ThreadCall(a.ec);
        if (!opt_no_msg->get())
            install_proto(a, recv_chunk_size);
        a.net->start();
    }

    for (auto &a: apps)
        threads.push_back(std::thread([&]() {
            masksigs();
            a.net->listen(a.addr);
            for (auto &b: apps)
                if (b.addr != a.addr)
                {
                    auto pid = use_tls ?
                        salticidae::PeerId(*b.net->get_cert()) :
                        salticidae::PeerId(b.addr);
                    a.net->add_peer(pid);
                    a.net->set_peer_addr(pid, b.addr);
                    a.net->conn_peer(pid);
                }
            a.ec.dispatch();}));
    
    EventContext ec;
    auto shutdown = [&](int) {
        for (auto &a: apps)
        {
            auto &tc = a.tcall;
            tc->async_call([ec=a.ec](ThreadCall::Handle &) {
                ec.stop();
            });
        }
        for (auto &t: threads) t.join();
        ec.stop();
    };
    salticidae::SigEvent ev_sigint(ec, shutdown);
    salticidae::SigEvent ev_sigterm(ec, shutdown);
    ev_sigint.add(SIGINT);
    ev_sigterm.add(SIGTERM);
    ec.dispatch();
    return 0;
}
