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

import java.util.List;

/**
 * This class work as a factory of messages used in the paxos protocol.
 */
public class MessageFactory {

    // constants for messages types
    public static final int PROPOSE = 44781;
    public static final int WRITE = 44782;
    public static final int ACCEPT = 44783;

    private int from; // Replica ID of the process which sent this message

    /**
     * Creates a message factory
     * 
     * @param from Replica ID of the process which sent this message
     */
    public MessageFactory(int from) {

        this.from = from;

    }

    /**
     * Creates a PROPOSE message to be sent by this process
     * 
     * @param id    Consensus's execution ID
     * @param epoch Epoch number
     * @param value Proposed value
     * @param proof Proofs from other replicas
     * @return A paxos message of the PROPOSE type, with the specified id, epoch,
     *         value, and proof
     */
    public ConsensusMessage createPropose(int id, int epoch, byte[] value) {

        return new ConsensusMessage(PROPOSE, id, epoch, from, value);

    }

    /**
     * Creates a WRITE message to be sent by this process
     * 
     * @param id    Consensus's execution ID
     * @param epoch Epoch number
     * @param value Write value
     * @return A consensus message of the WRITE type, with the specified id, epoch,
     *         and value
     */
    public ConsensusMessage createWrite(int id, int epoch, byte[] value) {

        return new ConsensusMessage(WRITE, id, epoch, from, value);

    }

    /**
     * Creates a WRITE message to be sent by this process
     * 
     * @param id    Consensus's execution ID
     * @param epoch Epoch number
     * @param value Accepted value
     * @return A consensus message of the ACCEPT type, with the specified id, epoch,
     *         and value
     */
    public ConsensusMessage createAccept(int id, int epoch, byte[] value) {

        return new ConsensusMessage(ACCEPT, id, epoch, from, value);

    }

    // create gap-request
    public BidlMessage createGapReqMsg(int number) {
        return new BidlMessage(number, from, 1, new byte[32 * number], null);
    }

    public BidlMessage createGapReqMsg(int number, byte[] hashes) {
        return new BidlMessage(number, from, 1, hashes, null);
    }

    public BidlMessage createGapReqMsg(int number, List<byte[]> gapHashes) {
        byte[] hashes = new byte[32 * gapHashes.size()];
        for (int i = 0; i < number; i++) {
            for (int j = 0; j < 32; j++) {
                hashes[32 * i + j] = gapHashes.get(i)[j];
            }
        }
        return new BidlMessage(number, from, 1, hashes, null);
    }

    // leader node creates gap-reply message
    public BidlMessage createGapReplyMsg(int number) {
        return new BidlMessage(number, from, 2, null, new byte[1024 * number]);
    }

    public BidlMessage createGapReplyMsg(int number, byte[] transactions) {
        return new BidlMessage(number, from, 2, null, transactions);
    }

    // public BidlMessage createGapReplyMsg(List<byte[]> replyTxs, int byteLen) {
    // int number = replyTxs.size();
    // byte[] txBytes = new byte[byteLen];
    // int index = 0;
    // for (int i = 0; i < number; i++) {
    // byte[] tx = replyTxs.get(i);
    // for (int j = 0; j < tx.length; j++) {
    // txBytes[index++] = tx[j];
    // }
    // txBytes[index++] = ',';
    // }
    // return new BidlMessage(number, from, 2, null, txBytes);
    // }
}
