#!/bin/bash
ssh hkucs@202.45.128.162 "sudo pkill -f netserver; sudo netserver"
ssh hkucs@202.45.128.162 "sudo netserver"
netperf -H 10.22.1.3
ssh hkucs@202.45.128.162 "sudo pkill -f netserver"

ssh hkucs@202.45.128.163 "sudo pkill -f netserver; sudo netserver"
ssh hkucs@202.45.128.163 "sudo netserver"
netperf -H 10.22.1.4
ssh hkucs@202.45.128.163 "sudo pkill -f netserver"

ssh hkucs@202.45.128.164 "sudo pkill -f netserver; sudo netserver"
ssh hkucs@202.45.128.164 "sudo netserver"
netperf -H 10.22.1.14
ssh hkucs@202.45.128.164 "sudo pkill -f netserver"

ssh hkucs@202.45.128.174 "sudo pkill -f netserver; sudo netserver"
ssh hkucs@202.45.128.174 "sudo netserver"
netperf -H 10.22.1.15
ssh hkucs@202.45.128.174 "sudo pkill -f netserver"