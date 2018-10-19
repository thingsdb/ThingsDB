################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cfg.c \
../src/clients.c \
../src/dbs.c \
../src/events.c \
../src/nodes.c \
../src/props.c \
../src/thingsdb.c \
../src/users.c 

OBJS += \
./src/cfg.o \
./src/clients.o \
./src/dbs.o \
./src/events.o \
./src/nodes.o \
./src/props.o \
./src/thingsdb.o \
./src/users.o 

C_DEPS += \
./src/cfg.d \
./src/clients.d \
./src/dbs.d \
./src/events.d \
./src/nodes.d \
./src/props.d \
./src/thingsdb.d \
./src/users.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DDEBUG -I../inc -I/usr/include/python3.6m/ -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


