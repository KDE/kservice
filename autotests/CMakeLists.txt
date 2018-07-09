remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMAddTests)

########### unittests ###############

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Concurrent Test)

macro(KSERVICE_UNIT_TESTS)
  foreach(_testname ${ARGN})
    ecm_add_test(${_testname}.cpp TEST_NAME ${_testname}
        LINK_LIBRARIES KF5::Service Qt5::Test Qt5::Concurrent Qt5::Xml)
    target_compile_definitions(${_testname} PRIVATE -DKBUILDSYCOCAEXE=\"$<TARGET_FILE:kbuildsycoca5>\")
  endforeach()
endmacro(KSERVICE_UNIT_TESTS)

kservice_unit_tests(
 ksycocatest
 ksycoca_xdgdirstest
 kautostarttest
 ksycocadicttest
 ksycocathreadtest
 kservicetest
 pluginlocatortest
 kplugininfotest
)

# we want to test the deprecated API without a (useless) warning
target_compile_definitions(kplugininfotest PRIVATE KSERVICE_DEPRECATED=)

# the test plays with the timestamp of ~/.qttest/share/kservicetypes5, and with the ksycoca file, other tests can collide
set_tests_properties(ksycocatest PROPERTIES RUN_SERIAL TRUE)
# KServiceTest::testAllServices can fail if any service is deleted while the test runs
set_tests_properties(kservicetest PROPERTIES RUN_SERIAL TRUE)

target_sources(kservicetest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/ktraderparsetree.cpp
)

add_library(fakeplugin MODULE nsaplugin.cpp)
ecm_mark_as_test(fakeplugin)
target_link_libraries(fakeplugin KF5::Service)


# generate new-style and old-style JSON from the .desktop file
file(COPY fakeplugin.desktop DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin.desktop ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin_json_old.desktop)
kcoreaddons_desktop_to_json(fakeplugin ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin_json_old.desktop COMPAT_MODE)

file(COPY fakeplugin.desktop DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin.desktop ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin_json_new.desktop)
kcoreaddons_desktop_to_json(fakeplugin ${CMAKE_CURRENT_BINARY_DIR}/fakeplugin_json_new.desktop)

######### kmimeassociationstest ########

set(kmimeassociationstest_SRCS kmimeassociationstest.cpp ../src/sycoca/kmimeassociations.cpp)
ecm_qt_declare_logging_category(kmimeassociationstest_SRCS HEADER sycocadebug.h IDENTIFIER SYCOCA CATEGORY_NAME kf5.kservice.sycoca)

ecm_add_test(${kmimeassociationstest_SRCS} TEST_NAME kmimeassociationstest
             LINK_LIBRARIES KF5::Service Qt5::Test Qt5::Xml)

