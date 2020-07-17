FROM google/cloud-sdk
COPY ./ /tmp/thingsdb/
RUN apt-get update && \
    apt-get install -y libpcre2-dev libuv1-dev libyajl-dev && \
    git clone https://github.com/transceptor-technology/libcleri.git /tmp/libcleri && \
    cd /tmp/libcleri/Release && \
    make all && \
    make install && \
    cd /tmp/thingsdb/Release && \
    make clean && \
    make && \
    mkdir -p /var/lib/thingsdb && \
    cp /tmp/thingsdb/Release/thingsdb /usr/local/bin/ && \
    rm /tmp/thingsdb -R && \
    rm /tmp/libcleri -R

# Data
VOLUME ["/var/lib/thingsdb/"]
# Client connections
EXPOSE 9200
# HTTP API connections
EXPOSE 9210
# back-end connections
EXPOSE 9220
# back-end connections
EXPOSE 9220

ENV THINGSDB_BIND_NODE_ADDR 0.0.0.0
ENV THINGSDB_BIND_CLIENT_ADDR 0.0.0.0

ENTRYPOINT ["/usr/local/bin/thingsdb"]
