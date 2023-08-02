#include <vulkan/vulkan_core.h>
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#if defined( _WIN32 )
#    define VK_USE_PLATFORM_WIN32_KHR
#    define GLFW_EXPOSE_NATIVE_WIN32
#elif defined( __LINUX__ )
#    define VK_USE_PLATFORM_X11_KHR
#    define GLFW_EXPOSE_NATIVE_X11
#endif
#include <stb_image.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <array>
#include <vector>
#include <format>

typedef void ( *LoggerCallback )( const char *data );

typedef struct LoggerCallbacks
{
    LoggerCallback trace;
    LoggerCallback debug;
    LoggerCallback info;
    LoggerCallback warn;
    LoggerCallback error;
    LoggerCallback critical;
} LoggerCallbacks;

struct SwapChainProperties
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Format;
    std::vector<VkPresentModeKHR> PresentModes;
};

struct SwapChain
{
    VkSwapchainKHR Swapchain;
    VkSurfaceCapabilitiesKHR Capabilitie;
    VkFormat Format;
    VkPresentModeKHR PresentMode;
};

struct Vertex
{
    glm::vec3 coordinate;
    glm::vec4 color;
    glm::vec2 texture;
    static VkVertexInputBindingDescription InputBindingDescription()
    {
        VkVertexInputBindingDescription VertexInputBindingDescription{};
        VertexInputBindingDescription.binding   = 0;
        VertexInputBindingDescription.stride    = sizeof( Vertex );
        VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return VertexInputBindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> InputAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 3> VertexInputAttributeDescription{};
        VertexInputAttributeDescription[ 0 ].binding  = 0;
        VertexInputAttributeDescription[ 0 ].location = 0;
        VertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
        VertexInputAttributeDescription[ 0 ].offset   = offsetof( Vertex, coordinate );

        VertexInputAttributeDescription[ 1 ].binding  = 0;
        VertexInputAttributeDescription[ 1 ].location = 1;
        VertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        VertexInputAttributeDescription[ 1 ].offset   = offsetof( Vertex, color );

        VertexInputAttributeDescription[ 2 ].binding  = 0;
        VertexInputAttributeDescription[ 2 ].location = 2;
        VertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32_SFLOAT;
        VertexInputAttributeDescription[ 2 ].offset   = offsetof( Vertex, texture );
        return VertexInputAttributeDescription;
    }
    bool operator==( const Vertex &other ) const
    {
        return coordinate == other.coordinate && color == other.color && texture == other.texture;
    }
};

struct Model
{
    std::vector<Vertex> ModelVertecies;
    std::vector<uint32_t> ModelVerteciesIndices;
    uint32_t VerteciesOffset{};
    uint32_t IndeciesOffset{};
};

struct DemensionsUniformrObject
{
    alignas( 16 ) glm::mat4 model;
    alignas( 16 ) glm::mat4 view;
    alignas( 16 ) glm::mat4 proj;
};

namespace std
{
template <>
struct hash<Vertex>
{
    size_t operator()( Vertex const &vertex ) const
    {
        return ( ( hash<glm::vec3>()( vertex.coordinate ) ^
                   ( hash<glm::vec3>()( vertex.color ) << 1 ) ) >>
                 1 ) ^
               ( hash<glm::vec2>()( vertex.texture ) << 1 );
    }
};
} // namespace std

class VulkanInstance
{
  public:
#if defined( _WIN32 )
    VulkanInstance( const char *AppName, uint32_t AppVersion, HWND hwnd, HINSTANCE instance, LoggerCallbacks LoggerCallback ) : Loggers{ LoggerCallback }
    {
#endif
#if defined( __LINUX__ )
        VulkanInstance( const char *AppName, uint32_t AppVersion, Display dpy, Window window, LoggerCallbacks LoggerCallback ) : Loggers{ LoggerCallback }
        {
#endif
            Loggers.info( "Initialize Vulkan.h::VulkanInstance class." );
            VkInstanceCreateInfo InstanceCreateInfo{};
            InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            uint32_t glfwExtensionsCount{ 0 };
            const char **glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionsCount );
            std::vector<const char *> glfwExtensionsVector( glfwExtensions, glfwExtensions + glfwExtensionsCount );

#ifdef _DEBUG
            uint32_t _c;
            vkEnumerateInstanceLayerProperties( &_c, nullptr );
            VkLayerProperties *AviableLayers = new VkLayerProperties[ _c ];
            vkEnumerateInstanceLayerProperties( &_c, AviableLayers );
            size_t c{ sizeof( ValidationLayers ) / sizeof( ValidationLayers[ 0 ] ) };
            std::vector<const char *> NotAvilableLayers{ c };
            memcpy( &NotAvilableLayers[ 0 ], ValidationLayers, sizeof( ValidationLayers ) );
            for( size_t i{ 0 }; i < c; i++ )
            {
                for( uint32_t _i{ 0 }; _i < _c; _i++ )
                    if( !strcmp( NotAvilableLayers[ c - 1 - i ], AviableLayers[ _i ].layerName ) )
                    {
                        NotAvilableLayers.erase( NotAvilableLayers.end() - i - 1 );
                        break;
                    }
                if( NotAvilableLayers.empty() )
                    break;
            }
            if( !NotAvilableLayers.empty() )
            {
                std::string Err = std::format( "Not finded validation layers: {}:\n", std::to_string( NotAvilableLayers.size() ) );
                for( const auto VL : NotAvilableLayers )
                {
                    Err += std::format( "\t {}\n", VL );
                }
                LoggerCallback.critical( Err.c_str() );
            }

            VkValidationFeatureEnableEXT enabled[]{ VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
            VkValidationFeaturesEXT ValidationFeatures{};
            ValidationFeatures.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            ValidationFeatures.enabledValidationFeatureCount = sizeof( enabled ) / sizeof( enabled[ 0 ] );
            ValidationFeatures.pEnabledValidationFeatures    = enabled;

            VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfoEXT{};
            InstanceCreateInfo.pNext                         = &DebugUtilsMessengerCreateInfoEXT;
            DebugUtilsMessengerCreateInfoEXT.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            DebugUtilsMessengerCreateInfoEXT.pNext           = &ValidationFeatures;
            DebugUtilsMessengerCreateInfoEXT.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            DebugUtilsMessengerCreateInfoEXT.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            DebugUtilsMessengerCreateInfoEXT.pfnUserCallback = DebugCallback;
            DebugUtilsMessengerCreateInfoEXT.pUserData       = this;
            glfwExtensionsVector.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
            InstanceCreateInfo.enabledLayerCount   = sizeof( ValidationLayers ) / sizeof( ValidationLayers[ 0 ] );
            InstanceCreateInfo.ppEnabledLayerNames = ValidationLayers;

#endif
            InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>( glfwExtensionsVector.size() );
            InstanceCreateInfo.ppEnabledExtensionNames = glfwExtensionsVector.data();
            VkApplicationInfo ApplicationInfo{};
            InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
            ApplicationInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            ApplicationInfo.engineVersion       = 0;
            ApplicationInfo.apiVersion          = VK_API_VERSION_1_0;
            ApplicationInfo.pApplicationName    = AppName;
            ApplicationInfo.applicationVersion  = AppVersion;
            VkResult Result{ vkCreateInstance( &InstanceCreateInfo, nullptr, &Instance ) };

#if defined( _WIN32 )
            VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo{
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                nullptr,
                0,
                instance,
                hwnd };
            Result = vkCreateWin32SurfaceKHR( Instance, &Win32SurfaceCreateInfo, nullptr, &Screen );
#endif
#if defined( __LINUX__ )

            VkXlibSurfaceCreateInfoKHR XlibSurfaceCreateInfoKHR{
                VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                nullptr,
                0,
                &dpy,
                &window };
            Result = vkCreateXlibSurfaceKHR( Instance, &XlibSurfaceCreateInfoKHR, nullptr, &Screen )
        };
#endif

        if( Result != VK_SUCCESS )
        {
            Loggers.critical( "Failed to Create Surface, error" );
        }
    }

  private:
    // Custom
    const char *ValidationLayers[ 1 ] = { "VK_LAYER_KHRONOS_validation" };
    // System
    LoggerCallbacks Loggers;
    VkSurfaceKHR Screen;
    VkInstance Instance;
    VkDevice LogicalDevice;
    VkPhysicalDevice PhysicalDevice;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageLevel,
        VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT *CallbackData,
        void *UserData )
    {
        VulkanInstance *Vulkan = static_cast<VulkanInstance *>( UserData );
        std::string StrMessageType;
        switch( MessageType )
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
                StrMessageType = string_VkDebugUtilsMessageTypeFlagsEXT( MessageType );
                break;
        }
        switch( static_cast<uint32_t>( MessageLevel ) )
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                Vulkan->Loggers.debug( std::format( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage ).c_str() );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                Vulkan->Loggers.info( std::format( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage ).c_str() );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                Vulkan->Loggers.warn( std::format( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage ).c_str() );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                Vulkan->Loggers.critical( std::format( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage ).c_str() );
                break;
        }
        return VK_FALSE;
    }
};
