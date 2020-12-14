cmake_minimum_required(VERSION 3.9)

###
# PROJECT CONFIG
###
# set the project name
project(LMSS)

set(CMAKE_PROJECT_NAME "LMSS")
set(CMAKE_PROJECT_DESCRIPTION
    "Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for Linux and Unix OS")

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(XRandR REQUIRED IMPORTED_TARGET xrandr)
pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

set(XDG_AUTOSTART_DIR "/etc/xdg/autostart"
    CACHE PATH "Path to the freedesktop autostart directory")

## solaris workaround
if(CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)" )
    add_definitions(-DSOLARIS_WORKAROUND)
endif(CMAKE_SYSTEM_NAME MATCHES "(Solaris|SunOS)" )

# add the executable
add_executable(lmss
    src/functions.c
    src/log.c
    src/screens.c
    src/utilities.c
    src/usb_helper.c
    src/weyip_usb.c
    src/main.c
)

# add library
target_link_libraries(lmss PUBLIC PkgConfig::XRandR PkgConfig::libusb PkgConfig::X11)

set_property(TARGET lmss PROPERTY C_STANDARD 11)
target_compile_options(lmss PRIVATE -Wall -Wextra)

install(TARGETS lmss RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

message(STATUS "lmss will use xdg/autostart")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/install/lmss.desktop
    DESTINATION ${XDG_AUTOSTART_DIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

###
# CPACK CONFIG
###
set(CPACK_PACKAGE_VENDOR "WEY Technology AG")
set(CPACK_PACKAGE_NAME "LMSS")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Package for Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for Linux and Unix OS ")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "TGZ;DEB")

# DEB
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "WEY Technology AG")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/install/postinst;${CMAKE_CURRENT_SOURCE_DIR}/install/prerm;" )
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

# RPM
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/etc/xdg"
    "${XDG_AUTOSTART_DIR}"
)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/install/postinst")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/install/prerm")

set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /.vscode
    /build.sh
    /build
)

include(CPack)
