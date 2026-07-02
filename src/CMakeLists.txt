set(jenovakdev_SRCS
    JenovaPlugin.cpp
    ui/AiChatWidget.cpp
    ui/AiChatInputWidget.cpp
    network/LlamaClient.cpp
    context/ContextManager.cpp
    completion/AiCompletionModel.cpp
)

kdevplatform_add_plugin(jenovakdev
    SOURCES ${jenovakdev_SRCS}
)

target_link_libraries(jenovakdev
    KDev::Interfaces
    KDev::Util
    KDev::Project
    KDev::Shell
    KDev::Language
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
