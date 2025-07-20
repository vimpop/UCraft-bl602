#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

include $(BL60X_SDK_PATH)/components/network/ble/ble_common.mk

ifeq ($(CONFIG_ENABLE_PSM_RAM),1)
CPPFLAGS += -DCONF_USER_ENABLE_PSRAM
endif

ifeq ($(CONFIG_ENABLE_CAMERA),1)
CPPFLAGS += -DCONF_USER_ENABLE_CAMERA
endif

ifeq ($(CONFIG_ENABLE_BLSYNC),1)
CPPFLAGS += -DCONF_USER_ENABLE_BLSYNC
endif

ifeq ($(CONFIG_ENABLE_VFS_SPI),1)
CPPFLAGS += -DCONF_USER_ENABLE_VFS_SPI
endif

ifeq ($(CONFIG_ENABLE_VFS_ROMFS),1)
CPPFLAGS += -DCONF_USER_ENABLE_VFS_ROMFS
endif

CPPFLAGS += -DLWJSON_IGNORE_USER_OPTS -DCUSTOM_WRAPPER -DMBEDTLS_CONFIG_FILE=\"mbedtls_custom_config.h\"

COMPONENT_ADD_INCLUDEDIRS +=  UCraft/include/UCraft UCraft/3rdparty/lwjson/lwjson/src/include UCraft/3rdparty/mbedtls/include UCraft 



COMPONENT_SRCDIRS := ./ UCraft/src UCraft/3rdparty/lwjson/lwjson/src/lwjson UCraft/3rdparty/mbedtls/library

