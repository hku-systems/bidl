#!/bin/bash

dst=backup_logs/log_$(date | tr " " "_" | tr ":" "_")
mkdir -p $dst

mv logs $dst