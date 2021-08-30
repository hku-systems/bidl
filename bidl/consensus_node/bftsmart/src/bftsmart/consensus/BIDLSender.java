package bftsmart.consensus;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.*;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Enumeration;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import bftsmart.tom.BidlFrontend;

public class BIDLSender {
    private static Logger logger = LoggerFactory.getLogger(BIDLSender.class.getName());
    private InetAddress group;
    private int port;
    // private DatagramSocket socket;
    private MulticastSocket socket;

    public BIDLSender() {
        try {
            NetworkInterface ni = NetworkInterface.getByName("enp5s0");
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
            logger.info("Preparing to use the interface: {}, IP: {}", ni.getName(), inetAddr);
            this.group = InetAddress.getByName("230.0.0.0");
			this.port = 7777;
			// this.socket = new DatagramSocket(inetAddr);
			this.socket = new MulticastSocket(this.port);
            this.socket.setNetworkInterface(ni);;
            this.socket.setLoopbackMode(false);
            logger.info("loopback disabled? {}", this.socket.getLoopbackMode());
            this.socket.joinGroup(this.group);
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (SocketException e) {
            e.printStackTrace();
        } catch (IOException e){
            e.printStackTrace();
        }
    }
	public void send(byte[] data) {
        try {
			DatagramPacket sendPacket = new DatagramPacket(data, data.length, this.group, this.port);
            logger.debug("bidl: broadcasting message, length: {}, content: {}", data.length, Arrays.copyOf(data, 20));
			this.socket.send(sendPacket);
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
		}
    }
}