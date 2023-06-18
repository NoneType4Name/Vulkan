#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <format>
#include <string>
#include <map>
#include <optional>
// #include <stdlib.h>

namespace
{
    static void GLFWerrorCallback(int code, const char *data)
    {
        spdlog::critical("GLFW ERROR {}: {}", code, data);
    }
    struct GLFWinit
    {
        GLFWinit()
        {
            spdlog::set_level(spdlog::level::trace);
            spdlog::debug("--- Start logging. ---");
            int32_t code{glfwInit()};
            if ( code==GLFW_TRUE )
            {
                spdlog::debug("GLFW{} inititialized", glfwGetVersionString());
                glfwSetErrorCallback( GLFWerrorCallback );
                exit(-1);
            }
            else
            {
                spdlog::critical("GLFW not initialized, errno{}", code);
            }
        }
    };
}

class App
{
public:
    const uint16_t WIDTH;
    const uint16_t HEIGHT;
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
        cleanup();
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
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        CreateVKinstance();
        SetupDebugMessenger();
        GetPhysicalDevice();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {   
        if (EnableValidationLayers) DestroyDebugUtilsMessengerEXT(instance, DebugMessenger, nullptr);
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // Support Functios
    void GetPhysicalDevice()
    {
        uint32_t CountPhysicalDevices{0};
        vkEnumeratePhysicalDevices(instance, &CountPhysicalDevices, nullptr);
        if (!CountPhysicalDevices)
        {
            spdlog::critical("Vulkan find {} Physical Devices.", CountPhysicalDevices);
            throw std::runtime_error("Failed to find Physical Devices.");
        }
        std::vector<VkPhysicalDevice>PhysicalDevices{CountPhysicalDevices};
        vkEnumeratePhysicalDevices(instance, &CountPhysicalDevices, PhysicalDevices.data());
        for (const auto& device : PhysicalDevices)
        {
            // if (isDeviceSuitable(device))
            // {
            //     PhysicalDevice = device;
            //     break;
            // }
            VkPhysicalDeviceProperties dProperties;
            VkPhysicalDeviceFeatures dFeatures;
            vkGetPhysicalDeviceProperties(device, &dProperties);
            vkGetPhysicalDeviceFeatures(device, &dFeatures);
            // return dProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && dFeatures.geometryShader;
        }
        if (PhysicalDevice == VK_NULL_HANDLE){
            spdlog::critical("Instance haven't suitable Physical Devices.");
            throw std::runtime_error("Failed to find suitable Physical Devices.");
        }
    }

    // bool isDeviceSuitable(VkPhysicalDevice device)
    // {
        
    // }
    
    void CreateVKinstance()
    {
        if (EnableValidationLayers && !CheckValidationLayers())
            throw std::runtime_error("Unavilable validation layer."); // VLayers

        uint32_t glfwExtensionsCount{0};
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        // Additional funcs (may be include it from H file?).
        // as func name: void ChekForSupportAllExtensions.
        uint32_t glfwAvailableExtensionsCount{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> AvailableExtensions(glfwAvailableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, AvailableExtensions.data());
        // Extension is Available?.

        //
        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = "Vulkan start";
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
                std::cout << "Validation layer " << lName << " isn't support.";
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
                spdlog::debug("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                spdlog::info("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                spdlog::warn("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                spdlog::error("{}message: {}", (MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "":format("Type: {}, ", StrMessageType)), CallbackData->pMessage);
                break;
            }
            // cerr << "Validation layer get " << StrMessageType << ": " << CallbackData->pMessage << endl;
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

int main(int argc, char *argv[])
{    
    try
    {
        App app{1920, 1080, "HV"};
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        spdlog::critical("{}\n Exit with error code {}.", e.what(), EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    spdlog::info("Exit with code {}.", EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

// test for map from py:

// auto some_function = [](auto&& x) { cout << x << endl;};
// auto the_tuple = make_tuple(1, 2.1, '3', "four");
// apply([&](auto& ...x){(..., some_function(x));}, the_tuple);
