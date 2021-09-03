#!/bin/bash

exp=$1

if [ $exp == "performance" ]; then 
    bash run-ff.sh performance 
    bash run-fabric.sh 
    bash run-streamchain.sh 
    # TODO: bidl
    python3 figure/performance.py

elif [ $exp == "contention" ]; then 
    bash run-ff.sh contention 

    python3 figure/contention.py

elif [ $exp == "nd" ]; then 
    bash run-ff.sh nd

    python3 figure/nondeterminism.py

elif [ $exp == "breakdown" ]; then 
    bash run-ff.sh breakdown endorse
    bash run-ff.sh breakdown commit

elif [ $exp == "malicious" ]; then 
    echo "TODO"

fi