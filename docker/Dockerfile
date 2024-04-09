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

FROM amd64/alpine:latest
RUN apk update && \
    apk add pcre2 libuv yajl curl tzdata && \
    mkdir -p /var/lib/thingsdb
COPY --from=0 /tmp/thingsdb/thingsdb /usr/local/bin/

# Volume mounts
VOLUME ["/data"]
VOLUME ["/modules"]

# Client (Socket) connections
EXPOSE 9200
# Client (HTTP) connections
EXPOSE 9210
# Node connections
EXPOSE 9220
# Status (HTTP) connections
EXPOSE 8080

ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_STORAGE_PATH=/data

ENTRYPOINT ["/usr/local/bin/thingsdb"]
