FROM amd64/alpine:3.13
COPY ./ /tmp/thingsdb/
RUN apk update && \
    apk upgrade && \
    apk add gcc make libuv-dev musl-dev pcre2-dev yajl-dev curl-dev util-linux-dev linux-headers git && \
    git clone https://github.com/cesbit/libcleri.git /tmp/libcleri && \
    cd /tmp/libcleri/Release && \
    make all && \
    make install && \
    cd /tmp/thingsdb/Release && \
    make clean && \
    make

FROM amd64/alpine:latest
RUN apk update && \
    apk add pcre2 libuv yajl curl && \
    mkdir -p /var/lib/thingsdb
COPY --from=0 /tmp/thingsdb/Release/thingsdb /usr/local/bin/
COPY --from=0 /usr/lib/libcleri* /usr/lib/

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
