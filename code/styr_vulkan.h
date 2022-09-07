#ifndef STYR_VULKAN_H
#define STYR_VULKAN_H
//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

typedef struct vulkan_device
{
	VkPhysicalDevice PhysicalDevice;
	VkDevice LogicalDevice;
	u32 GraphicsQueueIndex;
	u32 PresentQueueIndex;
	u32 TransferQueueIndex;
	VkQueue GraphicsQueue;
	VkQueue PresentQueue;
	VkQueue TransferQueue;
	VkCommandPool CommandPool;
	VkPhysicalDeviceProperties Properties;
	VkPhysicalDeviceFeatures Features;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	VkFormat DepthFormat;
	u32 DepthChannelCount;
} vulkan_device;

typedef struct vulkan_image 
{
	VkImage Handle;
	VkDeviceMemory Memory;
	VkImageView View;
	u32 Width;
	u32 Height;
} vulkan_image;

typedef struct vulkan_renderpass
{
	VkRenderPass Handle;
	f32 DepthClearValue;
	u32 StencilCrearValue;
	
	b32 HasPreviewPass;
	b32 HasNextPass;
} vulkan_renderpass;

typedef struct vulkan_swapchain
{
	VkSwapchainKHR Handle;
	VkSurfaceFormatKHR ImageFormat;
	u8 MaxFramesInFlight;
	u32 ImageCount;
} vulkan_swapchain;

typedef struct vulkan_buffer
{} vulkan_buffer;


internal void CreateDepthResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *DepthImageView, VkSampleCountFlagBits MSAA_Samples);

internal void
CreateColorResources(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, VkCommandPool *CommandPool, VkQueue GraphicsQueue, VkExtent2D SwapChainExtent, VkImage *Image, VkDeviceMemory *ImageMemory, VkImageView *ColorImageView, VkFormat SwapChainImageFormat, u32 MipLevels, VkSampleCountFlagBits MSAA_Samples);

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	VkSurfaceFormatKHR Formats[10];
	VkPresentModeKHR PresentModes[10];
};

#endif //STYR_VULKAN_H
