# ===== Build stage =====
FROM ubuntu:26.04 AS build

# Set noninteractive frontend
ENV DEBIAN_FRONTEND=noninteractive

# Build arguments
ARG GIT_REPOSITORY=https://github.com/GloryOfNight/udp-relay.git
ARG GIT_BRANCH=main

# Install build dependencies in one RUN to reduce layers
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        clang \
        clang-tools \
        cmake \
        ninja-build \
        git

# Clone and build
RUN cd /opt && \
    git clone --branch ${GIT_BRANCH:-main} --depth 1 $GIT_REPOSITORY udp-relay && \
    cd udp-relay && \
    cmake --preset ninja-multi -DENABLE_BUILD_TEST=OFF && \
    cmake --build --preset ninja-release

# ===== Runtime stage =====
FROM ubuntu:26.04

# Expose UDP port
EXPOSE 6060/udp

# Copy built binary
COPY --from=build /opt/udp-relay/build/Release/udp-relay /opt/bin/udp-relay
COPY --from=build /opt/udp-relay/build/Release/udp-relay-healthcheck /opt/bin/udp-relay-healthcheck

RUN chmod +x /opt/bin/udp-relay
RUN chmod +x /opt/bin/udp-relay-healthcheck

WORKDIR /opt/udp-relay

HEALTHCHECK --interval=60s --timeout=10s --retries=1 CMD ./udp-relay-healthcheck

ENTRYPOINT ["/opt/bin/udp-relay"]
