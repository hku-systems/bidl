FROM hotstuff:base
COPY . /home/libhotstuff
WORKDIR /home/libhotstuff
# RUN git submodule update --init --recursive \
# RUN rm -rf CMakeCache.txt CMakeFiles && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON \
    #&& make -j

RUN bash docker/ssh.sh
WORKDIR /home/libhotstuff/scripts/deploy

ENTRYPOINT service ssh restart && bash

