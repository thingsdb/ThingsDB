# ThingsDB

## TODO / Road map

- [ ] Update watch documentation
- [ ] Start project **ThingsBook** for a beginner guide on how to work with ThingsDB
- [ ] check long running events, in combination with node connect loop


## Plans and Ideas for the Future

- [ ] Big number support?
- [ ] support for things with any "raw" keys?

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)
 - [HTTP Parser](https://github.com/nodejs/http-parser/releases)
 - [MessagePack](https://github.com/msgpack/msgpack-c)
 - [libuv](https://libuv.org)
 - [PCRE](https://www.pcre.org)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam

## Installation

https://docs.thingsdb.net/v0/installation/

## Documentation

https://docs.thingsdb.net

## Run integration tests
```
docker build -t thingsdb/itest -f itest/Dockerfile .
docker run thingsdb/itest:latest
```
