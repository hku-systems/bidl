package bftsmart.demo.bidl;

import bftsmart.tom.ServiceProxy;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Enumeration;

import io.netty.buffer.ByteBuf;
import io.netty.channel.socket.DatagramPacket;
import io.netty.channel.socket.InternetProtocolFamily;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioDatagramChannel;

public class BidlFrontendSequencer {
    /*
     * args[0]: group address, args[1]: port, args[2]: proxy?
     */
    public static void main(String args[]) {
        if (args.length != 3) {
            System.err.println("args[0]: group address, args[1]: port, args[2]: proxy?");
        }
        int batchSize = 500;
        InetSocketAddress groupAddress = new InetSocketAddress(args[0], Integer.parseInt(args[1]));
        boolean isLeader = Boolean.parseBoolean(args[2]);
        Bootstrap bootstrap = new Bootstrap();
        NioEventLoopGroup acceptGroup = new NioEventLoopGroup(1);
        try {
            NetworkInterface ni = NetworkInterface.getByName("enp5s0"); // used en0 for
            Enumeration<InetAddress> addresses = ni.getInetAddresses();
            InetAddress localAddress = null;
            while (addresses.hasMoreElements()) {
                InetAddress address = addresses.nextElement();
                if (address instanceof Inet4Address) {
                    localAddress = address;
                }
            }
            System.out.println("Local address is " + localAddress);

            bootstrap = new Bootstrap().group(acceptGroup).channelFactory(new ChannelFactory<NioDatagramChannel>() {
                @Override
                public NioDatagramChannel newChannel() {
                    return new NioDatagramChannel(InternetProtocolFamily.IPv4);
                }
            }).localAddress(localAddress, groupAddress.getPort()).option(ChannelOption.IP_MULTICAST_IF, ni)
                    .option(ChannelOption.SO_RCVBUF, 1024 * 1024 * 100)
                    .option(ChannelOption.RCVBUF_ALLOCATOR, new FixedRecvByteBufAllocator(1024 * 1024))
                    .option(ChannelOption.SO_REUSEADDR, true).handler(new ChannelInitializer<NioDatagramChannel>() {
                        @Override
                        public void initChannel(NioDatagramChannel ch) throws Exception {
                            ChannelPipeline pipeline = ch.pipeline();
                            if (isLeader) {
                                pipeline.addLast(new SequencerDecodeHandler(batchSize));
                            } else {
                                pipeline.addLast(new SimpleInboundHandler(batchSize));
                            }
                        }
                    });
            System.out.println("Frontend listening on UDP port " + groupAddress.getPort());
            System.out.println("Joining multicast group " + groupAddress.getAddress());
            ChannelFuture channelFuture = bootstrap.bind(new InetSocketAddress(groupAddress.getPort()));
            channelFuture.awaitUninterruptibly();
            NioDatagramChannel ch = (NioDatagramChannel) channelFuture.channel();
            ch.joinGroup(groupAddress, ni).sync();
            ch.closeFuture().await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (SocketException e) {
            e.printStackTrace();
        } finally {
            acceptGroup.shutdownGracefully();
        }
    }

    static private class SimpleInboundHandler extends ChannelInboundHandlerAdapter {
        private int totalNum = 0;
        private int batchSize;

        private SimpleInboundHandler(int batchSize) {
            this.batchSize = batchSize;
            System.out.println("SimpleInboundHandler initialized.");
        }

        public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
            // Get input stream
            if (totalNum % this.batchSize == 0) {
                System.out.println("Number of messages received: " + totalNum);
            }
            totalNum++;
            io.netty.channel.socket.DatagramPacket packet = (DatagramPacket) msg;
            ByteBuf bytebuf = packet.content();
            bytebuf.release();
        }
    }

    static private class SequencerDecodeHandler extends ChannelInboundHandlerAdapter {
        private int batchSize;
        private int totalNum;
        private MessageDigest digest;
        private byte[] block;
        private ServiceProxy proxy;
        private byte[] signature = new byte[0];

        SequencerDecodeHandler(int batchSize) {
            System.out.println("SequencerDecodeHandler initialized.");
            try {
                this.batchSize = batchSize;
                this.totalNum = 0;
                this.block = new byte[0];
                this.digest = MessageDigest.getInstance("SHA-256");
                this.proxy = new ServiceProxy(1001);
            } catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
            // Get input stream
            io.netty.channel.socket.DatagramPacket packet = (DatagramPacket) msg;
            ByteBuf bytebuf = packet.content();
            final int rcvPktLength = bytebuf.readableBytes();

            byte[] rcvPktBuf = new byte[rcvPktLength];
            bytebuf.readBytes(rcvPktBuf);

            // get sha256 of the transaction bytes
            byte[] hash = this.digest.digest(rcvPktBuf);

            this.totalNum++;
            // System.out.println("New transaction received " + this.totalNum);

            // add the hash to the block
            int aLen = block.length;
            int bLen = hash.length;
            ByteBuffer blockBuf = ByteBuffer.allocate(aLen + bLen);
            blockBuf.put(block);
            blockBuf.put(hash);
            this.block = blockBuf.array();

            if (this.totalNum % this.batchSize == 0) {
                ByteBuffer buffer = ByteBuffer.allocate(block.length + signature.length + (Integer.BYTES * 2));
                buffer.putInt(block.length);
                buffer.put(block);
                buffer.putInt(signature.length);
                buffer.put(signature);
                this.block = buffer.array();

                byte[] reply = proxy.invokeOrdered(this.block);
                if (reply != null) {
                    System.out.println("One block committed " + this.totalNum);
                }
                this.block = new byte[0];
            }
            bytebuf.release();
        }
    }
}