#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

PLUTONIUM_DIR := include/Plutonium
PLUTONIUM_INCLUDE_SWITCH := include/Plutonium/Plutonium/Output-switch/include
PLUTONIUM_INCLUDE_OUTPUT := include/Plutonium/Plutonium/Output/include
PLUTONIUM_INCLUDE_SOURCE := include/Plutonium/Plutonium/Include

TARGET := cyberfoil
BUILD := build
SOURCES := source source/ui source/data source/install source/nx source/nx/ipc source/util source/runtime external/libhaze/source \
           include/libusbhsfs/source include/libusbhsfs/source/fatfs include/libusbhsfs/source/ntfs-3g \
           include/libusbhsfs/source/lwext4 include/libusbhsfs/source/sxos
DATA := data
INCLUDES := include include/ui include/data include/install include/nx include/nx/ipc include/util include/runtime \
            include/libusbhsfs/include include/libusbhsfs/source \
            $(PLUTONIUM_INCLUDE_SWITCH) $(PLUTONIUM_INCLUDE_OUTPUT) $(PLUTONIUM_INCLUDE_SOURCE) external/libhaze/include
APP_TITLE := CyberFoil
APP_AUTHOR := luketanti
APP_VERSION := 1.4.6
GIT_COMMIT := $(shell if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then git rev-parse --short=8 HEAD 2>/dev/null; elif [ -n "$$GITHUB_SHA" ]; then printf "%s" "$$GITHUB_SHA" | cut -c1-8; else echo nogit; fi)
GIT_STATUS := $(shell if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then if git diff --quiet --ignore-submodules HEAD -- 2>/dev/null && git diff --cached --quiet --ignore-submodules HEAD -- 2>/dev/null; then echo clean; else echo dirty; fi; elif [ -n "$$GITHUB_ACTIONS" ]; then echo clean; else echo nogit; fi)
ifeq ($(RELEASE),1)
APP_GIT_META :=
APP_VERSION_FULL := $(APP_VERSION)
else
APP_GIT_META := $(GIT_COMMIT).$(GIT_STATUS)
APP_VERSION_FULL := $(APP_VERSION)+$(GIT_COMMIT).$(GIT_STATUS)
endif
ICON := romfs/images/icon.jpg
ROMFS := romfs
LIB_BLOB ?= $(TOPDIR)/prebuilt/lib.a
COMMA := ,
HAS_LIB_BLOB := $(if $(wildcard $(LIB_BLOB)),1,0)

DEFINES += -DAPP_VERSION=\"$(APP_VERSION)\"
DEFINES += -DAPP_GIT_META=\"$(APP_GIT_META)\"
DEFINES += -DAPP_VERSION_FULL=\"$(APP_VERSION_FULL)\"
DEFINES += -DHAVE_LIB_BLOB=$(HAS_LIB_BLOB)
ifeq ($(DEBUG),1)
DEFINES += -DAPP_DEBUG_LOG
endif
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS += `curl-config --cflags`
CFLAGS += `sdl2-config --cflags`
CFLAGS += `$(PREFIX)pkg-config --cflags libturbojpeg freetype2`
CFLAGS += $(INCLUDE) -D__SWITCH__ -DGPL_BUILD -Wall
ifeq ($(RELEASE),1)
CFLAGS := $(filter-out -g,$(CFLAGS))
CFLAGS += -g0 -DNDEBUG -fdata-sections
endif
CXXFLAGS := $(CFLAGS) -fno-rtti -std=gnu++20
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
ifeq ($(RELEASE),1)
ASFLAGS := $(filter-out -g,$(ASFLAGS))
ASFLAGS += -g0
LDFLAGS := $(filter-out -g,$(LDFLAGS))
LDFLAGS += -Wl,--gc-sections
endif
LIBS := `curl-config --libs`
LIBS += -lSDL2_mixer -lopusfile -lopus -lmodplug -lmpg123 -lvorbisidec -logg
LIBS += -lpu -lSDL2_gfx -lSDL2_image -lwebp -lpng -ljpeg `sdl2-config --libs` `$(PREFIX)pkg-config --libs freetype2`
LIBS += -lminizip -lzstd -lntfs-3g -llwext4
LIBS_BLOB := $(if $(filter 1,$(HAS_LIB_BLOB)),-Wl$(COMMA)--whole-archive $(LIB_BLOB) -Wl$(COMMA)--no-whole-archive,)
LIBS += $(LIBS_BLOB)
LIBS += -lmbedtls -lmbedx509 -lmbedcrypto
LIBDIRS := $(PORTLIBS) $(LIBNX) $(CURDIR)/include/Plutonium/Plutonium/Output

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export LIB_BLOB
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
ifeq ($(strip $(CONFIG_JSON)),)
jsons := $(wildcard *.json)
ifneq (,$(findstring $(TARGET).json,$(jsons)))
export APP_JSON := $(TOPDIR)/$(TARGET).json
else
ifneq (,$(findstring config.json,$(jsons)))
export APP_JSON := $(TOPDIR)/config.json
endif
endif
else
export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif
ifeq ($(strip $(ICON)),)
icons := $(wildcard *.jpg)
ifneq (,$(findstring $(TARGET).jpg,$(icons)))
export APP_ICON := $(TOPDIR)/$(TARGET).jpg
else
ifneq (,$(findstring icon.jpg,$(icons)))
export APP_ICON := $(TOPDIR)/icon.jpg
endif
endif
else
export APP_ICON := $(TOPDIR)/$(ICON)
endif
ifeq ($(strip $(NO_ICON)),)
export NROFLAGS += --icon=$(APP_ICON)
endif
ifeq ($(strip $(NO_NACP)),)
export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif
ifneq ($(APP_TITLEID),)
export NACPFLAGS += --titleid=$(APP_TITLEID)
endif
ifneq ($(ROMFS),)
export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif
.PHONY: $(BUILD) clean all
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	$(MAKE) --no-print-directory -C $(PLUTONIUM_DIR) -f Makefile lib
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@echo clean ...
ifeq ($(strip $(APP_JSON)),)
	@$(MAKE) --no-print-directory -C $(PLUTONIUM_DIR)/Plutonium -f Makefile clean
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
	@$(MAKE) --no-print-directory -C $(PLUTONIUM_DIR)/Plutonium -f Makefile clean
	@rm -fr $(BUILD) $(TARGET).nsp $(TARGET).nso $(TARGET).npdm $(TARGET).elf
endif
else
.PHONY: all
DEPENDS := $(OFILES_SRC:.o=.d)
ifeq ($(strip $(APP_JSON)),)
all: $(OUTPUT).nro
ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro: $(OUTPUT).elf
endif
else
all: $(OUTPUT).nsp
$(OUTPUT).nsp: $(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso: $(OUTPUT).elf
endif
$(OUTPUT).elf: $(OFILES)
$(OFILES_SRC): $(HFILES_BIN)
%.bin.o %_bin.h: %.bin
	@echo $(notdir $<)
	@$(bin2o)
-include $(DEPENDS)
endif
