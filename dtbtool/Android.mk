#
# Copyright (C) 2020 The MoKee Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    dtbtool.c

LOCAL_CFLAGS += \
    -Wall

LOCAL_MODULE := dtbToolApple
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)
