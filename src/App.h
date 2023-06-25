#pragma once
#define GLFW_INCLUDE_VULKAN
#if defined(WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__LINUX__)
#define VK_USE_PLATFORM_X11_KHR
#define GLFW_EXPOSE_NATIVE_X11
#endif

#ifdef _DEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
    const bool APP_DEBUG = true;
#else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
    const bool APP_DEBUG = false;
#endif

#include <map>
#include <set>
#include <limits>
#include <vector>
#include <format>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vk_enum_string_helper.h>
#include <spdlog/sinks/stdout_sinks.h>


namespace
{
    uint16_t DEFAULT_WIDTH{800};
    uint16_t DEFAULT_HEIGHT{600};
    static void GLFWerrorCallback(int code, const char *data)
    {
        SPDLOG_CRITICAL("GLFW ERROR {}: {}", code, data);
    }
    struct _initialize
    {
        _initialize()
        {
            try
            {
                spdlog::set_level(APP_DEBUG ? spdlog::level::trace : spdlog::level::critical);
                spdlog::set_pattern("[func %!] [line %#] [%H:%M:%S.%e] [%^%l%$] %v");
                SPDLOG_DEBUG("--- Start logging. ---");
            }
            catch(const std::exception &logInitE)
            {
                std::cerr << "Logger initialize error:\t" <<logInitE.what() << '\n.';
            }
            
            // exit(EXIT_FAILURE);

            if (glfwInit())
            {
                SPDLOG_DEBUG("GLFW{} inititialized.", glfwGetVersionString());
                glfwSetErrorCallback( GLFWerrorCallback );
            }
            else
            {
                SPDLOG_CRITICAL("GLFW not initialized.");
                SPDLOG_INFO("Exit with code {}.", EXIT_FAILURE);
                exit(EXIT_FAILURE);
            }
        }
        ~_initialize()
        {
            glfwTerminate();
            SPDLOG_DEBUG("App closed.");
            SPDLOG_DEBUG("--- Log finish. ---");
            spdlog::shutdown();
        }
    }_;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
struct SwapChainProperties
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Format;
    std::vector<VkPresentModeKHR> PresentModes;
};
struct Vertex
{
    glm::vec3 coordinate;
    glm::vec4 color;
};

static void FramebufferResizeCallback(GLFWwindow *AppPointer, int width, int height);
static void WindwoResizeCallback(GLFWwindow *AppPointer, int width, int height);
// static void WindowMaximizeCallback(GLFWwindow *AppPointer, int maximized);

class App
{
public:
    uint16_t WIDTH;
    uint16_t HEIGHT;
    int32_t DISPLAY_WIDTH;
    int32_t DISPLAY_HEIGHT;
    std::string TITLE;

    App(uint16_t width, uint16_t height, const char * title) : WIDTH{width}, HEIGHT{height}, TITLE{title}
    {
        initWindow();
        initVulkan();
        CheckValidationLayers();
        mainLoop();
    }
    ~App()
    {
        DestroySwapchain();
        vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
        for (uint8_t i{0}; i < _MaxFramesInFlight; i++)
        {
            vkDestroySemaphore(LogicalDevice, ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(LogicalDevice, RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(LogicalDevice, WaitFrames[i], nullptr);
        }
        vkDestroyDevice(LogicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, Surface, nullptr);
        if (APP_DEBUG) DestroyDebugUtilsMessengerEXT(instance, DebugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
    }

    void CentralizeWindow()
    {
        glfwSetWindowPos(window, (DISPLAY_WIDTH/2)-(WIDTH/2), (DISPLAY_HEIGHT/2)-(HEIGHT/2));
    }

    void SetWindowSize(uint32_t width, uint32_t height)
    {
        if (WIDTH != width || HEIGHT != height)
        {
            WIDTH = width;
            HEIGHT = height;
            glfwSetWindowSize(window, WIDTH, HEIGHT);
        }
        ReCreateSwapChain();
    }

    void GetScreenResolution(int32_t &width, int32_t &height)
    {
        GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
        glfwGetVideoMode(Monitor);
        const GLFWvidmode* mode = glfwGetVideoMode(Monitor);
        width = mode->width;
        height = mode->height;
    }

private:
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR Surface;
    const std::vector<const char *> ValidationLayers{"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> RequeredDeviceExts{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDevice LogicalDevice;
    VkQueue LogicalDeviceGraphickQueue;
    VkQueue LogicalDevicePresentQueue;
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStage;
    VkPipelineLayout PipelineLayout;
    VkRenderPass RenderPass;
    VkPipeline GraphicsPipeline;
    SwapChainProperties PhysiacalDeviceSwapchainProperties;
    VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
    QueueFamilyIndices PhysicalDeviceIndices;
    VkSwapchainKHR SwapChain;
    VkFormat SwapchainFormat;
    VkPresentModeKHR SwapchainPresentMode;
    std::vector<VkImage>SwapchainImgs;
    std::vector<VkImageView>SwapchainImgsView;
    std::vector<VkFramebuffer>SwapchainFrameBuffers;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> WaitFrames;
    uint8_t _CurrentFrame{0};
    uint8_t _MaxFramesInFlight{1};
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT DbgMessengerCreateInfo{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        NULL,
        NULL,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        DebugCallback,
        this};

    void initWindow()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GetScreenResolution(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        if (!(WIDTH | HEIGHT))
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        if (!WIDTH) WIDTH = DISPLAY_WIDTH;
        if (!HEIGHT) HEIGHT = DISPLAY_HEIGHT;
        window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.data(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
        glfwSetWindowSizeCallback(window, WindwoResizeCallback);
        // glfwSetWindowMaximizeCallback(window, WindowMaximizeCallback);
        CentralizeWindow();
    }

    void initVulkan()
    {
        CreateVKinstance();
        SetupDebugMessenger();
        #if defined( WIN32 )
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);
        VkResult Result{vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &Surface)};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("WIN32 surface wasn't be created, error: {}", string_VkResult(Result));
            throw std::runtime_error(std::format("WIN32 surface wasn't be created, error: {}", string_VkResult(Result)));
        }
        #elif defined( __LINUX__ )
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetX11Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);
        VkResult Result{vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &Surface)};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Linux surface wasn't be created, error: {}", string_VkResult(Result));
            throw std::runtime_error(std::format("Linux surface wasn't be created, error: {}", string_VkResult(Result)));
        }
        #endif
        GetPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipelines();
        CreateFrameBuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo FenceInfo{};
        FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint8_t i{0}; i < _MaxFramesInFlight; i++)
        {
            ImageAvailableSemaphores.resize(_MaxFramesInFlight);
            RenderFinishedSemaphores.resize(_MaxFramesInFlight);
            WaitFrames.resize(_MaxFramesInFlight);
            Result = vkCreateSemaphore(LogicalDevice, &semaphoreInfo, nullptr, &ImageAvailableSemaphores[i]);
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to create Semaphore, error: {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to create Semaphore, error: {}.", string_VkResult(Result)));
            }
            Result = vkCreateSemaphore(LogicalDevice, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[i]);
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to create Semaphore, error: {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to create Semaphore, error: {}.", string_VkResult(Result)));
            }
            Result = vkCreateFence(LogicalDevice, &FenceInfo, nullptr, &WaitFrames[i]);
            if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to create Semaphore, error: {}.", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to create Semaphore, error: {}.", string_VkResult(Result)));
        }
        }
    }

    void mainLoop()
    {
        while (!(glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS))
        {
            glfwPollEvents();
            drawFrame();
        }
    }
   
    void drawFrame()
    {
        uint32_t imgI;
        vkWaitForFences(LogicalDevice, 1, &WaitFrames[_CurrentFrame], VK_TRUE, UINT64_MAX);
        switch (vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphores[_CurrentFrame], nullptr, &imgI))
        {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            ReCreateSwapChain();
            return;
        default:
            SPDLOG_CRITICAL("Failed to Acqueire image from swapchain.");
            throw std::runtime_error("Failed to Acqueire image from swapchain.");
        }
        vkResetFences(LogicalDevice, 1, &WaitFrames[_CurrentFrame]);
        vkResetCommandBuffer(CommandBuffers[_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        RecordCommandBuffer(CommandBuffers[_CurrentFrame], imgI);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore ImageSemaphore[] = {ImageAvailableSemaphores[_CurrentFrame]};
        VkSemaphore RenderSemaphore[] = {RenderFinishedSemaphores[_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = ImageSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &CommandBuffers[_CurrentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = RenderSemaphore;
        VkResult Result{vkQueueSubmit(LogicalDeviceGraphickQueue, 1, &submitInfo, WaitFrames[_CurrentFrame])};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to Submit draw frame, error: {}.", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to Submit draw frame, error: {}.", string_VkResult(Result)));
        }
        VkPresentInfoKHR PresentInfo{};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = RenderSemaphore;
        VkSwapchainKHR swapChains[] = {SwapChain};
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = swapChains;
        PresentInfo.pImageIndices = &imgI;
        vkQueuePresentKHR(LogicalDeviceGraphickQueue, &PresentInfo);
        _CurrentFrame = ++_CurrentFrame%_MaxFramesInFlight;
    }

    // Support Functios
    void DestroySwapchain()
    {
        vkDeviceWaitIdle(LogicalDevice);
        for (size_t i{0}; i < SwapchainImgsView.size(); i++)
        {
            vkDestroyFramebuffer(LogicalDevice, SwapchainFrameBuffers[i], nullptr);
            vkDestroyImageView(LogicalDevice, SwapchainImgsView[i], nullptr);
        }
        for (const auto &shaderS : ShaderStage) vkDestroyShaderModule(LogicalDevice, shaderS.module, nullptr);
        vkFreeCommandBuffers(LogicalDevice, CommandPool, static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
        vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
        vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
        vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
        vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
    }


    void GetPhysicalDevice()
    {
        uint32_t CountPhysicalDevices{0};
        vkEnumeratePhysicalDevices(instance, &CountPhysicalDevices, nullptr);
        if (!CountPhysicalDevices)
        {
            SPDLOG_CRITICAL("Not finded physical device.", CountPhysicalDevices);
            throw std::runtime_error("Failed to find Physical Device.");
        }
        std::vector<VkPhysicalDevice>PhysicalDevices{CountPhysicalDevices};
        vkEnumeratePhysicalDevices(instance, &CountPhysicalDevices, PhysicalDevices.data());
        // if (!CountPhysicalDevices-1) PhysicalDevice = PhysicalDevices[0];
        for (const auto& device : PhysicalDevices)
        {
            if (isDeviceSuitable(device)) break;
        }
        if (PhysicalDevice == VK_NULL_HANDLE){
            PhysicalDeviceProperties = {};
            PhysicalDeviceFeatures = {};
            SPDLOG_CRITICAL("Instance haven't suitable Physical Devices.");
            throw std::runtime_error("Failed to find suitable Physical Devices.");
        }
        SPDLOG_INFO("Selected {}.", PhysicalDeviceProperties.deviceName);
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        vkGetPhysicalDeviceProperties(device, &PhysicalDeviceProperties);
        vkGetPhysicalDeviceFeatures(device, &PhysicalDeviceFeatures);
        auto indices{findQueueFamilies(device)};
        bool dSupportExts{isDeviceSupportExtensions(device)};
        if (!(
            indices.isComplete() || 
            (PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && PhysicalDeviceFeatures.geometryShader) ||
            dSupportExts
            )) return false;
        if (dSupportExts)
        {
            SwapChainProperties deviceSwapChainProperties{GetSwapchainProperties(device)};
            if (!deviceSwapChainProperties.Format.size() && !deviceSwapChainProperties.PresentModes.size()) return false;
            PhysiacalDeviceSwapchainProperties = deviceSwapChainProperties;
        }
        PhysicalDevice = device;
        PhysicalDeviceIndices = indices;
        return true;
    }
    
    bool isDeviceSupportExtensions(VkPhysicalDevice device)
    {
        uint32_t extCount{0};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties>AvilableExtNames{extCount};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, AvilableExtNames.data());
        std::set<std::string>tmpRequeredDeviceExts{RequeredDeviceExts.begin(), RequeredDeviceExts.end()};
        for (const auto &ext : AvilableExtNames)
        {
            tmpRequeredDeviceExts.erase(ext.extensionName);
        }
        return tmpRequeredDeviceExts.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties>QueueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, QueueFamilies.data());
        uint32_t index{0};
        for (const auto &queueF : QueueFamilies)
        {
            if (queueF.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = index;
            VkBool32 presentSupport{false};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, Surface, &presentSupport);
            if (presentSupport) indices.presentFamily = index;
            index++;
        }
        return indices;
    }

    void CreateLogicalDevice()
    {
        std::vector<VkDeviceQueueCreateInfo>queueCreateInfos{};
        std::unordered_map<uint32_t, float>QueueFamiliesPriority{
            {PhysicalDeviceIndices.graphicsFamily.value(), 1.0f},
            {PhysicalDeviceIndices.presentFamily.value(), 1.0f},
            };
        for (auto &QueueFamily : QueueFamiliesPriority)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = QueueFamily.first;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &QueueFamily.second;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(RequeredDeviceExts.size());
        createInfo.ppEnabledExtensionNames = RequeredDeviceExts.data();
        if (APP_DEBUG)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            createInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else createInfo.enabledLayerCount = 0;
        // extension

        VkResult DeviceCreateCode = vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &LogicalDevice);
        if (DeviceCreateCode != VK_SUCCESS) throw std::runtime_error(std::format("Failed create logic device, error code: {}.", string_VkResult(DeviceCreateCode)));
        vkGetDeviceQueue(LogicalDevice, PhysicalDeviceIndices.graphicsFamily.value(), 0, &LogicalDeviceGraphickQueue);
        vkGetDeviceQueue(LogicalDevice, PhysicalDeviceIndices.presentFamily.value(), 0, &LogicalDevicePresentQueue);
    }
    
    SwapChainProperties GetSwapchainProperties(VkPhysicalDevice device)
    {
        SwapChainProperties Properties;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Surface, &Properties.Capabilities);
        uint32_t formatsCount{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, Surface, &formatsCount, nullptr);
        // if (formatsCount)
        Properties.Format.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, Surface, &formatsCount, Properties.Format.data());
        uint32_t PresentModesCount{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, Surface, &PresentModesCount, nullptr);
        Properties.PresentModes.resize(PresentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, Surface, &PresentModesCount, Properties.PresentModes.data());
        return Properties;
    }

    void CreateImageViews()
    {
        SwapchainImgsView.resize(SwapchainImgs.size());
        for (size_t i{0}; i < SwapchainImgs.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = SwapchainImgs[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = SwapchainFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            VkResult Result{vkCreateImageView(LogicalDevice, &createInfo, nullptr, &SwapchainImgsView[i])};
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to create imageView, error {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to create imageView, error {}.", string_VkResult(Result)));
            }
        }
    }

    void CreateSwapchain()
    {
        PhysiacalDeviceSwapchainProperties = GetSwapchainProperties(PhysicalDevice);
        VkSurfaceFormatKHR SurfaceFormat{PhysiacalDeviceSwapchainProperties.Format[0]};
        for (const auto &format : PhysiacalDeviceSwapchainProperties.Format)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) SurfaceFormat = format;
        }
        SwapchainFormat = SurfaceFormat.format;

        VkPresentModeKHR SurfacePresentMode{VK_PRESENT_MODE_FIFO_KHR};
        for (const auto &mode : PhysiacalDeviceSwapchainProperties.PresentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) SwapchainPresentMode = SurfacePresentMode = mode;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) 
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.width = DISPLAY_WIDTH;
        PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.height = DISPLAY_HEIGHT;
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width = std::clamp(static_cast<uint32_t>(width), PhysiacalDeviceSwapchainProperties.Capabilities.minImageExtent.width, PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.width);
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height = std::clamp(static_cast<uint32_t>(height), PhysiacalDeviceSwapchainProperties.Capabilities.minImageExtent.height, PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.height);
        // PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height = height;
        uint32_t imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.minImageCount;
        if (PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount > 0 && imageCount < PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount) {
            imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount;
        }
        _MaxFramesInFlight = imageCount;
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = SurfaceFormat.format;
        createInfo.imageColorSpace = SurfaceFormat.colorSpace;
        createInfo.presentMode = SurfacePresentMode;
        createInfo.imageExtent = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t physicalDeviceIndicesValue[]{PhysicalDeviceIndices.graphicsFamily.value(), PhysicalDeviceIndices.presentFamily.value()};
        if (PhysicalDeviceIndices.graphicsFamily != PhysicalDeviceIndices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = physicalDeviceIndicesValue;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        createInfo.preTransform = PhysiacalDeviceSwapchainProperties.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        VkResult Result{vkCreateSwapchainKHR(LogicalDevice, &createInfo, nullptr, &SwapChain)};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Swapchain wasn't be created, error: {}", string_VkResult(Result));
            throw std::runtime_error(std::format("Swapchain wasn't be created, error: {}", string_VkResult(Result)));
        }
        vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &imageCount, nullptr);
        SwapchainImgs.resize(imageCount);
        vkGetSwapchainImagesKHR(LogicalDevice, SwapChain, &imageCount, SwapchainImgs.data());
        
    }

    void ReCreateSwapChain()
    {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        while (!(w|| h)) 
        {
            glfwGetFramebufferSize(window, &w, &h);
            glfwWaitEvents();
        }
        DestroySwapchain();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipelines();
        CreateFrameBuffers();
        CreateCommandBuffers();
    }

    void CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = SwapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    VkResult Result{vkCreateRenderPass(LogicalDevice, &renderPassInfo, nullptr, &RenderPass)};
    if (Result != VK_SUCCESS)
    {
        SPDLOG_CRITICAL("Failed to create render pass, error: {}.", string_VkResult(Result));
        throw std::runtime_error(std::format("Failed to create render pass, error: {}.", string_VkResult(Result)));
    }
    }

    void CreateGraphicsPipelines()
    {
        VkPipelineShaderStageCreateInfo VertexShaderStage{};
        VertexShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertexShaderStage.module = CreateShaderModule("../shaders/shader.vert.spv");
        VertexShaderStage.pName = "main";

        VkPipelineShaderStageCreateInfo FragmentShaderStage{};
        FragmentShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragmentShaderStage.module = CreateShaderModule("../shaders/shader.frag.spv");
        FragmentShaderStage.pName = "main";
        ShaderStage = {VertexShaderStage, FragmentShaderStage};

        std::vector<VkDynamicState>dStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dStatescreateInfo{};
        dStatescreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dStatescreateInfo.dynamicStateCount = static_cast<uint32_t>(dStates.size());
        dStatescreateInfo.pDynamicStates = dStates.data();

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_TRUE;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth = 1.f;
        Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        Rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo{};
        // DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;


        VkResult Result{vkCreatePipelineLayout(LogicalDevice, &pipelineLayoutInfo, nullptr, &PipelineLayout)};
        if (Result!=VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to create pipeline layout, error: {}", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to create pipeline layout, error: {}", string_VkResult(Result)));
        }

        VkGraphicsPipelineCreateInfo GraphicPipeLineCreateInfo{};
        GraphicPipeLineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        GraphicPipeLineCreateInfo.stageCount = static_cast<uint32_t>(ShaderStage.size());
        GraphicPipeLineCreateInfo.pStages = ShaderStage.data();
        GraphicPipeLineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        GraphicPipeLineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        GraphicPipeLineCreateInfo.pViewportState = &viewportState;
        GraphicPipeLineCreateInfo.pRasterizationState = &Rasterizer;
        GraphicPipeLineCreateInfo.pMultisampleState = &Multisampling;
        GraphicPipeLineCreateInfo.pColorBlendState = &colorBlending;
        GraphicPipeLineCreateInfo.pDynamicState = &dStatescreateInfo;
        GraphicPipeLineCreateInfo.layout = PipelineLayout;
        GraphicPipeLineCreateInfo.renderPass = RenderPass;
        GraphicPipeLineCreateInfo.subpass = 0;
        Result = vkCreateGraphicsPipelines(LogicalDevice, nullptr, 1, &GraphicPipeLineCreateInfo, nullptr, &GraphicsPipeline);
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to create Graphics pipeline, error: {}.", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to create Graphics pipeline, error: {}.", string_VkResult(Result)));
        }

    }

    VkShaderModule CreateShaderModule(const char * shPath)
    {
        std::ifstream File{shPath, std::fstream::ate|std::fstream::binary};
        if (!File.is_open())
        {
            SPDLOG_CRITICAL("Shader binary {} can't be opened.", shPath);
            throw std::runtime_error(std::format("Shader binary {} can't be opened.", shPath));
        }
        size_t shBsize{static_cast<size_t>(File.tellg())};
        std::vector<char>Data(shBsize);
        File.seekg(0);
        File.read(Data.data(), shBsize);
        File.close();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = Data.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(Data.data());
        VkShaderModule shaderModule;
        VkResult Result{vkCreateShaderModule(LogicalDevice, &createInfo, nullptr, &shaderModule)};
        if (Result!=VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to create shader module for {}, error:{}.", shPath, string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to create shader module for {}, error:{}.", shPath, string_VkResult(Result)));
        }
        return shaderModule;
    }

    void CreateFrameBuffers()
    {
        SwapchainFrameBuffers.resize(SwapchainImgsView.size());
        for (size_t i{0}; i < SwapchainImgsView.size(); i++)
        {
            VkFramebufferCreateInfo FrameBufferCreateInfo{};
            FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FrameBufferCreateInfo.renderPass = RenderPass;
            FrameBufferCreateInfo.attachmentCount = 1;
            FrameBufferCreateInfo.pAttachments = &SwapchainImgsView[i];
            FrameBufferCreateInfo.width = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width;
            FrameBufferCreateInfo.height = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height;
            FrameBufferCreateInfo.layers = 1;
            VkResult Result{vkCreateFramebuffer(LogicalDevice, &FrameBufferCreateInfo, nullptr, &SwapchainFrameBuffers[i])};
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to create Swapchain frame buffer, error: {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to create Swapchain frame buffer, error: {}.", string_VkResult(Result)));
            }
        }

    }

    void CreateCommandPool()
    {
        VkCommandPoolCreateInfo CommandPoolCreateInfo{};
        CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CommandPoolCreateInfo.queueFamilyIndex = PhysicalDeviceIndices.graphicsFamily.value();
        VkResult Result{vkCreateCommandPool(LogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool)};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to create grapchi Command pool, error: {}.", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to create grapchi Command pool, error: {}.", string_VkResult(Result)));
        }
    }
    
    void CreateCommandBuffers()
    {
        VkCommandBufferAllocateInfo CommandBufferAllocateInfo{};
        CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CommandBufferAllocateInfo.commandPool = CommandPool;
        CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferAllocateInfo.commandBufferCount = _MaxFramesInFlight;
        CommandBuffers.resize(_MaxFramesInFlight);
        VkResult Result{vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, CommandBuffers.data())};
        if (Result != VK_SUCCESS)
        {
            SPDLOG_CRITICAL("Failed to allocate Command buffers, error: {}.", string_VkResult(Result));
            throw std::runtime_error(std::format("Failed to allocate Command buffers, error: {}.", string_VkResult(Result)));
        }
    
    }
    
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imI)
    {
        // for (size_t i{0}; i < CommandBuffers.size(); i++)
        // {
            VkCommandBufferBeginInfo CommandBufferBeginInfo{};
            CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            // Result = vkBeginCommandBuffer(CommandBuffers[i], &CommandBufferBeginInfo);
            VkResult Result{vkBeginCommandBuffer(commandBuffer, &CommandBufferBeginInfo)};
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to beging recording in Command buffer, error: {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to beging recording in Command buffer, error: {}.", string_VkResult(Result)));
            }

            VkRenderPassBeginInfo RenderPassBeginInfo{};
            RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            RenderPassBeginInfo.renderPass = RenderPass;
            RenderPassBeginInfo.framebuffer = SwapchainFrameBuffers[imI];
            RenderPassBeginInfo.renderArea.offset = {0, 0};
            RenderPassBeginInfo.renderArea.extent = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
            VkClearValue ClsClr{0.68f, .0f, 1.f, 1.f};
            RenderPassBeginInfo.clearValueCount = 1;
            RenderPassBeginInfo.pClearValues = &ClsClr;

            vkCmdBeginRenderPass(commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
            
            VkViewport Viewport{};
            Viewport.x = .0f;
            Viewport.y = .0f;
            Viewport.width = static_cast<float>(PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width);
            Viewport.height = static_cast<float>(PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height);
            Viewport.minDepth = .0f;
            Viewport.maxDepth = 1.f;
            vkCmdSetViewport(commandBuffer, 0, 1, &Viewport);

            VkRect2D Scissor{};
            Scissor.offset = {0, 0};
            Scissor.extent = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &Scissor);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffer);

            Result = vkEndCommandBuffer(commandBuffer);
            if (Result != VK_SUCCESS)
            {
                SPDLOG_CRITICAL("Failed to record Command buffer, error: {}.", string_VkResult(Result));
                throw std::runtime_error(std::format("Failed to record Command buffer, error: {}.", string_VkResult(Result)));
            }
        // }
    }

    void CreateVKinstance()
    {
        if (APP_DEBUG && !CheckValidationLayers())
            throw std::runtime_error("Unavilable validation layer.");

        uint32_t glfwExtensionsCount{0};
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        uint32_t glfwAvailableExtensionsCount{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> AvailableExtensions(glfwAvailableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, AvailableExtensions.data());


        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = TITLE.data();
        AppInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        AppInfo.apiVersion = VK_API_VERSION_1_0;
        // AppInfo.pNext = ;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;

        if (APP_DEBUG)
        {
            CreateInfo.enabledLayerCount = uint32_t(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
            CreateInfo.pNext = &DbgMessengerCreateInfo;
        }
        else{
            CreateInfo.enabledLayerCount = 0;
            CreateInfo.pNext = nullptr;
        }

        std::vector<const char*> Extensions{glfwExtensions, glfwExtensions + glfwExtensionsCount};
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
        CreateInfo.ppEnabledExtensionNames = Extensions.data();
        
        VkResult InstanceCreateCode = vkCreateInstance(&CreateInfo, nullptr, &instance);
        if (InstanceCreateCode != VK_SUCCESS)
        {   
            throw std::runtime_error(std::format("Failed create instance, error code: {}.", string_VkResult(InstanceCreateCode)));
        }
    }

    bool CheckValidationLayers()
    {
        uint32_t Count;
        vkEnumerateInstanceLayerProperties(&Count, NULL);
        std::vector<VkLayerProperties> AviableLayers{Count};
        vkEnumerateInstanceLayerProperties(&Count, AviableLayers.data());
        for (const char *lName : ValidationLayers)
        {
            bool SupportLayer = false;
            for (const auto &AlName : AviableLayers)
            {
                if (!strcmp(lName, AlName.layerName))
                {
                    SupportLayer = true;
                    break;
                }
            }
            if (!SupportLayer)
            {
                SPDLOG_CRITICAL("Validation layer {} isn't support.", lName);
                return false;
            }
        }
        return true;
    }

    void SetupDebugMessenger()
    {
        if (!APP_DEBUG) return;
        VkResult CreateDebugUtilsMessengerEXT_result{CreateDebugUtilsMessengerEXT(instance, &DbgMessengerCreateInfo, nullptr, &DebugMessenger)};
        if (CreateDebugUtilsMessengerEXT_result != VK_SUCCESS) throw std::runtime_error(std::format("Failed to setup Debug Messenger, error: {}.", string_VkResult(CreateDebugUtilsMessengerEXT_result)));
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageLevel,
        VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT *CallbackData,
        void *UserData){
            std::string StrMessageType;
            switch (MessageType)
            {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                StrMessageType = "GeneralError";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                StrMessageType = "SpecificationError";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                StrMessageType = "MisuseVulkanApiError";
                break;
            
            default:
                StrMessageType = string_VkDebugUtilsMessageTypeFlagsEXT(MessageType);
                break;
            }
            switch (MessageLevel)
            {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                SPDLOG_DEBUG("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                SPDLOG_INFO("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                SPDLOG_WARN("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                SPDLOG_ERROR("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            }
            return VK_FALSE;
    }

    // Ext Functions load.

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
    {
        PFN_vkCreateDebugUtilsMessengerEXT _crtDbgUtMsgrEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        return (_crtDbgUtMsgrEXT == nullptr ? VK_ERROR_EXTENSION_NOT_PRESENT : _crtDbgUtMsgrEXT(instance, pCreateInfo, pAllocator, pMessenger));
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator){
        PFN_vkDestroyDebugUtilsMessengerEXT _dstrDbgUtMsgrEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (_dstrDbgUtMsgrEXT != nullptr) _dstrDbgUtMsgrEXT(instance, messenger, pAllocator);
    }

};

static void FramebufferResizeCallback(GLFWwindow *AppPointer, int width, int height)
{
    auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(AppPointer));
    // int a, b;
    // glfwGetFramebufferSize(AppPointer, &a, &b);
    // SPDLOG_CRITICAL("{} {} / {} {} / {} {}", app->WIDTH, app->HEIGHT, width, height, a, b);
    app->SetWindowSize(app->WIDTH, app->HEIGHT);
}
static void WindwoResizeCallback(GLFWwindow *AppPointer, int width, int height)
{
    auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(AppPointer));
    app->WIDTH = width;
    app->HEIGHT = height;
    app->SetWindowSize(width, height);

}
// static void WindowMaximizeCallback(GLFWwindow *AppPointer, int maximized)
// {
// }

