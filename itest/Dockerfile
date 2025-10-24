FROM python AS builder
RUN apt-get update && \
    apt-get install -y \
        libuv1-dev \
        libpcre2-dev \
        libyajl-dev \
        libcurl4-gnutls-dev \
        build-essential \
        cmake
WORKDIR /opt
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./main.c ./main.c
COPY ./src/ ./src/
COPY ./inc/ ./inc/
COPY ./libwebsockets/ ./libwebsockets/
RUN cmake -DCMAKE_BUILD_TYPE=Debug . && make

FROM python
RUN apt-get update && \
    apt-get install -y \
        valgrind \
        libuv1 \
        libpcre2-8-0 \
        libyajl2 \
        libcurl3t64-gnutls
COPY --from=builder ./opt/thingsdb /thingsdb
COPY ./itest/ /itest/
COPY ./inc/doc.h /inc/doc.h
COPY ./memleak.supp /memleak.supp
WORKDIR /itest
RUN pip install -r requirements.txt
ENV THINGSDB_BIN=/thingsdb
ENV THINGSDB_MEMCHECK=1
CMD [ "python", "run_all_tests.py" ]
