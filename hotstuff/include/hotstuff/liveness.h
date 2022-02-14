/**
 * Copyright 2018 VMware
 * Copyright 2018 Ted Yin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _HOTSTUFF_LIVENESS_H
#define _HOTSTUFF_LIVENESS_H

#include "salticidae/util.h"
#include "hotstuff/hotstuff.h"

namespace hotstuff {

using salticidae::_1;
using salticidae::_2;

/** Abstraction for liveness gadget (oracle). */
class PaceMaker {
    protected:
    HotStuffCore *hsc;
    public:
    virtual ~PaceMaker() = default;

    bool is_using_udp_multi;
    TimerEvent timer_recv_prop;
    double recv_timeout;
    uint64_t count;

    /** Initialize the PaceMaker. A derived class should also call the default implementation to set `hsc`. */
    virtual void init(HotStuffCore *_hsc) { 
        hsc = _hsc;
    }

    /** Get a promise resolved when the pacemaker thinks it is a *good* time to issue new commands.
     *  When promise is resolved, the replica should propose the command. */
    virtual promise_t beat() = 0;

    /** Get the current proposer. */
    virtual ReplicaID get_proposer() = 0;

    /** Select the parent blocks for a new block.
     *  @return Parent blocks. The block at index 0 is the direct parent,
     *  while the others are uncles/aunts. The returned vector should be non-empty. */
    virtual std::vector<block_t> get_parents() = 0;
    
    /** Get a promise resolved when the pacemaker thinks it is a *good* time
     *  to vote for a block. The promise is resolved with the next proposer's ID
     */
    virtual promise_t beat_resp(ReplicaID last_proposer) = 0;
    
    /** Impeach the current proposer. */
    virtual void impeach() {}
    virtual void on_consensus(const block_t &) {}
    virtual size_t get_pending_size() = 0;
};

using pacemaker_bt = BoxObj<PaceMaker>;

/** Parent selection implementation for PaceMaker
 *  select the highest tail that follows the current hqc block. */
class PMHighTail: public virtual PaceMaker {
    block_t hqc_tail;
    const int32_t parent_limit;         /**< maximum number of parents */
    EventContext ec;

    bool check_ancestry(const block_t &_a, const block_t &_b) {
        block_t b;

        for (
            b = _b;
            b->get_height() > _a->get_height();
            b = b->get_parents()[0]
        );

        return b == _a;
    }
    
    void reg_hqc_update() {
        // when highQC is updated, resolve(hqc.block)
        hsc->async_hqc_update().then([this](const block_t &hqc) {
            hqc_tail = hqc;
            for (const auto &tail: hsc->get_tails())
                if (check_ancestry(hqc, tail) && tail->get_height() > hqc_tail->get_height())
                    hqc_tail = tail;
            reg_hqc_update();
        });
    }

    void reg_proposal() {
        // when a new block is proposed, then set hqc_tail
        hsc->async_wait_proposal().then([this](const Proposal &prop) {
            hqc_tail = prop.blk;
            reg_proposal();
        });
    }

    void reg_receive_proposal() {
        // when a proposal is received, then set hqc_tail
        hsc->async_wait_receive_proposal().then([this](const Proposal &prop) {
            if (is_using_udp_multi) {
                timer_recv_prop.del();
                HOTSTUFF_LOG_INFO("Pacemaker : STOP RECEIVE Timer %d", count++);

                if (count % 4 != 1) {
                    timer_recv_prop.add(recv_timeout);
                    HOTSTUFF_LOG_INFO("Pacemaker : QC LOOP - Start RECEIVE Timer %d", count);
                }
            }

            const auto &hqc = hsc->get_hqc();

            const auto &blk = prop.blk;

            if (check_ancestry(hqc, blk) && blk->get_height() > hqc_tail->get_height())
                hqc_tail = blk;

            reg_receive_proposal();
        });
    }

    public:
    PMHighTail(EventContext ec, int32_t parent_limit): ec(ec), parent_limit(parent_limit) {}

    void init() {
        hqc_tail = hsc->get_genesis();
        reg_hqc_update();
        reg_proposal();
        reg_receive_proposal();
        
    }

    std::vector<block_t> get_parents() override {
        const auto &tails = hsc->get_tails();
        std::vector<block_t> parents{hqc_tail};
        return parents;
    }
};

/** Beat implementation for PaceMaker: simply wait for the QC of last proposed
 * block.  PaceMakers derived from this class will beat only when the last
 * block proposed by itself gets its QC. */
class PMWaitQC: public virtual PaceMaker {
    std::queue<promise_t> pending_beats;
    block_t last_proposed;
    bool locked;
    promise_t pm_qc_finish;
    promise_t pm_wait_propose;

    protected:
    void schedule_next() {
        if (!pending_beats.empty() && !locked) {
            auto pm = pending_beats.front();
            pending_beats.pop();

            pm_qc_finish.reject();

            (pm_qc_finish = hsc->async_qc_finish(last_proposed)).then([this, pm]() {
                pm.resolve(get_proposer());
            });

            locked = true;
        }
    }

    void update_last_proposed() {
        pm_wait_propose.reject();
        (pm_wait_propose = hsc->async_wait_proposal()).then(
                [this](const Proposal &prop) {
            last_proposed = prop.blk;
            locked = false;
            schedule_next();
            update_last_proposed();
        });
    }

    public:

    size_t get_pending_size() override { return pending_beats.size(); }

    void init() {
        last_proposed = hsc->get_genesis();
        locked = false;
        update_last_proposed();
    }

    ReplicaID get_proposer() override {
        return hsc->get_id();
    }

    promise_t beat() override {
        promise_t pm;
        pending_beats.push(pm);
        schedule_next();
        return pm;
    }

    promise_t beat_resp(ReplicaID last_proposer) override {
        return promise_t([last_proposer](promise_t &pm) {
            pm.resolve(last_proposer);
        });
    }
};

/** Naive PaceMaker where everyone can be a proposer at any moment. */
struct PaceMakerDummy: public PMHighTail, public PMWaitQC {
    PaceMakerDummy(EventContext ec, int32_t parent_limit): PMHighTail(ec, parent_limit), PMWaitQC() 
    {}

    void init(HotStuffCore *hsc) override {
        PaceMaker::init(hsc);
        PMHighTail::init();
        PMWaitQC::init();
    }
};

/** PaceMakerDummy with a fixed proposer. */
class PaceMakerDummyFixed: public PaceMakerDummy {
    ReplicaID proposer;

    public:
    PaceMakerDummyFixed(EventContext ec, ReplicaID proposer, int32_t parent_limit):
        PaceMakerDummy(ec, parent_limit),
        proposer(proposer) {}

    ReplicaID get_proposer() override {
        return proposer;
    }

    promise_t beat_resp(ReplicaID) override {
        return promise_t([this](promise_t &pm) {
            pm.resolve(proposer);
        });
    }
};

/**
 * Simple long-standing round-robin style proposer liveness gadget.
 */
class PMRoundRobinProposer: virtual public PaceMaker {
    double base_timeout;
    double exp_timeout;
    double prop_delay;
    EventContext ec;

    TimerEvent timer;  // QC timer or randomized timeout

    /** the proposer it believes */
    ReplicaID proposer;
    std::unordered_map<ReplicaID, block_t> prop_blk; // contains the block proposed by Leader
    bool rotating;

    /* extra state needed for a proposer */
    std::queue<promise_t> pending_beats;
    block_t last_proposed;
    bool locked;
    promise_t pm_qc_finish; // use reject() to reset the promise
    promise_t pm_wait_propose;
    promise_t pm_qc_manual;

    void reg_proposal() {
        // when a new block is proposed, add it to prop_blk
        hsc->async_wait_proposal().then([this](const Proposal &prop) {
            auto &pblk = prop_blk[hsc->get_id()];
            if (!pblk) pblk = prop.blk;

            if (rotating) reg_proposal();
        });
    }

    void reg_receive_proposal() {
        // when a proposal is received, add it to prop_blk
        hsc->async_wait_receive_proposal().then([this](const Proposal &prop) {
            auto &pblk = prop_blk[prop.proposer];
            if (!pblk) pblk = prop.blk;

            if (rotating) reg_receive_proposal();
        });
    }

    // resolved with proposer when last proposed block get a QC
    void proposer_schedule_next() {
        if (!pending_beats.empty() && !locked) {
            auto pm = pending_beats.front(); // pm : promise
            pending_beats.pop();

            pm_qc_finish.reject();

            // when the last proposed block get a QC
            (pm_qc_finish = hsc->async_qc_finish(last_proposed)).then([this, pm]() {
                HOTSTUFF_LOG_PROTO("got QC, propose a new block");
                pm.resolve(proposer);
            });

            locked = true;
        }
    }

    void proposer_update_last_proposed() {
        pm_wait_propose.reject();
        // when a new block is proposed
        (pm_wait_propose = hsc->async_wait_proposal()).then([this](const Proposal &prop) {
            last_proposed = prop.blk;
            locked = false;
            proposer_schedule_next();
            proposer_update_last_proposed();
        });
    }

    void do_new_consensus(int x, const std::vector<uint256_t> &cmds) {
#ifdef HOTSTUFF_CMD_REQSIZE    
        int n = cmds.size();
        int length_of_propose = n * HOTSTUFF_CMD_REQSIZE;
        auto blk = hsc->on_propose(cmds, get_parents(), bytearray_t(length_of_propose), count);
#else
        auto blk = hsc->on_propose(cmds, get_parents(), bytearray_t(), count);
#endif

        pm_qc_manual.reject();
        (pm_qc_manual = hsc->async_qc_finish(blk)).then([this, x, n]() {
#ifdef HOTSTUFF_TWO_STEP
            if (x >= 2) return;
#else
            if (x >= 3) return;
#endif
            HOTSTUFF_LOG_PROTO("Pacemaker: got QC for block %d, cmd_size = %d", x, n);
            do_new_consensus(x + 1, std::vector<uint256_t>{});
        });
    }

    void on_exp_timeout(TimerEvent &) {
        if (is_using_udp_multi) {
            timer_recv_prop.add(recv_timeout);
            HOTSTUFF_LOG_INFO("Pacemaker : ROOT - START RECEIVE Timer %d", count);
        }

        if (proposer == hsc->get_id()) {
            do_new_consensus(0, std::vector<uint256_t>{});
        }

        timer = TimerEvent(ec, [this](TimerEvent &){ 
            HOTSTUFF_LOG_PROTO("Pacemaker : TIMEOUT! : Rotate!");
            rotate(); 
        });
        timer.add(prop_delay); // tolerate 1s, will stop timer at on_consensus stop_rotate()

    }

    /* role transitions */
    void rotate() {
        // add proposal.block to prop_blk according to my role : when Leader send / Replica recv
        reg_proposal();
        reg_receive_proposal(); 
        prop_blk.clear();

        rotating = true;
        proposer = (proposer + 1) % hsc->get_config().nreplicas;

        pm_qc_finish.reject();
        pm_wait_propose.reject();
        pm_qc_manual.reject();

        // Leader broadcast proposal after exp_timeout
        timer = TimerEvent(ec, salticidae::generic_bind(&PMRoundRobinProposer::on_exp_timeout, this, _1));
        timer.add(exp_timeout); // default 1s

        HOTSTUFF_LOG_INFO("Pacemaker : rotate to %d", proposer);

        exp_timeout *= 2;
    }

    void stop_rotate() {
        timer.del();

        pm_qc_finish.reject();
        pm_wait_propose.reject();
        pm_qc_manual.reject();

        rotating = false;
        locked = false;

        last_proposed = hsc->get_genesis();

        proposer_update_last_proposed(); // ???

        auto hs = static_cast<hotstuff::HotStuffBase *>(hsc);

        // Replica may already receive reproposed proposal before starting the timer
        int saved_count = count; 

        if (proposer == hsc->get_id()) { // if I am the proposer
            hs->do_elected(); // ???

            // if there is still a cmd not yet decided, run a new consensus
            hs->get_tcall().async_call([this, hs](salticidae::ThreadCall::Handle &) {
                auto &pending = hs->get_decision_waiting(); // insert at cmd_pending handler, erase at do_decide()

                if (!pending.size()) return;

                std::vector<uint256_t> cmds;

                for (auto &p: pending)
                    cmds.push_back(p.first);

                if(is_using_udp_multi) {
                    timer_recv_prop.add(recv_timeout);
                    HOTSTUFF_LOG_INFO("Pacemaker : PENDING DECISION & REPROPOSE - START RECEIVE Timer %d", count);
                }

                do_new_consensus(0, cmds);
            });
        }
        else {
            hs->get_tcall().async_call([this, hs, saved_count](salticidae::ThreadCall::Handle &) {
                auto &pending = hs->get_decision_waiting(); // insert at cmd_pending handler, erase at do_decide()

                if (!pending.size() || saved_count != count) return;

                if(is_using_udp_multi) {
                    timer_recv_prop.add(recv_timeout);
                    HOTSTUFF_LOG_INFO("Pacemaker : PENDING DECISION & REPROPOSE - START RECEIVE Timer %d", count);
                }

            });
        }
    }

    protected:
    void on_consensus(const block_t &blk) override {
        timer.del();
        HOTSTUFF_LOG_INFO("Pacemaker : on consensus");

        exp_timeout = base_timeout; // expired timeout
        if (prop_blk[proposer] == blk)
            stop_rotate();
    }

    void impeach() override {
        if (rotating) return;
        rotate();
        HOTSTUFF_LOG_INFO("schedule to impeach the proposer");
    }

    public:
    PMRoundRobinProposer(const EventContext &ec, double base_timeout, double prop_delay):
        base_timeout(base_timeout),
        prop_delay(prop_delay),
        ec(ec), proposer(0), rotating(false) {}

    size_t get_pending_size() override { return pending_beats.size(); }

    void init() {
        exp_timeout = base_timeout; // default init = 1s
        stop_rotate();
    }

    ReplicaID get_proposer() override {
        return proposer;
    }

    promise_t beat() override {
        if (!rotating && proposer == hsc->get_id()) { // if Leader
            promise_t pm;
            pending_beats.push(pm);

            proposer_schedule_next();

            return pm;
        }
        else {
            return promise_t([proposer=proposer](promise_t &pm) {
                pm.resolve(proposer);
            });
        }
    }

    promise_t beat_resp(ReplicaID last_proposer) override {
        return promise_t([last_proposer](promise_t &pm) {
            pm.resolve(last_proposer);
        });
    }
};

struct PaceMakerRR: public PMHighTail, public PMRoundRobinProposer {
    PaceMakerRR(EventContext ec, int32_t parent_limit, double base_timeout = 1, double prop_delay = 1):
        PMHighTail(ec, parent_limit),
        PMRoundRobinProposer(ec, base_timeout, prop_delay) 
    {
        is_using_udp_multi = true;

        if (is_using_udp_multi) {
            timer_recv_prop = TimerEvent(ec, [this](TimerEvent &){ 
                ReplicaID proposer = get_proposer();
                ReplicaID requester = hsc->get_id();
                RetransRequest request = RetransRequest(requester, count, hsc);

                HOTSTUFF_LOG_PROTO("Pacemaker : TIMEOUT! : sending %s to Proposer %d", std::string(request).c_str(), proposer);

                if (proposer != requester)
                    hsc->on_request(proposer, request);
            });;

            recv_timeout = 3.0 / 1000;

            count = 1;
        }
    }

    void init(HotStuffCore *hsc) override {
        PaceMaker::init(hsc);
        PMHighTail::init();
        PMRoundRobinProposer::init();
    }
};

}

#endif
