################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/rql/args.c \
../src/rql/back.c \
../src/rql/cfg.c \
../src/rql/db.c \
../src/rql/elem.c \
../src/rql/event.c \
../src/rql/front.c \
../src/rql/node.c \
../src/rql/pkg.c \
../src/rql/raw.c \
../src/rql/ref.c \
../src/rql/rql.c \
../src/rql/signals.c \
../src/rql/sock.c \
../src/rql/task.c \
../src/rql/user.c \
../src/rql/users.c \
../src/rql/val.c \
../src/rql/version.c 

OBJS += \
./src/rql/args.o \
./src/rql/back.o \
./src/rql/cfg.o \
./src/rql/db.o \
./src/rql/elem.o \
./src/rql/event.o \
./src/rql/front.o \
./src/rql/node.o \
./src/rql/pkg.o \
./src/rql/raw.o \
./src/rql/ref.o \
./src/rql/rql.o \
./src/rql/signals.o \
./src/rql/sock.o \
./src/rql/task.o \
./src/rql/user.o \
./src/rql/users.o \
./src/rql/val.o \
./src/rql/version.o 

C_DEPS += \
./src/rql/args.d \
./src/rql/back.d \
./src/rql/cfg.d \
./src/rql/db.d \
./src/rql/elem.d \
./src/rql/event.d \
./src/rql/front.d \
./src/rql/node.d \
./src/rql/pkg.d \
./src/rql/raw.d \
./src/rql/ref.d \
./src/rql/rql.d \
./src/rql/signals.d \
./src/rql/sock.d \
./src/rql/task.d \
./src/rql/user.d \
./src/rql/users.d \
./src/rql/val.d \
./src/rql/version.d 


# Each subdirectory must supply rules for building sources it contributes
src/rql/%.o: ../src/rql/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DDEBUG -I"/home/joente/workspace/rqldb/inc" -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


