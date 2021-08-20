package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.sql;

import java.sql.CallableStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Operation;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.TransactionOutput;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.TimeUtils;

public abstract class TransactionSQL implements Transaction {

	public TransactionOutput call() throws Exception {
		
		boolean status = false;
		boolean hasResults = false;
		
		StringBuilder opOutput = new StringBuilder();
		
		Long startTimestamp = TimeUtils.getNanoTime();
		
		CallableStatement procedure = getCallableStatement();
		initializeParameters(procedure);
		
		long currTime = TimeUtils.getNanoTime();
		
		try {
			hasResults = procedure.execute();
			status = true;
		} catch (SQLException e) {
			e.printStackTrace();
			opOutput.append(e.getSQLState());
		}
		long expendedTime = TimeUtils.getNanoTime();

		long periodOfExecution = expendedTime - currTime;
		while( hasResults ) {
			ResultSet rs = procedure.getResultSet();
			if( rs.next() ) {
				if( rs.getInt(1) == 0 ) {
					status = false;  
				}
				opOutput.append(rs.getString(2));
			}
			rs.close();
			break;
		}
		
		Long endTimestamp = TimeUtils.getNanoTime();
		procedure.getConnection().close();
		return new TransactionOutput(startTimestamp, endTimestamp, 
				opOutput.toString(), status, getParametersCSV(),
				periodOfExecution,getOperationName());
		
	}
	
	public abstract CallableStatement initializeParameters(CallableStatement statement) throws SQLException;
	public abstract String getParametersCSV();
	public abstract Operation getOperationName();
	public abstract CallableStatement getCallableStatement() throws Exception;
}
