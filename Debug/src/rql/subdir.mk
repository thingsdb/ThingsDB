################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/rql/arr.c \
../src/rql/elem.c \
../src/rql/kind.c \
../src/rql/prop.c \
../src/rql/raw.c \
../src/rql/ref.c \
../src/rql/rql.c \
../src/rql/val.c 

OBJS += \
./src/rql/arr.o \
./src/rql/elem.o \
./src/rql/kind.o \
./src/rql/prop.o \
./src/rql/raw.o \
./src/rql/ref.o \
./src/rql/rql.o \
./src/rql/val.o 

C_DEPS += \
./src/rql/arr.d \
./src/rql/elem.d \
./src/rql/kind.d \
./src/rql/prop.d \
./src/rql/raw.d \
./src/rql/ref.d \
./src/rql/rql.d \
./src/rql/val.d 


# Each subdirectory must supply rules for building sources it contributes
src/rql/%.o: ../src/rql/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/joente/workspace/rqldb/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


