#include <cstdio>
#include <thread>
#include <atomic>

#include "salticidae/event.h"
#include "salticidae/util.h"

using salticidae::TimerEvent;
using salticidae::Config;

void masksigs() {
	sigset_t mask;
	sigemptyset(&mask);
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

void test_mpsc(int nproducers, int nops, size_t burst_size, bool test_rewind) {
    size_t total = nproducers * nops;
    salticidae::EventContext ec;
    std::atomic<size_t> collected(0);
    using queue_t = salticidae::MPSCQueueEventDriven<int>;
    queue_t q;
    q.set_capacity(65536);
    q.reg_handler(ec, [&collected, burst_size, test_rewind](queue_t &q) {
        size_t cnt = burst_size;
        int x;
        while (q.try_dequeue(x))
        {
            if (test_rewind && (rand() & 1))
                q.rewind(x);
            else
            {
                collected.fetch_add(1);
                printf("%d\n", x);
            }
            if (!--cnt) return true;
        }
        return false;
    });
    std::vector<std::thread> producers;
    std::thread consumer([&collected, total, &ec]() {
        masksigs();
        TimerEvent timer(ec, [&ec, &collected, total](TimerEvent &timer) {
            if (collected.load() == total) ec.stop();
            timer.add(1);
        });
        timer.add(1);
        ec.dispatch();
    });

    for (int i = 0; i < nproducers; i++)
    {
        producers.emplace(producers.end(), std::thread([&q, nops, i, nproducers]() {
            masksigs();
            int x = i;
            for (int j = 0; j < nops; j++)
            {
                //usleep(rand() / double(RAND_MAX) * 100);
                while (!q.enqueue(x, false))
                    std::this_thread::yield();
                x += nproducers;
            }
        }));
    }
    for (auto &t: producers) t.join();
    SALTICIDAE_LOG_INFO("producers terminate");
    consumer.join();
    SALTICIDAE_LOG_INFO("consumers terminate");
}

void test_mpmc(int nproducers, int nconsumers, int nops, size_t burst_size) {
    size_t total = nproducers * nops;
    using queue_t = salticidae::MPMCQueueEventDriven<int>;
    queue_t q;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<salticidae::EventContext> ecs;
    std::atomic<size_t> collected(0);
    ecs.resize(nconsumers);
    q.set_capacity(65536);
    for (int i = 0; i < nconsumers; i++)
    {
        q.reg_handler(ecs[i], [&collected, burst_size](queue_t &q) {
            size_t cnt = burst_size;
            int x;
            while (q.try_dequeue(x))
            {
                //usleep(10);
                printf("%d\n", x);
                collected.fetch_add(1);
                if (!--cnt) return true;
            }
            return false;
        });
    }
    for (int i = 0; i < nconsumers; i++)
    {
        consumers.emplace(consumers.end(), std::thread(
                [&collected, total, &ec = ecs[i]]() {
            masksigs();
            TimerEvent timer(ec, [&ec, &collected, total](TimerEvent &timer) {
                if (collected.load() == total) ec.stop();
                timer.add(1);
            });
            timer.add(1);
            ec.dispatch();
        }));
    }
    for (int i = 0; i < nproducers; i++)
    {
        producers.emplace(producers.end(), std::thread([&q, nops, i, nproducers]() {
            masksigs();
            int x = i;
            for (int j = 0; j < nops; j++)
            {
                //usleep(rand() / double(RAND_MAX) * 100);
                while (!q.enqueue(x, false))
                    std::this_thread::yield();
                x += nproducers;
            }
        }));
    }
    for (auto &t: producers) t.join();
    SALTICIDAE_LOG_INFO("producers terminate");
    for (auto &t: consumers) t.join();
    SALTICIDAE_LOG_INFO("consumers terminate");
}

int main(int argc, char **argv) {
    Config config;
    auto opt_nproducers = Config::OptValInt::create(16);
    auto opt_nconsumers = Config::OptValInt::create(4);
    auto opt_burst_size = Config::OptValInt::create(128);
    auto opt_nops = Config::OptValInt::create(100000);
    auto opt_mpmc = Config::OptValFlag::create(false);
    auto opt_help = Config::OptValFlag::create(false);
    auto opt_rewind = Config::OptValFlag::create(false);
    config.add_opt("nproducers", opt_nproducers, Config::SET_VAL);
    config.add_opt("nconsumers", opt_nconsumers, Config::SET_VAL);
    config.add_opt("burst-size", opt_burst_size, Config::SET_VAL);
    config.add_opt("nops", opt_nops, Config::SET_VAL);
    config.add_opt("mpmc", opt_mpmc, Config::SWITCH_ON);
    config.add_opt("rewind", opt_rewind, Config::SWITCH_ON);
    config.add_opt("help", opt_help, Config::SWITCH_ON, 'h', "show this help info");
    config.parse(argc, argv);
    srand(time(0));
    if (opt_help->get())
    {
        config.print_help();
        exit(0);
    }

    if (!opt_mpmc->get())
    {
        SALTICIDAE_LOG_INFO("testing an MPSC queue...");
        test_mpsc(opt_nproducers->get(), opt_nops->get(),
                opt_burst_size->get(), opt_rewind->get());
    }
    else
    {
        SALTICIDAE_LOG_INFO("testing an MPMC queue...");
        test_mpmc(opt_nproducers->get(), opt_nconsumers->get(),
                opt_nops->get(), opt_burst_size->get());
    }
    return 0;
}
