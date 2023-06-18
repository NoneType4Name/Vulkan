#define GLFW_INCLUDE_VULKAN
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <GLFW/glfw3.h>
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
#include <optional>

// #if defined( WIN32 )
// #include <windows.h>
// #elif defined ( __linux__ )
// #include <X11/Xlib.h>
// #endif

uint16_t DEFAULT_WIDTH{800};
uint16_t DEFAULT_HEIGHT{600};

class App
{
public:
    uint16_t WIDTH;
    uint16_t HEIGHT;
    int32_t DISPLAY_WIDTH;
    int32_t DISPLAY_HEIGHT;
    std::string TITLE;
    const std::vector<const char *> ValidationLayers{"VK_LAYER_KHRONOS_validation"};

    #ifdef NDEBUG
        const bool EnableValidationLayers = false;
    #else
        const bool EnableValidationLayers = true;
    #endif
    App(uint16_t width, uint16_t height, const char * title) : WIDTH{width}, HEIGHT{height}, TITLE{title}
    {
        run();
    }
    ~App()
    {
        if (EnableValidationLayers) DestroyDebugUtilsMessengerEXT(instance, DebugMessenger, nullptr);
        vkDestroyInstance(instance, NULL);
        vkDestroyDevice(LogicalDevice, NULL);
        glfwDestroyWindow(window);
    }

    void run()
    {
        initVulkan();
        initWindow();
        CheckValidationLayers();
        mainLoop();
    }

private:
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
    VkDevice LogicalDevice;
    VkQueue LogicalDeviceGraphickQueue;
    VkPhysicalDeviceProperties PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
    std::unordered_map<uint16_t, uint32_t> PhysicalDeviceIndices;
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GetScreenResolution(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        if (!WIDTH) WIDTH = DISPLAY_WIDTH;
        if (!HEIGHT) HEIGHT = DISPLAY_HEIGHT;
        window = glfwCreateWindow(WIDTH, HEIGHT, TITLE.data(), nullptr, nullptr);
        CentralizeWindow();
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
        // #if defined( WIN32 )
        // // RECT display;
        // // GetWindowRect(GetDesktopWindow(), &display);
        // width = GetSystemMetrics(SM_CXSCREEN);
        // height = GetSystemMetrics(SM_CYSCREEN);
        // #elif defined ( __linux__ )
        // // Display DefaultDisplay;
        // // XDefaultScreen(DefaultDisplayX);
        // // if ( !XOpenDisplay(DefaultDisplay) )
        // // {
        // //     spdlog::critical("Default display {} isn't aviliable, set default resolution: {}x{}", DefaultDisplay, DEFAULT_WIDTH, DEFAULT_HEIGHT);
        // //     width = DEFAULT_WIDTH;
        // //     height = DEFAULT_HEIGHT;
        // //     return;
        // // }
        // Display* disp = XOpenDisplay(NULL);
        // Screen*  scrn = DefaultScreenOfDisplay(disp); 
        // height = scrn->height;
        // width  = scrn->width;
        // // int32_t snum{DefaultScreen(dpy)};
        // // width = DisplayWidth(dpy, snum);
        // // height = DisplayHeight(dpy, snum);
        // #endif
    }

    void initVulkan()
    {
        CreateVKinstance();
        SetupDebugMessenger();
        GetPhysicalDevice();
        CreateLogicalDevice();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
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
            SPDLOG_CRITICAL("Vulkan not find physical device.", CountPhysicalDevices);
            throw std::runtime_error("Failed to find Physical Device.");
        }
        std::vector<VkPhysicalDevice>PhysicalDevices{CountPhysicalDevices};
        vkEnumeratePhysicalDevices(instance, &CountPhysicalDevices, PhysicalDevices.data());
        // if (!CountPhysicalDevices-1) PhysicalDevice = PhysicalDevices[0];
        for (const auto& device : PhysicalDevices)
        {
            if (isDeviceSuitable(device)) break;
            
            // return dProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && dFeatures.geometryShader;
        }
        if (PhysicalDevice == VK_NULL_HANDLE){
            PhysicalDeviceProperties = {};
            PhysicalDeviceFeatures = {};
            SPDLOG_CRITICAL("Instance haven't suitable Physical Devices.");
            throw std::runtime_error("Failed to find suitable Physical Devices.");
        }
        SPDLOG_INFO("Selected {}.", PhysicalDeviceProperties.deviceName);
    }

    void CreateLogicalDevice()
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = PhysicalDeviceIndices[VK_QUEUE_GRAPHICS_BIT];
        queueCreateInfo.queueCount = 1;
        float priority[]{1.0f};
        queueCreateInfo.pQueuePriorities = priority;
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;
        if (EnableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
            createInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else createInfo.enabledLayerCount = 0;
        // extension

        VkResult DeviceCreateCode = vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &LogicalDevice);
        if (DeviceCreateCode != VK_SUCCESS) throw std::runtime_error(std::format("Failed create logic device, error code: {}.", string_VkResult(DeviceCreateCode)));
        vkGetDeviceQueue(LogicalDevice, PhysicalDeviceIndices[VK_QUEUE_GRAPHICS_BIT], 0, &LogicalDeviceGraphickQueue);
        

    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        vkGetPhysicalDeviceProperties(device, &PhysicalDeviceProperties);
        vkGetPhysicalDeviceFeatures(device, &PhysicalDeviceFeatures);
        auto indices{findQueueFamilies(device)};
        if (indices.find(VK_QUEUE_GRAPHICS_BIT) == indices.end()){SPDLOG_CRITICAL("Graphic family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;};
        if (indices.find(VK_QUEUE_COMPUTE_BIT) == indices.end()) {SPDLOG_CRITICAL("Compute family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;};
        if (indices.find(VK_QUEUE_TRANSFER_BIT) == indices.end()) {SPDLOG_CRITICAL("Transfer family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        if (indices.find(VK_QUEUE_SPARSE_BINDING_BIT) == indices.end()) {SPDLOG_CRITICAL("Sparse binding family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        // if (indices.find(VK_QUEUE_PROTECTED_BIT) == indices.end()) {SPDLOG_CRITICAL("Protect memory family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        if (indices.find(VK_QUEUE_VIDEO_DECODE_BIT_KHR) == indices.end()) {SPDLOG_CRITICAL("Video decode family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        // if (indices.find(VK_QUEUE_VIDEO_ENCODE_BIT_KHR) == indices.end()) {SPDLOG_CRITICAL("Video encode family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        // if (indices.find(VK_QUEUE_OPTICAL_FLOW_BIT_NV) == indices.end()) {SPDLOG_CRITICAL("Optical Flow family queue isn't support by {}.", PhysicalDeviceProperties.deviceName); return false;}
        PhysicalDevice = device;
        PhysicalDeviceIndices = indices;
        return true;
    }
    // struct QueueFamilyIndices {
    //     std::optional<uint32_t> graphicsFamily;
    //     std::optional<uint32_t> computeFamily;
    //     std::optional<uint32_t> transferFamily;
    //     std::optional<uint32_t> sparseBindingFamily;
    //     // std::optional<uint32_t> protectMemoryFamily;
    //     std::optional<uint32_t> videoDecodeFamily;
    //     // std::optional<uint32_t> videoEncodeFamily;
    //     // std::optional<uint32_t> opticalFlowFamily;
    // };
    std::unordered_map<uint16_t, uint32_t> findQueueFamilies(VkPhysicalDevice device) {
        std::unordered_map<uint16_t, uint32_t>indices;
        uint32_t queueCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties>QueueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, QueueFamilies.data());
        uint32_t index{0};
        for (const auto &queueF : QueueFamilies)
        {
            if (queueF.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices[VK_QUEUE_GRAPHICS_BIT] = index++;
            if (queueF.queueFlags & VK_QUEUE_COMPUTE_BIT) indices[VK_QUEUE_COMPUTE_BIT] = index++;
            if (queueF.queueFlags & VK_QUEUE_TRANSFER_BIT) indices[VK_QUEUE_TRANSFER_BIT] = index++;
            if (queueF.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) indices[VK_QUEUE_SPARSE_BINDING_BIT] = index++;
            if (queueF.queueFlags & VK_QUEUE_PROTECTED_BIT) indices[VK_QUEUE_PROTECTED_BIT] = index++;
            if (queueF.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) indices[VK_QUEUE_VIDEO_DECODE_BIT_KHR] = index++;
            // if (queueF.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) indices[VK_QUEUE_VIDEO_ENCODE_BIT_KHR] = index++;
            if (queueF.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) indices[VK_QUEUE_OPTICAL_FLOW_BIT_NV] = index++;
        }


        return indices;
    }
    
    void CreateVKinstance()
    {
        if (EnableValidationLayers && !CheckValidationLayers())
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

        if (EnableValidationLayers)
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
        if (!EnableValidationLayers) return;
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

