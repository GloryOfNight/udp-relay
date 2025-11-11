FROM ubuntu:24.04 as build

ARG GIT_REPOSITORY=https://github.com/GloryOfNight/udp-relay.git
ARG GIT_BRANCH=main

RUN set -ex && \
    apt-get update && \
    apt-get install -y clang && \
    apt-get install -y clang-tools && \
    apt-get install -y ninja-build && \
    apt-get install -y cmake && \
    apt-get install -y git

RUN cd /opt && git clone $GIT_REPOSITORY -b ${GIT_BRANCH} --depth 1 && cd udp-relay && cmake --preset ninja-multi && cmake --build --preset ninja-release

FROM ubuntu:24.04

EXPOSE 6060/udp

RUN apt update

RUN mkdir /opt/udp-relay
COPY --from=build /opt/udp-relay/build/Release /opt/udp-relay/udp-relay

WORKDIR /opt/udp-relay

ENTRYPOINT ["/opt/udp-relay/udp-relay"]
