# Install script for directory: /Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp/src/libzrtpcpp

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/Users/chris/Code/Gargoyle/linphone-iphone/liblinphone-sdk/armv7-apple-darwin")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libzrtpcpp" TYPE FILE FILES
    "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp/src/libzrtpcpp/ZrtpCodes.h"
    "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp/src/libzrtpcpp/ZrtpConfigure.h"
    "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp/src/libzrtpcpp/ZrtpCallback.h"
    "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp/src/libzrtpcpp/ZrtpCWrapper.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

