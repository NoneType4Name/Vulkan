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

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vk_enum_string_helper.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <format>
#include <string>
#include <map>
#include <set>
#include <optional>
#include <limits>


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
        for (const auto &imView : SwapchainImgsView) vkDestroyImageView(LogicalDevice, imView, nullptr);
        vkDestroySwapchainKHR(LogicalDevice, SwapChain, nullptr);
        vkDestroyDevice(LogicalDevice, NULL);
        vkDestroySurfaceKHR(instance, Surface, nullptr);
        if (APP_DEBUG) DestroyDebugUtilsMessengerEXT(instance, DebugMessenger, nullptr);
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
    }

    void CentralizeWindow()
    {
        glfwSetWindowPos(window, (DISPLAY_WIDTH/2)-(WIDTH/2), (DISPLAY_HEIGHT/2)-(HEIGHT/2));
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
    const std::vector<const char *> ValidationLayers{"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> RequeredDeviceExts{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDevice LogicalDevice;
    VkQueue LogicalDeviceGraphickQueue;
    VkQueue LogicalDevicePresentQueue;
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
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT DbgMessengerCreateInfo{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        NULL,
        NULL,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        DebugCallback,
        this};
    VkSurfaceKHR Surface;

    void initWindow()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GetScreenResolution(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        if (!WIDTH) WIDTH = DISPLAY_WIDTH;
        if (!HEIGHT) HEIGHT = DISPLAY_HEIGHT;
        window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.data(), nullptr, nullptr);
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
    }

    void mainLoop()
    {
        // while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }
    // Support Functios
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
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width = WIDTH;
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height = HEIGHT;
        uint32_t imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.minImageCount + 1;
        if (PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount > 0 && imageCount > PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount) {
            imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount;
        }
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
