package org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting;

import java.io.BufferedWriter;
import java.io.IOException;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.common.FilesystemReport;

public class TXTReport extends FilesystemReport<String> {

	BufferedWriter writer;
	
	public TXTReport(String fileName, boolean isAppend) throws Exception {
		super();
		writer = getWriter(fileName, isAppend); 
	}
	
	public TXTReport(String folder,String fileName, boolean isAppend) throws Exception {
		super(folder);
		writer = getWriter(fileName, isAppend); 
	}
	
	@Override
	public void openConnection(String logIdentifier, boolean isAppend) throws IOException {
		
	}

	@Override
	public void writeToLog(String object) throws IOException {
		writer.write(object);
	}

	@Override
	public void closeConnection() throws IOException {
		writer.flush();
		writer.close();
	}
}