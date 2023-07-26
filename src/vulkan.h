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
    uint32_t a{ 1 };
#if defined( _WIN32 )
    VulkanInstance( HWND hwnd, HINSTANCE instance, LoggerCallbacks LoggerCallback ) : Loggers{ LoggerCallback }
    {
#endif
#if defined( __LINUX__ )
        VulkanInstance( Display dpy, Window window, LoggerCallbacks LoggerCallback ) : Loggers{ LoggerCallback }
        {
#endif
            uint32_t _c;
            vkEnumerateInstanceLayerProperties( &_c, nullptr );
            VkLayerProperties *AviableLayers = new VkLayerProperties[ _c ];
            vkEnumerateInstanceLayerProperties( &_c, AviableLayers );
            // for( const char *lName : ValidationLayers )
            // {
            //     bool SupportLayer = false;
            //     for( const auto &AlName : AviableLayers )
            //     {
            //         if( !strcmp( lName, AlName.layerName ) )
            //         {
            //             SupportLayer = true;
            //             break;
            //         }
            //     }
            //     if( !SupportLayer )
            //     {
            //         SPDLOG_CRITICAL( "Validation layer {} isn't support.", lName );
            //         return false;
            //     }
            // }
            // return true;
#if defined( _WIN32 )
            VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo{
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                nullptr,
                0,
                instance,
                hwnd };
            // Result = vkCreateWin32SurfaceKHR( Instance, &Win32SurfaceCreateInfo, nullptr, &Screen );
#endif
#if defined( __LINUX__ )

            VkXlibSurfaceCreateInfoKHR XlibSurfaceCreateInfoKHR{
                VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                nullptr,
                0,
                &dpy,
                &window };
            VkResult Result{ vkCreateXlibSurfaceKHR( Instance, &XlibSurfaceCreateInfoKHR, nullptr, &Screen ) };
#endif

            // if( Result != VK_SUCCESS )
            // {
            //     Loggers.critical( "Failed to Create Win32 Surface, error" );
            // }
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
    };
