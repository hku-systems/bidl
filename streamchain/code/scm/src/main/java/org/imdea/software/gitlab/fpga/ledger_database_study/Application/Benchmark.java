//package org.imdea.software.gitlab.fpga.ledger_database_study.Application;
//
//import java.io.FileInputStream;
//import java.time.LocalTime;
//import java.util.ArrayList;
//import java.util.Arrays;
//import java.util.HashMap;
//import java.util.List;
//import java.util.Map;
//import java.util.Properties;
//import java.util.function.DoubleFunction;
//import java.util.function.Function;
//
//import org.imdea.software.gitlab.fpga.ledger_database_study.Application.generator.TransactionFactory;
//import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.CSVReport;
//import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.TXTReport;
//import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.Executor;
//import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
//import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.sql.TransactionalSQLImpl;
//import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
//import org.imdea.software.gitlab.fpga.ledger_database_study.common.Transactional;
//import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.RandomGenerationUtils;
//
//public class Benchmark {
//	
//	static int noOfVendors = 4;
//	static int noOfProducts = 2;
//	
//	static int initialVendorQuantity[] = new int[noOfVendors];
//	
//	static int maxInitialInventory = 1000000;
//	
//	static int maxVendors;
//	static int maxProducts;
//	static int maxBenchmarkTransactions;
//	
//	static int minOrderQuantity;
//	static int maxOrderQuantity;
//	
//	static long randomSeed;
//	static int maxThreads;
//	
//	static RandomGenerationUtils randomNumberGenerator;
//	static TransactionFactory transactionFactory;
//	static Transactional transactionPlatformInstance;
//	
//	static boolean includeInsertionQueries;
//	static boolean includeBenchmarkQueries;
//	
//	//Probability of link between a supplier and buyer
//	static double contractProbability;
//	
//	static boolean clearSQL;
//	
//	//Probability of each benchmark operation to be chosen
//	static Double benchmarkChoiceProbabilitiesArr[];
//	
//	static Map<Integer,List<Integer>> contract_seller_buyers = null;
//	
//	static String configPath = "/Users/srivatsan.lakshmi/Documents/config.properties";
//	
//	public static void main(String[] args) throws Exception {
//		
//		if( args.length == 0 ) {
//			System.err.print("Missing argument - path to configuration file");
//			return;
//		}
//		
//		configPath = args[0];
//		
//		buildConfiguration();
//
//		if( randomSeed == -1 ) {
//			randomSeed = System.nanoTime();
//		}
//		
//		randomNumberGenerator = RandomGenerationUtils.getInstance(randomSeed);
//		transactionFactory = TransactionFactory.getInstance(
//				randomNumberGenerator);
//		
//		transactionPlatformInstance = TransactionalSQLImpl.getInstance();
//		
//		LocalTime generationStartTime = LocalTime.now();
//		
//		List<List<Transaction>> insertionTransactionList = new ArrayList<List<Transaction>>();
//		List<Transaction> analyticalTransactionList = new ArrayList<Transaction>();
//		
//		contract_seller_buyers = generateSupplierBuyerLink();
//		
//		if(includeInsertionQueries ) {
//			insertionTransactionList = getInsertionTransactions();
//		}
//		
//		if(includeBenchmarkQueries ) {
//			analyticalTransactionList = getBenchmarkingTransactions();
//		}
//		
//		LocalTime generationEndTime = LocalTime.now();
//		
//		writeMetaReport(randomSeed, generationStartTime, generationEndTime);
//		
//		CSVReport csvStat = new CSVReport(
//				"Stats.csv", new String[] {"noOfThreads","startInsertion","endInsertion","startAnalytical","endAnalytical","InsertionCount","AnalyticalCount"});
//		
//		for( int noOfThreads = 1; noOfThreads <= maxThreads; noOfThreads ++ ) {
//			
//			if( clearSQL ) {
//				transactionFactory.clearStateTransaction(transactionPlatformInstance).call();
//			}
//			
//			Executor exec = new Executor(noOfThreads);
//			List<Object> output = exec.executeTransactions(insertionTransactionList,analyticalTransactionList);
//			csvStat.writeToLog(output);
//		}
//		
//		csvStat.closeConnection();
//		System.out.println("Done !" + " RandomSeed - " + randomSeed);
//	}
//	
//	public static void printConfiguration() {
//		System.out.println("hostname : " + Configuration.mysqlHostName);
//		System.out.println("port : " + Configuration.mysqlPort);
//		System.out.println("User : " + Configuration.mysqlUserName);
//		System.out.println("Password : " + Configuration.mysqlPassword);
//		System.out.println("Report base : " + Configuration.reportBaseFolder);
//		System.out.println("Vendors: "+maxVendors);
//		System.out.println("Products : "+ maxProducts);
//		System.out.println("Max Transactions : "+maxBenchmarkTransactions);
//		
//		System.out.println("Min order quantity : "+ minOrderQuantity);
//		
//		System.out.println("Max order quantity : "+ maxOrderQuantity);
//		
//		System.out.println("Random seed : " + randomSeed);
//		System.out.println("Max Threads : " + maxThreads);
//		System.out.println("Include insertion queries ? "+ includeInsertionQueries);
//		
//		System.out.println("Include benchmark queries ? "+ includeBenchmarkQueries);
//		
//		System.out.println("Contract probability : "+ contractProbability);
//		System.out.print("BenchmarkChoice proabilities : ");
//		for( double val : benchmarkChoiceProbabilitiesArr) {
//			System.out.print(val+" ");
//		}
//	}
//	
//	public static void buildConfiguration() throws Exception {
//		
//		FileInputStream propertyFile = new FileInputStream(configPath);
//		Properties connectionProps = new Properties();
//		connectionProps.load(propertyFile);
//		
//		Configuration.mysqlHostName = connectionProps.getProperty("hostname");
//		Configuration.mysqlPort = connectionProps.getProperty("port");
//		Configuration.mysqlUserName = connectionProps.getProperty("user");
//		Configuration.mysqlPassword = connectionProps.getProperty("password");
//		Configuration.reportBaseFolder = connectionProps.getProperty("reportbase");
//		Configuration.maxVendors = maxVendors = Integer.parseInt(connectionProps.getProperty("maxvendors"));
//		Configuration.maxProducts = maxProducts = Integer.parseInt(connectionProps.getProperty("maxproducts"));
//		Configuration.maxBenchmarkTransactions = maxBenchmarkTransactions = 
//				Integer.parseInt(connectionProps.getProperty("maxbenchmarktransactions"));
//		
//		Configuration.minOrderQuantity = minOrderQuantity = 
//				Integer.parseInt(connectionProps.getProperty("minorderquantity"));
//		
//		Configuration.maxOrderQuantity = maxOrderQuantity = 
//				Integer.parseInt(connectionProps.getProperty("maxorderquantity"));
//		
//		Configuration.randomSeed = randomSeed = Long.parseLong(connectionProps.getProperty("randomseed"));
//		Configuration.maxThreads = maxThreads = Integer.parseInt(connectionProps.getProperty("maxthreads"));
//		
//		Configuration.clearPersistentState = clearSQL = 
//				Boolean.parseBoolean(connectionProps.getProperty("clearsql"));
//		
//		Configuration.includeInsertionQueries = includeInsertionQueries = 
//				Boolean.parseBoolean(connectionProps.getProperty("includeinsertionqueries"));
//		
//		Configuration.includeBenchmarkQueries = includeBenchmarkQueries = 
//				Boolean.parseBoolean(connectionProps.getProperty("includebenchmarkqueries"));
//		
//		Configuration.contractProbability = contractProbability = 
//				Double.parseDouble(connectionProps.getProperty("contractprobability"));
//		
//		String benchmarkChoiceProbabilitiesStr = 
//				connectionProps.getProperty("operationchoiceprobabilityarr");
//		
//		Configuration.benchmarkChoiceProbabilitiesArr = benchmarkChoiceProbabilitiesArr = 
//				Arrays.asList(benchmarkChoiceProbabilitiesStr.split(",")).stream().map(new Function<String, Double>() {
//
//					@Override
//					public Double apply(String t) {
//						return Double.parseDouble(t);
//					}
//				}).toArray(Double[]::new);
//		
//	}
//	
//	public static List<List<Transaction>> getInsertionTransactions() throws Exception{
//
//		List<List<Transaction>> transactionList = new ArrayList<List<Transaction>>();
//		
//		transactionList.add(transactionFactory.createVendorTransactions(
//				transactionPlatformInstance, maxVendors));
//		
//		transactionList.add(transactionFactory.createProductTransactions(
//				transactionPlatformInstance, maxProducts));
//		
//		transactionList.add(transactionFactory.updateVendorInventory(
//				transactionPlatformInstance, maxVendors, maxProducts, maxInitialInventory));
//		
//		transactionList.add(transactionFactory.generateContractTransactions(
//				transactionPlatformInstance, maxOrderQuantity, contract_seller_buyers, 
//				maxProducts));
//		
//		return transactionList;
//	}
//	
//	public static List<Transaction> getBenchmarkingTransactions() throws Exception {
//		
//		List<Transaction> transactionList = new ArrayList<Transaction>();
//		
//		for( int i = 0; i < maxBenchmarkTransactions; i++ ) {
//			
//			int operation = generateRandomAnalyticalOperation();
//			
//			switch(operation) {
//				case 1 : transactionList.add(
//							transactionFactory.generateOrderTransaction(
//								transactionPlatformInstance,contract_seller_buyers, 
//								maxVendors, maxProducts, minOrderQuantity, 
//								maxOrderQuantity));
//				
//						  break;
//						  
//				case 2 :  transactionList.add(
//							transactionFactory.generateCalcInvDaysOfSupplyTransaction(
//									transactionPlatformInstance, maxVendors, maxProducts));
//				
//						  break;
//						  
//				case 3 :  transactionList.add(
//							transactionFactory.generateCalcBullwhipCoeffTransaction(
//									transactionPlatformInstance));
//				
//						  break;
//						  
//				default:  continue;
//			}
//		}
//		
//		return transactionList;
//	}
//	
//	public static void writeMetaReport(long randomSeed, 
//			LocalTime generationStartTime, LocalTime generationEndTime) throws Exception {
//		
//		TXTReport metaReport = new TXTReport("Meta.txt");
//		
//		metaReport.writeToLog("Random Seed : " + randomSeed + "\n"
//				+"Query Generation Start Time : " + generationStartTime + "\n"
//    			+"Query Generation End Time : " + generationEndTime);
//		
//		metaReport.closeConnection();
//	}
//	
//	public static Map<Integer,List<Integer>> generateSupplierBuyerLink(){
//		
//		Map<Integer,List<Integer>> supplier_buyerMap = 
//				new HashMap<Integer, List<Integer>>();
//		
//		for( int vendor = 1; vendor <= maxVendors; vendor++ ) {
//			supplier_buyerMap.put(vendor, 
//					getBuyersForSupplier(vendor));
//			
//		}
//		
//		return supplier_buyerMap;
//	}
//	
//	public static List<Integer> getBuyersForSupplier(
//			int supplier){
//		
//		List<Integer> links = new ArrayList<Integer>();
//		
//		int start = (supplier + 1) % maxVendors;
//		
//		if( start == 0 ) {
//			start = maxVendors;
//		}
//		
//		for( int i = start; i != supplier; i++ ) {
//			
//			double classifier = randomNumberGenerator.getRandomDouble();
//			
//			if( Double.compare(classifier, 
//					contractProbability) <= 0 ) {
//				
//				links.add(i);
//			}
//			
//			if( i == maxVendors ) {
//				i = 0;
//			}
//		}
//		
//		return links;
//	}
//	
//	public static int generateRandomAnalyticalOperation() {
//		
//		DoubleFunction<Integer> probClassifier = (double classifier) -> {
//			
//			double sum = benchmarkChoiceProbabilitiesArr[0];
//			
//			int i = 1;
//			
//			for( ; i < benchmarkChoiceProbabilitiesArr.length; i++ ) {
//				if( Double.compare(classifier, sum ) <= 0 ) {
//					break;
//				}
//				
//				sum += benchmarkChoiceProbabilitiesArr[i];
//			}
//			
//			return i;
//		};
//		
//		double choice = randomNumberGenerator.getRandomDouble();
//		
//		return probClassifier.apply(choice);
//	}
//}
