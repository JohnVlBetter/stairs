#pragma once

struct Shader {
	VkShaderModule module;
	VkShaderStageFlagBits stage;
};

bool loadShader(Shader& shader, VkDevice device, const char* path);
void destroyShader(Shader& shader, VkDevice device);
VkDescriptorSetLayout createSetLayout(VkDevice device, bool rtxEnabled);
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout);
VkDescriptorUpdateTemplate createUpdateTemplate(VkDevice device, VkPipelineBindPoint bindPoint, VkDescriptorSetLayout setLayout, VkPipelineLayout layout, bool rtxEnabled);
VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkRenderPass renderPass, const Shader& vs, const Shader& fs, 
	VkPipelineLayout layout, bool rtxEnabled);

struct DescriptorInfo {
	union {
		VkDescriptorImageInfo image;
		VkDescriptorBufferInfo buffer;
	};

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