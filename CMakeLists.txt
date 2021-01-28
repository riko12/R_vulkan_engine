# https://github.com/ocornut/imgui
cmake_minimum_required(VERSION 3.10)
project(imgui_example_glfw_vulkan C CXX)
# Compiler -------------------------
add_definitions(-DIMGUI_VULKAN_DEBUG_REPORT -DIMGUI_VULKAN_DEBUG_REPORT)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-std=c++17)
# Flags------------------------------------
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set (CMAKE_C_FLAGS 					"${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS 				"${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")
set (LIBRARIES 						"vulkan;glfw3;dl;X11;pthread")
set (CMAKE_BINARY_DIR 				"${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${PROJECT_SOURCE_DIR}/bin")
set (IMGUI_DIR 						include/imgui)										# Dear ImGui
set (PROJ_GLM_GTX 					"include/glm/gtx")
set (PROJ_INCL_DIR 					${PROJECT_SOURCE_DIR}/include)

message (STATUS "PROJ_INCL_DIR: ${PROJ_INCL_DIR}")
message (STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
# include_directories("include/glm/gtx")

# Packages
# GLFW3----Package----------------------------
find_package(glfw3)
# vulkan----Package----------------------------
find_package(Vulkan REQUIRED)
include_directories(${glfw3_DIR}/include)
# Including --------------------------------------------
include_directories(
					${PROJ_INCL_DIR}
					${IMGUI_DIR} 
					${IMGUI_DIR}/backends 
					${PROJ_INCL_DIR}/stb_image 
					${PROJ_INCL_DIR}/tiny_obj_loader 
					${PROJ_GLM_GTX} 						)
include_directories(${GLFW_DIR}/deps)

file(GLOB ${PROJECT_SOURCE_DIR}/ *.cpp)
# Show all CMAKE vars and att--------------------
# message (STATUS "All Vars and attrs")
get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
	# message(STATUS "${_variableName}=${${_variableName}}")
	endforeach() 

SET(MYLIST 30 31 32 33 34 35 36 37 41 42 45)
foreach (i ${MYLIST})
	message (STATUS "tut${i}")
	add_executable(tut${i} 
				${PROJECT_SOURCE_DIR}/tut${i}/main.cpp 
				${PROJECT_SOURCE_DIR}/tut${i}/application.h 
				${PROJECT_SOURCE_DIR}/tut${i}/application.cpp 
				${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
				${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp 
				${IMGUI_DIR}/imgui.cpp 
				${IMGUI_DIR}/imgui_draw.cpp 
				${IMGUI_DIR}/imgui_demo.cpp 
				${IMGUI_DIR}/imgui_tables.cpp 
				${IMGUI_DIR}/imgui_widgets.cpp			)
	target_link_libraries(tut${i} ${LIBRARIES})
endforeach() 

#------------------------------------------------------
# add_subdirectory(${PROJECT_SOURCE_DIR}/theguide/third_party)
# add_library(vkbootstrap STATIC)
# add_library(glm INTERFACE)
# add_library(vma INTERFACE)
# add_library(stb_image INTERFACE)
# add_library(tinyobjloader STATIC)
# target_sources(vkbootstrap PRIVATE 
#   vkbootstrap/VkBootstrap.h
#   vkbootstrap/VkBootstrap.cpp
#   )
# target_include_directories(vkbootstrap PUBLIC vkbootstrap)
# target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

# target_include_directories(vkbootstrap PUBLIC vkbootstrap)

# include_directories(${PROJECT_SOURCE_DIR}/theguide/third_party)
# SET(MYLIST2 46)
# foreach (i ${MYLIST2})
#	include_directories(${PROJECT_SOURCE_DIR}/tut${i})
#	message (STATUS "tut${i}")
#	add_executable(tut${i} 
##		${PROJECT_SOURCE_DIR}/tut${i}/main.cpp 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_engine.h 
#	#	${PROJECT_SOURCE_DIR}/tut${i}/vk_engine.cpp
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_initializers.h 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_initializers.cpp 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_mesh.h 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_mesh.cpp 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_textures.h 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_textures.cpp 
#		${PROJECT_SOURCE_DIR}/tut${i}/vk_types.h		
#		${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
#		${IMGUI_DIR}/backends/imgui_impl_vulkan.h 
#		${IMGUI_DIR}/imgui.cpp 
#		${IMGUI_DIR}/imgui_draw.cpp 
#		${IMGUI_DIR}/imgui_demo.cpp 
#		${IMGUI_DIR}/imgui_tables.cpp 
#		${IMGUI_DIR}/imgui_widgets.cpp			)
#	target_link_libraries(tut${i} ${LIBRARIES})
#endforeach() 

