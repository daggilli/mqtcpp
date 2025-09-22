set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/subscriber/$<CONFIG>")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

add_executable(mqttsubscriber mqttsubscriber.cpp)

target_compile_options(mqttsubscriber PRIVATE
  $<$<CONFIG:Debug>:-g -O1 -fno-omit-frame-pointer>
  $<$<CONFIG:Release>: -O4 -DNODEBUG -ffunction-sections -fdata-sections -fno-plt>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_ASAN}>>:-fsanitize=address>
)

target_link_options(mqttsubscriber PRIVATE
  $<$<CONFIG:Release>: -Wl,--gc-sections -Wl,--as-needed>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_ASAN}>>:-fsanitize=address>
)

target_link_libraries(mqttsubscriber PRIVATE ${PROJECT_NAME})
target_link_libraries(mqttsubscriber PRIVATE PahoMqttCpp::paho-mqttpp3 ${PAHO_C_TARGET})
