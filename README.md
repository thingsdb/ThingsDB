# ThingsDB

## TODO / Road map

- [x] Update watch documentation
- [ ] Start project **ThingsBook** for a beginner guide on how to work with ThingsDB
- [ ] Check long running events, in combination with node connect loop
- [x] Re-visit recursive closures

## Plans and Ideas for the Future

- [ ] Big number support?
- [ ] Support for things with any "raw" keys?
- [x] Allow procedure arguments as a dictionary
- [x] Should closures accept any amount of arguments when called? and `nil` values
      when not enough arguments are given? This behavior would be more inline with
      how functions currently handle closures when given as argument.
- [ ] Support for Type closures, see *Thoughts about Type closures*
- [ ] Assign data to set of nodes with an X redundancy and async methods to
      retreive the data. Biggest issue is how to staty operational while 
      upscaling.
      
## Thoughts about Type closures

```
set_type('Foo', {
    props: || this.keys(),
});
```

Do we want to keep track of `this`?, or should we write more like Python
```
set_type('Foo', {
    props: |this| this.keys(),
});
```

Introducing `this` as a keyword will include more complex behavior as this
should work and return with a proper value but enables more flexibilty for
the same reason. For example, we could use `this` with normal things as well.

we have to think of the following:
 - Foo{}.len() will return 0, and does not count the Type closures
 - Foo{}.props may return 'unknown property', this way we only need to catch
      true calls to the function.
 - Foo{}.props() return and empty [] since the Type closure is not included
 
    

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)
 - [HTTP Parser](https://github.com/nodejs/http-parser/releases)
 - [MessagePack](https://github.com/msgpack/msgpack-c)
 - [libuv](https://libuv.org)
 - [PCRE](https://www.pcre.org)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam

## Installation

https://docs.thingsdb.net/v0/getting-started/build-from-source/

## Documentation

https://docs.thingsdb.net

## Run integration tests
```
docker build -t thingsdb/itest -f itest/Dockerfile .
docker run thingsdb/itest:latest
```
