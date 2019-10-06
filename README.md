# ThingsDB

## TODO / Road map

- [ ] Update tests
    - [ ] More subscribe tests
    - [ ] More Type tests
    - [ ] More Wrap tests
    - [ ] Complete ThingsDB functions test
- [ ] Update documentation
- [ ] Documentation testing
- [ ] Start project **ThingsBook** for a beginner guide on how to work with ThingsDB
- [ ] Accept environment variable and make config file optional
    - When both a config file and environment variable, the latter should win
- [ ] Syntax Highlighting
    - [ ] Pygments (Python)
    - [ ] Chroma  (Go, first make Pygments, it should be easy to convert to Chroma)
    - [ ] Monoca editor
    - [ ] VSCode (related to Monaco editor)
    - [ ] Ace editor
- [ ] Consider switching to MessagePack, since Qpack does not really add something
      and MessagePack does not require to add the serialization protocol for
      each new language. I seriously do not like the unpacker written for C, but
      it is not that hard to write this part for ThingsDB usage.



## Plans and Ideas for the Future
- [ ] Big number support?


## Get status

```
wget -q -O - http://node.local:8080/status
```

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam
