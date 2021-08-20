package fabric;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.PrivateKey;
import java.security.Security;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

import org.hyperledger.fabric.sdk.BlockEvent.TransactionEvent;
import org.hyperledger.fabric.sdk.ChaincodeID;
import org.hyperledger.fabric.sdk.ChaincodeResponse;
import org.hyperledger.fabric.sdk.Channel;
import org.hyperledger.fabric.sdk.Enrollment;
import org.hyperledger.fabric.sdk.HFClient;
import org.hyperledger.fabric.sdk.NetworkConfig;
import org.hyperledger.fabric.sdk.Peer;
import org.hyperledger.fabric.sdk.ProposalResponse;
import org.hyperledger.fabric.sdk.QueryByChaincodeRequest;
import org.hyperledger.fabric.sdk.TransactionProposalRequest;
import org.hyperledger.fabric.sdk.exception.InvalidArgumentException;
import org.hyperledger.fabric.sdk.exception.NetworkConfigurationException;
import org.hyperledger.fabric.sdk.exception.ProposalException;
import org.hyperledger.fabric.sdk.identity.X509Enrollment;
import org.hyperledger.fabric.sdk.security.CryptoSuite;
import org.apache.htrace.shaded.fasterxml.jackson.databind.ObjectMapper;
import org.bouncycastle.asn1.pkcs.PrivateKeyInfo;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.openssl.PEMParser;
import org.bouncycastle.openssl.jcajce.JcaPEMKeyConverter;

import com.yahoo.ycsb.ByteIterator;
import com.yahoo.ycsb.DB;
import com.yahoo.ycsb.DBException;
import com.yahoo.ycsb.Status;
import com.yahoo.ycsb.StringByteIterator;
	
/**
 * Version: 3.0
 */
    
public class FabricDB extends DB {
	
	private NetworkConfig config;
	
	private final String org = "org1";
	private final String orgMSP = "Org1MSP";
	private final String channelName = "mychannel";
	private String chaincodeName;
	
	private PrintWriter endorseLog;
	private PrintWriter orderLog;
	private PrintWriter validateLog;
	private boolean endorseLogging;
	private boolean orderLogging;
	private boolean validateLogging;
	
	private Channel channel;
	private HFClient client;

	//To wait for a transaction or not, it is read form the properties file as a "wait"
	public boolean waitForTransaction;
	private String eventName = "Operation done";

	private static final AtomicInteger endorserIndex = new AtomicInteger(1);
	private List<Peer> endorsers;
	
	private String userFolder = "crypto-config/peerOrganizations/org1.example.com/users/User1@org1.example.com";
	
	public FabricDB() throws InvalidArgumentException, NetworkConfigurationException, IOException {
		config = NetworkConfig.fromYamlFile(new File("netconfig.yaml"));
	}
		
	private PrintWriter prepareFile(String name) throws IOException {
		
		File file = new File(name);
		
		if (!file.exists()) {
			file.createNewFile();
		}
		
		Writer writer = new BufferedWriter(new FileWriter(file, true));
		
		return new PrintWriter(writer);
	}
	
	private void log(PrintWriter p, long startTime) {
		p.printf("%s,%f\n", System.currentTimeMillis(), (float)(System.nanoTime()-startTime)/1000/1000);
	}
	
	@Override
	public void init() throws DBException {		
		Properties p = getProperties();
		
		waitForTransaction =  Boolean.valueOf((String) p.getProperty("wait", "false"));
		endorseLogging = Boolean.valueOf((String) p.getProperty("endorseLogging", "false"));
		orderLogging = Boolean.valueOf((String) p.getProperty("orderLogging", "false"));
		validateLogging = Boolean.valueOf((String) p.getProperty("validateLogging", "false"));
		chaincodeName = p.getProperty("chaincode", "YCSB");
		
		try {
			endorseLog = endorseLogging ? prepareFile("endorse.log") : null;
			orderLog = orderLogging ? prepareFile("order.log") : null;
			validateLog = validateLogging ? prepareFile("validate.log") : null;
		} catch (IOException e) {
			e.printStackTrace();
		}
		
				
		try {
			AppUser c = enrollClient();
			client = getClient(c);
			channel = initializeChannel(client);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	private String readCer()  throws IOException {
	  byte[] encoded = Files.readAllBytes(Paths.get(userFolder + "/msp/signcerts/User1@org1.example.com-cert.pem"));
	  return new String(encoded, StandardCharsets.UTF_8);
	}
	
	private PrivateKey loadKey () throws Exception {
		String keyPath = userFolder+"/msp/keystore/";
		File folder = new File(keyPath);
		for (File fileEntry : Objects.requireNonNull(folder.listFiles())) {
			 keyPath = fileEntry.getPath();
		}
		BufferedReader br = new BufferedReader(new FileReader(keyPath));
		Security.addProvider(new BouncyCastleProvider());
		PEMParser pp = new PEMParser(br);
		PrivateKeyInfo info =  (PrivateKeyInfo) pp.readObject();
		PrivateKey kp = new JcaPEMKeyConverter().getPrivateKey(info);
		pp.close();
		return kp;
		
	}
	
	private AppUser enrollClient() throws Exception {
	
    	PrivateKey key = loadKey();
		String cer = readCer();
		Enrollment userEnrollment = new X509Enrollment(key,cer);
		return new AppUser("User1", org, orgMSP, userEnrollment);

	}
		
	private HFClient getClient(AppUser appUser) throws Exception  {
		CryptoSuite cryptoSuite = CryptoSuite.Factory.getCryptoSuite();
		HFClient client = HFClient.createNewInstance();
		client.setCryptoSuite(cryptoSuite);
		client.setUserContext(appUser);
		return client;
	}
	
	private Channel initializeChannel(HFClient client)  throws Exception {
		Channel channel = client.loadChannelFromConfig(channelName, config);
		channel.initialize();
		endorsers = new ArrayList<>(channel.getPeers());
		return channel;
	}
	
	@Override
	public Status read(String table, String key, Set<String> fields, Map<String, ByteIterator> result) {
		QueryByChaincodeRequest qpr = client.newQueryProposalRequest();
		ArrayList<String> args = new ArrayList<>();
		ChaincodeID chainCCId = ChaincodeID.newBuilder().setName(chaincodeName).build();
		qpr.setChaincodeID(chainCCId);
		qpr.setFcn("read");
		args.add(key);
		if(fields != null) {
			args.addAll(fields);
		}else {
			args.add("all");
		}
		qpr.setArgs(args);
				
		try {
			Collection<ProposalResponse> res;
			
			if (endorseLogging) {
				long startTime = System.nanoTime();
				res = channel.queryByChaincode(qpr);
				log(endorseLog, startTime);
			} else {
				res = channel.queryByChaincode(qpr);
			}			
			
			for (ProposalResponse pres : res) {
				if(pres.getStatus() == ChaincodeResponse.Status.FAILURE) {
					return Status.ERROR;
				}
			    byte[] response = pres.getChaincodeActionResponsePayload();		
			    			    
				try {
					ObjectMapper mapper = new ObjectMapper();
					StringMap map = new StringMap();
					map = mapper.readValue(response, StringMap.class); 
					StringByteIterator.putAllAsByteIterators(result, map);
				} catch (Exception e) {
					System.out.println("Error in read "+e);
					return Status.ERROR;
				}
			}
			return Status.OK;
		} catch (InvalidArgumentException | ProposalException e) {
			System.out.println("Error in read "+e);
			return Status.ERROR;
		}
	}

	@Override
	public Status scan(String table, String startkey, int recordcount, Set<String> fields,
			Vector<HashMap<String, ByteIterator>> result) {
		ArrayList<String> args = new ArrayList<>();
		QueryByChaincodeRequest qpr = client.newQueryProposalRequest();
		ChaincodeID chainCCId = ChaincodeID.newBuilder().setName(chaincodeName).build();
		qpr.setChaincodeID(chainCCId);
		qpr.setFcn("scan");
		args.add(startkey);
		args.add(String.valueOf(recordcount));
		if(fields != null) {
			args.addAll(fields);
		}else {
			args.add("all");
		}
		qpr.setArgs(args);
		try {
			Collection<ProposalResponse> res;
			
			if (endorseLogging) {
				long startTime = System.nanoTime();
				res = channel.queryByChaincode(qpr);
				log(endorseLog, startTime);
			} else {
				res = channel.queryByChaincode(qpr);
			}
			
			for (ProposalResponse pres : res) {
			    byte[] response = pres.getChaincodeActionResponsePayload();
			    try {

					ObjectMapper mapper = new ObjectMapper();
					List<StringMap> list = new ArrayList<>();
					list = mapper.readValue(response, list.getClass()); 
					for (Map<String,String> m : list) {
						result.addElement((HashMap<String, ByteIterator>) StringByteIterator.getByteIteratorMap(m));
					}
				} catch (Exception e) {
					System.out.println("Error in scan "+e);
					return Status.ERROR;
				}
			    
			}
			return Status.OK;
		} catch (InvalidArgumentException | ProposalException e) {
			System.out.println(e);
			return Status.ERROR;
		}
	}

	@Override
	public Status update(String table, String key, Map<String, ByteIterator> values) {
		TransactionProposalRequest qpr = client.newTransactionProposalRequest();
		ChaincodeID chainCCId = ChaincodeID.newBuilder().setName(chaincodeName).build();
		ArrayList<String> args = new ArrayList<>();
		qpr.setChaincodeID(chainCCId);
		qpr.setFcn("update");
		args.add(key);
		for (Map.Entry<String, ByteIterator> entry : values.entrySet())
		{
			args.add(entry.getKey()+":"+entry.getValue().toString());
		}
		qpr.setArgs(args);
		try {
			Collection<ProposalResponse> responses;
			
			if (endorseLogging) {
				long startTime = System.nanoTime();
				responses = channel.sendTransactionProposal(qpr);
				log(endorseLog, startTime);
			} else {
				responses = channel.sendTransactionProposal(qpr);
			}
						
			List<ProposalResponse> invalid = responses.stream().filter(ChaincodeResponse::isInvalid).collect(Collectors.toList());
			if (!invalid.isEmpty()) {
				return Status.ERROR;
			}
			
			if (orderLogging) {
				long startTime = System.nanoTime();
				channel.sendTransaction(responses);
				log(orderLog, startTime);
			} else {
				channel.sendTransaction(responses);
			}
			
			if(this.waitForTransaction) {
				
				Vector<ChaincodeEventCapture> chaincodeEvents = new Vector<>();
				String chaincodeEventListenerHandle = SdkIntegration.setChaincodeEventListener(channel, eventName, chaincodeEvents);
				
				if (validateLogging) {
					long startTime = System.nanoTime();
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle );
					log(validateLog, startTime);
				}
				else {
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle );
				}
			}
			return Status.OK;
		} catch (InvalidArgumentException | ProposalException e) {
			System.out.println("Error in update "+e);
			return Status.ERROR;
		}
	}

	@Override
	public Status insert(String table, String key, Map<String, ByteIterator> values) {
		TransactionProposalRequest qpr = client.newTransactionProposalRequest();
		ChaincodeID chainCCId = ChaincodeID.newBuilder().setName(chaincodeName).build();
		ArrayList<String> args = new ArrayList<>();
		qpr.setChaincodeID(chainCCId);
		qpr.setFcn("insert");
		args.add(key);
		for (Map.Entry<String, ByteIterator> entry : values.entrySet())
		{
			args.add(entry.getKey()+":"+entry.getValue().toString());				
		}
		qpr.setArgs(args);
		try {
			Collection<ProposalResponse> responses;
			
			if (endorseLogging) {
				long startTime = System.nanoTime();
				int idx = endorserIndex.incrementAndGet() % endorsers.size();
				responses = channel.sendTransactionProposal(qpr, (Collection<Peer>) Arrays.asList(endorsers.get(idx)));
				//responses = channel.sendTransactionProposal(qpr, (Collection<Peer>) Arrays.asList(endorsers.get(idx % 2)));
				//responses = channel.sendTransactionProposal(qpr);
				log(endorseLog, startTime);
			} else {
				responses = channel.sendTransactionProposal(qpr);
			}
			
			List<ProposalResponse> invalid = responses.stream().filter(ChaincodeResponse::isInvalid).collect(Collectors.toList());
			if (!invalid.isEmpty()) {
				return Status.ERROR;
			}
			
			/*
			if(waitForTransaction) {
				
				if(validateLogging) {
					long startTime = System.nanoTime();
					future = future.thenApply(te -> {
						log(validateLog, startTime);
						return null;
					});
				}
				
				future.get(10, TimeUnit.SECONDS);
			}
			*/
			
			if(waitForTransaction) {
				if(validateLogging) {
					long startTime = System.nanoTime();
					channel.sendTransaction(responses).thenApply(te -> {
						log(validateLog, startTime);
						return null;
					}).get(10, TimeUnit.SECONDS);
				}
				else {
														
					channel.sendTransaction(responses).get();
				}
			} else {
				channel.sendTransaction(responses);
			}
			
			/*
			if(this.waitForTransaction) {
							
				Vector<ChaincodeEventCapture> chaincodeEvents = new Vector<>();
				String chaincodeEventListenerHandle = SdkIntegration.setChaincodeEventListener(channel, eventName, chaincodeEvents);
				
				if (validateLogging) {
					long startTime = System.nanoTime();
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle);
					validateLog.println((float)(System.nanoTime()-startTime)/1000/1000);
				}
				else {
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle);
				}
			}*/
						
			return Status.OK;
		} catch (Exception e) {
			System.out.println("Error in insert "+e);
			return Status.ERROR;
		}
	}

	@Override
	public Status delete(String table, String key) {
		TransactionProposalRequest qpr = client.newTransactionProposalRequest();
		ChaincodeID chainCCId = ChaincodeID.newBuilder().setName(chaincodeName).build();
		qpr.setChaincodeID(chainCCId);
		qpr.setFcn("delete");
		qpr.setArgs(key);
		try {
			Collection<ProposalResponse> responses;
			if (endorseLogging) {
				long startTime = System.nanoTime();
				responses = channel.sendTransactionProposal(qpr);
				log(endorseLog, startTime);
			} else {
				responses = channel.sendTransactionProposal(qpr);
			}
			List<ProposalResponse> invalid = responses.stream().filter(ChaincodeResponse::isInvalid).collect(Collectors.toList());
			if (!invalid.isEmpty()) {
				return Status.ERROR;
			}
			
			if (orderLogging) {
				long startTime = System.nanoTime();
				channel.sendTransaction(responses);
				log(orderLog, startTime);
			} else {
				channel.sendTransaction(responses);
			}
			
			if(this.waitForTransaction) {
				
				Vector<ChaincodeEventCapture> chaincodeEvents = new Vector<>();
				String chaincodeEventListenerHandle = SdkIntegration.setChaincodeEventListener(channel, eventName, chaincodeEvents);
				
				if (validateLogging) {
					long startTime = System.nanoTime();
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle );
					log(validateLog, startTime);
				}
				else {
					SdkIntegration.waitForChaincodeEvent(10, channel, chaincodeEvents,chaincodeEventListenerHandle );
				}
			}
			
			return Status.OK;
		} catch (InvalidArgumentException | ProposalException e) {
			System.out.println("Error in delete "+e);
			return Status.ERROR;
		}
	}
	
	@Override
	public void cleanup() throws DBException {
		
		if (endorseLogging) {
			endorseLog.flush();
			endorseLog.close();
		}
		
		if (orderLogging) {
			orderLog.flush();
			orderLog.close();
		}

		if (validateLogging) {
			validateLog.flush();
			validateLog.close();
		}
	}
}
