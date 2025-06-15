#pragma once

struct Shader {
	VkShaderModule module;
	VkShaderStageFlagBits stage;
	uint32_t storageBufferMask;
};

bool loadShader(Shader& shader, VkDevice device, const char* path);

using Shaders = std::initializer_list<const Shader*>;

void destroyShader(Shader& shader, VkDevice device);
VkDescriptorSetLayout createSetLayout(VkDevice device, Shaders shaders);
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout);
VkDescriptorUpdateTemplate createUpdateTemplate(VkDevice device, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, Shaders shaders);
VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkRenderPass renderPass, Shaders shaders, VkPipelineLayout layout);

struct DescriptorInfo {
	union {
		VkDescriptorImageInfo image;
		VkDescriptorBufferInfo buffer;
	};

	DescriptorInfo() {
		
	}

	DescriptorInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout) {
		this->image.sampler = sampler;
		this->image.imageView = imageView;
		this->image.imageLayout = imageLayout;
	}

	DescriptorInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
		this->buffer.buffer = buffer;
		this->buffer.offset = offset;
		this->buffer.range = range;
	}

	DescriptorInfo(VkBuffer buffer) {
		this->buffer.buffer = buffer;
		this->buffer.offset = 0;
		this->buffer.range = VK_WHOLE_SIZE;
	}
};