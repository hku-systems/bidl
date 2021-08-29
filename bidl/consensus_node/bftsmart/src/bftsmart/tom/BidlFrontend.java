package bftsmart.tom;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.concurrent.*;

import io.netty.buffer.ByteBuf;
import io.netty.channel.socket.DatagramPacket;
import io.netty.channel.socket.InternetProtocolFamily;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioDatagramChannel;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import bftsmart.consensus.BIDLSender;
import bftsmart.reconfiguration.ServerViewController;
import bftsmart.tom.core.ExecutionManager;

public class BidlFrontend extends Thread {
    public static int maxSeqNum = 0;

    // public static final ConcurrentHashMap<String, HashSet<Integer>> conflictList = new ConcurrentHashMap<>();
    public static final ConcurrentHashMap<String, Integer> conflictList = new ConcurrentHashMap<>();
    public static final ConcurrentHashMap<String, Integer> denyList = new ConcurrentHashMap<>();
    public static final ConcurrentHashMap<Integer, String> seqMap = new ConcurrentHashMap<>();
    public static final ConcurrentMap<String, byte[]> txMap = new ConcurrentHashMap<>();
    public static final BlockingQueue<byte[]> txBlockingQueue = new LinkedBlockingQueue<>(100000);
    public static final byte[] MagicNumTxnMalicious = {8, 8, 8, 8}; 
    public static final byte[] MagicNumTxn = {7, 7, 7, 7};
    public static final byte[] MagicNumBlock = {6, 6, 6, 6};
    public static final byte[] MagicNumExecResult = {5, 5, 5, 5};
    public static final byte[] MagicNumPersist = {4, 4, 4, 4};

    public BIDLSender bidlSender; 

    private Logger logger = LoggerFactory.getLogger(this.getClass());
    private ServerViewController controller;
    private ExecutionManager execManager;
    private Assembler assember;
    public volatile int totalNum = 0;
    private volatile boolean maliciousFlag = false;

    BidlFrontend(ServerViewController controller, ExecutionManager executionManager) {
        super("BIDL frontend");
        this.controller = controller;
        this.execManager = executionManager;
        this.bidlSender = new BIDLSender();
        this.assember = new Assembler();
        this.assember.start();
    }

    @Override
    public void run() {
        InetSocketAddress groupAddress = new InetSocketAddress("230.0.0.0", 7777);
        logger.info("bidl: new BIDL frontend started, listening group address: {}", groupAddress);
        Bootstrap bootstrap = new Bootstrap();
        EventLoopGroup acceptGroup = new NioEventLoopGroup(4);
        try {
            NetworkInterface ni = NetworkInterface.getByName("enp5s0"); 
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
                    .option(ChannelOption.SO_RCVBUF, 1024 * 1024 * 1000)
                    .option(ChannelOption.RCVBUF_ALLOCATOR, new FixedRecvByteBufAllocator(1024 * 1024 * 1000))
					.option(ChannelOption.SO_REUSEADDR, true).handler(new ChannelInitializer<NioDatagramChannel>() {
                        @Override
                        public void initChannel(NioDatagramChannel ch) throws Exception {
                            ChannelPipeline pipeline = ch.pipeline();
                            pipeline.addLast(new SequencerDecodeHandler());
                        }
                    });
            ChannelFuture channelFuture = bootstrap.bind(new InetSocketAddress(groupAddress.getPort()));
            // channelFuture.awaitUninterruptibly();
            NioDatagramChannel ch = (NioDatagramChannel) channelFuture.channel();
            ch.joinGroup(groupAddress, ni).sync();
            ch.closeFuture().await();
        } catch (InterruptedException | SocketException e) {
            e.printStackTrace();
        } finally {
            acceptGroup.shutdownGracefully();
        }
    }

    private class SequencerDecodeHandler extends ChannelInboundHandlerAdapter {
        @Override
        public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
            // Get input stream
            io.netty.channel.socket.DatagramPacket packet = (DatagramPacket) msg;
            ByteBuf bytebuf = packet.content();
            final int rcvPktLength = bytebuf.readableBytes();

            byte[] rcvPktBuf = new byte[rcvPktLength];
            bytebuf.readBytes(rcvPktBuf);
            
            // check magic number
            byte[] magicNum = Arrays.copyOfRange(rcvPktBuf, 0, 4);
            if (denyList.containsKey(new String(magicNum))) {
                bytebuf.release();
                return;
            }
            if (Arrays.equals(magicNum, MagicNumTxn)) {
                totalNum++;
                logger.debug("bidl: transaction received");
            } else if (Arrays.equals(magicNum, MagicNumTxnMalicious)) {
                if (execManager.getCurrentLeader() == controller.getStaticConf().getProcessId()) {
                    bytebuf.release();
                    return;
                }
                maliciousFlag = true;
                logger.debug("bidl: transaction from the malicious client received");
            } else if (Arrays.equals(magicNum, MagicNumBlock)) {
                logger.debug("bidl: block received, just ignore.");
                bytebuf.release();
                return;
            } else if (Arrays.equals(magicNum, MagicNumExecResult)) {
                logger.debug("bidl: execution result received, send persist.");
                for (int i=0; i<4; i++) {
                    rcvPktBuf[i] = BidlFrontend.MagicNumPersist[i];
                }
                bidlSender.send(rcvPktBuf);
                bytebuf.release();
                return;
            } else if (Arrays.equals(magicNum, MagicNumPersist)) {
                logger.debug("bidl: persist message received");
                bytebuf.release();
                return;
            } else {
                logger.debug("bidl: invalid message received, just ignore");
                bytebuf.release();
                return;
            } 
            
            txBlockingQueue.add(rcvPktBuf);
            bytebuf.release();
            if (totalNum % 500 == 0){
                logger.info("bidl: received enough transactions. Num:{}", totalNum);
            }
        }
    }

    private class Assembler extends Thread {
        private ByteBuffer payloadBuffer = null;
        private ByteBuffer blockBuffer = null;
        private int num = 0;
        private MessageDigest digest = null;
        private ServiceProxy proxy = null;
        private byte[] signature = null;
        private int blockSize = 500;
        private Logger logger = LoggerFactory.getLogger(this.getClass());

        Assembler() {
            try {
                this.num = 0;
                this.digest = MessageDigest.getInstance("SHA-256");
                this.proxy = new ServiceProxy(1001, controller.getStaticConf().getProcessId());
                this.signature = new byte[0];
                this.blockBuffer = ByteBuffer.allocateDirect(this.blockSize * 36 + Integer.BYTES * 2 + Integer.BYTES);
                this.payloadBuffer = ByteBuffer.allocateDirect(this.blockSize * 36 + Integer.BYTES);
                logger.info("The default batch size is {}", this.blockSize);
            } catch (NoSuchAlgorithmException e) {
                e.printStackTrace();
            }
        }

        // retrieve int from the byte array
        int fromByteArrayLittleEndian(byte[] bytes, int start) {
            return (bytes[start+3] & 0xFF) << 24 | (bytes[start+2] & 0xFF) << 16 | (bytes[start+1] & 0xFF) << 8 | (bytes[start] & 0xFF);
        }
    
        @Override
        public void run() {
            while (true) {
                byte[] rcvPktBuf = null;
                try {
                    if (execManager.getCurrentLeader() == controller.getStaticConf().getProcessId() && maliciousFlag == false) {
                        if (totalNum > 50000) {
                            rcvPktBuf = BidlFrontend.txBlockingQueue.take();
                        } else {
                            continue;
                        }
                    } else {
                        rcvPktBuf = BidlFrontend.txBlockingQueue.take();
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                // retrieve the sequence number of this transaction
                int seqNum = fromByteArrayLittleEndian(rcvPktBuf, 4);
                if (seqMap.containsKey(seqNum)) {
                    logger.debug("bidl: I already hold a transactions with the same sequence number {}, discard.", seqNum, maxSeqNum);
                    continue; 
                }
                maxSeqNum = seqNum > maxSeqNum ? seqNum : maxSeqNum;
                logger.debug("bidl: sequence Number of current transaction is {}, maximum:{}", seqNum, maxSeqNum);
    
    
                // get sha256 of the transaction bytes
                byte[] hash = this.digest.digest(rcvPktBuf);
    
                this.num++;
    
                // index transactions according to the sequence number and transaction hash
                String hashStr = new String(hash);
                seqMap.put(seqNum, hashStr);
                txMap.put(hashStr, rcvPktBuf);
                // logger.debug("bidl: new transaction received, size: {}, totalNumber: {}, num: {}, txMap size: {} ",
                //         rcvPktLength, this.totalNum, num, txMap.size());


                // put the transaction's sequence number and hashes to the block buffer
                this.payloadBuffer.put(Arrays.copyOfRange(rcvPktBuf, 4, 8));
                this.payloadBuffer.put(hash);
    
                // if I have collected enough transactions and I am the leader, submit txs to the co-located node
                
                if (this.num == this.blockSize) {
                    logger.info("bidl: collected enough transactions. Num:{}, TotalNum:{}", this.num, totalNum);
                    payloadBuffer.putInt(maxSeqNum);
                    if (execManager.getCurrentLeader() == controller.getStaticConf().getProcessId()) {
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
    
                        // logger.info("bidl: BidlFrontend, block " + " length " + block.length + " content "
                        //         + Arrays.toString(block));
    
                        proxy.invokeOrdered(block);
                        logger.info("bidl: I am the leader, new block submitted, totalNumber: " + totalNum
                                + " block length: " + block.length + " signature.length " + this.signature.length
                                + "maximum sequence number" + maxSeqNum);
                    }
                    this.num = 0;
                    this.payloadBuffer.clear();
                    this.blockBuffer.clear();
                }
            }
        }
    }
}

