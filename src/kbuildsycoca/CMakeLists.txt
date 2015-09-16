include(ECMMarkNonGuiExecutable)

set(kbuildsycoca_SRCS
   kbuildsycoca_main.cpp
   )

# We need to add a '5' so that kde3/kde4 apps running kbuildsycoca don't run this one.
add_executable(kbuildsycoca5 ${kbuildsycoca_SRCS})

# Mark it as non-gui so we won't create an app bundle on Mac OS X
ecm_mark_nongui_executable(kbuildsycoca5)

target_link_libraries(kbuildsycoca5
   KF5::Service
   KF5::Crash      # set a crash handler for the kbuildsycoca binary
   KF5::CoreAddons # KAboutData
   KF5::I18n
)

install(TARGETS kbuildsycoca5 ${KF5_INSTALL_TARGETS_DEFAULT_ARGS} )
