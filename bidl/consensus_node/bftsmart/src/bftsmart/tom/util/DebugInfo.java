/**
Copyright (c) 2007-2013 Alysson Bessani, Eduardo Alchieri, Paulo Sousa, and the authors indicated in the @author tags

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
package bftsmart.tom.util;

import bftsmart.tom.core.messages.TOMMessage;

/**
 *
 * @author Joao Sousa
 */
public class DebugInfo {

    public final int cid;
    public final int epoch;
    public final int leader;
    public final TOMMessage msg;

    public DebugInfo(int cid, int epoch, int leader, TOMMessage msg) {
        this.cid = cid;
        this.epoch = epoch;
        this.leader = leader;
        this.msg = msg;
    }
}
