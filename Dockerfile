FROM ubuntu:24.04 as build

ARG GIT_REPOSITORY=https://github.com/GloryOfNight/udp-relay.git
ARG GIT_BRANCH=main

RUN set -ex && \
    apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y clang && \
    apt-get install -y cmake && \
    apt-get install -y git

RUN cd /opt && git clone $GIT_REPOSITORY -b ${GIT_BRANCH} && cd udp-relay && cmake --preset linux64-release && cmake --build build

FROM ubuntu:24.04

EXPOSE 6060/udp

RUN apt update

RUN mkdir /opt/udp-relay
COPY --from=build /opt/udp-relay/build/udp-relay /opt/udp-relay/udp-relay

WORKDIR /opt/udp-relay

ENTRYPOINT ["/opt/udp-relay/udp-relay"]
