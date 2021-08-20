package fabric;

import static java.nio.charset.StandardCharsets.UTF_8;

import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.hyperledger.fabric.shim.ChaincodeBase;
import org.hyperledger.fabric.shim.ChaincodeStub;

import com.google.protobuf.ByteString;


public class ChaincodeDB extends ChaincodeBase {

    private static Log _logger = LogFactory.getLog(ChaincodeDB.class);
	@Override
	public Response init(ChaincodeStub stub) {
		try {
            _logger.info("Init java simple chaincode");
			String func = stub.getFunction();
			if (!func.equals("init")) {
				return newErrorResponse("function other than init is not supported");
			}
            _logger.info("Init success");
			return newSuccessResponse();
		} catch (Throwable e) {
            _logger.info("Init fail");
			return newErrorResponse(e);
		}
	}

	@Override
	public Response invoke(ChaincodeStub stub) {
		try {
            _logger.info("Invoke");
			String func = stub.getFunction();
			List<String> params = stub.getParameters();
			if (func.equals("read")) {
				return read(stub, params);
			}
			if (func.equals("delete")) {
				return delete(stub, params);
			}
			if (func.equals("insert") || func.equals("update")) {
				return insert(stub, params);
			}
			return newErrorResponse(
					"Invalid invoke function name. Expecting one of: [\"read\", \"delete\",\"scan\", \"insert\", \"insert\"]");
		} catch (Throwable e) {
			return newErrorResponse(e);
		}
	}

	private Response read(ChaincodeStub stub, List<String> args) {
		if (args.size() != 1) {
			return newErrorResponse("Incorrect number of arguments. Expecting key");
		}
		String key = args.get(0);
		// byte[] stateBytes
		String val = stub.getStringState(key);
		if (val == null) {
			return newErrorResponse(String.format("Error: state for %s is null", key));
		}
		return newSuccessResponse(val, ByteString.copyFrom(val, UTF_8).toByteArray());
	}

	private Response insert(ChaincodeStub stub, List<String> args) {
        if (args.size() != 2) {
            return newErrorResponse("Incorrect number of arguments. Expecting 2");
        }
        
        stub.putStringState(args.get(0), args.get(1));
		return newSuccessResponse();
	}

	private Response delete(ChaincodeStub stub, List<String> args) {
		if (args.size() != 1) {
			return newErrorResponse("Incorrect number of arguments. Expecting 1");
		}
		String key = args.get(0);
		// Delete the key from the state in ledger
		stub.delState(key);
		return newSuccessResponse();
	}

    public static void main(String[] args) {
        _logger.info("Main");
        new ChaincodeDB().start(args);
    }
}
