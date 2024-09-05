cmake_minimum_required(VERSION 3.10)
project(MultiprocessTest)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install/")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON) 

  #set(BOOST_ROOT "${PCL_ROOT}/3rdParty/Boost" CACHE PATH "Boost root directory")
 
  # use static Boost in Windows
  if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC ON)
    set(Boost_USE_MULTITHREAD )
  endif()
  # set(Boost_ADDITIONAL_VERSIONS
  #   "1.74.0" "1.74"
  #   "1.74.0" "1.74" "1.73.0" "1.73" "1.72.0" "1.72" "1.71.0" "1.71" "1.70.0" "1.70"
  #   "1.69.0" "1.69" "1.68.0" "1.68" "1.67.0" "1.67" "1.66.0" "1.66" "1.65.1" "1.65.0" "1.65"
  #   "1.64.0" "1.64" "1.63.0" "1.63" "1.62.0" "1.62" "1.61.0" "1.61" "1.60.0" "1.60"
  #   "1.59.0" "1.59" "1.58.0" "1.58" "1.57.0" "1.57" "1.56.0" "1.56" "1.55.0" "1.55")
  # Disable the config mode of find_package(Boost)
  #set(Boost_NO_BOOST_CMAKE ON)
  find_package(Boost 1.55.0 REQUIRED COMPONENTS system filesystem date_time iostreams serialization)

  set(BOOST_FOUND ${Boost_FOUND})
  set(BOOST_INCLUDE_DIRS "${Boost_INCLUDE_DIR}")
  set(BOOST_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}")
  set(BOOST_LIBRARIES ${Boost_LIBRARIES})
  if(WIN32 AND NOT MINGW)
    set(BOOST_DEFINITIONS ${BOOST_DEFINITIONS} -DBOOST_ALL_NO_LIB)
  endif()
find_package(OpenCV REQUIRED)


add_executable(parent ${CMAKE_SOURCE_DIR}/src/parent.cpp)
add_executable(child ${CMAKE_SOURCE_DIR}/src/child.cpp)

add_compile_definitions(${BOOST_DEFINITIONS})
INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
target_link_libraries(parent ${BOOST_LIBRARIES} ${OpenCV_LIBS})
target_link_libraries(child ${BOOST_LIBRARIES} ${OpenCV_LIBS})

