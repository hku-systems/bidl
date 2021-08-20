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

#include "salticidae/msg.h"
#include "salticidae/network.h"

using salticidae::uint256_t;
using salticidae::DataStream;
using salticidae::get_hash;
using salticidae::get_hex;
using salticidae::htole;
using salticidae::letoh;

using opcode_t = uint8_t;

struct MsgTest {
    static const opcode_t opcode = 0x0;
    DataStream serialized;
    MsgTest(int cnt) {
        serialized << htole((uint32_t)cnt);
        for (int i = 0; i < cnt; i++)
        {
            uint256_t hash = get_hash(i);
            printf("adding hash %s\n", get_hex(hash).c_str());
            serialized << hash;
        }
    }

    MsgTest(DataStream &&s) {
        uint32_t cnt;
        s >> cnt;
        cnt = letoh(cnt);
        printf("got %d hashes\n", cnt);
        for (int i = 0; i < cnt; i++)
        {
            uint256_t hash;
            s >> hash;
            printf("got hash %s\n", get_hex(hash).c_str());
        }
    }
};

const opcode_t MsgTest::opcode;

int main() {
    salticidae::MsgBase<opcode_t> msg(MsgTest(10), 0x0);
    printf("%s\n", std::string(msg).c_str());
    MsgTest parse(msg.get_payload());
    return 0;
}
