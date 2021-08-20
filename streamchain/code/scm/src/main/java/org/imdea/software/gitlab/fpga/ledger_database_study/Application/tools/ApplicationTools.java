package org.imdea.software.gitlab.fpga.ledger_database_study.Application.tools;

import java.io.FileInputStream;
import java.time.LocalTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.function.DoubleFunction;
import java.util.function.Function;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.generator.TransactionFactory;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.TransactionOutput;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration.TransactionType;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Transactional;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.RandomGenerationUtils;

public class ApplicationTools {

	private static ApplicationTools __instance;
	
	RandomGenerationUtils randomNumberGenerator;
	TransactionFactory transactionFactory;
	Transactional transactionPlatformInstance;
	private static Map<Integer,List<Integer>> contract_seller_buyers;
	
	private ApplicationTools(Transactional transactionPlatformInstance) {
		
		if( Configuration.randomSeed == -1 ) {
			Configuration.randomSeed = System.nanoTime();
		}
		
		randomNumberGenerator = RandomGenerationUtils.getInstance(Configuration.randomSeed);
		transactionFactory = TransactionFactory.getInstance(randomNumberGenerator);
		this.transactionPlatformInstance = transactionPlatformInstance;
		if( contract_seller_buyers == null ) {
			contract_seller_buyers = generateSupplierBuyerLink();
		}
		
	}
	
	public static ApplicationTools getInstance(String configFilePath, 
			Transactional contractImpl) throws Exception {
		
		if( __instance == null ) {
			loadInitialConfiguration(configFilePath);
			__instance = new ApplicationTools(contractImpl);
		}
		
		return __instance;
	}
	
	public Map<TransactionType, List<List<Transaction>>> generateTransactions() 
			throws Exception{
		
		LocalTime generationStartTime = LocalTime.now();
		
		Map<TransactionType,List<List<Transaction>>> transactionMap = 
				new HashMap<TransactionType, List<List<Transaction>>>();
		
		List<List<Transaction>> benchmarkTransactionLists = new ArrayList<List<Transaction>>();
		
		if(Configuration.includeInsertionQueries ) {
			transactionMap.put(TransactionType.Load,getInsertionTransactions());
		}
		
		if(Configuration.includeBenchmarkQueries ) {
			
			benchmarkTransactionLists.add(getBenchmarkingTransactions());
			transactionMap.put(TransactionType.Benchmark, benchmarkTransactionLists);
		}
		
		LocalTime generationEndTime = LocalTime.now();
		
		ReportTools.writeMetaReport(Configuration.randomSeed, generationStartTime, generationEndTime, printConfiguration());
		
		return transactionMap;
	}
	
	public TransactionOutput clearPersistentState() throws Exception {
		return transactionFactory.clearStateTransaction(transactionPlatformInstance).call();
	}
	
	private static void loadInitialConfiguration(String configFilePath) throws Exception {
		FileInputStream propertyFile = new FileInputStream(configFilePath);
		Properties connectionProps = new Properties();
		connectionProps.load(propertyFile);
		
		Configuration.mysqlHostName = connectionProps.getProperty("hostname");
		Configuration.mysqlPort = connectionProps.getProperty("port");
		Configuration.mysqlUserName = connectionProps.getProperty("user");
		Configuration.mysqlPassword = connectionProps.getProperty("password");
		Configuration.reportBaseFolder = connectionProps.getProperty("reportbase");
		Configuration.maxVendors = Integer.parseInt(connectionProps.getProperty("maxvendors"));
		Configuration.maxProducts = Integer.parseInt(connectionProps.getProperty("maxproducts"));
		Configuration.maxBenchmarkTransactions =  
				Integer.parseInt(connectionProps.getProperty("maxbenchmarktransactions"));
		
		Configuration.minOrderQuantity =  
				Integer.parseInt(connectionProps.getProperty("minorderquantity"));
		
		Configuration.maxOrderQuantity =  
				Integer.parseInt(connectionProps.getProperty("maxorderquantity"));
		
		Configuration.maxInitialInventory =  
				Integer.parseInt(connectionProps.getProperty("maxinitialinventory"));
		
		Configuration.randomSeed = Long.parseLong(connectionProps.getProperty("randomseed"));
		Configuration.fixedRunSeconds = Long.parseLong(connectionProps.getProperty("executionbatchseconds"));
		
		Configuration.minBaseTransactions = Integer.parseInt(connectionProps.getProperty("minbasictransactions"));
		
		Configuration.minThreads = Integer.parseInt(connectionProps.getProperty("minthreads"));
		Configuration.maxThreads = Integer.parseInt(connectionProps.getProperty("maxthreads"));
		Configuration.sleepBetweenBatchSeconds = Integer.parseInt(connectionProps.getProperty("sleepAfterExecutionSec"));
		
		Configuration.clearPersistentState = Boolean.parseBoolean(connectionProps.getProperty("clearstate"));
		
		Configuration.includeInsertionQueries = 
				Boolean.parseBoolean(connectionProps.getProperty("includeinsertionqueries"));
		
		Configuration.includeBenchmarkQueries =  
				Boolean.parseBoolean(connectionProps.getProperty("includebenchmarkqueries"));
		
		Configuration.contractProbability =  
				Double.parseDouble(connectionProps.getProperty("contractprobability"));
		
		String benchmarkChoiceProbabilitiesStr = 
				connectionProps.getProperty("operationchoiceprobabilityarr");
		
		if( connectionProps.containsKey("fabricworkloadpath") ) {
			Configuration.fabricWorkloadDirectoryPath = 
					connectionProps.getProperty("fabricworkloadpath");
			
		}
		
		Configuration.benchmarkChoiceProbabilitiesArr = 
				Arrays.asList(benchmarkChoiceProbabilitiesStr.split(",")).stream().map(new Function<String, Double>() {

					@Override
					public Double apply(String t) {
						return Double.parseDouble(t);
					}
				}).toArray(Double[]::new);
		
	}
	
	public String printConfiguration() {
		
		StringBuilder outputStr = new StringBuilder();
		
		outputStr.append("hostname : " + Configuration.mysqlHostName+"\n");
		outputStr.append("port : " + Configuration.mysqlPort+"\n");
		outputStr.append("User : " + Configuration.mysqlUserName+"\n");
		outputStr.append("Password : " + Configuration.mysqlPassword+"\n");
		outputStr.append("Report base : " + Configuration.reportBaseFolder+"\n");
		outputStr.append("Vendors: "+Configuration.maxVendors+"\n");
		outputStr.append("Products : "+ Configuration.maxProducts+"\n");
		outputStr.append("Max Transactions : "+Configuration.maxBenchmarkTransactions+"\n");
		
		outputStr.append("Min order quantity : "+ Configuration.minOrderQuantity+"\n");
		
		outputStr.append("Max order quantity : "+ Configuration.maxOrderQuantity+"\n");
		
		outputStr.append("Random seed : " + Configuration.randomSeed +"\n");
		outputStr.append("Max Threads : " + Configuration.maxThreads +"\n");
		outputStr.append("Include insertion queries ? "+ Configuration.includeInsertionQueries +"\n");
		
		outputStr.append("Include benchmark queries ? "+ Configuration.includeBenchmarkQueries +"\n");
		
		outputStr.append("Contract probability : "+ Configuration.contractProbability +"\n");
		outputStr.append("BenchmarkChoice proabilities : ");
		for( double val : Configuration.benchmarkChoiceProbabilitiesArr) {
			outputStr.append(val+" ");
		}
		
		return outputStr.toString();
	}
		
	public List<List<Transaction>> getInsertionTransactions() throws Exception{

		List<List<Transaction>> transactionList = new ArrayList<List<Transaction>>();
		
		transactionList.add(transactionFactory.createVendorTransactions(
				transactionPlatformInstance, Configuration.maxVendors));
		
		transactionList.add(transactionFactory.createProductTransactions(
				transactionPlatformInstance, Configuration.maxProducts));
		
		transactionList.add(transactionFactory.updateVendorInventory(
				transactionPlatformInstance, Configuration.maxVendors, Configuration.maxProducts, 
				Configuration.maxInitialInventory));
		
		transactionList.add(transactionFactory.generateContractTransactions(
				transactionPlatformInstance, Configuration.maxOrderQuantity, contract_seller_buyers, 
				Configuration.maxProducts));
		
		List<Transaction> baseOrderTransactions = new ArrayList<Transaction>();
		
		for( int i = 0; i < Configuration.minBaseTransactions; i++ ) {
			baseOrderTransactions.add(transactionFactory.generateOrderTransaction(
					transactionPlatformInstance, contract_seller_buyers, Configuration.maxVendors, 
					Configuration.maxProducts, Configuration.minOrderQuantity, Configuration.maxOrderQuantity));
			
		}
		
		transactionList.add(baseOrderTransactions);
		return transactionList;
	}
	
	public List<Transaction> getBenchmarkingTransactions() throws Exception {
		
		List<Transaction> transactionList = new ArrayList<Transaction>();
		
		for( int i = 0; i < Configuration.maxBenchmarkTransactions; i++ ) {
			
			int operation = generateRandomAnalyticalOperation();
			
			switch(operation) {
				case 1 : transactionList.add(
							transactionFactory.generateOrderTransaction(
								transactionPlatformInstance,contract_seller_buyers, 
								Configuration.maxVendors, Configuration.maxProducts, 
								Configuration.minOrderQuantity,Configuration.maxOrderQuantity));
				
						  break;
						  
				case 2 :  transactionList.add(
							transactionFactory.generateCalcInvDaysOfSupplyTransaction(
									transactionPlatformInstance, Configuration.maxVendors, 
									Configuration.maxProducts));
				
						  break;
						  
				case 3 :  transactionList.add(
							transactionFactory.generateCalcBullwhipCoeffTransaction(
									transactionPlatformInstance));
				
						  break;
						  
				default:  continue;
			}
		}
		
		return transactionList;
	}
	
	private Map<Integer,List<Integer>> generateSupplierBuyerLink(){
		
		Map<Integer,List<Integer>> supplier_buyerMap = 
				new HashMap<Integer, List<Integer>>();
		
		for( int vendor = 1; vendor <= Configuration.maxVendors; vendor++ ) {
			supplier_buyerMap.put(vendor, 
					getBuyersForSupplier(vendor));
			
		}
		
		return supplier_buyerMap;
	}
	
	private List<Integer> getBuyersForSupplier(
			int supplier){
		
		List<Integer> links = new ArrayList<Integer>();
		
		int start = (supplier + 1) % Configuration.maxVendors;
		
		if( start == 0 ) {
			start = Configuration.maxVendors;
		}
		
		for( int i = start; i != supplier; i++ ) {
			
			double classifier = randomNumberGenerator.getRandomDouble();
			
			if( Double.compare(classifier, 
					Configuration.contractProbability) <= 0 ) {
				
				links.add(i);
			}
			
			if( i == Configuration.maxVendors ) {
				i = 0;
			}
		}
		
		return links;
	}
	
	private int generateRandomAnalyticalOperation() {
		
		DoubleFunction<Integer> probClassifier = (double classifier) -> {
			
			double sum = Configuration.benchmarkChoiceProbabilitiesArr[0];
			
			int i = 1;
			
			for( ; i < Configuration.benchmarkChoiceProbabilitiesArr.length; i++ ) {
				if( Double.compare(classifier, sum ) <= 0 ) {
					break;
				}
				
				sum += Configuration.benchmarkChoiceProbabilitiesArr[i];
			}
			
			return i;
		};
		
		double choice = randomNumberGenerator.getRandomDouble();
		
		return probClassifier.apply(choice);
	}
}