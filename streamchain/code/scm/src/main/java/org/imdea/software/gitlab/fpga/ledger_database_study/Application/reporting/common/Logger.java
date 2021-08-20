package org.imdea.software.gitlab.fpga.ledger_database_study.Application.reporting.common;

import java.io.IOException;

public interface Logger<T> {

	void openConnection(String logIdentifier, boolean isAppend) throws IOException, Exception;
	void writeToLog(T object) throws IOException;
	void closeConnection() throws IOException;
}
