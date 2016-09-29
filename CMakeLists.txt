cmake_minimum_required (VERSION 3.0.2)

project(cpota)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(FLAGS ${FLAGS} -std=c++11)
endif()

add_executable(cpota ${SOURCES})

target_link_libraries(cpota PRIVATE ${LIBS})
target_compile_options(cpota PRIVATE ${FLAGS})

install(TARGETS cpota DESTINATION bin)
