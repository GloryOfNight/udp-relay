# ===== Build stage =====
FROM ubuntu:25.10 AS build

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
    git clone --branch $GIT_BRANCH --depth 1 $GIT_REPOSITORY udp-relay && \
    cd udp-relay && \
    cmake --preset ninja-multi -DENABLE_BUILD_TEST=OFF && \
    cmake --build --preset ninja-release

# ===== Runtime stage =====
FROM ubuntu:25.10

# Expose UDP port
EXPOSE 6060/udp

# Copy built binary
COPY --from=build /opt/udp-relay/build/Release/udp-relay /opt/bin/udp-relay
COPY --from=build /opt/udp-relay/build/Release/udp-relay-healthchecker /opt/bin/udp-relay-healthchecker

RUN chmod +x /opt/bin/udp-relay
RUN chmod +x /opt/bin/udp-relay-healthchecker

WORKDIR /opt/udp-relay

HEALTHCHECK --interval=60s --timeout=10s --retries=1 CMD ./udp-relay-healthchecker

ENTRYPOINT ["/opt/bin/udp-relay"]
