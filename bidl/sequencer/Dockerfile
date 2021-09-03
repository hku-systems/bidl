FROM frolvlad/alpine-gxx
WORKDIR /sequencer
COPY . /sequencer

RUN rm -f ./sequencer
RUN g++ -std=c++11 sequencer_multicast.cpp -o sequencer
