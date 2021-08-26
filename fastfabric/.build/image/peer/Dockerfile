# Copyright Greg Haskins All Rights Reserved
#
# SPDX-License-Identifier: Apache-2.0
#
FROM hyperledger/fabric-baseos:amd64-0.4.13
ENV FABRIC_CFG_PATH /etc/hyperledger/fabric
RUN mkdir -p /var/hyperledger/production $FABRIC_CFG_PATH
COPY payload/peer /usr/local/bin
ADD  payload/sampleconfig.tar.bz2 $FABRIC_CFG_PATH
CMD ["peer","node","start"]
LABEL org.hyperledger.fabric.version=1.3.0 \
      org.hyperledger.fabric.base.version=0.4.13
