LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libmsx264


LOCAL_SRC_FILES = src/msx264.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../linphone/oRTP/include \
	$(LOCAL_PATH)/../linphone/mediastreamer2/include \
	$(LOCAL_PATH)/../externals/x264

LOCAL_CFLAGS += -DVERSION=\"android\"

include $(BUILD_STATIC_LIBRARY)


