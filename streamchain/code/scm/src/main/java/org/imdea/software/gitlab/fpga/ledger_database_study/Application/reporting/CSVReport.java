package org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVPrinter;
import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.common.FilesystemReport;

public class CSVReport extends FilesystemReport<Object>{

	private CSVPrinter printer;
	
	public CSVReport(String fileName, String[] csvHeader, boolean isAppend) throws Exception {
		super();
		BufferedWriter writer = this.getWriter(fileName, isAppend);
		this.printer = new CSVPrinter(writer, CSVFormat.DEFAULT.withHeader(
				csvHeader));
	}
	
	@Override
	public void openConnection(String logIdentifier, boolean isAppend) throws Exception {
		BufferedWriter writer = this.getWriter(logIdentifier, isAppend);
		this.printer = new CSVPrinter(writer, CSVFormat.DEFAULT);
	}

	@Override
	public void writeToLog(Object object) throws IOException {
		if( object instanceof List<?> ) {
			printer.printRecord((List<Object>)object);	
		}else {
			printer.printRecord(object);
		}
		printer.flush();
	}

	@Override
	public void closeConnection() throws IOException {
		printer.flush();
		printer.close();
	}
}