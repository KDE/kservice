include(ECMAddTests)

########### unittests ###############

find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Concurrent Test)

macro(KSERVICE_UNIT_TESTS)
  foreach(_testname ${ARGN})
    ecm_add_test(${_testname}.cpp TEST_NAME ${_testname}
        LINK_LIBRARIES KF5::Service Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Concurrent Qt${QT_MAJOR_VERSION}::Xml)
    target_compile_definitions(${_testname} PRIVATE -DKBUILDSYCOCAEXE=\"$<TARGET_FILE:kbuildsycoca5>\")
  endforeach()
endmacro(KSERVICE_UNIT_TESTS)

kservice_unit_tests(
 ksycocatest
 ksycoca_xdgdirstest
 ksycocadicttest
 ksycocathreadtest
 kservicetest
 kapplicationtradertest
)

if(UNIX AND
  NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.89.0)
  kservice_unit_tests(ktoolinvocation_x11test)
endif()

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.82.0)
   kservice_unit_tests(kplugintradertest)
endif()
if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.87.0)
   kservice_unit_tests(kautostarttest)
endif()

# the test plays with the timestamp of ~/.qttest/share/kservicetypes5, and with the ksycoca file, other tests can collide
set_tests_properties(ksycocatest PROPERTIES RUN_SERIAL TRUE)
# KServiceTest::testAllServices can fail if any service is deleted while the test runs
set_tests_properties(kservicetest PROPERTIES RUN_SERIAL TRUE)
# This test deletes ~/.qttest/share/applications
set_tests_properties(kapplicationtradertest PROPERTIES RUN_SERIAL TRUE)

target_sources(kapplicationtradertest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/services/ktraderparsetree.cpp
)

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.90.0)
    kservice_unit_tests(kplugininfotest)

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
endif()

######### kmimeassociationstest ########

set(kmimeassociationstest_SRCS kmimeassociationstest.cpp ../src/sycoca/kmimeassociations.cpp)
ecm_qt_declare_logging_category(kmimeassociationstest_SRCS
    HEADER sycocadebug.h
    IDENTIFIER SYCOCA
    CATEGORY_NAME kf.service.sycoca
)

ecm_add_test(${kmimeassociationstest_SRCS} TEST_NAME kmimeassociationstest
             LINK_LIBRARIES KF5::Service Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml)

