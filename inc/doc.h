/*
 * doc.h
 */
#ifndef DOC_H_
#define DOC_H_

#include <ti/version.h>

#define DOC_DOCS(__uri) \
    "https://docs.thingsdb.net/"TI_VERSION_SYNTAX_STR"/"__uri
#define DOC_SEE(__uri) \
    "; see "DOC_DOCS(__uri)

/* Collection API */
#define DOC_ALT_RAISE               DOC_SEE("collection-api/alt_raise")
#define DOC_ASSERT                  DOC_SEE("collection-api/assert")
#define DOC_BASE64_DECODE           DOC_SEE("collection-api/base64_decode")
#define DOC_BASE64_ENCODE           DOC_SEE("collection-api/base64_encode")
#define DOC_BOOL                    DOC_SEE("collection-api/bool")
#define DOC_BYTES                   DOC_SEE("collection-api/bytes")
#define DOC_CHANGE_ID               DOC_SEE("collection-api/change_id")
#define DOC_CLOSURE                 DOC_SEE("collection-api/closure")
#define DOC_DATETIME                DOC_SEE("collection-api/datetime")
#define DOC_DEEP                    DOC_SEE("collection-api/deep")
#define DOC_DEL_ENUM                DOC_SEE("collection-api/del_enum")
#define DOC_DEL_TYPE                DOC_SEE("collection-api/del_type")
#define DOC_ENUM                    DOC_SEE("collection-api/enum")
#define DOC_ENUM_INFO               DOC_SEE("collection-api/enum_info")
#define DOC_ENUM_MAP                DOC_SEE("collection-api/enum_map")
#define DOC_ENUMS_INFO              DOC_SEE("collection-api/enums_info")
#define DOC_ERR                     DOC_SEE("collection-api/err")
#define DOC_FLOAT                   DOC_SEE("collection-api/float")
#define DOC_FUTURE                  DOC_SEE("collection-api/future")
#define DOC_HAS_ENUM                DOC_SEE("collection-api/has_enum")
#define DOC_HAS_TYPE                DOC_SEE("collection-api/has_type")
#define DOC_INT                     DOC_SEE("collection-api/int")
#define DOC_IS_ARRAY                DOC_SEE("collection-api/is_array")
#define DOC_IS_ASCII                DOC_SEE("collection-api/is_ascii")
#define DOC_IS_BOOL                 DOC_SEE("collection-api/is_bool")
#define DOC_IS_BYTES                DOC_SEE("collection-api/is_bytes")
#define DOC_IS_CLOSURE              DOC_SEE("collection-api/is_closure")
#define DOC_IS_DATETIME             DOC_SEE("collection-api/is_datetime")
#define DOC_IS_ENUM                 DOC_SEE("collection-api/is_enum")
#define DOC_IS_ERR                  DOC_SEE("collection-api/is_err")
#define DOC_IS_FLOAT                DOC_SEE("collection-api/is_float")
#define DOC_IS_FUTURE               DOC_SEE("collection-api/is_future")
#define DOC_IS_INF                  DOC_SEE("collection-api/is_inf")
#define DOC_IS_INT                  DOC_SEE("collection-api/is_int")
#define DOC_IS_LIST                 DOC_SEE("collection-api/is_list")
#define DOC_IS_MPDATA               DOC_SEE("collection-api/is_mpdata")
#define DOC_IS_NAN                  DOC_SEE("collection-api/is_nan")
#define DOC_IS_NIL                  DOC_SEE("collection-api/is_nil")
#define DOC_IS_RAW                  DOC_SEE("collection-api/is_raw")
#define DOC_IS_REGEX                DOC_SEE("collection-api/is_regex")
#define DOC_IS_ROOM                 DOC_SEE("collection-api/is_room")
#define DOC_IS_SET                  DOC_SEE("collection-api/is_set")
#define DOC_IS_STR                  DOC_SEE("collection-api/is_str")
#define DOC_IS_TASK                 DOC_SEE("collection-api/is_task")
#define DOC_IS_THING                DOC_SEE("collection-api/is_thing")
#define DOC_IS_TIME_ZONE            DOC_SEE("collection-api/is_time_zone")
#define DOC_IS_TIMEVAL              DOC_SEE("collection-api/is_timeval")
#define DOC_IS_TUPLE                DOC_SEE("collection-api/is_tuple")
#define DOC_IS_UTF8                 DOC_SEE("collection-api/is_utf8")
#define DOC_JSON_DUMP               DOC_SEE("collection-api/json_dump")
#define DOC_JSON_LOAD               DOC_SEE("collection-api/json_load")
#define DOC_LIST                    DOC_SEE("collection-api/list")
#define DOC_LOG                     DOC_SEE("collection-api/log")
#define DOC_MOD_ENUM                DOC_SEE("collection-api/mod_enum")
#define DOC_MOD_ENUM_ADD            DOC_SEE("collection-api/mod_enum/add")
#define DOC_MOD_ENUM_DEF            DOC_SEE("collection-api/mod_enum/def")
#define DOC_MOD_ENUM_DEL            DOC_SEE("collection-api/mod_enum/del")
#define DOC_MOD_ENUM_MOD            DOC_SEE("collection-api/mod_enum/mod")
#define DOC_MOD_ENUM_REN            DOC_SEE("collection-api/mod_enum/ren")
#define DOC_MOD_TYPE                DOC_SEE("collection-api/mod_type")
#define DOC_MOD_TYPE_ADD            DOC_SEE("collection-api/mod_type/add")
#define DOC_MOD_TYPE_ALL            DOC_SEE("collection-api/mod_type/all")
#define DOC_MOD_TYPE_DEL            DOC_SEE("collection-api/mod_type/del")
#define DOC_MOD_TYPE_HID            DOC_SEE("collection-api/mod_type/hid")
#define DOC_MOD_TYPE_IMP            DOC_SEE("collection-api/mod_type/imp")
#define DOC_MOD_TYPE_MOD            DOC_SEE("collection-api/mod_type/mod")
#define DOC_MOD_TYPE_REL            DOC_SEE("collection-api/mod_type/rel")
#define DOC_MOD_TYPE_REN            DOC_SEE("collection-api/mod_type/ren")
#define DOC_MOD_TYPE_WPO            DOC_SEE("collection-api/mod_type/wpo")
#define DOC_NEW                     DOC_SEE("collection-api/new")
#define DOC_NEW_TYPE                DOC_SEE("collection-api/new_type")
#define DOC_NOW                     DOC_SEE("collection-api/now")
#define DOC_NSE                     DOC_SEE("collection-api/nse")
#define DOC_RAISE                   DOC_SEE("collection-api/raise")
#define DOC_RAND                    DOC_SEE("collection-api/rand")
#define DOC_RANDINT                 DOC_SEE("collection-api/randint")
#define DOC_RANDSTR                 DOC_SEE("collection-api/randstr")
#define DOC_RANGE                   DOC_SEE("collection-api/range")
#define DOC_REFS                    DOC_SEE("collection-api/refs")
#define DOC_REGEX                   DOC_SEE("collection-api/regex")
#define DOC_RENAME_ENUM             DOC_SEE("collection-api/rename_enum")
#define DOC_RENAME_TYPE             DOC_SEE("collection-api/rename_type")
#define DOC_ROOM                    DOC_SEE("collection-api/room")
#define DOC_SET                     DOC_SEE("collection-api/set")
#define DOC_SET_ENUM                DOC_SEE("collection-api/set_enum")
#define DOC_SET_TYPE                DOC_SEE("collection-api/set_type")
#define DOC_STR                     DOC_SEE("collection-api/str")
#define DOC_TASK                    DOC_SEE("collection-api/task")
#define DOC_TASKS                   DOC_SEE("collection-api/tasks")
#define DOC_THING                   DOC_SEE("collection-api/thing")
#define DOC_TIME_ZONES_INFO         DOC_SEE("collection-api/time_zones_info")
#define DOC_TIMEVAL                 DOC_SEE("collection-api/timeval")
#define DOC_TRY                     DOC_SEE("collection-api/try")
#define DOC_TYPE                    DOC_SEE("collection-api/type")
#define DOC_TYPE_ASSERT             DOC_SEE("collection-api/type_assert")
#define DOC_TYPE_COUNT              DOC_SEE("collection-api/type_count")
#define DOC_TYPE_INFO               DOC_SEE("collection-api/type_info")
#define DOC_TYPES_INFO              DOC_SEE("collection-api/types_info")
#define DOC_WSE                     DOC_SEE("collection-api/wse")

/* Node API */
#define DOC_BACKUP_INFO             DOC_SEE("node-api/backup_info")
#define DOC_BACKUPS_INFO            DOC_SEE("node-api/backups_info")
#define DOC_COUNTERS                DOC_SEE("node-api/counters")
#define DOC_DEL_BACKUP              DOC_SEE("node-api/del_backup")
#define DOC_HAS_BACKUP              DOC_SEE("node-api/has_backup")
#define DOC_NEW_BACKUP              DOC_SEE("node-api/new_backup")
#define DOC_NODE_INFO               DOC_SEE("node-api/node_info")
#define DOC_NODES_INFO              DOC_SEE("node-api/nodes_info")
#define DOC_RESET_COUNTERS          DOC_SEE("node-api/reset_counters")
#define DOC_RESTART_MODULE          DOC_SEE("node-api/restart_module")
#define DOC_SET_LOG_LEVEL           DOC_SEE("node-api/set_log_level")
#define DOC_SHUTDOWN                DOC_SEE("node-api/shutdown")

/* ThingsDB API */
#define DOC_COLLECTION_INFO         DOC_SEE("thingsdb-api/collection_info")
#define DOC_COLLECTIONS_INFO        DOC_SEE("thingsdb-api/collections_info")
#define DOC_DEL_COLLECTION          DOC_SEE("thingsdb-api/del_collection")
#define DOC_DEL_EXPIRED             DOC_SEE("thingsdb-api/del_expired")
#define DOC_DEL_MODULE              DOC_SEE("thingsdb-api/del_module")
#define DOC_DEL_NODE                DOC_SEE("thingsdb-api/del_node")
#define DOC_DEL_TOKEN               DOC_SEE("thingsdb-api/del_token")
#define DOC_DEL_USER                DOC_SEE("thingsdb-api/del_user")
#define DOC_DEPLOY_MODULE           DOC_SEE("thingsdb-api/deploy_module")
#define DOC_GRANT                   DOC_SEE("thingsdb-api/grant")
#define DOC_HAS_COLLECTION          DOC_SEE("thingsdb-api/has_collection")
#define DOC_HAS_MODULE              DOC_SEE("thingsdb-api/has_module")
#define DOC_HAS_NODE                DOC_SEE("thingsdb-api/has_node")
#define DOC_HAS_TOKEN               DOC_SEE("thingsdb-api/has_token")
#define DOC_HAS_USER                DOC_SEE("thingsdb-api/has_user")
#define DOC_MODULE_INFO             DOC_SEE("thingsdb-api/module_info")
#define DOC_MODULES_INFO            DOC_SEE("thingsdb-api/modules_info")
#define DOC_NEW_COLLECTION          DOC_SEE("thingsdb-api/new_collection")
#define DOC_NEW_MODULE              DOC_SEE("thingsdb-api/new_module")
#define DOC_NEW_NODE                DOC_SEE("thingsdb-api/new_node")
#define DOC_NEW_TOKEN               DOC_SEE("thingsdb-api/new_token")
#define DOC_NEW_USER                DOC_SEE("thingsdb-api/new_user")
#define DOC_REFRESH_MODULE          DOC_SEE("thingsdb-api/refresh_module")
#define DOC_RENAME_COLLECTION       DOC_SEE("thingsdb-api/rename_collection")
#define DOC_RENAME_MODULE           DOC_SEE("thingsdb-api/rename_module")
#define DOC_RENAME_USER             DOC_SEE("thingsdb-api/rename_user")
#define DOC_RESTORE                 DOC_SEE("thingsdb-api/restore")
#define DOC_REVOKE                  DOC_SEE("thingsdb-api/revoke")
#define DOC_SET_DEFAULT_DEEP        DOC_SEE("thingsdb-api/set_default_deep")
#define DOC_SET_MODULE_CONF         DOC_SEE("thingsdb-api/set_module_conf")
#define DOC_SET_MODULE_SCOPE        DOC_SEE("thingsdb-api/set_module_scope")
#define DOC_SET_PASSWORD            DOC_SEE("thingsdb-api/set_password")
#define DOC_SET_TIME_ZONE           DOC_SEE("thingsdb-api/set_time_zone")
#define DOC_USER_INFO               DOC_SEE("thingsdb-api/user_info")
#define DOC_USERS_INFO              DOC_SEE("thingsdb-api/users_info")

/* Procedures API */
#define DOC_DEL_PROCEDURE           DOC_SEE("procedures-api/del_procedure")
#define DOC_HAS_PROCEDURE           DOC_SEE("procedures-api/has_procedure")
#define DOC_NEW_PROCEDURE           DOC_SEE("procedures-api/new_procedure")
#define DOC_PROCEDURE_DOC           DOC_SEE("procedures-api/procedure_doc")
#define DOC_PROCEDURE_INFO          DOC_SEE("procedures-api/procedure_info")
#define DOC_PROCEDURES_INFO         DOC_SEE("procedures-api/procedures_info")
#define DOC_RENAME_PROCEDURE        DOC_SEE("procedures-api/rename_procedure")
#define DOC_RUN_PROCEDURE           DOC_SEE("procedures-api/run")

/* Data Types */
#define DOC_BYTES_LEN               DOC_SEE("data-types/bytes/len")
#define DOC_CLOSURE_CALL            DOC_SEE("data-types/closure/call")
#define DOC_CLOSURE_DOC             DOC_SEE("data-types/closure/doc")
#define DOC_DATETIME_EXTRACT        DOC_SEE("data-types/datetime/extract")
#define DOC_DATETIME_FORMAT         DOC_SEE("data-types/datetime/format")
#define DOC_DATETIME_MOVE           DOC_SEE("data-types/datetime/move")
#define DOC_DATETIME_REPLACE        DOC_SEE("data-types/datetime/replace")
#define DOC_DATETIME_TO             DOC_SEE("data-types/datetime/to")
#define DOC_DATETIME_WEEK           DOC_SEE("data-types/datetime/week")
#define DOC_DATETIME_WEEKDAY        DOC_SEE("data-types/datetime/weekday")
#define DOC_DATETIME_YDAY           DOC_SEE("data-types/datetime/yday")
#define DOC_DATETIME_ZONE           DOC_SEE("data-types/datetime/zone")
#define DOC_ENUM_NAME               DOC_SEE("data-types/enum/name")
#define DOC_ENUM_VALUE              DOC_SEE("data-types/enum/value")
#define DOC_ERROR_CODE              DOC_SEE("data-types/error/code")
#define DOC_ERROR_MSG               DOC_SEE("data-types/error/msg")
#define DOC_FUTURE_ELSE             DOC_SEE("data-types/future/else")
#define DOC_FUTURE_THEN             DOC_SEE("data-types/future/then")
#define DOC_LIST_CHOICE             DOC_SEE("data-types/list/choice")
#define DOC_LIST_CLEAR              DOC_SEE("data-types/list/clear")
#define DOC_LIST_COPY               DOC_SEE("data-types/list/copy")
#define DOC_LIST_DUP                DOC_SEE("data-types/list/dup")
#define DOC_LIST_EACH               DOC_SEE("data-types/list/each")
#define DOC_LIST_EVERY              DOC_SEE("data-types/list/every")
#define DOC_LIST_EXTEND             DOC_SEE("data-types/list/extend")
#define DOC_LIST_EXTEND_UNIQUE      DOC_SEE("data-types/list/extend_unique")
#define DOC_LIST_FILL               DOC_SEE("data-types/list/fill")
#define DOC_LIST_FILTER             DOC_SEE("data-types/list/filter")
#define DOC_LIST_FIND               DOC_SEE("data-types/list/find")
#define DOC_LIST_FIND_INDEX         DOC_SEE("data-types/list/find_index")
#define DOC_LIST_FIRST              DOC_SEE("data-types/list/first")
#define DOC_LIST_HAS                DOC_SEE("data-types/list/has")
#define DOC_LIST_INDEX_OF           DOC_SEE("data-types/list/index_of")
#define DOC_LIST_IS_UNIQUE          DOC_SEE("data-types/list/is_unique")
#define DOC_LIST_JOIN               DOC_SEE("data-types/list/join")
#define DOC_LIST_LAST               DOC_SEE("data-types/list/last")
#define DOC_LIST_LEN                DOC_SEE("data-types/list/len")
#define DOC_LIST_MAP                DOC_SEE("data-types/list/map")
#define DOC_LIST_MAP_ID             DOC_SEE("data-types/list/map_id")
#define DOC_LIST_MAP_TYPE           DOC_SEE("data-types/list/map_type")
#define DOC_LIST_MAP_WRAP           DOC_SEE("data-types/list/map_wrap")
#define DOC_LIST_POP                DOC_SEE("data-types/list/pop")
#define DOC_LIST_PUSH               DOC_SEE("data-types/list/push")
#define DOC_LIST_REDUCE             DOC_SEE("data-types/list/reduce")
#define DOC_LIST_REMOVE             DOC_SEE("data-types/list/remove")
#define DOC_LIST_RESTRICTION        DOC_SEE("data-types/list/restriction")
#define DOC_LIST_REVERSE            DOC_SEE("data-types/list/reverse")
#define DOC_LIST_SHIFT              DOC_SEE("data-types/list/shift")
#define DOC_LIST_SOME               DOC_SEE("data-types/list/some")
#define DOC_LIST_SORT               DOC_SEE("data-types/list/sort")
#define DOC_LIST_SPLICE             DOC_SEE("data-types/list/splice")
#define DOC_LIST_UNIQUE             DOC_SEE("data-types/list/unique")
#define DOC_LIST_UNSHIFT            DOC_SEE("data-types/list/unshift")
#define DOC_MPDATA_LEN              DOC_SEE("data-types/mpdata/len")
#define DOC_MPDATA_LOAD             DOC_SEE("data-types/mpdata/load")
#define DOC_REGEX_TEST              DOC_SEE("data-types/regex/test")
#define DOC_ROOM_EMIT               DOC_SEE("data-types/room/emit")
#define DOC_ROOM_ID                 DOC_SEE("data-types/room/id")
#define DOC_SET_ADD                 DOC_SEE("data-types/set/add")
#define DOC_SET_CLEAR               DOC_SEE("data-types/set/clear")
#define DOC_SET_COPY                DOC_SEE("data-types/set/copy")
#define DOC_SET_DUP                 DOC_SEE("data-types/set/dup")
#define DOC_SET_EACH                DOC_SEE("data-types/set/each")
#define DOC_SET_EVERY               DOC_SEE("data-types/set/every")
#define DOC_SET_FILTER              DOC_SEE("data-types/set/filter")
#define DOC_SET_FIND                DOC_SEE("data-types/set/find")
#define DOC_SET_HAS                 DOC_SEE("data-types/set/has")
#define DOC_SET_MAP_ID              DOC_SEE("data-types/set/map_id")
#define DOC_SET_MAP_WRAP            DOC_SEE("data-types/set/map_wrap")
#define DOC_SET_LEN                 DOC_SEE("data-types/set/len")
#define DOC_SET_MAP                 DOC_SEE("data-types/set/map")
#define DOC_SET_ONE                 DOC_SEE("data-types/set/one")
#define DOC_SET_REDUCE              DOC_SEE("data-types/set/reduce")
#define DOC_SET_REMOVE              DOC_SEE("data-types/set/remove")
#define DOC_SET_RESTRICTION         DOC_SEE("data-types/set/restriction")
#define DOC_SET_SOME                DOC_SEE("data-types/set/some")
#define DOC_STR_CONTAINS            DOC_SEE("data-types/str/contains")
#define DOC_STR_ENDS_WITH           DOC_SEE("data-types/str/ends_with")
#define DOC_STR_LEN                 DOC_SEE("data-types/str/len")
#define DOC_STR_LOWER               DOC_SEE("data-types/str/lower")
#define DOC_STR_REPLACE             DOC_SEE("data-types/str/replace")
#define DOC_STR_SPLIT               DOC_SEE("data-types/str/split")
#define DOC_STR_STARTS_WITH         DOC_SEE("data-types/str/starts_with")
#define DOC_STR_TRIM                DOC_SEE("data-types/str/trim")
#define DOC_STR_TRIM_LEFT           DOC_SEE("data-types/str/trim_left")
#define DOC_STR_TRIM_RIGHT          DOC_SEE("data-types/str/trim_right")
#define DOC_STR_UPPER               DOC_SEE("data-types/str/upper")
#define DOC_TASK_AGAIN_AT           DOC_SEE("data-types/task/again_at")
#define DOC_TASK_AGAIN_IN           DOC_SEE("data-types/task/again_in")
#define DOC_TASK_ARGS               DOC_SEE("data-types/task/args")
#define DOC_TASK_AT                 DOC_SEE("data-types/task/at")
#define DOC_TASK_CANCEL             DOC_SEE("data-types/task/cancel")
#define DOC_TASK_CLOSURE            DOC_SEE("data-types/task/closure")
#define DOC_TASK_DEL                DOC_SEE("data-types/task/del")
#define DOC_TASK_ERR                DOC_SEE("data-types/task/err")
#define DOC_TASK_ID                 DOC_SEE("data-types/task/id")
#define DOC_TASK_OWNER              DOC_SEE("data-types/task/owner")
#define DOC_TASK_SET_ARGS           DOC_SEE("data-types/task/set_args")
#define DOC_TASK_SET_CLOSURE        DOC_SEE("data-types/task/set_closure")
#define DOC_TASK_SET_OWNER          DOC_SEE("data-types/task/set_owner")
#define DOC_THING_ASSIGN            DOC_SEE("data-types/thing/assign")
#define DOC_THING_CLEAR             DOC_SEE("data-types/thing/clear")
#define DOC_THING_COPY              DOC_SEE("data-types/thing/copy")
#define DOC_THING_DEL               DOC_SEE("data-types/thing/del")
#define DOC_THING_DUP               DOC_SEE("data-types/thing/dup")
#define DOC_THING_EACH              DOC_SEE("data-types/thing/each")
#define DOC_THING_EQUALS            DOC_SEE("data-types/thing/equals")
#define DOC_THING_FILTER            DOC_SEE("data-types/thing/filter")
#define DOC_THING_GET               DOC_SEE("data-types/thing/get")
#define DOC_THING_HAS               DOC_SEE("data-types/thing/has")
#define DOC_THING_ID                DOC_SEE("data-types/thing/id")
#define DOC_THING_KEYS              DOC_SEE("data-types/thing/keys")
#define DOC_THING_LEN               DOC_SEE("data-types/thing/len")
#define DOC_THING_MAP               DOC_SEE("data-types/thing/map")
#define DOC_THING_REMOVE            DOC_SEE("data-types/thing/remove")
#define DOC_THING_REN               DOC_SEE("data-types/thing/ren")
#define DOC_THING_RESTRICT          DOC_SEE("data-types/thing/restrict")
#define DOC_THING_RESTRICTION       DOC_SEE("data-types/thing/restriction")
#define DOC_THING_SEARCH            DOC_SEE("data-types/thing/search")
#define DOC_THING_SET               DOC_SEE("data-types/thing/set")
#define DOC_THING_TO_TYPE           DOC_SEE("data-types/thing/to_type")
#define DOC_THING_VALUES            DOC_SEE("data-types/thing/values")
#define DOC_THING_VMAP              DOC_SEE("data-types/thing/vmap")
#define DOC_THING_WRAP              DOC_SEE("data-types/thing/wrap")
#define DOC_TYPED_TO_THING          DOC_SEE("data-types/typed/to_thing")
#define DOC_WTYPE_COPY              DOC_SEE("data-types/wtype/copy")
#define DOC_WTYPE_DUP               DOC_SEE("data-types/wtype/dup")
#define DOC_WTYPE_ID                DOC_SEE("data-types/wtype/id")
#define DOC_WTYPE_UNWRAP            DOC_SEE("data-types/wtype/unwrap")

/* Errors */
#define DOC_ASSERT_ERR              DOC_SEE("errors/assert_err")
#define DOC_AUTH_ERR                DOC_SEE("errors/auth_err")
#define DOC_BAD_DATA_ERR            DOC_SEE("errors/bad_data_err")
#define DOC_CANCELLED_ERR           DOC_SEE("errors/cancelled_err")
#define DOC_FORBIDDEN_ERR           DOC_SEE("errors/forbidden_err")
#define DOC_LOOKUP_ERR              DOC_SEE("errors/lookup_err")
#define DOC_MAX_QUOTA_ERR           DOC_SEE("errors/max_quota_err")
#define DOC_NODE_ERR                DOC_SEE("errors/node_err")
#define DOC_NUM_ARGUMENTS_ERR       DOC_SEE("errors/num_arguments_err")
#define DOC_OPERATION_ERR           DOC_SEE("errors/operation_err")
#define DOC_OVERFLOW_ERR            DOC_SEE("errors/overflow_err")
#define DOC_SYNTAX_ERR              DOC_SEE("errors/syntax_err")
#define DOC_TYPE_ERR                DOC_SEE("errors/type_err")
#define DOC_VALUE_ERR               DOC_SEE("errors/value_err")
#define DOC_ZERO_DIV_ERR            DOC_SEE("errors/zero_div_err")

/* Other */
#define DOC_NAMES                   DOC_SEE("overview/names")
#define DOC_PROPERTIES              DOC_SEE("overview/properties")
#define DOC_SCOPES                  DOC_SEE("overview/scopes")
#define DOC_SLICES                  DOC_SEE("overview/slices")
#define DOC_SOCKET_QUERY            DOC_SEE("connect/socket/query")
#define DOC_T_TYPED                 DOC_SEE("data-types/typed")
#define DOC_T_TYPE                  DOC_SEE("overview/type")
#define DOC_T_ENUM                  DOC_SEE("data-types/enum")
#define DOC_LISTENING               DOC_SEE("listening")
#define DOC_MODULES                 DOC_SEE("modules")

/* No functions */
#define DOC_PROCEDURES_API          DOC_SEE("procedures-api")
#define DOC_TYPE_CLOSURE            DOC_SEE("data-types/closure")
#define DOC_HTTP_API                DOC_SEE("connect/http-api")
#define DOC_CONFIGURATION           DOC_SEE("getting-started/configuration")

#endif  /* TI_DOC_H_ */
