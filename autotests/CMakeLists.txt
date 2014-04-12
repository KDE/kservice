remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMMarkAsTest)

########### unittests ###############

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Concurrent Test)

macro(KSERVICE_UNIT_TESTS)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp kwaitforsignal.cpp)
    add_test(kservice-${_testname} ${_testname})
    target_link_libraries(${_testname}
       KF5::Service
       Qt5::Test
       Qt5::Concurrent
    )
    ecm_mark_as_test(${_testname})
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

add_executable(kmimeassociationstest ${kmimeassociationstest_SRCS})
add_test(kservice-kmimeassociationstest kmimeassociationstest)
ecm_mark_as_test(kmimeassociationstest)

target_link_libraries(kmimeassociationstest KF5::Service Qt5::Test)
target_include_directories(kmimeassociationstest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/kbuildsycoca/")

