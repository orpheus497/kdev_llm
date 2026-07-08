find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(TestContextManager TestContextManager.cpp ../src/context/ContextManager.cpp)
target_include_directories(TestContextManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(TestContextManager PRIVATE Qt6::Core Qt6::Test)

add_test(NAME TestContextManager COMMAND TestContextManager)
