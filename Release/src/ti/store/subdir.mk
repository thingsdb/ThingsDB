################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ti/store/storeaccess.c \
../src/ti/store/storecollection.c \
../src/ti/store/storecollections.c \
../src/ti/store/storeenums.c \
../src/ti/store/storegcollect.c \
../src/ti/store/storemodules.c \
../src/ti/store/storenames.c \
../src/ti/store/storeprocedures.c \
../src/ti/store/storestatus.c \
../src/ti/store/storetasks.c \
../src/ti/store/storethings.c \
../src/ti/store/storetypes.c \
../src/ti/store/storeusers.c 

OBJS += \
./src/ti/store/storeaccess.o \
./src/ti/store/storecollection.o \
./src/ti/store/storecollections.o \
./src/ti/store/storeenums.o \
./src/ti/store/storegcollect.o \
./src/ti/store/storemodules.o \
./src/ti/store/storenames.o \
./src/ti/store/storeprocedures.o \
./src/ti/store/storestatus.o \
./src/ti/store/storetasks.o \
./src/ti/store/storethings.o \
./src/ti/store/storetypes.o \
./src/ti/store/storeusers.o 

C_DEPS += \
./src/ti/store/storeaccess.d \
./src/ti/store/storecollection.d \
./src/ti/store/storecollections.d \
./src/ti/store/storeenums.d \
./src/ti/store/storegcollect.d \
./src/ti/store/storemodules.d \
./src/ti/store/storenames.d \
./src/ti/store/storeprocedures.d \
./src/ti/store/storestatus.d \
./src/ti/store/storetasks.d \
./src/ti/store/storethings.d \
./src/ti/store/storetypes.d \
./src/ti/store/storeusers.d 


# Each subdirectory must supply rules for building sources it contributes
src/ti/store/%.o: ../src/ti/store/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -DNDEBUG -I../inc -O3 -Wall -Wextra $(CFLAGS) -c -fmessage-length=0 -msse4.2 -finline-limit=4000 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


