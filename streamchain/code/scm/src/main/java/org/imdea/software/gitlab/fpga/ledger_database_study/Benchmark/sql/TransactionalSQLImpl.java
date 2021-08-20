package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.sql;

import java.sql.CallableStatement;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Timestamp;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Operation;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Transactional;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.sql.SQLConnection;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.TimeUtils;

public class TransactionalSQLImpl implements Transactional{

	private static TransactionalSQLImpl _instance;
	
//	private Connection connection;
	
	private TransactionalSQLImpl() throws Exception {
//		if(connection == null) {
//			connection = SQLConnection.getInstance().getConnection(); 
//		}
	}
	
	public static TransactionalSQLImpl getInstance() throws Exception {
		
		if(_instance == null) {
			_instance = new TransactionalSQLImpl();
		}
		
		return _instance;
	}
	
	public Transaction placeOrder(long orderKey, int supplierKey, 
			int buyerKey, int productKey, int orderQuantity, int orderDate, 
			int orderMonth, int orderYear, int orderHour, int orderMin, 
			int orderSec) throws Exception {

		class OrderSQLTransaction extends TransactionSQL {
			
			private final long orderKey;
			private final int supplierKey;
			private final int buyerKey;
			private int productKey;
			private int orderQuantity;
			private final Timestamp orderDateTime;
			
			public OrderSQLTransaction(long orderKey,int supplierKey, int buyerKey, int productKey, int orderQuantity,
					Timestamp orderDateTime) {

				this.orderKey = orderKey;
				this.supplierKey = supplierKey;
				this.buyerKey = buyerKey;
				this.productKey = productKey;
				this.orderQuantity = orderQuantity;
				this.orderDateTime = orderDateTime;
			}

			@Override
			public CallableStatement initializeParameters(
					CallableStatement placeOrderStatement) throws SQLException {
				
				placeOrderStatement.setLong(1, orderKey);
				placeOrderStatement.setInt(2, supplierKey);
				placeOrderStatement.setInt(3, buyerKey);
				placeOrderStatement.setInt(4, productKey);
				placeOrderStatement.setInt(5, orderQuantity);
				placeOrderStatement.setTimestamp(6, orderDateTime);
				
				return placeOrderStatement;
			}

			@Override
			public String getParametersCSV() {
				return "ordrKey: " + orderKey + " "
						+ "spKey: " + supplierKey + " "
						+ "byKey: " + buyerKey +" "
						+ "pdtKey: " + productKey + " "
						+ "ordQty: " + orderQuantity + " "
						+ "ordDate: " + TimeUtils.formatSQLTime(orderDateTime);
				
			}

			@Override
			public Operation getOperationName() {
				return Operation.PLACE_ORDER;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception{
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("placeOrder", 6));
				
			}
		}
		
		TransactionSQL transaction = new OrderSQLTransaction(orderKey, supplierKey, 
				buyerKey, productKey, orderQuantity, 
				TimeUtils.getSQLTimestamp(
						orderDate, orderMonth, orderYear, 
						orderHour, orderMin, orderSec));
		
		return transaction;
	}

	public Transaction createContract(int supplierKey, int buyerKey, int productKey,
			int orderLimit, int date, int year, int month) throws Exception {
		
		class ContractRegTransaction extends TransactionSQL{
			
			private int supplierKey;
			private int buyerKey;
			private int productKey;
			private int orderLimit;
			private Date expiryDate;
			
			public ContractRegTransaction(int supplierKey,
					int buyerKey, int productKey,
					int orderLimit, Date expiryDate) {
				
				this.supplierKey = supplierKey;
				this.buyerKey = buyerKey;
				this.productKey = productKey;
				this.orderLimit = orderLimit;
				this.expiryDate = expiryDate;
			}

			@Override
			public CallableStatement initializeParameters(
					CallableStatement ctCreationStmt) throws SQLException {
				
				ctCreationStmt.setInt(1, supplierKey);
				ctCreationStmt.setInt(2, buyerKey);
				ctCreationStmt.setInt(3, productKey);
				ctCreationStmt.setInt(4, orderLimit);
				ctCreationStmt.setDate(5, expiryDate);
				
				return ctCreationStmt;
			}

			@Override
			public String getParametersCSV() {
				return ( " spKey: " + supplierKey 
						+ " byKey: " + buyerKey 
						+ " pdtKey: " + productKey 
						+ " ordLmt: " + orderLimit 
						+ " validUntil: " + TimeUtils.formatSQLDate(expiryDate));
				
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_CONTRACT;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception{
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("createContract", 5));
				
			}
		}
		
		TransactionSQL transaction = new ContractRegTransaction(supplierKey, buyerKey, 
				productKey, orderLimit, TimeUtils.getSQLDatestamp(date, month, year));
		
		return transaction;
	}

	public Transaction updateInventory(int productKey, int vendorKey, 
			int quantity, int date, int month, int year, int hours, int minutes, 
			int seconds) throws Exception {
		
		class InventoryUpdateSQLTransaction extends TransactionSQL {

			private final int productKey;
			private final int vendorKey;
			private final int quantity;
			private final Timestamp inventoryDateTime;
			
			public InventoryUpdateSQLTransaction(int productKey, int vendorKey, int quantity,
					Timestamp inventoryDateTime) {
			
				this.productKey = productKey;
				this.vendorKey = vendorKey;
				this.quantity = quantity;
				this.inventoryDateTime = inventoryDateTime;
			}

			@Override
			public CallableStatement initializeParameters(CallableStatement updateInventoryStatement) throws SQLException {
				updateInventoryStatement.setInt(1, productKey);
				updateInventoryStatement.setInt(2, vendorKey);
				updateInventoryStatement.setInt(3, quantity);
				updateInventoryStatement.setTimestamp(4, inventoryDateTime);
				
				return updateInventoryStatement;
			}

			@Override
			public String getParametersCSV() {
				return " pdtKey: " + productKey
						+" vendorKey: " + vendorKey
						+" qty: " + quantity
						+" inventoryTime: "+ TimeUtils.formatSQLTime(inventoryDateTime);
				
			}

			@Override
			public Operation getOperationName() {
				return Operation.UPDATE_INVENTORY;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception {
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("updateInventory", 4));
				
			}
		}
		
		TransactionSQL transaction = new InventoryUpdateSQLTransaction(productKey, vendorKey, quantity, 
				TimeUtils.getSQLTimestamp(date, month, year, hours, minutes, seconds));
		
		return transaction;
	}

	public Transaction createProduct(int productkey, String productTitle) throws Exception {
		
		class ProductRegistrationTransaction extends TransactionSQL{
			
			private final int productKey;
			private final String productTitle;
			
			public ProductRegistrationTransaction(int productKey, String productTitle) {
				this.productKey = productKey;
				this.productTitle = productTitle;
			}

			@Override
			public CallableStatement initializeParameters(CallableStatement createPdtStatement) throws SQLException {
				createPdtStatement.setInt(1, productKey);
				createPdtStatement.setString(2, productTitle);
				return createPdtStatement;
			}

			@Override
			public String getParametersCSV() {
				return ("PdtKey: " + productKey
						+ " PdtTitle: " + productTitle);
				
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_PRODUCT;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception{
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("createProduct", 2));
				
			}
		}
		
		TransactionSQL transaction = new ProductRegistrationTransaction(productkey, productTitle);
		return transaction;
	}

	public Transaction createVendor(int vendorKey, String vendorTitle) throws Exception {
		
		class VendorRegistrationSQLTransaction extends TransactionSQL {

			private final int vendorKey;
			private final String vendorTitle;

			public VendorRegistrationSQLTransaction(int vendorKey, String vendorTitle) {
				synchronized (this) {
					this.vendorKey = vendorKey;
					this.vendorTitle = vendorTitle;
				}
			}	
			
			@Override
			public CallableStatement initializeParameters(CallableStatement createVendor) throws SQLException {
				createVendor.setInt(1, vendorKey);
				createVendor.setString(2, vendorTitle);
				
				return createVendor;
			}
			
			@Override
			public String getParametersCSV() {
				return ("VendorKey: "+ vendorKey
						+ "Vendor: "+vendorTitle);
				
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_VENDOR;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception {
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("createVendor", 2));
			}
		}

		TransactionSQL transaction = new VendorRegistrationSQLTransaction(vendorKey, 
				vendorTitle);
		
		return transaction;
	}

	@Override
	public Transaction findInventoryDaysOfSupply(int vendorKey, 
			int productKey) throws Exception {
		
		class CalculateInventoryDaysOfSupplySQL extends TransactionSQL {

			private final int vendorKey;
			private final int productKey;
			
			public CalculateInventoryDaysOfSupplySQL(int vendorKey, 
					int productKey ) {
				
				this.vendorKey = vendorKey;
				this.productKey = productKey;
			}
			
			@Override
			public CallableStatement initializeParameters(CallableStatement calcDaysSupplyStatement) throws SQLException {
				calcDaysSupplyStatement.setInt(1, vendorKey);
				calcDaysSupplyStatement.setInt(2, productKey);
				return calcDaysSupplyStatement;
			}

			@Override
			public String getParametersCSV() {
				return ("VendorKey: "+ vendorKey
						+ "PdtKey: "+ productKey);
			}

			@Override
			public Operation getOperationName() {
				return Operation.STAT_DAYS_OF_SUPPLY;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception {
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("findInventoryDaysOfSupply", 2));
				
			}
		}
		
		TransactionSQL transaction = new CalculateInventoryDaysOfSupplySQL(vendorKey, 
				productKey);
		
		return transaction;
	}

	@Override
	public Transaction findBullwhipCoefficient() throws Exception {
		
		class CalculateBullwhipCoeffSQL extends TransactionSQL {
			
			@Override
			public CallableStatement initializeParameters(
					CallableStatement calcBullwhipCoeffStatement) throws SQLException {
				
				return calcBullwhipCoeffStatement;
			}

			@Override
			public String getParametersCSV() {
				return ("");
			}

			@Override
			public Operation getOperationName() {
				return Operation.STAT_BULLWHIP_COEFFICIENT;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception{
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("findBullwhipCoefficient", 0));
				
			}
		}
		
		TransactionSQL transaction = new CalculateBullwhipCoeffSQL();
		return transaction;
	}

	@Override
	public Transaction clearState() throws Exception {
		
		class ClearStateSQL extends TransactionSQL {

			@Override
			public CallableStatement initializeParameters(CallableStatement clearSQLStatement) throws SQLException {
				return clearSQLStatement;
			}

			@Override
			public String getParametersCSV() {
				return "";
			}

			@Override
			public Operation getOperationName() {
				return Operation.CLEAR_STATE;
			}

			@Override
			public CallableStatement getCallableStatement() throws Exception{
				return SQLConnection.getInstance().getConnection().prepareCall(
						SQLConnection.getPreparedCall("cleanState", 0));
				
			}
		}
		
		TransactionSQL transaction = new ClearStateSQL();
		return transaction;
	}
}