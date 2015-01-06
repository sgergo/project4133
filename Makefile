# MSP430 Makefile
# #####################################
#
# Part of the uCtools project
# uctools.github.com
# modified by sgergo
#######################################
# user configuration:
#######################################
# TARGET: name of the output file
TARGET = main
# MCU: part number to build for
MCU = msp430fr4133
# SOURCES: list of input source sources
SOURCES = $(shell find . -name '[!.]*.c' -not -path "./contrib/*")
# INCLUDES: list of includes, by default, use Includes directory
# paths to extra libraries and extra standard includes
ROOTPATH := /opt/msp430-toolchain
BINPATH := $(ROOTPATH)/bin
LIBPATH := -L$(ROOTPATH)/msp430-none-elf/lib/430
INCLUDES := -I$(ROOTPATH)/msp430-none-elf/include
# OUTDIR: directory to use for output
OUTDIR = build
# define flags
CFLAGS = -mmcu=$(MCU) -g -Os -Wall -Wunused $(INCLUDES)
ASFLAGS = -mmcu=$(MCU) -x assembler-with-cpp -Wa,-gstabs
LDFLAGS = -mmcu=$(MCU) -Wl,$(LIBPATH),-Map=$(OUTDIR)/$(TARGET).map
#######################################
# end of user configuration
#######################################
#
#######################################
# binaries
#######################################
CC      	= $(BINPATH)/msp430-gcc
LD      	= $(BINPATH)/msp430-ld
AR      	= $(BINPATH)/msp430-ar
AS      	= $(BINPATH)/msp430-gcc
GASP    	= $(BINPATH)/msp430-gasp
NM      	= $(BINPATH)/msp430-nm
OBJCOPY 	= $(BINPATH)/msp430-objcopy
SIZE 		= $(BINPATH)/msp430-size
MAKETXT 	= srec_cat
UNIX2DOS	= unix2dos
RM      	= rm -f
MKDIR		= mkdir -p
INSTALL  	= mspdebug tilib

# Output text color codes
BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
YELLOW = '\033[0;33m'
NC='\033[0m'
#######################################

# file that includes all dependencies
DEPEND = $(SOURCES:.c=.d)

# list of object files, placed in the build directory regardless of source path
OBJECTS = $(addprefix $(OUTDIR)/,$(notdir $(SOURCES:.c=.o)))

# default: build hex file and TI TXT file
all: $(OUTDIR)/$(TARGET).hex $(OUTDIR)/$(TARGET).txt 

# TI TXT file
$(OUTDIR)/$(TARGET).txt: $(OUTDIR)/$(TARGET).hex
	$(MAKETXT) -O $@ -TITXT $< -I
	$(UNIX2DOS) $(OUTDIR)/$(TARGET).txt

# intel hex file
$(OUTDIR)/$(TARGET).hex: $(OUTDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# elf file
$(OUTDIR)/$(TARGET).elf: $(OBJECTS)
	@echo
	@echo -e $(BLUE)Linking...$(NC)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo
	@echo -e $(GREEN)Firmware size...
	$(SIZE) $(OUTDIR)/$(TARGET).elf
	@echo -e $(NC)

$(OUTDIR)/%.o: src/%.c | $(OUTDIR)
	@echo
	@echo -e $(YELLOW)Compiling $<...$(NC)
	$(CC) -c $(CFLAGS) -o $@ $<

# assembly listing
%.lst: %.c
	$(CC) -c $(ASFLAGS) -Wa,-anlhd $< > $@

# create the output directory
$(OUTDIR):
	$(MKDIR) $(OUTDIR)

.PHONY: install
install: $(OUTDIR)/$(TARGET).elf
	$(INSTALL) "prog $(OUTDIR)/${TARGET}.elf"

# remove build artifacts and executables
clean:
	-$(RM) $(OUTDIR)/*

.PHONY: all clean