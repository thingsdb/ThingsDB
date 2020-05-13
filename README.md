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

## Run integration tests

Docker is required to run the integration tests.

```
docker build -t thingsdb/itest -f itest/Dockerfile .
docker run thingsdb/itest:latest
```

