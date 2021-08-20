package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TransactionOutput {

	Long startTimestamp;
	Long endTimestamp;
	String output;
	Boolean status;
	long executionTimeNS;
	Operation operation;
	String parameters;
	
	public TransactionOutput(Long startTimestamp, Long endTimestamp, 
			String output, Boolean status, String parameters, 
			long executionTimeNS, Operation operation) {
		
		this.startTimestamp = startTimestamp;
		this.endTimestamp = endTimestamp;
		this.output = output;
		this.status = status;
		this.parameters = parameters;
		this.executionTimeNS = executionTimeNS;
		this.operation = operation;
	}

	public Long getStartTimestamp() {
		return startTimestamp;
	}

	public void setStartTimestamp(Long startTimestamp) {
		this.startTimestamp = startTimestamp;
	}

	public Long getEndTimestamp() {
		return endTimestamp;
	}

	public void setEndTimestamp(Long endTimestamp) {
		this.endTimestamp = endTimestamp;
	}

	public String getOutput() {
		return output;
	}

	public void setOutput(String output) {
		this.output = output;
	}

	public Boolean getStatus() {
		return status;
	}

	public void setStatus(Boolean status) {
		this.status = status;
	}

	public String getQuery() {
		return parameters;
	}

	public void setQuery(String parameters) {
		this.parameters = parameters;
	}
	
	public Operation getOperation() {
		return operation;
	}

	public void setOperation(Operation operation) {
		this.operation = operation;
	}

	public List<Object> getCSVRecord(){
		return new ArrayList<Object>(Arrays.asList((Object) startTimestamp, (Object) endTimestamp, 
				(Object) output, (Object) status, (Object) operation.toString(), 
				(Object) parameters, (Object) executionTimeNS));
		
	}
	
	public static String[] getHeader() {
		return (new String[]{"noOfThreads","JobStartTime","JobEndTime","output","status",
				"Operation","Parameters","TransactionExecutionTime(ns)"});
		
	}
}
