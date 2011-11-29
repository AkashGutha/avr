################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include default/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
FLASH_IMAGE += \
IR_Receiver.hex \

SIZEDUMMY += \
sizedummy \


# All Target
all: IR_Receiver.elf secondary-outputs

# Tool invocations
IR_Receiver.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,IR_Receiver.map -mmcu=atmega8 -o "IR_Receiver.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

IR_Receiver.hex: IR_Receiver.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex IR_Receiver.elf  "IR_Receiver.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: IR_Receiver.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega8 IR_Receiver.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(FLASH_IMAGE)$(ELFS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) IR_Receiver.elf
	-@echo ' '

secondary-outputs: $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
