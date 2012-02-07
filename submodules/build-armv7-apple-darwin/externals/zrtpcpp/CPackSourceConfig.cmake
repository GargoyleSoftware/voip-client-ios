# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. Example variables are:
#   CPACK_GENERATOR                     - Generator used to create package
#   CPACK_INSTALL_CMAKE_PROJECTS        - For each project (path, name, component)
#   CPACK_CMAKE_GENERATOR               - CMake Generator used for the projects
#   CPACK_INSTALL_COMMANDS              - Extra commands to install components
#   CPACK_INSTALLED_DIRECTORIES           - Extra directories to install
#   CPACK_PACKAGE_DESCRIPTION_FILE      - Description file for the package
#   CPACK_PACKAGE_DESCRIPTION_SUMMARY   - Summary of the package
#   CPACK_PACKAGE_EXECUTABLES           - List of pairs of executables and labels
#   CPACK_PACKAGE_FILE_NAME             - Name of the package generated
#   CPACK_PACKAGE_ICON                  - Icon used for the package
#   CPACK_PACKAGE_INSTALL_DIRECTORY     - Name of directory for the installer
#   CPACK_PACKAGE_NAME                  - Package project name
#   CPACK_PACKAGE_VENDOR                - Package project vendor
#   CPACK_PACKAGE_VERSION               - Package project version
#   CPACK_PACKAGE_VERSION_MAJOR         - Package project version (major)
#   CPACK_PACKAGE_VERSION_MINOR         - Package project version (minor)
#   CPACK_PACKAGE_VERSION_PATCH         - Package project version (patch)

# There are certain generator specific ones

# NSIS Generator:
#   CPACK_PACKAGE_INSTALL_REGISTRY_KEY  - Name of the registry key for the installer
#   CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS - Extra commands used during uninstall
#   CPACK_NSIS_EXTRA_INSTALL_COMMANDS   - Extra commands used during install


SET(CPACK_BINARY_BUNDLE "")
SET(CPACK_BINARY_CYGWIN "")
SET(CPACK_BINARY_DEB "")
SET(CPACK_BINARY_DRAGNDROP "")
SET(CPACK_BINARY_NSIS "")
SET(CPACK_BINARY_OSXX11 "")
SET(CPACK_BINARY_PACKAGEMAKER "")
SET(CPACK_BINARY_RPM "")
SET(CPACK_BINARY_STGZ "")
SET(CPACK_BINARY_TBZ2 "")
SET(CPACK_BINARY_TGZ "")
SET(CPACK_BINARY_TZ "")
SET(CPACK_BINARY_ZIP "")
SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")
SET(CPACK_COMPONENTS_ALL "")
SET(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
SET(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
SET(CPACK_GENERATOR "TBZ2")
SET(CPACK_IGNORE_FILES "\\.svn/;\\.cvsignore$;\\.swp$;~$;\\.cdtproject$;\\.cproject$;\\.project$;\\.settings/;\\.kdev4/;\\.kdev4$;\\.#;/#;/build/;/autom4te\\.cache/;/_build/;/doc/html/;/\\.git/;/CVS/;/\\.libs/;/\\.deps/;\\.o$;\\.lo$;\\.la$;\\.sh$;Makefile\\.in$")
SET(CPACK_INSTALLED_DIRECTORIES "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp;/")
SET(CPACK_INSTALL_CMAKE_PROJECTS "")
SET(CPACK_INSTALL_PREFIX "/Users/chris/Code/Gargoyle/linphone-iphone/liblinphone-sdk/armv7-apple-darwin")
SET(CPACK_MODULE_PATH "")
SET(CPACK_NSIS_DISPLAY_NAME "libzrtpcpp 2.1.0")
SET(CPACK_NSIS_INSTALLER_ICON_CODE "")
SET(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
SET(CPACK_NSIS_PACKAGE_NAME "libzrtpcpp 2.1.0")
SET(CPACK_OUTPUT_CONFIG_FILE "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build-armv7-apple-darwin/externals/zrtpcpp/CPackConfig.cmake")
SET(CPACK_PACKAGE_DEFAULT_LOCATION "/")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "/opt/local/share/cmake-2.8/Templates/CPack.GenericDescription.txt")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libzrtpcpp built using CMake")
SET(CPACK_PACKAGE_FILE_NAME "libzrtpcpp-2.1.0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "libzrtpcpp 2.1.0")
SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "libzrtpcpp 2.1.0")
SET(CPACK_PACKAGE_NAME "libzrtpcpp")
SET(CPACK_PACKAGE_RELOCATABLE "true")
SET(CPACK_PACKAGE_VENDOR "Werner Dittmann")
SET(CPACK_PACKAGE_VERSION "2.1.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_RESOURCE_FILE_LICENSE "/opt/local/share/cmake-2.8/Templates/CPack.GenericLicense.txt")
SET(CPACK_RESOURCE_FILE_README "/opt/local/share/cmake-2.8/Templates/CPack.GenericDescription.txt")
SET(CPACK_RESOURCE_FILE_WELCOME "/opt/local/share/cmake-2.8/Templates/CPack.GenericWelcome.txt")
SET(CPACK_SET_DESTDIR "OFF")
SET(CPACK_SOURCE_CYGWIN "")
SET(CPACK_SOURCE_GENERATOR "TBZ2")
SET(CPACK_SOURCE_IGNORE_FILES "\\.svn/;\\.cvsignore$;\\.swp$;~$;\\.cdtproject$;\\.cproject$;\\.project$;\\.settings/;\\.kdev4/;\\.kdev4$;\\.#;/#;/build/;/autom4te\\.cache/;/_build/;/doc/html/;/\\.git/;/CVS/;/\\.libs/;/\\.deps/;\\.o$;\\.lo$;\\.la$;\\.sh$;Makefile\\.in$")
SET(CPACK_SOURCE_INSTALLED_DIRECTORIES "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/externals/zrtpcpp;/")
SET(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/Users/chris/Code/Gargoyle/linphone-iphone/submodules/build-armv7-apple-darwin/externals/zrtpcpp/CPackSourceConfig.cmake")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "libzrtpcpp-2.1.0")
SET(CPACK_SOURCE_TBZ2 "")
SET(CPACK_SOURCE_TGZ "")
SET(CPACK_SOURCE_TOPLEVEL_TAG "Generic-Source")
SET(CPACK_SOURCE_TZ "")
SET(CPACK_SOURCE_ZIP "")
SET(CPACK_STRIP_FILES "")
SET(CPACK_SYSTEM_NAME "Generic")
SET(CPACK_TOPLEVEL_TAG "Generic-Source")
