# ThingsDB

## TODO list

- [ ] 64Bit Requirements, support 64 bit only?? else change this following
    - [ ] wareq uses uinpntr_t for 64bit thing id's
    - [ ] if we do require 64bit, then we can take advantage
- [ ] Special floats
    - [ ] Inf
    - [ ] NaN
    - [ ] (negative) Inf
- [ ] We can make indexing by scope instead of fixed integer, but allowing only
      fixed integers makes it a bit faster and easier
- [ ] Overflow handling? Right now ThingsDB is naive
- [x] Refactor
    - [x] database -> collection
    - [x] user_new etc -> new_user
    - [x] ti_res_t & ti_root_t  -> ti_query_t
    - [x] watch request only on collections (target to ->collection)
- [ ] Language
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
    - [ ] Functions:
        - [x] `blob`
            - [x] array implementation
            - [*] Future feature: map implementation
        - [x] `endswith`
        - [x] `filter`
        - [x] `get`
        - [x] `id`
        - [x] `lower`
        - [x] `map`
        - [x] `match`
        - [x] `now`
        - [x] `ret`
        - [x] `startswith`
        - [x] `thing`
        - [x] `upper`
        - [x] `del`
        - [x] `push`
        - [x] `rename`
        - [x] `set`
        - [ ] `splice`
        - [x] `unset`
- [x] Redundancy as initial argument together with --init ?
- [ ] Task generating
    - [x] `assign`
    - [x] `del`
    - [x] `push`
    - [x] `rename`
    - [x] `set`
    - [ ] `splice`
    - [x] `unset`
- [x] Events without tasks could be saved smaller
- [ ] Jobs processing from `EPKG`
    - [x] `assign`
    - [x] `del`
    - [x] `push`
    - [x] `rename`
    - [x] `set`
    - [ ] `splice`
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
    - [x] Arrow
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
- [x] Expressions
    - [x] AND
    - [x] OR
    - [x] Parenthesis
- [ ] Keep sevid ?? (lowest stored event_id on all nodes)
- [x] Build first event on init
- [x] Storing
    - [x] Full storage on disk
        - [x] Status
        - [x] Nodes
        - [x] Databases
        - [x] Access
        - [ ] Things
            - [x] Skeleton
            - [x] Data
            - [x] Attributes
        - [x] Users
    - [x] Archive storing
        - [x] Store in away mode
        - [x] Load on startup (Required Jobs implementation for full coverage)
- [ ] Multi node
    - [ ] Design flow
    - [ ] Lookup Should not be a singleton so we can create a desired lookup.
    - [ ] Add node
        - [ ] In cq -> parse address:port and secret
        - [ ] In Node, connect to new node
            - [ ] Protocol send node_id and secret (using crypt?)
                - Will it work if we send a node_id to replace an existing
                  node? In this case we can keep the argument --init and do
                  not need an extra argument for replacing an existing node
            - [ ] Send all nodes to new node
                - [ ] On callback -> Add the node and register a task for the
                      other nodes. Now all nodes can connect to the new node.
                      The new node should not set-up connections.
                - [ ] On shutdown and restart, the event makes sure the node is
                      still registered.
                - [ ] nope, stupid idea --> What if we store the 'known' nodes length, and
                      only after finished expanding update the length for the
                      lookup?
                - [ ] I think this works --> What if we store node flags (SYNCHRONIZING) ? But also
                      keep the status SYNCHRONIZING which is based on the flag.
                      The status can be used for communicating, and the flag for
                      hard storage. ( we only need the flag on the 'new' node) -->
                      not true, we should store the flag on all nodes, and also add
                      a dropped flag for scaling down. Scaling down is then possible
                      in case we scale down by one node at a time. I think we
                      should only allow to pop the last node, since otherwise
                      we need to change node id's to change the place of the
                      removed node.

                - [ ] What if we add another node while synchronizing is not
                      finished yet? As long as the redundancy is not changed,
                      it should work since the new nodes only can request too
                      much attributes which later can be removed.
                      (but never too little, and this is not the case if the
                      redundancy setting changes)
                      - Solution to support change of redundancy: say we have state A,
                        and create a desired state B and keep
                        attributes for both state A and B. If state B is replaced with
                        state C, then we can forget about state B, and continue saving
                        state A and C. As soon as the higher state is finished we can
                        forget about state A.



            - [ ] Connect to all node
            - [ ] Search away mode node
            - [ ] Request everything stored except attributes
            - [ ] Request archived events
            - [ ]
        - [ ] In nodes -> ti_nodes_new_node function
- [ ] Root
    - [ ] functions
        - [ ] `collection`
        - [ ] `collections`
        - [ ] `del_collection`
        - [ ] `pop_node` --> pop so we do not need to replace node id's
        - [x] `del_user`
        - [x] `grant`
        - [x] `new_collection`
        - [ ] `new_node`
        - [x] `new_user`
        - [ ] `node`
        - [ ] `nodes`
        - [ ] `reset_counters`
        - [x] `revoke`
        - [x] `user`
        - [x] `users`
    - [ ] jobs
        - [ ] `del_collection`
        - [ ] `pop_node` ?? --> pop so we do not need to replace node id's
        - [x] `del_user`
        - [x] `grant`
        - [x] `new_collection`
        - [ ] `new_node`
        - [x] `new_user`
        - [x] `revoke`

