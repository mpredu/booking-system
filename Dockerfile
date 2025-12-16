# Multi-stage build for lock-free booking system
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .

# Build
RUN mkdir -p build && cd build && \
    cmake .. && \
    cmake --build . && \
    ctest --output-on-failure

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled binaries
# COPY main binary
COPY --from=builder /app/build/booking_lockfree /usr/local/bin/

# COPY test binaries as well
COPY --from=builder /app/build/test_lockfree /usr/local/bin/
COPY --from=builder /app/build/test_scalability /usr/local/bin/
COPY --from=builder /app/build/test_overbooking /usr/local/bin/
COPY --from=builder /app/build/test_two_thread_race /usr/local/bin/


WORKDIR /app

CMD ["booking_lockfree"]
