#
# Copyright (C) 2019 GlobalLogic
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Include only for Renesas ones.
ifneq (,$(filter $(TARGET_PRODUCT), salvator ulcb kingfisher))
LOCAL_PATH:= $(call my-dir)

################################################################################
# Build OemLock HAL                                                            #
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.oemlock@1.0-service.renesas
LOCAL_INIT_RC := android.hardware.oemlock@1.0-service.renesas.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS = \
	-Wall \
	-Werror \
	-DANDROID_BUILD

LOCAL_C_INCLUDES := \
	hardware/renesas/optee-client/public \
	$(LOCAL_PATH)/ta

LOCAL_SRC_FILES := \
	service.cpp \
	OemLock.cpp

LOCAL_SHARED_LIBRARIES := \
	libteec \
	libbase \
	liblog \
	libcutils \
	libhardware \
	libhidlbase \
	libhidltransport \
	libutils \
	android.hardware.oemlock@1.0

include $(BUILD_EXECUTABLE)

################################################################################
# Build OemLock HAL TA                                                         #
################################################################################

# Please keep this variable consistent with TA_OEMLOCK_UUID defined
# in oemlock_ta.h
TA_UUID:=be1e65f4-40ca-11e9-b210d663bd873d93
TA_SRC:=$(LOCAL_PATH)/ta

include device/renesas/common/build/build_executable.mk

endif # Include only for Renesas ones.
