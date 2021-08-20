/**
Copyright (c) 2007-2013 Alysson Bessani, Eduardo Alchieri, Paulo Sousa, and the authors indicated in the @author tags

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package bftsmart.consensus.messages;

import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectOutput;

import bftsmart.communication.SystemMessage;

/**
 * This class represents a message used in a epoch of a consensus instance.
 */
public class BidlMessage extends SystemMessage {

    // BIDL message type: gapReq: 1, gapReply: 2
    public static final int GAPREQ = 1;
    public static final int GAPREPLY = 2;

    private int number; // number of requested transactions
    private int from; // from
    private int type;
    private byte[] hashes = null; // hashes of request transactions
    private byte[] transactions = null; // requested transactions

    public BidlMessage() {
    }

    public BidlMessage(int number, int from, int type) {
        this(number, from, type, null, null);
    }

    public BidlMessage(int number, int from, int type, byte[] hashes, byte[] transactions) {
        super(from);

        this.number = number;
        this.from = from;
        this.type = type;
        this.hashes = hashes;
        this.transactions = transactions;
    }

    // Implemented method of the Externalizable interface
    @Override
    public void writeExternal(ObjectOutput out) throws IOException {
        super.writeExternal(out);
        out.writeInt(number);
        out.writeInt(from);
        out.writeInt(type);

        if (hashes == null) {
            out.writeInt(-1);
        } else {
            out.writeInt(hashes.length);
            out.write(hashes);
        }

        if (transactions == null) {
            out.writeInt(-1);
        } else {
            out.writeInt(transactions.length);
            out.write(transactions);
        }
    }

    // Implemented method of the Externalizable interface
    @Override
    public void readExternal(ObjectInput in) throws IOException, ClassNotFoundException {

        super.readExternal(in);

        number = in.readInt();
        from = in.readInt();
        type = in.readInt();

        int hashesLen = in.readInt();
        if (hashesLen != -1) {
            hashes = new byte[hashesLen];
            do {
                hashesLen -= in.read(hashes, hashes.length - hashesLen, hashesLen);

            } while (hashesLen > 0);
        }

        int txnLen = in.readInt();
        if (txnLen != -1) {
            transactions = new byte[txnLen];
            do {
                txnLen -= in.read(transactions, transactions.length - txnLen, txnLen);

            } while (txnLen > 0);
        }

    }

    public int getNumber() {
        return this.number;
    }

    public int getFrom() {
        return this.from;
    }

    public int getHashLength() {
        return this.hashes.length;
    }

    public int getTxsLength() {
        return this.transactions.length;
    }

    public int getType() {
        return this.type;
    }

    public byte[] getHashes() {
        return this.hashes;
    }

    public byte[] getTransactions() {
        return this.transactions;
    }

    @Override
    public String toString() {
        return "BIDL gapReq/gapReply message, from=" + getFrom() + ", number=" + getNumber() + ", type=" + getType();
    }
}
