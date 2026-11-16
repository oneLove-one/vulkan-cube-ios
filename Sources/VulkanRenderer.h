#pragma once

#ifndef VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_METAL_EXT 1
#endif

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class VulkanRenderer {
public:
    bool init(void* metalLayer, uint32_t width, uint32_t height,
              const char* vertexShaderPath, const char* fragmentShaderPath);
    void resize(uint32_t width, uint32_t height);
    void draw();
    void shutdown();
    ~VulkanRenderer() { shutdown(); }

private:
    bool createInstance();
    bool createSurface(void* metalLayer);
    bool pickPhysicalDevice();
    bool createDevice();
    bool createSwapchain();
    bool createRenderPass();
    bool createDepthResources();
    bool createFramebuffers();
    bool createPipeline(const char* vertexShaderPath, const char* fragmentShaderPath);
    bool createVertexBuffer();
    bool createCommands();
    bool createSyncObjects();
    VkShaderModule loadShader(const char* path);
    bool recreateSwapchain();
    void cleanupSwapchain();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_gpu = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    uint32_t m_graphicsFamily = 0;
    uint32_t m_presentFamily = 0;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_extent{};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView m_depthView = VK_NULL_HANDLE;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_renderFinished = VK_NULL_HANDLE;
    VkFence m_inFlight = VK_NULL_HANDLE;

    uint32_t m_pendingWidth = 0;
    uint32_t m_pendingHeight = 0;
    bool m_resizePending = false;
    bool m_initialized = false;
};
