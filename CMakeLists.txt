cmake_minimum_required(VERSION 3.6)

project(LMSS LANGUAGES C CXX)

set(CMAKE_PROJECT_NAME "LMSS")
set(CMAKE_PROJECT_DESCRIPTION
    "Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for Linux and Unix OS")

set(lmss_VERSION_MAJOR 4)
set(lmss_VERSION_MINOR 2)
set(lmss_VERSION_PATCH 3)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(XRandR REQUIRED IMPORTED_TARGET xrandr)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(Xi REQUIRED IMPORTED_TARGET xi)

set(XDG_AUTOSTART_DIR "/etc/xdg/autostart"
    CACHE PATH "Path to the freedesktop autostart directory")

configure_file(
    "${PROJECT_SOURCE_DIR}/src/config.hpp.in"
    "${PROJECT_BINARY_DIR}/config.hpp"
)

include_directories("${PROJECT_BINARY_DIR}")

add_executable(lmss
    src/display.cpp
    src/event_loop.cpp
    src/file_descriptor.cpp
    src/lmss.cpp
    src/logger.cpp
    src/main.cpp
    src/usb.cpp
)

target_link_libraries(lmss
    PUBLIC
    PkgConfig::XRandR
    PkgConfig::X11
    PkgConfig::Xi)

target_compile_options(lmss PRIVATE -Wall -Wextra -std=c++20)

install(TARGETS lmss RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

message(STATUS "lmss will use xdg/autostart")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/install/lmss.desktop
    DESTINATION ${XDG_AUTOSTART_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

set(CPACK_PACKAGE_VENDOR "WEY Technology AG")
set(CPACK_PACKAGE_NAME "LMSS")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Package for Mouse Switching Software for WEYTEC IP Remote and Deskswitch products for Linux and Unix OS ")
set(CPACK_PACKAGE_VERSION_MAJOR ${lmss_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${lmss_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${lmss_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "TGZ;DEB")

# DEB
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxi6 (>= 1.7.0)")
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
set(CPACK_RPM_PACKAGE_REQUIRES "libXi >= 1.7.0")

set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /.vscode
    /build.sh
    /build
)

include(CPack)
