typedef struct ti_type_s ti_type_t;
typedef struct ti_field_s ti_field_t;
typedef struct ti_object_s ti_object_t;


struct ti_object_s
{
    uint32_t ref;
    uint8_t tp;             /* thing or instance */
    uint8_t flags;
    uint16_t _pad16;

    uint64_t id;
    imap_t * objects;       /* instance is added to this map */
    vec_t * items;          /* ti_prop_t for things, ti_val_t for instances */
    vec_t * watchers;       /* vec contains ti_watch_t,
                               NULL if no watchers,  */
};

struct ti_instance_s
{
    uint32_t ref;
    uint8_t tp;             /* instance */
    uint8_t flags;
    uint16_t class;         /* type id */

    uint64_t id;
    imap_t * objects;       /* instance is added to this map */
    vec_t * values;         /* vec contains ti_val_t */
    vec_t * watchers;       /* vec contains ti_watch_t,
                               NULL if no watchers,  */
};

struct ti_wrap_s
{
    uint32_t ref;
    uint8_t tp;             /* CAST */
    uint8_t flags;
    uint16_t kind;          /* to kind id */

    ti_object_t * object;
};

struct ti_field_s
{
    ti_name_t * name;
    uint16_t sid;
    char * spec;
};

struct ti_kind_s
{
    uint16_t kind;          /* kind id */
    vec_t * requires;       /* ti_struct_t */
    vec_t * fields;         /* ti_field_t */
};

struct ti_kind_s
{
    imap_t * kinds;
};

/*

// 'str'   -> do utf8 check
// 'raw'   -> like str, without utf8 check
// 'int'   -> integer value
// 'float' -> float value
// 'bool'  -> bool value
// 'thing' -> thing
// '[]'    -> array
// '{}'    -> set
// '[...]`  -> array of ...
// '{...}`  -> set of ...


define('User', {});


kind_info('Channel');
new_kind('Channel');

del_kind('Channel');  // marks as deleted immediately,
                         convert to things in away mode





define('Channel', {
    name: 'str',          // defaults to ""
    description: 'str?',  // defaults to nil
    parent: 'Channel?',   // self references must be optional or inside an array
    users: '[User]',
});

define('UserName', {
    name: 'str'
})

define('User', {
    name: 'str',
    age: 'int'
    channel: 'Channel'
});

define('ChannelCompact' {
    name: 'str',
    users: '[UserName]'
})

// event `define` is only handled by the nodes, so for updating watchers
// we should generate `set` events and write them immediately while processing
// the change. A difference is that we only require the set events when there
// are actually watchers

channel = new('Channel', {})
            -->
            {
                #: 123
                name: "",
                description: nil,
                parent: nil,
                users: [],
            }

channel.x = 1;          // error, type `Channel` has no property `x`
channel.name = 1;       // error, type `Channel` expects `raw`...

user = new('User', {})
user.name = 'Iris';

as_channel = wrap('Channel', user);   // create short syntax <Channel>user;
as_channel.description;    //  return default, nil? or error? and what in return?

as_channel.name = 'xxx';    // error, cannot assign to <Channel>

type(as_channel);           // <Channel>
type(user);                 // User

user = unwrap(as_channel);  // returns User //


// what should a double cast do? wrap('X', wrap('Y', value)); ?
// replace the wrap or allow double wrapping? If returning default

// internally we can pack a wrap as {'&': kind, object: ...} but we expose
// the wrapped object as it is cast.

*/
