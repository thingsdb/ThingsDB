################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/util/argparse.c \
../src/util/cfgparser.c \
../src/util/cryptx.c \
../src/util/fx.c \
../src/util/guid.c \
../src/util/imap.c \
../src/util/link.c \
../src/util/lock.c \
../src/util/logger.c \
../src/util/omap.c \
../src/util/qpx.c \
../src/util/queue.c \
../src/util/res.c \
../src/util/smap.c \
../src/util/strx.c \
../src/util/vec.c 

OBJS += \
./src/util/argparse.o \
./src/util/cfgparser.o \
./src/util/cryptx.o \
./src/util/fx.o \
./src/util/guid.o \
./src/util/imap.o \
./src/util/link.o \
./src/util/lock.o \
./src/util/logger.o \
./src/util/omap.o \
./src/util/qpx.o \
./src/util/queue.o \
./src/util/res.o \
./src/util/smap.o \
./src/util/strx.o \
./src/util/vec.o 

C_DEPS += \
./src/util/argparse.d \
./src/util/cfgparser.d \
./src/util/cryptx.d \
./src/util/fx.d \
./src/util/guid.d \
./src/util/imap.d \
./src/util/link.d \
./src/util/lock.d \
./src/util/logger.d \
./src/util/omap.d \
./src/util/qpx.d \
./src/util/queue.d \
./src/util/res.d \
./src/util/smap.d \
./src/util/strx.d \
./src/util/vec.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I../inc -I/usr/include/python3.6m/ -O0 -g3 -Wall -Wextra -Winline -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


