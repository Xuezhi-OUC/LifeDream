################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/fsp/src/r_ioport/r_ioport.c 

C_DEPS += \
./ra/fsp/src/r_ioport/r_ioport.d 

OBJS += \
./ra/fsp/src/r_ioport/r_ioport.o 

SREC += \
LifeDream.srec 

MAP += \
LifeDream.map 


# Each subdirectory must supply rules for building sources it contributes
ra/fsp/src/r_ioport/%.o: ../ra/fsp/src/r_ioport/%.c
	$(file > $@.in,-mthumb -mfloat-abi=hard -mcpu=cortex-m85+nopacbti -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra_cfg/fsp_cfg/bsp" -I"." -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra_gen" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra_cfg/fsp_cfg" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/src" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra/fsp/inc" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra/fsp/inc/api" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra/fsp/inc/instances" -I"C:/Users/32894/Desktop/RuiSa/WorkSpace/LifeDream/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -Wno-stringop-overflow -Wno-format-truncation -flax-vector-conversions --param=min-pagesize=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

