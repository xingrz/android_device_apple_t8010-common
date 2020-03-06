#
# Copyright (C) 2020 The MoKee Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

BOARD_VENDOR := apple

COMMON_PATH := device/apple/t8010-common

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := generic

TARGET_USES_64_BIT_BINDER := true

# APEX
TARGET_FLATTEN_APEX := true

# Audio
AUDIOSERVER_MULTILIB := 64
USE_XML_AUDIO_POLICY_CONF := 1

# Bootloader
TARGET_NO_BOOTLOADER := true

# HIDL
DEVICE_MANIFEST_FILE := $(COMMON_PATH)/manifest.xml

# Media
TARGET_ENABLE_MEDIADRM_64 := true

# Properties
BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED := true
TARGET_SYSTEM_PROP := $(COMMON_PATH)/system.prop

# Recovery
TARGET_NO_RECOVERY := true

# RIL
ENABLE_VENDOR_RIL_SERVICE := true

# Treble
BOARD_VNDK_VERSION := current

# Wi-Fi
BOARD_WLAN_DEVICE           := brcmfmac
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
WIFI_DRIVER_FW_PATH_PARAM   := "/dev/null"
WIFI_DRIVER_FW_PATH_STA     := "/dev/null"
WIFI_DRIVER_FW_PATH_AP      := "/dev/null"
