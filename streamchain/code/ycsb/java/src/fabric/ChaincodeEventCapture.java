package fabric;

import org.hyperledger.fabric.sdk.BlockEvent;
import org.hyperledger.fabric.sdk.ChaincodeEvent;

public class ChaincodeEventCapture { 
	  private final String handle;
	  private final BlockEvent blockEvent;
	  private final ChaincodeEvent chaincodeEvent;

	  public ChaincodeEventCapture(String handle, BlockEvent blockEvent,
	      ChaincodeEvent chaincodeEvent) {
	    this.handle = handle;
	    this.blockEvent = blockEvent;
	    this.chaincodeEvent = chaincodeEvent;
	  }

	  /**
	   * @return the handle
	   */
	  public String getHandle() {
	    return handle;
	  }

	  /**
	   * @return the blockEvent
	   */
	  public BlockEvent getBlockEvent() {
	    return blockEvent;
	  }

	  /**
	   * @return the chaincodeEvent
	   */
	  public ChaincodeEvent getChaincodeEvent() {
	    return chaincodeEvent;
	  }
	}