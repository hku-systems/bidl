package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.fabric;

import java.util.Arrays;
import java.util.function.Consumer;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Operation;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.TransactionOutput;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

public abstract class TransactionFabric implements Transaction{

	String functionName;
	String arguments[];
	
	public TransactionFabric(String functionName, String ...arguments) {
		this.functionName = functionName;
		this.arguments = arguments;
	}

	@Override
	public TransactionOutput call() throws Exception {
		
		JsonArray jsonArr = new JsonArray();
		jsonArr.add(functionName);
		
		Arrays.asList(arguments).stream().forEach(new Consumer<String>() {

			@Override
			public void accept(String argument) {
				jsonArr.add(argument);
			}
		});
		
		JsonObject object = new JsonObject();
		object.add("Args", jsonArr);
		
		return new TransactionOutput(0l, 0l, null, null, object.toString(), 0l, getOperationName());
	}
	
	public abstract Operation getOperationName();
}