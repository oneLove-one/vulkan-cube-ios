#include "VulkanRenderer.h"

#include <vulkan/vulkan_metal.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

#define VK_CHECK(x)                                                      \
    do {                                                                 \
        VkResult _res = (x);                                             \
        if (_res != VK_SUCCESS) {                                        \
            std::fprintf(stderr, "Vulkan error %d at %s:%d\n",           \
                         static_cast<int>(_res), __FILE__, __LINE__);    \
            return false;                                                \
        }                                                                \
    } while (0)

struct Vertex {
    float pos[3];
    float color[3];
};

constexpr float kCubeSize = 1.0f;

const Vertex kVertices[] = {
    {{-kCubeSize, -kCubeSize, -kCubeSize}, {1.0f, 0.2f, 0.2f}},
    {{-kCubeSize, -kCubeSize, kCubeSize}, {1.0f, 0.2f, 0.2f}},
    {{-kCubeSize, kCubeSize, kCubeSize}, {1.0f, 0.2f, 0.2f}},
    {{-kCubeSize, -kCubeSize, -kCubeSize}, {1.0f, 0.2f, 0.2f}},
    {{-kCubeSize, kCubeSize, kCubeSize}, {1.0f, 0.2f, 0.2f}},
    {{-kCubeSize, kCubeSize, -kCubeSize}, {1.0f, 0.2f, 0.2f}},

    {{kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 0.8f, 0.3f}},
    {{kCubeSize, -kCubeSize, -kCubeSize}, {0.2f, 0.8f, 0.3f}},
    {{kCubeSize, kCubeSize, -kCubeSize}, {0.2f, 0.8f, 0.3f}},
    {{kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 0.8f, 0.3f}},
    {{kCubeSize, kCubeSize, -kCubeSize}, {0.2f, 0.8f, 0.3f}},
    {{kCubeSize, kCubeSize, kCubeSize}, {0.2f, 0.8f, 0.3f}},

    {{-kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},
    {{kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},
    {{kCubeSize, kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},
    {{-kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},
    {{kCubeSize, kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},
    {{-kCubeSize, kCubeSize, kCubeSize}, {0.2f, 0.4f, 1.0f}},

    {{kCubeSize, -kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},
    {{-kCubeSize, -kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},
    {{-kCubeSize, kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},
    {{kCubeSize, -kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},
    {{-kCubeSize, kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},
    {{kCubeSize, kCubeSize, -kCubeSize}, {1.0f, 0.8f, 0.2f}},

    {{-kCubeSize, kCubeSize, kCubeSize}, {0.8f, 0.3f, 1.0f}},
    {{kCubeSize, kCubeSize, kCubeSize}, {0.8f, 0.3f, 1.0f}},
    {{kCubeSize, kCubeSize, -kCubeSize}, {0.8f, 0.3f, 1.0f}},
    {{-kCubeSize, kCubeSize, kCubeSize}, {0.8f, 0.3f, 1.0f}},
    {{kCubeSize, kCubeSize, -kCubeSize}, {0.8f, 0.3f, 1.0f}},
    {{-kCubeSize, kCubeSize, -kCubeSize}, {0.8f, 0.3f, 1.0f}},

    {{-kCubeSize, -kCubeSize, -kCubeSize}, {0.2f, 1.0f, 1.0f}},
    {{kCubeSize, -kCubeSize, -kCubeSize}, {0.2f, 1.0f, 1.0f}},
    {{kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 1.0f, 1.0f}},
    {{-kCubeSize, -kCubeSize, -kCubeSize}, {0.2f, 1.0f, 1.0f}},
    {{kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 1.0f, 1.0f}},
    {{-kCubeSize, -kCubeSize, kCubeSize}, {0.2f, 1.0f, 1.0f}},
};

void matIdentity(float* m) {
    for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

void matMultiply(float* out, const float* a, const float* b) {
    float r[16];
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a[k * 4 + row] * b[col * 4 + k];
            r[col * 4 + row] = sum;
        }
    }
    std::memcpy(out, r, sizeof(r));
}

void matPerspective(float* m, float fovY, float aspect, float zNear, float zFar) {
    std::memset(m, 0, 16 * sizeof(float));
    const float f = 1.0f / std::tan(fovY * 0.5f);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = zFar / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (zFar * zNear) / (zNear - zFar);
}

void matTranslation(float* m, float x, float y, float z) {
    matIdentity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

void matRotationX(float* m, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    matIdentity(m);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void matRotationY(float* m, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    matIdentity(m);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

uint32_t findMemoryType(VkPhysicalDevice gpu, uint32_t typeBits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem{};
    vkGetPhysicalDeviceMemoryProperties(gpu, &mem);
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) &&
            (mem.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

}  // namespace

bool VulkanRenderer::init(void* metalLayer, uint32_t width, uint32_t height,
                          const char* vertexShaderPath, const char* fragmentShaderPath) {
    m_pendingWidth = width;
    m_pendingHeight = height;

    if (!createInstance()) return false;
    if (!createSurface(metalLayer)) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createDevice()) return false;
    if (!createSwapchain()) return false;
    if (!createRenderPass()) return false;
    if (!createDepthResources()) return false;
    if (!createFramebuffers()) return false;
    if (!createPipeline(vertexShaderPath, fragmentShaderPath)) return false;
    if (!createVertexBuffer()) return false;
    if (!createCommands()) return false;
    if (!createSyncObjects()) return false;

    m_initialized = true;
    return true;
}

bool VulkanRenderer::createInstance() {
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "VulkanCube";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "None";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
    };

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = 2;
    info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateInstance(&info, nullptr, &m_instance));
    return true;
}

bool VulkanRenderer::createSurface(void* metalLayer) {
    VkMetalSurfaceCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    info.pLayer = metalLayer;
    VK_CHECK(vkCreateMetalSurfaceEXT(m_instance, &info, nullptr, &m_surface));
    return true;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &count, nullptr));
    if (count == 0) return false;

    std::vector<VkPhysicalDevice> devices(count);
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &count, devices.data()));

    for (VkPhysicalDevice gpu : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());

        for (uint32_t i = 0; i < familyCount; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_surface, &present);
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                m_gpu = gpu;
                m_graphicsFamily = i;
                m_presentFamily = i;
                return true;
            }
        }
    }
    return false;
}

bool VulkanRenderer::createDevice() {
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    uint32_t availableCount = 0;
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &availableCount, nullptr);
    std::vector<VkExtensionProperties> available(availableCount);
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &availableCount, available.data());

    std::vector<const char*> enabledExtensions;
    enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    for (const auto& ext : available) {
        if (std::strcmp(ext.extensionName, "VK_KHR_portability_subset") == 0) {
            enabledExtensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }
    (void)extensions;

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queueInfo;
    info.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    info.ppEnabledExtensionNames = enabledExtensions.data();

    VK_CHECK(vkCreateDevice(m_gpu, &info, nullptr, &m_device));
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    m_presentQueue = m_graphicsQueue;
    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &caps));

    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &formatCount, formats.data()));

    VkSurfaceFormatKHR chosen = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    m_swapFormat = chosen.format;

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = m_pendingWidth;
        extent.height = m_pendingHeight;
    }
    extent.width = std::max(1u, extent.width);
    extent.height = std::max(1u, extent.height);
    m_extent = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = m_surface;
    info.minImageCount = imageCount;
    info.imageFormat = chosen.format;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapchain));

    uint32_t actualCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualCount, nullptr));
    m_images.resize(actualCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualCount, m_images.data()));

    m_imageViews.resize(actualCount);
    for (uint32_t i = 0; i < actualCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_swapFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageViews[i]));
    }
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription color{};
    color.format = m_swapFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color, depth};

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(m_device, &info, nullptr, &m_renderPass));
    return true;
}

bool VulkanRenderer::createDepthResources() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {m_extent.width, m_extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage));

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(m_device, m_depthImage, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = findMemoryType(m_gpu, req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &m_depthMemory));
    VK_CHECK(vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthView));
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_imageViews.size(); ++i) {
        VkImageView attachments[] = {m_imageViews[i], m_depthView};

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = m_renderPass;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.width = m_extent.width;
        info.height = m_extent.height;
        info.layers = 1;
        VK_CHECK(vkCreateFramebuffer(m_device, &info, nullptr, &m_framebuffers[i]));
    }
    return true;
}

VkShaderModule VulkanRenderer::loadShader(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "Cannot open shader: %s\n", path);
        return VK_NULL_HANDLE;
    }
    std::vector<char> data((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = data.size();
    info.pCode = reinterpret_cast<const uint32_t*>(data.data());

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device, &info, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

bool VulkanRenderer::createPipeline(const char* vertexShaderPath, const char* fragmentShaderPath) {
    VkShaderModule vert = loadShader(vertexShaderPath);
    VkShaderModule frag = loadShader(fragmentShaderPath);
    if (!vert || !frag) return false;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributes[2]{};
    attributes[0].location = 0;
    attributes[0].binding = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, pos);
    attributes[1].location = 1;
    attributes[1].binding = 0;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthState{};
    depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthState.depthTestEnable = VK_TRUE;
    depthState.depthWriteEnable = VK_TRUE;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamicStates;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = 16 * sizeof(float);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout));

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depthState;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = m_pipelineLayout;
    info.renderPass = m_renderPass;

    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr,
                                                &m_pipeline);
    vkDestroyShaderModule(m_device, frag, nullptr);
    vkDestroyShaderModule(m_device, vert, nullptr);
    if (result != VK_SUCCESS) return false;
    return true;
}

bool VulkanRenderer::createVertexBuffer() {
    const VkDeviceSize size = sizeof(kVertices);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer));

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &req);

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = findMemoryType(m_gpu, req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &m_vertexMemory));
    VK_CHECK(vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexMemory, 0));

    void* mapped = nullptr;
    VK_CHECK(vkMapMemory(m_device, m_vertexMemory, 0, size, 0, &mapped));
    std::memcpy(mapped, kVertices, static_cast<size_t>(size));
    vkUnmapMemory(m_device, m_vertexMemory);
    return true;
}

bool VulkanRenderer::createCommands() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsFamily;
    VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool));

    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = m_commandPool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(m_device, &alloc, &m_commandBuffer));
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable));
    VK_CHECK(vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight));
    return true;
}

void VulkanRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == m_extent.width && height == m_extent.height) return;
    m_pendingWidth = width;
    m_pendingHeight = height;
    m_resizePending = true;
}

void VulkanRenderer::draw() {
    if (!m_initialized) return;
    if (m_resizePending) {
        m_resizePending = false;
        if (!recreateSwapchain()) return;
    }

    vkWaitForFences(m_device, 1, &m_inFlight, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquire = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailable,
                                             VK_NULL_HANDLE, &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        m_pendingWidth = m_extent.width;
        m_pendingHeight = m_extent.height;
        m_resizePending = true;
        return;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) return;

    vkResetFences(m_device, 1, &m_inFlight);
    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_commandBuffer, &begin);

    VkClearValue clears[2]{};
    clears[0].color = {{0.04f, 0.06f, 0.10f, 1.0f}};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = m_renderPass;
    pass.framebuffer = m_framebuffers[imageIndex];
    pass.renderArea.extent = m_extent;
    pass.clearValueCount = 2;
    pass.pClearValues = clears;
    vkCmdBeginRenderPass(m_commandBuffer, &pass, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{0.0f, 0.0f, static_cast<float>(m_extent.width),
                        static_cast<float>(m_extent.height), 0.0f, 1.0f};
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_extent};
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const float t = std::chrono::duration<float>(now).count();

    float projection[16];
    float aspect = static_cast<float>(m_extent.width) / static_cast<float>(m_extent.height);
    matPerspective(projection, 60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);

    float view[16];
    matTranslation(view, 0.0f, 0.0f, -4.0f);

    float rotX[16];
    float rotY[16];
    matRotationX(rotX, t * 0.6f);
    matRotationY(rotY, t * 0.9f);

    float model[16];
    matMultiply(model, rotY, rotX);

    float viewModel[16];
    matMultiply(viewModel, view, model);

    float mvp[16];
    matMultiply(mvp, projection, viewModel);

    vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       16 * sizeof(float), mvp);

    VkBuffer buffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, buffers, offsets);
    vkCmdDraw(m_commandBuffer, 36, 1, 0, 0);

    vkCmdEndRenderPass(m_commandBuffer);
    vkEndCommandBuffer(m_commandBuffer);

    VkSemaphore waitSemaphores[] = {m_imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {m_renderFinished};

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = waitSemaphores;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submit, m_inFlight) != VK_SUCCESS) return;

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = signalSemaphores;
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain;
    present.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(m_presentQueue, &present);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        m_pendingWidth = m_extent.width;
        m_pendingHeight = m_extent.height;
        m_resizePending = true;
    }
}

void VulkanRenderer::cleanupSwapchain() {
    for (VkFramebuffer fb : m_framebuffers) {
        vkDestroyFramebuffer(m_device, fb, nullptr);
    }
    m_framebuffers.clear();

    for (VkImageView view : m_imageViews) {
        vkDestroyImageView(m_device, view, nullptr);
    }
    m_imageViews.clear();
    m_images.clear();

    if (m_depthView) {
        vkDestroyImageView(m_device, m_depthView, nullptr);
        m_depthView = VK_NULL_HANDLE;
    }
    if (m_depthImage) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthMemory) {
        vkFreeMemory(m_device, m_depthMemory, nullptr);
        m_depthMemory = VK_NULL_HANDLE;
    }
    if (m_swapchain) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::recreateSwapchain() {
    uint32_t width = m_pendingWidth ? m_pendingWidth : m_extent.width;
    uint32_t height = m_pendingHeight ? m_pendingHeight : m_extent.height;
    if (width == 0 || height == 0) return false;

    vkDeviceWaitIdle(m_device);
    cleanupSwapchain();

    m_pendingWidth = width;
    m_pendingHeight = height;

    if (!createSwapchain()) return false;
    if (!createDepthResources()) return false;
    if (!createFramebuffers()) return false;
    return true;
}

void VulkanRenderer::shutdown() {
    if (m_device) {
        vkDeviceWaitIdle(m_device);
        cleanupSwapchain();

        if (m_pipeline) vkDestroyPipeline(m_device, m_pipeline, nullptr);
        if (m_pipelineLayout) vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        if (m_renderPass) vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        if (m_vertexBuffer) vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        if (m_vertexMemory) vkFreeMemory(m_device, m_vertexMemory, nullptr);
        if (m_commandPool) vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        if (m_imageAvailable) vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
        if (m_renderFinished) vkDestroySemaphore(m_device, m_renderFinished, nullptr);
        if (m_inFlight) vkDestroyFence(m_device, m_inFlight, nullptr);
        vkDestroyDevice(m_device, nullptr);
    }
    if (m_surface && m_instance) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (m_instance) vkDestroyInstance(m_instance, nullptr);

    m_pipeline = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexMemory = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_imageAvailable = VK_NULL_HANDLE;
    m_renderFinished = VK_NULL_HANDLE;
    m_inFlight = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;
    m_initialized = false;
}
