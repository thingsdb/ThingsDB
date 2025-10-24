FROM amd64/alpine
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

FROM google/cloud-sdk:alpine
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
# WebSocket connections
EXPOSE 9270
# Status (HTTP) connections
EXPOSE 8080

ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_WS_PORT=9270
# ENV THINGSDB_WS_CERT_FILE=<replace-with-cert-file-path>
# ENV THINGSDB_WS_KEY_FILE=<replace-with-private-key-file-path>
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_STORAGE_PATH=/data

# Configure THINGSDB_GCLOUD_KEY_FILE to enable Google Cloud Backup support
# For example:
#
#  ENV THINGSDB_GCLOUD_KEY_FILE=service_account.json

ENTRYPOINT ["/usr/local/bin/thingsdb"]

