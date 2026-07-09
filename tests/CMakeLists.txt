find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(TestContextManager TestContextManager.cpp ../src/context/ContextManager.cpp)
target_include_directories(TestContextManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(TestContextManager PRIVATE Qt6::Core Qt6::Test)

add_test(NAME TestContextManager COMMAND TestContextManager)
add_executable(TestAiCompletionModel ${TEST_SRCS})
set_target_properties(TestAiCompletionModel PROPERTIES AUTOMOC ON)

target_link_libraries(TestAiCompletionModel PRIVATE
    kdevllm
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

find_package(KF6 REQUIRED COMPONENTS TextEditor CoreAddons)
target_link_libraries(TestAiCompletionModel PRIVATE KF6::TextEditor KF6::CoreAddons)

add_test(NAME TestAiCompletionModel COMMAND TestAiCompletionModel)

set(TEST_CMD_SRCS
    TestCommandTextEdit.cpp
)
add_executable(TestCommandTextEdit ${TEST_CMD_SRCS})
set_target_properties(TestCommandTextEdit PROPERTIES AUTOMOC ON)
target_link_libraries(TestCommandTextEdit PRIVATE kdevllm Qt6::Test Qt6::Core Qt6::Widgets)
add_test(NAME TestCommandTextEdit COMMAND TestCommandTextEdit)
