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

### Current situation
There are three protocols which can be used to query the current node, thingsdb
or a collection. This prevents the option to create a single script which will
setup a collection. You always need a "thingsdb" script and a "colection" script
for each collection. It also prevents to get node_info() from other nodes than
the one you are connected to.

### Solution
A single protocol and include the target scope in the query. See examples.

Query ThingsDB
```
#thingsdb
new_collection('my_collection');
```

Query the current node
```
#node
node_info();
```

Query a specific node will be possible (this is not possible with the current solution)
```
#node:3
node_info();
```

Query a collection
```
#collection:stuff
.greet = "Hello world";
```

..or by omitting the "collection" part as short syntax
```
#:stuff
.greet = "Hello world";
```

Collections and ThingsDB can be mixed in a single query. Only when targeting a
specific node, switching to another scope will raise an error. This should not
be a problem since node target queries never have events, and so are never used
in building collections, users or anything else.

So the following will be possible:
```
#thingsdb
new_collection('my_collection');

// Now we switch to the collection scope, this will set the query result to `nil`
#:my_collection
.greet = "Hello world";
```

A client can still implement something like `client.use('stuff')` so as a user
you don't have to write `#:stuff` on top of each query.


## Get status

```
wget -q -O - http://node.local:8080/status
```

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)

## Fonts:

https://fonts.adobe.com/fonts/keraleeyam