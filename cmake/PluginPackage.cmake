##---------------------------------------------------------------------------
## Author:      Pavel Kalian (Based on the work of Sean D'Epagnier)
## Copyright:   2014
## License:     GPLv3+
##---------------------------------------------------------------------------

IF(NOT QT_ANDROID)

# build a CPack driven installer package
#include (InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
SET(CPACK_PACKAGE_VENDOR "opencpn.org")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_NAME} ${PACKAGE_VERSION})
SET(PACKAGE_VERSION "${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_PATCH}-${PLUGIN_NAME_SUFFIX}" )
SET(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${PLUGIN_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PLUGIN_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PLUGIN_VERSION_PATCH})
SET(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${PACKAGE_NAME};ALL;/")
SET(CPACK_PACKAGE_EXECUTABLES OpenCPN ${PACKAGE_NAME})
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

IF(WIN32)

  # override install directory to put package files in the opencpn directory
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenCPN")
  SET(CPACK_NSIS_PACKAGE_NAME "${PACKAGE_NAME}")

  # Let cmake find _customized_ NSIS.template.in first here, then continue with its own modules
  SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/buildwin ${CMAKE_MODULE_PATH})

  #  Set the name of the Windows Start Menu shortcut and the icon that goes with it
  SET(CPACK_NSIS_DISPLAY_NAME "OpenCPN ${PACKAGE_NAME}")
  set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Call installInstruJS")
  set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Call un.installInstruJS")
  set (CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/cmake/icons/Dashboard_Tactics.ico")
  set (CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/cmake/icons/Dashboard_Tactics_toggled.ico")
  set (CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${PROJECT_SOURCE_DIR}\\\\cmake\\\\bitmaps\\\\wizard-install.bmp")
  set (CPACK_NSIS_MUI_OPENCPN_PLUGIN_INST_HEADER_BITMAP "${PROJECT_SOURCE_DIR}\\\\cmake\\\\bitmaps\\\\header-install.bmp") 
  set (CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${PROJECT_SOURCE_DIR}\\\\cmake\\\\bitmaps\\\\wizard-uninstall.bmp")
  set (CPACK_NSIS_MUI_OPENCPN_PLUGIN_UNINST_HEADER_BITMAP "${PROJECT_SOURCE_DIR}\\\\cmake\\\\bitmaps\\\\header-uninstall.bmp") 

ELSE(WIN32)
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY ${PACKAGE_NAME})
ENDIF(WIN32)

SET(CPACK_STRIP_FILES "${PACKAGE_NAME}")

SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gpl.txt")

IF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README")
#    MESSAGE(STATUS "Using generic cpack package description file.")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
    SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
ENDIF ()

#SET(CPACK_SOURCE_GENERATOR "TGZ")

# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
"^${CMAKE_CURRENT_SOURCE_DIR}/.git/*"
"^${CMAKE_CURRENT_SOURCE_DIR}/build*"
"^${CPACK_PACKAGE_INSTALL_DIRECTORY}/*"
)

IF(UNIX AND NOT APPLE)
#    INCLUDE(UseRPMTools)
#    IF(RPMTools_FOUND)
#        RPMTools_ADD_RPM_TARGETS(packagename ${PROJECT_SOURCE_DIR}/package.spec)
#    ENDIF(RPMTools_FOUND)

# need apt-get install rpm, for rpmbuild
    SET(PACKAGE_DEPS "opencpn, bzip2, gzip")
    SET(PACKAGE_RELEASE 1)
    SET(CPACK_GENERATOR "DEB;RPM;TBZ2")

  IF (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "arm64")
    ELSE ()
      SET (ARCH "armhf")
    ENDIF ()
  ELSE ()
    IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "amd64")
      SET(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    ELSE (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET (ARCH "i386")
      # note: in a chroot must use "setarch i686 make package"
      SET(CPACK_RPM_PACKAGE_ARCHITECTURE "i686")
    ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  ENDIF ()

    SET(CPACK_DEBIAN_PACKAGE_DEPENDS ${PACKAGE_DEPS})
    SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS ${PACKAGE_RECS})
    SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
    SET(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
    SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${PROJECT_SOURCE_DIR}/cmake/postinst;${PROJECT_SOURCE_DIR}/cmake/postrm;)

    SET(CPACK_RPM_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    SET(CPACK_RPM_PACKAGE_REQUIRES  ${PACKAGE_DEPS})
#    SET(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    SET(CPACK_RPM_PACKAGE_LICENSE "gplv3+")

    SET(CPACK_RPM_COMPRESSION_TYPE "xz")
#    SET(CPACK_RPM_USER_BINARY_SPECFILE "${PROJECT_SOURCE_DIR}/opencpn.spec.in")

    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_NAME} PlugIn for OpenCPN")
    SET(CPACK_PACKAGE_DESCRIPTION "${PACKAGE_NAME} PlugIn for OpenCPN")
#    SET(CPACK_SET_DESTDIR ON)
    SET(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")


    SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}_${PACKAGE_VERSION}-${PACKAGE_RELEASE}_${ARCH}" )
ENDIF(UNIX AND NOT APPLE)

IF(TWIN32 AND NOT UNIX)
# configure_file(${PROJECT_SOURCE_DIR}/src/opencpn.rc.in ${PROJECT_SOURCE_DIR}/src/opencpn.rc)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_GERMAN.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_GERMAN.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FRENCH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FRENCH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CZECH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CZECH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DANISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DANISH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SPANISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SPANISH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_ITALIAN.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_ITALIAN.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_DUTCH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_DUTCH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_POLISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_POLISH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESEBR.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESEBR.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_PORTUGUESE.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_PORTUGUESE.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_RUSSIAN.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_RUSSIAN.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_SWEDISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_SWEDISH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_FINNISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_FINNISH.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_NORWEGIAN.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_NORWEGIAN.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_CHINESETW.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_CHINESETW.nsh" @ONLY)
 configure_file("${PROJECT_SOURCE_DIR}/buildwin/NSIS_Unicode/Language files/Langstrings_TURKISH.nsh.in" "${PROJECT_SOURCE_DIR}//buildwin/NSIS_Unicode/Include/Langstrings_TURKISH.nsh" @ONLY)
ENDIF(TWIN32 AND NOT UNIX)


# this dummy target is necessary to make sure the ADDITIONAL_MAKE_CLEAN_FILES directive is executed.
# apparently, the base CMakeLists.txt file must have "some" target to activate all the clean steps.
#ADD_CUSTOM_TARGET(dummy COMMENT "dummy: Done." DEPENDS ${PACKAGE_NAME})


INCLUDE(CPack)

IF(NOT STANDALONE MATCHES "BUNDLED")
IF(APPLE)
MESSAGE (STATUS "*** Staging to build PlugIn OSX Package ***")

 #  Copy a bunch of files so the Packages installer builder can find them
 #  relative to ${CMAKE_CURRENT_BINARY_DIR}
 #  This avoids absolute paths in the chartdldr_pi.pkgproj file

configure_file(${PROJECT_SOURCE_DIR}/cmake/gpl.txt
            ${CMAKE_CURRENT_BINARY_DIR}/license.txt COPYONLY)
	    
configure_file(${PROJECT_SOURCE_DIR}/cmake/buildosx/InstallOSX/pkg_background.jpg
            ${CMAKE_CURRENT_BINARY_DIR}/pkg_background.jpg COPYONLY)

 # Patch the pkgproj.in file to make the output package name conform to Xxx-Plugin_x.x.pkg format
 #  Key is:
 #  <key>NAME</key>
 #  <string>${VERBOSE_NAME}-Plugin_${VERSION_MAJOR}.${VERSION_MINOR}</string>

 configure_file(${PROJECT_SOURCE_DIR}/cmake/buildosx/InstallOSX/${PACKAGE_NAME}.pkgproj.in
            ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}.pkgproj)

 ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}-Plugin_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${NAME_SUFFIX}.pkg
   COMMAND /usr/local/bin/packagesbuild -F ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}.pkgproj
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   DEPENDS ${PACKAGE_NAME}
   COMMENT "create-pkg [${PACKAGE_NAME}]: Generating pkg file."
)

 ADD_CUSTOM_TARGET(create-pkg COMMENT "create-pkg: Done."
 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${VERBOSE_NAME}-Plugin_${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${NAME_SUFFIX}.pkg )


ENDIF(APPLE)

IF(WIN32)
  SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}_${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}-win32.exe" )
  MESSAGE(STATUS "FILE: ${CPACK_PACKAGE_FILE_NAME}")
  add_custom_command(OUTPUT ${CPACK_PACKAGE_FILE_NAME}
	  COMMAND signtool sign /v /f \\cert\\OpenCPNSPC.pfx /d http://www.opencpn.org /t http://timestamp.verisign.com/scripts/timstamp.dll ${CPACK_PACKAGE_FILE_NAME}
	  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	  DEPENDS ${PACKAGE_NAME}
	  COMMENT "Code-Signing: ${CPACK_PACKAGE_FILE_NAME}")
  ADD_CUSTOM_TARGET(codesign COMMENT "code signing: Done."
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME} )

ENDIF(WIN32)
ENDIF(NOT STANDALONE MATCHES "BUNDLED")

ENDIF(NOT QT_ANDROID)
