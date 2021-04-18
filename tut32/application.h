/*
 * Application.h
 *
 *  Created on: 25.01.2021
 *      Author: rik
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>          										// printf, fprintf
#include <stdlib.h>         										// abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

class application {
public:
    void run();
private:
    int WIDTH 	= 1280;
    int HEIGHT = 720;
    GLFWwindow				 *window;
    ImGui_ImplVulkanH_Window *wd;
    VkInstance 				 instance;
    VkAllocationCallbacks*   g_Allocator 		= NULL;
    VkInstance               g_Instance 		= VK_NULL_HANDLE;
    VkPhysicalDevice         g_PhysicalDevice 	= VK_NULL_HANDLE;
    VkDevice                 g_Device 			= VK_NULL_HANDLE;
    uint32_t                 g_QueueFamily 		= (uint32_t)-1;
    VkQueue                  g_Queue 			= VK_NULL_HANDLE;
    VkDebugReportCallbackEXT g_DebugReport 		= VK_NULL_HANDLE;
    VkPipelineCache          g_PipelineCache 	= VK_NULL_HANDLE;
    VkDescriptorPool         g_DescriptorPool 	= VK_NULL_HANDLE;
    ImGui_ImplVulkanH_Window g_MainWindowData;
    int                      g_MinImageCount 	= 2;
    bool                     g_SwapChainRebuild  = false;
    //Methods....................................................
    void initWindow			();
    void initVulkan			();
    void mainLoop			();
    void cleanup			();
    void FramePresent		( ImGui_ImplVulkanH_Window* wd);
    void FrameRender		( ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
    void CleanupVulkanWindow();
    void CleanupVulkan		();
    void SetupVulkanWindow	( ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
    void SetupVulkan		( const char** extensions, uint32_t extensions_count);
    void createInstance		();

};

#endif /* APPLICATION_H_ */
