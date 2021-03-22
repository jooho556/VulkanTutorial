#pragma once
#include <optional>
#include <fstream>

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family; //Location of graphics queue family
	std::optional<uint32_t> presentation_family; //Location of presentation queue family

	bool IsComplete() {
		return graphics_family.has_value() && presentation_family.has_value();
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surface_capabilities; // Surface properties (iamge size / extent)
	std::vector<VkSurfaceFormatKHR> formats; //Surface image formats (RGBA / size of each color)
	std::vector<VkPresentModeKHR> presentation_modes; //How image should be presented to screen
};

struct SwapchainImage {
	VkImage image;
	VkImageView image_view;
};

static std::vector<char> ReadFile(const std::string& filename) {
	//std::ios::ate tells stream to start reading from end of file
	std::ifstream infile(filename, std::ios::binary | std::ios::ate);
	if (!infile.is_open()) {
		throw std::runtime_error("Failed to open a file : " + filename);
	}

	//Get current read position
	size_t file_size = (size_t)infile.tellg();
	std::vector<char> file_buffer(file_size);

	infile.seekg(0);
	infile.read(file_buffer.data(), file_size);
	infile.close();

	return file_buffer;
}