package org.imdea.software.gitlab.fpga.ledger_database_study.common;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;

public interface Transactional {
	
	Transaction clearState() throws Exception;

	Transaction placeOrder(long orderKey, int supplierKey, 
			int buyerKey, int productKey, int orderQuantity, int orderDate, 
			int orderMonth, int orderYear, int orderHour, int orderMin, 
			int orderSec) throws Exception;
	
	Transaction createContract(int supplierKey, int buyerKey, int productKey,
			int orderLimit, int date, int year, int month) throws Exception;
	
	Transaction updateInventory(int productKey, int vendorKey, 
			int quantity, int date, int month, int year, int hours, int minutes, 
			int seconds) throws Exception;
	
	Transaction createProduct(int productkey, String productTitle) throws Exception;
	Transaction createVendor(int vendorKey, String vendorTitle) throws Exception;
	
	Transaction findInventoryDaysOfSupply(int vendorKey, int productKey) throws Exception;
	Transaction findBullwhipCoefficient() throws Exception;
}
