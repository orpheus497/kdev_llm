add_library(LlamaClientObj OBJECT network/LlamaClient.cpp)
target_include_directories(LlamaClientObj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/network)
target_link_libraries(LlamaClientObj PUBLIC Qt6::Core Qt6::Network)

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
