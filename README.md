[![CI](https://github.com/thingsdb/ThingsDB/workflows/CI/badge.svg)](https://github.com/thingsdb/ThingsDB/actions)
[![Release Version](https://img.shields.io/github/release/thingsdb/ThingsDB)](https://github.com/thingsdb/ThingsDB/releases)

# ThingsDB Node

ThingsDB is an object-oriented database with event-driven features that allows
developers to store *things* in an intuitive way.

## Website

Want your own **playground**? Visit the website: https://thingsdb.net

## Documentation

ThingsDB documentation: https://docs.thingsdb.net

## Installation

- From source: https://docs.thingsdb.net/v0/getting-started/build-from-source/
- Image on docker hub: https://hub.docker.com/repository/docker/thingsdb/node

## Docker

When using Docker, do not forget to mount a volume
if you want ThingsDB to keep all changes.

For example:

```bash
docker run \
    -d \
    -p 9200:9200 \
    -p 9210:9210 \
    -p 8080:8080 \
    -v ~/thingsdb-data:/data \
    -v ~/thingsdb-modules:/modules \
    thingsdb/node:latest \
    --init
```

> :point_up: **Note:** The `--init` argument is technically only required when running ThingsDB for the first time.

Replace the `latest` tag or build your own Docker image if you want to test using Python modules or Google Cloud support *(see the [getting started page](https://docs.thingsdb.net/v0/getting-started/) for more info)*

## Run integration tests

Docker is required to run the integration tests.

```
docker build -t thingsdb/itest -f itest/Dockerfile .
docker run thingsdb/itest:latest
```

Or, if you want all the output:

```
docker run \
    -e THINGSDB_VERBOSE=1 \
    -e THINGSDB_MEMCHECK=1 \
    -e THINGSDB_LOGLEVEL=debug \
    -e THINGSDB_NODE_OUTPUT=true \
    thingsdb/itest:latest
```
