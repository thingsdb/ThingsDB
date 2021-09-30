/*
 * util/osarch.c
 */
#include <string.h>
#include <sys/utsname.h>
#include <util/strx.h>

/*

Known by Go:

aix/ppc64        freebsd/amd64   linux/mipsle   openbsd/386
android/386      freebsd/arm     linux/ppc64    openbsd/amd64
android/amd64    illumos/amd64   linux/ppc64le  openbsd/arm
android/arm      js/wasm         linux/s390x    openbsd/arm64
android/arm64    linux/386       nacl/386       plan9/386
darwin/386       linux/amd64     nacl/amd64p32  plan9/amd64
darwin/amd64     linux/arm       nacl/arm       plan9/arm
darwin/arm       linux/arm64     netbsd/386     solaris/amd64
darwin/arm64     linux/mips      netbsd/amd64   windows/386
dragonfly/amd64  linux/mips64    netbsd/arm     windows/amd64
freebsd/386      linux/mips64le  netbsd/arm64   windows/arm

*/

static char osarch__this[130];
static struct utsname osarch__uinfo;

void osarch_init(void)
{

    (void) uname(&osarch__uinfo);

    strx_lower_case(osarch__uinfo.sysname);
    strx_lower_case(osarch__uinfo.machine);

    if (strcmp(osarch__uinfo.machine, "x86_64") == 0 ||
        strcmp(osarch__uinfo.machine, "x64") == 0)
        strcpy(osarch__uinfo.machine, "amd64");
    else if (strcmp(osarch__uinfo.machine, "i386") == 0 ||
        strcmp(osarch__uinfo.machine, "i686") == 0)
        strcpy(osarch__uinfo.machine, "386");

    strcpy(osarch__this, osarch__uinfo.sysname);
    strcat(osarch__this, "/");
    strcat(osarch__this, osarch__uinfo.machine);
}

const char * osarch_get_os(void)
{
    return osarch__uinfo.sysname;
}

const char * osarch_get_arch(void)
{
    return osarch__uinfo.machine;
}

const char * osarch_get(void)
{
    return osarch__this;
}
