
cmake_minimum_required(VERSION 2.8.12)

project(KService)

# ECM setup
find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(FeatureSummary)
include(GenerateExportHeader)

set(KF5_VERSION "5.3.0") # handled by release scripts
set(KF5_DEP_VERSION "5.3.0") # handled by release scripts

ecm_setup_version(${KF5_VERSION}
                  VARIABLE_PREFIX KSERVICE
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kservice_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5ServiceConfigVersion.cmake"
                  SOVERSION 5)

# TODO: Remove these
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

# Dependencies
set(REQUIRED_QT_VERSION "5.2.0")
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED DBus Xml)

find_package(KF5Config ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5CoreAddons ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Crash ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5DBusAddons ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5DocTools REQUIRED)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5Service")

include(ECMPackageConfigHelpers)

ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/KF5ServiceConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/KF5ServiceConfig.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                             )

install(EXPORT KF5ServiceTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5ServiceTargets.cmake NAMESPACE KF5:: )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/KF5ServiceConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/KF5ServiceConfigVersion.cmake"
              "${CMAKE_CURRENT_SOURCE_DIR}/KF5ServiceMacros.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
       )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kservice_version.h
        DESTINATION ${KF5_INCLUDE_INSTALL_DIR} COMPONENT Devel )

add_definitions(-DTRANSLATION_DOMAIN=\"kservice5\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
    kdoctools_install(po)
endif()
add_subdirectory(docs)
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
