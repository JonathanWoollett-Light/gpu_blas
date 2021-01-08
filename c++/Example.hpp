#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP
#endif

#include <vulkan/vulkan.h> // Vulkan
#include <vector> // vector
#include <iostream> // cout
#include <string.h> // strcmp
#include <algorithm> // find_if
#include <optional> // optional
#include <assert.h> // assert
#include <cmath> // ceil

// If debug, enable validation layers
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

class ComputeApp {
    // --------------------------------------------------
    // Public members
    // --------------------------------------------------
    public:
        VkDevice device;
        VkDeviceMemory* bufferMemories;
    // --------------------------------------------------
    // Private members
    // --------------------------------------------------
    private:
        // In order of usage
        std::vector<char const*> enabledLayers;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        uint32_t queueFamilyIndex;
        VkQueue queue;

        VkBuffer* buffers;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;

        VkDescriptorSet descriptorSet;

        VkShaderModule computeShaderModule;

        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        uint32_t numBuffers; // necessary for destruction
    // --------------------------------------------------
    // Public methods
    // --------------------------------------------------
    public:
        static float* map(VkDevice& device, VkDeviceMemory& bufferMemory);
        static void print(VkDevice& device,VkDeviceMemory& bufferMemory, uint32_t size);

        ComputeApp(
            char* shaderFile,
            uint32_t const * bufferSizes,
            uint32_t numBuffers,
            float**& bufferData,
            float* pushConstants,
            uint32_t numPushConstants,
            int const* dims, // [x,y,z],
            int const* dimLengths, // [local_size_x, local_size_y, local_size_z]
            bool requiresAtomic
        );
        ~ComputeApp();
    // --------------------------------------------------
    // Private methods
    // --------------------------------------------------
    private:
        // Gets Vulkan instance
        void createInstance(std::vector<char const*> &enabledLayers, VkInstance& instance, bool requiresAtomic);
        
        // Gets physical device
        void getPhysicalDevice(VkInstance& instance, VkPhysicalDevice& physicalDevice);

        // Gets index of 1st queue family which supports compute
        uint32_t getComputeQueueFamilyIndex();
        // Creates logical device
        void createDevice(
            VkPhysicalDevice& physicalDevice,
            uint32_t& queueFamilyIndex,
            VkDevice& device,
            VkQueue& queue
        );

        // Findsmemory type with given properties
        uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);

        // Creates buffer
        void createBuffer(
            VkDevice& device,
            uint32_t size,
            VkBuffer* buffer,
            VkDeviceMemory* bufferMemory
        );

        // Creates buffers
        void createBuffers(
            VkDevice& device,
            uint32_t const* bufferSizes,
            uint32_t numBuffers,
            VkBuffer*& buffers,
            VkDeviceMemory*& bufferMemories
        );

        // Fills buffers with data
        // Must be after `createBuffers` but before `createComputePipeline`
        void fillBuffers(
            VkDevice& device,
            float**& bufferData,
            VkDeviceMemory*& bufferMemories,
            uint32_t numBuffers,
            uint32_t const* bufferSizes
        );

        // Creates descriptor set layout
        void createDescriptorSetLayout(
            VkDevice& device,
            uint32_t numBuffers,
            VkDescriptorSetLayout* descriptorSetLayout
        );

        // Creates descriptor set
        void createDescriptorSet(
            VkDevice& device,
            uint32_t storageBuffers,
            VkDescriptorPool* descriptorPool,
            VkDescriptorSetLayout* descriptorSetLayout,
            VkBuffer* buffers
        );

        // Reads shader file
        uint32_t* readShader(uint32_t& length, const char* filename);

        // Creates compute pipeline
        void createComputePipeline(
            VkDevice& device,
            char* shaderFile,
            VkShaderModule* computeShaderModule,
            VkDescriptorSetLayout* descriptorSetLayout,
            VkPipelineLayout* pipelineLayout,
            VkPipeline* pipeline,
            float const* pushConstants,
            uint32_t numPushConstants
        );

        // Creates command buffer
        void createCommandBuffer(
            uint32_t queueFamilyIndex,
            VkDevice& device,
            VkCommandPool* commandPool,
            VkCommandBuffer* commandBuffer,
            VkPipeline& pipeline,
            VkPipelineLayout& pipelineLayout,
            float const* pushConstants,
            uint32_t numPushConstants,
            int const* dims, // [x,y,z],
            int const* dimLengths // [local_size_x, local_size_y, local_size_z]
        );

        // Runs command buffer
        void runCommandBuffer(
            VkCommandBuffer* commandBuffer,
            VkDevice& device,
            VkQueue& queue
        );

        
};

int TwentyOne();