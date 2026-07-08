set(CMAKE_AUTOMOC ON)

add_executable(TestLlamaClient
    TestLlamaClient.cpp
)

target_include_directories(TestLlamaClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

target_link_libraries(TestLlamaClient
    LlamaClientObj
    Qt6::Test
)

add_test(NAME TestLlamaClient COMMAND TestLlamaClient)
