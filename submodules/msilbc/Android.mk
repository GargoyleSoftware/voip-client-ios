LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libmsilbc



LOCAL_SRC_FILES = ilbc.c \
		../libilbc-rfc3951/src/FrameClassify.c \
		../libilbc-rfc3951/src/LPCdecode.c \
		../libilbc-rfc3951/src/LPCencode.c \
		../libilbc-rfc3951/src/StateConstructW.c \
		../libilbc-rfc3951/src/StateSearchW.c \
		../libilbc-rfc3951/src/anaFilter.c \
		../libilbc-rfc3951/src/constants.c \
		../libilbc-rfc3951/src/createCB.c \
		../libilbc-rfc3951/src/doCPLC.c \
		../libilbc-rfc3951/src/enhancer.c \
		../libilbc-rfc3951/src/filter.c \
		../libilbc-rfc3951/src/gainquant.c \
		../libilbc-rfc3951/src/getCBvec.c \
		../libilbc-rfc3951/src/helpfun.c \
		../libilbc-rfc3951/src/hpInput.c \
		../libilbc-rfc3951/src/hpOutput.c \
		../libilbc-rfc3951/src/iCBConstruct.c \
		../libilbc-rfc3951/src/iCBSearch.c \
		../libilbc-rfc3951/src/iLBC_decode.c \
		../libilbc-rfc3951/src/iLBC_encode.c \
		../libilbc-rfc3951/src/lsf.c \
		../libilbc-rfc3951/src/packing.c \
		../libilbc-rfc3951/src/syntFilter.c



LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../linphone/oRTP/include \
	$(LOCAL_PATH)/../linphone/mediastreamer2/include \
	$(LOCAL_PATH)/../libilbc-rfc3951/src


include $(BUILD_STATIC_LIBRARY)


