package bftsmart.demo.bidl;

import java.io.IOException;
import java.net.*;
import java.util.Enumeration;
import java.util.concurrent.LinkedBlockingQueue;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.netty.bootstrap.Bootstrap;
import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.FixedRecvByteBufAllocator;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioDatagramChannel;
import io.netty.util.ReferenceCountUtil;

public class Sequencer {
    private static Logger logger = LoggerFactory.getLogger(Sequencer.class.getName());
    private static LinkedBlockingQueue<Transaction> inQueue = new LinkedBlockingQueue<Transaction>();

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

            new SequencerMulticast(inetAddr, to, 7777, inQueue).start();
            new ClientUDPServer(7776, inQueue).start();

        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (SocketException e) {
            e.printStackTrace();
        }
    }
}

class ClientUDPServer extends Thread {
    private Logger logger = LoggerFactory.getLogger(this.getName());
    private int port;
    private LinkedBlockingQueue<Transaction> inQueue;

    ClientUDPServer(int port, LinkedBlockingQueue<Transaction> inQueue) {
        this.port = port;
        this.inQueue = inQueue;
    }

    public void run() {
        EventLoopGroup group = new NioEventLoopGroup();

        try {
            // 启动NIO服务的引导程序类
            Bootstrap b = new Bootstrap();

            b.group(group) // 设置EventLoopGroup
                    .channel(NioDatagramChannel.class) // 指明新的Channel的类型
                    .option(ChannelOption.SO_BROADCAST, true) // 设置的Channel的一些选项
                    .option(ChannelOption.SO_RCVBUF, 1024 * 1024 * 100)
                    .option(ChannelOption.RCVBUF_ALLOCATOR, new FixedRecvByteBufAllocator(1024 * 1024))
                    .option(ChannelOption.SO_REUSEADDR, true).handler(new ClientServerHandler()); // 指定ChannelHandler

            // 绑定端口
            ChannelFuture f = b.bind(port).sync();

            logger.info("DatagramChannelEchoServer started, port{}", port);

            // 等待服务器 socket 关闭 。
            // 在这个例子中，这不会发生，但你可以优雅地关闭你的服务器。
            f.channel().closeFuture().sync();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {

            // 优雅的关闭
            group.shutdownGracefully();
        }

    }

    class ClientServerHandler extends ChannelInboundHandlerAdapter {

        @Override
        public void channelRead(ChannelHandlerContext ctx, Object msg) {
            io.netty.channel.socket.DatagramPacket packet = (io.netty.channel.socket.DatagramPacket) msg;
            ByteBuf in = packet.content();
            final int rcvPktLength = in.readableBytes();
            byte[] rcvPktBuf = new byte[rcvPktLength];
            in.readBytes(rcvPktBuf);
            Transaction txn = new Transaction(rcvPktBuf);
            inQueue.add(txn);
            ReferenceCountUtil.release(msg);
        }

        @Override
        public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
            // 当出现异常就关闭连接
            cause.printStackTrace();
            ctx.close();
        }
    }
}

class SequencerMulticast extends Thread {
    private Logger logger = LoggerFactory.getLogger(this.getName());
    private InetAddress to;
    private int port;
    private DatagramSocket socket;
    private LinkedBlockingQueue<Transaction> inQueue;

    SequencerMulticast(InetSocketAddress inetAddr, InetAddress to, int port, LinkedBlockingQueue<Transaction> inQueue)
            throws SocketException {
        this.to = to;
        this.port = port;
        this.socket = new DatagramSocket(inetAddr);
        this.socket.setBroadcast(true);
        this.inQueue = inQueue;
    }

    public void run() {
        logger.info("SequencerMulticaster started, send to group address, IP: {}, Port: {}", this.to, this.port);
        try {
            while (true) {
                Transaction txn = this.inQueue.take();
                byte[] data = txn.getPayload();
                DatagramPacket sendPacket = new DatagramPacket(data, data.length, this.to, this.port);
                this.socket.send(sendPacket);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            this.socket.close();
        }
    }
}

class Transaction {
    byte[] payload;

    public Transaction(byte[] payload) {
        this.payload = payload;
    }

    public byte[] getPayload() {
        return this.payload;
    }
}