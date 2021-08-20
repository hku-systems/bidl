package org.imdea.software.gitlab.fpga.ledger_database_study.Application.generator;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Transactional;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.RandomGenerationUtils;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.TimeUtils;

public class TransactionFactory {

	private AtomicLong orderIndex;

	private RandomGenerationUtils randomGen = null;
	
	private static TransactionFactory __instance = null;
	
	private TransactionFactory(RandomGenerationUtils random) {
		randomGen = random;
		Long currTimeStamp = new Date().getTime();
		orderIndex = new AtomicLong(currTimeStamp * 10);
	}
	
	public static TransactionFactory getInstance(RandomGenerationUtils random) {
		if( __instance == null ) {
			__instance = new TransactionFactory(random);
		}
		
		return __instance;
	}
	
	public List<Transaction> generateContractTransactions(Transactional contractImpl,int orderLimit, 
			Map<Integer,List<Integer>> seller_buyerList, int maxProducts) throws Exception {
		
		
		List<Transaction> transactionList = new ArrayList<Transaction>();
		for( int seller : seller_buyerList.keySet() ) {
			for( int buyer : seller_buyerList.get(seller) ) {
				for( int product = 1; product <= maxProducts; product ++) {
					Date date = TimeUtils.getSimulatedYearEnd(10);
					transactionList.add(
							contractImpl.createContract(seller, buyer, product, orderLimit, 
									date.getDate(), ( date.getYear() + 1900), (date.getMonth() + 1)));
				}
			}
		}
		
		return transactionList;
	}
	
	public List<Transaction> createVendorTransactions(
			Transactional contractImpl, int noOfVendors) throws Exception{
		
		List<Transaction> transactionList = new ArrayList<Transaction>();
		
		for( int vendor = 1; vendor <= noOfVendors; vendor++ ) {
			transactionList.add(contractImpl.createVendor(vendor, "Vendor"+vendor));
		}
		
		return transactionList;
	}
	
	public List<Transaction> createProductTransactions(
			Transactional contractImpl, int noOfProducts) throws Exception{
		
		List<Transaction> transactionList = new ArrayList<Transaction>();
		
		for( int product = 1; product <= noOfProducts; product++ ) {
			transactionList.add(contractImpl.createProduct(product, "Product"+product));
		}
		
		return transactionList;
	}
	
	public List<Transaction> updateVendorInventory(Transactional contract, int noOfVendors, int maxProducts, int quantity) throws Exception{
		
		List<Transaction> transactionList = new ArrayList<Transaction>();
		
		for( int vendor = 1; vendor <= noOfVendors; vendor++ ) {
			for( int product = 1; product <= maxProducts; product++ ) {
				Date date = TimeUtils.getSimulatedDate();
				transactionList.add(
						contract.updateInventory(product, vendor, quantity, 
								date.getDate(), (date.getMonth() + 1), 
								(date.getYear() + 1900), date.getHours(), 
								date.getMinutes(), date.getSeconds()));
				
			}
		}
		return transactionList;
	}
	
	public Transaction generateOrderTransaction(Transactional contract, Map<Integer,List<Integer>> seller_buyerList, int maxVendors, int maxProducts, int minOrderQuantity, int maxOrderQuantity) throws Exception {
		
		int productKey = randomGen.getInteger(1, maxProducts);
		int orderQuantity = randomGen.getInteger(minOrderQuantity, maxOrderQuantity);
		
		int buyerKey = -1;
		int supplierKey = -1;
		
		while ( buyerKey == -1 || supplierKey == -1 ) {
			
			int sellerKeyTemp = randomGen.getInteger(1, maxVendors);
			
			if( ! ( seller_buyerList.containsKey(sellerKeyTemp) 
					&& ( seller_buyerList.get(sellerKeyTemp) != null ) 
					&& ( seller_buyerList.get(sellerKeyTemp).size() > 0 ) ) ) {
				
				continue;
			}
			
			int maxBuyerArrayIndex = seller_buyerList.get(sellerKeyTemp).size() - 1;
			
			int buyerKeyArrayIndex = randomGen.getInteger(0, maxBuyerArrayIndex);
			
			buyerKey = seller_buyerList.get(sellerKeyTemp).get(buyerKeyArrayIndex);
			supplierKey = sellerKeyTemp;
		}
		
		Date transactionDate = TimeUtils.getSimulatedDate(); 
		
		return contract.placeOrder(orderIndex.getAndIncrement(),supplierKey, 
				buyerKey, productKey, orderQuantity, 
				transactionDate.getDate(), (transactionDate.getMonth() + 1), 
				(transactionDate.getYear() + 1900), transactionDate.getHours(), 
				transactionDate.getMinutes(), transactionDate.getSeconds());
		
	}
	
	public Transaction generateCalcBullwhipCoeffTransaction(Transactional contract) throws Exception {
		
		return contract.findBullwhipCoefficient();
	}
	
	public Transaction generateCalcInvDaysOfSupplyTransaction(Transactional contract, 
			int maxVendors, int maxProducts ) throws Exception {
		
		int vendorKey = randomGen.getInteger(1, maxVendors - 1);
		int productKey = randomGen.getInteger(1, maxProducts);
		
		return contract.findInventoryDaysOfSupply(vendorKey, productKey);
	}
	
	public Transaction clearStateTransaction(Transactional contract) throws Exception {
		return contract.clearState();
	}
}