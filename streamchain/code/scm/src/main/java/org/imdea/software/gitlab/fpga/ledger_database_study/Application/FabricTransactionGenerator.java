package org.imdea.software.gitlab.fpga.ledger_database_study.Application;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.TXTReport;
import org.imdea.software.gitlab.fpga.ledger_database_study.Application.tools.ApplicationTools;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.fabric.TransactionalFabricImpl;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Configuration.TransactionType;

public class FabricTransactionGenerator {
	static int loadNum = 1;
	public static void main(String[] args) throws Exception {
		if( args.length == 0 ) {
			System.err.print("Missing argument - path to configuration file");
			System.exit(1);
		}
		
		String configPath = args[0];
		
		ApplicationTools appTools = ApplicationTools.getInstance(configPath, 
				new TransactionalFabricImpl());
		
		if( Configuration.fabricWorkloadDirectoryPath == null 
				|| Configuration.fabricWorkloadDirectoryPath == "") {
			
			System.err.print("Fabric workload directory not defined !");
			System.exit(1);
		}
		
		if( Configuration.includeInsertionQueries) {
			List<List<Transaction>> insertionTransactions = 
					appTools.getInsertionTransactions();

			insertionTransactions.forEach(new Consumer<List<Transaction>>() {

				@Override
				public void accept(List<Transaction> transactions) {
					TXTReport fabricLoadWorkload;
					try {
						fabricLoadWorkload = new TXTReport(
								Configuration.fabricWorkloadDirectoryPath,
								"workload-load-"+ (loadNum++)+".template", false);

						for( Transaction transaction : transactions	) {
							try {
								fabricLoadWorkload.writeToLog(transaction.call().getQuery()+"\n");
								System.out.println(transaction.call().getQuery());
							} catch (IOException e) {
								e.printStackTrace();
							} catch (Exception e) {
								e.printStackTrace();
							}
						}
						fabricLoadWorkload.closeConnection();
					} catch (Exception e1) {
						e1.printStackTrace();
					}
				}
			});
		}
		
		if( Configuration.includeBenchmarkQueries ) {
			TXTReport fabricBenchmarkWorkload = new TXTReport(
					Configuration.fabricWorkloadDirectoryPath,
					"workload-benchmark.template", false);

			List<Transaction> benchmarkTransactionList = appTools.getBenchmarkingTransactions();

			benchmarkTransactionList.forEach(new Consumer<Transaction>() {

				@Override
				public void accept(Transaction t) {
					try {
						fabricBenchmarkWorkload.writeToLog(t.call().getQuery()+"\n");
					} catch (IOException e) {
						e.printStackTrace();
					} catch (Exception e) {
						e.printStackTrace();
					}
				}
			});

			fabricBenchmarkWorkload.closeConnection();
		}
		System.out.println("Done !" + " RandomSeed - " + Configuration.randomSeed);
	}
}
