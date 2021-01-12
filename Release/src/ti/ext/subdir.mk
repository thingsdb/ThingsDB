################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/ext/async.c 

OBJS += \
./src/ti/ext/async.o 

C_DEPS += \
./src/ti/ext/async.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/ext/%.o: ../src/ti/ext/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -I../inc -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -msse4.2 -finline-limit=4000 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


