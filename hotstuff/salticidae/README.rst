Salticidae: minimal C++ asynchronous network library.
=====================================================

.. image:: https://img.shields.io/badge/License-MIT-yellow.svg
   :target: https://opensource.org/licenses/MIT


Features
--------

- Simplicity. The library is self-contained, small in code base, and only
  relies on libuv and libssl/libcrypto (OpenSSL, for SHA256 purpose).
  Despite the multi-threaded nature of the library, a user only needs to
  understand the callbacks are invoked in a sequential order driven by a single
  user-initiated event loop.

- Clarity. With moderate use of C++ template and new features, the vast
  majority of the code is self-documenting.

- Layered design. You can use network abstraction from the lowest socket
  connection level to the highest P2P network level.

- Performance. Based on a hybrid solution that combines both thread-based and
  event-driven concurrency paradigms, it gets the best of both worlds.
  The implementation strives to incur very little overhead in processing
  network I/O, and avoid unnecessary memory copies thanks to the move semantics.

- Utilities. The library also provides with some useful gadgets, such as
  command-line parser, libuv abstraction, etc.

- Security. It supports SSL/TLS, with customized certificate verification (as
  part of the connection callback).

- Bindings for other languages. The library itself supports C APIs and the
  other project ``salticidae-go`` supports invoking the library in Go through
  cgo.

Functionalities
---------------

- ``ConnPool``: byte level connection pool implementation, ``ConnPool::Conn`` (or
  ``ConnPool::conn_t``) objects represent connections to which one can
  send/receive a stream of binary data asynchronously.

- ``MsgNetwork<OpcodeType>``: message level network pool implementation,
  ``MsgNetwork::Conn`` (or ``MsgNetwork::cont_t``) objects represent channels to
  which one can send/receive predefined messages asynchronously. Message
  handler functions are registered by ``reg_handler()`` and invoked upon
  receiving a new message.  ``OpcodeType`` is the type used for identifying
  message types. A valid message class must have:

  - a static member ``opcode`` typed ``OpcodeType`` as the message type identifier
  - a member ``serialized`` typed ``DataStream`` which contains the serialized data
    of the message.

  - a constructor ``MsgType(DataStream &&)`` that parse the message from stream.

Based on ``MsgNetwork``, salticidae provides the following higher level abstractions:

- ``PeerNetwork<OpcodeType>``: simple P2P network pool implementation. It will
  ensure exactly one underlying bi-directional connection is established per
  added peer, and retry the connection when it is broken. Ping-pong messages
  are utilized to test the connectivity periodically.

- ``ClientNetwork<OpcodeType>``: simple client-server network pool
  implementation. A server who initially calls ``listen()`` will accept the
  incoming client messages, while a client simply calls ``connect()`` to connect
  to a known server.

Dependencies
------------

- CMake >= 3.9
- C++14
- libuv >= 1.10.0
- openssl >= 1.1.0

Minimal working P2P network
---------------------------
.. code-block:: cpp

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

Using MsgNetwork class
----------------------
.. code-block:: cpp

   #include <cstdio>
   #include <string>
   #include <functional>
   
   #include "salticidae/msg.h"
   #include "salticidae/event.h"
   #include "salticidae/network.h"
   #include "salticidae/stream.h"
   
   using salticidae::NetAddr;
   using salticidae::DataStream;
   using salticidae::MsgNetwork;
   using salticidae::htole;
   using salticidae::letoh;
   using std::placeholders::_1;
   using std::placeholders::_2;
   
   /** Hello Message. */
   struct MsgHello {
       static const uint8_t opcode = 0x0;
       DataStream serialized;
       std::string name;
       std::string text;
       /** Defines how to serialize the msg. */
       MsgHello(const std::string &name,
               const std::string &text) {
           serialized << htole((uint32_t)name.length());
           serialized << name << text;
       }
       /** Defines how to parse the msg. */
       MsgHello(DataStream &&s) {
           uint32_t len;
           s >> len;
           len = letoh(len);
           name = std::string((const char *)s.get_data_inplace(len), len);
           len = s.size();
           text = std::string((const char *)s.get_data_inplace(len), len);
       }
   };
   
   /** Acknowledgement Message. */
   struct MsgAck {
       static const uint8_t opcode = 0x1;
       DataStream serialized;
       MsgAck() {}
       MsgAck(DataStream &&s) {}
   };
   
   const uint8_t MsgHello::opcode;
   const uint8_t MsgAck::opcode;
   
   using MsgNetworkByteOp = MsgNetwork<uint8_t>;
   
   struct MyNet: public MsgNetworkByteOp {
       const std::string name;
       const NetAddr peer;
   
       MyNet(const salticidae::EventContext &ec,
               const std::string name,
               const NetAddr &peer):
               MsgNetwork<uint8_t>(ec, MsgNetwork::Config()),
               name(name),
               peer(peer) {
           /* message handler could be a bound method */
           reg_handler(
               salticidae::generic_bind(&MyNet::on_receive_hello, this, _1, _2));
   
           reg_conn_handler([this](const ConnPool::conn_t &conn, bool connected) {
               if (connected)
               {
                   if (conn->get_mode() == ConnPool::Conn::ACTIVE)
                   {
                       printf("[%s] connected, sending hello.\n",
                               this->name.c_str());
                       /* send the first message through this connection */
                       send_msg(MsgHello(this->name, "Hello there!"),
                               salticidae::static_pointer_cast<Conn>(conn));
                   }
                   else
                       printf("[%s] accepted, waiting for greetings.\n",
                               this->name.c_str());
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
   
       void on_receive_hello(MsgHello &&msg, const MyNet::conn_t &conn) {
           printf("[%s] %s says %s\n", name.c_str(), msg.name.c_str(), msg.text.c_str());
           /* send acknowledgement */
           send_msg(MsgAck(), conn);
       }
   };
   
   void on_receive_ack(MsgAck &&msg, const MyNet::conn_t &conn) {
       auto net = static_cast<MyNet *>(conn->get_net());
       printf("[%s] the peer knows\n", net->name.c_str());
   }
   
   int main() {
       salticidae::EventContext ec;
       NetAddr alice_addr("127.0.0.1:12345");
       NetAddr bob_addr("127.0.0.1:12346");
   
       /* test two nodes in the same main loop */
       MyNet alice(ec, "alice", bob_addr);
       MyNet bob(ec, "bob", alice_addr);
   
       /* message handler could be a normal function */
       alice.reg_handler(on_receive_ack);
       bob.reg_handler(on_receive_ack);
   
       /* start all threads */
       alice.start();
       bob.start();
   
       /* accept incoming connections */
       alice.listen(alice_addr);
       bob.listen(bob_addr);
   
       /* try to connect once */
       alice.connect(bob_addr);
       bob.connect(alice_addr);
   
       /* the main loop can be shutdown by ctrl-c or kill */
       auto shutdown = [&](int) {ec.stop();};
       salticidae::SigEvent ev_sigint(ec, shutdown);
       salticidae::SigEvent ev_sigterm(ec, shutdown);
       ev_sigint.add(SIGINT);
       ev_sigterm.add(SIGTERM);
   
       /* enter the main loop */
       ec.dispatch();
       return 0;
   }
