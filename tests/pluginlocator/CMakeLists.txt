project("plugintest")

include(ECMAddTests)

ecm_add_test(plugintest.cpp main.cpp
             TEST_NAME kservice-pluginloadertest
	     LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core KF5::Service)
