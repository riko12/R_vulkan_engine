# Vulkan tutorial #
This tutorial is based on the official Vulkan tutorial
To use Vulkan in your application it is important to install it on the machine.
The runtime is a library which should be linked to your application.
I'm doing it, for example (ubuntu 18.04 machine),  like this:
```
target_link_libraries(tut04 vulkan
```  
Currently VulkanSDK used in this tutorial is version 1.2  
As known, Vulkan is an low level generic API software to achive performance and efficiency of the graphic card independently from hardware. 
Mean target of this tutorial is understanding of the architecture and methodology to build a graphic engine, vulkan based.
This tutorial, is build in the form of steps, which passing through as described below.

According to the Imgui integration I made use of the [tutorial:](https://frguthmann.github.io/posts/vulkan_imgui/ )

**Applications file system structure needed to be build with cmake**
* RikImguiVulkan
  * src
     * imgui
     main.cpp
  * include
    * glm
    * imgui
    * stb_image
  * libs
    * libglm_shared.so
    * libglm_static.a
    
```  
** How to call this cmake .. -Wno-dev **
cmake_minimum_required(VERSION 3.10)
project(rikvulkan_tuti)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-std=c++17)

option(TUTI   "Build all " 		TRUE)
option(DOXY   "Build Doxygen " 	FALSE)
option(CLEAN  "Cleaning... " 	TRUE)
option(SHADER "Shaders ... " 	TRUE)
include_directories("include")
include_directories("include/glm/gtx")
include_directories("include/stb_image")
include_directories("include/tiny_obj_loader")
include_directories("include/imgui")

if (SHADER)
	find_package(Vulkan REQUIRED)
	set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
	find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
	 find all the shader files under the shaders folder
	message(STATUS "Shader Folder is ${PROJECT_SOURCE_DIR}/shader")

	** Build the shaders start section-------------------- ** 
	message(STATUS "Start building the shaders...")
	file(GLOB_RECURSE GLSL_SOURCE_FILES
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.frag"
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.vert"
	    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/*.comp"
	    )
	
	** iterate each shader **
	foreach(GLSL ${GLSL_SOURCE_FILES})
	  message(STATUS "BUILDING SHADER")
	  get_filename_component(FILE_NAME ${GLSL} NAME)
	  set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${FILE_NAME}.spv")
	  message(STATUS ${GLSL})
	  ** execute glslang command to compile that specific shader **
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
** end of shader section-------------------- ** 
```
  
**To build all steps**

```
cd build
cmake .. -Wno-dev
make
```
or after at least one time **cmake**-call, **make tutx** where the x is representing the proper step number and tutx is the proper target witch you would like to build for example: 
```
make tut01
```

**Step 29**
This application will search all the physical gr-card contained in the machine  
Start programm: 
```
tut29  
```
