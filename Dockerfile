FROM debian:trixie-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake ninja-build libssl-dev ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DTESTGRAPH_BUILD_TESTS=OFF && cmake --build /build --parallel 2

FROM debian:trixie-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends libssl3t64 git curl ca-certificates && rm -rf /var/lib/apt/lists/*
RUN useradd -r -m -d /var/lib/testgraph testgraph
COPY --from=build /build/testgraph /usr/local/bin/testgraph
COPY --from=build /build/testgraph-server /usr/local/bin/testgraph-server
COPY --from=build /build/testgraph-worker /usr/local/bin/testgraph-worker
USER testgraph
WORKDIR /var/lib/testgraph
EXPOSE 8080
ENTRYPOINT ["testgraph-server"]
