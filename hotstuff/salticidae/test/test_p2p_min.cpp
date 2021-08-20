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
#include <memory>
#include "salticidae/event.h"
#include "salticidae/network.h"

using Net = salticidae::PeerNetwork<uint8_t>;

int main() {
    std::vector<std::pair<salticidae::NetAddr, std::unique_ptr<Net>>> nodes;
    Net::Config config;
    salticidae::EventContext ec;
    config.ping_period(2);
    nodes.resize(4);
    for (size_t i = 0; i < nodes.size(); i++)
    {
        salticidae::NetAddr addr("127.0.0.1:" + std::to_string(10000 + i));
        auto &net = (nodes[i] = std::make_pair(addr, std::make_unique<Net>(ec, config))).second;
        net->start();
        net->listen(addr);
    }
    for (size_t i = 0; i < nodes.size(); i++)
        for (size_t j = 0; j < nodes.size(); j++)
            if (i != j)
            {
                auto &node = nodes[i].second;
                auto &peer_addr = nodes[j].first;
                salticidae::PeerId pid{peer_addr};
                node->add_peer(pid);
                node->set_peer_addr(pid, peer_addr);
                node->conn_peer(pid);
            }
    ec.dispatch();
    return 0;
}
