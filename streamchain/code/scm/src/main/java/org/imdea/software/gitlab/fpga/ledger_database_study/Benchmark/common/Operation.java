package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common;

public enum Operation {

	ADD_CONTRACT("Add Contract", "AC", false), 
	UPDATE_INVENTORY("Update Inventory", "UI", false),
	ADD_PRODUCT("Add Product","AP", false),
	ADD_VENDOR("Add Vendor", "AV", false),
	PLACE_ORDER("Place Order", "PO", true), 
	STAT_DAYS_OF_SUPPLY("calcDaysofSupply", "S_DS", true),
	STAT_BULLWHIP_COEFFICIENT("calcBullwhipCoeff","S_BWC", true),
	CLEAR_STATE("clearState","CLR_STATE", false);
	
	private String operationName;
	private String shortCode;
	private boolean isAnalytical;
	
	private Operation(String operation, String shortCode, boolean isAnalytical) {
		this.operationName = operation;
		this.shortCode = shortCode;
		this.isAnalytical = isAnalytical;
	}
	
	public String getShortCode() {
		return this.shortCode;
	}
	
	public boolean isAnalytical() {
		return this.isAnalytical;
	}
	
	public String getOperationName() {
		return this.operationName;
	}
	
	@Override
	public String toString() {
		return this.getShortCode();
	}
}