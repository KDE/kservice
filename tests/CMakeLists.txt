remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)

add_subdirectory(pluginlocator)

include(ECMMarkAsTest)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

macro(kservice_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    target_link_libraries(${_testname} KF5::Service)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro()

kservice_executable_tests(
  startserviceby
  kdbusservicestartertest
  kservicegroup_dumper
  findservice
)

add_executable(kmimeassociations_dumper kmimeassociations_dumper.cpp ../src/sycoca/kmimeassociations.cpp)
target_link_libraries(kmimeassociations_dumper KF5::Service)
ecm_mark_as_test(kmimeassociations_dumper)
