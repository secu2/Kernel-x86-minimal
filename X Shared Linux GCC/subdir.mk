################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../kprintf.c \
../loader.c \
../main.c 

O_SRCS += \
../boot.o \
../crt0.o \
../kprintf.o \
../loader.o \
../main.o 

S_UPPER_SRCS += \
../boot.S \
../crt0.S 

OBJS += \
./boot.o \
./crt0.o \
./kprintf.o \
./loader.o \
./main.o 

C_DEPS += \
./kprintf.d \
./loader.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.S
	@echo 'Building file: $<'
	@echo 'Invoking: GCC Assembler'
	as  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


