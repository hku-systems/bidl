package main

//Version 2.0
import (
	"fmt"
	"strconv"
	"time"

	"encoding/json"
	"strings"

	"github.com/hyperledger/fabric/core/chaincode/shim"
	"github.com/hyperledger/fabric/protos/peer"
)

// SimpleAsset implements a simple chaincode to manage an asset
type SimpleAsset struct {
}

// Init is called during chaincode instantiation to initialize any
// data. Note that chaincode upgrade also calls this function to reset
// or to migrate data, so be careful to avoid a scenario where you
// inadvertently clobber your ledger's data!
func (t *SimpleAsset) Init(stub shim.ChaincodeStubInterface) peer.Response {
	return shim.Success(nil)
}

func (t *SimpleAsset) Invoke(stub shim.ChaincodeStubInterface) peer.Response {

	startTime := time.Now()

	fn, args := stub.GetFunctionAndParameters()

	var result peer.Response

	switch fn {
	case "read":
		result = read(stub, args)
	case "delete":
		result = delete(stub, args)
	case "insert":
		result = insert(stub, args)
	case "update":
		result = update(stub, args)
	case "deleteAll":
		result = deleteAllKeys(stub, args)
	default:
		result = scan(stub, args)
	}

	fmt.Printf("%d,%f\n", time.Now().UnixNano()/1000000, time.Since(startTime).Seconds()*1000)

	return result
}

func scan(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 3 {
		return shim.Error("Incorrect number of arguments. Expecting 2 keys and fields")
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

func read(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 1 {
		return shim.Error("Incorrect number of arguments. Expecting a key.")
	}
	bytes, err := stub.GetState(args[0])

	if err != nil {
		return shim.Error("Failed read")
	}

	if bytes == nil {
		return shim.Error("Keys not found")
	}

	return shim.Success(bytes)
}

func insert(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting key and values")
	}

	if err := stub.PutState(args[0], []byte(strings.Join(args[1:], ","))); err != nil {
		return shim.Error("Failed insert")
	}

	return shim.Success([]byte("Insert Done"))
}

// Not used in benchmarks
func update(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting key and fields")
	}
	bytes, err := stub.GetState(args[0])

	if err != nil {
		return shim.Error("Failed read")
	}

	dic := make(map[string]string)
	if bytes != nil {
		json.Unmarshal(bytes, &dic)
	}
	for _, v := range args[1:] {
		aux := strings.SplitN(v, ":", 2)
		dic[aux[0]] = aux[1]
	}
	bytes, _ = json.Marshal(dic)
	stub.PutState(args[0], bytes)
	return shim.Success([]byte("Done"))
}

// Not used in benchmarks
func delete(stub shim.ChaincodeStubInterface, args []string) peer.Response {
	if len(args) != 1 {
		return shim.Error("Incorrect number of arguments. Expecting key")
	}
	if err := stub.DelState(args[0]); err != nil {
		return shim.Error("Error")
	}
	return shim.Success([]byte("Done"))
}

// Not used in benchmarks
func deleteAllKeys(stub shim.ChaincodeStubInterface, args []string) peer.Response {

	iterator, err := stub.GetStateByRange("", "")
	if err != nil {
		return shim.Error("Error")
	}
	next, err := iterator.Next()
	for err == nil {
		key := next.GetKey()
		stub.DelState(key)
		next, err = iterator.Next()
	}

	return shim.Success([]byte("Done"))
}

// main function starts up the chaincode in the container during instantiate
func main() {
	if err := shim.Start(new(SimpleAsset)); err != nil {
		fmt.Printf("Error starting chaincode: %s", err)
	}
}
