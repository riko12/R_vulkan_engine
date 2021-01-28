Swapchain frame buffers
Add member std::vector<VkFramebuffer> swapChainFramebuffers in the class definition

    std::vector<VkFramebuffer> swapChainFramebuffers;

Add implementation for framebuffer creation  

	createFramebuffers();
	
Add the destruction of the framebuffers object in cleanup

	for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
	
Add proper implementation of create frame buffers

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

Add commandPool and commandBuffers as new member in the class definition

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

Add createCommandPool and createCommandBuffers in the InitVulkan

	...
    createCommandPool();
    createCommandBuffers();

Add destruct commandPool in the cleanup

    vkDestroyCommandPool(device, commandPool, nullptr);

Add the implementation for the commandPool commandBuffer in the cpp

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }
    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;
            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }
    
Add const int MAX_FRAMES_IN_FLIGHT = 2 global

Add the proper member as described below i the class definition 

	std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
	 
Add createSyncObjects in the initVulkan

	...
	createSyncObjects();

Add the drawFrame in the mainloop after glfwPollEvens

	...
	drawFrame();

In while loop after the while loo is done add vkDeviceWaitIdle(device)
	While ...
	{
	...
	}
    vkDeviceWaitIdle(device);

Add in the Cleanup crearing all the synch objects created

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
 
Set the attribute values for the dependencies to sub path

    VkSubpassDependency dependency{};
    dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass 		= 0;
    dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask 	= 0;
    dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

Set finally the dependency and the dependency count

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

Add the implementation of createSyncObjects and drawFrame

    void createSyncObjects() {
        imageAvailableSemaphores.resize 	(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize 	(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize				(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize				(swapChainImages.size(), VK_NULL_HANDLE);
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType 				= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType 					= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags 					= VK_FENCE_CREATE_SIGNALED_BIT;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore			(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore			(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence				(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] 			= inFlightFences[currentFrame];
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] 		= {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] 	= {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount 		= 1;
        submitInfo.pWaitSemaphores 			= waitSemaphores;
        submitInfo.pWaitDstStageMask 		= waitStages;
        submitInfo.commandBufferCount 		= 1;
        submitInfo.pCommandBuffers 			= &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] 		= {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount 	= 1;
        submitInfo.pSignalSemaphores 		= signalSemaphores;
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType 					= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount 		= 1;
        presentInfo.pWaitSemaphores 		= signalSemaphores;
        VkSwapchainKHR swapChains[] 		= {swapChain};
        presentInfo.swapchainCount 			= 1;
        presentInfo.pSwapchains 			= swapChains;
        presentInfo.pImageIndices 			= &imageIndex;
        vkQueuePresentKHR(presentQueue, &presentInfo);
        currentFrame 						= (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }





















 