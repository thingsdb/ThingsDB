################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/langdef/compat.c \
../src/langdef/langdef.c \
../src/langdef/translate.c 

OBJS += \
./src/langdef/compat.o \
./src/langdef/langdef.o \
./src/langdef/translate.o 

C_DEPS += \
./src/langdef/compat.d \
./src/langdef/langdef.d \
./src/langdef/translate.d 


# Each subdirectory must supply rules for building sources it contributes
src/langdef/%.o: ../src/langdef/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DLWS_WITH_LIBUV=1 -DLWS_WITH_PLUGINS=1 -I../inc -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -march=native -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


