#!/bin/bash

exp=$1

if [ $exp == "performance" ]; then 
    bash run-ff.sh performance 
    bash run-fabric.sh 
    bash run-streamchain.sh 
    bash run-bidl.sh performance
    python3 figure/performance.py

elif [ $exp == "scalability" ]; then 
    bash run-bidl.sh scalability
    python3 figure/scalability.py

elif [ $exp == "contention" ]; then 
    bash run-ff.sh contention 
    bash run-bidl.sh contention
    python3 figure/contention.py

elif [ $exp == "nd" ]; then 
    bash run-ff.sh nd
    bash run-bidl.sh nd
    python3 figure/nondeterminism.py

elif [ $exp == "breakdown" ]; then 
    bash run-ff.sh breakdown endorse
    bash run-ff.sh breakdown commit

elif [ $exp == "malicious" ]; then 
    bash run-bidl.sh malicious
    python3 figure/malicious.py
fi