################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../inc/msgpack/version.c 

OBJS += \
./inc/msgpack/version.o 

C_DEPS += \
./inc/msgpack/version.d 


# Each subdirectory must supply rules for building sources it contributes
inc/msgpack/%.o: ../inc/msgpack/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -I../inc -I/opt/homebrew/opt/libuv/include -I/opt/homebrew/opt/pcre2/include -I/opt/homebrew/opt/yajl/include -I/opt/homebrew/opt/curl/include -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


