﻿#include <iostream>
#include <assert.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(call) { VkResult result = call; assert(result == VK_SUCCESS); }

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice physicalDevices[], uint32_t physicalDeviceCount) {
	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			printf("Using discrete GPU %s\n", properties.deviceName);
			return physicalDevices[i];
		}
	}

	if (physicalDeviceCount > 0) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);
		printf("Using fallback GPU %s\n", properties.deviceName);
		return physicalDevices[0];
	}

	printf("No suitable physical device found\n");
	return 0;
}

VkInstance createInstance() {
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
	const char* debugLayers[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	createInfo.ppEnabledLayerNames = debugLayers;
	createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);
#endif

	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
	};
	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);

	VkInstance instance = 0;

	VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));
	return instance;
}

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window) {

#ifdef VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hinstance = GetModuleHandle(0);
	createInfo.hwnd = glfwGetWin32Window(window);

	VkSurfaceKHR surface;
	VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
	return surface;
#else
#error Unsupported platform
#endif
}

VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t* familyIndex) {
	*familyIndex = 0;

	float queuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	deviceQueueCreateInfo.queueFamilyIndex = *familyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	const char* extensions[] = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.ppEnabledExtensionNames = extensions;
	deviceCreateInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);

	VkDevice device = 0;
	VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, 0, &device));
	return device;
}

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, uint32_t windowWidth, uint32_t windowHeight, uint32_t familyIndex) {
	VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = 2;
	swapchainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent.width = windowWidth;
	swapchainCreateInfo.imageExtent.height = windowHeight;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.queueFamilyIndexCount = 1;
	swapchainCreateInfo.pQueueFamilyIndices = &familyIndex;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkSwapchainKHR swapchain = 0;
	VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, 0, &swapchain));
	return swapchain;
}

VkSemaphore createSemaphore(VkDevice device) {
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore = 0;
	VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

	return semaphore;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex) {
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = familyIndex;

	VkCommandPool commandPool = 0;
	VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));
	return commandPool;
}

int main()
{
	int rc = glfwInit();
	assert(rc);

	VkInstance instance = createInstance();
	assert(instance);

	VkPhysicalDevice physicalDevices[16];
	uint32_t physicalDeviceCount = sizeof(physicalDevices)/ sizeof(physicalDevices[0]);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
	assert(physicalDevice);

	uint32_t familyIndex = 0;
	VkDevice device = createDevice(instance, physicalDevice, &familyIndex);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Hello Vulkan", 0, 0);
	assert(window);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

	int windowWidth = 0, windowHeight = 0;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	VkSwapchainKHR swapchain = createSwapchain(device, surface, windowWidth, windowHeight, familyIndex);
	assert(swapchain);

	VkSemaphore acquireSemaphore = createSemaphore(device);
	assert(acquireSemaphore);

	VkSemaphore releaseSemaphore = createSemaphore(device);
	assert(releaseSemaphore);

	VkQueue queue = 0;
	vkGetDeviceQueue(device, familyIndex, 0, &queue);

	VkImage swapchainImages[16];
	uint32_t swapchainImageCount = sizeof(swapchainImages) / sizeof(swapchainImages[0]);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

	VkCommandPool commandPool = createCommandPool(device, familyIndex);
	assert(commandPool);

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer = 0;
	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(device, swapchain, ~0ull, acquireSemaphore, VK_NULL_HANDLE, &imageIndex);

		VK_CHECK(vkResetCommandPool(device, commandPool, 0));

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };

		VkImageSubresourceRange range = { };
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.layerCount = 1;
		range.levelCount = 1;

		vkCmdClearColorImage(commandBuffer, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pWaitSemaphores = &acquireSemaphore;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = &submitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &releaseSemaphore;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pWaitSemaphores = &releaseSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &imageIndex;

		VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

		VK_CHECK(vkDeviceWaitIdle(device));
	}

	glfwDestroyWindow(window);

	vkDestroyInstance(instance, 0);
}
