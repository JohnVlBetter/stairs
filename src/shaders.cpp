#include "common.h"
#include "shaders.h"

#include <stdio.h>

#include <spirv_cross/spirv.h>

static void parseShader(Shader& shader, const uint32_t* code, uint32_t codeSize) {
	assert(code[0] == SpvMagicNumber);
	

}

bool loadShader(Shader& shader, VkDevice device, const char* path)
{
	FILE* file = fopen(path, "rb"); 
	if (!file) return false;

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	assert(length >= 0);
	fseek(file, 0, SEEK_SET);

	char* buffer = new char[length];
	assert(buffer);

	size_t rc = fread(buffer, 1, length, file);
	assert(rc == size_t(length));
	fclose(file);

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = length;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);

	VkShaderModule shaderModule = 0;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));

	assert(length % 4 == 0);
	parseShader(shader, reinterpret_cast<const uint32_t*>(buffer), length / 4);

	delete[] buffer;

	shader.module = shaderModule;

	return shaderModule;
}

void destroyShader(Shader& shader, VkDevice device) {
	vkDestroyShaderModule(device, shader.module, 0);
}

VkDescriptorSetLayout createSetLayout(VkDevice device, bool rtxEnabled) {
	std::vector<VkDescriptorSetLayoutBinding> setBinding = {};
	if (rtxEnabled) {
		setBinding.resize(2);
		setBinding[0].binding = 0;
		setBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setBinding[0].descriptorCount = 1;
		setBinding[0].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
		setBinding[1].binding = 1;
		setBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setBinding[1].descriptorCount = 1;
		setBinding[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;
	}
	else {
		setBinding.resize(1);
		setBinding[0].binding = 0;
		setBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setBinding[0].descriptorCount = 1;
		setBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkDescriptorSetLayoutCreateInfo setCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	setCreateInfo.bindingCount = uint32_t(setBinding.size());
	setCreateInfo.pBindings = setBinding.data();

	VkDescriptorSetLayout setLayout = 0;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &setCreateInfo, 0, &setLayout));

	return setLayout;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout)
{
	VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &setLayout;

	VkPipelineLayout layout = 0;
	VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

	return layout;
}

VkDescriptorUpdateTemplate createUpdateTemplate(VkDevice device, VkPipelineBindPoint bindPoint, VkDescriptorSetLayout setLayout, VkPipelineLayout layout, bool rtxEnabled) {
	std::vector<VkDescriptorUpdateTemplateEntry> entries;

	if (rtxEnabled) {
		entries.resize(2);
		entries[0].dstBinding = 0;
		entries[0].dstArrayElement = 0;
		entries[0].descriptorCount = 1;
		entries[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		entries[0].offset = sizeof(DescriptorInfo) * 0;
		entries[0].stride = sizeof(DescriptorInfo);
		entries[1].dstBinding = 1;
		entries[1].dstArrayElement = 0;
		entries[1].descriptorCount = 1;
		entries[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		entries[1].offset = sizeof(DescriptorInfo) * 1;
		entries[1].stride = sizeof(DescriptorInfo);
	}
	else {
		entries.resize(1);
		entries[0].dstBinding = 0;
		entries[0].dstArrayElement = 0;
		entries[0].descriptorCount = 1;
		entries[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		entries[0].offset = sizeof(DescriptorInfo) * 0;
		entries[0].stride = sizeof(DescriptorInfo);
	}

	VkDescriptorUpdateTemplateCreateInfo updateCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };

	updateCreateInfo.descriptorUpdateEntryCount = uint32_t(entries.size());
	updateCreateInfo.pDescriptorUpdateEntries = entries.data();

	updateCreateInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
	updateCreateInfo.pipelineBindPoint = bindPoint;
	updateCreateInfo.pipelineLayout = layout;
	updateCreateInfo.descriptorSetLayout = setLayout;

	VkDescriptorUpdateTemplate updateTemplate = 0;
	VK_CHECK(vkCreateDescriptorUpdateTemplate(device, &updateCreateInfo, 0, &updateTemplate));

	return updateTemplate;
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkRenderPass renderPass, const Shader& vs, const Shader& fs,
	VkPipelineLayout layout, bool rtxEnabled)
{
	VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	if (rtxEnabled) {
		stages[0].stage = VK_SHADER_STAGE_MESH_BIT_NV;
	}
	else {
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	}

	stages[0].module = vs.module;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fs.module;
	stages[1].pName = "main";

	createInfo.stageCount = sizeof(stages) / sizeof(stages[0]);
	createInfo.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	createInfo.pVertexInputState = &vertexInput;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.pInputAssemblyState = &inputAssembly;

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	createInfo.pViewportState = &viewportState;

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.lineWidth = 1.f;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	createInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	createInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState colorAttachmentState = {};
	colorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorAttachmentState;
	createInfo.pColorBlendState = &colorBlendState;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
	dynamicState.pDynamicStates = dynamicStates;
	createInfo.pDynamicState = &dynamicState;

	createInfo.layout = layout;
	createInfo.renderPass = renderPass;

	VkPipeline pipeline = 0;
	VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

	return pipeline;
}
