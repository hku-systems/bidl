package org.imdea.software.gitlab.fpga.ledger_database_study.common;

public class Configuration {

	public static String mysqlUserName;
	public static String mysqlPassword;
	public static String mysqlHostName;
	public static String mysqlPort;
	
	public static String reportBaseFolder;
	
	public static int maxVendors;
	public static int maxProducts;
	public static int maxBenchmarkTransactions;
	
	public static int minOrderQuantity;
	public static int maxOrderQuantity;
	
	public static int minBaseTransactions;
	public static int maxInitialInventory;
	
	public static long randomSeed;
	
	public static int minThreads;
	public static int maxThreads;
	
	public static boolean clearPersistentState;
	public static boolean includeInsertionQueries;
	public static boolean includeBenchmarkQueries;
	
	public static int sleepBetweenBatchSeconds;
	public static long fixedRunSeconds;
	
	public static double contractProbability;
	public static Double[] benchmarkChoiceProbabilitiesArr;
	
	public static String fabricWorkloadDirectoryPath;
	
	public enum TransactionType{
		Load, Benchmark
	}
}
