################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/modu/expose.c \
../src/ti/modu/github.c \
../src/ti/modu/manifest.c \
../src/ti/modu/work.c 

OBJS += \
./src/ti/modu/expose.o \
./src/ti/modu/github.o \
./src/ti/modu/manifest.o \
./src/ti/modu/work.o 

C_DEPS += \
./src/ti/modu/expose.d \
./src/ti/modu/github.d \
./src/ti/modu/manifest.d \
./src/ti/modu/work.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/modu/%.o: ../src/ti/modu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -I../inc -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -march=native -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


