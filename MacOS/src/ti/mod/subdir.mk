################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/mod/expose.c \
../src/ti/mod/github.c \
../src/ti/mod/manifest.c \
../src/ti/mod/work.c 

OBJS += \
./src/ti/mod/expose.o \
./src/ti/mod/github.o \
./src/ti/mod/manifest.o \
./src/ti/mod/work.o 

C_DEPS += \
./src/ti/mod/expose.d \
./src/ti/mod/github.d \
./src/ti/mod/manifest.d \
./src/ti/mod/work.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/mod/%.o: ../src/ti/mod/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -DLWS_WITH_PLUGINS=1 -DLWS_WITH_LIBUV=1 -I../inc -I/opt/homebrew/opt/libuv/include -I/opt/homebrew/opt/pcre2/include -I/opt/homebrew/opt/yajl/include -I/opt/homebrew/opt/curl/include -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


