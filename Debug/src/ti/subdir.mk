################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/access.c \
../src/ti/archive.c \
../src/ti/args.c \
../src/ti/auth.c \
../src/ti/back.c \
../src/ti/cfg.c \
../src/ti/db.c \
../src/ti/event.c \
../src/ti/front.c \
../src/ti/item.c \
../src/ti/lookup.c \
../src/ti/maint.c \
../src/ti/misc.c \
../src/ti/node.c \
../src/ti/pkg.c \
../src/ti/prom.c \
../src/ti/prop.c \
../src/ti/raw.c \
../src/ti/ref.c \
../src/ti/req.c \
../src/ti/signals.c \
../src/ti/sock.c \
../src/ti/store.c \
../src/ti/task.c \
../src/ti/thing.c \
../src/ti/things.c \
../src/ti/user.c \
../src/ti/val.c \
../src/ti/version.c \
../src/ti/write.c 

OBJS += \
./src/ti/access.o \
./src/ti/archive.o \
./src/ti/args.o \
./src/ti/auth.o \
./src/ti/back.o \
./src/ti/cfg.o \
./src/ti/db.o \
./src/ti/event.o \
./src/ti/front.o \
./src/ti/item.o \
./src/ti/lookup.o \
./src/ti/maint.o \
./src/ti/misc.o \
./src/ti/node.o \
./src/ti/pkg.o \
./src/ti/prom.o \
./src/ti/prop.o \
./src/ti/raw.o \
./src/ti/ref.o \
./src/ti/req.o \
./src/ti/signals.o \
./src/ti/sock.o \
./src/ti/store.o \
./src/ti/task.o \
./src/ti/thing.o \
./src/ti/things.o \
./src/ti/user.o \
./src/ti/val.o \
./src/ti/version.o \
./src/ti/write.o 

C_DEPS += \
./src/ti/access.d \
./src/ti/archive.d \
./src/ti/args.d \
./src/ti/auth.d \
./src/ti/back.d \
./src/ti/cfg.d \
./src/ti/db.d \
./src/ti/event.d \
./src/ti/front.d \
./src/ti/item.d \
./src/ti/lookup.d \
./src/ti/maint.d \
./src/ti/misc.d \
./src/ti/node.d \
./src/ti/pkg.d \
./src/ti/prom.d \
./src/ti/prop.d \
./src/ti/raw.d \
./src/ti/ref.d \
./src/ti/req.d \
./src/ti/signals.d \
./src/ti/sock.d \
./src/ti/store.d \
./src/ti/task.d \
./src/ti/thing.d \
./src/ti/things.d \
./src/ti/user.d \
./src/ti/val.d \
./src/ti/version.d \
./src/ti/write.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/%.o: ../src/ti/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DDEBUG -I../inc -I/usr/include/python3.6m/ -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


