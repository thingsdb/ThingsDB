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
	gcc -std=gnu11 -DNDEBUG -I../inc -I/opt/homebrew/opt/libuv/include -I/opt/homebrew/opt/pcre2/include -I/opt/homebrew/opt/yajl/include -I/opt/homebrew/opt/curl/include -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


