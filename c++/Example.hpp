#pragma once

#include <vulkan/vulkan.h> // Vulkan
#include <optional> // optional
#include <string.h> // strcmp
#include <assert.h> // assert
#include <cmath> // ceil
#include <variant> // std::variant
#include <array> // std::array
#include <numeric> // std::accumulate

#include <iostream>
#include <tuple>

//#include <iostream>

#ifdef NDEBUG
const auto enableValidationLayers = std::nullopt;
#else
const auto enableValidationLayers = std::optional<char const*>{"VK_LAYER_KHRONOS_validation"};
#endif

// Macro to check Vulkan result
#define VK_CHECK_RESULT(f) 																\
{																						\
    VkResult res = (f);																	\
    if (res != VK_SUCCESS)																\
    {																					\
        printf("Fatal : VkResult is %d in %s at line %d\n", res,  __FILE__, __LINE__);  \
        assert(res == VK_SUCCESS);														\
    }																					\
}

namespace Utility {
    // Creates Vulkan instance
    void createInstance(VkInstance& instance);
    // Gets physical device
    void getPhysicalDevice(VkInstance const& instance, VkPhysicalDevice& physicalDevice);
    // Gets an index to a queue family
     uint32_t getComputeQueueFamilyIndex(VkPhysicalDevice const& physicalDevice);
    // Creates logical device
    void createDevice(
        VkPhysicalDevice const& physicalDevice,
        uint32_t& queueFamilyIndex,
        VkDevice& device,
        VkQueue& queue
    );
    // Finds the memory type by which we can access memory allocated from the heap
     uint32_t findMemoryType(
        VkPhysicalDevice const& physicalDevice,
        uint32_t const memoryTypeBits,
        VkMemoryPropertyFlags const properties
    );
    // Creates buffers
    void createBuffers(
        VkPhysicalDevice const& physicalDevice,
        VkDevice const& device,
        uint32_t const numBuffers,
        uint32_t const* bufferSize,
        VkBuffer*& buffer,
        VkDeviceMemory*& bufferMemory
    );
    // Creates buffer
    void createBuffer(
        VkPhysicalDevice const& physicalDevice,
        VkDevice const& device,
        uint32_t const size,
        VkBuffer * const buffer,
        VkDeviceMemory * const bufferMemory
    );
    // Fills a buffer with given data
    void fillBuffer(
        VkDevice const& device,
        VkDeviceMemory& bufferMemory,
        float*& bufferData, 
        uint32_t const bufferSize
    );
    // Creates descriptor set layout
    void createDescriptorSetLayout(
        VkDevice const& device, 
        VkDescriptorSetLayout* descriptorSetLayout,
        uint32_t const numBuffers
    );
    // Creates descriptor set
    void createDescriptorSet(
        VkDevice const& device,
        VkDescriptorPool* descriptorPool,
        VkDescriptorSetLayout* descriptorSetLayout,
        uint32_t const numBuffers,
        VkBuffer*& buffer,
        VkDescriptorSet& descriptorSet
    );
    // Reads shader file
    std::pair<uint32_t,uint32_t*> readShader(char const* filename);

    // Creates compute pipeline
    template <uint32_t NumPushConstants>
    std::optional<uint32_t> createComputePipeline(
        VkDevice const& device,
        char const* shaderFile,
        VkShaderModule* computeShaderModule,
        VkDescriptorSetLayout* descriptorSetLayout,
        VkPipelineLayout* pipelineLayout,
        VkPipeline* pipeline,
        std::array<std::variant<uint32_t,float>,NumPushConstants>& pushConstants
    ) {
        // Creates shader module (just a wrapper around our shader)
        // std::pair<uint32_t,uint32_t*> file = readShader(shaderFile); // (length,bytes)
        // VkShaderModuleCreateInfo createInfo = {
        //     .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        //     .codeSize = file.first,
        //     .pCode = file.second
        // };
        auto [fileLength, fileBytes] = readShader(shaderFile); // (length,bytes)
        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fileLength,
            .pCode = fileBytes
        };

        VK_CHECK_RESULT(vkCreateShaderModule(
            device, &createInfo, nullptr, computeShaderModule
        ));

        // A compute pipeline is very simple compared to a graphics pipeline.
        // It only consists of a single stage with a compute shader.

        // The pipeline layout allows the pipeline to access descriptor sets. 
        // So we just specify the descriptor set layout we created earlier.
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1, // 1 descriptor set
            .pSetLayouts = descriptorSetLayout // the 1 descriptor set 
        };

        // Sets push constants
        std::optional<uint32_t> pushConstantSize = std::nullopt;
        if(pushConstants.size() != 0) {
            auto size_fn = [](auto variant) -> uint32_t {
                using T = std::decay_t<decltype(variant)>;
                if constexpr (std::is_same_v<uint32_t, T>) { return sizeof(uint32_t); }
                if constexpr (std::is_same_v<float, T>) { return sizeof(float);}
            };
            pushConstantSize = std::accumulate(pushConstants.begin(),pushConstants.end(),0,
                [size_fn](uint32_t acc, auto variant) { return std::move(acc) + std::visit(size_fn,variant); }
            );

            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = new VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = pushConstantSize.value()
            };
        }
        
        VK_CHECK_RESULT(vkCreatePipelineLayout(
            device, &pipelineLayoutCreateInfo, nullptr, pipelineLayout
        ));

        // We specify the compute shader stage, and it's entry point(main).
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT, // Shader type
            .module = *computeShaderModule, // Shader module
            .pName = "main" // Shader entry point
        };

        // Set our pipeline options
        VkComputePipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCreateInfo,
            .layout = *pipelineLayout
        };

        // Create compute pipeline
        VK_CHECK_RESULT(vkCreateComputePipelines(
            device, VK_NULL_HANDLE,
            1, &pipelineCreateInfo,
            nullptr, pipeline
        ));

        return pushConstantSize;
    }
    
    // Creates command buffer
    template <uint32_t NumPushConstants>
    void createCommandBuffer(
        uint32_t queueFamilyIndex,
        VkDevice& device,
        VkCommandPool* commandPool,
        VkCommandBuffer* commandBuffer,
        VkPipeline& pipeline,
        VkPipelineLayout& pipelineLayout,
        VkDescriptorSet& descriptorSet,
        uint32_t const* dims, // [x,y,z],
        uint32_t const* dimLengths, // [local_size_x, local_size_y, local_size_z]
        std::optional<uint32_t> const pushConstantSize,
        std::array<std::variant<uint32_t,float>,NumPushConstants>& pushConstants
    ) {
        // Creates command pool
        VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndex // Sets queue family
        };
        VK_CHECK_RESULT(vkCreateCommandPool(
            device, &commandPoolCreateInfo, nullptr, commandPool
        ));

        // Allocates command buffer
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = *commandPool,  // Pool to allocate from
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1  // Allocates 1 command buffer. 
        };
        VK_CHECK_RESULT(vkAllocateCommandBuffers(
            device, &commandBufferAllocateInfo, commandBuffer
        ));

        // Allocated command buffer options
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            // Buffer only submitted once
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        // Start recording commands
        VK_CHECK_RESULT(vkBeginCommandBuffer(*commandBuffer, &beginInfo));

        // Binds pipeline (our functions)
        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        // Binds descriptor set (our data)
        vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        // Sets push constants
        if(pushConstantSize.has_value()) {
            vkCmdPushConstants(
                *commandBuffer, 
                pipelineLayout, 
                VK_SHADER_STAGE_COMPUTE_BIT, 
                0, 
                pushConstantSize.value(), 
                pushConstants.data()
            );
        }

        auto const [x,y,z] = std::make_tuple(
            ceil(dims[0] / static_cast<float>(dimLengths[0])),
            ceil(dims[1] / static_cast<float>(dimLengths[1])),
            ceil(dims[2] / static_cast<float>(dimLengths[2]))
        );

        // Sets invocations
        vkCmdDispatch(
            *commandBuffer,
            x,
            y,
            z
        );

        // End recording commands
        VK_CHECK_RESULT(vkEndCommandBuffer(*commandBuffer));
    }

    // Runs command buffer
    void runCommandBuffer(
        VkCommandBuffer* commandBuffer,
        VkDevice const& device,
        VkQueue const& queue
    );

    void* map(VkDevice& device, VkDeviceMemory& bufferMemory);
}

template <uint32_t NumPushConstants>
class ComputeApp {
    // -------------------------------------------------
    // Private members
    // -------------------------------------------------
    public:
        VkInstance instance;                        // Vulkan instance.
        VkPhysicalDevice physicalDevice;            // Physical device (e.g. GPU).
        VkDevice device;                            // Logical device by which we connect to our physical device.
        uint32_t queueFamilyIndex;                  // Index to a queue family.
        VkQueue queue;                              // Queue.
        uint32_t numBuffers;                        // Number of buffers (necessary for destruction).
        VkBuffer* buffer;                           // Buffers.
        VkDeviceMemory* bufferMemory;               // Buffer memories.
        VkDescriptorSetLayout descriptorSetLayout;  // Layout of a descriptor set.
        VkDescriptorPool descriptorPool;            // Pool from which to pull descriptor sets.
        VkDescriptorSet descriptorSet;              // Descriptor set.
        VkShaderModule computeShaderModule;         // Shader.
        VkPipelineLayout pipelineLayout;            // Layout for a pipeline.
        VkPipeline pipeline;                        // Pipeline.
        VkCommandPool commandPool;                  // Pool from which to pull command buffer.
        VkCommandBuffer commandBuffer;              // Command buffer.
    // -------------------------------------------------
    // Public methods
    // -------------------------------------------------
    public:
        ComputeApp(
            char const* shaderFile,
            uint32_t const numBuffers,
            uint32_t const* bufferSize,
            float** bufferData,
            std::array<std::variant<uint32_t,float>, NumPushConstants> pushConstant,
            uint32_t* dims, // [x,y,z],
            uint32_t const* dimLengths // [local_size_x, local_size_y, local_size_z]
        )  {
            this->numBuffers = numBuffers;

            // Initialize vulkan:
            Utility::createInstance(this->instance);

            // Gets physical device
            Utility::getPhysicalDevice(this->instance, this->physicalDevice);

            // Gets logical device
            Utility::createDevice(this->physicalDevice, this->queueFamilyIndex, this->device, this->queue);

            // Creates buffers
            Utility::createBuffers(this->physicalDevice, this->device, numBuffers, bufferSize, this->buffer, this->bufferMemory);

            for(uint32_t i=0;i<numBuffers;++i) {
                // Fills buffer
                Utility::fillBuffer(this->device, this->bufferMemory[i], bufferData[i], bufferSize[i]);
            }

            // Creates descriptor set layout
            Utility::createDescriptorSetLayout(this->device,&this->descriptorSetLayout,numBuffers);

            // Creates descriptor set
            Utility::createDescriptorSet(this->device,&this->descriptorPool,&this->descriptorSetLayout,numBuffers,buffer,this->descriptorSet);

            // Creates compute pipeline
            std::optional<uint32_t> const pushConstantSize = 
            Utility::createComputePipeline<NumPushConstants>(
                this->device,
                shaderFile,
                &this->computeShaderModule,
                &this->descriptorSetLayout,
                &this->pipelineLayout,
                &this->pipeline,
                pushConstant
            );

            // Creates command buffer
            Utility::createCommandBuffer<NumPushConstants>(
                this->queueFamilyIndex,
                this->device,
                &this->commandPool,
                &this->commandBuffer,
                this->pipeline,
                this->pipelineLayout,
                this->descriptorSet,
                dims,
                dimLengths,
                pushConstantSize,
                pushConstant
            );

            Utility::runCommandBuffer(
                &this->commandBuffer,
                this->device,
                this->queue
            );
        }
        ~ComputeApp()  {
            for(uint32_t i=0;i<numBuffers;++i) {
                vkFreeMemory(device, bufferMemory[i], nullptr);
                vkDestroyBuffer(device, buffer[i], nullptr);
            }

            vkDestroyShaderModule(device, computeShaderModule, nullptr);
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
            vkDestroyCommandPool(device, commandPool, nullptr);

            vkDestroyDevice(device, nullptr);
            vkDestroyInstance(instance, nullptr);		
        }
};