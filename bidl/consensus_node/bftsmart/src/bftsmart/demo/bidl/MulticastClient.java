package bftsmart.demo.bidl;

import java.io.IOException;
import java.net.*;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MulticastClient {
    private static Logger logger = LoggerFactory.getLogger(MulticastClient.class.getName());

    public static void main(String args[]) {
        try {
            NetworkInterface ni = NetworkInterface.getByName("enp5s0");
            // NetworkInterface ni = NetworkInterface.getByName("eth0");
            if (ni == null) {
                logger.error("Error getting the Network Interface");
                return;
            }
            Enumeration<InetAddress> addresses = ni.getInetAddresses();
            InetAddress localAddress = null;
            while (addresses.hasMoreElements()) {
                InetAddress address = addresses.nextElement();
                if (address instanceof Inet4Address) {
                    localAddress = address;
                }
            }
            InetSocketAddress inetAddr = new InetSocketAddress(localAddress, 0);
            logger.info("Preparing to using the interface: {}, IP: {}", ni.getName(), inetAddr);

            InetAddress to = InetAddress.getByName("230.0.0.0");
            int numOfThreads = 1;
            int perThreadTxs = 50000;
            int stopInterval = 10000;
            logger.info("Number of Threads {}, per-thread transactions {}", numOfThreads, perThreadTxs);
            ExecutorService fixedThreadPool = Executors.newFixedThreadPool(numOfThreads);
            for (int i = 0; i < numOfThreads; i++) {
                fixedThreadPool.execute(new SenderMulticast(inetAddr, to, 7777, perThreadTxs, stopInterval));
            }
            fixedThreadPool.shutdown();
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (SocketException e) {
            e.printStackTrace();
        }
    }
}

class SenderMulticast extends Thread {
    private Logger logger = LoggerFactory.getLogger(this.getName());
    private InetAddress to;
    private int port;
    private DatagramSocket socket;
    private int number;
    private int stopInterval;

    SenderMulticast(InetSocketAddress inetAddr, InetAddress to, int port, int number, int stopInterval)
            throws SocketException {
        this.to = to;
        this.port = port;
        this.socket = new DatagramSocket(inetAddr);
        this.stopInterval = stopInterval;
        this.socket.setBroadcast(true);
        this.number = number;
    }

    byte[] toByteArray(int value) {
        return new byte[] { (byte) (value >> 24), (byte) (value >> 16), (byte) (value >> 8), (byte) value };
    }

    public void run() {
        try {
            int stopCounter = 0;
            byte[] data = new byte[1024];
            for (int i = 0; i < this.number; i++, stopCounter++) {
                new Random().nextBytes(data);
                byte[] seqBytes = toByteArray(i);
                for (int j = 0; j < seqBytes.length; j++) {
                    data[j] = seqBytes[j];
                }
                DatagramPacket sendPacket = new DatagramPacket(data, data.length, this.to, this.port);
                this.socket.send(sendPacket);
                if (stopCounter == this.stopInterval) {
                    logger.info("bidl: sent {} transactions, stop for 5ms", this.stopInterval);
                    stopCounter = 0;
                    Thread.sleep(10);
                }
            }
            this.socket.close();
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
