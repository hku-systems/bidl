package org.imdea.software.gitlab.fpga.ledger_database_study.Application.tools;

import java.time.LocalTime;

import org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.TXTReport;

public class ReportTools {
	
	public static void writeMetaReport(long randomSeed, 
			LocalTime generationStartTime, LocalTime generationEndTime,
			String configurationList) throws Exception {
		
		TXTReport metaReport = new TXTReport("Meta.txt",true);
		
		metaReport.writeToLog("Random Seed : " + randomSeed + "\n"
				+"Query Generation Start Time : " + generationStartTime + "\n"
    			+"Query Generation End Time : " + generationEndTime+ "\n"
    			+ configurationList);
		
		metaReport.closeConnection();
	}
}
