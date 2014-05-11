remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMAddTests)

########### unittests ###############

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Concurrent Test)

macro(KSERVICE_UNIT_TESTS)
  foreach(_testname ${ARGN})
    ecm_add_test(${_testname}.cpp kwaitforsignal.cpp TEST_NAME ${_testname}
    NAME_PREFIX "kservice-"
    LINK_LIBRARIES KF5::Service Qt5::Test Qt5::Concurrent)
  endforeach()
endmacro(KSERVICE_UNIT_TESTS)

kservice_unit_tests(
 kautostarttest
 ksycocadicttest
 ksycocathreadtest
 kservicetest
 pluginlocatortest
)

add_library(fakeplugin MODULE nsaplugin.cpp)
ecm_mark_as_test(fakeplugin)
target_link_libraries(fakeplugin KF5::Service)


######### kmimeassociationstest ########

set(kmimeassociationstest_SRCS kmimeassociationstest.cpp ../src/kbuildsycoca/kmimeassociations.cpp)

ecm_add_test(${kmimeassociationstest_SRCS} TEST_NAME kmimeassociationstest
             NAME_PREFIX "kservice-" LINK_LIBRARIES KF5::Service Qt5::Test)
target_include_directories(kservice-kmimeassociationstest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/kbuildsycoca/")

