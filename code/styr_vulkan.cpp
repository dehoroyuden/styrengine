//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

internal void CreateDepthResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *DepthImageView, VkSampleCountFlagBits MSAA_Samples);

internal void
CreateColorResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *ColorImageView, VkFormat SwapChainImageFormat, u32 MipLevels, VkSampleCountFlagBits MSAA_Samples);

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	VkSurfaceFormatKHR Formats[10];
	VkPresentModeKHR PresentModes[10];
};

internal VkCommandBuffer
BeginSingleTimeCommands(VkDevice LogicalDevice, VkCommandPool &CommandPool)
{
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = CommandPool;
	AllocInfo.commandBufferCount = 1;
	
	VkCommandBuffer CommandBuffer = {};
	Assert_(vkAllocateCommandBuffers(LogicalDevice, &AllocInfo, &CommandBuffer), "Command Buffer Allocated : %s");
	
	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
	
	return CommandBuffer;
}

internal void
EndSingleTimeCommands(VkCommandBuffer CommandBuffer, VkCommandPool &CommandPool, VkQueue GraphicsQueue, VkDevice LogicalDevice)
{
	vkEndCommandBuffer(CommandBuffer);
	
	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;
	
	vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(GraphicsQueue);
	
	vkFreeCommandBuffers(LogicalDevice, CommandPool, 1, &CommandBuffer);
}

inline b32
HasStencilComponent(VkFormat Format)
{
	return Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT;
}

inline void
TransitionImageLayout(VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, u32 MipLevels)
{
	VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(LogicalDevice, *CommandPool);
	
	VkImageMemoryBarrier Barrier = {};
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.oldLayout = OldLayout;
	Barrier.newLayout = NewLayout;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image;
	Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = MipLevels;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = 1;
	
	VkPipelineStageFlags SourceStage = {};
	VkPipelineStageFlags DestinationStage = {};
	
	if(NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		
		if(HasStencilComponent(Format))
		{
			Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	
	if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} 
	else if(OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		
		SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		Assert(!"unsupported layout transition!");
	}
	
	vkCmdPipelineBarrier(CommandBuffer,
						 SourceStage, DestinationStage,
						 0,
						 0, 0,
						 0, 0,
						 1, &Barrier);
	
	EndSingleTimeCommands(CommandBuffer, *CommandPool, GraphicsQueue, LogicalDevice);
}

internal void
CopyBufferToImage(VkDevice LogicalDevice, VkCommandPool &CommandPool, VkQueue GraphicsQueue, VkBuffer Buffer, VkImage Image, u32 Width, u32 Height)
{
	VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(LogicalDevice, CommandPool);
	
	VkBufferImageCopy Region = {};
	Region.bufferOffset = 0;
	Region.bufferRowLength = 0;
	Region.bufferImageHeight = 0;
	
	Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = 1;
	
	Region.imageOffset = {0,0,0};
	Region.imageExtent = 
	{
		Width,
		Height,
		1
	};
	
	vkCmdCopyBufferToImage(CommandBuffer,
						   Buffer,
						   Image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,
						   &Region);
	
	EndSingleTimeCommands(CommandBuffer, CommandPool, GraphicsQueue, LogicalDevice);
}

internal void
Win32VkRecordCommandBuffer(VkViewport *Viewport, VkRect2D *Scissor, VkCommandBuffer CommandBuffer, VkRenderPass RenderPass, 
						   VkPipeline GraphicsPipeline, VkPipelineLayout PipelineLayout, VkDescriptorSet *DescriptorSets, VkFramebuffer *SwapChainFramebuffers, VkExtent2D &SwapChainExtent, VkBuffer VertexBuffer, VkBuffer IndexBuffer, u32 VerticesCount, u32 IndicesCount, u32 CurrentFrame, u32 ImageIndex)
{
	// COMMAND BUFFER RECORDING
	VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
	CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = 0;
	b32 IsCommandBufferBeginInfoCreated = false;
	IsCommandBufferBeginInfoCreated = (vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo) == VK_SUCCESS);
	Assert(IsCommandBufferBeginInfoCreated);
	// End of COMMAND BUFFER RECORDING
	
	// Starting a Render Pass
	VkRenderPassBeginInfo RenderPassBeginInfo = {};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.framebuffer = SwapChainFramebuffers[ImageIndex];
	RenderPassBeginInfo.renderArea.offset = {0, 0};
	RenderPassBeginInfo.renderArea.extent = SwapChainExtent;
	
	// NOTE(Denis): Note that the order of clearValues should be identical to the order of our attachments.
	VkClearValue ClearValues[2];
	ClearValues[0].color = {{0.0f,0.0f,0.0f, 1.0f }};
	ClearValues[1].depthStencil = {1.0f,0};
	
	RenderPassBeginInfo.clearValueCount = ArrayCount(ClearValues);
	RenderPassBeginInfo.pClearValues = ClearValues;
	vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	// End of Starting a Render Pass 
	
	// Basic drawing commands
	vkCmdSetViewport(CommandBuffer, 0, 1, Viewport);
	vkCmdSetScissor(CommandBuffer, 0, 1, Scissor);
	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
	VkBuffer VertexBuffers[] = {VertexBuffer};
	VkDeviceSize Offsets[] = {0};
	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
	vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSets[CurrentFrame], 0, 0);
	vkCmdDrawIndexed(CommandBuffer, IndicesCount, 1, 0, 0, 0);
	vkCmdEndRenderPass(CommandBuffer);
	
	b32 IsEndCommandBuffer = false;
	IsEndCommandBuffer = (vkEndCommandBuffer(CommandBuffer) == VK_SUCCESS);
	Assert(IsEndCommandBuffer);
	// End of Basic drawing commands
}

internal void
Win32VkCreateSwapChain(u32 QueueFamilyCount, VkQueueFamilyProperties *QueueFamilies, VkPhysicalDevice PhysicalDevice,
					   VkSurfaceKHR VulkanWindowSurface, QueueFamilyIndices QueueIndices, r32 QueuePriority,
					   VkDeviceCreateInfo &CreateDeviceInfo, VkDevice LogicalDevice, VkFormat &SwapChainImageFormat, VkSwapchainKHR &SwapChain, VkViewport *Viewport, VkRect2D *Scissor, VkExtent2D &SwapChainExtent, VkQueue &PresentQueue, VkImage *SwapChainImages, u32 &ImageCountOutside)
{
	// Creating presentation queue
	for(u32 QueueFamilyIndex = 0;
		QueueFamilyIndex < QueueFamilyCount;
		++QueueFamilyIndex)
	{
		if(QueueFamilies[QueueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			VkBool32 PresentSupport = false;
			Assert_(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamilyIndex, VulkanWindowSurface, &PresentSupport), "Physical device surface support : %s");
			if(PresentSupport)
			{
				QueueIndices.PresentFamily = QueueFamilyIndex;
				Assert_NotVulkan("Device Support Present", "%s");
			}
		}
	}
	
	PresentQueue = {};
	VkDeviceQueueCreateInfo QueueCreateInfos[2];
	for(u32 Index = 0;
		Index < 2;
		++Index)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = {};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = Index;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos[Index] = QueueCreateInfo;
	}
	
	CreateDeviceInfo.queueCreateInfoCount = ArrayCount(QueueCreateInfos);
	CreateDeviceInfo.pQueueCreateInfos = QueueCreateInfos;
	
	vkGetDeviceQueue(LogicalDevice,QueueIndices.PresentFamily,0,&PresentQueue);
	// END
	
	// Create SwapChain
	// Querying details of swap chain support
	SwapChainSupportDetails SupportDetails = {};
	Assert_(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, VulkanWindowSurface, &SupportDetails.Capabilities), "Physical Device Surface Capabilities : %s");
	
	u32 SurfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, VulkanWindowSurface, &SurfaceFormatCount, 0);
	Assert_NotVulkan(SurfaceFormatCount, "Surface format count : %d");
	if(SurfaceFormatCount > 0)
	{
		Assert_(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, VulkanWindowSurface, &SurfaceFormatCount, SupportDetails.Formats),"Physical Device Surface Format : %s");
	}
	
	u32 SurfacePresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, VulkanWindowSurface, &SurfacePresentModeCount, 0);
	Assert_NotVulkan(SurfacePresentModeCount, "Surface present mode count : %d");
	if(SurfacePresentModeCount > 0)
	{
		Assert_(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, VulkanWindowSurface, &SurfacePresentModeCount, SupportDetails.PresentModes), "Physical device present modes : %s");
	}
	
	b32 AllNeededFormatsAvailable = false;
	VkSurfaceFormatKHR AvailableFormat = {};
	for(u32 Index = 0;
		Index < ArrayCount(SupportDetails.Formats);
		++Index)
	{
		if(SupportDetails.Formats[Index].format == VK_FORMAT_B8G8R8A8_SRGB && SupportDetails.Formats[Index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			AvailableFormat = SupportDetails.Formats[Index];
			AllNeededFormatsAvailable = true;
			Assert_NotVulkan("Format == VK_FORMAT_B8G8R8A8_SRGB | colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KH", "%s true");
		}
	}
	
	// Presentation mode
	// The presentation mode is arguably the most important setting for the swap chain, because it represents the actual conditions 
	// for showing images to the screen
	
	b32 NeededPresentModeAvailable = false;
	VkPresentModeKHR AvailablePresentMode = {};
	for(u32 Index = 0;
		Index < ArrayCount(SupportDetails.PresentModes);
		++Index)
	{
		if(SupportDetails.PresentModes[Index] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			AvailablePresentMode = SupportDetails.PresentModes[Index];
			NeededPresentModeAvailable = true;
			Assert_NotVulkan("PresentModes VK_PRESENT_MODE_MAILBOX_KHR", "%s");
			break;
		}
		else if(SupportDetails.PresentModes[Index] == VK_PRESENT_MODE_FIFO_KHR)
		{
			AvailablePresentMode = SupportDetails.PresentModes[Index];
			NeededPresentModeAvailable = false;
			Assert_NotVulkan("PresentModes VK_PRESENT_MODE_FIFO_KHR", "%s");
		}
	}
	
	Assert_NotVulkan(NeededPresentModeAvailable ? "true" : "false","In best case we need : VK_PRESENT_MODE_MAILBOX_KHR\n VK_PRESENT_MODE_MAILBOX_KHR available = %s\n VK_PRESENT_MODE_FIFO_KHR = Always Present");
	
	// Swap Extent
	// TODO(Denis): Change this later to variables, not harcoded values!!!
	u32 Width = 0;
	u32 Height = 0;
	
	RECT Rect;
	if(GetWindowRect(GlobalWindowHandle, &Rect))
	{
		Width = Rect.right - Rect.left;
		Height = Rect.bottom - Rect.top;
	}
	
	VkExtent2D ActualExtent = {Width, Height};
	ActualExtent.width = (u32)Clamp((r32)SupportDetails.Capabilities.minImageExtent.width,
									(r32)ActualExtent.width,
									(r32)SupportDetails.Capabilities.maxImageExtent.width);
	
	ActualExtent.height = (u32)Clamp((r32)SupportDetails.Capabilities.minImageExtent.height,
									 (r32)ActualExtent.height,
									 (r32)SupportDetails.Capabilities.maxImageExtent.height);
	
	
	// Create the swap chain
	u32 ImageCount = SupportDetails.Capabilities.minImageCount + 1;
	
	if(SupportDetails.Capabilities.maxImageCount > 0 && ImageCount > SupportDetails.Capabilities.maxImageCount)
	{
		ImageCount = SupportDetails.Capabilities.maxImageCount;
	}
	
	ImageCountOutside = ImageCount;
	
	VkSwapchainCreateInfoKHR SwapchainInfo = {};
	SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapchainInfo.surface = VulkanWindowSurface;
	SwapchainInfo.minImageCount = ImageCount;
	SwapchainInfo.imageFormat = AvailableFormat.format;
	SwapchainInfo.imageColorSpace = AvailableFormat.colorSpace;
	SwapchainInfo.imageExtent = ActualExtent;
	SwapchainInfo.imageArrayLayers = 1;
	SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SwapchainInfo.preTransform = SupportDetails.Capabilities.currentTransform;
	SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainInfo.presentMode = AvailablePresentMode;
	SwapchainInfo.clipped = VK_TRUE;
	SwapchainInfo.oldSwapchain = VK_NULL_HANDLE;
	
	u32 Vk_QueueFamilyIndices[] = {QueueIndices.GraphicsFamily,QueueIndices.PresentFamily};
	
	if(QueueIndices.GraphicsFamily != QueueIndices.PresentFamily)
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapchainInfo.queueFamilyIndexCount = 2;
		SwapchainInfo.pQueueFamilyIndices = Vk_QueueFamilyIndices;
	}
	else
	{
		SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		SwapchainInfo.queueFamilyIndexCount = 0;
		SwapchainInfo.pQueueFamilyIndices = 0;
	}
	
	// SWAP CHAIN IMAGES!
	
	SwapChainImageFormat = AvailableFormat.format;
	SwapChainExtent = ActualExtent;
	
	if(Viewport)
	{
		Viewport->x = 0.0f;
		Viewport->y = 0.0f;
		Viewport->width = (r32)SwapChainExtent.width;
		Viewport->height = (r32)SwapChainExtent.height;
		Viewport->minDepth = 0.0f;
		Viewport->maxDepth = 1.0f;
	}
	
	if(Scissor)
	{
		Scissor->offset = {0, 0};
		Scissor->extent = SwapChainExtent;
	}
	
	Assert_(vkCreateSwapchainKHR(LogicalDevice, &SwapchainInfo, 0, &SwapChain), "Swapchain Created : %s");
	
	Assert_(vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &ImageCount, 0), "Swapchain Image Get : %s");
	Assert_(vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &ImageCount, SwapChainImages), "Swapchain Image Get : %s");
	
	Assert_NotVulkan("Here was end last error prone momen", "%s");
	// END
}

internal VkImageView
CreateImageView(VkDevice LogicalDevice, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags, u32 MipLevels)
{
	VkImageViewCreateInfo ImageViewCreateInfo = {};
	ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.image = Image;
	ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ImageViewCreateInfo.format = Format;
	
	ImageViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.levelCount = MipLevels;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	
	VkImageView Result = {};
	
	Assert_(vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, 0, &Result), "Texture Image View Creation : %s");
	
	return Result;
}

internal void
Win32VkCreateImageViews(VkImageView *SwapChainImageViews, VkImage *SwapChainImages, VkFormat &SwapChainImageFormat, VkDevice LogicDevice, u32 ImageCount)
{
	for(u32 Index = 0;
		Index < ImageCount;
		++Index)
	{
		SwapChainImageViews[Index] = CreateImageView(LogicDevice, SwapChainImages[Index], SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

internal void
Win32VkCreateFramebuffers(VkImageView *SwapChainImageViews, u32 ImageViewsCount, VkImageView DepthImageView, VkImageView ColorImageView, VkExtent2D &SwapChainExtent, VkRenderPass RenderPass, VkDevice LogicalDevice,
						  VkFramebuffer *SwapChainFramebuffers)
{
	// Framebuffers
	//
	//
	
	for(u32 Index = 0;
		Index < ImageViewsCount;
		++Index)
	{
		VkImageView Attachments[] = {ColorImageView, DepthImageView, SwapChainImageViews[Index]};
		
		VkFramebufferCreateInfo FramebufferInfo = {};
		FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferInfo.renderPass = RenderPass;
		FramebufferInfo.attachmentCount = ArrayCount(Attachments);
		FramebufferInfo.pAttachments = Attachments;
		FramebufferInfo.width = SwapChainExtent.width;
		FramebufferInfo.height = SwapChainExtent.height;
		FramebufferInfo.layers = 1;
		
		b32 IsFramebufferCreated = ((vkCreateFramebuffer(LogicalDevice, &FramebufferInfo, 0, &SwapChainFramebuffers[Index])) == VK_SUCCESS);
		
	}
	
	//
	//
	// End of Framebuffers
	
}

internal void
Win32VkCleanupSwapChain(VkDevice LogicDevice, VkFramebuffer *SwapChainFramebuffers, u32 SwapChainFramebuffersCount, VkImageView *SwapChainImageViews, u32 SwapChainImageViewsCount, VkSwapchainKHR SwapChain)
{
	for(u32 Index = 0;
		Index < SwapChainFramebuffersCount;
		++Index)
	{
		vkDestroyFramebuffer(LogicDevice, SwapChainFramebuffers[Index], 0);
	}
	
	for(u32 Index = 0;
		Index < SwapChainImageViewsCount;
		++Index)
	{
		vkDestroyImageView(LogicDevice, SwapChainImageViews[Index], 0);
	}
	
	vkDestroySwapchainKHR(LogicDevice, SwapChain, 0);
}

internal void
Win32VkRecreateSwapChain(u32 QueueFamilyCount, VkQueueFamilyProperties *QueueFamilies, VkPhysicalDevice PhysicalDevice,
						 VkSurfaceKHR VulkanWindowSurface, QueueFamilyIndices QueueIndices, r32 QueuePriority,
						 VkDeviceCreateInfo &CreateDeviceInfo, VkDevice LogicDevice, VkCommandPool *CommandPool, VkQueue *GraphicQueue, VkFormat &SwapChainImageFormat, VkFramebuffer *SwapChainFramebuffers, u32 SwapChainFramebuffersCount, VkImageView *SwapChainImageViews, u32 SwapChainImageViewsCount, VkImageView DepthImageView, VkImage *DepthImage, VkDeviceMemory *DepthImageMemory, VkImageView ColorImageView, VkImage *ColorImage, VkDeviceMemory *ColorImageMemory,VkSwapchainKHR &SwapChain, VkViewport *Viewport, VkRect2D *Scissor, VkExtent2D &SwapChainExtent, VkQueue &PresentQueue, VkImage *SwapChainImages, u32 SwapChainImagesCount, u32 MipLevels,  VkRenderPass RenderPass, u32 &ImageCountOutside, VkSampleCountFlagBits MSAA_Samples)
{
	u32 Width = 0;
	u32 Height = 0;
	
	RECT Rect;
	if(GetWindowRect(GlobalWindowHandle, &Rect))
	{
		Width = Rect.right - Rect.left;
		Height = Rect.bottom - Rect.top;
	}
	
	while(Width == 0 || Height == 0)
	{
		Sleep(100);
	}
	
	vkDeviceWaitIdle(LogicDevice);
	
	Win32VkCleanupSwapChain(LogicDevice, SwapChainFramebuffers, SwapChainFramebuffersCount, SwapChainImageViews, SwapChainImageViewsCount, SwapChain);
	
	Win32VkCreateSwapChain(QueueFamilyCount, QueueFamilies,PhysicalDevice, VulkanWindowSurface, QueueIndices, QueuePriority,
						   CreateDeviceInfo, LogicDevice, SwapChainImageFormat, SwapChain, Viewport, Scissor, SwapChainExtent, 
						   PresentQueue, SwapChainImages, ImageCountOutside);
	
	Win32VkCreateImageViews(SwapChainImageViews, SwapChainImages, SwapChainImageFormat, LogicDevice, ImageCountOutside);
	
	CreateColorResources(PhysicalDevice, LogicDevice, CommandPool, *GraphicQueue, SwapChainExtent, ColorImage, ColorImageMemory, &ColorImageView, SwapChainImageFormat, MipLevels, MSAA_Samples);
	
	CreateDepthResources(PhysicalDevice, LogicDevice, CommandPool, *GraphicQueue, SwapChainExtent, DepthImage, DepthImageMemory, &DepthImageView, MSAA_Samples);
	
	Win32VkCreateFramebuffers(SwapChainImageViews, SwapChainImageViewsCount, DepthImageView, ColorImageView, SwapChainExtent, RenderPass, LogicDevice, SwapChainFramebuffers);
}

inline VkVertexInputBindingDescription
GetBindingDescription()
{
	VkVertexInputBindingDescription Result = {};
	Result.binding = 0;
	Result.stride = sizeof(Vertex);
	Result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	return Result;
}

inline Array
GetAttributeDescriptions(memory_arena *TranArena)
{
	Array Result = {};
	Result.Count = 3;
	Result.Size = sizeof(VkVertexInputAttributeDescription) * Result.Count;
	Result.Base = PushArray(TranArena, Result.Count, VkVertexInputAttributeDescription);
	
	VkVertexInputAttributeDescription *AttributeDescriptions = (VkVertexInputAttributeDescription *)Result.Base;
	
	AttributeDescriptions[0].binding = 0;
	AttributeDescriptions[0].location = 0;
	AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	AttributeDescriptions[0].offset = offsetof(Vertex, Pos);
	
	AttributeDescriptions[1].binding = 0;
	AttributeDescriptions[1].location = 1;
	AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	AttributeDescriptions[1].offset = offsetof(Vertex, Color);
	
	AttributeDescriptions[2].binding = 0;
	AttributeDescriptions[2].location = 2;
	AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	AttributeDescriptions[2].offset = offsetof(Vertex, TexCoord);
	
	return Result;
}

u32 FindMemoryType(VkPhysicalDevice PhysicalDevice, u32 TypeFilter, VkMemoryPropertyFlags Properties)
{
	VkPhysicalDeviceMemoryProperties MemProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProperties);
	
	for(u32 Index = 0;
		Index < MemProperties.memoryTypeCount;
		++Index)
	{
		if((TypeFilter & (1 << Index)) && (MemProperties.memoryTypes[Index].propertyFlags & Properties) == Properties)
		{
			return Index;
		}
	}
	
	Assert(!"Failded to find suitable memory type!");
	
	return 0xFFFF;
}

inline void 
CreateBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties, VkBuffer *Buffer, VkDeviceMemory *BufferMemory)
{
	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = Size;
	BufferInfo.usage = Usage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	Assert_(vkCreateBuffer(LogicalDevice, &BufferInfo, 0, Buffer), "Buffer Created : %s");
	
	VkMemoryRequirements MemRequirements = {};
	vkGetBufferMemoryRequirements(LogicalDevice, *Buffer, &MemRequirements);
	
	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = MemRequirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, MemRequirements.memoryTypeBits, Properties);
	
	Assert_(vkAllocateMemory(LogicalDevice, &AllocInfo, 0, BufferMemory), "Buffer Memory Allocated : %s");
	
	vkBindBufferMemory(LogicalDevice, *Buffer, *BufferMemory, 0);
}

inline void
CopyBuffer(VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkBuffer SrcBuffer, VkBuffer DstBuffer, VkDeviceSize Size)
{
	VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(LogicalDevice, *CommandPool);
	
	VkBufferCopy CopyRegion = {};
	CopyRegion.size = Size;
	vkCmdCopyBuffer(CommandBuffer, SrcBuffer, DstBuffer, 1, &CopyRegion);
	
	EndSingleTimeCommands(CommandBuffer,*CommandPool, GraphicsQueue, LogicalDevice);
}

inline void
CreateVertexBuffer(VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkBuffer &VertexBuffer, VkDeviceMemory *VertexBufferMemory, VkPhysicalDevice PhysicalDevice,
				   VkDevice LogicalDevice, Vertex *Vertices, u32 VertexCount)
{
	VkDeviceSize BufferSize = VertexCount * sizeof(Vertex);
	
	VkBuffer StagingBuffer = {};
	VkDeviceMemory StagingBufferMemory = {};
	CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				 &StagingBuffer, &StagingBufferMemory);
	
	void *VertexBufferData;
	vkMapMemory(LogicalDevice, StagingBufferMemory, 0 , BufferSize, 0, &VertexBufferData);
	memcpy(VertexBufferData, Vertices, BufferSize);
	vkUnmapMemory(LogicalDevice, StagingBufferMemory);
	
	CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &VertexBuffer, VertexBufferMemory);
	
	CopyBuffer(LogicalDevice, CommandPool, GraphicsQueue, StagingBuffer, VertexBuffer, BufferSize);
	
	vkDestroyBuffer(LogicalDevice, StagingBuffer, 0);
	vkFreeMemory(LogicalDevice, StagingBufferMemory, 0);
}

inline void
CreateIndexBuffer(VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkBuffer &IndicesBuffer, VkDeviceMemory *IndicesBufferMemory, VkPhysicalDevice PhysicalDevice,
				  VkDevice LogicalDevice, u32 *Indices, u32 IndicesCount)
{
	VkDeviceSize BufferSize = IndicesCount * sizeof(u32);
	
	VkBuffer StagingBuffer = {};
	VkDeviceMemory StagingBufferMemory = {};
	CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				 &StagingBuffer, &StagingBufferMemory);
	
	void *IndicesBufferData;
	vkMapMemory(LogicalDevice, StagingBufferMemory, 0 , BufferSize, 0, &IndicesBufferData);
	memcpy(IndicesBufferData, Indices, BufferSize);
	vkUnmapMemory(LogicalDevice, StagingBufferMemory);
	
	CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &IndicesBuffer, IndicesBufferMemory);
	
	CopyBuffer(LogicalDevice, CommandPool, GraphicsQueue, StagingBuffer, IndicesBuffer, BufferSize);
	
	vkDestroyBuffer(LogicalDevice, StagingBuffer, 0);
	vkFreeMemory(LogicalDevice, StagingBufferMemory, 0);
}

inline void
CreateDescriptorSetLayout(VkDevice LogicDevice, VkDescriptorSetLayout *DescriptorSetLayout)
{
	VkDescriptorSetLayoutBinding UboLayoutBinding = {};
	UboLayoutBinding.binding = 0;
	UboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UboLayoutBinding.descriptorCount = 1;
	UboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	UboLayoutBinding.pImmutableSamplers = 0;
	
	VkDescriptorSetLayoutBinding SamplerLayoutBinding = {};
	SamplerLayoutBinding.binding = 1;
	SamplerLayoutBinding.descriptorCount =1;
	SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SamplerLayoutBinding.pImmutableSamplers = 0;
	SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	VkDescriptorSetLayoutBinding Bindings[] = {UboLayoutBinding, SamplerLayoutBinding};
	VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
	LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutInfo.bindingCount = ArrayCount(Bindings);
	LayoutInfo.pBindings = Bindings;
	
	b32 IsDescriptorSetLayoutIsCreated = false;
	IsDescriptorSetLayoutIsCreated = (vkCreateDescriptorSetLayout(LogicDevice, &LayoutInfo, 0, DescriptorSetLayout) == VK_SUCCESS);
	Assert(IsDescriptorSetLayoutIsCreated);
}

internal void
CreateUniformBuffers(memory_arena *TranArena, VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkBuffer *UniformBuffers, VkDeviceMemory *UniformBuffersMemory)
{
	VkDeviceSize BufferSize = sizeof(UniformBufferObject);
	
	for(u32 Index = 0;
		Index < MAX_FRAMES_IN_FLIGHT;
		++Index)
	{
		CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &UniformBuffers[Index], &UniformBuffersMemory[Index]);
	}
}

internal void
Win32UpdateUniformBuffer(VkDevice LogicDevice, VkExtent2D SwapChainExtent, VkDeviceMemory *UniformBuffersMemory, u32 CurrentImage)
{
	static auto StartTime = std::chrono::high_resolution_clock::now();
	
	auto CurrentTime = std::chrono::high_resolution_clock::now();
	r32 Time = std::chrono::duration<r32, std::chrono::seconds::period>(CurrentTime - StartTime).count();
	
	UniformBufferObject Ubo = {};
	Ubo.Model = glm::rotate(glm::mat4(1.0f), Time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	Ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0, 0, 0), glm::vec3(0,0,1.0f));
	Ubo.Proj = ConvertFromMatrix4x4(PerspectiveProjection(RadiansFromDegrees(45.0f), (r32)SwapChainExtent.width / (r32)SwapChainExtent.height, 0.2f, 10.0f));
	Ubo.Proj[1][1] *= -1;
	
	void *Data;
	vkMapMemory(LogicDevice, UniformBuffersMemory[CurrentImage], 0, sizeof(Ubo), 0, &Data);
	memcpy(Data, &Ubo, sizeof(Ubo));
}


internal void
CreateDescriptorPool(VkDevice LogicDevice, VkDescriptorPool *DescriptorPool)
{
	VkDescriptorPoolSize PoolSizes[2] = {{},{}};
	PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
	PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	PoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;
	
	VkDescriptorPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.poolSizeCount = ArrayCount(PoolSizes);
	PoolInfo.pPoolSizes = PoolSizes;
	PoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
	
	b32 IsDescriptorPoolCreated = false;
	IsDescriptorPoolCreated = (vkCreateDescriptorPool(LogicDevice, &PoolInfo, 0, DescriptorPool) == VK_SUCCESS);
	Assert(IsDescriptorPoolCreated);
}

internal void
CreateDescriptorSets(memory_arena *TranArena, VkDevice LogicDevice, VkBuffer *UniformBuffers, VkDescriptorPool *DescriptorPool, VkDescriptorSet *DescriptorSets, VkDescriptorSetLayout DescriptorSetLayout, VkImageView TextureImageView, VkSampler TextureSampler)
{
	VkDescriptorSetLayout Layouts[MAX_FRAMES_IN_FLIGHT];
	Layouts[0] = DescriptorSetLayout;
	Layouts[1] = DescriptorSetLayout;
	VkDescriptorSetAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = *DescriptorPool;
	AllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	AllocInfo.pSetLayouts = Layouts;
	
	b32 IsDescriptorSetAllocated = false;
	IsDescriptorSetAllocated = (vkAllocateDescriptorSets(LogicDevice, &AllocInfo, DescriptorSets) == VK_SUCCESS);
	
	for(u32 Index = 0;
		Index < MAX_FRAMES_IN_FLIGHT;
		++Index)
	{
		VkDescriptorBufferInfo BufferInfo = {};
		BufferInfo.buffer = UniformBuffers[Index];
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(UniformBufferObject);
		
		VkDescriptorImageInfo ImageInfo = {};
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageInfo.imageView = TextureImageView;
		ImageInfo.sampler = TextureSampler;
		
		VkWriteDescriptorSet DescriptorWrites[2] = {{},{}};
		
		DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescriptorWrites[0].dstSet = DescriptorSets[Index];
		DescriptorWrites[0].dstBinding = 0;
		DescriptorWrites[0].dstArrayElement = 0;
		DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DescriptorWrites[0].descriptorCount = 1;
		DescriptorWrites[0].pBufferInfo = &BufferInfo;
		
		DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescriptorWrites[1].dstSet = DescriptorSets[Index];
		DescriptorWrites[1].dstBinding = 1;
		DescriptorWrites[1].dstArrayElement = 0;
		DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		DescriptorWrites[1].descriptorCount = 1;
		DescriptorWrites[1].pImageInfo = &ImageInfo;
		
		u32 ArraySize = ArrayCount(DescriptorWrites);
		vkUpdateDescriptorSets(LogicDevice, ArraySize, DescriptorWrites, 0, 0);
	}
	
}

internal void
CreateImage(VkPhysicalDevice PhysicalDevice,VkDevice LogicalDevice, u32 Width, u32 Height, u32 MipLevels, VkSampleCountFlagBits NumSamples, VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags Usage, VkMemoryPropertyFlags Properties, VkImage *Image, VkDeviceMemory &ImageMemory)
{
	VkImageCreateInfo ImageInfo = {};
	ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = Width;
    ImageInfo.extent.height = Height;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = MipLevels;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = Format;
    ImageInfo.tiling = Tiling;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = Usage;
    ImageInfo.samples = NumSamples;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	Assert_(vkCreateImage(LogicalDevice, &ImageInfo, 0, Image),"Image Create, line 1087 - %s");
	
	VkMemoryRequirements MemoryRequirements = {};
	vkGetImageMemoryRequirements(LogicalDevice, *Image, &MemoryRequirements);
	
	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = MemoryRequirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);
	
	b32 IsMemoryAllocated = false;
	IsMemoryAllocated = (vkAllocateMemory(LogicalDevice, &AllocInfo, 0, &ImageMemory) == VK_SUCCESS);
	Assert(IsMemoryAllocated);
	
	vkBindImageMemory(LogicalDevice, *Image, ImageMemory, 0);
}

internal void
GenerateMipmaps(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool &CommandPool, VkQueue GraphicsQueue, VkImage Image, VkFormat ImageFormat, u32 TextureWidth, u32 TextureHeight, u32 MipLevels)
{
	// NOTE(Denis): Check if image format supports linear blitting
	VkFormatProperties FormatProperties = {};
	vkGetPhysicalDeviceFormatProperties(PhysicalDevice, ImageFormat, &FormatProperties);
	Assert((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT));
	
	VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(LogicalDevice, CommandPool);
	VkImageMemoryBarrier Barrier = {};
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.image = Image;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = 1;
	Barrier.subresourceRange.levelCount = 1;
	
	u32 MipWidth = TextureWidth;
	u32 MipHeight = TextureHeight;
	
	for(u32 i = 1;
		i < MipLevels;
		++i)
	{
		Barrier.subresourceRange.baseMipLevel = i - 1;
		Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		vkCmdPipelineBarrier(CommandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, 0,
							 0, 0,
							 1, &Barrier);
		
		VkImageBlit Blit = {};
		Blit.srcOffsets[0] = {0, 0, 0};
		Blit.srcOffsets[1] = {(s32)MipWidth, (s32)MipHeight, 1};
		Blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Blit.srcSubresource.mipLevel = i - 1;
		Blit.srcSubresource.baseArrayLayer = 0;
		Blit.srcSubresource.layerCount = 1;
		Blit.dstOffsets[0] = {0, 0, 0};
		Blit.dstOffsets[1] = {((s32)MipWidth > 1) ? ((s32)MipWidth / 2) : 1, ((s32)MipHeight > 1) ? ((s32)MipHeight / 2) : 1, 1};
		Blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Blit.dstSubresource.mipLevel = i;
		Blit.dstSubresource.baseArrayLayer = 0;
		Blit.dstSubresource.layerCount = 1;
		
		vkCmdBlitImage(CommandBuffer,
					   Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &Blit,
					   VK_FILTER_LINEAR);
		
		Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		vkCmdPipelineBarrier(CommandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, 0,
							 0, 0,
							 1, &Barrier);
		
		if(MipWidth > 1) MipWidth /= 2;
		if(MipHeight > 1) MipHeight /= 2;
	}
	
	Barrier.subresourceRange.baseMipLevel = MipLevels - 1;
	Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	
	vkCmdPipelineBarrier(CommandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, 0,
						 0, 0,
						 1, &Barrier);
	
	EndSingleTimeCommands(CommandBuffer, CommandPool, GraphicsQueue, LogicalDevice);
}

internal void 
CreateTextureImage(VkImage *TextureImage, VkDeviceMemory *TextureImageMemory, VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool &CommandPool, VkQueue GraphicsQueue, VkBuffer *Buffer, VkDeviceMemory *BufferMemory, u32 &MipLevels)
{
	s32 TextureWidth;
	s32 TextureHeight;
	s32 TextureChannels;
	
	stbi_uc *Pixels = stbi_load(TEXTURE_PATH, &TextureWidth, &TextureHeight, &TextureChannels, STBI_rgb_alpha);
	MipLevels = (Log2(Maximum(TextureWidth, TextureHeight))) + 1;
	
	VkDeviceSize ImageSize = TextureWidth * TextureHeight * 4; // 4 - is bytes per pixel
	
	Assert(Pixels);
	
	VkBuffer StagingBuffer = {};
	VkDeviceMemory StagingBufferMemory = {};
	
	CreateBuffer(PhysicalDevice, LogicalDevice, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &StagingBuffer, &StagingBufferMemory);
	
	void *Data;
	vkMapMemory(LogicalDevice, StagingBufferMemory, 0, ImageSize, 0, &Data);
	memcpy(Data, Pixels, (umm)ImageSize);
	vkUnmapMemory(LogicalDevice, StagingBufferMemory);
	
	stbi_image_free(Pixels);
	
	CreateImage(PhysicalDevice, LogicalDevice, TextureWidth, TextureHeight, MipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, TextureImage, *TextureImageMemory);
	
	TransitionImageLayout(LogicalDevice, &CommandPool, GraphicsQueue, *TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, MipLevels);
	
	CopyBufferToImage(LogicalDevice, CommandPool, GraphicsQueue, StagingBuffer, *TextureImage, TextureWidth, TextureHeight);
	
	vkDestroyBuffer(LogicalDevice, StagingBuffer, 0);
	vkFreeMemory(LogicalDevice, StagingBufferMemory, 0);
	
	GenerateMipmaps(PhysicalDevice, LogicalDevice, CommandPool, GraphicsQueue, *TextureImage, VK_FORMAT_R8G8B8A8_SRGB, TextureWidth, TextureHeight, MipLevels);
}

internal void
CreateTextureSampler(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkSampler &TextureSampler, u32 MipLevels)
{
	VkSamplerCreateInfo SamplerInfo = {};
	SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	SamplerInfo.magFilter = VK_FILTER_LINEAR;
	SamplerInfo.minFilter = VK_FILTER_LINEAR;
	SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkPhysicalDeviceProperties Properties = {};
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	SamplerInfo.anisotropyEnable = VK_TRUE;
	SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
	SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;
	SamplerInfo.compareEnable = VK_FALSE;
	SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = (r32)MipLevels;
	SamplerInfo.mipLodBias = 0.0f;
	
	Assert_(vkCreateSampler(LogicalDevice, &SamplerInfo, 0, &TextureSampler), "Texture sampler creation : %s");
}

inline void
CreateTextureImageView(VkDevice LogicalDevice, VkImageView &TextureImageView, VkImage TextureImage, u32 MipLevels)
{
	TextureImageView = CreateImageView(LogicalDevice, TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, MipLevels);
}

internal b32 
IsDeviceSuitable(VkPhysicalDevice Device, VkPhysicalDeviceFeatures &DeviceFeatures)
{
	VkPhysicalDeviceProperties DeviceProperties;
	DeviceFeatures.samplerAnisotropy = VK_TRUE;
	
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
    vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);
	
    return DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		DeviceFeatures.geometryShader && DeviceFeatures.samplerAnisotropy;
}

internal VkSampleCountFlagBits
GetMaxUsableSampleCount(VkPhysicalDevice PhysicalDevice)
{
	VkPhysicalDeviceProperties PhysicalDeviceProperties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
	
	VkSampleCountFlags Counts = PhysicalDeviceProperties.limits.framebufferColorSampleCounts & PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
	
	if(Counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT;}
	if(Counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT;}
	if(Counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT;}
	if(Counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT;}
	if(Counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT;}
	if(Counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT;}
	
	return VK_SAMPLE_COUNT_1_BIT;
}

internal void
PickPhysicalDevice(VkInstance Instance, VkPhysicalDevice &PhysicalDevice, VkPhysicalDeviceFeatures &DeviceFeatures, VkSampleCountFlagBits &MSAA_Samples)
{
	u32 DeviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, 0);
	
	if(DeviceCount == 0)
	{
		Assert_NotVulkan("Failed to find GPUs with Vulkan support!","%s");
	}
	
	VkPhysicalDevice Devices[5];
	vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices);
	
	Assert_NotVulkan(DeviceCount, "Devices : %d");
	
	for(u32 Index = 0;
		Index < DeviceCount;
		++Index)
	{
		if(IsDeviceSuitable(Devices[Index], DeviceFeatures))
		{
			PhysicalDevice = Devices[Index];
			MSAA_Samples = GetMaxUsableSampleCount(PhysicalDevice);
			break;
		}
	}
	
	if(!PhysicalDevice)
	{
		Assert_NotVulkan("failed to find a suitable GPU","%s");
	}
}

internal VkFormat 
FindSupportedFormat(VkPhysicalDevice PhysicalDevice,  VkFormat *Candidates, u32 CandidatesCount, VkImageTiling Tiling, VkFormatFeatureFlags Features)
{
	for(u32 Index = 0;
		Index < CandidatesCount;
		++Index)
	{
		VkFormatProperties Props = {};
		vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Candidates[Index], &Props);
		
		if(Tiling == VK_IMAGE_TILING_LINEAR && (Props.linearTilingFeatures & Features) == Features)
		{
			return Candidates[Index];
		} 
		else if(Tiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & Features) == Features)
		{
			return Candidates[Index];
		}
	}
	
	Assert_NotVulkan("Failed to find supported format!","%s");
	
	return Candidates[-1];
}

internal VkFormat
FindDepthFormat(VkPhysicalDevice PhysicalDevice)
{
	VkFormat Formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return FindSupportedFormat(PhysicalDevice, Formats,
							   ArrayCount(Formats),
							   VK_IMAGE_TILING_OPTIMAL,
							   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

internal void
CreateColorResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *ColorImageView, VkFormat SwapChainImageFormat, u32 MipLevels, VkSampleCountFlagBits MSAA_Samples)
{
	VkFormat ColorFormat = SwapChainImageFormat;
	
	CreateImage(PhysicalDevice, LogicalDevice, SwapChainExtent.width, SwapChainExtent.height, MipLevels, MSAA_Samples, ColorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Image, *ImageMemory);
	
	*ColorImageView = CreateImageView(LogicalDevice, *Image, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

internal void
CreateDepthResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *DepthImageView, VkSampleCountFlagBits MSAA_Samples)
{
	VkFormat DepthFormat = FindDepthFormat(PhysicalDevice);
	
	CreateImage(PhysicalDevice, LogicalDevice, SwapChainExtent.width, SwapChainExtent.height, 1, MSAA_Samples, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Image, *ImageMemory);
	
	*DepthImageView = CreateImageView(LogicalDevice, *Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	
	TransitionImageLayout(LogicalDevice, CommandPool, GraphicsQueue, *Image, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	
}
