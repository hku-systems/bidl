DELIMITER //
CREATE PROCEDURE placeOrder
(IN orderKey Bigint, supplierKey int, buyerKey int, productKey int, orderQuantity int, orderDate datetime)
BEGIN
  DECLARE orderLMT int; 
  DECLARE contractID int;
  DECLARE contractExpiry date;
  
  DECLARE supplierQuantity int;
  DECLARE buyerQuantity int;
  
  DECLARE outputStatus int DEFAULT 1;
  DECLARE outputMessage varchar(100) DEFAULT 'Order Issued and executed';
  
  START TRANSACTION;
  SELECT Contract.orderLimit , Contract.contractExpiryDate into orderLMT, contractExpiry from Contract where supplierId = supplierKey and buyerId = buyerKey and productId = productKey order by contractExpiryDate desc limit 1;
  
  iF supplierKey < buyerKey THEN
	select quantity into supplierQuantity from Inventory where productId=productKey and vendorId=supplierKey for update;
	select quantity into buyerQuantity from Inventory where productId=productKey and vendorId=buyerKey for update;
  ELSE
	select quantity into buyerQuantity from Inventory where productId=productKey and vendorId=buyerKey for update;
    select quantity into supplierQuantity from Inventory where productId=productKey and vendorId=supplierKey for update;
  END IF;
  
  IF orderQuantity is null THEN
	select 0 into orderQuantity;
  END IF;
  
  IF supplierQuantity is null THEN
	select 0 into supplierQuantity;
  END IF;
  
  IF buyerQuantity is null THEN
	select 0 into buyerQuantity;
  END IF;
  
  IF orderLMT is null THEN
	SELECT 0, CONCAT('No valid contract available on ', orderDate, ' between supplier ', supplierkey, ' and buyer ', buyerKey, ' for product ', productKey) into outputStatus,outputMessage;
  ELSEIF DATE(orderDate) > contractExpiry THEN
	SELECT 0, CONCAT('Contract has expired on ',contractExpiry, '. Cannot transact on ', orderDate, '.') into outputStatus,outputMessage;
  ELSEIF orderQuantity > supplierQuantity THEN
	SELECT 0, CONCAT('Order quantity of ', orderQuantity, ' exceeds seller inventory of ', supplierQuantity, '.') into outputStatus,outputMessage;
  ELSEIF orderQuantity > orderLMT THEN
	SELECT 0, CONCAT('order quantity ', orderQuantity, ' exceeds contract limit of ', orderLMT) into outputStatus,outputMessage;
  END IF;

  IF outputStatus = 1 THEN
	INSERT into `Order` values (orderKey,buyerKey, supplierKey, productKey, orderQuantity, OrderDate, true);
    update Inventory set quantity=(supplierQuantity - orderQuantity), inventoryDate=orderDate where productId=productKey and vendorId=supplierKey;
    update Inventory set quantity=(buyerQuantity + orderQuantity), inventoryDate=orderDate where productId=productKey and vendorId=buyerKey;
    COMMIT;
  ELSE
	INSERT into `Order`  values (orderKey, buyerKey, supplierKey, productKey, orderQuantity, OrderDate, false);
    COMMIT;
  END IF;
  
  SELECT outputStatus, outputMessage;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE createContract
(IN supplierKey int, buyerKey int, productKey int, orderLMT int, contractExpiry date)
BEGIN
	START TRANSACTION WITH CONSISTENT SNAPSHOT;
	INSERT into Contract values(supplierKey, buyerKey, productKey, orderLMT, contractExpiry);
    COMMIT;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE updateInventory
(IN productKey int, vendorKey int, qty int, inventoryTime dateTime)
BEGIN
	START TRANSACTION WITH CONSISTENT SNAPSHOT;
	INSERT into Inventory values(productKey, vendorKey, qty, inventoryTime);
    COMMIT;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE createProduct
(IN pdtKey int, pdtName varchar(100))
BEGIN
	INSERT into Product values(pdtKey, pdtName);
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE createVendor
(IN vendorKey int, vendorTitle varchar(200))
BEGIN
	INSERT into Vendor values(vendorKey, vendorTitle);
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE findInventoryDaysOfSupply
(IN vendorKey int, productKey int)
BEGIN
	DECLARE inventory, avgDailyUsage int;
    DECLARE outputStatus int DEFAULT 0;
	DECLARE outputMessage varchar(100) DEFAULT 'Error';
    DECLARE ratio double DEFAULT 0;
    select quantity into inventory from Inventory where productId=productKey and vendorId=vendorKey;
	select avg(dailyTable.dailyOrder) into avgDailyUsage  from (select sellerId,productId, Date(orderDate) as date,sum(orderQuantity) as dailyOrder from `Order` where orderFulfilled = true and productId = productKey and sellerId = vendorKey group by date) as dailyTable group by dailyTable.sellerId,dailyTable.productId;
    IF avgDailyUsage  <> 0 THEN
		select (inventory/avgDailyUsage) into ratio;
	END IF;
    select 1, ratio into outputStatus,outputMessage;
    select outputStatus,outputMessage;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE findBullwhipCoefficient()
BEGIN
	DECLARE outputStatus int DEFAULT 0;
	DECLARE outputMessage varchar(100) DEFAULT 'Error';
    DECLARE ratio double DEFAULT 0;
    select avg(Cout/Cin) into ratio from (select buyerId,productId, IF(avg(dailyTable.dailyOrder)=0 or avg(dailyTable.dailyOrder) is null, 0, std(dailyTable.dailyOrder)/avg(dailyTable.dailyOrder)) as Cout from (select buyerId,productId, Date(orderDate) as date,sum(orderQuantity) as dailyOrder from `Order` where orderFulfilled = true group by date,buyerId,productId) as dailyTable group by dailyTable.buyerId,dailyTable.productId) as demandGenerated join ( select sellerId,productId,IF(avg(dailyTable.dailyOrder)=0 or avg(dailyTable.dailyOrder) is null, 0, std(dailyTable.dailyOrder)/avg(dailyTable.dailyOrder)) as Cin from (select sellerId,productId, Date(orderDate) as date,sum(orderQuantity) as dailyOrder from `Order` where orderFulfilled = true group by date,sellerId,productId) as dailyTable group by dailyTable.sellerId,dailyTable.productId) as demandReceived on demandGenerated.buyerId = demandReceived.sellerId and demandGenerated.productId = demandReceived.ProductId;
    select 1, ratio into outputStatus,outputMessage;
    select outputStatus,outputMessage;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE cleanState()
BEGIN
	SET FOREIGN_KEY_CHECKS = 0;
	truncate table Inventory;
	truncate table Contract;
	truncate table `Order`;
	truncate table Product;
	truncate table Vendor;
	SET FOREIGN_KEY_CHECKS = 1;
END //
DELIMITER ;

