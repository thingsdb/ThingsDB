################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/tin/access.c \
../src/tin/archive.c \
../src/tin/args.c \
../src/tin/auth.c \
../src/tin/back.c \
../src/tin/cfg.c \
../src/tin/db.c \
../src/tin/dbs.c \
../src/tin/event.c \
../src/tin/events.c \
../src/tin/front.c \
../src/tin/item.c \
../src/tin/lookup.c \
../src/tin/maint.c \
../src/tin/misc.c \
../src/tin/node.c \
../src/tin/nodes.c \
../src/tin/pkg.c \
../src/tin/prom.c \
../src/tin/prop.c \
../src/tin/props.c \
../src/tin/raw.c \
../src/tin/ref.c \
../src/tin/req.c \
../src/tin/signals.c \
../src/tin/sock.c \
../src/tin/store.c \
../src/tin/task.c \
../src/tin/thing.c \
../src/tin/things.c \
../src/tin/user.c \
../src/tin/users.c \
../src/tin/val.c \
../src/tin/version.c \
../src/tin/write.c 

OBJS += \
./src/tin/access.o \
./src/tin/archive.o \
./src/tin/args.o \
./src/tin/auth.o \
./src/tin/back.o \
./src/tin/cfg.o \
./src/tin/db.o \
./src/tin/dbs.o \
./src/tin/event.o \
./src/tin/events.o \
./src/tin/front.o \
./src/tin/item.o \
./src/tin/lookup.o \
./src/tin/maint.o \
./src/tin/misc.o \
./src/tin/node.o \
./src/tin/nodes.o \
./src/tin/pkg.o \
./src/tin/prom.o \
./src/tin/prop.o \
./src/tin/props.o \
./src/tin/raw.o \
./src/tin/ref.o \
./src/tin/req.o \
./src/tin/signals.o \
./src/tin/sock.o \
./src/tin/store.o \
./src/tin/task.o \
./src/tin/thing.o \
./src/tin/things.o \
./src/tin/user.o \
./src/tin/users.o \
./src/tin/val.o \
./src/tin/version.o \
./src/tin/write.o 

C_DEPS += \
./src/tin/access.d \
./src/tin/archive.d \
./src/tin/args.d \
./src/tin/auth.d \
./src/tin/back.d \
./src/tin/cfg.d \
./src/tin/db.d \
./src/tin/dbs.d \
./src/tin/event.d \
./src/tin/events.d \
./src/tin/front.d \
./src/tin/item.d \
./src/tin/lookup.d \
./src/tin/maint.d \
./src/tin/misc.d \
./src/tin/node.d \
./src/tin/nodes.d \
./src/tin/pkg.d \
./src/tin/prom.d \
./src/tin/prop.d \
./src/tin/props.d \
./src/tin/raw.d \
./src/tin/ref.d \
./src/tin/req.d \
./src/tin/signals.d \
./src/tin/sock.d \
./src/tin/store.d \
./src/tin/task.d \
./src/tin/thing.d \
./src/tin/things.d \
./src/tin/user.d \
./src/tin/users.d \
./src/tin/val.d \
./src/tin/version.d \
./src/tin/write.d 


# Each subdirectory must supply rules for building sources it contributes
src/tin/%.o: ../src/tin/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DDEBUG -I../inc -I/usr/include/python3.6m/ -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


