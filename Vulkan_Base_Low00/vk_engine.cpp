/*
 * vk_engine.cpp
 *
 *  Created on: 25.02.2021
 *      Author: rik
 */
#include "vk_engine.h"
#include <VulkanDebug.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <rallcallbacks.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/hash.hpp>

#if defined(VK_USE_GLFW3)
	#define GLFW_INCLUDE_VULKAN
	#include <GLFW/glfw3.h>
#elif defined(VK_USE_SDL)
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_vulkan.h>
#endif

#include <imgui_impl_sdl.h>
#include <iostream>

//#define VMA_IMPLEMENTATION
//#include "vk_mem_alloc.h"

//we want to immediately abort when there is an error. In normal engines this would give an error message to the user, or perform a dump of state.
using namespace std;
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)


std::vector<const char*> VulkanEngine::args;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
#if defined(VK_USE_GLFW3)
    return VK_FALSE;		glfwSetErrorCallback(error_callback);
#elif defined(VK_USE_SDL)
#endif
}

#if defined(VK_USE_GLFW3)
static void onWindowResised(GLFWwindow * window, int w, int h){
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
	if (app->width == 0 || app->height == 0) return; //I am doing nothing
	app->width 				= w;
	app->height 			= h;
	//app->recreateSwapChain	();
    app->framebufferResized = true;
}
#endif

void VulkanEngine::drawFrame() {

    //now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

//    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
//    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
//	vkAcquireNextImageKHR(_device, _swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	vkAcquireNextImageKHR(_device, _swapChain, std::numeric_limits<uint64_t>::max(), _frames[imageIndex]._presentSemaphore, VK_NULL_HANDLE, &imageIndex);
	//ImGui::Render();
	VkSubmitInfo submitInfo 				= {};
	submitInfo.sType 						= VK_STRUCTURE_TYPE_SUBMIT_INFO;

//	VkSemaphore waitSemaphores[] 			= {imageAvailableSemaphore};
	VkSemaphore waitSemaphores[] 			= {_frames[imageIndex]._presentSemaphore};
	VkPipelineStageFlags waitStages[] 		= {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount 			= 1;
	submitInfo.pWaitSemaphores 				= waitSemaphores;
	submitInfo.pWaitDstStageMask 			= waitStages;

	submitInfo.commandBufferCount 			= 1;
	submitInfo.pCommandBuffers 				= &commandBuffers[imageIndex];

//	VkSemaphore signalSemaphores[] 			= {renderFinishedSemaphore};
	VkSemaphore signalSemaphores[] 			= {_frames[imageIndex]._renderSemaphore};
	submitInfo.signalSemaphoreCount 		= 1;
	submitInfo.pSignalSemaphores 			= signalSemaphores;

	if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkSwapchainKHR swapChains[] 			= {_swapChain};

	VkPresentInfoKHR presentInfo 			= {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount 			= 1;
	presentInfo.pWaitSemaphores 			= signalSemaphores;
	presentInfo.swapchainCount 				= 1;
	presentInfo.pSwapchains 				= swapChains;
	presentInfo.pImageIndices 				= &imageIndex;
	vkQueuePresentKHR						(presentQueue, &presentInfo);

    //wait until the GPU has finished rendering the last frame. Timeout of 1 second
//	VK_CHECK(vkWaitForFences				(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
//	VK_CHECK(vkResetFences					(_device, 1, &get_current_frame()._renderFence));

}

void VulkanEngine::init()
{
#if defined(VK_USE_GLFW3)
		glfwInit						();
		glfwWindowHint					(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint					(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow		(width, height, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer		(_window, this);
		glfwSetKeyCallback				(_window, key_callback);
		if (!_window)
		{
			glfwTerminate();
			//exit(EXIT_FAILURE);
			std::cerr << "Error: It was not possible to create the GLFWWindow \"" << std::endl;
		}
		glfwSetCursorEnterCallback		(_window, cursor_enter_callback);
		glfwSetMouseButtonCallback		(_window, mouse_button_callback);
		glfwSetWindowSizeCallback		(_window, onWindowResised);
	//    glfwMakeContextCurrent			(_window);
		glfwSetFramebufferSizeCallback	(_window, framebufferResizeCallback);
		glfwSetErrorCallback			(error_callback);
#elif defined(VK_USE_SDL)
    	SDL_Init(SDL_INIT_VIDEO);							// We initialize SDL and create a window with it.
    	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
    	_window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, window_flags );
#endif
	init_vulkan();
    if (settings.validation)							// If requested, we enable the default validation layers for debugging
	{
		VK_CHECK_RESULT(createInstance(settings.validation));
		setupDebugMessenger();
		// The report flags determine what type of messages for the layers will be displayed
		// For validating (debugging) an application the error and warning bits should suffice
//		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
//		// Additional flags include performance info, loader and layer debug messages, etc.
//		BRE::debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
	}

	pickPhysicalDevice					();

	createLogicalDevice					();
	getEnabledFeatures					();
	createSwapChain						();
	createImageViews					();
	// ------------>Baustelle ---> createBuffer <-----------------
	createRenderPass					();
	createGraphicsPipeline				();


	createDescriptorSetLayout			();
	createDescriptorPool				();
	createUniformBuffers				();
	createDescriptorSet					();

	createFramebuffers					();
	createCommandPool					();
	createCommandBuffers				();
	init_commands						();

	//createSemaphores					();
	init_sync_structures				();
	//init_imgui						();
	//prepare								();
	_isInitialized 						= true;								//everything went fine
}

void VulkanEngine::cleanupSwapChain(){
//    vkDestroyImageView		(_device, depthImageView, nullptr);
//    vkDestroyImage			(_device, depthImage, nullptr);
//    vkFreeMemory			(_device, depthImageMemory, nullptr);
//    vkDestroyImageView		(_device, colorImageView, nullptr);
//    vkDestroyImage			(_device, colorImage, nullptr);
//    vkFreeMemory			(_device, colorImageMemory, nullptr);
//    for (auto framebuffer : swapChainFramebuffers) {
//        vkDestroyFramebuffer(_device, framebuffer, nullptr);
//    }
//
//    vkFreeCommandBuffers	(_device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
//    vkDestroyPipeline		(_device, graphicsPipeline, nullptr);
//    vkDestroyPipelineLayout	(_device, pipelineLayout, nullptr);
//    vkDestroyRenderPass		(_device, renderPass, nullptr);
//    for (auto imageView : swapChainImageViews) {
//        vkDestroyImageView	(_device, imageView, nullptr);
//    }
//
//    vkDestroySwapchainKHR	(_device, _swapChain, nullptr);
//    for (size_t i = 0; i < swapChainImages.size(); i++) {
//        vkDestroyBuffer		(_device, uniformBuffers[i], nullptr);
//        vkFreeMemory		(_device, uniformBuffersMemory[i], nullptr);
//    }
//    vkDestroyDescriptorPool	(_device, descriptorPool, nullptr);

}

void VulkanEngine::cleanup()
{
	vkDeviceWaitIdle					(_device);//make sure the gpu has stopped doing its things
	_mainDeletionQueue.flush			();

	cleanupSwapChain();
	//make sure the GPU has stopped doing its things
	vkWaitForFences						(_device, 1, &_uploadContext._uploadFence, true, 1000000000);

	vkDestroySemaphore					(_device, renderFinishedSemaphore, 	nullptr	);
	vkDestroySemaphore					(_device, imageAvailableSemaphore, 	nullptr	);
	//vkDestroyCommandPool				(_device, _uploadContext._commandPool, 				nullptr	);

	_mainDeletionQueue.flush			();

	deletionFrameBuffersQueue.flush		();

	vkDestroyDescriptorSetLayout		(_device, descriptorSetLayout, 		nullptr	);

	vkDestroyPipeline					(_device, graphicsPipeline, 		nullptr	);
	vkDestroyPipelineLayout				(_device, pipelineLayout, 			nullptr	);

	//createVertices						(settings.useStagingBuffers				);

	vkDestroyRenderPass					(_device, renderPass, 				nullptr	);

	deletionImageViewsQueue.flush		();
	vkDestroySwapchainKHR				(_device, 	_swapChain, 				nullptr	);

	vkDestroyDescriptorPool				(_device, descriptorPool, 			nullptr	);

	vkDestroySurfaceKHR					(_instance, _surface, 			nullptr	);
	vkDestroyDevice						(_device, nullptr);

	if (this->settings.validation) 		{
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
//		DestroyDebugReportCallbackEXT	(_instance, 	m_DebugReport, m_Allocator		);
//		DestroyDebugUtilsMessengerEXT	(_instance, 	_debug_messenger, m_Allocator		);
	}

	vkDestroyInstance(_instance, nullptr);

#if defined(VK_USE_GLFW3)
    glfwDestroyWindow					(_window);
    glfwTerminate						();
#elif defined(VK_USE_SDL)
    SDL_DestroyWindow					(_window);
#endif

}

void VulkanEngine::run()
{
	bool bQuit = false;
	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

#if defined(VK_USE_GLFW3)
	while (!glfwWindowShouldClose(_window) && bQuit != true) {
		glfwPollEvents();
// Imgui-----------------------------------------------------------------------------------
        // Resize swap chain?
//        if (imGui->g_SwapChainRebuild)
//        {
//            int width, height;
//            glfwGetFramebufferSize(_window, &width, &height);
//            if (width > 0 && height > 0)
//            {
//                ImGui_ImplVulkan_SetMinImageCount(imGui->g_MinImageCount);
//                ImGui_ImplVulkanH_CreateOrResizeWindow(
//                		_instance,
//						_chosenGPU,
//						_device,
//						&imGui->g_MainWindowData,
//						queueFamilyIndices.presentFamily,
//						nullptr,
//						width,
//						height,
//						imGui->g_MinImageCount);
//                imGui->g_MainWindowData.FrameIndex = 0;
//                imGui->g_SwapChainRebuild = false;
//            }
//        }

        // Start the Dear ImGui frame
//        ImGui_ImplVulkan_NewFrame();
//        ImGui_ImplGlfw_NewFrame();
//        ImGui::NewFrame();
//
//        if (show_demo_window)
//            ImGui::ShowDemoWindow(&show_demo_window);
//
//        ImGui::Render();
//        ImDrawData* draw_data = ImGui::GetDrawData();
//        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
//        if (!is_minimized)
//        {
//        	imGui->wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
//        	imGui->wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
//        	imGui->wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
//        	imGui->wd->ClearValue.color.float32[3] = clear_color.w;
//        	imGui->FrameRender(imGui->wd, draw_data);
//        	imGui->FramePresent(imGui->wd);
//        }

//-----------------------------------------------------------------------------------

		drawFrame();
	    bQuit = true;
	}

#elif defined(VK_USE_SDL)
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			bQuit = true;
		}
		else if (e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_SPACE)
			{
				_selectedShader += 1;
				if(_selectedShader > 1)
				{
					_selectedShader = 0;
				}
			}
		}

#endif
	vkDeviceWaitIdle(_device);
	}
}

/** ---------------------------------------------------------------
 * Structure functions
 * uint32_t VulkanEngine::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
 *
 ---------------------------------------------------------------*/
void VulkanEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Example Vulkan Application")//make the Vulkan instance, with basic debug features
		.request_validation_layers	(true)
		.require_api_version		(1, 1, 0)
		.use_default_debug_messenger()
		.build						();

	vkb::Instance vkb_inst 			= inst_ret.value();
	//store the instance
	_instance 						= vkb_inst.instance;
	//store the debug messenger
	_debug_messenger 				= vkb_inst.debug_messenger;

#if defined(VK_USE_GLFW3)
	if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
#elif defined(VK_USE_SDL)
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);// get the surface of the window we opened with SDL
#endif


	//use vkbootstrap to select a GPU.
	//We want a GPU that can write to the SDL surface and supports Vulkan 1.1
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(_surface)
		.select()
		.value();

	//create the final Vulkan _device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	queueFamilyIndices.graphicsFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
	//_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	//initialize the memory allocator
//	VmaAllocatorCreateInfo allocatorInfo = {};
//	allocatorInfo.physicalDevice = _chosenGPU;
//	allocatorInfo.device = _device;
//	allocatorInfo.instance = _instance;
//	vmaCreateAllocator(&allocatorInfo, &_allocator);
//
//	_mainDeletionQueue.push_function([&]() {
//		vmaDestroyAllocator(_allocator);
//	});
}

void VulkanEngine::init_swapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	//store swapchain and its related images
	_swapChain = vkbSwapchain.swapchain;
	swapChainImages = vkbSwapchain.get_images().value();
	swapChainImageViews = vkbSwapchain.get_image_views().value();

	swapChainImageFormat = vkbSwapchain.image_format;

	_mainDeletionQueue.push_function([=]() {
		vkDestroySwapchainKHR(_device, _swapChain, nullptr);
	});

	//depth image size will match the window
	VkExtent3D depthImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be a image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	//for the depth image, we want to allocate it from gpu local memory
//	VmaAllocationCreateInfo dimg_allocinfo = {};
//	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
//	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
//	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

	//build a image-view for the depth image to use for rendering
//	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);;

//	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

	//add to deletion queues
//	_mainDeletionQueue.push_function([=]() {
//		vkDestroyImageView(_device, _depthImageView, nullptr);
//		vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
//	});
}


VkResult VulkanEngine::createInstance(bool enableValidation)
{
	this->settings.validation = enableValidation;
	layers.push_back("VK_LAYER_KHRONOS_validation");
	layers.push_back("VK_LAYER_LUNARG_api_dump");

    if (enableValidation && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

	VkApplicationInfo appInfo 				= {};
	appInfo.sType 							= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName 				= name.c_str();
    appInfo.applicationVersion 				= VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName 					= name.c_str();
    appInfo.engineVersion 					= VK_MAKE_VERSION(1, 2, 162);
	appInfo.apiVersion 						= apiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType 						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo 			= &appInfo;

#if defined(VK_USE_GLFW3)
    auto extensions 						= getRequiredExtensions();
    extensions_count 						= static_cast<uint32_t>(extensions.size());
    createInfo.enabledExtensionCount		= static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames 		= extensions.data();
#elif defined(VK_USE_SDL)
#endif

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (this->settings.validation) {
		// Rik createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.enabledLayerCount 		= static_cast<uint32_t>(layers.size());
		//createInfo.ppEnabledLayerNames 	= validationLayers.data();
		createInfo.ppEnabledLayerNames 		= layers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext 					= (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount 		= 0;
		createInfo.pNext 					= nullptr;
	}
    // Rik const char* layers[] 				= { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_api_dump" };
    createInfo.enabledLayerCount 			= 2;
    createInfo.ppEnabledLayerNames 			= layers.data();

    // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
    const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
#if defined(VK_USE_GLFW3)
    memcpy(extensions_ext, &extensions[0], extensions_count * sizeof(const char*));
#elif defined(VK_USE_SDL)
#endif
    extensions_ext[extensions_count] 		= "VK_EXT_debug_report";
    createInfo.enabledExtensionCount 		= extensions_count + 1;
    createInfo.ppEnabledExtensionNames 		= extensions_ext;


	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	// Get extensions supported by the instance and store for later use
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (VkExtensionProperties extension : extensions)
			{
				supportedInstanceExtensions.push_back(extension.extensionName);
				std::cout << extension.extensionName << std::endl;
			}
		}
	}

	// Enabled requested instance extensions
	if (enabledInstanceExtensions.size() > 0)
	{
		for (const char * enabledExtension : enabledInstanceExtensions)
		{
			// Output message if requested extension is not available
			if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
			{
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			}
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType 				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext 				= NULL;
	instanceCreateInfo.pApplicationInfo 	= &appInfo;
	if (instanceExtensions.size() > 0)
	{
		if (settings.validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}

	// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
	// Note that on Android this layer requires at least NDK r20
	//layers.push_back("VK_LAYER_KHRONOS_validation");
	//layers.push_back("VK_LAYER_LUNARG_api_dump");
	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	if (settings.validation)
	{
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties	(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> 		instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties	(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent 		= false;
		for (VkLayerProperties layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent 		= true;
				break;
			}
		}
		if (validationLayerPresent) {
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		} else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

	VkResult err;
    // Create Vulkan Instance
	err = vkCreateInstance(&createInfo, m_Allocator, &_instance);
	if (VK_SUCCESS != err){
		free(extensions_ext);
		throw std::runtime_error("failed to create instance!");
	}
	free(extensions_ext);
	return err;
}

void VulkanEngine::createSurface() {
}

void VulkanEngine::createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(_chosenGPU);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

        float queuePriority = 1.0f;
        for (int queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(_chosenGPU, &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, indices.presentFamily, 0, &presentQueue);
    }

void VulkanEngine::createSwapChain() {
        SwapChainSupportDetails swapChainSupport 	= querySwapChainSupport		(_chosenGPU);

        surfaceFormat 								= chooseSwapSurfaceFormat	(swapChainSupport.formats);
        presentMode 								= chooseSwapPresentMode		(swapChainSupport.presentModes);
        VkExtent2D extent 							= chooseSwapExtent			(swapChainSupport.capabilities);

        uint32_t imageCount 						= swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo 		= {};
        createInfo.sType 							= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface 							= _surface;
        createInfo.minImageCount 					= imageCount;
        createInfo.imageFormat 						= surfaceFormat.format;
        createInfo.imageColorSpace 					= surfaceFormat.colorSpace;
        createInfo.imageExtent 						= extent;
        createInfo.imageArrayLayers 				= 1;
        createInfo.imageUsage 						= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices 					= findQueueFamilies(_chosenGPU);
        uint32_t queueFamilyIndices[] 				= {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode 			= VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount 		= 2;
            createInfo.pQueueFamilyIndices 			= queueFamilyIndices;
        } else {
            createInfo.imageSharingMode 			= VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform 					= swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha 					= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode 						= presentMode;
        createInfo.clipped 							= VK_TRUE;
        createInfo.oldSwapchain 					= VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
            throw std::runtime_error				("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR						(_device, _swapChain, &imageCount, nullptr);
        swapChainImages.resize						(imageCount);
        vkGetSwapchainImagesKHR						(_device, _swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat 						= surfaceFormat.format;
        swapChainExtent = extent;
    }
//VK_IMAGE_ASPECT_COLOR_BIT
void VulkanEngine::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType 				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image 				= swapChainImages[i];
            createInfo.viewType 			= VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format 				= swapChainImageFormat;
            createInfo.components.r 		= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g 		= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b 		= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a 		= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(_device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
            deletionImageViewsQueue.push_function([=]() {
        		vkDestroyImageView(_device, swapChainImageViews[i], nullptr);
        	    });

        }
    }

//void VulkanEngine::createRenderPass()
//{
//	std::array<VkAttachmentDescription, 2> attachments = {};
//	// Color attachment
//	attachments[0].format = swapChain.colorFormat;
//	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
//	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//	// Depth attachment
//	attachments[1].format = depthFormat;
//	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
//	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference colorReference = {};
//	colorReference.attachment = 0;
//	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	VkAttachmentReference depthReference = {};
//	depthReference.attachment = 1;
//	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//	VkSubpassDescription subpassDescription = {};
//	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpassDescription.colorAttachmentCount = 1;
//	subpassDescription.pColorAttachments = &colorReference;
//	subpassDescription.pDepthStencilAttachment = &depthReference;
//	subpassDescription.inputAttachmentCount = 0;
//	subpassDescription.pInputAttachments = nullptr;
//	subpassDescription.preserveAttachmentCount = 0;
//	subpassDescription.pPreserveAttachments = nullptr;
//	subpassDescription.pResolveAttachments = nullptr;
//
//	// Subpass dependencies for layout transitions
//	std::array<VkSubpassDependency, 2> dependencies;
//
//	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//	dependencies[0].dstSubpass = 0;
//	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//	dependencies[1].srcSubpass = 0;
//	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
//	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//	VkRenderPassCreateInfo renderPassInfo = {};
//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//	renderPassInfo.pAttachments = attachments.data();
//	renderPassInfo.subpassCount = 1;
//	renderPassInfo.pSubpasses = &subpassDescription;
//	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
//	renderPassInfo.pDependencies = dependencies.data();
//
//	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
//}

void VulkanEngine::createRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format 					= swapChainImageFormat;
	colorAttachment.samples 				= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp 					= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp 				= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp 			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp 			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout 			= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout 			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef= {};
	colorAttachmentRef.attachment 			= 0;
	colorAttachmentRef.layout 				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass 			= {};
	subpass.pipelineBindPoint 				= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 			= 1;
	subpass.pColorAttachments 				= &colorAttachmentRef;

	VkSubpassDependency dependency 			= {};
	dependency.srcSubpass 					= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 					= 0;
	dependency.srcStageMask 				= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask 				= 0;
	dependency.dstStageMask 				= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask 				= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo 	= {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 			= 1;
	renderPassInfo.pAttachments 			= &colorAttachment;
	renderPassInfo.subpassCount 			= 1;
	renderPassInfo.pSubpasses 				= &subpass;
	renderPassInfo.dependencyCount 			= 1;
	renderPassInfo.pDependencies 			= &dependency;

	if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanEngine::createUniformBuffers()
{
	// Prepare and initialize a uniform buffer block containing shader uniforms
	// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
	VkMemoryRequirements memReqs;

	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo 				= {};
	VkMemoryAllocateInfo allocInfo 				= {};
	allocInfo.sType 							= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext 							= nullptr;
	allocInfo.allocationSize 					= 0;
	allocInfo.memoryTypeIndex 					= 0;

	bufferInfo.sType 							= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size 							= sizeof(uboVS);
	// This buffer will be used as a uniform buffer
	bufferInfo.usage 							= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// Create a new buffer
	VK_CHECK_RESULT(vkCreateBuffer(_device, &bufferInfo, nullptr, &uniformBufferVS.buffer));
	// Get memory requirements including size, alignment and memory type
	vkGetBufferMemoryRequirements(_device, uniformBufferVS.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	// Get the memory type index that supports host visible memory access
	// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
	// We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
	// Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
	//Rik allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// Allocate memory for the uniform buffer
	VK_CHECK_RESULT(vkAllocateMemory(_device, &allocInfo, nullptr, &(uniformBufferVS.memory)));
	// Bind memory to buffer
	VK_CHECK_RESULT(vkBindBufferMemory(_device, uniformBufferVS.buffer, uniformBufferVS.memory, 0));

	// Store information in the uniform's descriptor that is used by the descriptor set
	uniformBufferVS.descriptor.buffer = uniformBufferVS.buffer;
	uniformBufferVS.descriptor.offset = 0;
	uniformBufferVS.descriptor.range = sizeof(uboVS);

	updateUniformBuffers();
}

void VulkanEngine::createDescriptorPool()
{
	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[1];
	// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	descriptorPoolInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void VulkanEngine::createDescriptorSetLayout()
{
	// Setup layout of descriptors used in this example
	// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout binding

	// Binding 0: Uniform buffer (Vertex shader)
	VkDescriptorSetLayoutBinding layoutBinding 	= {};
	layoutBinding.descriptorType 				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount 				= 1;
	layoutBinding.stageFlags 					= VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers 			= nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType 						= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext 						= nullptr;
	descriptorLayout.bindingCount 				= 1;
	descriptorLayout.pBindings 					= &layoutBinding;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayout, nullptr, &descriptorSetLayout));

	// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType 			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext 			= nullptr;
	pPipelineLayoutCreateInfo.setLayoutCount 	= 1;
	pPipelineLayoutCreateInfo.pSetLayouts 		= &descriptorSetLayout;

	VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void VulkanEngine::createDescriptorSet()
{
	// Allocate a new descriptor set from the global descriptor pool
	VkDescriptorSetAllocateInfo allocInfo 		= {};
	allocInfo.sType 							= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool 					= descriptorPool;
	allocInfo.descriptorSetCount 				= 1;
	allocInfo.pSetLayouts 						= &descriptorSetLayout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet));

	// Update the descriptor set determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point

	VkWriteDescriptorSet writeDescriptorSet 	= {};

	// Binding 0 : Uniform buffer
	writeDescriptorSet.sType 					= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet 					= descriptorSet;
	writeDescriptorSet.descriptorCount 			= 1;
	writeDescriptorSet.descriptorType 			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo 				= &uniformBufferVS.descriptor;
	// Binds this uniform buffer to binding point 0
	writeDescriptorSet.dstBinding 				= 0;

	vkUpdateDescriptorSets(_device, 1, &writeDescriptorSet, 0, nullptr);
}

void VulkanEngine::updateUniformBuffers()
{
	// Pass matrices to the shaders
	uboVS.projectionMatrix = camera.matrices.perspective;
	uboVS.viewMatrix = camera.matrices.view;
	uboVS.modelMatrix = glm::mat4(1.0f);

	// Map uniform buffer and update it
	uint8_t *pData;
	VK_CHECK_RESULT(vkMapMemory(_device, uniformBufferVS.memory, 0, sizeof(uboVS), 0, (void **)&pData));
	memcpy(pData, &uboVS, sizeof(uboVS));
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vkUnmapMemory(_device, uniformBufferVS.memory);
}


void VulkanEngine::createGraphicsPipeline() {
        auto vertShaderCode 					= readFile("shaders/09_shader_base.vert.spv");
        auto fragShaderCode 					= readFile("shaders/09_shader_base.frag.spv");

        VkShaderModule vertShaderModule 		= createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule 		= createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage 				= VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module 				= vertShaderModule;
        vertShaderStageInfo.pName 				= "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage 				= VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module 				= fragShaderModule;
        fragShaderStageInfo.pName 				= "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType 					= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType 					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology 					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable 	= VK_FALSE;

        VkViewport viewport 					= {};
        viewport.x 								= 0.0f;
        viewport.y 								= 0.0f;
        viewport.width 							= (float) swapChainExtent.width;
        viewport.height 						= (float) swapChainExtent.height;
        viewport.minDepth 						= 0.0f;
        viewport.maxDepth 						= 1.0f;

        VkRect2D scissor 						= {};
        scissor.offset 							= {0, 0};
        scissor.extent 							= swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType 					= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount 			= 1;
        viewportState.pViewports 				= &viewport;
        viewportState.scissorCount 				= 1;
        viewportState.pScissors 				= &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType 						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable 			= VK_FALSE;
        rasterizer.rasterizerDiscardEnable 		= VK_FALSE;
        rasterizer.polygonMode 					= VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth 					= 1.0f;
        rasterizer.cullMode 					= VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace 					= VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable 				= VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType 					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable 		= VK_FALSE;
        multisampling.rasterizationSamples 		= VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask 	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable 		= VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable 			= VK_FALSE;
        colorBlending.logicOp 					= VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount 			= 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0]         = 0.0f;
        colorBlending.blendConstants[1]         = 0.0f;
        colorBlending.blendConstants[2]         = 0.0f;
        colorBlending.blendConstants[3]         = 0.0f;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    }

void VulkanEngine::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
		deletionFrameBuffersQueue.push_function([=]() {
			vkDestroyFramebuffer(_device, swapChainFramebuffers[i], nullptr);
			});
	}
}

void VulkanEngine::createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_chosenGPU);

        VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(queueFamilyIndices.graphicsFamily);

        uploadCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        uploadCommandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

        VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
        _mainDeletionQueue.push_function([=]() {
        		vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
        	});
    }

void VulkanEngine::createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo 		= {};
        allocInfo.sType 							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool 						= _uploadContext._commandPool;
        allocInfo.level 							= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount 				= (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo 		= {};
            beginInfo.sType 						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags 						= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

            VkRenderPassBeginInfo renderPassInfo 	= {};
            renderPassInfo.sType 					= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass 				= renderPass;
            renderPassInfo.framebuffer 				= swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset 		= {0, 0};
            renderPassInfo.renderArea.extent 		= swapChainExtent;

            VkClearValue clearColor 				= {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount 			= 1;
            renderPassInfo.pClearValues 			= &clearColor;

            vkCmdBeginRenderPass					(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline					(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                vkCmdDraw							(commandBuffers[i], 3, 1, 0, 0);


            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

void VulkanEngine::createSemaphores() {
	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
	_mainDeletionQueue.push_function([=]() {
		vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
	});

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}
}
void VulkanEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
	for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

        _mainDeletionQueue.push_function([=]() {	//enqueue the destruction of the fence
            vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
            });
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));


        _mainDeletionQueue.push_function([=]() {	//enqueue the destruction of semaphores
            vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
            });
	}
}

/** ---------------------------------------------------------------
 * Helper functions
 ---------------------------------------------------------------*/


void VulkanEngine::init_imgui()
{
	//imGui = new RImGui(this);
	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	check_vk_result(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

#if defined(VK_USE_GLFW3)
	ImGui_ImplGlfw_InitForVulkan(_window, true);
#elif defined(VK_USE_SDL)
	ImGui_ImplSDL2_InitForVulkan(_window);
#endif

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	//init_info.PhysicalDevice = chosenGPU;physicalDevice
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	//execute a gpu command to upload imgui font textures
	immediate_submit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {

		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
}



// Prepare vertex and index buffers for an indexed triangle
// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
void VulkanEngine::createVertices(bool useStagingBuffers)
{

	std::vector<Vertex> vertexBuffer =
	{	//Front				  //Front colors
		{{-1.0f,-1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 1.0f,-1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{ 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
		//Back				  //Back Colors
		{{-1.0f,-1.0f,-1.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 1.0f,-1.0f,-1.0f}, {0.0f, 1.0f, 0.0f}},
		{{ 1.0f, 1.0f,-1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
	};


	uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

	// Setup indices
	// Rik std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
	std::vector<uint32_t> indexBuffer = {
			/* front */0, 1, 2, 2, 3, 0,
			/* right */1, 5, 6, 6, 2, 1,
			/* back  */7, 6, 5, 5, 4, 7,
			/* left  */4, 0, 3, 3, 7, 4,
			/* bottom*/4, 5, 1, 1, 0, 4,
			/* top	 */3, 2, 6, 6, 7, 3
		};

	idx.count = static_cast<uint32_t>(indexBuffer.size());
	uint32_t indexBufferSize = idx.count * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	void *data;

	if (useStagingBuffers)
	{
		// Static data like vertex and index buffer should be stored on the device memory
		// for optimal (and fastest) access by the GPU
		//
		// To achieve this we use so-called "staging buffers" :
		// - Create a buffer that's visible to the host (and can be mapped)
		// - Copy the data to this buffer
		// - Create another buffer that's local on the device (VRAM) with the same size
		// - Copy the data from the host to the device using a command buffer
		// - Delete the host visible (staging) buffer
		// - Use the device local buffers for rendering

		struct StagingBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
		};

		struct {
			StagingBuffer vertices;
			StagingBuffer indices;
		} stagingBuffers;

		// Vertex buffer
		//----------------------------------------------------------------------------------------
		VkBufferCreateInfo vertexBufferInfo 		= {};
		vertexBufferInfo.sType 						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size 						= vertexBufferSize;
		// Buffer is used as the copy source
		vertexBufferInfo.usage 						= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		VK_CHECK_RESULT(vkCreateBuffer				(_device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
		vkGetBufferMemoryRequirements				(_device, stagingBuffers.vertices.buffer, &memReqs);

		memAlloc.allocationSize 					= memReqs.size;
		// Request a host visible memory type that can be used to copy our data do
		// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory			(_device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		// Map and copy
		VK_CHECK_RESULT(vkMapMemory					(_device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(_device, stagingBuffers.vertices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory			(_device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		// Create a vulkanDevice local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer				(_device, &vertexBufferInfo, nullptr, &vtx.buffer));
		vkGetBufferMemoryRequirements				(_device, vtx.buffer, &memReqs);
		memAlloc.allocationSize 					= memReqs.size;
		memAlloc.memoryTypeIndex 					= getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory			(_device, &memAlloc, nullptr, &vtx.memory));
		VK_CHECK_RESULT(vkBindBufferMemory			(_device, vtx.buffer, vtx.memory, 0));

		// Index buffer
		//----------------------------------------------------------------------------------------
		VkBufferCreateInfo indexbufferInfo 			= {};
		indexbufferInfo.sType 						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size 						= indexBufferSize;
		indexbufferInfo.usage 						= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// Copy index data to a buffer visible to the host (staging buffer)
		VK_CHECK_RESULT(vkCreateBuffer				(_device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
		vkGetBufferMemoryRequirements				(_device, stagingBuffers.indices.buffer, &memReqs);
		memAlloc.allocationSize 					= memReqs.size;
		memAlloc.memoryTypeIndex 					= getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory			(_device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
		VK_CHECK_RESULT(vkMapMemory					(_device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(_device, stagingBuffers.indices.memory);
		VK_CHECK_RESULT(vkBindBufferMemory			(_device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

		// Create destination buffer with vulkanDevice only visibility
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VK_CHECK_RESULT(vkCreateBuffer				(_device, &indexbufferInfo, nullptr, &idx.buffer));
		vkGetBufferMemoryRequirements				(_device, idx.buffer, &memReqs);
		memAlloc.allocationSize 					= memReqs.size;
		memAlloc.memoryTypeIndex 					= getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory			(_device, &memAlloc, nullptr, &idx.memory));
		VK_CHECK_RESULT(vkBindBufferMemory			(_device, idx.buffer, idx.memory, 0));

		// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		// Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
		VkCommandBuffer copyCmd = getCommandBuffer	(true);

		// Put buffer region copies into command buffer
		VkBufferCopy copyRegion = {};

		// Vertex buffer
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vtx.buffer, 1, &copyRegion);
		// Index buffer
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, idx.buffer,	1, &copyRegion);

		// Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
		flushCommandBuffer(copyCmd);

		// Destroy staging buffers
		// Note: Staging buffer must not be deleted before the copies have been submitted and executed
		vkDestroyBuffer	(_device, stagingBuffers.vertices.buffer, 	nullptr);
		vkFreeMemory	(_device, stagingBuffers.vertices.memory, 	nullptr);
		vkDestroyBuffer	(_device, stagingBuffers.indices.buffer, 	nullptr);
		vkFreeMemory	(_device, stagingBuffers.indices.memory, 	nullptr);
	}
	else
	{
		// Don't use staging
		// Create host-visible buffers only and use these for rendering. This is not advised and will usually result in lower rendering performance
		// Vertex buffer
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size = vertexBufferSize;
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		// Copy vertex data to a buffer visible to the host
		VK_CHECK_RESULT(vkCreateBuffer(_device, &vertexBufferInfo, nullptr, &vtx.buffer));
		vkGetBufferMemoryRequirements(_device, vtx.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT is host visible memory, and VK_MEMORY_PROPERTY_HOST_COHERENT_BIT makes sure writes are directly visible
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(_device, &memAlloc, nullptr, &vtx.memory));
		VK_CHECK_RESULT(vkMapMemory(_device, vtx.memory, 0, memAlloc.allocationSize, 0, &data));
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(_device, vtx.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(_device, vtx.buffer, vtx.memory, 0));

		// Index buffer
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size = indexBufferSize;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		// Copy index data to a buffer visible to the host
		VK_CHECK_RESULT(vkCreateBuffer(_device, &indexbufferInfo, nullptr, &idx.buffer));
		vkGetBufferMemoryRequirements(_device, idx.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(_device, &memAlloc, nullptr, &idx.memory));
		VK_CHECK_RESULT(vkMapMemory(_device, idx.memory, 0, indexBufferSize, 0, &data));
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(_device, idx.memory);
		VK_CHECK_RESULT(vkBindBufferMemory(_device, idx.buffer, idx.memory, 0));
	}
}

// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer VulkanEngine::getCommandBuffer(bool begin)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType 						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool 					= _uploadContext._commandPool;
	cmdBufAllocateInfo.level 						= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount 			= 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = BRE::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}

	return cmdBuffer;
}

// End the command buffer and submit it to the queue
// Uses a fence to ensure command buffer has finished executing before deleting it
void VulkanEngine::flushCommandBuffer(VkCommandBuffer commandBuffer)
{
	assert(commandBuffer != VK_NULL_HANDLE);

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(_device, fence, nullptr);
	vkFreeCommandBuffers(_device, cmdPool, 1, &commandBuffer);
}


/**
* Get the index of a memory type that has all the requested property bits set
*
* @param typeBits Bit mask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
* @param properties Bit mask of properties for the memory type to request
* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
*
* @return Index of the requested memory type
*
* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
*/
uint32_t VulkanEngine::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

/**
* Create a buffer on the device
*
* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
* @param size Size of the buffer in byes
* @param buffer Pointer to the buffer handle acquired by the function
* @param memory Pointer to the memory handle acquired by the function
* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
VkResult VulkanEngine::createBuffer(VkBufferUsageFlags usageFlags	,
									VkMemoryPropertyFlags memoryPropertyFlags,
									VkDeviceSize size				,
									VkBuffer *buffer				,
									VkDeviceMemory *memory			,
									void *data)
{
	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = BRE::initializers::bufferCreateInfo(usageFlags, size);
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));
	deletionDeviceBufferQueue.push_function([=]() {
	    if (buffer != VK_NULL_HANDLE){
	        vkDestroyBuffer(logicalDevice, *buffer, nullptr);
	    }
	});
	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = BRE::initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memAlloc.pNext = &allocFlagsInfo;
	}
	VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		void *mapped;
		VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange = BRE::initializers::mappedMemoryRange();
			mappedRange.memory = *memory;
			mappedRange.offset = 0;
			mappedRange.size = size;
			vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
		}
		vkUnmapMemory(logicalDevice, *memory);
	}

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

	return VK_SUCCESS;
}

/**
* Create a buffer on the device
*
* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
* @param buffer Pointer to a vk::Vulkan buffer object
* @param size Size of the buffer in bytes
* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
VkResult VulkanEngine::createBuffer(VkBufferUsageFlags usageFlags,
									VkMemoryPropertyFlags memoryPropertyFlags,
									BRE::Buffer *buffer,
									VkDeviceSize size,
									void *data)
{
	buffer->device = logicalDevice;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = BRE::initializers::bufferCreateInfo(usageFlags, size);
	//VkResult err = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer);
	VkResult err = vkCreateBuffer(_device, &bufferCreateInfo, nullptr, &buffer->buffer);
	VK_CHECK_RESULT( err );
//	deletionDeviceBufferQueue.push_function([=]() {
//	    if (buffer->buffer != VK_NULL_HANDLE){
//	        vkDestroyBuffer(logicalDevice, buffer->buffer, nullptr);
//	    }
//	});

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = BRE::initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(_device, buffer->buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memAlloc.pNext = &allocFlagsInfo;
	}
	VK_CHECK_RESULT(vkAllocateMemory(_device, &memAlloc, nullptr, &buffer->memory));

	buffer->alignment = memReqs.alignment;
	buffer->size = size;
	buffer->usageFlags = usageFlags;
	buffer->memoryPropertyFlags = memoryPropertyFlags;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		VK_CHECK_RESULT(buffer->map());
		memcpy(buffer->mapped, data, size);
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			buffer->flush();

		buffer->unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	buffer->setupDescriptor();

	return vkBindBufferMemory(_device, buffer->buffer, buffer->memory, 0);
	// Attach the memory to the buffer object
	//return buffer->bind();
}

/**
* Allocate a command buffer from the command pool
*
* @param level Level of the new command buffer (primary or secondary)
* @param pool Command pool from which the command buffer will be allocated
* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
*
* @return A handle to the allocated command buffer
*/
VkCommandBuffer VulkanEngine::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = BRE::initializers::commandBufferAllocateInfo(pool, level, 1);
	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &cmdBuffer));
	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = BRE::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}
	return cmdBuffer;
}

VkCommandBuffer VulkanEngine::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	return createCommandBuffer(level, _uploadContext._commandPool, begin);
}

/**
* Finish command buffer recording and submit it to a queue
*
* @param commandBuffer Command buffer to flush
* @param queue Queue to submit the command buffer to
* @param pool Command pool on which the command buffer has been created
* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
*
* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
* @note Uses a fence to ensure command buffer has finished executing
*/
void VulkanEngine::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = BRE::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = BRE::initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(_device, &fenceInfo, nullptr, &fence));
	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
	vkDestroyFence(_device, fence, nullptr);
	if (free)
	{
		vkFreeCommandBuffers(_device, pool, 1, &commandBuffer);
	}
}

void VulkanEngine::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	return flushCommandBuffer(commandBuffer, queue, _uploadContext._commandPool, free);
}

VkPipelineShaderStageCreateInfo VulkanEngine::loadShader(std::string fileName, VkShaderStageFlagBits stage )
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = loadShader(fileName.c_str(), _device);
	shaderStage.pName = "main";
	assert(shaderStage.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

VkShaderModule VulkanEngine::loadShader(const char *fileName, VkDevice device)
{
	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();

		assert(size > 0);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;

		VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
		return VK_NULL_HANDLE;
	}
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFormat : availableFormats) {
        if (	availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM 				&&
        		availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
				) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void VulkanEngine::pickPhysicalDevice() {
// Using BootStrap willl be implemented in vulkan_init
//        uint32_t deviceCount = 0;
//        vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
//
//        if (deviceCount == 0) {
//            throw std::runtime_error("failed to find GPUs with Vulkan support!");
//        }
//
//        std::vector<VkPhysicalDevice> devices(deviceCount);
//        vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
//
//        for (const auto& device : devices) {
//            if (isDeviceSuitable(device)) {
//                physicalDevice = device;
//                break;
//            }
//        }
//
//        if (physicalDevice == VK_NULL_HANDLE) {
//            throw std::runtime_error("failed to find a suitable GPU!");
//        }
//
//    	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
    }

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice Device)
{
	volatile bool isDedicatedGPU = false;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(Device, &physicalDeviceProperties);

	QueueFamilyIndices Indices = findQueueFamilies(Device);

	bool extensionsSupported = checkDeviceExtensionSupport(Device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(Device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(Device, &supportedFeatures);
	if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		isDedicatedGPU = true;
	}
	return Indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && isDedicatedGPU;
}

VulkanEngine::QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice Device)
{
	QueueFamilyIndices queueIndices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &queueFamilyCount, queueFamilies.data());
	int i = 0;
	for (const auto& queueFamily : queueFamilies)	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)		{
			queueIndices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, _surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)		{
			queueIndices.presentFamily = i;
		}

		if (queueIndices.isComplete())		{
			break;
		}
		i++;
	}
	return queueIndices;
}

SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice Device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, _surface, &details.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(Device, _surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, _surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(Device, _surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, _surface, &presentModeCount, details.presentModes.data());
	}
	return details;
}

void VulkanEngine::getEnabledFeatures() {
	// Geometry shader support is required for writing to multiple shadow map layers in one single pass
//	if (deviceFeatures.geometryShader) {
//		enabledFeatures.geometryShader = VK_TRUE;
//	}
//	else {
//		BRE::tools::exitFatal("Selected GPU does not support geometry shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
//	}
	// Enable anisotropic filtering if supported
	if (deviceFeatures.samplerAnisotropy) {
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	}
	// Enable texture compression
	if (deviceFeatures.textureCompressionBC) {
		enabledFeatures.textureCompressionBC = VK_TRUE;
	}
	else if (deviceFeatures.textureCompressionASTC_LDR) {
		enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
	}
	else if (deviceFeatures.textureCompressionETC2) {
		enabledFeatures.textureCompressionETC2 = VK_TRUE;
	}

	// Enable multi draw indirect if supported
	if (deviceFeatures.multiDrawIndirect) {
		enabledFeatures.multiDrawIndirect 	= VK_TRUE;
	}

	enabledFeatures.fillModeNonSolid 		= deviceFeatures.fillModeNonSolid;
	enabledFeatures.wideLines 				= deviceFeatures.wideLines;

	// Enable sample rate shading filtering if supported
	if (deviceFeatures.sampleRateShading) {
		enabledFeatures.sampleRateShading = VK_TRUE;
	}

	// Tessellation shader support is required for this example
	if (deviceFeatures.tessellationShader) {
		enabledFeatures.tessellationShader = VK_TRUE;
	}

//	if (deviceFeatures.fragmentStoresAndAtomics) {
//		enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
//	} else {
//		BRE::tools::exitFatal("Selected GPU does not support stores and atomic operations in the fragment stage", VK_ERROR_FEATURE_NOT_PRESENT);
//	}

	// Support for pipeline statistics is optional
//	if (deviceFeatures.pipelineStatisticsQuery) {
//		enabledFeatures.pipelineStatisticsQuery = VK_TRUE;
//	}
//	else {
//		BRE::tools::exitFatal("Selected GPU does not support pipeline statistics!", VK_ERROR_FEATURE_NOT_PRESENT);
//	}
}


bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice Device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

bool VulkanEngine::checkValidationLayerSupport() {
    uint32_t 							layerCount;
    bool 								layerFound = false;
    vkEnumerateInstanceLayerProperties	(&layerCount, nullptr);
    std::vector<VkLayerProperties> 		availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties	(&layerCount, availableLayers.data());

    for (const char* layerName : layers) {
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                if (!layerFound) layerFound = true;
                // Rik if(isDBOpenVal <= 0){ VDbmgr::insertDBValLayers(db, std::string(layerName)); }
            }
        }
    }
    return layerFound;// Rik  true;
}

void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)  {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    								VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = 		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 	|
    								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 	|
									VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanEngine::createImage(	uint32_t width					,
								uint32_t height					,
								VkSampleCountFlagBits sampleCount,
								VkFormat format					,
								VkImageTiling tiling			,
								VkImageUsageFlags usage			,
								VkMemoryPropertyFlags properties,
								VkImage& image					,
								VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}
	vkBindImageMemory(_device, image, imageMemory, 0);
}

uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_chosenGPU, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
// Upon success it will return the index of the memory type that fits our requested memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties.
// You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
uint32_t VulkanEngine::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		typeBits >>= 1;
	}

	throw "Could not find a suitable memory type!";
}

VkResult VulkanEngine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanEngine::setupDebugMessenger() {
    if (!settings.validation) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }

    // Get the function pointer (required for any extensions)
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
    IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

    // Setup the debug report callback
    VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
    debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_report_ci.pfnCallback = debug_report;
    debug_report_ci.pUserData = NULL;
    if (VK_SUCCESS != vkCreateDebugReportCallbackEXT(_instance, &debug_report_ci, m_Allocator, &m_DebugReport)){
        throw std::runtime_error("failed vkCreateDebugReportCallbackEXT!");
    }

}

void VulkanEngine::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT m_DebugReport, const VkAllocationCallbacks* pAllocator)
{
	//PFN_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, m_DebugReport, pAllocator);
	}
}


void VulkanEngine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#if defined(VK_USE_GLFW3)
std::vector<const char*> VulkanEngine::getRequiredExtensions() {
    uint32_t 		glfwExtensionCount = 0;
    const char		**glfwExtensions;
    glfwExtensions 	= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (this->settings.validation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}
#endif

std::string VulkanEngine::getShadersPath() const
{
	return getAssetPath() + "shaders/" + settings.shaderDir + "/";
}

std::vector<char> VulkanEngine::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule VulkanEngine::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();

	std::vector<uint32_t> codeAligned(code.size() / 4 + 1);
	memcpy(codeAligned.data(), code.data(), code.size());

	createInfo.pCode = codeAligned.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void VulkanEngine::prepareImGui()
{
	//imGui = new RImGui(this);
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
    VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &cmd));
	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    //execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));
	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);
    //clear the command pool. This will free the command buffer too
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::OnUpdateUIOverlay()
{
	if (overlay->header("Settings")) {
//		if (overlay->checkBox("Bloom", &bloom)) {
			buildCommandBuffers();
//		}
//		if (overlay->inputFloat("Scale", &ubos.blurParams.blurScale, 0.1f, 2)) {
			updateUniformBuffersBlur();
//		}
	}
}

void VulkanEngine::updateOverlay()
{
	if (!settings.overlay)
		return;

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)width, (float)height);
	io.DeltaTime = frameTimer;

	io.MousePos = ImVec2(mousePos.x, mousePos.y);
	io.MouseDown[0] = mouseButtons.left;
	io.MouseDown[1] = mouseButtons.right;

	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(title.c_str());
	//ImGui::TextUnformatted(deviceProperties.deviceName);properties
	ImGui::TextUnformatted(properties.deviceName);
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * UIOverlay.scale));
#endif
	ImGui::PushItemWidth(110.0f * overlay->scale);
	//OnUpdateUIOverlay(&UIOverlay);
	OnUpdateUIOverlay();
	ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PopStyleVar();
#endif

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	if (overlay->update() || overlay->updated) {
		buildCommandBuffers();
		overlay->updated = false;
	}

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	if (mouseButtons.left) {
		mouseButtons.left = false;
	}
#endif
}

void VulkanEngine::buildCommandBuffers()
{
//	VkCommandBufferBeginInfo cmdBufInfo = BRE::initializers::commandBufferBeginInfo();
//
//	VkClearValue clearValues[2];
//	VkViewport viewport;
//	VkRect2D scissor;
//
//	/*
//		The blur method used in this example is multi pass and renders the vertical blur first and then the horizontal one
//		While it's possible to blur in one pass, this method is widely used as it requires far less samples to generate the blur
//	*/
//
//	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
//	{
//		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
//
//		if (bloom) {
//			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
//			clearValues[1].depthStencil = { 1.0f, 0 };
//
//			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
//			renderPassBeginInfo.renderPass = offscreenPass.renderPass;
//			renderPassBeginInfo.framebuffer = offscreenPass.framebuffers[0].framebuffer;
//			renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
//			renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
//			renderPassBeginInfo.clearValueCount = 2;
//			renderPassBeginInfo.pClearValues = clearValues;
//
//			viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
//			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
//
//			scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
//			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
//
//			/*
//				First render pass: Render glow parts of the model (separate mesh) to an offscreen frame buffer
//			*/
//
//			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
//			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.glowPass);
//
//			models.ufoGlow.draw(drawCmdBuffers[i]);
//
//			vkCmdEndRenderPass(drawCmdBuffers[i]);
//
//			/*
//				Second render pass: Vertical blur
//
//				Render contents of the first pass into a second framebuffer and apply a vertical blur
//				This is the first blur pass, the horizontal blur is applied when rendering on top of the scene
//			*/
//
//			renderPassBeginInfo.framebuffer = offscreenPass.framebuffers[1].framebuffer;
//
//			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.blur, 0, 1, &descriptorSets.blurVert, 0, NULL);
//			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.blurVert);
//			vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
//
//			vkCmdEndRenderPass(drawCmdBuffers[i]);
//		}
//
//		/*
//			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
//		*/
//
//		/*
//			Third render pass: Scene rendering with applied vertical blur
//
//			Renders the scene and the (vertically blurred) contents of the second framebuffer and apply a horizontal blur
//
//		*/
//		{
//			clearValues[0].color = defaultClearColor;
//			clearValues[1].depthStencil = { 1.0f, 0 };
//
//			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
//			renderPassBeginInfo.renderPass = renderPass;
//			renderPassBeginInfo.framebuffer = frameBuffers[i];
//			renderPassBeginInfo.renderArea.extent.width = width;
//			renderPassBeginInfo.renderArea.extent.height = height;
//			renderPassBeginInfo.clearValueCount = 2;
//			renderPassBeginInfo.pClearValues = clearValues;
//
//			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
//			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
//
//			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
//			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
//
//			VkDeviceSize offsets[1] = { 0 };
//
//			// Skybox
//			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.skyBox, 0, NULL);
//			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skyBox);
//			models.skyBox.draw(drawCmdBuffers[i]);
//
//
//			// 3D scene
//			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
//			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phongPass);
//			models.ufo.draw(drawCmdBuffers[i]);
//
//			if (bloom)
//			{
//				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.blur, 0, 1, &descriptorSets.blurHorz, 0, NULL);
//				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.blurHorz);
//				vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
//			}
//
//			drawUI(drawCmdBuffers[i]);
//
//			vkCmdEndRenderPass(drawCmdBuffers[i]);
//
//		}
//
//		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
//	}
}
// Update blur pass parameter uniform buffer
void VulkanEngine::updateUniformBuffersBlur()
{
	//memcpy(uniformBuffers.blurParams.mapped, &ubos.blurParams, sizeof(ubos.blurParams));
}

void VulkanEngine::prepare()
{
//	if (vulkanDevice->enableDebugMarkers) {
//		vks::debugmarker::setup(device);
//	}
	//initSwapchain();
	ImGui::CreateContext();
	createCommandPool();
	//setupSwapChain();
	createCommandBuffers();
	createSemaphores();   //createSynchronizationPrimitives

	setupDepthStencil();
	createRenderPass();
	createPipelineCache();
	setupFrameBuffer();
	//settings.overlay = settings.overlay && (!benchmark.active);
	if (settings.overlay) {
		//overlay->device = _device;
		//overlay->queue = queue;
		overlay->shaders = {
			loadShader(getShadersPath() + "base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		overlay->prepareResources();
		overlay->preparePipeline(pipelineCache, renderPass);
	}
}

void VulkanEngine::setupDepthStencil()
{

	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = depthFormat;
	imageCI.extent = { width, height, 1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VK_CHECK_RESULT(vkCreateImage(_device, &imageCI, nullptr, &depthStencil.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(_device, depthStencil.image, &memReqs);

	VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
	memAllloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(_device, &memAllloc, nullptr, &depthStencil.mem));
	VK_CHECK_RESULT(vkBindImageMemory(_device, depthStencil.image, depthStencil.mem, 0));

	VkImageViewCreateInfo imageViewCI{};
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.image = depthStencil.image;
	imageViewCI.format = depthFormat;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
	if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK_RESULT(vkCreateImageView(_device, &imageViewCI, nullptr, &depthStencil.view));

//	VkImageCreateInfo imageCI{};
//	imageCI.sType 						= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//	imageCI.imageType 					= VK_IMAGE_TYPE_2D;
//	imageCI.format 						= surfaceFormat.format;
//	imageCI.extent 						= { width, height, 1 };
//	imageCI.mipLevels 					= 1;
//	imageCI.arrayLayers 				= 1;
//	imageCI.samples 					= VK_SAMPLE_COUNT_1_BIT;
//	imageCI.tiling 						= VK_IMAGE_TILING_OPTIMAL;
//	imageCI.usage 						= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//
//	if (vkCreateImage(_device, &imageCI, nullptr, &depthStencil.image) != VK_SUCCESS)
//	{
//		throw std::runtime_error("failed to create image!");
//	}
//	//VK_CHECK_RESULT(vkCreateImage		(_device, &imageCI, nullptr, &depthStencil.image));
//	//VK_CHECK_RESULT(createImage(_device, &imageCI, nullptr, &depthStencil.image));
//	//VK_CHECK_RESULT(createImage(width,height,VK_SAMPLE_COUNT_1_BIT,swapChainImageFormat,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,properties,&depthStencil.image,&depthStencil.mem));
//
//
//	VkMemoryRequirements memReqs{};
//	vkGetImageMemoryRequirements(_device, depthStencil.image, &memReqs);
//
//	VkMemoryAllocateInfo memAllloc{};
//	memAllloc.sType 							= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	memAllloc.allocationSize 					= memReqs.size;
//	memAllloc.memoryTypeIndex 					= getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//	VK_CHECK_RESULT(vkAllocateMemory	(_device, &memAllloc, nullptr, &depthStencil.mem));
//	VK_CHECK_RESULT(vkBindImageMemory	(_device, depthStencil.image, depthStencil.mem, 0));
//
//	VkImageViewCreateInfo imageViewCI{};
//	imageViewCI.sType 						= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//	imageViewCI.viewType 						= VK_IMAGE_VIEW_TYPE_2D;
//	imageViewCI.image 						= depthStencil.image;
//	imageViewCI.format 						= swapChainImageFormat;
//	imageViewCI.subresourceRange.baseMipLevel = 0;
//	imageViewCI.subresourceRange.levelCount = 1;
//	imageViewCI.subresourceRange.baseArrayLayer = 0;
//	imageViewCI.subresourceRange.layerCount = 1;
//	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
//	// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
//	if (swapChainImageFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
//		imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
//	}
//	VK_CHECK_RESULT(vkCreateImageView(_device, &imageViewCI, nullptr, &depthStencil.view));
}

void VulkanEngine::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void VulkanEngine::setupFrameBuffer()
{
	VkImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = depthStencil.view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width;
	frameBufferCreateInfo.height = height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	frameBuffers.resize(swapChainImageViews.size());
	for (uint32_t i = 0; i < swapChainImageViews.size(); i++)
	{
		attachments[0] = swapChainImageViews[i];
		VK_CHECK_RESULT(vkCreateFramebuffer(_device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

FrameData& VulkanEngine::get_current_frame()
{
	return _frames[_frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::init_commands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
			queueFamilyIndices.graphicsFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			//_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool				(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo 	= vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);//allocate the default command buffer that we will use for rendering
		VK_CHECK(vkAllocateCommandBuffers			(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
		});
	}
}




