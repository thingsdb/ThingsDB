FROM google/cloud-sdk:latest
COPY ./ /tmp/thingsdb/
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    libuv1-dev \
    libpcre2-dev \
    libyajl-dev && \
    git clone https://github.com/transceptor-technology/libcleri.git /tmp/libcleri && \
    cd /tmp/libcleri/Release && \
    make all && \
    make install && \
    cd /tmp/thingsdb/Release && \
    make clean && \
    make

FROM google/cloud-sdk:latest
RUN mkdir -p /var/lib/thingsdb && \
    apt-get update && apt-get install -y \
    libuv1 \
    libpcre2-8-0 \
    libyajl2 && \
    pip3 install py-timod

COPY --from=0 /tmp/thingsdb/Release/thingsdb /usr/local/bin/
COPY --from=0 /usr/lib/libcleri* /usr/lib/

ENV PYTHONUNBUFFERED=1
ENV THINGSDB_PYTHON_INTERPRETER=python3

# Data
VOLUME ["/var/lib/thingsdb/"]
# Client (Socket) connections
EXPOSE 9200
# Client (HTTP) connections
EXPOSE 9210
# Node connections
EXPOSE 9220
# Status (HTTP) connections
EXPOSE 8080

ENV THINGSDB_BIND_NODE_ADDR 0.0.0.0
ENV THINGSDB_BIND_CLIENT_ADDR 0.0.0.0

ENTRYPOINT ["/usr/local/bin/thingsdb"]

