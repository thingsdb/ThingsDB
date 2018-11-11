################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/access.c \
../src/ti/archive.c \
../src/ti/args.c \
../src/ti/arrow.c \
../src/ti/auth.c \
../src/ti/back.c \
../src/ti/cfg.c \
../src/ti/clients.c \
../src/ti/connect.c \
../src/ti/db.c \
../src/ti/dbs.c \
../src/ti/event.c \
../src/ti/events.c \
../src/ti/ex.c \
../src/ti/fetch.c \
../src/ti/fwd.c \
../src/ti/iter.c \
../src/ti/lookup.c \
../src/ti/maint.c \
../src/ti/misc.c \
../src/ti/name.c \
../src/ti/names.c \
../src/ti/node.c \
../src/ti/nodes.c \
../src/ti/pipe.c \
../src/ti/pkg.c \
../src/ti/prom.c \
../src/ti/prop.c \
../src/ti/proto.c \
../src/ti/query.c \
../src/ti/quorum.c \
../src/ti/quota.c \
../src/ti/raw.c \
../src/ti/ref.c \
../src/ti/req.c \
../src/ti/res.c \
../src/ti/root.c \
../src/ti/scope.c \
../src/ti/signals.c \
../src/ti/store.c \
../src/ti/stream.c \
../src/ti/task.c \
../src/ti/tcp.c \
../src/ti/thing.c \
../src/ti/things.c \
../src/ti/user.c \
../src/ti/users.c \
../src/ti/val.c \
../src/ti/version.c \
../src/ti/write.c 

OBJS += \
./src/ti/access.o \
./src/ti/archive.o \
./src/ti/args.o \
./src/ti/arrow.o \
./src/ti/auth.o \
./src/ti/back.o \
./src/ti/cfg.o \
./src/ti/clients.o \
./src/ti/connect.o \
./src/ti/db.o \
./src/ti/dbs.o \
./src/ti/event.o \
./src/ti/events.o \
./src/ti/ex.o \
./src/ti/fetch.o \
./src/ti/fwd.o \
./src/ti/iter.o \
./src/ti/lookup.o \
./src/ti/maint.o \
./src/ti/misc.o \
./src/ti/name.o \
./src/ti/names.o \
./src/ti/node.o \
./src/ti/nodes.o \
./src/ti/pipe.o \
./src/ti/pkg.o \
./src/ti/prom.o \
./src/ti/prop.o \
./src/ti/proto.o \
./src/ti/query.o \
./src/ti/quorum.o \
./src/ti/quota.o \
./src/ti/raw.o \
./src/ti/ref.o \
./src/ti/req.o \
./src/ti/res.o \
./src/ti/root.o \
./src/ti/scope.o \
./src/ti/signals.o \
./src/ti/store.o \
./src/ti/stream.o \
./src/ti/task.o \
./src/ti/tcp.o \
./src/ti/thing.o \
./src/ti/things.o \
./src/ti/user.o \
./src/ti/users.o \
./src/ti/val.o \
./src/ti/version.o \
./src/ti/write.o 

C_DEPS += \
./src/ti/access.d \
./src/ti/archive.d \
./src/ti/args.d \
./src/ti/arrow.d \
./src/ti/auth.d \
./src/ti/back.d \
./src/ti/cfg.d \
./src/ti/clients.d \
./src/ti/connect.d \
./src/ti/db.d \
./src/ti/dbs.d \
./src/ti/event.d \
./src/ti/events.d \
./src/ti/ex.d \
./src/ti/fetch.d \
./src/ti/fwd.d \
./src/ti/iter.d \
./src/ti/lookup.d \
./src/ti/maint.d \
./src/ti/misc.d \
./src/ti/name.d \
./src/ti/names.d \
./src/ti/node.d \
./src/ti/nodes.d \
./src/ti/pipe.d \
./src/ti/pkg.d \
./src/ti/prom.d \
./src/ti/prop.d \
./src/ti/proto.d \
./src/ti/query.d \
./src/ti/quorum.d \
./src/ti/quota.d \
./src/ti/raw.d \
./src/ti/ref.d \
./src/ti/req.d \
./src/ti/res.d \
./src/ti/root.d \
./src/ti/scope.d \
./src/ti/signals.d \
./src/ti/store.d \
./src/ti/stream.d \
./src/ti/task.d \
./src/ti/tcp.d \
./src/ti/thing.d \
./src/ti/things.d \
./src/ti/user.d \
./src/ti/users.d \
./src/ti/val.d \
./src/ti/version.d \
./src/ti/write.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/%.o: ../src/ti/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I../inc -I/usr/include/python3.6m/ -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


