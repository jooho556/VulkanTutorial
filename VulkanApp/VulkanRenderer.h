#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer() {}
	~VulkanRenderer();

	int Init(const std::string& name = "VulkanApp",
		const int width = 800, const int height = 600);

	void Clean();

	void Update();

private:
	GLFWwindow* window = nullptr;

	//vk components
	VkInstance vk_instance;
	VkDebugUtilsMessengerEXT debug_messenger;

	struct MainDevice {
		VkPhysicalDevice physical_device;
		VkDevice logical_device;
	} devices;

	VkQueue graphics_queue;
	VkQueue presentation_queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<SwapchainImage> swapchain_images;

	//utilities
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;

	//vk functions
	void CreateInstance();
	bool CheckInstanceExtensionSupport(const std::vector<const char*>& check_extensions);

	bool CheckValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallback_data,
		void* pUser_data) {
		std::cerr << "validation layer: " << pCallback_data->pMessage << std::endl;
		return VK_FALSE;
	}
	void SetupDebugMessenger();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

	void GetPhysicalDevice();
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);

	void CreateLogicalDevice();

	void CreateSurface();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice device);
	void CreateSwapChain();
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentation_modes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

	void CreateGraphicsPipiline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
};
