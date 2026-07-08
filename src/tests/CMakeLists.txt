find_package(Qt6 REQUIRED COMPONENTS Test)

set(TestCommandTextEdit_SRCS
    TestCommandTextEdit.cpp
    ../ui/AiChatInputWidget.cpp
)

add_executable(TestCommandTextEdit ${TestCommandTextEdit_SRCS})
set_target_properties(TestCommandTextEdit PROPERTIES AUTOMOC ON)

target_include_directories(TestCommandTextEdit PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(TestCommandTextEdit PRIVATE
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
    KF6::I18n
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

add_test(NAME TestCommandTextEdit COMMAND TestCommandTextEdit)
