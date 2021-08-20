package fabric;

import java.util.EnumSet;
import java.util.Vector;
import java.util.regex.Pattern;

import org.hyperledger.fabric.sdk.BlockEvent;
import org.hyperledger.fabric.sdk.ChaincodeEvent;
import org.hyperledger.fabric.sdk.ChaincodeEventListener;
import org.hyperledger.fabric.sdk.Channel;
import org.hyperledger.fabric.sdk.Peer;
import org.hyperledger.fabric.sdk.exception.InvalidArgumentException;

import com.google.protobuf.InvalidProtocolBufferException;

public class SdkIntegration {
	public static String setChaincodeEventListener(Channel channel, String expectedEventName,
			Vector<ChaincodeEventCapture> chaincodeEvents) throws InvalidArgumentException {

		ChaincodeEventListener chaincodeEventListener = new ChaincodeEventListener() {

			@Override
			public void received(String handle, BlockEvent blockEvent, ChaincodeEvent chaincodeEvent) {
				chaincodeEvents.add(new ChaincodeEventCapture(handle, blockEvent, chaincodeEvent));
			}
		};
		// chaincode events.
		String eventListenerHandle = channel.registerChaincodeEventListener(Pattern.compile(".*"),
				Pattern.compile(Pattern.quote(expectedEventName)), chaincodeEventListener);
		return eventListenerHandle;
	}

	public static boolean waitForChaincodeEvent(Integer timeout, Channel channel,
			Vector<ChaincodeEventCapture> chaincodeEvents, String chaincodeEventListenerHandle)
			throws InvalidArgumentException {
		boolean eventDone = false;
		if (chaincodeEventListenerHandle != null) {
			int numberEventsExpected = channel.getEventHubs().size()
					+ channel.getPeers(EnumSet.of(Peer.PeerRole.EVENT_SOURCE)).size();
			if (timeout.equals(0)) {
				while (chaincodeEvents.size() != numberEventsExpected) {
					// do nothing
				}
				eventDone = true;
			} else { // get event with timer
				for (int i = 0; i < timeout; i++) {
					if (chaincodeEvents.size() == numberEventsExpected) {
						eventDone = true;
						break;
					} else {
						try {
							double j = i;
							j = j / 10;
							Thread.sleep(100); // wait for the events for one tenth of second.
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
					}
				}
			}

			// unregister event listener
			channel.unregisterChaincodeEventListener(chaincodeEventListenerHandle);
		}
		return eventDone;
	}

}
