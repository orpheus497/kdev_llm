# CMakeLists.txt for testing

find_package(Qt6 REQUIRED COMPONENTS Test Core Network Widgets)

set(TEST_SRCS
    TestAiCompletionModel.cpp
    ../src/completion/AiCompletionModel.cpp
    ../src/network/LlamaClient.cpp
)

add_executable(TestAiCompletionModel ${TEST_SRCS})
target_link_libraries(TestAiCompletionModel Qt6::Test Qt6::Core Qt6::Network Qt6::Widgets)

find_package(KF6 REQUIRED COMPONENTS TextEditor CoreAddons)
target_link_libraries(TestAiCompletionModel KF6::TextEditor KF6::CoreAddons)

add_test(NAME TestAiCompletionModel COMMAND TestAiCompletionModel)
