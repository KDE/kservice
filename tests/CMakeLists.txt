if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.82.0)
    add_subdirectory(pluginlocator)
endif()

include(ECMMarkAsTest)

find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

macro(kservice_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    target_link_libraries(${_testname} KF5::Service)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro()

kservice_executable_tests(
  kservicegroup_dumper
  findservice
)
if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.61.0)
  kservice_executable_tests(
    kdbusservicestartertest
  )
endif()

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.82.0)
  kservice_executable_tests(
    startserviceby
  )
endif()

add_executable(kmimeassociations_dumper)
ecm_mark_as_test(kmimeassociations_dumper)

ecm_qt_declare_logging_category(kmimeassociations_dumper
    HEADER sycocadebug.h
    IDENTIFIER SYCOCA
    CATEGORY_NAME kf.service.sycoca
)
target_sources(kmimeassociations_dumper PRIVATE
    kmimeassociations_dumper.cpp
    ../src/sycoca/kmimeassociations.cpp
)

target_link_libraries(kmimeassociations_dumper KF5::Service)
