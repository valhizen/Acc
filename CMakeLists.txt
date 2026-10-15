cmake_minimum_required(VERSION 4.0)

project("GameEngine" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CM                          AKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)


set(IMGUI_SOURCES 
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/Vendors/imgui/backends/imgui_impl_vulkan.cpp
)

add_executable(GameEngine "src/main.cpp"
"Engine/Engine.cpp"
"${IMGUI_SOURCE}"
"Engine/Window.cpp"
"Engine/Renderer/VMAImpl.cpp"
"Engine/Renderer/Renderer.cpp"
"Engine/Renderer/Pipeline.cpp"
"Engine/Renderer/Rendering.cpp"
"Engine/Renderer/Resources.cpp"
"Engine/Renderer/Utils.cpp"
"Engine/Entity/Component.cpp"
"Engine/Entity/ECS.cpp"
"Engine/Entity/Components/Camera.cpp"
"Engine/Entity/Components/Transform.cpp"
"Engine/Core/Texture.cpp"

)

add_executable(Test "Test/main.cpp"
"Engine/Core/Texture.cpp"
)



target_include_directories(GameEngine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors/GLFW/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors/VMA
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors
  ${CMAKE_SOURCE_DIR}/vendors/imgui
  ${CMAKE_SOURCE_DIR}/vendors/imgui/backends
    
)

target_compile_definitions(GameEngine PRIVATE GLFW_DLL)

target_link_libraries(GameEngine PRIVATE
  Vulkan::Vulkan
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendors/GLFW/lib-vc2026/glfw3dll.lib"
)

target_include_directories(Test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors/GLFW/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors/VMA
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendors
)

target_compile_definitions(Test PRIVATE GLFW_DLL)

target_link_libraries(GameEngine PRIVATE
  Vulkan::Vulkan
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendors/GLFW/lib-vc2026/glfw3dll.lib"
)



add_custom_command(TARGET Test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/Vendors/GLFW/lib-vc2026/glfw3.dll"
    $<TARGET_FILE_DIR:GameEngine>
    )
