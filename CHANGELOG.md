# v0.10.4

* Added things as dictionary support, issue #146.
* Allow zero arguments for `wse(..)` function, issue #148.
* Added `regex()` function, issue #149.
* Added `first()` and `last()` functions for a list, issue #150.
* Deprecate injection of special variable, issue #151.
* Fixed bug with multiple HTTP API request on the same connection, issue #153.
* Deprecated *str*.test() and added *regex*.test() function, issue #155.
* Added `is_regex(..)` function, issue #156.
* Fixed bug with `future()` and `mod_type()`, issue #157.
* Fixed bug with cached queries containing invalid syntax, issue #159.
* Fixed bug with `wse(..)` internal status, issue #160.

# v0.10.3

* Fixed Alpine Linux build, issue #152.

# v0.10.2

* Evaluate `error` values as `false`, issue #135.
* Removed list of deprecated functions, issue #136.
* Fixed bug in enumerator member conversion to `str` or `bytes`, issue #137.
* Changed access policy, issue #138.
* Fixed possible memory leak in `imap`, issue #139.
* Added future type, issue #140.
  - `then`: https://docs.thingsdb.net/v0/data-types/future/then/
  - `else`: https://docs.thingsdb.net/v0/data-types/future/else/
  - `future`: https://docs.thingsdb.net/v0/collection-api/future/
  - `is_future`: https://docs.thingsdb.net/v0/collection-api/is_future/
  - `cancelled_err`: https://docs.thingsdb.net/v0/errors/cancelled_err/
* Added module support, issue #141.
  - `has_module`: https://docs.thingsdb.net/v0/thingsdb-api/has_module/
  - `module_info`: https://docs.thingsdb.net/v0/thingsdb-api/module_info/
  - `modules_info`: https://docs.thingsdb.net/v0/thingsdb-api/modules_info/
  - `new_module`: https://docs.thingsdb.net/v0/thingsdb-api/new_module/
  - `rename_module`: https://docs.thingsdb.net/v0/thingsdb-api/rename_module/
  - `set_module_conf`: https://docs.thingsdb.net/v0/thingsdb-api/set_module_conf/
  - `set_module_scope`: https://docs.thingsdb.net/v0/thingsdb-api/set_module_scope/
  - `restart_module`: https://docs.thingsdb.net/v0/node-api/restart_module/
* Added `equals` function for comparing things, issue #144.
* Remove procedures in the `@thingsdb` scope on restore, issue #147.

# v0.10.1

* Added `datetime` and `timeval` types and related functions, issue #127.
  - `is_datetime`: https://docs.thingsdb.net/v0/collection-api/is_datetime/
  - `is_timeval`: https://docs.thingsdb.net/v0/collection-api/is_timeval/
  - `datetime`: https://docs.thingsdb.net/v0/collection-api/datetime/
  - `timeval`: https://docs.thingsdb.net/v0/collection-api/timeval/
  - `set_time_zone`: https://docs.thingsdb.net/v0/thingsdb-api/set_time_zone/
  - `time_zones_info`: https://docs.thingsdb.net/v0/thingsdb-api/time_zones_info/
  - `extract`: https://docs.thingsdb.net/v0/data-types/datetime/extract/
  - `format`: https://docs.thingsdb.net/v0/data-types/datetime/format/
  - `move`: https://docs.thingsdb.net/v0/data-types/datetime/move/
  - `replace`: https://docs.thingsdb.net/v0/data-types/datetime/replace/
  - `to`: https://docs.thingsdb.net/v0/data-types/datetime/to/
  - `week`: https://docs.thingsdb.net/v0/data-types/datetime/week/
  - `weekday`: https://docs.thingsdb.net/v0/data-types/datetime/weekday/
  - `yday`: https://docs.thingsdb.net/v0/data-types/datetime/yday/
  - `zone`: https://docs.thingsdb.net/v0/data-types/datetime/zone/
* Added a lookup for procedures, issue #129.
* Ordered procedures, types and enumerators by name, issue #130.
* Added query caching, issue #131.
* Honor IP configuration for HTTP API and health checkes, issue #133.
* Functions `new_token` and `new_backup` now accept date/time type, issue #134. 

# v0.10.0

* Change local variable in scope behavior, issue #123.
* Fixed variable in closure body, issue #122.
* Fixed cleanup variable after exception in body, issue #121.
* Reserve `date`, `time` and `datetime` names, issue #125.
* Changed exception when trying to delete an online node, issue #126.

# v0.9.24

* Fixed bug in `export()` function, issue #119.
* Fixed bug in order when applying wrap-only mode, issue #120.

# v0.9.23

* Fixed bug in wrap-only when set by function `set_type(..)`, issue #118.

# v0.9.22

* Added `join(..)` function, issue #114.
* Added `randstr(..)` function, issue #115.
* Fixed bug in restoring things from the garbage collector, issue #117.

# v0.9.21

* Fixed internal buffer resize when formatter is used, issue #113.

# v0.9.20

* Enable `warning` logging for deprecated functions, issue #109.
* Fixed Kubernetes compatibility, issue #111.

# v0.9.19

* Added `export(..)` as an experimental function, issue #104.
* Fixed bug in formatting closure definitions, issue #103.
* Enable `debug` logging for deprecated functions, issue #106.
* Fixed bug in garbage collection, issue #107.

# v0.9.18

* Added `result_size_limit` to prevent large return data, issue #101.
* Added `largest_result_size` counter, issue #102. 

# v0.9.17

* Remove restrictions when a `set` is converted to a `list`, issue #96.
* No longer allocate too much space for a `tuple` in a `list`, issue #97.
* Improved token generation by using the `getrandom` syscall, issue #98.
* Fixed releasing cached integer values, issue #99.
* Added `is_closure(..)` function, issue #100.

# v0.9.16

* Added new functions, issue #89.
  - `has`: https://docs.thingsdb.net/v0/data-types/list/has/
  - `reverse`: https://docs.thingsdb.net/v0/data-types/list/reverse/
  - `shift`: https://docs.thingsdb.net/v0/data-types/list/shift/
  - `unshift`: https://docs.thingsdb.net/v0/data-types/list/unshift/
  - `split`: https://docs.thingsdb.net/v0/data-types/str/split/
  - `trim`: https://docs.thingsdb.net/v0/data-types/str/trim/
  - `trim_left`: https://docs.thingsdb.net/v0/data-types/str/trim_left/
  - `trim_right`: https://docs.thingsdb.net/v0/data-types/str/trim_right/
  - `replace`: https://docs.thingsdb.net/v0/data-types/str/replace/
  - `alt_raise`: https://docs.thingsdb.net/v0/collection-api/alt_raise/
* Consistent function naming, issue #93.
* Fixed possible error when ThingsDB types are cleared, issue #94.

# v0.9.15

* Fixed incorrect default value after changing a type, issue #91.
* Fixed error when changing a type definition with a new condition, issue #90.

# v0.9.14

* Fixed bug with lost parents after a type property rename action, issue #88.

# v0.9.13

* Fixed bug in full storage after renaming enumerator or type, issue #87.

# v0.9.12

* Added `rename_type(..)` function, issue #83.
* Added `rename_enum(..)` function, issue #84.
* Added `rename_procedure(..)` function, issue #85.

# v0.9.11

* Fixed bug in nested `set_type(..)` calls when they are linked, issue #79.
* Fixed bug when using the `--version` argument flag, issue #77.
* Fixed error handling when using the `set_type(..)` function, issue #78.
* Fixed bug in assigning a `set` from a thing to a variable, issue #80.
* Fixed misssing colon `:` character in slices, issue #81.
* Never set the `any` type as nillable for quick compare, issue #82.

# v0.9.10

* Fixed bug in function `filter(..)` when used on a list, issue #76.

# v0.9.9

* Fixed bug in `mod_type(..)` when wrap-only mode is enabled, issue #72.
* Improvements on managing backup files, issue #71.
* Added Google Cloud Storage backup support, issue #74.

# v0.9.8

* Fixed bug in replacing a method with a new closure, issue #70.

# v0.9.7

* Fixed potential memory leak when re-using variable, issue #63.
* Fixed a bug when wrapping a `set` type to a `list` type, issue #65.
* Added range and pattern conditions, issue #62.
* Added option to set a `Type` in *wrap-only* mode, issue #64.
* Added option to take access control after restore, issue #68.

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
