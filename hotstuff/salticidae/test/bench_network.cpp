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
#include <thread>
#include <signal.h>

/* disable SHA256 checksum */
#define SALTICIDAE_NOCHECKSUM

#include "salticidae/msg.h"
#include "salticidae/event.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"

using salticidae::NetAddr;
using salticidae::DataStream;
using salticidae::MsgNetwork;
using salticidae::htole;
using salticidae::letoh;
using salticidae::bytearray_t;
using salticidae::TimerEvent;
using salticidae::ThreadCall;
using std::placeholders::_1;
using std::placeholders::_2;
using opcode_t = uint8_t;

struct MsgBytes {
    static const opcode_t opcode = 0xa;
    DataStream serialized;
    bytearray_t bytes;
    MsgBytes(size_t size) {
        bytes.resize(size);
        serialized << htole((uint32_t)size) << bytes;
    }
    MsgBytes(DataStream &&s) {
        uint32_t len;
        s >> len;
        len = letoh(len);
        auto base = s.get_data_inplace(len);
        bytes = bytearray_t(base, base + len);
    }
};

const opcode_t MsgBytes::opcode;

using MsgNetworkByteOp = MsgNetwork<opcode_t>;

struct MyNet: public MsgNetworkByteOp {
    const std::string name;
    TimerEvent ev_period_stat;
    ThreadCall tcall;
    size_t nrecv;
    std::function<void(ThreadCall::Handle &)> trigger;

    MyNet(const salticidae::EventContext &ec,
            const std::string name,
            double stat_timeout = -1):
            MsgNetworkByteOp(ec, MsgNetworkByteOp::Config(
                ConnPool::Config()
                    .max_send_buff_size(65536)
                ).burst_size(1000)),
            name(name),
            ev_period_stat(ec, [this, stat_timeout](TimerEvent &) {
                SALTICIDAE_LOG_INFO("%.2f mps", nrecv / (double)stat_timeout);
                fflush(stderr);
                nrecv = 0;
                ev_period_stat.add(stat_timeout);
            }),
            tcall(ec),
            nrecv(0) {
        /* message handler could be a bound method */
        reg_handler(salticidae::generic_bind(&MyNet::on_receive_bytes, this, _1, _2));
        if (stat_timeout > 0)
            ev_period_stat.add(0);
        reg_conn_handler([this, ec](const ConnPool::conn_t &conn, bool connected) {
            if (connected)
            {
                if (conn->get_mode() == MyNet::Conn::ACTIVE)
                {
                    printf("[%s] connected, sending bytes.\n", this->name.c_str());
                    /* send the first message through this connection */
                    trigger = [this, conn](ThreadCall::Handle &) {
                        send_msg(MsgBytes(256), salticidae::static_pointer_cast<Conn>(conn));
                        if (!conn->is_terminated())
                            tcall.async_call(trigger);
                    };
                    tcall.async_call(trigger);
                }
                else
                    printf("[%s] passively connected, waiting for bytes.\n", this->name.c_str());
            }
            else
            {
                printf("[%s] disconnected, retrying.\n", this->name.c_str());
                /* try to reconnect to the same address */
                connect(conn->get_addr());
            }
            return true;
        });
    }

    void on_receive_bytes(MsgBytes &&msg, const conn_t &conn) { nrecv++; }
};

salticidae::EventContext ec;
NetAddr alice_addr("127.0.0.1:1234");
NetAddr bob_addr("127.0.0.1:1235");

void masksigs() {
	sigset_t mask;
	sigemptyset(&mask);
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

int main() {
    salticidae::BoxObj<MyNet> alice = new MyNet(ec, "Alice", 10);
    alice->start();
    alice->listen(alice_addr);
    salticidae::EventContext tec;
    MyNet bob(tec, "Bob");
    std::thread bob_thread([&]() {
        masksigs();
        bob.start();
        bob.connect(alice_addr);
        tec.dispatch();
    });
    auto shutdown = [&](int) {
        bob.tcall.async_call([&](salticidae::ThreadCall::Handle &) {
            tec.stop();
        });
        ec.stop();
        bob_thread.join();
    };
    salticidae::SigEvent ev_sigint(ec, shutdown);
    salticidae::SigEvent ev_sigterm(ec, shutdown);
    ev_sigint.add(SIGINT);
    ev_sigterm.add(SIGTERM);
    ec.dispatch();
    return 0;
}
