FROM ubuntu:19.10 as builder
RUN apt-get update && \
    apt-get install -y \
        git \
        libuv1-dev \
        libpcre2-dev \
        libyajl-dev \
        build-essential
RUN git clone https://github.com/transceptor-technology/libcleri.git /tmp/libcleri && \
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
    CFLAGS="-Werror" make

FROM python
RUN apt-get update && \
    apt-get install -y \
        valgrind \
        libuv1 \
        libpcre2-8-0 \
        libyajl2
COPY --from=builder ./Release/thingsdb /Release/thingsdb
COPY --from=builder /usr/lib/libcleri* /usr/lib/
COPY ./itest/ /itest/
COPY ./inc/doc.h /inc/doc.h
WORKDIR /itest
RUN pip install -r requirements.txt
ENV THINGSDB_BIN /Release/thingsdb
CMD [ "python", "run_all_tests.py" ]
