# ThingsDB

## TODO / Road map

- [ ] Improve syntax errors
- [ ] Update documentation
  - [ ] Slices doc
  - [ ] Update parenthesis which are not required anymore
  - [ ] Add procedure docs
  - [ ] Update ternary operator
- [ ] Start project **ThingsBook** for a beginner guide on how to work with ThingsDB
- [ ] Review exceptions raised bt ThingsDB. (maybe add some additional ones?)
- [ ] Introduce a WARNING protocol which will be send to client as fire-and-forget
      Maybe in the format `{"warn_msg": ... "warn_code": }` ?
      Somewhere it should be possible to disable warning messages. This could be
      done with a client protocol, or change the auth protocol? or maybe it should
      be disabled explicitly for a query. This last option is maybe not so bad
      because it will prevent missing warnings.

## Plans and Ideas for the Future
- [ ] Big number support?
- [ ] Some sort of `Class` / `Type` support for strict types?
  - [ ] A `type` variable can help to build this

## Plan for a single QUERY protocol



## Get status

```
wget -q -O - http://node.local:8080/status
```

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam
