Image views creation 

	
  
Add  member in the class definition  

	std::vector<VkImageView> swapChainImageViews;

Add createImageViews() in the initVulkam method   

    createImageViews();

Destroy the image view in the cleanup

    for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);        }

Implement the createImageViews   

	void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType 						= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image 						= swapChainImages[i];
            createInfo.viewType 					= VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format 						= swapChainImageFormat;
            createInfo.components.r 				= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g 				= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b 				= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a 				= VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel= 0;
            createInfo.subresourceRange.levelCount 	= 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount 	= 1;
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }
