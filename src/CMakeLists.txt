set(kdevllm_core_SRCS
    ui/AiChatWidget.cpp
    ui/AiChatInputWidget.cpp
    network/LlamaClient.cpp
    context/ContextManager.cpp
    completion/AiCompletionModel.cpp
)

add_library(kdevllm_core STATIC ${kdevllm_core_SRCS})
set_property(TARGET kdevllm_core PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kdevllm_core
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

kdevplatform_add_plugin(kdevllm
    JSON kdevllm.json
    SOURCES KDevLLMPlugin.cpp
)

target_link_libraries(kdevllm
    kdevllm_core
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
