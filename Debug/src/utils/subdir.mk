################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/utils/argparse.c \
../src/utils/cfgparser.c \
../src/utils/imap.c \
../src/utils/logger.c \
../src/utils/smap.c \
../src/utils/strx.c \
../src/utils/vec.c 

OBJS += \
./src/utils/argparse.o \
./src/utils/cfgparser.o \
./src/utils/imap.o \
./src/utils/logger.o \
./src/utils/smap.o \
./src/utils/strx.o \
./src/utils/vec.o 

C_DEPS += \
./src/utils/argparse.d \
./src/utils/cfgparser.d \
./src/utils/imap.d \
./src/utils/logger.d \
./src/utils/smap.d \
./src/utils/strx.d \
./src/utils/vec.d 


# Each subdirectory must supply rules for building sources it contributes
src/utils/%.o: ../src/utils/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/joente/workspace/rqldb/inc" -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


