project("plugintest")

include(ECMAddTests)

ecm_add_test(plugintest.cpp main.cpp
             TEST_NAME kservice-pluginloadertest
	     LINK_LIBRARIES Qt5::Core KF5::Service)
