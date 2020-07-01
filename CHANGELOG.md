# v0.9.6

* ThingDB `Type` may be extended using methods, issue #56.
* Prevent quick away mode while processing many events, issue #57.
* Expose `Type` methods when wrapping a thing, issue #58.
* Use fast hash method to find constant names, issue #59.
* Fixed function `del_node()` which could leak some data, issue #60.
* Fixed restore old types for backwards compatibility, issue #61.

# v0.9.5

* Allow creating backups on a single node, issue #50.
* Added a new function `event_id()`, issue #51.
* Fixed bug when using `wrap(..)` in thingsdb or node scope, issue #53.
* Support `Type(..)` and `Enum(..)` syntax, issue #54.


# v0.9.4

* Added a new function `type_assert()`, issue #45.
* Changed return value for function `assert()` to `nil`, issue #44.
* Fixed removing the last enumerator member, issue #48.
* Changed `emit(..)` to contain values like query results, issue #47.


# v0.9.3

* Added `emit(..)` function for broadcasting events, issue #31.
* Fixed error in generating a new type instance with enumerators, issue #33.
* Fixed invalid copy of list when using the `new(..)` function, issue #36.
* Added a new function `assign(..)`, issue #32.
* Fixed incorrect type count when using thing as variable, issue #42.
* Fixed missing Thing ID after receiving a new thing by an event, issue #41.
* Changed the return value for function `.del(..)`, issue #40. 
* Fixed instances may be in use while changing a type, issue #39.
* Fixed `list` or `set` may keep restriction after using mod_type(), issue #38.
* Allow the `mod` action for function `mod_type(..)` to use a closure and remove specification restriction, issue #34.


# v0.9.2

* Accept a closure when adding a type definition, issue #10.
* Initialize type using default values, issue #25.
* Added option to change default value for enum type, issue #26.
* Fixed iterating over a set returning `nil` instead of `0`, issue #29.
* Added `reduce(..)`, `some(..)` and `every(..)` for the `set` type, issue #24.


# v0.9.1

* Fixed wrong formatting thing with one item, issue #23.
* Add a rename action to the `mod_enum(..)` and `mod_type(..)` functions, issue #18.
* Support for minus (and plus) sign before expressions, issue #21.


# v0.9.0

* Added an *enum* type, issue #9.
* Variable in scope is not re-used, issue #16.
* Prevent creating type which name conflict with a build-in type or specification, issue #15.
* Existing array and set specifications are not updated after using mod_type(...), issue #14.
* No copy is created for an array when calling a closure bug, issue #13.
* Incorrect delete type after using mod_type(), issue #12.
* Typo in function "new" error message, issue #11.
* Return procedure definitions using a readable format enhancement, issue #7.


# v0.8.9

* Wrap() might fail after changing a Type, issue #6.


# v0.8.8

* Exclude backup files from ThingsDB backup file.


# v0.8.7

* Fixed connecting nodes.
* Fixed synchronizing on alphine linux.
* Added restore() function.