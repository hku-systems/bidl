FROM openjdk:8u181-jdk
# FROM openjdk:11
COPY . /home
WORKDIR /home

#install and source ansible
RUN apt-get update && apt-get install -y \
    iptables && apt-get install -y ant && \
    apt-get install -y net-tools

RUN ant clean
RUN ant

RUN chmod +x ./runscripts/*

