package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common;

import java.util.concurrent.Callable;

public interface Transaction extends Callable<TransactionOutput>{

	TransactionOutput call() throws Exception;
}
