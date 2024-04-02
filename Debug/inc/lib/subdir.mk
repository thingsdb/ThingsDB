################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../inc/lib/http_parser.c 

OBJS += \
./inc/lib/http_parser.o 

C_DEPS += \
./inc/lib/http_parser.d 


# Each subdirectory must supply rules for building sources it contributes
inc/lib/%.o: ../inc/lib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DLWS_WITH_LIBUV=1 -DLWS_WITH_PLUGINS=1 -I../inc -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -march=native -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


