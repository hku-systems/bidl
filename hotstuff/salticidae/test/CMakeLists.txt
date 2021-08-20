# Copyright (c) 2018 Cornell University.
#
# Author: Ted Yin <tederminant@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

add_executable(test_msg test_msg.cpp)
target_link_libraries(test_msg salticidae_static)

add_executable(test_bits test_bits.cpp)
target_link_libraries(test_bits salticidae_static)

add_executable(test_msgnet test_msgnet.cpp)
target_link_libraries(test_msgnet salticidae_static)

add_executable(test_msgnet_tls test_msgnet_tls.cpp)
target_link_libraries(test_msgnet_tls salticidae_static)

add_executable(test_p2p test_p2p.cpp)
target_link_libraries(test_p2p salticidae_static)

add_executable(test_p2p_tls test_p2p_tls.cpp)
target_link_libraries(test_p2p_tls salticidae_static)

add_executable(test_p2p_stress test_p2p_stress.cpp)
target_link_libraries(test_p2p_stress salticidae_static)

add_executable(test_p2p_min test_p2p_min.cpp)
target_link_libraries(test_p2p_min salticidae_static)

add_executable(test_queue test_queue.cpp)
target_link_libraries(test_queue salticidae_static pthread)

add_executable(bench_network bench_network.cpp)
target_link_libraries(bench_network salticidae_static pthread)

add_executable(bench_network_tls bench_network_tls.cpp)
target_link_libraries(bench_network_tls salticidae_static pthread)

add_executable(test_msgnet_c test_msgnet_c.c)
target_link_libraries(test_msgnet_c salticidae_static pthread)

add_executable(test_bounded_recv_buffer test_bounded_recv_buffer.cpp)
target_link_libraries(test_bounded_recv_buffer salticidae_static pthread)
