include(CheckSymbolExists)
include(CheckFunctionExists)
check_function_exists(mmap HAVE_MMAP)
check_symbol_exists(posix_madvise "sys/mman.h" HAVE_MADVISE)
configure_file(config-ksycoca.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksycoca.h )

add_library(KF5Service)
add_library(KF5::Service ALIAS KF5Service)

set_target_properties(KF5Service PROPERTIES
    VERSION     ${KSERVICE_VERSION}
    SOVERSION   ${KSERVICE_SOVERSION}
    EXPORT_NAME Service
)

target_sources(KF5Service PRIVATE
   services/kautostart.cpp
   services/kapplicationtrader.cpp
   services/kmimetypefactory.cpp
   services/kmimetypetrader.cpp
   services/kservice.cpp
   services/kserviceaction.cpp
   services/kservicefactory.cpp
   services/kservicegroup.cpp
   services/kservicegroupfactory.cpp
   services/kserviceoffer.cpp
   services/kservicetype.cpp
   services/kservicetypefactory.cpp
   services/kservicetypeprofile.cpp
   services/kservicetypetrader.cpp
   services/ktraderparse.cpp
   services/ktraderparsetree.cpp
   services/kplugininfo.cpp
   sycoca/ksycoca.cpp
   sycoca/ksycocadevices.cpp
   sycoca/ksycocadict.cpp
   sycoca/ksycocaentry.cpp
   sycoca/ksycocafactory.cpp
   sycoca/kmemfile.cpp
   sycoca/kbuildmimetypefactory.cpp
   sycoca/kbuildservicetypefactory.cpp
   sycoca/kbuildservicefactory.cpp
   sycoca/kbuildservicegroupfactory.cpp
   sycoca/kbuildsycoca.cpp
   sycoca/kctimefactory.cpp
   sycoca/kmimeassociations.cpp
   sycoca/vfolder_menu.cpp
   plugin/kplugintrader.cpp

   kservice.qrc
)

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.89.0)
   target_sources(KF5Service PRIVATE kdeinit/ktoolinvocation.cpp)
   if (WIN32)
        target_sources(KF5Service PRIVATE kdeinit/ktoolinvocation_win.cpp)
    endif()
    if (UNIX)
        target_sources(KF5Service PRIVATE kdeinit/ktoolinvocation_x11.cpp)
    endif()
endif()

if (TARGET Qt${QT_MAJOR_VERSION}::DBus)
    target_sources(KF5Service PRIVATE
        plugin/kdbusservicestarter.cpp
    )
endif()

ecm_qt_declare_logging_category(KF5Service
    HEADER servicesdebug.h
    IDENTIFIER SERVICES
    CATEGORY_NAME kf.service.services
    OLD_CATEGORY_NAMES kf5.kservice.services
    EXPORT KSERVICE
    DESCRIPTION "service (kservice lib)"
)
ecm_qt_declare_logging_category(KF5Service
    HEADER sycocadebug.h
    IDENTIFIER SYCOCA
    CATEGORY_NAME kf.service.sycoca
    OLD_CATEGORY_NAMES kf5.kservice.sycoca
    EXPORT KSERVICE
    DESCRIPTION "sycoca (kservice)"
)

bison_target(TraderParser
             services/yacc.y
             ${CMAKE_CURRENT_BINARY_DIR}/yacc.c
             COMPILE_FLAGS "-p kiotrader -d"
)
flex_target(TraderLexer
            services/lex.l
            ${CMAKE_CURRENT_BINARY_DIR}/lex.c
            COMPILE_FLAGS "-Pkiotrader -B -i"
)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/lex.c ${CMAKE_CURRENT_BINARY_DIR}/yacc.c PROPERTY SKIP_AUTOMOC ON)
add_flex_bison_dependency(TraderLexer TraderParser)
target_sources(KF5Service PRIVATE
    ${BISON_TraderParser_OUTPUTS}
    ${FLEX_TraderLexer_OUTPUTS}
)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/yacc.h PROPERTY SKIP_AUTOMOC TRUE) # don't run automoc on this file

# kservice cannot depend on kinit (because kinit->kio->kservice), so we need a copy of org.kde.KLauncher.xml here.
# And I don't want to have it here as a source file (who wants to edit dbus xml by hand), so it can be
# generated from klauncher's implementation header.
if (TARGET Qt${QT_MAJOR_VERSION}::DBus)
    set(kservice_dbus_SRCS)
    qt_add_dbus_interface(kservice_dbus_SRCS kdeinit/org.kde.KLauncher.xml klauncher_iface)
    target_sources(KF5Service PRIVATE ${kservice_dbus_SRCS})
endif()

if(WIN32)
    #unistd.h does not exist on windows
    target_compile_definitions(KF5Service PRIVATE YY_NO_UNISTD_H=1)
endif()
ecm_generate_export_header(KF5Service
    BASE_NAME KService
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.0 5.15 5.61 5.63 5.66 5.67 5.70 5.71 5.79 5.80 5.81 5.82 5.83 5.86 5.87 5.88 5.89 5.90 5.102
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)
set(kservice_includes
   ${CMAKE_CURRENT_BINARY_DIR}/.. # Since we publicly include kservice_version.h
   ${CMAKE_CURRENT_SOURCE_DIR}/services
   ${CMAKE_CURRENT_SOURCE_DIR}/sycoca
   ${CMAKE_CURRENT_SOURCE_DIR}/plugin
   ${CMAKE_CURRENT_SOURCE_DIR}/kdeinit
)
target_include_directories(KF5Service
    PUBLIC
        "$<BUILD_INTERFACE:${kservice_includes}>"
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KService>"
)

target_link_libraries(KF5Service
  PRIVATE
    KF5::I18n
    Qt${QT_MAJOR_VERSION}::Xml  # (for vfolder menu) QDomDocument
)

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.94.0)
    target_link_libraries(KF5Service
    PUBLIC
        KF5::ConfigCore   # KConfig and friends
        KF5::CoreAddons   # KShell KPluginLoader
    )
else()
    target_link_libraries(KF5Service
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        KF5::ConfigCore   # KConfig and friends
        KF5::CoreAddons   # KShell KPluginLoader
    )
endif()

if (TARGET KF5::DBusAddons)
  target_link_libraries(KF5Service PRIVATE KF5::DBusAddons)  # KDEInitInterface
endif()

ecm_generate_headers(KService_HEADERS
  HEADER_NAMES
  KPluginTrader
  KDBusServiceStarter

  RELATIVE plugin REQUIRED_HEADERS KService_HEADERS
)
ecm_generate_headers(KService_HEADERS
  HEADER_NAMES
  KSycoca
  KSycocaEntry
  KSycocaType

  RELATIVE sycoca REQUIRED_HEADERS KService_HEADERS
)
ecm_generate_headers(KService_HEADERS
  HEADER_NAMES
  KToolInvocation

  RELATIVE kdeinit REQUIRED_HEADERS KService_HEADERS
)
ecm_generate_headers(KService_HEADERS
  HEADER_NAMES
  KApplicationTrader
  KAutostart
  KMimeTypeTrader
  KService
  KServiceAction
  KServiceGroup
  KServiceType
  KServiceTypeProfile
  KServiceTypeTrader
  KPluginInfo

  RELATIVE services REQUIRED_HEADERS KService_HEADERS
)

install(FILES
   services/kplugininfo.desktop # TODO move to qrc too, like application.desktop?
   services/application.desktop # KF6: TODO: stop installing this file, it's now bundled into the qrc
   DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR}
)

# Local copy for the unittests
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5)
add_custom_command(TARGET KF5Service PRE_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/services/kplugininfo.desktop
    ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5) # not using KDE_INSTALL_KSERVICETYPESDIR because QStandardPaths wants "data" on Windows

if (WIN32)
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_DATAROOTDIR}/xdg/menus  RENAME ${APPLICATIONS_MENU_NAME} )
else ()
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_SYSCONFDIR}/xdg/menus  RENAME ${APPLICATIONS_MENU_NAME} )
endif ()

# Local copy for the unittests
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/menus)
add_custom_command(TARGET KF5Service PRE_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/applications.menu
    ${CMAKE_BINARY_DIR}/bin/data/menus/${APPLICATIONS_MENU_NAME})

install(TARGETS KF5Service EXPORT KF5ServiceTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
   "${CMAKE_CURRENT_BINARY_DIR}/kservice_export.h"
   ${KService_HEADERS}
   DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KService COMPONENT Devel
)

ecm_qt_install_logging_categories(
    EXPORT KSERVICE
    FILE kservice.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

if(BUILD_QCH)
    ecm_add_qch(
        KF5Service_QCH
        NAME KService
        BASE_NAME KF5Service
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KService_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            KF5Config_QCH
            KF5CoreAddons_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
            ${kservice_includes}
        BLANK_MACROS
            KSERVICE_EXPORT
            KSERVICE_DEPRECATED
            KSERVICE_DEPRECATED_EXPORT
            "KSERVICE_DEPRECATED_VERSION(x, y, t)"
            "KSERVICE_DEPRECATED_VERSION_BELATED(x, y, xt, yt, t)"
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

add_subdirectory(kbuildsycoca)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KService LIB_NAME KF5Service DEPS "KConfigCore" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF}/KService)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
