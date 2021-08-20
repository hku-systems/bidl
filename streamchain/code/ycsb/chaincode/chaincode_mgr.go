package main

//Version 2.0
import (
	"errors"
	"fmt"
	"math"
	"strconv"
	"time"

	"encoding/json"
	"strings"

	"github.com/hyperledger/fabric/core/chaincode/shim"
	"github.com/hyperledger/fabric/protos/peer"
)

//Example : expirydate-KbuyerId-->24 for {"expiryDate-buyerId": 24} in json equivalent
const valueStringKeySeparator string = "-K"
const valueStringKeyValueSeparator string = "-->"

const (
	dateTimeFormat = "2006-01-02%15:04:05"
	timeFormat     = "15:04:05"
	dateFormat     = "2006-01-02"
)

// SimpleAsset implements a simple chaincode to manage an asset
type Product struct {
	productId   int
	productName string
}

type Vendor struct {
	vendorId   int
	vendorName string
}

type Contract struct {
	productId       string
	buyerId         string
	supplierId      string
	expiryDate      string
	orderLimit      int
	isBuyerContract bool
}
type SimpleAsset struct {
}

type orderDateQuantityTuple struct {
	orderDate     string
	orderQuantity int
}

// Init is called during chaincode instantiation to initialize any
// data. Note that chaincode upgrade also calls this function to reset
// or to migrate data, so be careful to avoid a scenario where you
// inadvertently clobber your ledger's data!
func (t *SimpleAsset) Init(stub shim.ChaincodeStubInterface) peer.Response {
	return shim.Success(nil)
}

func (t *SimpleAsset) Invoke(stub shim.ChaincodeStubInterface) peer.Response {

	//startTime := time.Now()

	fn, args := stub.GetFunctionAndParameters()

	var result peer.Response

	switch fn {
	case "addvendor":
		result = addVendor(stub, args)
	case "addproduct":
		result = addProduct(stub, args)
	case "addcontract":
		result = addContract(stub, args)
	case "addinventory":
		result = addInventory(stub, args)
	case "placeorder":
		result = placeOrder(stub, args)
	case "calcbullwhipcoefficient":
		result = calculateBullwhipCoefficient(stub, args)
	case "calcinvdaysofsupply":
		result = calculateInvDaysOfSupply(stub, args)
	case "read":
		result = read(stub, args)
	default:
		return shim.Error("no such function")
	}

	return result
}

func scan(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 3 {
		return shim.Error("Incorrect number of arguments. Expecting 2 keys and fields - New chaincode")
	}

	num, _ := strconv.Atoi(args[1])

	iterator, _, err := stub.GetStateByRangeWithPagination(args[0], "", int32(num), "")

	if err != nil {
		return shim.Error("Error")
	}

	list := make([]map[string]string, 0)
	next, err := iterator.Next()
	for err == nil {
		value := next.GetValue()
		dic := make(map[string]string)
		res := make(map[string]string)
		json.Unmarshal(value, &dic)
		if args[2] != "all" {
			for _, v := range args[2:] {
				res[v] = dic[v]
			}
		} else {
			res = dic
		}
		list = append(list, res)
		next, err = iterator.Next()
	}

	bytes, _ := json.Marshal(list)

	return shim.Success(bytes)

}
func isExistingProduct(stub shim.ChaincodeStubInterface, productId string) bool {
	if productId == "" {
		return false
	}

	key, _ := stub.CreateCompositeKey("Product", []string{productId})
	bytes, _ := stub.GetState(key)

	if bytes == nil {
		return false
	}

	return true
}

func isExistingVendor(stub shim.ChaincodeStubInterface, vendorId string) bool {
	if vendorId == "" {
		return false
	}

	key, _ := stub.CreateCompositeKey("Vendor", []string{vendorId})
	bytes, _ := stub.GetState(key)

	if bytes == nil {
		return false
	}

	return true
}

func addProduct(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting productId and productName")
	}

	key, _ := stub.CreateCompositeKey("Product", []string{args[0]})

	if err := stub.PutState(key, []byte(args[1])); err != nil {
		return shim.Error("Failed to add product")
	}

	return shim.Success([]byte("Added product successfully"))
}

func addVendor(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting vendorId and vendorName")
	}

	key, _ := stub.CreateCompositeKey("Vendor", []string{args[0]})

	if err := stub.PutState(key, []byte(args[1])); err != nil {
		return shim.Error("Failed to add vendor")
	}

	return shim.Success([]byte("Added vendor successfully"))
}

// String productId = args.get(0);
// 		String buyerId = args.get(1);
// 		String supplierId = args.get(2);
// 		String expiryDate = args.get(3);
// 		String orderLimit = args.get(4);

// Contract:buyer:$productid:$buyerId:$supplierId:{
//    “$contractId:$expiryDate” : 	$orderLimit
//}
//Contract:supplier:$productid:$supplierId:$buyerId:{
//    “$contractId:$expiryDate” : 	$orderLimit
//}

func addContract(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 5 {
		return shim.Error("Incorrect number of arguments. Expecting productId, buyerId, supplierId, expiryDate, orderLimit")
	}

	//Buyer:product:buyer:supplier={expiryDate:quantity}
	buyerContractKey, _ := stub.CreateCompositeKey("Contract", []string{"buyer", args[0], args[1], args[2]})
	supplierContractKey, _ := stub.CreateCompositeKey("Contract", []string{"supplier", args[0], args[2], args[1]})
	_, error := strconv.Atoi(args[4])

	if error != nil {
		return shim.Error("Quantity must be numeric")
	}

	contractValue := args[3] + valueStringKeyValueSeparator + args[4]

	if err := stub.PutState(buyerContractKey, []byte(contractValue)); err != nil {
		return shim.Error("Failed to add buyer contract")
	}

	if err := stub.PutState(supplierContractKey, []byte(contractValue)); err != nil {
		return shim.Error("Failed to add supplier contract")
	}

	return shim.Success([]byte("Added contract successfully"))
}

// String inventoryDateTime = args.get(0);
// String productId = args.get(1);
// String vendorId = args.get(2);
// String quantity = args.get(3);

func addInventory(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 4 {
		return shim.Error("Incorrect number of arguments. Expecting InventoryDateTime, productId, vendorId, quantity")
	}

	//Inventory:vendorId:productId={InventoryDateTime:quantity}
	inventoryKey, _ := stub.CreateCompositeKey("Inventory", []string{args[2], args[1]})
	_, error := strconv.Atoi(args[3])

	if error != nil {
		return shim.Error("Quantity must be numeric")
	}

	inventoryValue := args[0] + valueStringKeyValueSeparator + args[3]

	if err := stub.PutState(inventoryKey, []byte(inventoryValue)); err != nil {
		return shim.Error("Failed to add Inventory")
	}

	return shim.Success([]byte("Added inventory successfully"))
}

// String productId = args.get(0);
// String buyerId = args.get(1);
// String supplierId = args.get(2);
// String orderDateTime = args.get(3);
// String orderId = args.get(4);
// String quantity = args.get(5)

//Order:$Fulfilled:$productId:$supplierId:$buyerId:$orderDate:$orderTime:$orderId
// = {"orderQuantity" : $quantity}
func placeOrder(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 6 {
		return shim.Error("Incorrect number of arguments. Expecting productId, buyerId, supplierId, orderDateTime, orderId, quantity")
	}

	var message = "Order executed successfully"
	var status = false

	var contractExpiryDateString string
	var orderLimit, orderQuantity int
	var e error
	var orderDateTime time.Time
	var contractExpiryDate time.Time

	orderDateTimeString := args[3]
	productId := args[0]
	buyerId := args[1]
	supplierId := args[2]
	orderId := args[4]
	orderQuantityStr := args[5]

	if orderDateTime, e = parseDateTime(orderDateTimeString); e != nil {
		return shim.Error("Invalid order date. Must of yyyy-dd-mm HH:MM:SS format")
	}

	if orderQuantity, e = strconv.Atoi(orderQuantityStr); e != nil {
		return shim.Error("Order quantity must be a number")
	}

	if contractExpiryDateString, orderLimit, e = getBuyerContract(stub, productId, buyerId, supplierId); e != nil {
		status = false
		message = "A valid contract doesn't exist"

		return commitOrder(stub, status, message, productId, supplierId, buyerId, orderDateTime, orderId, -1, -1, orderQuantity)
	}

	if contractExpiryDate, e = parseDateTime(contractExpiryDateString); e != nil {
		return shim.Error(e.Error())
	}

	isContractValid := isValidTimeWithinExpiry(orderDateTime, contractExpiryDate)

	if !isContractValid {
		status = false
		message = "Contract has expired"

		return commitOrder(stub, status, message, productId, supplierId, buyerId, orderDateTime, orderId, -1, -1, orderQuantity)
	}

	if orderQuantity > orderLimit {
		status = false
		message = "Order quantity exceeds order limit"

		return commitOrder(stub, status, message, productId, supplierId, buyerId, orderDateTime, orderId, -1, -1, orderQuantity)
	}

	var supplierInventoryQuantity, buyerInventoryQuantity int

	if supplierInventoryQuantity, e = getInventoryQuantity(stub, supplierId, productId); e != nil {
		return shim.Error("Invalid supplier inventory quantity - not a number")
	}

	if buyerInventoryQuantity, e = getInventoryQuantity(stub, buyerId, productId); e != nil {
		return shim.Error("Invalid buyer inventory quantity - not a number")
	}

	if orderQuantity > supplierInventoryQuantity {
		status = false
		message = "Order failed because seller inventory is less than the order quantity"
	}

	return commitOrder(stub, true, message, productId, supplierId, buyerId, orderDateTime,
		orderId, buyerInventoryQuantity, supplierInventoryQuantity, orderQuantity)

}

// String vendorId = args.get(0);
//String productId = args.get(1);
func calculateInvDaysOfSupply(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting vendorId, productId")
	}

	var inventoryQuantity = 0
	var e error

	vendorId := args[0]
	productId := args[1]

	if inventoryQuantity, e = getInventoryQuantity(stub, vendorId, productId); e != nil {
		return shim.Error(e.Error())
	}

	dailyOrderSumMap := make(map[string]int)
	var ordersDateAndQuantityAsSeller []orderDateQuantityTuple
	if ordersDateAndQuantityAsSeller, e = getOrderDateQuantityAsSeller(stub, vendorId, productId); e != nil {
		return shim.Error(e.Error())
	}

	for _, element := range ordersDateAndQuantityAsSeller {
		date := element.orderDate
		orderQuantity := element.orderQuantity

		sumQuantity := dailyOrderSumMap[date] + orderQuantity
		dailyOrderSumMap[date] = sumQuantity
	}

	dailyOrderValues := make([]int, 0, len(dailyOrderSumMap))
	for _, val := range dailyOrderSumMap {
		dailyOrderValues = append(dailyOrderValues, val)
	}

	var avgDailySales float64
	if avgDailySales, e = mean(dailyOrderValues); e != nil {
		return shim.Success([]byte("0"))
	}

	if avgDailySales == 0 {
		return shim.Success([]byte("0"))
	}

	return shim.Success([]byte(strconv.FormatFloat((float64(inventoryQuantity) / float64(avgDailySales)), 'f', 4, 64)))
}

func calculateBullwhipCoefficient(stub shim.ChaincodeStubInterface, args []string) peer.Response {

	var orders shim.StateQueryIteratorInterface
	var e error
	if orders, e = getOrderResultsForPartialKey(stub, "Order", []string{strconv.FormatBool(true)}); e != nil {
		return shim.Error("Unable to retrieve orders to calculate Bullwhip coefficient")
	}

	//[productId][vendor] = [Cout] 24
	//[productId][vendor] = [Cin] 12
	pdtVendorAggregateMap := make(map[string]map[string]float64)
	//[productId][vendorId] = [date] (sumOrderquantity as a buyer)
	pdtVendorDailyGeneratedOrderAggregateMap := make(map[string]map[string]int)
	//[productId][vendorId] = [date] (sumOrderquantity as a seller)
	pdtVendorDailyReceivedOrderAggregateMap := make(map[string]map[string]int)

	for orders.HasNext() {
		orderKeyValue, _ := orders.Next()
		_, orderKeys, _ := stub.SplitCompositeKey(orderKeyValue.GetKey())
		productId := orderKeys[1]
		supplierId := orderKeys[2]
		buyerId := orderKeys[3]
		orderDate := orderKeys[4]

		orderVal := orderKeyValue.GetValue()
		orderQuantityString := strings.Split(string(orderVal), valueStringKeyValueSeparator)
		orderQuantity, _ := strconv.Atoi(orderQuantityString[1])

		//Generates [date]= quantity for {productId + vendor as buyer (buyerId)}
		dailyOrdersGenerated := pdtVendorDailyGeneratedOrderAggregateMap[productId+buyerId]
		if dailyOrdersGenerated == nil {
			dailyOrdersGenerated = make(map[string]int)
		}

		dailyOrdersGenerated[orderDate] = dailyOrdersGenerated[orderDate] + orderQuantity
		pdtVendorDailyGeneratedOrderAggregateMap[productId+buyerId] = dailyOrdersGenerated

		dailyOrdersReceived := pdtVendorDailyReceivedOrderAggregateMap[productId+supplierId]
		if dailyOrdersReceived == nil {
			dailyOrdersReceived = make(map[string]int)
		}
		dailyOrdersReceived[orderDate] = dailyOrdersReceived[orderDate] + orderQuantity
		pdtVendorDailyReceivedOrderAggregateMap[productId+supplierId] = dailyOrdersReceived
	}

	for key, val := range pdtVendorDailyGeneratedOrderAggregateMap {
		//key has pdt + vendorId
		//value has date and sum of orders

		dailyOrderValues := make([]int, 0, len(val))
		for _, quantitySum := range val {
			dailyOrderValues = append(dailyOrderValues, quantitySum)
		}

		var cout float64
		stdDevDailyOrder, eStd := stddev(dailyOrderValues)
		avgDailyOrder, eAvg := mean(dailyOrderValues)

		if eAvg != nil || eStd != nil || avgDailyOrder == 0 || stdDevDailyOrder == 0 {
			cout = 0
		} else {
			cout = stdDevDailyOrder / avgDailyOrder
		}

		pdtVendorAggregateMap[key] = map[string]float64{
			"cout": cout,
		}
	}

	for key, val := range pdtVendorDailyReceivedOrderAggregateMap {
		//key has pdt + vendorId
		//value has date and sum of orders received

		dailyOrderValues := make([]int, 0, len(val))
		for _, quantitySum := range val {
			dailyOrderValues = append(dailyOrderValues, quantitySum)
		}

		var cin float64
		stdDevDailyOrder, eStd := stddev(dailyOrderValues)
		avgDailyOrder, eAvg := mean(dailyOrderValues)

		if eAvg != nil || eStd != nil || avgDailyOrder == 0 || stdDevDailyOrder == 0 {
			cin = 0
		} else {
			cin = stdDevDailyOrder / avgDailyOrder
		}

		tempMap := pdtVendorAggregateMap[key]
		if tempMap == nil {
			tempMap = make(map[string]float64)
		}
		tempMap["cin"] = cin
		pdtVendorAggregateMap[key] = tempMap

	}
	//finalAggPdtVendorMap : [productId + vendorId] = Cout/Cin
	var finalAggPdtVendorMap = make(map[string]float64)
	//Iterate through product vendor map, calculate ratio of count/cin
	for key, val := range pdtVendorAggregateMap {
		//val = {"cout" : 24.24, "cin" : 244.123} etc
		cout := val["cout"]
		cin := val["cin"]

		var ratio float64 = 0

		if !(cin == 0) {
			ratio = cout / cin
		}

		finalAggPdtVendorMap[key] = ratio
	}

	//Find avg of all (Cout/Cin) values across product and vendors

	var sumOfRatios float64 = 0
	for _, ratio := range finalAggPdtVendorMap {
		sumOfRatios += ratio
	}
	var result float64 = 0
	if len(finalAggPdtVendorMap) > 0 {
		result = sumOfRatios / float64(len(finalAggPdtVendorMap))
	}

	return shim.Success([]byte(strconv.FormatFloat(result, 'f', 4, 64)))
}

//Order:$Fulfilled:$productId:$supplierId:$buyerId:$orderDate:$orderTime:$orderId
// = {"orderQuantity" : $quantity}

func commitOrder(stub shim.ChaincodeStubInterface, status bool, message string, productId string, supplierId string,
	buyerId string, orderDateTime time.Time, orderId string, retrievedBuyerInventoryQuantity int, retrievedSupplierInventoryQuantity int,
	orderQuantity int) peer.Response {

	orderDate, orderTime := splitDateTimeToDateAndTime(orderDateTime)
	orderKey, _ := stub.CreateCompositeKey("Order", []string{strconv.FormatBool(status), productId, supplierId, buyerId, orderDate, orderTime, orderId})
	orderValue := "orderQuantity" + valueStringKeyValueSeparator + strconv.Itoa(orderQuantity)

	stub.PutState(orderKey, []byte(orderValue))

	if !status {
		return shim.Success([]byte("Order failed - " + message))
	}

	//Inventory:vendorId:productId={InventoryDateTime:quantity}

	supplierInventoryKey, _ := stub.CreateCompositeKey("Inventory", []string{supplierId, productId})
	buyerInventoryKey, _ := stub.CreateCompositeKey("Inventory", []string{buyerId, productId})
	inventoryDateTime := formatDateTime(orderDateTime)
	supplierInventoryValue := inventoryDateTime + valueStringKeyValueSeparator + strconv.Itoa(retrievedSupplierInventoryQuantity-orderQuantity)
	buyerInventoryValue := inventoryDateTime + valueStringKeyValueSeparator + strconv.Itoa(retrievedBuyerInventoryQuantity+orderQuantity)

	stub.PutState(supplierInventoryKey, []byte(supplierInventoryValue))
	stub.PutState(buyerInventoryKey, []byte(buyerInventoryValue))

	return shim.Success([]byte(message))
}

func read(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 1 {
		return shim.Error("Incorrect number of arguments. Expecting key and fields")
	}

	if args[0] == "contract" {
		bytes, err := stub.GetStateByPartialCompositeKey("Contract", []string{})
		if err != nil {
			return shim.Error("Failed read")
		}

		if bytes == nil {
			return shim.Error("Keys not found")
		}

		var result string

		for bytes.HasNext() {
			obj, _ := bytes.Next()
			result = result + obj.GetKey() + "===" + string(obj.GetValue())
		}

		return shim.Success([]byte(result))
	} else if args[0] == "inventory" {
		bytes, err := stub.GetStateByPartialCompositeKey("Inventory", []string{})
		if err != nil {
			return shim.Error("Failed read")
		}

		if bytes == nil {
			return shim.Error("Keys not found")
		}

		var result string

		for bytes.HasNext() {
			obj, _ := bytes.Next()
			result = result + obj.GetKey() + "===" + string(obj.GetValue())
		}

		return shim.Success([]byte(result))
	} else {
		key, _ := stub.CreateCompositeKey(args[0], []string{args[1]})
		bytes, err := stub.GetState(key)
		if err != nil {
			return shim.Error("Failed read")
		}

		if bytes == nil {
			return shim.Error("Keys not found")
		}

		return shim.Success(bytes)
	}
	key, _ := stub.CreateCompositeKey(args[0], []string{args[1]})
	bytes, _ := stub.GetState(key)
	fields := strings.Split(string(bytes), ",")

	dic := make(map[string]string)
	res := make(map[string]string)

	for _, r := range fields {
		tmp := strings.Split(r, ":")
		dic[tmp[0]] = tmp[1]
	}

	if args[1] != "all" {
		for _, v := range args[1:] {
			res[v] = dic[v]
		}
	} else {
		res = dic
	}

	bytes, _ = json.Marshal(res)

	return shim.Success(bytes)
}

func getOrderDateQuantityAsSeller(stub shim.ChaincodeStubInterface, vendorId string, productId string) ([]orderDateQuantityTuple, error) {

	result := []orderDateQuantityTuple{}
	keys := []string{strconv.FormatBool(true), productId, vendorId}
	objectType := "Order"

	var queryResult shim.StateQueryIteratorInterface
	var e error

	if queryResult, e = getOrderResultsForPartialKey(stub, objectType, keys); e != nil {
		return result, e
	}

	for queryResult.HasNext() {
		orderKeyValue, _ := queryResult.Next()
		_, orderKeys, _ := stub.SplitCompositeKey(orderKeyValue.GetKey())
		orderDate := orderKeys[4]

		orderVal := orderKeyValue.GetValue()
		orderQuantityString := strings.Split(string(orderVal), valueStringKeyValueSeparator)
		orderQuantity, _ := strconv.Atoi(orderQuantityString[1])

		result = append(result, orderDateQuantityTuple{orderDate, orderQuantity})
	}

	return result, nil
}

//Order:$Fulfilled:$productId:$supplierId:$buyerId:$orderDate:$orderTime:$orderId
// = {"orderQuantity" : $quantity}

func getOrderResultsForPartialKey(stub shim.ChaincodeStubInterface, objectType string, keys []string) (shim.StateQueryIteratorInterface, error) {
	return stub.GetStateByPartialCompositeKey(objectType, keys)
}

func getInventoryQuantity(stub shim.ChaincodeStubInterface, vendorId string, productId string) (int, error) {
	//Inventory:vendorId:productId={InventoryDateTime:quantity}
	inventoryKey, _ := stub.CreateCompositeKey("Inventory", []string{vendorId, productId})
	bytes, _ := stub.GetState(inventoryKey)

	if bytes == nil {
		return -1, errors.New("Inventory not initialized for vendor")
	}

	inventoryValue := string(bytes)
	inventoryValueComponents := strings.Split(inventoryValue, valueStringKeyValueSeparator)

	if inventoryQuantity, e := strconv.Atoi(inventoryValueComponents[1]); e != nil {
		return -1, errors.New("Invalid inventory quantity - not a number")
	} else {
		return inventoryQuantity, nil
	}
}

func getBuyerContract(stub shim.ChaincodeStubInterface, productId string, buyerId string, supplierId string) (string, int, error) {
	// Contract:buyer:$productid:$buyerId:$supplierId:{
	//    “$contractId:$expiryDate” : 	$orderLimit
	//}
	buyerContractKey, _ := stub.CreateCompositeKey("Contract", []string{"buyer", productId, buyerId, supplierId})
	bytes, _ := stub.GetState(buyerContractKey)

	if bytes == nil {
		return "", -1, errors.New("No existing contract for buyer, product and supplier")
	}

	contractValue := string(bytes)
	contractValueComponents := strings.Split(contractValue, valueStringKeyValueSeparator)
	contractExpiry := contractValueComponents[0]
	contractOrderLimit, e := strconv.Atoi(contractValueComponents[1])

	if e != nil {
		return "", -1, errors.New("Invalid contract order limit - not a number")
	}

	return contractExpiry, contractOrderLimit, nil
}

func parseDateTime(datetimeString string) (time.Time, error) {
	timeObj, e := time.Parse(dateTimeFormat, datetimeString)
	if e != nil {
		return time.Time{}, e
	}
	return timeObj, nil
}

func Sum(input []int) (float64, error) {

	var sum int
	if len(input) == 0 {
		return math.NaN(), errors.New("Nothing to sum")
	}

	for _, n := range input {
		sum += n
	}

	return float64(sum), nil
}

func mean(input []int) (float64, error) {

	if len(input) == 0 {
		return math.NaN(), errors.New("No elements to compute average")
	}

	sum, _ := Sum(input)
	return sum / float64(len(input)), nil
}

func getVariance(input []int) (pvar float64, err error) {

	var variance float64

	if len(input) == 0 {
		return math.NaN(), errors.New("Variance cannot be calculated for an empty set of values")
	}

	m, _ := mean(input)

	for _, n := range input {
		eachNum := float64(n)
		variance += (eachNum - m) * (eachNum - m)
	}
	return variance / float64(len(input)), nil
}

func stddev(input []int) (float64, error) {
	if len(input) == 0 {
		return math.NaN(), errors.New("Stddev cannot be calculated for an empty set of values")
	}

	variance, _ := getVariance(input)
	return math.Pow(variance, 0.5), nil
}

func formatDateTime(dateTime time.Time) string {

	timeString := dateTime.Format(dateTimeFormat)
	return timeString
}

func isValidTimeWithinExpiry(orderTime time.Time, expiryDate time.Time) bool {
	return orderTime.Before(expiryDate)
}

func splitDateTimeToDateAndTime(dateTime time.Time) (string, string) {
	return dateTime.Format(dateFormat), dateTime.Format(timeFormat)
}

func main() {
	if err := shim.Start(new(SimpleAsset)); err != nil {
		fmt.Printf("Error starting chaincode: %s", err)
	}
}
