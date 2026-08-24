cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("VulkanBaseProject")

add_executable(VulkanBaseProject
	src/main.cpp
	src/Application.hpp
	src/Window.hpp
	src/Window.cpp
	src/Config.hpp
	src/QueueFamily.hpp
	src/QueueFamily.cpp
	src/Device.hpp
	src/Device.cpp
	src/SwapChainSupport.hpp
	src/SwapChainSupport.cpp
	src/ImageUtils.hpp
	src/ImageUtils.cpp
	src/Swapchain.hpp
	src/Swapchain.cpp
	src/CommandUtils.hpp
	src/CommandUtils.cpp
	src/Image.hpp
	src/Image.cpp
	src/Vertex.hpp
	src/Debugging.hpp
	src/Utils.hpp
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VulkanBaseProject PROPERTY CXX_STANDARD 20)
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

target_include_directories(VulkanBaseProject PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(VulkanBaseProject PRIVATE
	Vulkan::Vulkan
	glfw
	glm::glm
	tinyobjloader::tinyobjloader
)