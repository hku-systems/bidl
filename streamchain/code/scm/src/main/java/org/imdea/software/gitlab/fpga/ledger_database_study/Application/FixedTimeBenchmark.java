package org.imdea.software.gitlab.fpga.ledger_database_study.Application;

import java.util.List;
import java.util.Map;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.CSVReport;
import org.imdea.software.gitlab.fpga.ledger_database_study.Application.tools.ApplicationTools;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.Executor;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.sql.TransactionalSQLImpl;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration.TransactionType;

public class FixedTimeBenchmark {
	
	public static void main(String[] args) throws Exception {
		
		if( args.length == 0 ) {
			System.err.print("Missing argument - path to configuration file");
			return;
		}
		
		String configPath = args[0];
		
		ApplicationTools appTools = ApplicationTools.getInstance(configPath, 
				TransactionalSQLImpl.getInstance());
		
		Map<TransactionType,List<List<Transaction>>> type_TransactionMap = 
				appTools.generateTransactions();
		
		CSVReport csvStat = new CSVReport(
				"Stats.csv", new String[] {"noOfThreads","BaseLoadTransactions","OrderTransaction","InventoryDaysOfSupply","Bullwhip-Coefficient","startInsertion","endInsertion","startAnalytical",
						"endAnalytical","InsertionCount","AnalyticalCount"},true);
		
		for( int noOfThreads = Configuration.minThreads; noOfThreads <= Configuration.maxThreads; noOfThreads += 20 ) {
			
			if( Configuration.clearPersistentState ) {
				appTools.clearPersistentState();
			}
			
//			int threads = (int) Math.pow(2, noOfThreads);
			Executor exec = new Executor((noOfThreads));
			List<Object> output = exec.executeTransactions(type_TransactionMap, Configuration.fixedRunSeconds);
			csvStat.writeToLog(output);
			
			if( Configuration.sleepBetweenBatchSeconds > 0 ) {
				Thread.sleep(Configuration.sleepBetweenBatchSeconds * 1000);
			}
		}
		
		csvStat.closeConnection();
		System.out.println("Done !" + " RandomSeed - " + Configuration.randomSeed);
	}
}
