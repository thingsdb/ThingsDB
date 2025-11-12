[![CI](https://github.com/thingsdb/ThingsDB/workflows/CI/badge.svg)](https://github.com/thingsdb/ThingsDB/actions)
[![Release Version](https://img.shields.io/github/release/thingsdb/ThingsDB)](https://github.com/thingsdb/ThingsDB/releases)
[![Github stargazers](https://img.shields.io/github/stars/thingsdb/ThingsDB)](https://github.com/thingsdb/ThingsDB/stargazers)

# ThingsDB Node

## An Open-Source SSDI Solution for Scalable and Robust Data Management

ThingsDB is an open-source Stored-State-Distributed-Interpreter (SSDI) solution written in the C programming language. It offers a powerful and intuitive way to store, manage, and process data, making it an ideal choice for a wide range of applications.

### Key Features:

- **Robustness:** The state of the interpreter is always synchronized across nodes, ensuring data integrity and high availability even in the event of node failures.
- **Ease of Use:** ThingsDB provides a simple and intuitive programming language that makes it easy to develop applications.
- **Versatility:** ThingsDB can replace both SQL databases and message-broker solutions like RabbitMQ, simplifying application architecture.

### Capabilities:

- **Create procedures:** Define custom functions to execute complex logic within the database.
- **Define types:** Specify data structures to represent your data accurately and efficiently.
- **Emit events:** Trigger actions based on data changes and keep applications informed.
- **Create tasks:** Schedule background jobs to perform asynchronous operations.
- **Extend with modules:** Integrate with external resources and platforms using modular plugins.

### Benefits:

- **Performance:** ThingsDB is optimized for speed and efficiency, ensuring fast data processing and retrieval.
- **Flexibility:** ThingsDB's adaptable architecture allows it to fit seamlessly into various application scenarios.
- **Cost-Effectiveness:** As an open-source solution, ThingsDB eliminates licensing costs, making it a budget-friendly option.

## Website

Want your own **playground**? Visit the website: https://thingsdb.io

## Documentation

ThingsDB documentation: https://docs.thingsdb.io

## Book

Learn ThingsDB: https://docs.thingsdb.io/v1/getting-started/book/

## Installation

- From source: https://docs.thingsdb.io/v1/getting-started/build-from-source/
- Image GitHub: https://github.com/thingsdb/ThingsDB/pkgs/container/node/

## Docker

When using Docker, do not forget to mount a volume
if you want ThingsDB to keep all changes.

For example:

```bash
docker run \
    -d \
    -p 9200:9200 \
    -p 9210:9210 \
    -p 9270:9270 \
    -p 8080:8080 \
    -v ~/thingsdb-data:/data \
    -v ~/thingsdb-modules:/modules \
    ghcr.io/thingsdb/node:latest \
    --init
```

> :point_up: **Note:** The `--init` argument is technically only required when running ThingsDB for the first time.

Replace the `latest` tag or build your own Docker image if you want to test using Python modules or Google Cloud support *(see the [getting started page](https://docs.thingsdb.io/v1/getting-started/) for more info)*

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
