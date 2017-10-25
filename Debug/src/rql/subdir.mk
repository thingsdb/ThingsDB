################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/rql/access.c \
../src/rql/archive.c \
../src/rql/args.c \
../src/rql/auth.c \
../src/rql/back.c \
../src/rql/cfg.c \
../src/rql/db.c \
../src/rql/dbs.c \
../src/rql/elem.c \
../src/rql/elems.c \
../src/rql/event.c \
../src/rql/events.c \
../src/rql/front.c \
../src/rql/item.c \
../src/rql/lookup.c \
../src/rql/maint.c \
../src/rql/misc.c \
../src/rql/node.c \
../src/rql/nodes.c \
../src/rql/pkg.c \
../src/rql/prom.c \
../src/rql/prop.c \
../src/rql/props.c \
../src/rql/raw.c \
../src/rql/ref.c \
../src/rql/req.c \
../src/rql/rql.c \
../src/rql/signals.c \
../src/rql/sock.c \
../src/rql/store.c \
../src/rql/task.c \
../src/rql/user.c \
../src/rql/users.c \
../src/rql/val.c \
../src/rql/version.c \
../src/rql/write.c 

OBJS += \
./src/rql/access.o \
./src/rql/archive.o \
./src/rql/args.o \
./src/rql/auth.o \
./src/rql/back.o \
./src/rql/cfg.o \
./src/rql/db.o \
./src/rql/dbs.o \
./src/rql/elem.o \
./src/rql/elems.o \
./src/rql/event.o \
./src/rql/events.o \
./src/rql/front.o \
./src/rql/item.o \
./src/rql/lookup.o \
./src/rql/maint.o \
./src/rql/misc.o \
./src/rql/node.o \
./src/rql/nodes.o \
./src/rql/pkg.o \
./src/rql/prom.o \
./src/rql/prop.o \
./src/rql/props.o \
./src/rql/raw.o \
./src/rql/ref.o \
./src/rql/req.o \
./src/rql/rql.o \
./src/rql/signals.o \
./src/rql/sock.o \
./src/rql/store.o \
./src/rql/task.o \
./src/rql/user.o \
./src/rql/users.o \
./src/rql/val.o \
./src/rql/version.o \
./src/rql/write.o 

C_DEPS += \
./src/rql/access.d \
./src/rql/archive.d \
./src/rql/args.d \
./src/rql/auth.d \
./src/rql/back.d \
./src/rql/cfg.d \
./src/rql/db.d \
./src/rql/dbs.d \
./src/rql/elem.d \
./src/rql/elems.d \
./src/rql/event.d \
./src/rql/events.d \
./src/rql/front.d \
./src/rql/item.d \
./src/rql/lookup.d \
./src/rql/maint.d \
./src/rql/misc.d \
./src/rql/node.d \
./src/rql/nodes.d \
./src/rql/pkg.d \
./src/rql/prom.d \
./src/rql/prop.d \
./src/rql/props.d \
./src/rql/raw.d \
./src/rql/ref.d \
./src/rql/req.d \
./src/rql/rql.d \
./src/rql/signals.d \
./src/rql/sock.d \
./src/rql/store.d \
./src/rql/task.d \
./src/rql/user.d \
./src/rql/users.d \
./src/rql/val.d \
./src/rql/version.d \
./src/rql/write.d 


# Each subdirectory must supply rules for building sources it contributes
src/rql/%.o: ../src/rql/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DDEBUG -I../inc -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


