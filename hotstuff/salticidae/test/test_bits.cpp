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

#include <vector>
#include "salticidae/stream.h"

using salticidae::DataStream;
using salticidae::get_hash;
using salticidae::get_hex;
using salticidae::Bits;

void print(const Bits &a, int n) {
    for (int i = 0; i < n; i++)
        printf("%d", a.get(i));
    puts("");
}

int main() {
    int n = 80;
    Bits a(n);
    for (int i = 0; i < n; i++)
    {
        a.set(i);
        print(a, n);
    }
    for (int i = n - 1; i >= 0; i--)
    {
        a.unset(i);
        print(a, n);
    }
    std::vector<int> idxs;
    for (int i = 0; i < 1000; i++)
    {
        idxs.push_back(rand() % n);
        a.flip(idxs.back());
        print(a, n);
    }
    for (auto idx: idxs)
    {
        a.flip(idx);
        print(a, n);
    }
    for (int i = 0; i < n; i++)
        if (i & 1) a.set(i);
    print(a, n);
    for (int i = 0; i < n; i++)
        if (i & 1) a.unset(i);
        else a.set(i);
    print(a, n);
    printf("%s size: %d\n", get_hex(a).c_str(), a.size());
    DataStream s;
    s << a;
    Bits b;
    s >> b;
    Bits c(b);
    Bits d(std::move(c));
    printf("%s\n", get_hex(b).c_str());
    print(b, b.size());
    Bits e(4);
    e.set(0);
    e.set(1);
    print(e, e.size());
    return 0;
}
