#include <iostream>
#include "VulkanRenderer.h"

int main() {
	VulkanRenderer vk_renderer;
	if (vk_renderer.Init() == EXIT_FAILURE)
		return EXIT_FAILURE;

	vk_renderer.Update();

	vk_renderer.Clean();
	return 0;
}
