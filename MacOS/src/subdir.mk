################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ex.c \
../src/ti.c 

OBJS += \
./src/ex.o \
./src/ti.o 

C_DEPS += \
./src/ex.d \
./src/ti.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -DLWS_WITH_PLUGINS=1 -DLWS_WITH_LIBUV=1 -I../inc -I/opt/homebrew/opt/libuv/include -I/opt/homebrew/opt/pcre2/include -I/opt/homebrew/opt/yajl/include -I/opt/homebrew/opt/curl/include -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


