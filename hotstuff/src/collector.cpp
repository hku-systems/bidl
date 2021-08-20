#include "hotstuff/collector.h"

namespace collector {

Collector::Collector():
    config("collector.conf") {
    // setup TCP server


}

void Collector::on_receive_proposal(const Proposal &prop) {
    do_broadcast(prop);
}


} // end collector namespace