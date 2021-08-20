#ifndef _HOTSTUFF_HOTSTUFFCOLLECTOR_H
#define _HOTSTUFF_HOTSTUFFCOLLECTOR_H

#include "hotstuff/config.h"
#include "hotstuff/collector.h"
#include "hotstuff/consensus.h"

namespace collector {

using hotstuff::Proposal;

class HotStuffCollector: public Collector {
public:
    HotStuffCollector();


private:
    void do_broadcast(const Proposal &prop) override;

private:


};


} // end collector namespace


#endif
