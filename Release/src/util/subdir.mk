################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/util/argparse.c \
../src/util/big.c \
../src/util/buf.c \
../src/util/cfgparser.c \
../src/util/cryptx.c \
../src/util/fx.c \
../src/util/guid.c \
../src/util/imap.c \
../src/util/iso8601.c \
../src/util/link.c \
../src/util/lock.c \
../src/util/logger.c \
../src/util/olist.c \
../src/util/omap.c \
../src/util/queue.c \
../src/util/smap.c \
../src/util/strx.c \
../src/util/syncpart.c \
../src/util/util.c \
../src/util/vec.c 

OBJS += \
./src/util/argparse.o \
./src/util/big.o \
./src/util/buf.o \
./src/util/cfgparser.o \
./src/util/cryptx.o \
./src/util/fx.o \
./src/util/guid.o \
./src/util/imap.o \
./src/util/iso8601.o \
./src/util/link.o \
./src/util/lock.o \
./src/util/logger.o \
./src/util/olist.o \
./src/util/omap.o \
./src/util/queue.o \
./src/util/smap.o \
./src/util/strx.o \
./src/util/syncpart.o \
./src/util/util.o \
./src/util/vec.o 

C_DEPS += \
./src/util/argparse.d \
./src/util/big.d \
./src/util/buf.d \
./src/util/cfgparser.d \
./src/util/cryptx.d \
./src/util/fx.d \
./src/util/guid.d \
./src/util/imap.d \
./src/util/iso8601.d \
./src/util/link.d \
./src/util/lock.d \
./src/util/logger.d \
./src/util/olist.d \
./src/util/omap.d \
./src/util/queue.d \
./src/util/smap.d \
./src/util/strx.d \
./src/util/syncpart.d \
./src/util/util.d \
./src/util/vec.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -I../inc -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -msse4.2 -finline-limit=4000 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


