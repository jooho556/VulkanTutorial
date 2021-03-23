#include <iostream>
#include "VulkanRenderer.h"

const std::vector<const char*> required_validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

VulkanRenderer::~VulkanRenderer() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

int VulkanRenderer::Init(const std::string& name, const int width, const int height) {
	glfwInit();

	//Set GLFW to not work with opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

	try {
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateGraphicsPipiline();
	}
	catch (const std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	return 0;
}

void VulkanRenderer::Clean() {
	for (auto image : swapchain_images) {
		vkDestroyImageView(devices.logical_device, image.image_view, nullptr);
	}

	vkDestroySwapchainKHR(devices.logical_device, swapchain, nullptr);
	vkDestroySurfaceKHR(vk_instance, surface, nullptr);
	if (enable_validation_layers) {
		DestroyDebugUtilsMessengerEXT(vk_instance, debug_messenger, nullptr);
	}
	vkDestroyDevice(devices.logical_device, nullptr);
	vkDestroyInstance(vk_instance, nullptr);
}

void VulkanRenderer::Update() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void VulkanRenderer::CreateInstance() {
	if (enable_validation_layers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("Validation layers requested, but not available");
	}

	//Application info - not vulkan instance
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Application";			//Custom name of the app
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		//Custom version of the app
	app_info.pEngineName = "No engine";							//Custom engine name
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);			//Custom engine version
	app_info.apiVersion = VK_API_VERSION_1_2;					//Vulkan version

	//Vulkan instance info
	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;

	//Set up extensions that'll used by the instance
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	//Check "Instance extensions" supported
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (!CheckInstanceExtensionSupport(extensions)) {
		throw std::runtime_error("VKInstance does not support required extensions");
	}

	instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
	if (enable_validation_layers) {
		instance_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
		instance_info.ppEnabledLayerNames = required_validation_layers.data();

		PopulateDebugMessengerCreateInfo(debug_create_info);
		instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
	}
	else {
		instance_info.enabledLayerCount = 0;
		instance_info.pNext = nullptr;
	}
	

	//Create instance
	if (vkCreateInstance(&instance_info, nullptr, &vk_instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a vulkan instance");
}

bool VulkanRenderer::CheckInstanceExtensionSupport(const std::vector<const char*>& check_extensions)
{
	//Get the numner of extensions first - the size of the list is unknown at this point (third parameter)
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	//Now create the list
	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

	//Check if given extensions are in the list of available extensions
	for (auto& check_extension : check_extensions) {
		bool has_extension = false;
		for (auto& extension : extensions) {
			if (strcmp(check_extension, extension.extensionName) == 0) {
				has_extension = true;
				break;
			}
		}

		if (!has_extension)
			return false;
	}

	return true;
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* requiring_layer_name : required_validation_layers) {
		bool layer_found = false;
		for (const auto& layer_properties : available_layers) {
			if (strcmp(requiring_layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

void VulkanRenderer::SetupDebugMessenger() {
	if (!enable_validation_layers) return;
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};

	PopulateDebugMessengerCreateInfo(create_info);

	if (CreateDebugUtilsMessengerEXT(vk_instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger");
	}
}

void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity =
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
	create_info.pUserData = nullptr;
}

void VulkanRenderer::GetPhysicalDevice() {
	//Enumerate physical devices that vk instance can access
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);

	if (device_count == 0)
		throw std::runtime_error("Can't find GPUs that support Vulkan instance");

	//Get the list of physical device
	std::vector<VkPhysicalDevice> device_list(device_count);
	vkEnumeratePhysicalDevices(vk_instance, &device_count, device_list.data());

	for (auto& device : device_list) {
		if (CheckDeviceSuitable(device)) {
			devices.physical_device = device;
			break;
		}
	}
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
	/*
	
	//Information about the device itself (id, name, type, vendor, ...)
	
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);


	//Information about what the device can do (geometry shader, tess shader, ...)

	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);

	*/

	bool extension_support = CheckDeviceExtensionSupport(device);
	bool queue_families_complete = GetQueueFamilies(device).IsComplete();

	bool swapchain_valid = false;
	if (extension_support) {
		SwapChainDetails swapchain_details = GetSwapChainDetails(device);
		swapchain_valid = !swapchain_details.formats.empty() && !swapchain_details.presentation_modes.empty();
	}

	return queue_families_complete && extension_support && swapchain_valid;
}

QueueFamilyIndices VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	
	//Get all queue family info for the given device
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_family_list(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_list.data());

	//Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0;
	for (auto& queue_family : queue_family_list) {
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphics_family = i;

		//Check if queue family supports presentation
		VkBool32 presentation_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support);
		if (queue_family.queueCount > 0 && presentation_support) {
			indices.presentation_family = i;
		}

		if (indices.IsComplete())
			break;

		++i;
	}

	return indices;
}

void VulkanRenderer::CreateLogicalDevice()
{
	//Get the queue family indices for the chosen physical device
	QueueFamilyIndices indices = GetQueueFamilies(devices.physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.presentation_family.value() };

	float priority = 1.f; // the highest value
	for (uint32_t queue_family : unique_queue_families) {
		//Queue informations for creating logical device
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &priority;
		queue_create_infos.push_back(queue_create_info);
	}

	//Information to create logical device
	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_info.pQueueCreateInfos = queue_create_infos.data();
	device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	device_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers) {
		device_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
		device_info.ppEnabledLayerNames = required_validation_layers.data();
	}
	else {
		device_info.enabledLayerCount = 0;
		device_info.ppEnabledLayerNames = nullptr;
	}

	//Physical device features that the logical device will be using
	VkPhysicalDeviceFeatures device_features = {};

	device_info.pEnabledFeatures = &device_features;

	VkResult result = vkCreateDevice(devices.physical_device, &device_info, nullptr, &devices.logical_device);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create a logical device");

	//Queues are created ar the same time as the device - get the handle of that
	vkGetDeviceQueue(devices.logical_device, indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(devices.logical_device, indices.presentation_family.value(), 0, &presentation_queue);
}

void VulkanRenderer::CreateSurface() {
	if (glfwCreateWindowSurface(vk_instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a surface");
	}
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	if (extension_count == 0) {
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

	for (const auto& device_extension : device_extensions) {
		bool has_extension = false;
		for (const auto& extension : extensions) {
			if (strcmp(device_extension, extension.extensionName) == 0) {
				has_extension = true;
				break;
			}
		}

		if (!has_extension) {
			return false;
		}
	}

	return true;
}

SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapchain_details;

	//Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchain_details.surface_capabilities);

	//Formats
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0) {
		swapchain_details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, swapchain_details.formats.data());
	}

	//Presentation mode
	uint32_t presentation_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_count, nullptr);

	if (presentation_count != 0) {
		swapchain_details.presentation_modes.resize(presentation_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_count, swapchain_details.presentation_modes.data());
	}

	return swapchain_details;
}

void VulkanRenderer::CreateSwapChain() {
	SwapChainDetails swapchain_details = GetSwapChainDetails(devices.physical_device);

	//Find optimal surface values for our swap chain
	VkSurfaceFormatKHR surface_format = ChooseBestSurfaceFormat(swapchain_details.formats);
	VkPresentModeKHR present_mode = ChooseBestPresentationMode(swapchain_details.presentation_modes);
	VkExtent2D extent = ChooseSwapExtent(swapchain_details.surface_capabilities);

	//How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
	uint32_t image_count = swapchain_details.surface_capabilities.minImageCount + 1;

	//clamp
	if (swapchain_details.surface_capabilities.maxImageCount > 0 && 
		swapchain_details.surface_capabilities.maxImageCount < image_count) {
		image_count = swapchain_details.surface_capabilities.maxImageCount;
	}

	//Creation information for swap chain
	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = surface;
	swapchain_create_info.imageFormat = surface_format.format;
	swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.imageExtent = extent;
	swapchain_create_info.minImageCount = image_count;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.preTransform = swapchain_details.surface_capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.clipped = VK_TRUE;

	QueueFamilyIndices indices = GetQueueFamilies(devices.physical_device);
	//If graphics & presentation families are different then swapchain must let image be shared between families
	if (indices.graphics_family != indices.presentation_family) {
		uint32_t queueFamilyIndices[] = {
			indices.graphics_family.value(),
			indices.presentation_family.value()
		};
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//no need to do specify these but just for clarity
		swapchain_create_info.queueFamilyIndexCount = 0;
		swapchain_create_info.pQueueFamilyIndices = nullptr;
	}

	//If old swapchain has been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

	//Create Swapchain
	VkResult result = vkCreateSwapchainKHR(devices.logical_device, &swapchain_create_info, nullptr, &swapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}

	//Store for later reference
	swapchain_image_format = surface_format.format;
	swapchain_extent = extent;

	//Get swapchain images (count first then values)
	uint32_t swapchain_image_count = 0;
	vkGetSwapchainImagesKHR(devices.logical_device, swapchain, &swapchain_image_count, nullptr);
	std::vector<VkImage> images(swapchain_image_count);
	vkGetSwapchainImagesKHR(devices.logical_device, swapchain, &swapchain_image_count, images.data());

	for (VkImage image : images) {
		//Store image handle
		SwapchainImage swapchain_image;
		swapchain_image.image = image;

		//Create image view
		swapchain_image.image_view = CreateImageView(image, swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);

		//Add swapchain image to the vector
		swapchain_images.push_back(swapchain_image);
	}
}

//Best format are subjective but will choose:
//format : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as a backup)
//color space : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	//All formats are available
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats) {
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentation_modes) {
	for (const auto& presentation_mode : presentation_modes) {
		if (presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentation_mode;
		}
	}

	//Back up: This is always available according to Vulkan spec
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities) {
	if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return surface_capabilities.currentExtent;
	}
	else {
		//window size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D new_extent{};
		new_extent.width = static_cast<uint32_t>(width);
		new_extent.height = static_cast<uint32_t>(height);

		//Make sure within boundaries by clamping value
		new_extent.width = std::max(surface_capabilities.minImageExtent.width, 
			std::min(surface_capabilities.maxImageExtent.width, new_extent.width));
		new_extent.height = std::max(surface_capabilities.minImageExtent.height,
			std::min(surface_capabilities.maxImageExtent.height, new_extent.height));

		return new_extent;
	}
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo view_create_info;
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = format;
	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;		//Allows remapping of rgba components to other rgba value
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	//Subresources allow the view to view only a part of an image
	view_create_info.subresourceRange.aspectMask = aspect_flags;		//which aspect of image to view (e.g. COLOR_BIT for viewing color)
	view_create_info.subresourceRange.baseMipLevel = 0;					//Start mipmap level to view from
	view_create_info.subresourceRange.levelCount = 1;					//Number of mipmap levels to view
	view_create_info.subresourceRange.baseArrayLayer = 0;				//Start array level to view from
	view_create_info.subresourceRange.layerCount = 1;					//Number of array levels to view

	//Create image view and return it
	VkImageView image_view;
	VkResult result = vkCreateImageView(devices.logical_device, &view_create_info, nullptr, &image_view);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create an image view");
	}

	return image_view;
}

void VulkanRenderer::CreateGraphicsPipiline() {
	auto vertex_shader_code = ReadFile("Shaders/vert.spv");
	auto fragment_shader_code = ReadFile("Shaders/frag.spv");

	//Build shader modules to link to graphics pipeline
	VkShaderModule vertex_shader_module = CreateShaderModule(vertex_shader_code);
	VkShaderModule fragment_shader_module = CreateShaderModule(fragment_shader_code);

	//Shader stage creation information
	//Vertex stage creation information
	VkPipelineShaderStageCreateInfo vertex_shader_create_info;
	vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;				//Shader stage name
	vertex_shader_create_info.module = vertex_shader_module;					//shader module to be used by stage
	vertex_shader_create_info.pName = "main";									//Entry point in to shader

	//Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragment_shader_create_info;
	fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;				//Shader stage name
	fragment_shader_create_info.module = fragment_shader_module;					//shader module to be used by stage
	fragment_shader_create_info.pName = "main";										//Entry point in to shader

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_create_info, fragment_shader_create_info };

	//Create pipeline
	// -- Vertex input -- 
	VkPipelineVertexInputStateCreateInfo vertex_input_create_info;
	vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_create_info.pVertexBindingDescriptions = nullptr;
	vertex_input_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

	// -- Input Assembly -- 
	VkPipelineInputAssemblyStateCreateInfo input_assembly;
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;					// like GL_TRIANGLE / GL_LINE / ...
	input_assembly.primitiveRestartEnable = VK_FALSE;								// Allow overriding of "strip" topology to start new primitives

	// -- Viewport & Scissor --
	//Create a viewport info struct
	VkViewport viewport;
	viewport.x = 0.f;																//X start coordinate
	viewport.y = 0.f;																//Y start coordinate
	viewport.width = (float)swapchain_extent.width;									//Width of viewport
	viewport.height = (float)swapchain_extent.height;								//Height of viewport
	viewport.minDepth = 0.f;														//Min framebuffer depth
	viewport.maxDepth = 1.f;														//Max framebuffer depth

	//Create a scissor info struct
	VkRect2D scissor;
	scissor.offset = { 0, 0 };														//Offset to use region from
	scissor.extent = swapchain_extent;												//Extent to decribe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewport_state_create_info;
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	//// -- Dynamic State --
	////Dynamic states to enable
	//std::vector<VkDynamicState> dynamic_state_enables;
	//dynamic_state_enables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // dynamic viewport : can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	//dynamic_state_enables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// dynamic scissors : can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	////Dynamic state creation info
	//VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
	//dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());
	//dynamic_state_create_info.pDynamicStates = dynamic_state_enables.data();

	// -- Rasterizer -- 
	VkPipelineRasterizationStateCreateInfo rasterization_create_info;
	rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_create_info.depthClampEnable = VK_FALSE;							//Change if fragment beyond near / far plane are clipped (default) or clamped to plane -> NEED GPU FEATURE
	rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;					//Whether to discard data and skip rasterization - only suitable for pipeline without framebuffer output
	rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;					//Wireframe mode / ... -> NEED GPU FEATURE
	rasterization_create_info.lineWidth = 1.f;										//Thickness of line
	rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;						//Which face of a triangle to cull
	rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;					//Winding to determine which side is front

	//Destroy shader modules, no longer needed after pipeline created
	vkDestroyShaderModule(devices.logical_device, vertex_shader_module, nullptr);
	vkDestroyShaderModule(devices.logical_device, fragment_shader_module, nullptr);
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo shader_module_create_info;
	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize = code.size();
	shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(devices.logical_device, &shader_module_create_info, nullptr, &shader_module);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	return shader_module;
}
