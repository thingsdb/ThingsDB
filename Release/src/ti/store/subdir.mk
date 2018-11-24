################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/store/access.c \
../src/ti/store/db.c \
../src/ti/store/dbs.c \
../src/ti/store/names.c \
../src/ti/store/status.c \
../src/ti/store/things.c 

OBJS += \
./src/ti/store/access.o \
./src/ti/store/db.o \
./src/ti/store/dbs.o \
./src/ti/store/names.o \
./src/ti/store/status.o \
./src/ti/store/things.o 

C_DEPS += \
./src/ti/store/access.d \
./src/ti/store/db.d \
./src/ti/store/dbs.d \
./src/ti/store/names.d \
./src/ti/store/status.d \
./src/ti/store/things.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/store/%.o: ../src/ti/store/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DNDEBUG -I../inc -O3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


