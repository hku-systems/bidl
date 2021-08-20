#!/bin/bash

docker container prune -f
docker volume prune -f
# docker rmi $(docker images | grep bank | awk '{print $3}')