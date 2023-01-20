include(ECMMarkNonGuiExecutable)

# We need to add a '5' so that kde3/kde4 apps running kbuildsycoca don't run this one.
add_executable(kbuildsycoca5)

# Mark it as non-gui so we won't create an app bundle on Mac OS X
ecm_mark_nongui_executable(kbuildsycoca5)

target_sources(kbuildsycoca5 PRIVATE
   kbuildsycoca_main.cpp
)

target_link_libraries(kbuildsycoca5
   KF6::Service
   KF6::CoreAddons # KAboutData
   KF6::I18n
   Qt${QT_MAJOR_VERSION}::Xml
)
install(TARGETS kbuildsycoca5 ${KF_INSTALL_TARGETS_DEFAULT_ARGS} )
