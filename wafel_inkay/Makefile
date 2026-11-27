#---------------------------------------------------------------------------------
# wafel_inkay — Talonflame network patches as a stroopwafel plugin (.ipx)
#
# Build requirements:
#   - devkitARM (big-endian multilib, as shipped in ghcr.io/wiiu-env/devkitppc)
#   - STROOPWAFEL_ROOT pointing at a checkout of StroopwafelCFW/stroopwafel
#     (only wafel_core/include is used; symbols are resolved at load time)
#
#   export STROOPWAFEL_ROOT=/path/to/stroopwafel
#   make
#
# Output: wafel_inkay.ipx — copy to sd:/wiiu/ios_plugins/ next to 00core.ipx
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(strip $(STROOPWAFEL_ROOT)),)
$(error "Please set STROOPWAFEL_ROOT to a checkout of StroopwafelCFW/stroopwafel")
endif
endif

export TOPDIR ?= $(CURDIR)
export MAKEFILE_ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
include $(MAKEFILE_ROOT_DIR)/elf_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
export TARGET		:=	wafel_inkay
export BUILD		?=	build

R_SOURCES			:=
SOURCES				:=	source

R_INCLUDES			:=
INCLUDES 			:=	source

DATA				:=

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH			:=	-march=armv5te -mcpu=arm926ej-s -marm -mthumb-interwork -mbig-endian -mfloat-abi=soft -fPIE -fno-jump-tables -export-dynamic -s -isystem $(STROOPWAFEL_ROOT)/wafel_core/include

CFLAGS			:=	-g -std=c11 -Os -Wall \
					-fomit-frame-pointer -fdata-sections -ffunction-sections \
					$(ARCH) -nostartfiles

CFLAGS			+=	$(INCLUDE) -D_GNU_SOURCE -DCAN_HAZ_IRQ -fno-builtin-printf -Wno-nonnull -Werror=implicit

CXXFLAGS		:=	$(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS			:=	-g $(ARCH)
LDFLAGS			 =	-g --specs=$(MAKEFILE_ROOT_DIR)/ccd00.specs $(ARCH) -Wl,--gc-sections,-Map,$(TARGET).map \
					-L$(DEVKITARM)/lib/gcc/arm-none-eabi/$(GCC_VERSION)/be -L$(DEVKITARM)/arm-none-eabi/lib/be \
					-z max-page-size=4096 -nostartfiles

LIBS			:=	-lnosys

LIBDIRS			:=

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export ROOTDIR	:=	$(CURDIR)
export OUTPUT	:=	$(CURDIR)/$(TARGET)

SOURCES         := $(SOURCES) $(foreach dir,$(R_SOURCES), $(dir) $(filter %/, $(wildcard $(dir)/*/)))
INCLUDES        := $(INCLUDES) $(foreach dir,$(R_INCLUDES), $(dir) $(filter %/, $(wildcard $(dir)/*/)))

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.S=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@mv $(OUTPUT).elf $(OUTPUT).ipx
	@echo built ... $(TARGET).ipx

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).ipx $(TARGET).map

#---------------------------------------------------------------------------------
else

DEPENDS		:=	$(OFILES:.o=.d)

$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
