################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/store/access.c \
../src/ti/store/collection.c \
../src/ti/store/collections.c \
../src/ti/store/names.c \
../src/ti/store/procedures.c \
../src/ti/store/status.c \
../src/ti/store/things.c \
../src/ti/store/users.c 

OBJS += \
./src/ti/store/access.o \
./src/ti/store/collection.o \
./src/ti/store/collections.o \
./src/ti/store/names.o \
./src/ti/store/procedures.o \
./src/ti/store/status.o \
./src/ti/store/things.o \
./src/ti/store/users.o 

C_DEPS += \
./src/ti/store/access.d \
./src/ti/store/collection.d \
./src/ti/store/collections.d \
./src/ti/store/names.d \
./src/ti/store/procedures.d \
./src/ti/store/status.d \
./src/ti/store/things.d \
./src/ti/store/users.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/store/%.o: ../src/ti/store/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -std=gnu11 -I../inc -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -march=native -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


