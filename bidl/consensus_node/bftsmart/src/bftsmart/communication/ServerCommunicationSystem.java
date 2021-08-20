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
package bftsmart.communication;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.crypto.SecretKey;

import bftsmart.communication.client.CommunicationSystemServerSide;
import bftsmart.communication.client.CommunicationSystemServerSideFactory;
import bftsmart.communication.client.RequestReceiver;
import bftsmart.communication.server.ServersCommunicationLayer;
import bftsmart.consensus.roles.Acceptor;
import bftsmart.reconfiguration.ServerViewController;
import bftsmart.tom.ServiceReplica;
import bftsmart.tom.core.TOMLayer;
import bftsmart.tom.core.messages.TOMMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *
 * @author alysson
 */
public class ServerCommunicationSystem extends Thread {

    private Logger logger = LoggerFactory.getLogger(this.getClass());

    private boolean doWork = true;
    public final long MESSAGE_WAIT_TIME = 100;
    private LinkedBlockingQueue<SystemMessage> inQueue = null;// new LinkedBlockingQueue<SystemMessage>(IN_QUEUE_SIZE);
    protected MessageHandler messageHandler;

    private ServersCommunicationLayer serversConn;
    private CommunicationSystemServerSide clientsConn;
    private ServerViewController controller;

    /**
     * Creates a new instance of ServerCommunicationSystem
     */
    public ServerCommunicationSystem(ServerViewController controller, ServiceReplica replica) throws Exception {
        super("Server Comm. System");

        this.controller = controller;

        messageHandler = new MessageHandler();

        inQueue = new LinkedBlockingQueue<SystemMessage>(controller.getStaticConf().getInQueueSize());

        serversConn = new ServersCommunicationLayer(controller, inQueue, replica);

        // ******* EDUARDO BEGIN **************//
        clientsConn = CommunicationSystemServerSideFactory.getCommunicationSystemServerSide(controller);
        // ******* EDUARDO END **************//
    }

    // ******* EDUARDO BEGIN **************//
    public void joinViewReceived() {
        serversConn.joinViewReceived();
    }

    public void updateServersConnections() {
        this.serversConn.updateConnections();
        if (clientsConn == null) {
            clientsConn = CommunicationSystemServerSideFactory.getCommunicationSystemServerSide(controller);
        }

    }

    // ******* EDUARDO END **************//
    public void setAcceptor(Acceptor acceptor) {
        messageHandler.setAcceptor(acceptor);
    }

    public void setTOMLayer(TOMLayer tomLayer) {
        messageHandler.setTOMLayer(tomLayer);
    }

    public void setRequestReceiver(RequestReceiver requestReceiver) {
        if (clientsConn == null) {
            clientsConn = CommunicationSystemServerSideFactory.getCommunicationSystemServerSide(controller);
        }
        clientsConn.setRequestReceiver(requestReceiver);
    }

    /**
     * Thread method responsible for receiving messages sent by other servers.
     */
    @Override
    public void run() {

        long count = 0;
        while (doWork) {
            try {
                if (count % 1000 == 0 && count > 0) {
                    logger.debug("After " + count + " messages, inQueue size=" + inQueue.size());
                }


                SystemMessage sm = inQueue.poll(MESSAGE_WAIT_TIME, TimeUnit.MILLISECONDS);

                if (sm != null) {
                    logger.debug("<-- receiving, msg:" + sm);
                    messageHandler.processData(sm);
                    count++;
                } else {
                    messageHandler.verifyPending();
                }
            } catch (InterruptedException e) {

                logger.error("Error processing message", e);
            }
        }
        logger.info("ServerCommunicationSystem stopped.");

    }

    /**
     * Send a message to target processes. If the message is an instance of
     * TOMMessage, it is sent to the clients, otherwise it is set to the servers.
     *
     * @param targets the target receivers of the message
     * @param sm      the message to be sent
     */
    public void send(int[] targets, SystemMessage sm) {
        if (sm instanceof TOMMessage) {
            clientsConn.send(targets, (TOMMessage) sm, false);
        } else {
            logger.debug("--> sending message from: {} -> {}" + sm.getSender(), targets);
            serversConn.send(targets, sm, true);
        }
    }

    public ServersCommunicationLayer getServersConn() {
        return serversConn;
    }

    public CommunicationSystemServerSide getClientsConn() {
        return clientsConn;
    }

    @Override
    public String toString() {
        return serversConn.toString();
    }

    public void shutdown() {

        logger.info("Shutting down communication layer");

        this.doWork = false;
        clientsConn.shutdown();
        serversConn.shutdown();
    }

    public SecretKey getSecretKey(int id) {
        return serversConn.getSecretKey(id);
    }
}
