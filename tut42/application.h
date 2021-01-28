/*
 * Application.h
 * Add CreateDebugUtilsMessengerEXT and DestroyDebugUtilsMessengerEXT
 * Create the member VkDebugUtilsMessengerEXT debugMessenger in the header file
 * Add populateDebugMessengerCreateInfo, setupDebugMessenger, getRequiredExtensions, checkValidationLayerSupport
 * Created on: 25.01.2021
 * Author: rik
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
#include <vector>
#include <optional>

using namespace std;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
    	return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR 		capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> 	presentModes;
};

class application {
public:
    void run();
private:
    int WIDTH 	= 1280;
    int HEIGHT = 720;
    GLFWwindow				 *window;
    ImGui_ImplVulkanH_Window *wd;
    VkPhysicalDevice 		 physicalDevice 	= VK_NULL_HANDLE;
    VkDevice 				 device;			//Logical device
    VkQueue 				 graphicsQueue;     //Graphic Queue
    VkQueue 				 presentQueue;		//Presentation queue
    QueueFamilyIndices 		 findQueueFamilies	(VkPhysicalDevice device);
    bool 					 isDeviceSuitable	(VkPhysicalDevice device);
    void 					 pickPhysicalDevice	();
    void 					 createSurface		();
    VkInstance 				 instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR 			 surface;
    //Swapchain....................................................
    VkSwapchainKHR 			 swapChain;
    std::vector<VkImage> 	 swapChainImages;
    VkFormat 				 swapChainImageFormat;
    VkExtent2D 				 swapChainExtent;
	SwapChainSupportDetails	 querySwapChainSupport	(VkPhysicalDevice device);
    VkExtent2D				 chooseSwapExtent		(const VkSurfaceCapabilitiesKHR& capabilities);
    VkPresentModeKHR		 chooseSwapPresentMode	(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkSurfaceFormatKHR		 chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	void					 createSwapChain		();
	bool 					 checkDeviceExtensionSupport(VkPhysicalDevice device);
	std::vector<VkImageView> swapChainImageViews;
	void 					 createImageViews		();
	VkShaderModule 			 createShaderModule		(const std::vector<char>& code);
	void 					 createGraphicsPipeline	();
	VkPipelineLayout		 pipelineLayout;
    VkRenderPass 			 renderPass;
    VkPipeline 				 graphicsPipeline;
    //....................................................
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
    bool                     g_SwapChainRebuild = false;
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
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void createLogicalDevice();
    void createRenderPass	();
};

#endif /* APPLICATION_H_ */
