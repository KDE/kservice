include(ECMMarkNonGuiExecutable)

set(kbuildsycoca_SRCS
   kbuildsycoca_main.cpp
   kbuildsycoca.cpp
   kbuildmimetypefactory.cpp
   kmimeassociations.cpp
   kbuildservicetypefactory.cpp
   kbuildservicefactory.cpp
   kbuildservicegroupfactory.cpp
   kctimefactory.cpp
   vfolder_menu.cpp
   )

# We need to add a '5' so that kde3/kde4 apps running kbuildsycoca don't run the kde4 one.
add_executable( kbuildsycoca5 ${kbuildsycoca_SRCS})

# Mark it as non-gui so we won't create an app bundle on Mac OS X
ecm_mark_nongui_executable(kbuildsycoca5)

target_link_libraries(kbuildsycoca5
   KF5::Service #KBuildMimeTypeFactory subclasses KMimeTypeFactory, KServiceGroupFactory and KService used as well
   KF5::Crash  #set a crash handler for ksycoca binary
   KF5::CoreAddons # KAboutData
   KF5::I18n
   Qt5::Xml #vfolder menui QDomDocument and family
)

install(TARGETS kbuildsycoca5 ${KF5_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############
if (WIN32)
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_DATAROOTDIR}/xdg/menus )
else ()
install( FILES applications.menu  DESTINATION  ${KDE_INSTALL_SYSCONFDIR}/xdg/menus )
endif ()
