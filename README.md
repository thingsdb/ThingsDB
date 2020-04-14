# ThingsDB Node

ThingsDB is an object-oriented database with event-driven features that allows 
developers to store *things* in an intuitive way.

## Documentation

https://docs.thingsdb.net

## Installation

https://docs.thingsdb.net/v0/getting-started/build-from-source/

## Run integration tests

```
docker build -t thingsdb/itest -f itest/Dockerfile .
docker run thingsdb/itest:latest
```
