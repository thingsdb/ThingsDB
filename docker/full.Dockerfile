FROM google/cloud-sdk:458.0.1
WORKDIR /tmp/thingsdb
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./main.c ./main.c
COPY ./src/ ./src/
COPY ./inc/ ./inc/
COPY ./libwebsockets/ ./libwebsockets/
RUN apt-get update && apt-get install -y \
        build-essential \
        cmake \
        libuv1-dev \
        libpcre2-dev \
        libyajl-dev \
        libcurl4-nss-dev && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make

FROM google/cloud-sdk:458.0.1

RUN mkdir -p /var/lib/thingsdb && \
    apt-get update && apt-get install -y \
    libuv1 \
    libpcre2-8-0 \
    libyajl2 \
    libcurl3-nss && \
    pip3 install py-timod

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

# Environment variable
ENV PYTHONUNBUFFERED=1
ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_WS_PORT=9270
# ENV THINGSDB_WS_CERT_FILE=<replace-with-cert-file-path>
# ENV THINGSDB_WS_KEY_FILE=<replace-with-private-key-file-path>
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_PYTHON_INTERPRETER=python3
ENV THINGSDB_STORAGE_PATH=/data

ENTRYPOINT ["/usr/local/bin/thingsdb"]

