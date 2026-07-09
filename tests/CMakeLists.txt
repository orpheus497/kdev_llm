find_package(Qt6 REQUIRED COMPONENTS Core Test Widgets)
find_package(KF6 REQUIRED COMPONENTS TextEditor CoreAddons I18n)
find_package(KDevPlatform 6.0 REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# TestContextManager
add_executable(TestContextManager TestContextManager.cpp ../src/context/ContextManager.cpp)
target_include_directories(TestContextManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks ../src)
target_link_libraries(TestContextManager PRIVATE 
    Qt6::Core Qt6::Test 
    KF6::TextEditor 
    KDev::Interfaces KDev::Project KDev::Language
)
add_test(NAME TestContextManager COMMAND TestContextManager)

# TestCommandTextEdit
set(TEST_CMD_SRCS
    TestCommandTextEdit.cpp
    ../src/ui/AiChatInputWidget.cpp
)
add_executable(TestCommandTextEdit ${TEST_CMD_SRCS})
target_include_directories(TestCommandTextEdit PRIVATE ../src)
target_link_libraries(TestCommandTextEdit PRIVATE Qt6::Test Qt6::Core Qt6::Widgets KF6::TextEditor KF6::I18n)
add_test(NAME TestCommandTextEdit COMMAND TestCommandTextEdit)

# TestLlamaClient
add_executable(TestLlamaClient TestLlamaClient.cpp)
target_include_directories(TestLlamaClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks ../src)
target_link_libraries(TestLlamaClient LlamaClientObj Qt6::Test)
add_test(NAME TestLlamaClient COMMAND TestLlamaClient)
