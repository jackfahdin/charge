ifneq ($(TARGET_SIMULATOR),true)
ifneq ($(filter arm arm64 x86_64,$(TARGET_ARCH)),)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	mock.c \
	../battery.c \
	../backlight.c \
	../power.c \
	../log.c \
	../ui.c \
	../rtc.c \
	test.cpp

LOCAL_C_INCLUDES += external/libpng \
		external/zlib

LOCAL_MODULE := charge_arm_test
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -Wall -Wno-unused-parameter
LOCAL_CLANG := true
LOCAL_LDFLAGS := -Wl,-Bsymbolic

ifdef CODE_COVERAGE_ENABLED
LOCAL_CFLAGS += -fprofile-arcs -ftest-coverage -fprofile-dir=/storage/emulated/0/ylog/
LOCAL_CFLAGS += -Xclang -coverage-cfg-checksum -Xclang -coverage-no-function-names-in-data -Xclang -coverage-version='408*'
LOCAL_LDLIBS += --coverage
endif

LOCAL_SANITIZE := address
LOCAL_CFLAGS += -DK_BACKLIGHT
LOCAL_CFLAGS += -DUTIT_TEST
LOCAL_COMPATIBILITY_SUITE := units

LOCAL_STATIC_LIBRARIES := libpng
LOCAL_STATIC_LIBRARIES += libz
LOCAL_STATIC_LIBRARIES += libgtest
LOCAL_SHARED_LIBRARIES += libhardware_legacy libbase libc libcutils

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_NATIVE_TEST)

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

