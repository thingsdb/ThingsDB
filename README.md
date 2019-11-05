# ThingsDB

## TODO / Road map

- [ ] Update tests
    - [ ] More subscribe tests
    - [ ] More Type tests
    - [ ] More Wrap tests
    - [ ] Complete ThingsDB functions test
- [ ] Update documentation
- [x] Documentation testing
- [ ] Start project **ThingsBook** for a beginner guide on how to work with ThingsDB
- [x] Accept environment variable and make config file optional
    - When both a config file and environment variable, the latter should win
- [ ] Syntax Highlighting
    - [x] Pygments (Python)
    - [x] Chroma  (Go, first make Pygments, it should be easy to convert to Chroma)
    - [ ] Monoca editor
    - [ ] VSCode (related to Monaco editor)
    - [ ] Ace editor
- [x] set_type, for configuring an empty type without instances
- [ ] thing.watch()/thing.unwatch() function calls.

## Plans and Ideas for the Future
- [ ] Big number support?


## Installation msgpack

```
git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c
cmake .
make
make test
sudo make install
```

## Get status

```
wget -q -O - http://node.local:8080/status
```

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam
