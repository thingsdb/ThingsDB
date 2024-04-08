project(thingsdb C)
cmake_minimum_required(VERSION 3.5)
include_directories(inc)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG)
endif()

add_compile_definitions(
    LWS_WITH_LIBUV=1
    LWS_WITH_PLUGINS=1
)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finline-limit=4000")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   include_directories(
        AFTER
        /opt/homebrew/opt/libuv/include
        /opt/homebrew/opt/pcre2/include
        /opt/homebrew/opt/yajl/include
        /opt/homebrew/opt/curl/include
    )
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

link_libraries(
    uv
    pcre2-8
    m
    yajl
    curl
    websockets
)

set(SOURCES
    main.c
    src/ex.c
    src/ti.c
    src/langdef/compat.c
    src/langdef/langdef.c
    src/langdef/translate.c
    src/cleri/choice.c
    src/cleri/cleri.c
    src/cleri/dup.c
    src/cleri/expecting.c
    src/cleri/grammar.c
    src/cleri/keyword.c
    src/cleri/kwcache.c
    src/cleri/list.c
    src/cleri/node.c
    src/cleri/olist.c
    src/cleri/optional.c
    src/cleri/parse.c
    src/cleri/prio.c
    src/cleri/ref.c
    src/cleri/regex.c
    src/cleri/repeat.c
    src/cleri/rule.c
    src/cleri/sequence.c
    src/cleri/this.c
    src/cleri/token.c
    src/cleri/tokens.c
    src/cleri/version.c
    src/ti/access.c
    src/ti/api.c
    src/ti/archfile.c
    src/ti/archive.c
    src/ti/args.c
    src/ti/async.c
    src/ti/auth.c
    src/ti/away.c
    src/ti/backup.c
    src/ti/backups.c
    src/ti/build.c
    src/ti/cfg.c
    src/ti/change.c
    src/ti/changes.c
    src/ti/clients.c
    src/ti/closure.c
    src/ti/collection.c
    src/ti/collections.c
    src/ti/condition.c
    src/ti/connect.c
    src/ti/counters.c
    src/ti/cpkg.c
    src/ti/ctask.c
    src/ti/datetime.c
    src/ti/deep.c
    src/ti/do.c
    src/ti/dump.c
    src/ti/enum.c
    src/ti/enums.c
    src/ti/evars.c
    src/ti/export.c
    src/ti/field.c
    src/ti/flags.c
    src/ti/fmt.c
    src/ti/fn.c
    src/ti/forloop.c
    src/ti/future.c
    src/ti/fwd.c
    src/ti/gc.c
    src/ti/index.c
    src/ti/item.c
    src/ti/mapping.c
    src/ti/member.c
    src/ti/method.c
    src/ti/module.c
    src/ti/modules.c
    src/ti/name.c
    src/ti/names.c
    src/ti/ncache.c
    src/ti/nil.c
    src/ti/node.c
    src/ti/nodes.c
    src/ti/opr.c
    src/ti/pipe.c
    src/ti/pkg.c
    src/ti/preopr.c
    src/ti/proc.c
    src/ti/procedure.c
    src/ti/procedures.c
    src/ti/prop.c
    src/ti/proto.c
    src/ti/qbind.c
    src/ti/qcache.c
    src/ti/query.c
    src/ti/quorum.c
    src/ti/raw.c
    src/ti/regex.c
    src/ti/req.c
    src/ti/restore.c
    src/ti/room.c
    src/ti/rooms.c
    src/ti/rpkg.c
    src/ti/scope.c
    src/ti/signals.c
    src/ti/spec.c
    src/ti/store.c
    src/ti/stream.c
    src/ti/syncarchive.c
    src/ti/sync.c
    src/ti/syncer.c
    src/ti/syncevents.c
    src/ti/syncfull.c
    src/ti/task.c
    src/ti/tasks.c
    src/ti/tcp.c
    src/ti/template.c
    src/ti/thing.c
    src/ti/things.c
    src/ti/token.c
    src/ti/ttask.c
    src/ti/type.c
    src/ti/types.c
    src/ti/tz.c
    src/ti/user.c
    src/ti/users.c
    src/ti/val.c
    src/ti/varr.c
    src/ti/vbool.c
    src/ti/verror.c
    src/ti/version.c
    src/ti/vfloat.c
    src/ti/vint.c
    src/ti/vset.c
    src/ti/vtask.c
    src/ti/warn.c
    src/ti/watch.c
    src/ti/web.c
    src/ti/wrap.c
    src/ti/write.c
    src/ti/ws.c
    src/ti/mod/expose.c
    src/ti/mod/github.c
    src/ti/mod/manifest.c
    src/ti/mod/work.c
    src/ti/store/storeaccess.c
    src/ti/store/storecollection.c
    src/ti/store/storecollections.c
    src/ti/store/storeenums.c
    src/ti/store/storegcollect.c
    src/ti/store/storemodules.c
    src/ti/store/storenames.c
    src/ti/store/storeprocedures.c
    src/ti/store/storestatus.c
    src/ti/store/storetasks.c
    src/ti/store/storethings.c
    src/ti/store/storetypes.c
    src/ti/store/storeusers.c
    src/util/argparse.c
    src/util/buf.c
    src/util/cfgparser.c
    src/util/cryptx.c
    src/util/fx.c
    src/util/guid.c
    src/util/imap.c
    src/util/iso8601.c
    src/util/link.c
    src/util/lock.c
    src/util/logger.c
    src/util/olist.c
    src/util/omap.c
    src/util/osarch.c
    src/util/queue.c
    src/util/rbuf.c
    src/util/smap.c
    src/util/strx.c
    src/util/syncpart.c
    src/util/util.c
    src/util/vec.c
    inc/lib/http_parser.c
)

add_executable(thingsdb ${SOURCES})
