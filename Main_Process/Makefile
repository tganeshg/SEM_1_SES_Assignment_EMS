#For Openwrt - Raspi
#Energy Monitor System @ Febraury 2025#

#cross compiler
#STAGING_DIR = /media/ganesh/Mine/My_Raspi/OpenWrt/Image/openwrt-raspi-toolchain/staging_dir

#export PATH := ${STAGING_DIR}/toolchain-arm_arm1176jzf-s+vfp_gcc-7.5.0_musl_eabi/bin:$(PATH)
#export STAGING_DIR := ${STAGING_DIR}

#CC 		= arm-openwrt-linux-gcc
CFLAGS	= -Wall -Wno-unused-variable -Wunused-but-set-variable -Wpointer-sign
INCS 	= -I./include
LFLAGS  = -L. -lmodbus -lsqlite3 -lmosquitto -lini

# change these to set the proper directories where each files should be
SRCDIR   = source
INCDIR	 = include
OBJDIR   = objects
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -Rf

TARGET=ems_mainProc

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(CC) -o  $@ $(OBJECTS) $(LFLAGS)
	@#cp $(TARGET) ../Firmware/bin
	@echo "Linking completed!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(INCS) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJDIR)/*.o
	@$(rm) $(BINDIR)/$(TARGET)
	@echo $(OBJECTS)
	@echo $(TARGET)
	@echo "Cleanup completed!"
