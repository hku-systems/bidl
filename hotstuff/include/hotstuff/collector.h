#ifndef _HOTSTUFF_COLLECTOR_H
#define _HOTSTUFF_COLLECTOR_H

#include "hotstuff/config.h"
#include "hotstuff/consensus.h"

namespace collector {

using hotstuff::Proposal;

class Collector {
public:
    Collector();
    virtual ~Collector();

    void on_receive_proposal(const Proposal &prop);
private:
    
protected:
    ConfigReader config;

    // network implementation
    virtual void do_broadcast(const Proposal &prop) = 0;

};


} // end collector namespace


#endif
