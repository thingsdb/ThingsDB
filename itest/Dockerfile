FROM ubuntu:20.04 as builder
RUN apt-get update && \
    apt-get install -y \
        git \
        libuv1-dev \
        libpcre2-dev \
        libyajl-dev \
        libcurl4-nss-dev \
        build-essential
RUN git clone https://github.com/cesbit/libcleri.git /tmp/libcleri && \
    cd /tmp/libcleri/Release && \
    make clean && \
    make && \
    make install
COPY ./main.c ./main.c
COPY ./src/ ./src/
COPY ./inc/ ./inc/
COPY ./Release/ ./Release/
RUN cd ./Release && \
    make clean && \
    CFLAGS="-Werror -Winline" make

FROM python
RUN apt-get update && \
    apt-get install -y \
        valgrind \
        libuv1 \
        libpcre2-8-0 \
        libyajl2 \
        libcurl3-nss
COPY --from=builder ./Release/thingsdb /Release/thingsdb
COPY --from=builder /usr/lib/libcleri* /usr/lib/
COPY ./itest/ /itest/
COPY ./inc/doc.h /inc/doc.h
COPY ./libcurl.supp /libcurl.supp
WORKDIR /itest
RUN pip install -r requirements.txt
ENV THINGSDB_BIN /Release/thingsdb
CMD [ "python", "run_all_tests.py" ]
