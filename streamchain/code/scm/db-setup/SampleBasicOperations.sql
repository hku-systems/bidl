CALL `SupplyChain-SQL-V1`.`createVendor`(1, 'BMW');
select * from Vendor;

CALL `SupplyChain-SQL-V1`.`createVendor`(2, 'Supplier');
select * from Vendor;

CALL `SupplyChain-SQL-V1`.`createVendor`(3, 'Retailer');
select * from Vendor;

CALL `SupplyChain-SQL-V1`.`createProduct`(1, 'BMW X4');
select * from Product;

CALL `SupplyChain-SQL-V1`.`createProduct`(2, 'BMW M4 coupe');
select * from Product;

CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 1, 50, curtime());
CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 1, 40, TIMESTAMPADD(MINUTE,1,curtime()));
select * from inventory;

CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 2, 20, curtime());
CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 2, 30, TIMESTAMPADD(MINUTE,1,curtime()));
select * from inventory;

CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 3, 20, curtime());
CALL `SupplyChain-SQL-V1`.`updateInventory`(1, 3, 20, TIMESTAMPADD(MINUTE,1,curtime()));
select * from inventory;

CALL `SupplyChain-SQL-V1`.`createContract`(1, 1, 2, 1, 10, DATE_ADD(curdate(), INTERVAL 3 DAY));
CALL `SupplyChain-SQL-V1`.`createContract`(2, 2, 3, 1, 10, DATE_ADD(curdate(), INTERVAL 3 DAY));
select * from Contract;