cmake_minimum_required(VERSION 3.16)
project(RailSetu LANGUAGES CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(railsetu
    src/Logger.cpp
    src/Persistence.cpp
    src/BookingManager.cpp
    src/ConsoleUi.cpp
)
target_include_directories(railsetu PUBLIC include)
target_compile_options(railsetu PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Werror>
)

add_executable(railsetu_app src/main.cpp)
target_link_libraries(railsetu_app PRIVATE railsetu Threads::Threads)

add_executable(railsetu_stress src/stress.cpp)
target_link_libraries(railsetu_stress PRIVATE railsetu Threads::Threads)

enable_testing()
add_executable(railsetu_tests tests/test_booking.cpp)
target_link_libraries(railsetu_tests PRIVATE railsetu Threads::Threads)
add_test(NAME railsetu_tests COMMAND railsetu_tests)

set(RAILSETU_FOCUSED_TESTS
    test_concurrent_booking
    test_cancellation
    test_waiting_promotion
    test_semaphore
    test_persistence
    test_race_validation
    test_visual_features
)
foreach(test_name IN LISTS RAILSETU_FOCUSED_TESTS)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE railsetu Threads::Threads)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
