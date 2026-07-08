set(TestContextManager_SRCS
    TestContextManager.cpp
    ../src/context/ContextManager.cpp
)

find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(TestContextManager ${TestContextManager_SRCS})

target_link_libraries(TestContextManager
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
)

add_test(NAME TestContextManager COMMAND TestContextManager)
