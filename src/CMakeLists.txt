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
   services/kplugininfo.cpp
   sycoca/ksycoca.cpp
   sycoca/ksycocadevices.cpp
   sycoca/ksycocadict.cpp
   sycoca/ksycocaentry.cpp
   sycoca/ksycocafactory.cpp
   sycoca/kmemfile.cpp
   sycoca/sycocadebug.cpp
   sycoca/kbuildmimetypefactory.cpp
   sycoca/kbuildservicetypefactory.cpp
   sycoca/kbuildservicefactory.cpp
   sycoca/kbuildservicegroupfactory.cpp
   sycoca/kbuildsycoca.cpp
   sycoca/kctimefactory.cpp
   sycoca/kmimeassociations.cpp
   sycoca/vfolder_menu.cpp
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
add_flex_bison_dependency(TraderLexer TraderParser)
list(APPEND kservice_SRCS ${BISON_TraderParser_OUTPUTS} ${FLEX_TraderLexer_OUTPUTS})

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

target_include_directories(KF5Service INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KService>")

target_link_libraries(KF5Service
  PUBLIC
    KF5::ConfigCore   # KConfig and friends
    KF5::CoreAddons   # KShell KPluginLoader
  PRIVATE
    KF5::I18n
    KF5::DBusAddons   # KDEInitInterface
    Qt5::Xml          # (for vfolder menu) QDomDocument
)

set_target_properties(KF5Service PROPERTIES VERSION ${KSERVICE_VERSION_STRING}
                                            SOVERSION ${KSERVICE_SOVERSION}
                                            EXPORT_NAME Service
)

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
   DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR}
)

if (WIN32)
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_DATAROOTDIR}/xdg/menus  RENAME ${APPLICATIONS_MENU_NAME} )
else ()
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_SYSCONFDIR}/xdg/menus  RENAME ${APPLICATIONS_MENU_NAME} )
endif ()

install(TARGETS KF5Service EXPORT KF5ServiceTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
   "${CMAKE_CURRENT_BINARY_DIR}/kservice_export.h"
   ${KService_HEADERS}
   DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KService COMPONENT Devel
)

add_subdirectory(kbuildsycoca)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KService LIB_NAME KF5Service DEPS "KConfigCore" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KService)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
