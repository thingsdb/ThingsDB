FROM google/cloud-sdk:458.0.1
COPY ./ /tmp/thingsdb/

RUN apt-get update && apt-get install -y \
    build-essential \
    libuv1-dev \
    libpcre2-dev \
    libyajl-dev \
    libcurl4-nss-dev \
    libwebsockets-dev && \
    cd /tmp/thingsdb/Release && \
    make clean && \
    make

FROM google/cloud-sdk:458.0.1

RUN mkdir -p /var/lib/thingsdb && \
    apt-get update && apt-get install -y \
    libuv1 \
    libpcre2-8-0 \
    libyajl2 \
    libcurl3-nss \
    libwebsockets16 && \
    pip3 install py-timod

COPY --from=0 /tmp/thingsdb/Release/thingsdb /usr/local/bin/

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

