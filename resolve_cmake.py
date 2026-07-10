import os

def resolve_file(filepath, content):
    with open(filepath, 'w') as f:
        f.write(content)

base_path = "/home/orpheus497/Documents/Projects/kdev_llm"

# 1. CMakeLists.txt
resolve_file(f"{base_path}/CMakeLists.txt", """cmake_minimum_required(VERSION 3.16)
project(kdevllm VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REQUIRED_KF_VERSION "6.0.0")
find_package(ECM ${REQUIRED_KF_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF6 ${REQUIRED_KF_VERSION} REQUIRED COMPONENTS
    I18n
    TextEditor
    CoreAddons
    XmlGui
    SyntaxHighlighting
    Config
)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

option(BUILD_TESTING "Build the testing tree" OFF)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

find_package(KDevPlatform 6.0 REQUIRED)

add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
""")

# 2. src/CMakeLists.txt
resolve_file(f"{base_path}/src/CMakeLists.txt", """add_library(LlamaClientObj OBJECT network/LlamaClient.cpp)
target_include_directories(LlamaClientObj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/network)
target_link_libraries(LlamaClientObj PUBLIC Qt6::Core Qt6::Network KF6::ConfigCore)

set(kdevllm_SRCS
    KDevLLMPlugin.cpp
    ui/AiChatWidget.cpp
    ui/AiChatInputWidget.cpp
    context/ContextManager.cpp
    completion/AiCompletionModel.cpp
)

kdevplatform_add_plugin(kdevllm
    JSON kdevllm.json
    SOURCES ${kdevllm_SRCS} $<TARGET_OBJECTS:LlamaClientObj>
)

target_link_libraries(kdevllm
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
""")

# 3. tests/CMakeLists.txt
resolve_file(f"{base_path}/tests/CMakeLists.txt", """find_package(Qt6 REQUIRED COMPONENTS Core Test Widgets Network)
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

# TestAiCompletionModel
set(TEST_COMPLETION_SRCS
    TestAiCompletionModel.cpp
    ../src/completion/AiCompletionModel.cpp
)
add_executable(TestAiCompletionModel ${TEST_COMPLETION_SRCS})
target_include_directories(TestAiCompletionModel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks ../src)
target_link_libraries(TestAiCompletionModel PRIVATE
    Qt6::Test Qt6::Core Qt6::Network Qt6::Widgets
    KF6::TextEditor KF6::CoreAddons
)
add_test(NAME TestAiCompletionModel COMMAND TestAiCompletionModel)

# TestAiChatWidget
set(TEST_WIDGET_SRCS
    TestAiChatWidget.cpp
    ../src/ui/AiChatWidget.cpp
)
add_executable(TestAiChatWidget ${TEST_WIDGET_SRCS})
target_include_directories(TestAiChatWidget PRIVATE ../src)
target_link_libraries(TestAiChatWidget PRIVATE
    Qt6::Test Qt6::Core Qt6::Widgets Qt6::Network
    KDev::Interfaces KDev::Util KDev::Project KDev::Shell KDev::Language
)
add_test(NAME TestAiChatWidget COMMAND TestAiChatWidget)
set_tests_properties(TestAiChatWidget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
""")

print("Saved primary resolved files.")
