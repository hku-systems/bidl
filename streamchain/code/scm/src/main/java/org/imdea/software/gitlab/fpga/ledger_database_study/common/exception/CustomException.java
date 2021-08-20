package org.imdea.software.gitlab.fpga.ledger_database_study.common.exception;

public class CustomException extends Exception{

	private ExceptionType type;
	private String message;
	
	public CustomException( ExceptionType type, String message) {
		this.type = type;
		this.message = message;
	}
	
	public ExceptionType getExceptionType() {
		return type;
	}
	
	public String getExceptionMessage() {
		return message;
	}
}
