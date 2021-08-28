/*
* High performance netty server receiving transactions from the sequencer.
* */
package bftsmart.demo.bidl;

import io.netty.buffer.ByteBuf;
import io.netty.channel.socket.DatagramPacket;
import io.netty.channel.socket.InternetProtocolFamily;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioDatagramChannel;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Enumeration;

public class MulticastServer {
    private static Logger logger = LoggerFactory.getLogger(MulticastServer.class.getName());

    public static void main(String args[]) {
        InetSocketAddress groupAddress = new InetSocketAddress("230.0.0.0", 7777);
        Bootstrap bootstrap = new Bootstrap();
        NioEventLoopGroup acceptGroup = new NioEventLoopGroup(1);
        try {
            // NetworkInterface ni = NetworkInterface.getByName("enp5s0"); // used en0 for
            // OSX, wlan0 for windows
            NetworkInterface ni = NetworkInterface.getByName("eth0"); // used en0 for
            // OSX,
            // wlan0 for windows

            Enumeration<InetAddress> addresses = ni.getInetAddresses();
            InetAddress localAddress = null;
            while (addresses.hasMoreElements()) {
                InetAddress address = addresses.nextElement();
                if (address instanceof Inet4Address) {
                    localAddress = address;
                }
            }
            logger.info("Local address is {}", localAddress);

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
                            pipeline.addLast(new SequencerDecodeHandler());
                        }
                    });
            logger.info("Server listening on UDP port {}", groupAddress.getPort());
            logger.info("Joining multicast group {}", groupAddress.getAddress());
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

    private static class SequencerDecodeHandler extends ChannelInboundHandlerAdapter {
        private int num = 0;
        private int totalNum = 0;
        private MessageDigest digest = null;
        private byte[] signature = null;
        private ByteBuffer blockBuffer = null;
        private ByteBuffer payloadBuffer = null;
        private int blockSize = 500;
        private long startTime;

        SequencerDecodeHandler() {
            try {
                this.startTime = 0;
                this.num = 0;
                this.totalNum = 0;
                this.digest = MessageDigest.getInstance("SHA-256");
                this.signature = new byte[0];
                this.blockBuffer = ByteBuffer.allocate(this.blockSize * 32 + Integer.BYTES * 2);
                this.payloadBuffer = ByteBuffer.allocate(this.blockSize * 32);
            } catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
            if (this.num == 0) {
                this.startTime = System.currentTimeMillis();
            }
            // Get input stream
            io.netty.channel.socket.DatagramPacket packet = (DatagramPacket) msg;
            ByteBuf bytebuf = packet.content();
            final int rcvPktLength = bytebuf.readableBytes();

            byte[] rcvPktBuf = new byte[rcvPktLength];
            bytebuf.readBytes(rcvPktBuf);

            // get sha256 of the transaction bytes
            byte[] hash = this.digest.digest(rcvPktBuf);

            this.num++;
            this.totalNum++;

            // add hashes to the next block
            this.payloadBuffer.put(hash);

            // int aLen = block.length;
            // int bLen = hash.length;
            // ByteBuffer blockBuf = ByteBuffer.allocate(aLen + bLen);
            // blockBuf.put(block);
            // blockBuf.put(hash);
            // this.block = blockBuf.array();
            // logger.trace("bidl: BidlFrontend, tx hash, length:" + bLen + " content: " +
            // Arrays.toString(hash)
            // + " Block after adding a hash " + " length: " + this.block.length + "
            // content: "
            // + Arrays.toString(this.block));

            // if I have collected enough transactions and I am the leader, submit txs
            if (this.num == this.blockSize) {
                logger.debug("bidl: collected enough transactions. Num:{}, TotalNum:{}", this.num, this.totalNum);

                logger.debug("bidl: I am the leader, enter!!");
                payloadBuffer.flip();
                byte[] payload = new byte[payloadBuffer.remaining()];
                payloadBuffer.get(payload);

                blockBuffer.putInt(payload.length);
                blockBuffer.put(payload);
                blockBuffer.putInt(signature.length);
                blockBuffer.put(signature);
                blockBuffer.flip();
                byte[] block = new byte[blockBuffer.remaining()];
                blockBuffer.get(block);

                logger.debug("bidl: I am the leader, new block submitted, totalNumber: " + this.totalNum
                        + " block length: " + block.length + " signature.length " + this.signature.length);
                this.num = 0;
                this.payloadBuffer.clear();
                this.blockBuffer.clear();

                long cur = System.currentTimeMillis();
                logger.info("Throughput: {} TPS, total number: {}",
                        this.blockSize / (double) (cur - this.startTime) * 1000.0, this.totalNum);
            }
            bytebuf.release();
        }
    }
}
