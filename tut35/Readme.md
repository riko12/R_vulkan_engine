Surface and presentation queue
Create and implement surface and presentation queue 
Add set inclusion in the apllication class implementation cpp

	#include <set>
  
Adapt **QueueFamilyIndices** struct as below  in the H-file (application.h) 

	struct QueueFamilyIndices {
	    std::optional<uint32_t> graphicsFamily;
	    std::optional<uint32_t> presentFamily;
	
	    bool isComplete() {
	        return graphicsFamily.has_value() && presentFamily.has_value();
	    }
	};

Add member VkSurfaceKHR surface and VkQueue presentQueue member in the application class  

    VkSurfaceKHR surface;
    VkQueue presentQueue;

Add createSurface() method in application class definition and also in the class implementation 
within initVulkan() after   setupDebugMessenger() call.

    createSurface();

Destroy the surface in cleanup before the destroyInstance call  

	vkDestroySurfaceKHR(instance, surface, nullptr);

Implement the createSurface in the implementation class
 
	void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }
   
Now we have two family queues, one for the graphic queue and the second is the presentation queue.
For this reason I need to handle these queues in a vector. So, 
in the createLogicalDevice change the source 
to manage this vector of queue list.

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    ...
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    ...
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    
Add the presentation queue implementation in the the findQueueFamilies

	VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport) {
        indices.presentFamily = i;
    }   
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    