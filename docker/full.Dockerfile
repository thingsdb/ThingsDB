FROM google/cloud-sdk:353.0.0
COPY ./ /tmp/thingsdb/

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    libuv1-dev \
    libpcre2-dev \
    libyajl-dev \
    libcurl4-nss-dev && \
    git clone https://github.com/cesbit/libcleri.git /tmp/libcleri && \
    cd /tmp/libcleri/Release && \
    make all && \
    make install && \
    cd /tmp/thingsdb/Release && \
    make clean && \
    make

FROM google/cloud-sdk:353.0.0

RUN mkdir -p /var/lib/thingsdb && \
    apt-get update && apt-get install -y \
    libuv1 \
    libpcre2-8-0 \
    libyajl2 \
    libcurl3-nss && \
    pip3 install py-timod

COPY --from=0 /tmp/thingsdb/Release/thingsdb /usr/local/bin/
COPY --from=0 /usr/lib/libcleri* /usr/lib/

# Client (Socket) connections
EXPOSE 9200
# Client (HTTP) connections
EXPOSE 9210
# Node connections
EXPOSE 9220
# Status (HTTP) connections
EXPOSE 8080

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

# Environment variable
ENV PYTHONUNBUFFERED=1
ENV THINGSDB_BIND_CLIENT_ADDR=0.0.0.0
ENV THINGSDB_BIND_NODE_ADDR=0.0.0.0
ENV THINGSDB_HTTP_API_PORT=9210
ENV THINGSDB_HTTP_STATUS_PORT=8080
ENV THINGSDB_MODULES_PATH=/modules
ENV THINGSDB_PYTHON_INTERPRETER=python3
ENV THINGSDB_STORAGE_PATH=/data

ENTRYPOINT ["/usr/local/bin/thingsdb"]

