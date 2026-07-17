FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    qt6-base-dev \
    qt6-base-private-dev \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY protocol/ ./protocol/
COPY Server/ ./Server/

WORKDIR /app/Server
RUN mkdir build && cd build && qmake6 ../Server.pro && make

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libqt6core6 \
    libqt6network6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/Server/build/Server .

EXPOSE 5000
CMD ["./Server"]