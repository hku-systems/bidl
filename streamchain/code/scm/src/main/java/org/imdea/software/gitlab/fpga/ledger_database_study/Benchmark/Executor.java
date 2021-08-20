package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.CSVReport;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.TransactionOutput;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration.TransactionType;

public class Executor {

	private int noOfThreads;
	private static CSVReport csvReport;
	
	public Executor(int numberOfThreads) throws Exception {
		
		if( csvReport == null ) {
			 csvReport = new CSVReport(
						"detailedReport.csv", TransactionOutput.getHeader(),true);
			 
			 csvReport.closeConnection();
		}
		
		this.noOfThreads = numberOfThreads;
	}
	
	public void executeAndLogTransactions(
			List<Transaction> transactionList,String name) throws Exception {
		
		CSVReport csvReport = new CSVReport(
				"Output-" + name  + "-" + noOfThreads + ".csv", TransactionOutput.getHeader(),true);
		
    	ExecutorService executorService = Executors.newFixedThreadPool(noOfThreads);
		
		List<Future<TransactionOutput>> futures = executorService.invokeAll(transactionList);
		
		for( Future<TransactionOutput> future : futures ) {
			csvReport.writeToLog(future.get().getCSVRecord());
		}
		
		csvReport.closeConnection();
		executorService.shutdown();
    }
	
	public List<Object> executeTransactions(
			Map<TransactionType,List<List<Transaction>>> transactionListsMap, long timeout) throws Exception {
		
		List<List<Transaction>> listLoadTransactionList = 
				transactionListsMap.get(TransactionType.Load);
		
		List<List<Transaction>> analyticalTransactionList = 
				transactionListsMap.get(TransactionType.Benchmark); 
		
		List<Transaction> unifiedAnalyticalTransactionList = new ArrayList<Transaction>();
		
		for( List<Transaction> benchmarkTransactionList : analyticalTransactionList ) {
			unifiedAnalyticalTransactionList.addAll(benchmarkTransactionList);
		}
		
		ThreadPoolExecutor executorService = new ThreadPoolExecutor(noOfThreads, noOfThreads, 0l, 
    			TimeUnit.MILLISECONDS, new LinkedBlockingQueue<Runnable>());
    	executorService.prestartAllCoreThreads();
		
    	csvReport.openConnection("detailedReport.csv",true);
    	
    	List<Future<TransactionOutput>> futuresLoad = 
    			new ArrayList<Future<TransactionOutput>>();
    	
    	List<Future<TransactionOutput>> futuresBenchmark = 
    			new ArrayList<Future<TransactionOutput>>();
    	
    	List<List<Object>> outputCSV = new ArrayList<List<Object>>();
    	
    	if( listLoadTransactionList != null && listLoadTransactionList.size() > 0 ) {
    		for( List<Transaction> loadTransactionList : listLoadTransactionList ) {
    			if( loadTransactionList != null && loadTransactionList.size() > 0 ) {
    				List<Future<TransactionOutput>> futureList = 
    						executorService.invokeAll(loadTransactionList);
    				
	        		futuresLoad.addAll(futureList);
    			}
        	}
    	}
    	
    	if( Configuration.sleepBetweenBatchSeconds > 0 ) {
			Thread.sleep(Configuration.sleepBetweenBatchSeconds * 1000);
		}
    	
    	Long startInsertionTimestamp = -1l;
		Long endInsertionTimestamp = -1l;
		Integer noOfInsertionTransactions = 0;
    	
		Long start = System.nanoTime();
    	
    	if( unifiedAnalyticalTransactionList != null && unifiedAnalyticalTransactionList.size() > 0 ) {
    		List<Future<TransactionOutput>> futureList;
    		
    		if( timeout > 0 ) {
    			futureList = executorService.invokeAll(unifiedAnalyticalTransactionList, timeout, TimeUnit.SECONDS);
    		} else {
    			futureList = executorService.invokeAll(unifiedAnalyticalTransactionList);
    		}
    			
    		executorService.shutdown();
    		futuresBenchmark.addAll(futureList);
    	}else {
    		executorService.shutdown();
    	}
    	
		Long startAnalyticalTimestamp = -1l;
		Long endAnalyticalTimestamp = -1l;
		
		Integer noOfAnalyticalTransactions = 0;
		
		for( Future<TransactionOutput> future : futuresLoad) {
			
			if(future.isCancelled())
				continue;
			
			TransactionOutput opOutput = future.get();
				
			if( startInsertionTimestamp == -1 ) {
				startInsertionTimestamp = opOutput.getStartTimestamp();
			}

			if( opOutput.getEndTimestamp() > endInsertionTimestamp ) {
				endInsertionTimestamp = opOutput.getEndTimestamp();
			}

			noOfInsertionTransactions++;

			List<Object> csvOutput = new ArrayList<Object>(Arrays.asList((Object)noOfThreads));
			csvOutput.addAll(opOutput.getCSVRecord());
			outputCSV.add(csvOutput);
		}
		
		for( Future<TransactionOutput> future : futuresBenchmark) {
			
			if(future.isCancelled())
				continue;
			
			TransactionOutput opOutput = future.get();

			if( startAnalyticalTimestamp == -1 ) {
				startAnalyticalTimestamp = opOutput.getStartTimestamp();
			}

			if( opOutput.getEndTimestamp() > endAnalyticalTimestamp ) {
				endAnalyticalTimestamp = opOutput.getEndTimestamp();
			}

			noOfAnalyticalTransactions++;
			List<Object> csvOutput = new ArrayList<Object>(Arrays.asList((Object)noOfThreads));
			csvOutput.addAll(opOutput.getCSVRecord());
			outputCSV.add(csvOutput);
		}
		
		Long end = System.nanoTime();
		
		System.out.println(("Time to execute "+ (double)(end-start)/(1000*1000*1000))+"\nThroughput : "+ (double)((noOfAnalyticalTransactions)/((double)(end-start)/(1000*1000*1000))));
		System.out.println("Writing output files");
		
		for( List<Object> object : outputCSV ) {
			csvReport.writeToLog(object);
		}
		
		csvReport.closeConnection();
		
		return Arrays.asList(new Object[] {noOfThreads,Configuration.minBaseTransactions,Configuration.benchmarkChoiceProbabilitiesArr[0],
				Configuration.benchmarkChoiceProbabilitiesArr[1],Configuration.benchmarkChoiceProbabilitiesArr[2],
				startInsertionTimestamp.toString(),endInsertionTimestamp.toString(),
				startAnalyticalTimestamp.toString(),endAnalyticalTimestamp.toString(),
				noOfInsertionTransactions.toString(),noOfAnalyticalTransactions.toString()});
    }
}