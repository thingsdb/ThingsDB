FROM python as builder
RUN apt-get update && \
    apt-get install -y \
        libuv1-dev \
        libpcre2-dev \
        libyajl-dev \
        libcurl4-nss-dev \
        libwebsockets-dev \
        build-essential
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
        libcurl3-nss \
        libwebsockets17
COPY --from=builder ./Release/thingsdb /Release/thingsdb
COPY ./itest/ /itest/
COPY ./inc/doc.h /inc/doc.h
COPY ./libcurl.supp /libcurl.supp
WORKDIR /itest
RUN pip install -r requirements.txt
ENV THINGSDB_BIN /Release/thingsdb
CMD [ "python", "run_all_tests.py" ]
