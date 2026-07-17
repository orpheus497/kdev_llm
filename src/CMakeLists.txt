add_library(LlamaClientObj OBJECT network/LlamaClient.cpp)
target_include_directories(LlamaClientObj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/network)
target_link_libraries(LlamaClientObj PUBLIC Qt6::Core Qt6::Network KF6::ConfigCore)

set(kdevllm_SRCS
    KDevLLMPlugin.cpp
    ui/AiChatWidget.cpp
    ui/AiChatInputWidget.cpp
    ui/ChatMessageModel.cpp
    ui/ChatMessageDelegate.cpp
    context/ContextManager.cpp
    completion/AiCompletionModel.cpp
    config/AiConfigPage.cpp
    storage/ChatDatabase.cpp
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
    KF6::ConfigCore
    KF6::I18n
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
