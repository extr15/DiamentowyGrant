LOCAL_PATH := $(call my-dir)

# Visual Odometry library
include $(CLEAR_VARS)
include ../sdk/native/jni/OpenCV.mk

LOCAL_MODULE    := visual_odometry 
LOCAL_SRC_FILES := visual_odometry/visual_odometry.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)

# IMU library
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= imu/include
LOCAL_MODULE    := imu 
LOCAL_SRC_FILES := imu/src/orientationEstKalman.cpp imu/imu_interface.cpp
LOCAL_LDLIBS +=  -llog -ldl 

include $(BUILD_SHARED_LIBRARY)

# Scale estimation library
include $(CLEAR_VARS)

LOCAL_MODULE    := scale_estimation 
LOCAL_SRC_FILES := scale_estimation/scale_estimation.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)

# nonfree
#include $(CLEAR_VARS)
#include ../sdk/native/jni/OpenCV.mk

#LOCAL_C_INCLUDES:= ../sdk/native/jni/include
#LOCAL_MODULE    := nonfree
#LOCAL_SRC_FILES := nonfree/nonfree_init.cpp nonfree/precomp.cpp nonfree/sift.cpp nonfree/surf.cpp nonfree/export.cpp
#LOCAL_LDLIBS +=  -llog -ldl

#include $(BUILD_SHARED_LIBRARY)

