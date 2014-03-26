include(CheckSymbolExists)
include(CheckFunctionExists)
check_function_exists(mmap HAVE_MMAP)
check_symbol_exists(posix_madvise "sys/mman.h" HAVE_MADVISE)
configure_file(config-ksycoca.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksycoca.h )

set(kservice_SRCS
   kdeinit/ktoolinvocation.cpp
   services/kautostart.cpp
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
   services/yacc.c
   services/lex.c
   services/kplugininfo.cpp
   sycoca/ksycoca.cpp
   sycoca/ksycocadict.cpp
   sycoca/ksycocaentry.cpp
   sycoca/ksycocafactory.cpp
   sycoca/ksycocautils.cpp
   sycoca/kmemfile.cpp
   plugin/kpluginfactory.cpp
   plugin/kpluginloader.cpp
   plugin/kplugintrader.cpp
   plugin/kdbusservicestarter.cpp
)
if (WIN32)
   LIST(APPEND kservice_SRCS
      kdeinit/ktoolinvocation_win.cpp
   )
endif()
if (UNIX)
   LIST(APPEND kservice_SRCS
      kdeinit/ktoolinvocation_x11.cpp
   )
endif()

# kservice cannot depend on kinit (because kinit->kio->kservice), so we need a copy of org.kde.KLauncher.xml here.
# And I don't want to have it here as a source file (who wants to edit dbus xml by hand), so it can be
# generated from klauncher's implementation header.
qt5_add_dbus_interface(kservice_SRCS kdeinit/org.kde.KLauncher.xml klauncher_iface)

add_library(KF5Service ${kservice_SRCS})
if(WIN32)
    #unistd.h does not exist on windows
    target_compile_definitions(KF5Service PRIVATE YY_NO_UNISTD_H=1)
endif()
generate_export_header(KF5Service BASE_NAME KService)
add_library(KF5::Service ALIAS KF5Service)
set(kservice_includes
   ${CMAKE_CURRENT_BINARY_DIR}/.. # Since we publicly include kservice_version.h
   ${CMAKE_CURRENT_SOURCE_DIR}/services
   ${CMAKE_CURRENT_SOURCE_DIR}/sycoca
   ${CMAKE_CURRENT_SOURCE_DIR}/plugin
   ${CMAKE_CURRENT_SOURCE_DIR}/kdeinit
)
target_include_directories(KF5Service PUBLIC "$<BUILD_INTERFACE:${kservice_includes}>")

target_include_directories(KF5Service INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KService>")

target_link_libraries(KF5Service
  PUBLIC
    KF5::ConfigCore   # KConfig and friends
  PRIVATE
    KF5::CoreAddons   # KShell
    KF5::I18n
    KF5::DBusAddons   # KDEInitInterface
)

set_target_properties(KF5Service PROPERTIES VERSION ${KSERVICE_VERSION_STRING}
                                            SOVERSION ${KSERVICE_SOVERSION}
                                            EXPORT_NAME Service
)

ecm_generate_headers(KService_HEADERS
  HEADER_NAMES
  KPluginFactory
  KPluginLoader
  KExportPlugin
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
   services/kplugininfo.desktop
   services/application.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR}
)

install(TARGETS KF5Service EXPORT KF5ServiceTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
   "${CMAKE_CURRENT_BINARY_DIR}/kservice_export.h"
   ${KService_HEADERS}
   DESTINATION ${INCLUDE_INSTALL_DIR}/KService COMPONENT Devel
)

add_subdirectory(kbuildsycoca)
add_subdirectory(desktoptojson)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KService LIB_NAME KF5Service DEPS "KConfigCore" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
