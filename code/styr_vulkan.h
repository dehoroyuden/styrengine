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
	VkRenderPass handle;
	f32 DepthClearValue;
	u32 StencilCrearValue;
	
	b32 HasPreviewPass;
	b32 HasNextPass;
} vulkan_renderpass;

typedef struct vulkan_swapchain
{
	VkSwapchainKHR handle;
	VkSurfaceFormatKHR ImageFormat;
	u8 MaxFramesInFlight;
	u32 ImageCount;
} vulkan_swapchain;

typedef struct vulkan_context
{
	vulkan_device device;
	vulkan_swapchain swapchain;
	vulkan_renderpass main_renderpass;
	vulkan_renderpass ui_renderpass;
	
	u32 image_index;
	u32 current_frame;
	
} vulkan_context;

typedef struct vulkan_buffer
{
	u32 count;
	VkBuffer *data;
	VkDeviceMemory *memory;
	
} vulkan_buffer;

typedef struct vulkan_material
{
	VkPipeline pipeline;
	u32 textures_count;
	VkImage *textures;
} vulkan_material;

struct UniformBufferObject
{
	mat4 MVP_Final;
	mat4 View;
	mat4 Model;
	mat4 Proj;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	VkSurfaceFormatKHR Formats[10];
	VkPresentModeKHR PresentModes[10];
};

#endif //STYR_VULKAN_H
