# v1.7.0

* Fix incorrect value error when user or token is not found, pr #390 (@rickmoonex).
* Implement optional _name_ for a room, pr #393.
* Use MessagePack Extension to internally store names, issue #394.
* Added whitelist support for both _"procedures"_ and _"rooms"_, pr #395.
* Added option for a default enumerator member in a type definition, pr #396.
* New functions:
  - `whitelist_add()`: https://docs.thingsdb.io/v1/thingsdb-api/whitelist_add/
  - `whitelist_del()`: https://docs.thingsdb.io/v1/thingsdb-api/whitelist_del/
  - `room.set_name()`: https://docs.thingsdb.io/v1/data-types/room/set_name/

# v1.6.6

* Allow `user_info()` in a collection scope, discussion #385.

# v1.6.5

* Fixed bug in debug logging when `task.del()` is called with an error, issue #384.

# v1.6.4

* Fixed using `task.del()` within the task callback, issue #383.

# v1.6.3

* Honor key restrictions with JSON load function, issue #382.

# v1.6.2

* Fixed loading JSON strings with `json_load()` function, issue #381.

# v1.6.1

* Add support for ARM64 container, pr #377 (@rickmoonex).
* Both the minimal and full docker builds compatible with legacy hardware, pr #380 (@rickmoonex).

# v1.6.0

* Listen to `THINGSDB_INIT`, `THINGSDB_SECRET` and `THINGSDB_DEPLOY` environment variable.
* Added `tls.Dockerfile` for TLS/SSL support in docker container.
* Fixed logging from forward request with API.
* Added optional [WebSocket](https://libwebsockets.org) (including TLS/SSL) support.
* Changed build system to `cmake` and added build scripts:
  - Release build: `./release-build.sh`
  - Debug build: `./debug-build.sh`

# v1.5.2

* Restrict future arguments for `then()` or `else()` to objects without an Id.
* Module as type, no longer the workaround of exposing modules as futures, pr #365.
* Added the `is_module()` function, pr #365.
* Support ThingsDB code in exposed module functions, pr #365.

# v1.5.1

* Added support for building on MacOS.
* Fixed spelling mistake in error message.

# v1.5.0

* Comments can now be placed anywhere where white-space is allowed.
* Comments are no longer stored within closures. This behavior was previously
  partially enforced, and now it is strictly followed.
* Statements must be separated with semicolons. Syntax checking is now more strict.
* Removed syntax option for selecting a collection scope by Id.
* The collection Id and collection root Id no longer need to be equal.
* Added `next_free_id` field to collection info and removed from nodes info.
  _(The response for `node_info()` still contains the `next_free_id` for the global free Id)_
* Added `dump` option to the `export(..)` function for a full collection export.
* Added function `import()` which can import a collection export dump.
* Added function `root()` for getting the collection root as thing.
* Function `export(..)` can no longer be used in queries with changes.
* A bug with adding enumerators to restricted lists has been fixed.
* A bug with applying a new relation to existing data has been fixed.
* A bug with async future error reporting has been fixed.
* A bug with clearing nesting tasks through arguments has been fixed.
* Tasks owners must be set to owners with equal or less access flags.
* The `change._tasks` structure in C code has been renamed.
  _(This has no impact on user-facing functionality)_
* Replaced Docker images _(both tests and builds)_ with newer versions.
* Removed `ti_pkg_check` hack for old compilers. _(this is no longer required with recent compilers)_
* Added integer minimum `INT_MIN` and maximum `INT_MAX` constants with corresponding values.
* The primary domain for the ThingsDB website has been changed from `.net` to `.io`.
* Several new mathematical functions have been added: `abs(..)`, `ceil(..)`,
  `cos(..)`, `exp()..`, `floor(..)`, `log10(..)`, `log2(..)`, `loge(..)`,
  `pow(..)`, `round(..)`, `sin(..)`, `sqrt(..)` and `tan(..)`.
* Added mathematical constants `MATH_E` and `MATH_PI` with corresponding values.
* The library `libcleri` is now integrated into the core ThingsDB code base, eliminating the need for separate installation.
* The `new_backup()` and `new_token()` functions no longer accept `int`, `float` or `str` as time.
  _(This was marked as deprecated since v0.10.1)_
* Added set operators `<=`, `<`, `>=`, `>` for subset, proper subset, super-set and proper super-set checking.
* Added range `<..>` support for UTF-8 type property definitions.
* Added bit-wise NOT (`~`) operator.
* Added bit-wise Left (`<<`) and Right (`>>`) shifting operators.
* Return `nil` instead of success error when a repeating task is successful.
* Corrected a spelling mistake in error message for integer range values.
* No longer allow a relation between a none-stored set and a none-stored value.
* Removed the `<` and `>` from returning a room to a client to be consistent.
* Allow explicit variable list for empty future using a direct closure.
* Return value of `mod_procedure(..)` has changed to `nil` on success.
* Enforce `argmap` property for exposed module methods.
* Prevent adding a duplicated node _(based on address and port)_.
* No shutdown wait time for uninitialized nodes.
* New user and new collection return the name, not the ID.
* Ignore empty collections on restore for better user experience.
* Increase range maximum from `9999` to `100000`.
* Increase maximum future closure calls from `8` to `255`.

# v1.4.16

* Improve `dup()` and `copy()` with self references, pr #355.

# v1.4.15

* Added support for methods on an _enumerator_ type, pr #352.
* Added function `count(..)`, pr #353 and #354.
* Added function `sum(..)`, pr #354.

# v1.4.14

* Fixed bug with using function `wrap(..)` in module argument, issue #351.

# v1.4.13

* Added function `mod_procedure(..)`, pr #348.
* Added function `flat(..)`, pr #349.
* Fixed enumerator lookup in wrong scope, issue #350.

# v1.4.12

* Added `timeit(..)` function, issue #345.
* Added type definitions: `email`, `url` and `tel`, issue #346.
* Added corresponding functions:
  - `is_email(..)`: Determine if a given value is an email address.
  - `is_url(..)`: Determine if a given value is a URL.
  - `is_tel(..)`: Determine if a given value is a telephone number.

# v1.4.11

* Fixed bug with computed methods, issue #343.
* Fixed bug with enumerator member to string conversion, issue #344.

# v1.4.10

* Fixed daylight saving times bug in `move(..)` function, issue #342.

# v1.4.9

* Added `backups_ok()` function for the _node_ scope, issue #341.

# v1.4.8

* Fixed bug with `move(..)` function with timezone information, issue #340.

# v1.4.7

* Added timezone **Europe/Kyiv** for the correct spelling, _Fuck ruZZia_.
* Fixed Docker builds to include timezone data.

# v1.4.6

* Implemented **optional chaining** _(double dot, `..`)_ syntax, issue #339.
* Implemented type definition `task`, issue #336.
* Implemented enumerator member flag `*`, issue #338.

# v1.4.5

* Fixed bug with recursion in computed properties, issue #332.
* Added `nse(..)` function, pr #333.
* Fixed bug with short syntax in closure, issue #334.

# v1.4.4

* Fixed minor bug with assertion test _(debug mode only)_, issue #329.
* Fixed trivial bug with deep level in computed properties, issue #331.

# v1.4.3

* Added _shutdown-period_ configuration option, issue #324.
* Added authentication mask `USER`, issue #325.
* Add `id()` to wrapped type and update `map_id()` function, issue #327.

# v1.4.2

* Added `fill(..)` function, issue #320.
* Allow away mode (blocking) on a single node, issue #321.
* Increase `range(..)` limitation, issue #322.

# v1.4.1

* Return same instance on type match, issue #315.
* Added `ren(..)` function, issue #316.
* Added `enum_map(..)` function, issue #318.

# v1.4.0

* Fixed mismatch in refcount, issue #309.
* Fixed bug with relations between things without Id, issue #308.
* Improve performance, pr #313.
* Fixed bug with nested tuples after copy or duplicate, issue #314.

# v1.3.3

* Definition must follow flags, issue #306.
* API request might free collection name before reading, issue #307.

# v1.3.2

* Improve performance when looping over sets with relations, pr #303.
* Add option to hide an Id for a Type, pr #304.
* Added additional prefix flags (`-`, `+` and `^`), pr #305.

# v1.3.1

* Fixed potential memory bug with relations, issue #302.

# v1.3.0

* Removed deprecated functions `return(..)` and `if(..)`, pr #297.
* Added Id (`'#'`) definition for Id mapping on typed things, issue #296.
* Added map shortcut functions, issue #298.
  - _list_.`map_id()`: https://docs.thingsdb.net/v1/data-types/list/map_id
  - _list_.`map_wrap(..)`: https://docs.thingsdb.net/v1/data-types/list/map_wrap
  - _list_.`map_type(..)`: https://docs.thingsdb.net/v1/data-types/list/map_type
  - _set_.`map_id()`: https://docs.thingsdb.net/v1/data-types/set/map_id
  - _set_.`map_wrap(..)`: https://docs.thingsdb.net/v1/data-types/set/map_wrap
* Changed the default *deep* value back to `1` (one), issue #300.
* Allow `&` to prefix a definition for the same deep level when wrapping a *thing*, issue #301.

# v1.2.9

* Fixed rename type when used with restricted things, issue #292.
* Added `enum` and `union` as reserved names, issue #294.
* Support `NO_IDS` flag on _return_ statement, issue #293.

# v1.2.8

* Fixed gcloud backup return message when failed, issue #288.
* Added `one()` function, pr #290.
* Fixed potential memory leak with keys, issue #291.

# v1.2.7

* Reduce indentation from four spaces to a single `TAB`, issue #284.
* Implement short syntax for init thing or instance, issue #283.
* Added `vmap(..)` function, issue #286.

# v1.2.6

* Fixed using `log(..)` within a future, issue #282.

# v1.2.5

* Use `/dev/urandom` for a seed to initialize random, issue #280.
* Added `search(..)` function for debugging purpose, issue #281.

# v1.2.4

* Fixed restriction mismatch after type removal, issue #277.
* Added `to_thing()` function, issue #278.

# v1.2.3

* Added `closure(..)` function to create closures from string, issue #273.
* Fixed task replication bug in `set_closure(..)` function, issue #274.
* Fixed task replication bug in `set_owner(..)` function, issue #275.
* Added the option to `restore_tasks` explicitly at restore, issue #246.

# v1.2.2

* Fixed storing migration change for *non-collection* scopes, issue #269.
* Deny revoking your own `QUERY` privileges to prevent lockout, issue #270.
* Fixed evaluation from *left-to-right*, issue #271.

# v1.2.1

* Switch from *common-* to *all-* time-zones, issue #267.

# v1.2.0

* Fixed allocation bug in reverse buffer, issue #253.
* Added `if..else` as statement *(marked `if()` as deprecated)*, issue #254.
* Added `return` as statement *(marked `return()` as deprecated)*, issue #255.
* Added `for..in` with `break` and `continue` statements, issue #257.
* Fixed memory leak with re-assigning closure variable, issue #259.
* Upgrade to **libcleri v1.0.0**, issue #263.
* Configurable `deep` value per scope and changed defaults, issue #261.
* Set time zone per scope instead of collection only, issue #260.
* Syntax changes, pr #264.
* Added `dup(..)` and `copy(..)` to *set*, *list* and *tuple* type, issue #265.

# v1.1.2

* Added `is_time_zone(..)` function, issue #247.
* Fixed task scheduling when restoring from file, issue #248.
* Prevent restore when tasks are created in the `@thingsdb` scope, issue #249.
* Added support for slice and index notation on bytes, issue #251.
* Fixed bug with slice and index notation on internal names, issue #252.

# v1.1.1

* Remove scope restriction from `time_zones_info()` function, issue #236.
* Function `deep(..)` can now be used to *set* the *deep* value, issue #237.
* Remove the deprecated hash (`#..`) syntax, issue #232.
* Added `all` action to the `mod_type(..)` function, issue #235.
* Fixed unexpected behavior with unassigned typed things, issue #239.
* Added `restrict(..)` function and restrictions on non-typed things, issue #210.
* Added `restriction()` function, issue #240.
* Fixed bug with deploying static module files, issue #241.
* Fixed memory leak when deleting a locked value from a *(dict)* thing, issue #242.
* Fixed generating Id for wrapped thing in list, issue #243.
* Added optional limit argument to `set().remove(..)`, issue #244.
* Added `remove()` function for non-typed things, issue #245.

# v1.1.0

* Replace `timers` with `tasks`, pr #230.
* Fixed bug in checking arguments for scheduled tasks, issue #225.
* Fixed bug with updating properties in different threads, issue #226.
* Fixed bug with `log()` in a scheduled closure, issue #224.
* Added `things` to string conversion, issue #231.
* Store task errors so they can be viewed, issue #227.
* Deprecate the hash (`#..`) syntax, issue #232.
* Removed deprecated `READ`, `MODIFY`, `EVENT` and `WATCH`, issue #233.
* Removed deprecated `test()` on type `str`, issue #234.

# v1.0.2

* Callable modules, issue #213.
* Install modules using a GitHub repository and `module.json`, pr #216.
* Added `refresh_module()` function, issue #217.
* Added `log` function, pr #218.
* Fixed bug with nested futures, issue #219.
* Added basic JSON support, issue #220.
  - `json_dump()`: https://docs.thingsdb.net/v1/collection-api/json_dump/
  - `json_load()`: https://docs.thingsdb.net/v1/collection-api/json_load/
* Accept zero arguments for the `wrap()` function, issue #223.

# v1.0.1

* Use data from buffer to solve re-allocation pointer bug, issue #214.

# v1.0.0

* Added a `room` type to replace watching things.
* Added `room()` and `is_room()` functions.
* Added `clear()` function on `thing`, `set` and `list`.
* Fixed bug with removing a self reference from a `set<->set` relation.
* Functions for the new `room` type:
  - `id()`: Return the Id of the room.git@github.com:cesbit/oversight.website.git
  - `emit(..)`: Emit an event for the room.
* Protocol types for the new `room` type:
  - `TI_PROTO_CLIENT_ROOM_JOIN` (6)
  - `TI_PROTO_CLIENT_ROOM_LEAVE` (7)
  - `TI_PROTO_CLIENT_ROOM_EMIT` (8)
  - `TI_PROTO_CLIENT_ROOM_DELETE` (9)
  - `TI_PROTO_CLIENT_REQ_JOIN` (38)
  - `TI_PROTO_CLIENT_REQ_LEAVE` (39)
  - `TI_PROTO_CLIENT_REQ_EMIT` (40)
* Stored closures with side effects no longer require `wse` but can be used as
  long as a change is created. It is still possible to use `wse` to enfore a change.

## Breaking changes from v0.10.x -> v1.0.0

* Removed `.watch()`, `.unwatch()` and `.emit()` functions on type `thing`.
* Removed the following protocol types *(replaced with room protocol)*:
  - `TI_PROTO_CLIENT_WATCH_INI` (1)
  - `TI_PROTO_CLIENT_WATCH_UPD` (2)
  - `TI_PROTO_CLIENT_WATCH_DEL` (3)
  - `TI_PROTO_CLIENT_WATCH_STOP` (4)
  - `TI_PROTO_CLIENT_REQ_WATCH` (35)
  - `TI_PROTO_CLIENT_REQ_UNWATCH` (36)
* Insert data using syntax like `{"X": ...}` *(where X is a reserved keyword)*
  is no longer possible.
* Function `.def()` on a closure is removed. Use `str(closure)` instead.
* Changed the return values of the following types:
  - `regex`: From object to string (e.g. `{"*": "/.*/"}` -> `"/.*/"`).
  - `closure`: From object to string (e.g. `{"/": "||nil"}` -> `"||nil"`).
                Note that `str(closure)` returns formatted closure code and
                just the type `closure` returns the closure as-is.
  - `error`: From object to string (e.g. `{"!": ...}` -> `"some error msg"`).
* Backup template no longer supports `{EVENT}` as template variable. This variable
  is replaced with `{CHANGE_ID}`.
* All the counter properties with `*event*` are replaced with `*change*`.
  Counter property `changes_quorum_lost` is renamed to simply `quorum_lost`.
* All the node(s) info properties with `*event*` are replaced with `*change*`.
  Property `next_thing_id` is changed to `next_free_id` as Ids are used
  for more than just things.
* Previous `EVENT` (or `MODIFY`) access to a scope is renamed to `CHANGE`.
  (Note: this is not truly breaking since both `EVENT` and `MODIFY` are marked
  as deprecated)
* Previous `WATCH` access is renamed to `JOIN`. `WATCH` is marked as deprecated.
  (Note: this is not truly breaking since `WATCH` is marked as deprecated)
* It is no longer possible to watch the `@node` scope. All socket connections
  will receive `NODE_STATUS` changes as soon as the client is authenticated.
  Watch (JOIN) privileges are no longer required on the `@node` scope for this
  feature.
* Protocol `TI_PROTO_CLIENT_NODE_STATUS (0)` no longer returns a plain string
  with the status but instead a `map` with `id` containing the node Id
  and `status` with the new node status.
* Function `remove(..)` on a `list` type will remove *all* values where a
  given closure evaluates to `true`. The return value will be a new `list` with
  the values which are removed. An alternative value is no longer possible.

# v0.10.15

* Added `to_type(..)` to convert a thing into a type instance, issue #205.
* Added definable properties: `regex`, `closure` and `error`, issue #204.
* Fixed bug with relations after invalid type creation, issue #203.
* Fixed bug when adding a closure to an `any` type property, issue #202.
* Changed default modules path, issue #207.

# v0.10.14

* Fixed bug with `replace(..)` using an empty regular expression, issue #198.
* Fixed incorrect default value bug after using `mod_type(..)`, issue #199.
* Fixed memory leak after invalid `mod_type(..)` usage, issue #200.
* Added regular expression support on `str.split(..)` function, issue #201.

# v0.10.13

* Added `extend_unique(..)` function, issue #189.
* Lookup procedures by name, issue #192.
* Fixed bug with regular expression syntax, issue #193.
* Added regular expression support on `str.replace(..)` function, issue #194.
* Fixed bug with storing modules during a full database store, issue #195.
* Search for Python interpreter in `PATH` when no absolute path is given, issue #196.
* Added `deploy_module(..)` function, issue #197.

# v0.10.12

* Fixed bug in query-cache when hitting the query-recursion-depth, issue #188.

# v0.10.11

* Added `is_unique()` and `unique()` functions, issue #185.
* Fixed bug regarding access rights when calling a procedure, issue #186.

# v0.10.10

* Fixed bug with relations between stored and non-stored things, issue #183.
* Fixed bug while iterating over a set with relation, issue #184.

# v0.10.9

* Fixed bug in storing relations with full database store, issue #182.

# v0.10.8

* Renamed type `info` to `mpdata`, issue #176.
* Add function `load()` for loading `mpdata` into ThingsDB, issue #177.
* Add function `is_mpdata(..)` for checking the `mpdata` type, issue #179.
* Module data as `mpdata` and `load` option to overwrite this behavior, issue #178.
* Delay module deletion to prevent a memory bug while loading a module, issue #181.

# v0.10.7

* Changed default `max` value when not given in a range definition, issue #172.
* Fixed missing relations in experimental `export()` function, issue #169.
* Fixed bug with `deep` argument with the `future()` function, issue #170.
* Handle *next run* value of timers correctly after reboot, issue #171.
* Fixed loading modules at startup after a configuration change, issue #174.
* Fixed renaming a method after modifying the same method, issue #175.

# v0.10.6

* Prevent too early rejection of events, issue #167.
* Added `copy(..)` and `dup..)` functions, issue #168.

# v0.10.5

* Added option to create relations between types, issue #161.
* Added support for Python modules, issue #162.
* Added support for Timers, issue #163.
  - `new_timer`: https://docs.thingsdb.net/v0/timers-api/new_timer/
  - `del_timer`: https://docs.thingsdb.net/v0/timers-api/del_timer/
  - `has_timer`: https://docs.thingsdb.net/v0/timers-api/has_timer/
  - `set_timer_args`: https://docs.thingsdb.net/v0/timers-api/set_timer_args/
  - `timer_args`: https://docs.thingsdb.net/v0/timers-api/timer_args/
  - `timer_info`: https://docs.thingsdb.net/v0/timers-api/timer_info/
  - `timers_info`: https://docs.thingsdb.net/v0/timers-api/timers_info/
* Added computed properties on wrapped type, issue #164.
* Fixed cleaning query garbage collection too early, issue #165.

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
* Improved speed and less memory usage for internal `imap_t` type, issue #158.
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
