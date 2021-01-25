# How to call this cmake .. -Wno-dev
cmake_minimum_required(VERSION 3.10)
project(rikvulkan_tuti)
add_definitions(-std=c++17)

option(TUTI   "Build all " 		TRUE)
option(DOXY   "Build Doxygen " 	FALSE)
option(CLEAN  "Cleaning... " 	TRUE)
option(SHADER "Shaders ... " 	TRUE)

if (SHADER)
	find_package(Vulkan REQUIRED)
	set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
	find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
	## find all the shader files under the shaders folder
	message(STATUS "Shader Folder is ${PROJECT_SOURCE_DIR}/shader")

	# Build the shaders start section-------------------- 
	message(STATUS "Start building the shaders...")
	file(GLOB_RECURSE GLSL_SOURCE_FILES
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.frag"
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.vert"
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.comp"
	    )
	
	## iterate each shader
	foreach(GLSL ${GLSL_SOURCE_FILES})
	  message(STATUS "BUILDING SHADER")
	  get_filename_component(FILE_NAME ${GLSL} NAME)
	  set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${FILE_NAME}.spv")
	  message(STATUS ${GLSL})
	  ##execute glslang command to compile that specific shader
	  add_custom_command(
	    OUTPUT ${SPIRV}
	    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
	    DEPENDS ${GLSL})
	  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
	endforeach(GLSL)
	
	add_custom_target(
	    Shaders 
	    DEPENDS ${SPIRV_BINARY_FILES}
	)
endif(SHADER)
# end of shader section--------------------

if(CLEAN)
	# clean-all.cmake
	set(cmake_generated ${CMAKE_BINARY_DIR}/CMakeCache.txt
	                    ${CMAKE_BINARY_DIR}/cmake_install.cmake
	                    ${CMAKE_BINARY_DIR}/Makefile
	                    ${CMAKE_BINARY_DIR}/CMakeFiles
	)
	
	foreach(file ${cmake_generated})	
	  if (EXISTS ${file})
	     file(REMOVE_RECURSE ${file})
	  endif()
	endforeach(file)  
endif(statement)

add_custom_target(clean-all
   COMMAND ${CMAKE_BUILD_TOOL} clean
   COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)

if(${DOXY})
	# first we can indicate the documentation build as an option and set it to ON by default
	option(BUILD_DOC "Build documentation" ON)
	# check if Doxygen is installed
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
	    # set input and output files
	    set(DOXYGEN_OUT /home/rik/eclipse-ws-RikKI/Vulkan_tutOverv/VulkanTutorial/code/Doxyfile.conf)
	
	    # request to configure the file
	    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
	    message("Doxygen build started")
	
	    # note the option ALL which allows to build the docs together with the application
	    add_custom_target( doc_doxygen ALL
	        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
	       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	       COMMENT "Generating API documentation with Doxygen"
	        VERBATIM )
	else (DOXYGEN_FOUND)
	  message("Doxygen need to be installed to generate the doxygen documentation")
	endif (DOXYGEN_FOUND)
endif (DOXY)

if("${TUTI}")

	include_directories( ${R_INCLUDE_DIRS} )
	# add the executable
	add_executable(tut00 00_base_code.cpp)
	target_link_libraries(tut00 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut01 01_instance_creation.cpp)
	target_link_libraries(tut01 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut02 02_validation_layers.cpp)
	target_link_libraries(tut02 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut03 03_physical_device_selection.cpp)
	target_link_libraries(tut03 vulkan glfw3 dl X11 pthread)

	add_executable(tut04 04_log_dev.cpp)
	target_link_libraries(tut04 vulkan glfw3 dl X11 pthread)

	add_executable(tut05 05_window_surface.cpp)
	target_link_libraries(tut05 vulkan glfw3 dl X11 pthread)

	add_executable(tut06 06_swap_chain_creation.cpp)
	target_link_libraries(tut06 vulkan glfw3 dl X11 pthread)

	add_executable(tut07 07_image_views.cpp)
	target_link_libraries(tut07 vulkan glfw3 dl X11 pthread)

	add_executable(tut08 08_graphics_pipeline.cpp)
	target_link_libraries(tut08 vulkan glfw3 dl X11 pthread)

	add_executable(tut09 09_shader_modules.cpp)
	target_link_libraries(tut09 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut10 10_pipeline_functions.cpp)
	target_link_libraries(tut10 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut11 11_render_passes.cpp)
	target_link_libraries(tut11 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut12 12_graphics_pipeline_complete.cpp)
	target_link_libraries(tut12 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut13 13_framebuffers.cpp)
	target_link_libraries(tut13 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut14 14_command_buffers.cpp)
	target_link_libraries(tut14 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut15 15_hello_triangle.cpp)
	target_link_libraries(tut15 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut16 16_swap_chain_recreation.cpp)
	target_link_libraries(tut16 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut17 17_vertex_input.cpp)
	target_link_libraries(tut17 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut18 18_vertex_buffer.cpp)
	target_link_libraries(tut18 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut19 19_staging_buffer.cpp)
	target_link_libraries(tut19 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut20 20_index_buffer.cpp)
	target_link_libraries(tut20 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut21 21_descriptor_layout.cpp)
	target_link_libraries(tut21 vulkan glfw3 dl X11 pthread)
	
	add_executable(tut22 22_descriptor_sets.cpp)
	target_link_libraries(tut22 vulkan glfw3 dl X11 pthread)
	
	include_directories(${PROJECT_SOURCE_DIR})
	add_executable(tut23 23_texture_image.cpp)
	target_link_libraries(tut23 vulkan glfw3 dl X11 pthread)
	
	include_directories(${PROJECT_SOURCE_DIR})
	add_executable(tut24 24_sampler.cpp)
	target_link_libraries(tut24 vulkan glfw3 dl X11 pthread)
	
 	include_directories(${PROJECT_SOURCE_DIR})
	add_executable(tut25 25_texture_mapping.cpp)
	target_link_libraries(tut25 vulkan glfw3 dl X11 pthread)
	
 	include_directories(${PROJECT_SOURCE_DIR})
	add_executable(tut26 26_depth_buffering.cpp)
	target_link_libraries(tut26 vulkan glfw3 dl X11 pthread)
	
 	include_directories( ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/libs)
	add_executable(tut27 27_model_loading.cpp )
	target_link_libraries(tut27 vulkan glfw3 dl X11 pthread)
	
 	include_directories( ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/libs)
	add_executable(tut28 28_mipmapping.cpp )
	target_link_libraries(tut28 vulkan glfw3 dl X11 pthread)
	
 	include_directories( ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/libs)
	add_executable(tut29 29_multisampling.cpp )
	target_link_libraries(tut29 vulkan glfw3 dl X11 pthread)
	
else()
  
endif(statement)


# option(VAR "this is the VAR " OFF)
# function(SIMPLE REQUIRED_ARG)
#     message(STATUS "Simple arguments: ${REQUIRED_ARG}, followed by ${ARGV}")
#     set(${REQUIRED_ARG} "From SIMPLE" PARENT_SCOPE)
# endfunction()

# simple(This)
# message("Output: ${This}")



