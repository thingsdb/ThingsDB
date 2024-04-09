FROM amd64/alpine:3.19
WORKDIR /tmp/thingsdb
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./main.c ./main.c
COPY ./src/ ./src/
COPY ./inc/ ./inc/
COPY ./libwebsockets/ ./libwebsockets/
RUN apk update && \
    apk upgrade && \
    apk add gcc make cmake libuv-dev musl-dev pcre2-dev yajl-dev curl-dev util-linux-dev linux-headers && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make

FROM ghcr.io/cesbit/tlsproxy:v0.1.1

FROM amd64/alpine:latest
RUN apk update && \
    apk add pcre2 libuv yajl curl tzdata && \
    mkdir -p /var/lib/thingsdb
COPY --from=0 /tmp/thingsdb/thingsdb /usr/local/bin/
COPY --from=1 /tlsproxy /usr/local/bin/

# Volume mounts
VOLUME ["/data"]
VOLUME ["/modules"]
VOLUME ["/certificates"]

# Client (Socket TLS/SSL) connections
EXPOSE 9443
# Client (HTTPS) connections
EXPOSE 443
# Status (HTTP) connections
EXPOSE 8080

ENV TLSPROXY_TARGET=127.0.0.1
ENV TLSPROXY_PORTS=9443:9200,443:9210
ENV TLSPROXY_CERT_FILE=certificates/server.crt
ENV TLSPROXY_KEY_FILE=certificates/server.key

ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_LISTEN_CLIENT_PORT=9200
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_STORAGE_PATH=/data

ENTRYPOINT ["sh", "-c", "/usr/local/bin/tlsproxy & /usr/local/bin/thingsdb"]
