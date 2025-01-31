FROM arm64v8/debian:bookworm-slim
WORKDIR /tmp/thingsdb

# Copy source files
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./main.c ./main.c
COPY ./src/ ./src/
COPY ./inc/ ./inc/
COPY ./libwebsockets/ ./libwebsockets/

# Install build dependencies (adjust as needed)
RUN apt-get update && \
    apt-get install -y build-essential cmake libuv1-dev libpcre2-dev libyajl-dev libcurl4-openssl-dev libssl-dev tzdata && \
    export OPENSSL_ROOT_DIR="/usr" && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make

# Final image
FROM arm64v8/debian:bookworm-slim
RUN apt-get update && \
    apt-get install -y libpcre2-8-0 libuv1 libyajl2 libcurl4 tzdata && \
    mkdir -p /var/lib/thingsdb

# Copy the built binary
COPY --from=0 /tmp/thingsdb/thingsdb /usr/local/bin/

# Volume mounts
VOLUME ["/data"]
VOLUME ["/modules"]

# Ports
EXPOSE 9200
EXPOSE 9210
EXPOSE 9220
EXPOSE 9270
EXPOSE 8080

# Environment variables
ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_WS_PORT=9270
# ENV THINGSDB_WS_CERT_FILE=<replace-with-cert-file-path>
# ENV THINGSDB_WS_KEY_FILE=<replace-with-private-key-file-path>
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_STORAGE_PATH=/data

# Entrypoint
ENTRYPOINT ["/usr/local/bin/thingsdb"]