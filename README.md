# ThingsDB

## TODO list

- [ ] Maybe implement a `set()` for things? For the implementation we could use an imap.
      This will make lookups very fast and set operations possible.
      Methods could be:
      - [ ] add
      - [ ] remove
      - [ ] contains
- [ ] Maybe add the option for read only queries? For example via a protocol type: TI_PROTO_CLIENT_REQ_QUERY_RO
- [x] Take advantage when on a 64 bit system
    - [x] wareq uses uinpntr_t for 64bit thing id's, on 32 bit id's are allocated
    - [x] pre-caching is done on 64bit systems for integer and float values
- [x] Special floats
    - [x] Inf
    - [x] -Inf
        - [x] Add `isinf` function for checking
    - [x] NaN
        - [x] Add `isnan` function for checking
    - [x] e+- notation
- [x] Support other base integers
    - [x] 0x for hex
    - [x] 0o for octal
    - [x] 0b for binary
- [V] ~~We can make `indexing by scope` instead of using a fixed integer, but
      allowing only fixed integers makes it a bit faster and easier~~
      We can always do this later since `index by scope` is backwards compatible with the
      current syntax, but not the other way around.
- [x] ~~Overflow handling? Right now ThingsDB is naive~~
      Overflow handling is now implemented correctly. We could in the future implement
      bit_t for supporting big integer numbers.
      Except maybe for `double` compare?)
      - [ ] Introduce big numbers, export as hex {-:''} and {+:''}
      - [ ] bit_t
        - [x] `big_to_str16n`
        - [x] `big_null`
        - [x] `big_from_int64`
        - [x] `big_to_int64`
        - [ ] `big_from_strn`
            - [x] `big_from_str2n`
            - [ ] `big_from_str8n`
            - [ ] `big_from_str10n`
            - [ ] `big_from_str16n`
        - [ ] `big_from_double`
        - [ ] `big_to_double`
        - [x] `big_mulii` -> big
        - [x] `big_mulbb` -> big
        - [x] `big_mulbi` -> big
        - [x] `big_mulbd` -> double
        - [ ] `big_addii` -> big
        - [ ] `big_addbb` -> big
        - [ ] `big_addbi` -> big
        - [ ] `big_addbd` -> double
        - [ ] `big_subii` -> big
        - [ ] `big_subbb` -> big
        - [ ] `big_subbi` -> big
        - [ ] `big_subib` -> big
        - [ ] `big_subbd` -> double
        - [ ] `big_divii` -> double
        - [ ] `big_divbb` -> double
        - [ ] `big_divbi` -> double
        - [ ] `big_divib` -> double
        - [ ] `big_divbd` -> double
        - [ ] `big_divdb` -> double
        - [ ] `big_idivii` -> big
        - [ ] `big_idivbb` -> big
        - [ ] `big_idivbi` -> big
        - [ ] `big_idivib` -> big
        - [ ] `big_idivbd` -> big
        - [ ] `big_idivdb` -> big
        - [ ] `big_modii` -> big
        - [ ] `big_modbb` -> big
        - [ ] `big_modbi` -> big
        - [ ] `big_modib` -> big
        - [ ] `big_andbi` -> big
        - [ ] `big_andbb` -> big
        - [ ] `big_orbi` -> big
        - [ ] `big_orbb` -> big
        - [ ] `big_xorbi` -> big
        - [ ] `big_xorbb` -> big
        - [ ] `big_eq`
        - [ ] `big_ne`
        - [ ] `big_lt`
        - [ ] `big_le`
        - [ ] `big_gt`
        - [ ] `big_ge`
        - [ ] `big_eqi` (should in practice return false)
        - [ ] `big_nei` (should in practice return true)
        - [ ] `big_lti` (compare with int64_t)
        - [ ] `big_lei` (compare with int64_t)
        - [ ] `big_gti` (compare with int64_t)
        - [ ] `big_gei` (compare with int64_t)
        - [ ] `big_eqd`
        - [ ] `big_ned`
        - [ ] `big_ltd` (compare with double)
        - [ ] `big_led` (compare with double)
        - [ ] `big_gtd` (compare with double)
        - [ ] `big_ged` (compare with double)
        - [x] `big_is_positive`
        - [x] `big_is_negative`
        - [x] `big_str16_msize`
        - [x] `big_fits_int64`
        - [x] `big_is_null`
- [x] Refactor
    - [x] database -> collection
    - [x] user_new etc -> new_user
    - [x] ti_res_t & ti_root_t  -> ti_query_t
    - [x] watch request only on collections (target to ->collection)
- [x] Watching
    - [x] watch
    - [x] unwatch
- [x] Language
    - [x] Primitives
        - [x] `false`
        - [x] `nil`
        - [x] `true`
        - [x] `float`
        - [x] `int`
        - [x] `string`
        - [x] `regex`
    - [x] Thing
    - [x] Array
    - [x] Functions:
        - [x] `blob`
            - [x] array implementation
            - [ ] ~~Future feature: map implementation~~
        - [x] `del`
        - [x] `endswith`
        - [x] `filter`
        - [x] `find`
        - [x] `get`
        - [x] `hasprop`
        - [x] `id`
        - [x] `int`
        - [x] `isarray`
        - [x] `isascci`
        - [x] `isbool`
        - [x] `isinf`
        - [x] `islist`
        - [x] `isnan`
        - [x] `lower`
        - [x] `map`
        - [x] `match`
        - [x] `now`
        - [x] `push`
        - [x] `rename`
        - [x] `ret`
        - [x] `set`
        - [x] `splice`
        - [x] `startswith`
        - [x] `str`
        - [x] `thing`
        - [x] `try`
        - [x] `unset`
        - [x] `upper`
- [x] Redundancy as initial argument together with --init ?
- [x] Task generating
    - [x] `assign`
    - [x] `del`
    - [x] ~~`push`~~ replaced with splice
    - [x] `rename`
    - [x] `set`
    - [x] `splice`
    - [x] `unset`
- [x] Events without tasks could be saved smaller
- [x] Jobs processing from `EPKG`
    - [x] `assign`
    - [x] `del`
    - [x] ~~`push`~~ replaced with splice
    - [x] `rename`
    - [x] `set`
    - [x] `splice`
    - [x] `unset`
- [x] Value implementation
    - [x] Bool
    - [x] Int
    - [x] Float
    - [x] Nil
    - [x] Thing
    - [x] Array
    - [x] Tuple
    - [x] Things
    - [x] Regex
    - [x] Closure
    - [x] Raw
    - [x] Qp
- [x] Operations
    - [x] `eq`
    - [x] `ge`
    - [x] `gt`
    - [x] `le`
    - [x] `lt`
    - [x] `ne`
    - [x] `add`
    - [x] `sub`
    - [x] `mul`
    - [x] `div`
    - [x] `idiv`
    - [x] `mod`
    - [x] `b_and`
    - [x] `b_xor`
    - [x] `b_or`
    - [x] `(b) ? x : y` conditional operator
- [x] Expressions
    - [x] AND
    - [x] OR
    - [x] Parenthesis
- [x] Keep lowest stored event_id on all nodes
- [x] Implement collection quotas
    - [x] max things
    - [x] max array size
    - [x] max raw size
    - [x] max properties
- [x] Build first event on init
- [x] Storing
    - [x] Full storage on disk
        - [x] Status
        - [x] Nodes
        - [x] Databases
        - [x] Access
        - [x] Things
            - [x] Skeleton
            - [x] Data
            - [x] Attributes
        - [x] Users
    - [x] Archive storing
        - [x] Store in away mode
        - [x] Load on startup (Required Jobs implementation for full coverage)
- [x] Remove *This* node id from `ti_.qp` file and use a separate file because this
      is the only value which is not the same over all nodes.
- [x] Multi node
    - [x] Zone info (so we can first try a node in the same zone for forward queries)
    - [x] Lookup Should not be a singleton so we can create a desired lookup.
    - [x] Each node should have a secret
        - [x] Secrets should be graphical only. (check on argument input and query input)
        - [x] Secrets must be stored (and restored)
        - [x] On --init, the first node should create a *random* secret.
    - [x] Create a request for the ThingsDB setup. (data in `ti_.qp`)
    - [x] Add node
- [x] Root
    - [x] functions
        - [x] `collection`
        - [x] `collections`
        - [x] `counters`
        - [x] `del_collection`
            - [x] Make sure dropped collections will be garbage collected while in away mode
        - [x] `pop_node` --> pop so we do not need to replace node id's
        - [x] `del_user`
        - [x] `grant`
        - [x] `new_collection`
        - [x] `new_node`  --> working on....
        - [x] `new_user`
        - [x] `node`
        - [x] `nodes`
        - [x] `rename_user`
        - [x] `rename_collection`
        - [x] `reset_counters`
        - [x] `revoke`
        - [x] `set_log_level`
        - [x] `set_password`
        - [x] `set_quota`
        - [x] `set_zone`
        - [x] `shutdown`
        - [x] `user`
        - [x] `users`
    - [x] jobs
        - [x] `del_collection`
        - [x] `pop_node`
        - [x] `del_user`
        - [x] `grant`
        - [x] `new_collection`
        - [x] `new_node`  (address:port, secret)
        - [x] `replace_node`
        - [x] `new_user`
        - [x] `revoke`
        - [x] `rename_user`
        - [x] `rename_collection`
        - [x] `set_password`
        - [x] `set_quota`

##

## Get status

```
wget -q -O - http://node.local:8080/status
```

## Special thanks to:

 - [Fast Validate UTF-8](https://github.com/lemire/fastvalidate-utf-8)

## Fonts:

*From this one I like the smaller case and very bold*
https://fonts.google.com/specimen/Work+Sans

*Uppercase is also oke*
https://fonts.google.com/specimen/Encode+Sans+Condensed


*Very simple*
https://fonts.google.com/specimen/Baloo+Thambi

*like the capital b*
https://fonts.google.com/specimen/Fredoka+One