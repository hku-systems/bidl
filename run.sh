#!/bin/bash

exp=$1

if [ $exp == "performance" ]; then 
    bash run-ff.sh performance 
    bash run-fabric.sh 
    bash run-streamchain.sh 
    # TODO: bidl

elif [ $exp == "contention" ]; then 
    bash run-ff.sh contention 

elif [ $exp == "nd" ]; then 
    bash run-ff.sh nd

elif [ $exp == "breakdown" ]; then 
    bash run-ff.sh breakdown endorse
    bash run-ff.sh breakdown commit

elif [ $exp == "malicious" ]; then 

fi