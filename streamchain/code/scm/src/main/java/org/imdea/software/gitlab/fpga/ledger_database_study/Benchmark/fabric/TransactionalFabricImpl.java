package org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.fabric;

import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Operation;
import org.imdea.software.gitlab.fpga.ledger_database_study.Benchmark.common.Transaction;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.Transactional;
import org.imdea.software.gitlab.fpga.ledger_database_study.common.util.TimeUtils;

public class TransactionalFabricImpl implements Transactional{

	@Override
	public Transaction clearState() throws Exception {
		return null;
	}

	@Override
	public Transaction placeOrder(long orderKey, int supplierKey, int buyerKey, int productKey, int orderQuantity,
			int orderDate, int orderMonth, int orderYear, int orderHour, int orderMin, int orderSec) throws Exception {

		class PlaceOrder extends TransactionFabric{

			public PlaceOrder(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.PLACE_ORDER;
			}
		}
		
		return new PlaceOrder("placeorder", Integer.toString(productKey), Integer.toString(buyerKey), Integer.toString(supplierKey),
				TimeUtils.getFabricDateTimeString(orderDate, orderMonth, orderYear, orderHour, orderMin, orderSec),
				Long.toString(orderKey), Integer.toString(orderQuantity));
		
	}

	@Override
	public Transaction createContract(int supplierKey, int buyerKey, int productKey, int orderLimit, int date, int year,
			int month) throws Exception {
		
		class AddContract extends TransactionFabric {

			public AddContract(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_CONTRACT;
			}
			
		}
		
		return new AddContract("addcontract", Integer.toString(productKey),Integer.toString(buyerKey),
				Integer.toString(supplierKey),TimeUtils.getFabricDateTimeString(date, month, year, 0, 0, 0),
				Integer.toString(orderLimit));
		
	}

	@Override
	public Transaction updateInventory(int productKey, int vendorKey, int quantity, int date, int month, int year,
			int hours, int minutes, int seconds) throws Exception {
		
		class UpdateInventory extends TransactionFabric {

			public UpdateInventory(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.UPDATE_INVENTORY;
			}
		}
		
		return new UpdateInventory("addinventory", TimeUtils.getFabricDateTimeString(date, month, year, hours, minutes, seconds),
				Integer.toString(productKey), Integer.toString(vendorKey),Integer.toString(quantity));
		
	}

	@Override
	public Transaction createProduct(int productkey, String productTitle) throws Exception {
		class CreateProduct extends TransactionFabric{

			public CreateProduct(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_PRODUCT;
			}
		}
		return new CreateProduct("addproduct", Integer.toString(productkey), productTitle);
	}

	@Override
	public Transaction createVendor(int vendorKey, String vendorTitle) throws Exception {
		class CreateVendor extends TransactionFabric{

			public CreateVendor(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.ADD_VENDOR;
			}
		}
		
		return new CreateVendor("addvendor", Integer.toString(vendorKey),vendorTitle);
	}

	@Override
	public Transaction findInventoryDaysOfSupply(int vendorKey, int productKey) throws Exception {
		class CalculateInvDaysOfSupply extends TransactionFabric {

			public CalculateInvDaysOfSupply(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.STAT_DAYS_OF_SUPPLY;
			}
		}
		return new CalculateInvDaysOfSupply("calcinvdaysofsupply", Integer.toString(vendorKey),
				Integer.toString(productKey));
		
	}

	@Override
	public Transaction findBullwhipCoefficient() throws Exception {
		class CalculateBullwhipCoefficient extends TransactionFabric {

			public CalculateBullwhipCoefficient(String functionName, String ...arguments) {
				super(functionName, arguments);
			}

			@Override
			public Operation getOperationName() {
				return Operation.STAT_BULLWHIP_COEFFICIENT;
			}
		}
		return new CalculateBullwhipCoefficient("calcbullwhipcoefficient");
	}

}
