#pragma once
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#if defined( WIN32 )
#    define VK_USE_PLATFORM_WIN32_KHR
#    define GLFW_EXPOSE_NATIVE_WIN32
#elif defined( __LINUX__ )
#    define VK_USE_PLATFORM_X11_KHR
#    define GLFW_EXPOSE_NATIVE_X11
#endif

#ifdef _DEBUG
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#    define PATH_PREFIX         "../"
const bool APP_DEBUG = true;
#else
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
#    define PATH_PREFIX         "./"

const bool APP_DEBUG = false;

#endif
#include <map>
#include <set>
#include <limits>
#include <vector>
#include <array>
#include <format>
#include <chrono>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#include <GLFW/glfw3native.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>

namespace
{
uint16_t DEFAULT_WIDTH{ 800 };
uint16_t DEFAULT_HEIGHT{ 600 };
static void GLFWerrorCallback( int code, const char *data )
{
    SPDLOG_CRITICAL( "GLFW ERROR {}: {}", code, data );
}
struct _initialize
{
    _initialize()
    {
        try
        {
            spdlog::set_level( APP_DEBUG ? spdlog::level::trace : spdlog::level::critical );
            spdlog::set_pattern( "[func %!] [%s:%#] [%H:%M:%S.%e] [%^%l%$] %v" );
            SPDLOG_DEBUG( "--- Start logging. ---" );
        }
        catch( const std::exception &logInitE )
        {
            std::cerr << "Logger initialize error:\t" << logInitE.what() << '\n.';
        }

        // exit(EXIT_FAILURE);

        if( glfwInit() )
        {
            SPDLOG_DEBUG( "GLFW{} inititialized.", glfwGetVersionString() );
            glfwSetErrorCallback( GLFWerrorCallback );
        }
        else
        {
            SPDLOG_CRITICAL( "GLFW not initialized." );
            SPDLOG_INFO( "Exit with code {}.", EXIT_FAILURE );
            exit( EXIT_FAILURE );
        }
    }
    ~_initialize()
    {
        glfwTerminate();
        SPDLOG_DEBUG( "App closed." );
        SPDLOG_DEBUG( "--- Log finish. ---" );
        spdlog::shutdown();
    }
} _;
} // namespace

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
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
struct UniformBufferObject
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
        return ( ( hash<glm::vec3>()( vertex.coordinate ) ^ ( hash<glm::vec3>()( vertex.color ) << 1 ) ) >> 1 );
    }
};
} // namespace std
struct Model
{
    std::vector<Vertex> ModelVertecies;
    std::vector<uint32_t> ModelVerteciesIndices;
    // VkBufferCopy VerticesBufferCopy{};
    // VkBufferCopy VertexIndeciesBufferCopy{};
    uint32_t VerteciesOffset{};
    uint32_t IndeciesOffset{};
};

static void FramebufferResizeCallback( GLFWwindow *, int, int );
static void WindwoResizeCallback( GLFWwindow *, int, int );

class App
{
  public:
    uint16_t WIDTH;
    uint16_t HEIGHT;
    int32_t DISPLAY_WIDTH;
    int32_t DISPLAY_HEIGHT;
    std::string TITLE;

    App( uint16_t width, uint16_t height, const char *title, std::vector<std::pair<const char *, const char *>> &Models ) : WIDTH{ width }, HEIGHT{ height }, TITLE{ title }, PathsToModel{ Models }
    {
        initWindow();
        initVulkan();
        CheckValidationLayers();
        mainLoop();
    }
    ~App()
    {
        DestroySwapchain();
        vkDestroyDescriptorPool( LogicalDevice, DescriptorPool, nullptr );
        vkDestroyDescriptorSetLayout( LogicalDevice, DescriptorsSetLayout, nullptr );
        vkDestroySampler( LogicalDevice, TextureSampler, nullptr );
        vkDestroyImageView( LogicalDevice, TextureImageView, nullptr );
        vkDestroyImage( LogicalDevice, TextureImage, nullptr );
        vkFreeMemory( LogicalDevice, TextureImageMemory, nullptr );
        vkDestroyBuffer( LogicalDevice, VertexBuffer, nullptr );
        vkFreeMemory( LogicalDevice, VertexBufferMemory, nullptr );
        vkDestroyBuffer( LogicalDevice, VertexIndecesBuffer, nullptr );
        vkFreeMemory( LogicalDevice, VertexIndecesBufferMemory, nullptr );
        vkDestroyCommandPool( LogicalDevice, CommandPool, nullptr );
        for( uint8_t i{ 0 }; i < _MaxFramesInFlight; i++ )
        {
            vkDestroySemaphore( LogicalDevice, ImageAvailableSemaphores[ i ], nullptr );
            vkDestroySemaphore( LogicalDevice, RenderFinishedSemaphores[ i ], nullptr );
            vkDestroyFence( LogicalDevice, WaitFrames[ i ], nullptr );
        }
        vkDestroyDevice( LogicalDevice, nullptr );
        vkDestroySurfaceKHR( instance, Surface, nullptr );
        if( APP_DEBUG ) DestroyDebugUtilsMessengerEXT( instance, DebugMessenger, nullptr );
        vkDestroyInstance( instance, nullptr );
        glfwDestroyWindow( window );
    }

    void CentralizeWindow()
    {
        glfwSetWindowPos( window, ( DISPLAY_WIDTH / 2 ) - ( WIDTH / 2 ), ( DISPLAY_HEIGHT / 2 ) - ( HEIGHT / 2 ) );
    }

    void SetWindowSize( uint32_t width, uint32_t height )
    {
        if( WIDTH != width || HEIGHT != height )
        {
            WIDTH  = width;
            HEIGHT = height;
            glfwSetWindowSize( window, WIDTH, HEIGHT );
        }
        ReCreateSwapChain();
    }

    void GetScreenResolution( int32_t &width, int32_t &height )
    {
        GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
        glfwGetVideoMode( Monitor );
        const GLFWvidmode *mode = glfwGetVideoMode( Monitor );
        width                   = mode->width;
        height                  = mode->height;
    }

  private:
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR Surface;
    const std::vector<const char *> ValidationLayers{ "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char *> RequeredDeviceExts{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME };
    std::vector<std::pair<const char *, const char *>> &PathsToModel;
    VkDevice LogicalDevice;
    VkQueue LogicalDeviceGraphickQueue;
    VkQueue LogicalDevicePresentQueue;
    VkQueue LogicalDeviceTransferQueue;
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStage;
    VkPipelineLayout PipelineLayout;
    VkDescriptorSetLayout DescriptorsSetLayout;
    VkRenderPass RenderPass;
    VkPipeline GraphicsPipeline;
    SwapChainProperties PhysiacalDeviceSwapchainProperties;
    VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
    VkPhysicalDeviceProperties PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
    QueueFamilyIndices PhysicalDeviceIndices;
    VkPhysicalDeviceFeatures LogicalDeviceFeatures{};
    VkSwapchainKHR SwapChain;
    VkFormat SwapchainFormat;
    VkPresentModeKHR SwapchainPresentMode;
    std::vector<VkImage> SwapchainImgs;
    std::vector<VkImageView> SwapchainImgsView;
    std::vector<VkFramebuffer> SwapchainFrameBuffers;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer VertexIndecesBuffer;
    VkDeviceMemory VertexIndecesBufferMemory;
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory; // TODO: vector off this and previous line
    VkImageView TextureImageView;      // TODO: texture struct
    VkSampler TextureSampler;
    std::vector<VkBuffer> UniformBuffers{};
    std::vector<VkDeviceMemory> UniformBuffersMemory{};
    VkDescriptorPool DescriptorPool;
    std::vector<VkDescriptorSet> DescriptorSets;
    VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> WaitFrames;
    uint8_t _CurrentFrame{ 0 };
    uint8_t _MaxFramesInFlight{ 1 };
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT DbgMessengerCreateInfo{};
    std::unordered_map<std::string, Model> Models;
    std::vector<VkDeviceSize> VerteciesOffSets{};

    void initWindow()
    {
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        GetScreenResolution( DISPLAY_WIDTH, DISPLAY_HEIGHT );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );
        if( !( WIDTH | HEIGHT ) )
        {
            glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
            glfwWindowHint( GLFW_DECORATED, GLFW_FALSE );
        }

        if( !WIDTH ) WIDTH = DISPLAY_WIDTH;
        if( !HEIGHT ) HEIGHT = DISPLAY_HEIGHT;
        window = glfwCreateWindow( WIDTH, HEIGHT, TITLE.data(), nullptr, nullptr );
        glfwSetWindowUserPointer( window, this );
        glfwSetFramebufferSizeCallback( window, FramebufferResizeCallback );
        glfwSetWindowSizeCallback( window, WindwoResizeCallback );
        // glfwSetWindowMaximizeCallback(window, WindowMaximizeCallback);
        CentralizeWindow();
    }

    void initVulkan()
    {
        CreateVKinstance();
        SetupDebugMessenger();
#if defined( WIN32 )
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd      = glfwGetWin32Window( window );
        createInfo.hinstance = GetModuleHandle( nullptr );
        VkResult Result{ vkCreateWin32SurfaceKHR( instance, &createInfo, nullptr, &Surface ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "WIN32 surface wasn't be created, error: {}", string_VkResult( Result ) ) );
        }
#elif defined( __LINUX__ )
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd      = glfwGetX11Window( window );
        createInfo.hinstance = GetModuleHandle( nullptr );
        VkResult Result{ vkCreateXcbSurfaceKHR( instance, &createInfo, nullptr, &Surface ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Linux surface wasn't be created, error: {}", string_VkResult( Result ) ) );
        }
#endif
        GetPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateDescriptionSetLayout();
        CreateGraphicsPipelines();
        CreateFrameBuffers();
        CreateCommandPool();
        CreateTextureImage();
        CreateTextureImageView();
        CreateTextureSempler();
        CreateVertexBuffer();
        CreateUniformBuffers();
        CreateDescriptorsPool();
        CreateDescriptorSets();
        CreateCommandBuffers();
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo FenceInfo{};
        FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for( uint8_t i{ 0 }; i < _MaxFramesInFlight; i++ )
        {
            ImageAvailableSemaphores.resize( _MaxFramesInFlight );
            RenderFinishedSemaphores.resize( _MaxFramesInFlight );
            WaitFrames.resize( _MaxFramesInFlight );
            Result = vkCreateSemaphore( LogicalDevice, &semaphoreInfo, nullptr, &ImageAvailableSemaphores[ i ] );
            if( Result != VK_SUCCESS )
            {
                _CriticalThrow( std::format( "Failed to create Semaphore, error: {}.", string_VkResult( Result ) ) );
            }
            Result = vkCreateSemaphore( LogicalDevice, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[ i ] );
            if( Result != VK_SUCCESS )
            {
                _CriticalThrow( std::format( "Failed to create Semaphore, error: {}.", string_VkResult( Result ) ) );
            }
            Result = vkCreateFence( LogicalDevice, &FenceInfo, nullptr, &WaitFrames[ i ] );
            if( Result != VK_SUCCESS )
            {
                _CriticalThrow( std::format( "Failed to create Semaphore, error: {}.", string_VkResult( Result ) ) );
            }
        }
    }

    void mainLoop()
    {
        while( !( glfwWindowShouldClose( window ) || glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) )
        {
            glfwPollEvents();
            drawFrame();
        }
    }

    void drawFrame()
    {
        uint32_t imgI;
        vkWaitForFences( LogicalDevice, 1, &WaitFrames[ _CurrentFrame ], VK_TRUE, UINT64_MAX );
        switch( vkAcquireNextImageKHR( LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphores[ _CurrentFrame ], nullptr, &imgI ) )
        {
            case VK_SUCCESS:
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
                ReCreateSwapChain();
                return;
            default:
                _CriticalThrow( "Failed to Acqueire image from swapchain." );
        }
        vkResetFences( LogicalDevice, 1, &WaitFrames[ _CurrentFrame ] );
        vkResetCommandBuffer( CommandBuffers[ _CurrentFrame ], /*VkCommandBufferResetFlagBits*/ 0 );
        RecordCommandBuffer( CommandBuffers[ _CurrentFrame ], imgI );
        UpdateUniformBuffer( imgI );
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore ImageSemaphore[]      = { ImageAvailableSemaphores[ _CurrentFrame ] };
        VkSemaphore RenderSemaphore[]     = { RenderFinishedSemaphores[ _CurrentFrame ] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = ImageSemaphore;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &CommandBuffers[ _CurrentFrame ];
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = RenderSemaphore;
        VkResult Result{ vkQueueSubmit( LogicalDeviceGraphickQueue, 1, &submitInfo, WaitFrames[ _CurrentFrame ] ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Submit draw frame, error: {}.", string_VkResult( Result ) ) );
        }
        VkPresentInfoKHR PresentInfo{};
        PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores    = RenderSemaphore;
        VkSwapchainKHR swapChains[]    = { SwapChain };
        PresentInfo.swapchainCount     = 1;
        PresentInfo.pSwapchains        = swapChains;
        PresentInfo.pImageIndices      = &imgI;
        vkQueuePresentKHR( LogicalDeviceGraphickQueue, &PresentInfo );
        _CurrentFrame = ++_CurrentFrame % _MaxFramesInFlight;
    }

    // Support Functios
    void UpdateUniformBuffer( uint32_t imgI )
    {
        // TEST
        static auto time{ std::chrono::high_resolution_clock::now() };
        UniformBufferObject UBO{};
        auto cTime = std::chrono::high_resolution_clock::now();
        float delta{ std::chrono::duration<float, std::chrono::seconds::period>( cTime - time ).count() };
        UBO.model = glm::rotate( glm::mat4( 1.f ), glm::radians( 90.f ) * delta, glm::vec3( .0f, 1.0f, .0f ) );
        UBO.view  = glm::lookAt( glm::vec3( .0f, .0f, 2.0f ), glm::vec3( .0f ), glm::vec3( .0f, 1.0f, .0f ) ); // Y = -Y
        UBO.proj  = glm::perspective( glm::radians( 50.f ), PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width / static_cast<float>( PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height ), .0f, 2.f );
        void *data;
        vkMapMemory( LogicalDevice, UniformBuffersMemory[ imgI ], 0, sizeof( UBO ), 0, &data );
        memcpy( data, &UBO, sizeof( UBO ) );
        vkUnmapMemory( LogicalDevice, UniformBuffersMemory[ imgI ] );
    }

    void DestroySwapchain()
    {
        vkDeviceWaitIdle( LogicalDevice );
        for( size_t i{ 0 }; i < SwapchainImgsView.size(); i++ )
        {
            vkDestroyBuffer( LogicalDevice, UniformBuffers[ i ], nullptr );
            vkFreeMemory( LogicalDevice, UniformBuffersMemory[ i ], nullptr );
            vkDestroyFramebuffer( LogicalDevice, SwapchainFrameBuffers[ i ], nullptr );
            vkDestroyImageView( LogicalDevice, SwapchainImgsView[ i ], nullptr );
        }
        for( const auto &shaderS : ShaderStage ) vkDestroyShaderModule( LogicalDevice, shaderS.module, nullptr );
        vkFreeCommandBuffers( LogicalDevice, CommandPool, static_cast<uint32_t>( CommandBuffers.size() ), CommandBuffers.data() );
        vkDestroyPipelineLayout( LogicalDevice, PipelineLayout, nullptr );
        vkDestroyRenderPass( LogicalDevice, RenderPass, nullptr );
        vkDestroyPipeline( LogicalDevice, GraphicsPipeline, nullptr );
        vkDestroySwapchainKHR( LogicalDevice, SwapChain, nullptr );
    }

    void GetPhysicalDevice()
    {
        uint32_t CountPhysicalDevices{ 0 };
        vkEnumeratePhysicalDevices( instance, &CountPhysicalDevices, nullptr );
        if( !CountPhysicalDevices )
        {
            _CriticalThrow( "Failed to find Physical Device." );
        }
        std::vector<VkPhysicalDevice> PhysicalDevices{ CountPhysicalDevices };
        vkEnumeratePhysicalDevices( instance, &CountPhysicalDevices, PhysicalDevices.data() );
        // if (!CountPhysicalDevices-1) PhysicalDevice = PhysicalDevices[0];
        for( const auto &device : PhysicalDevices )
        {
            if( isDeviceSuitable( device ) ) break;
        }
        if( PhysicalDevice == VK_NULL_HANDLE )
        {
            PhysicalDeviceProperties = {};
            PhysicalDeviceFeatures   = {};
            _CriticalThrow( "Failed to find suitable Physical Devices." );
        }
        SPDLOG_INFO( "Selected {}.", PhysicalDeviceProperties.deviceName );
        vkGetPhysicalDeviceMemoryProperties( PhysicalDevice, &PhysicalDeviceMemoryProperties );
    }

    bool isDeviceSuitable( VkPhysicalDevice device )
    {
        vkGetPhysicalDeviceProperties( device, &PhysicalDeviceProperties );
        vkGetPhysicalDeviceFeatures( device, &PhysicalDeviceFeatures );
        auto indices{ findQueueFamilies( device ) };
        bool dSupportExts{ isDeviceSupportExtensions( device ) };
        if( (
                !indices.isComplete() ||
                !( PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && PhysicalDeviceFeatures.geometryShader && PhysicalDeviceFeatures.samplerAnisotropy ) ||
                !dSupportExts ) ) return false;
        if( dSupportExts )
        {
            SwapChainProperties deviceSwapChainProperties{ GetSwapchainProperties( device ) };
            if( !deviceSwapChainProperties.Format.size() && !deviceSwapChainProperties.PresentModes.size() ) return false;
            PhysiacalDeviceSwapchainProperties = deviceSwapChainProperties;
        }
        PhysicalDevice        = device;
        PhysicalDeviceIndices = indices;
        return true;
    }

    bool isDeviceSupportExtensions( VkPhysicalDevice device )
    {
        uint32_t extCount{ 0 };
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extCount, nullptr );
        std::vector<VkExtensionProperties> AvilableExtNames{ extCount };
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extCount, AvilableExtNames.data() );
        std::set<std::string> tmpRequeredDeviceExts{ RequeredDeviceExts.begin(), RequeredDeviceExts.end() };
        for( const auto &ext : AvilableExtNames )
        {
            tmpRequeredDeviceExts.erase( ext.extensionName );
        }
        return tmpRequeredDeviceExts.empty();
    }

    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device )
    {
        QueueFamilyIndices indices;
        uint32_t queueCount{ 0 };
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueCount, nullptr );
        std::vector<VkQueueFamilyProperties> QueueFamilies( queueCount );
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueCount, QueueFamilies.data() );
        uint32_t index{ 0 };
        for( const auto &queueF : QueueFamilies )
        {
            if( queueF.queueFlags & VK_QUEUE_GRAPHICS_BIT ) indices.graphicsFamily = index;
            if( queueF.queueFlags & VK_QUEUE_TRANSFER_BIT ) indices.transferFamily = index;
            VkBool32 presentSupport{ false };
            vkGetPhysicalDeviceSurfaceSupportKHR( device, index, Surface, &presentSupport );
            if( presentSupport ) indices.presentFamily = index;
            index++;
        }
        return indices;
    }

    void CreateLogicalDevice()
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        std::unordered_map<uint32_t, float> QueueFamiliesPriority{
            { PhysicalDeviceIndices.graphicsFamily.value(), 1.0f },
            { PhysicalDeviceIndices.presentFamily.value(), 1.0f },
            { PhysicalDeviceIndices.transferFamily.value(), 1.0f } };
        for( auto &QueueFamily : QueueFamiliesPriority )
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = QueueFamily.first;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &QueueFamily.second;
            queueCreateInfos.push_back( queueCreateInfo );
        }
        LogicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
        VkDeviceCreateInfo createInfo{};
        createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
        createInfo.pQueueCreateInfos    = queueCreateInfos.data();
        createInfo.pEnabledFeatures     = &LogicalDeviceFeatures;
        // createInfo.pEnabledFeatures        = &PhysicalDeviceFeatures;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>( RequeredDeviceExts.size() );
        createInfo.ppEnabledExtensionNames = RequeredDeviceExts.data();
        if( APP_DEBUG )
        {
            createInfo.enabledLayerCount   = static_cast<uint32_t>( ValidationLayers.size() );
            createInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else
            createInfo.enabledLayerCount = 0;
        // extension

        VkResult DeviceCreateCode = vkCreateDevice( PhysicalDevice, &createInfo, nullptr, &LogicalDevice );
        if( DeviceCreateCode != VK_SUCCESS ) _CriticalThrow( std::format( "Failed create logic device, error code: {}.", string_VkResult( DeviceCreateCode ) ) );
        vkGetDeviceQueue( LogicalDevice, PhysicalDeviceIndices.graphicsFamily.value(), 0, &LogicalDeviceGraphickQueue );
        vkGetDeviceQueue( LogicalDevice, PhysicalDeviceIndices.presentFamily.value(), 0, &LogicalDevicePresentQueue );
        vkGetDeviceQueue( LogicalDevice, PhysicalDeviceIndices.transferFamily.value(), 0, &LogicalDeviceTransferQueue );
    }

    SwapChainProperties GetSwapchainProperties( VkPhysicalDevice device )
    {
        SwapChainProperties Properties;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, Surface, &Properties.Capabilities );
        uint32_t formatsCount{ 0 };
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, Surface, &formatsCount, nullptr );
        // if (formatsCount)
        Properties.Format.resize( formatsCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, Surface, &formatsCount, Properties.Format.data() );
        uint32_t PresentModesCount{ 0 };
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, Surface, &PresentModesCount, nullptr );
        Properties.PresentModes.resize( PresentModesCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, Surface, &PresentModesCount, Properties.PresentModes.data() );
        return Properties;
    }

    void CreateImageViews()
    {
        SwapchainImgsView.resize( SwapchainImgs.size() );
        for( size_t i{ 0 }; i < SwapchainImgs.size(); i++ )
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                           = SwapchainImgs[ i ];
            createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                          = SwapchainFormat;
            createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;
            VkResult Result{ vkCreateImageView( LogicalDevice, &createInfo, nullptr, &SwapchainImgsView[ i ] ) };
            if( Result != VK_SUCCESS )
            {
                _CriticalThrow( std::format( "Failed to create imageView, error {}.", string_VkResult( Result ) ) );
            }
        }
    }

    void CreateSwapchain()
    {
        PhysiacalDeviceSwapchainProperties = GetSwapchainProperties( PhysicalDevice );
        VkSurfaceFormatKHR SurfaceFormat{ PhysiacalDeviceSwapchainProperties.Format[ 0 ] };
        for( const auto &format : PhysiacalDeviceSwapchainProperties.Format )
        {
            if( format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR ) SurfaceFormat = format;
        }
        SwapchainFormat = SurfaceFormat.format;

        VkPresentModeKHR SurfacePresentMode{ VK_PRESENT_MODE_FIFO_KHR };
        for( const auto &mode : PhysiacalDeviceSwapchainProperties.PresentModes )
        {
            if( mode == VK_PRESENT_MODE_MAILBOX_KHR ) SwapchainPresentMode = SurfacePresentMode = mode;
        }
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );
        while( width == 0 || height == 0 )
        {
            glfwGetFramebufferSize( window, &width, &height );
            glfwWaitEvents();
        }
        PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.width  = DISPLAY_WIDTH;
        PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.height = DISPLAY_HEIGHT;
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width   = std::clamp( static_cast<uint32_t>( width ), PhysiacalDeviceSwapchainProperties.Capabilities.minImageExtent.width, PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.width );
        PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height  = std::clamp( static_cast<uint32_t>( height ), PhysiacalDeviceSwapchainProperties.Capabilities.minImageExtent.height, PhysiacalDeviceSwapchainProperties.Capabilities.maxImageExtent.height );
        // PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height = height;
        uint32_t imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.minImageCount;
        if( PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount > 0 && imageCount < PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount )
        {
            imageCount = PhysiacalDeviceSwapchainProperties.Capabilities.maxImageCount;
        }
        _MaxFramesInFlight = imageCount;
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface          = Surface;
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = SurfaceFormat.format;
        createInfo.imageColorSpace  = SurfaceFormat.colorSpace;
        createInfo.presentMode      = SurfacePresentMode;
        createInfo.imageExtent      = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t physicalDeviceIndicesValue[]{ PhysicalDeviceIndices.graphicsFamily.value(), PhysicalDeviceIndices.presentFamily.value() };
        if( PhysicalDeviceIndices.graphicsFamily != PhysicalDeviceIndices.presentFamily )
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = physicalDeviceIndicesValue;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        createInfo.preTransform   = PhysiacalDeviceSwapchainProperties.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.clipped        = VK_TRUE;
        createInfo.oldSwapchain   = VK_NULL_HANDLE;
        VkResult Result{ vkCreateSwapchainKHR( LogicalDevice, &createInfo, nullptr, &SwapChain ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Swapchain wasn't be created, error: {}", string_VkResult( Result ) ) );
        }
        vkGetSwapchainImagesKHR( LogicalDevice, SwapChain, &imageCount, nullptr );
        SwapchainImgs.resize( imageCount );
        vkGetSwapchainImagesKHR( LogicalDevice, SwapChain, &imageCount, SwapchainImgs.data() );
    }

    void ReCreateSwapChain()
    {
        int w, h;
        glfwGetFramebufferSize( window, &w, &h );
        while( !( w || h ) )
        {
            glfwGetFramebufferSize( window, &w, &h );
            glfwWaitEvents();
        }
        DestroySwapchain();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipelines();
        CreateFrameBuffers();
        CreateVertexBuffer();
        CreateUniformBuffers();
        CreateCommandBuffers();
    }

    void CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format         = SwapchainFormat;
        colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments    = &colorAttachment;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;
        VkResult Result{ vkCreateRenderPass( LogicalDevice, &renderPassInfo, nullptr, &RenderPass ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to create render pass, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void CreateGraphicsPipelines()
    {
        VkPipelineShaderStageCreateInfo VertexShaderStage{};
        VertexShaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertexShaderStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        VertexShaderStage.module = CreateShaderModule( "../shaders/shader.vert.spv" );
        VertexShaderStage.pName  = "main";

        VkPipelineShaderStageCreateInfo FragmentShaderStage{};
        FragmentShaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragmentShaderStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragmentShaderStage.module = CreateShaderModule( "../shaders/shader.frag.spv" );
        FragmentShaderStage.pName  = "main";

        ShaderStage = { VertexShaderStage, FragmentShaderStage };

        std::vector<VkDynamicState> dStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dStatescreateInfo{};
        dStatescreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dStatescreateInfo.dynamicStateCount = static_cast<uint32_t>( dStates.size() );
        dStatescreateInfo.pDynamicStates    = dStates.data();

        auto bindingDescription    = Vertex::InputBindingDescription();
        auto attributeDescriptions = Vertex::InputAttributeDescription();
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount   = 1;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescriptions.size() );
        vertexInputCreateInfo.pVertexBindingDescriptions      = &bindingDescription;
        vertexInputCreateInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_TRUE;
        inputAssemblyCreateInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable        = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth               = 1.f;
        Rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
        Rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        Rasterizer.depthBiasEnable         = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable  = VK_FALSE;
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo{};
        // DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable         = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable       = VK_FALSE;
        colorBlending.logicOp             = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount     = 1;
        colorBlending.pAttachments        = &colorBlendAttachment;
        colorBlending.blendConstants[ 0 ] = 0.0f; // Optional
        colorBlending.blendConstants[ 1 ] = 0.0f; // Optional
        colorBlending.blendConstants[ 2 ] = 0.0f; // Optional
        colorBlending.blendConstants[ 3 ] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts    = &DescriptorsSetLayout;

        VkResult Result{ vkCreatePipelineLayout( LogicalDevice, &pipelineLayoutInfo, nullptr, &PipelineLayout ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to create pipeline layout, error: {}", string_VkResult( Result ) ) );
        }

        VkGraphicsPipelineCreateInfo GraphicPipeLineCreateInfo{};
        GraphicPipeLineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        GraphicPipeLineCreateInfo.stageCount          = static_cast<uint32_t>( ShaderStage.size() );
        GraphicPipeLineCreateInfo.pStages             = ShaderStage.data();
        GraphicPipeLineCreateInfo.pVertexInputState   = &vertexInputCreateInfo;
        GraphicPipeLineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        GraphicPipeLineCreateInfo.pViewportState      = &viewportState;
        GraphicPipeLineCreateInfo.pRasterizationState = &Rasterizer;
        GraphicPipeLineCreateInfo.pMultisampleState   = &Multisampling;
        GraphicPipeLineCreateInfo.pColorBlendState    = &colorBlending;
        GraphicPipeLineCreateInfo.pDynamicState       = &dStatescreateInfo;
        GraphicPipeLineCreateInfo.layout              = PipelineLayout;
        GraphicPipeLineCreateInfo.renderPass          = RenderPass;
        GraphicPipeLineCreateInfo.subpass             = 0;

        Result = vkCreateGraphicsPipelines( LogicalDevice, nullptr, 1, &GraphicPipeLineCreateInfo, nullptr, &GraphicsPipeline );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to create Graphics pipeline, error: {}.", string_VkResult( Result ) ) );
        }
    }

    VkShaderModule CreateShaderModule( const char *shPath )
    {
        std::ifstream File{ shPath, std::fstream::ate | std::fstream::binary };
        if( !File.is_open() )
        {
            _CriticalThrow( std::format( "Shader binary {} can't be opened.", shPath ) );
        }
        size_t shBsize{ static_cast<size_t>( File.tellg() ) };
        std::vector<char> Data( shBsize );
        File.seekg( 0 );
        File.read( Data.data(), shBsize );
        File.close();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = Data.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t *>( Data.data() );
        VkShaderModule shaderModule;
        VkResult Result{ vkCreateShaderModule( LogicalDevice, &createInfo, nullptr, &shaderModule ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to create shader module for {}, error:{}.", shPath, string_VkResult( Result ) ) );
        }
        return shaderModule;
    }

    void CreateFrameBuffers()
    {
        SwapchainFrameBuffers.resize( SwapchainImgsView.size() );
        for( size_t i{ 0 }; i < SwapchainImgsView.size(); i++ )
        {
            VkFramebufferCreateInfo FrameBufferCreateInfo{};
            FrameBufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FrameBufferCreateInfo.renderPass      = RenderPass;
            FrameBufferCreateInfo.attachmentCount = 1;
            FrameBufferCreateInfo.pAttachments    = &SwapchainImgsView[ i ];
            FrameBufferCreateInfo.width           = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width;
            FrameBufferCreateInfo.height          = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height;
            FrameBufferCreateInfo.layers          = 1;
            VkResult Result{ vkCreateFramebuffer( LogicalDevice, &FrameBufferCreateInfo, nullptr, &SwapchainFrameBuffers[ i ] ) };
            if( Result != VK_SUCCESS )
            {
                _CriticalThrow( std::format( "Failed to create Swapchain frame buffer, error: {}.", string_VkResult( Result ) ) );
            }
        }
    }

    void CreateCommandPool()
    {
        VkCommandPoolCreateInfo CommandPoolCreateInfo{};
        CommandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CommandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CommandPoolCreateInfo.queueFamilyIndex = PhysicalDeviceIndices.graphicsFamily.value();
        VkResult Result{ vkCreateCommandPool( LogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to create grapchi Command pool, error: {}.", string_VkResult( Result ) ) );
        }
    }

    VkCommandBuffer BeginSingleTimeCommand()
    {
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo CommandBufferAllocateInfo{};
        CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CommandBufferAllocateInfo.commandPool        = CommandPool;
        CommandBufferAllocateInfo.commandBufferCount = 1;
        CommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VkResult Result{ vkAllocateCommandBuffers( LogicalDevice, &CommandBufferAllocateInfo, &commandBuffer ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Allocate Single Time Command Buffer, error: {}.", string_VkResult( Result ) ) );
        }
        VkCommandBufferBeginInfo CommandBufferBeginInfo{};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer( commandBuffer, &CommandBufferBeginInfo );
        return commandBuffer;
    }

    void EndSingleTimeCommand( VkCommandBuffer commandBuffer )
    {
        vkEndCommandBuffer( commandBuffer );
        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers    = &commandBuffer;
        VkResult Result{ vkQueueSubmit( LogicalDeviceGraphickQueue, 1, &SubmitInfo, nullptr ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Queue Submit, error: {}.", string_VkResult( Result ) ) );
        }
        vkQueueWaitIdle( LogicalDeviceGraphickQueue );
        vkFreeCommandBuffers( LogicalDevice, CommandPool, 1, &commandBuffer );
    }

    void CreateBuffer( VkDeviceSize size, VkBufferUsageFlags bUsage, VkMemoryPropertyFlags mProperties, VkBufferCreateInfo BufferCreateInfo, VkBuffer &Buffer, VkDeviceMemory &mBuffer, VkDeviceSize MemoryOffset )
    {
        BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = size;
        BufferCreateInfo.usage       = bUsage;
        BufferCreateInfo.flags       = BufferCreateInfo.flags ? BufferCreateInfo.flags : 0;
        BufferCreateInfo.sharingMode = BufferCreateInfo.sharingMode ? BufferCreateInfo.sharingMode : VK_SHARING_MODE_EXCLUSIVE;

        VkResult Result{ vkCreateBuffer( LogicalDevice, &BufferCreateInfo, nullptr, &Buffer ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create Buffer, error: {}.", string_VkResult( Result ) ) );
        }
        VkMemoryRequirements Requirements;
        vkGetBufferMemoryRequirements( LogicalDevice, Buffer, &Requirements );
        VkMemoryAllocateInfo MemoryAllocateInfo{};
        MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MemoryAllocateInfo.allocationSize  = Requirements.size;
        MemoryAllocateInfo.memoryTypeIndex = MemoryTypeIndex( Requirements.memoryTypeBits, mProperties );

        Result = vkAllocateMemory( LogicalDevice, &MemoryAllocateInfo, nullptr, &mBuffer );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Allocate Buffer Memory, error: {}.", string_VkResult( Result ) ) );
        }
        Result = vkBindBufferMemory( LogicalDevice, Buffer, mBuffer, MemoryOffset );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Bind Buffer to Buffer Memory, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void CreateImage( const VkDeviceSize size, const uint32_t width, const uint32_t height, const VkBufferUsageFlags iUsage, const VkMemoryPropertyFlags mProperties, VkImageCreateInfo ImageCreateInfo, VkImage &Image, VkDeviceMemory &mImage, VkDeviceSize MemoryOffset )
    {
        ImageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
        ImageCreateInfo.extent.width  = width;
        ImageCreateInfo.extent.height = height;
        ImageCreateInfo.extent.depth  = ImageCreateInfo.extent.depth ? ImageCreateInfo.extent.depth : 1;
        ImageCreateInfo.mipLevels     = ImageCreateInfo.mipLevels ? ImageCreateInfo.mipLevels : 1;
        ImageCreateInfo.arrayLayers   = ImageCreateInfo.arrayLayers ? ImageCreateInfo.arrayLayers : 1;
        ImageCreateInfo.format        = ImageCreateInfo.format ? ImageCreateInfo.format : VK_FORMAT_R8G8B8A8_SRGB;
        ImageCreateInfo.tiling        = ImageCreateInfo.tiling ? ImageCreateInfo.tiling : VK_IMAGE_TILING_OPTIMAL;
        ImageCreateInfo.initialLayout = ImageCreateInfo.initialLayout ? ImageCreateInfo.initialLayout : VK_IMAGE_LAYOUT_UNDEFINED;
        ImageCreateInfo.usage         = iUsage;
        ImageCreateInfo.sharingMode   = ImageCreateInfo.sharingMode ? ImageCreateInfo.sharingMode : VK_SHARING_MODE_EXCLUSIVE;
        ImageCreateInfo.samples       = ImageCreateInfo.samples ? ImageCreateInfo.samples : VK_SAMPLE_COUNT_1_BIT;

        VkResult Result{ vkCreateImage( LogicalDevice, &ImageCreateInfo, nullptr, &Image ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create Image, error {}.", string_VkResult( Result ) ) );
        }

        VkMemoryRequirements MemoryRequirements{};
        vkGetImageMemoryRequirements( LogicalDevice, Image, &MemoryRequirements );

        VkMemoryAllocateInfo MemoryAllocateInfo{};
        MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
        MemoryAllocateInfo.memoryTypeIndex = MemoryTypeIndex( MemoryRequirements.memoryTypeBits, mProperties );

        Result = vkAllocateMemory( LogicalDevice, &MemoryAllocateInfo, nullptr, &mImage );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Allocate Image Memory, error: {}", string_VkResult( Result ) ) );
        }
        Result = vkBindImageMemory( LogicalDevice, Image, mImage, MemoryOffset );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Bind Image to Image Memory, error: {}", string_VkResult( Result ) ) );
        }
    }

    void CreateTextureImage() // TODO: as normal class metod.
    {
        const char *path{ "textures/img.png" };
        int imgW, imgH, imgCs;
        stbi_uc *content{ stbi_load( std::format( "{}../{}", PATH_PREFIX, path ).c_str(), &imgW, &imgH, &imgCs, STBI_rgb_alpha ) };
        if( !content )
        {
            _CriticalThrow( std::format( "Failed to load texture [{}]", path ) );
        }
        VkDeviceSize Size = imgW * imgH * imgCs;
        VkBuffer TransferBuffer;
        VkDeviceMemory TransferBufferMemory;
        CreateBuffer( Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, {}, TransferBuffer, TransferBufferMemory, 0 );
        void *data;
        vkMapMemory( LogicalDevice, TransferBufferMemory, 0, Size, 0, &data );
        memcpy( data, content, static_cast<size_t>( Size ) );
        vkUnmapMemory( LogicalDevice, TransferBufferMemory );
        stbi_image_free( content );

        CreateImage( Size, imgW, imgH, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, {}, TextureImage, TextureImageMemory, 0 );
        ImageTransition( TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

        std::vector<VkBufferImageCopy> BufferImageCopy{ {} };
        BufferImageCopy[ 0 ].bufferImageHeight = 0;
        BufferImageCopy[ 0 ].bufferOffset      = 0;
        BufferImageCopy[ 0 ].bufferRowLength   = 0;

        BufferImageCopy[ 0 ].imageExtent.width  = imgW;
        BufferImageCopy[ 0 ].imageExtent.height = imgH;
        BufferImageCopy[ 0 ].imageExtent.depth  = 1;
        BufferImageCopy[ 0 ].imageOffset        = { 0, 0, 0 };

        BufferImageCopy[ 0 ].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        BufferImageCopy[ 0 ].imageSubresource.layerCount     = 1;
        BufferImageCopy[ 0 ].imageSubresource.baseArrayLayer = 0;
        BufferImageCopy[ 0 ].imageSubresource.mipLevel       = 0;

        TransferBufferToImage( TransferBuffer, TextureImage, BufferImageCopy );
        ImageTransition( TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
        vkDestroyBuffer( LogicalDevice, TransferBuffer, nullptr );
        vkFreeMemory( LogicalDevice, TransferBufferMemory, nullptr );
    }

    void CreateTextureImageView()
    {
        VkImageViewCreateInfo ImageViewCreateInfo{};
        ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageViewCreateInfo.image                           = TextureImage;
        ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
        ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        ImageViewCreateInfo.subresourceRange.levelCount     = 1;
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        ImageViewCreateInfo.subresourceRange.layerCount     = 1;

        VkResult Result{ vkCreateImageView( LogicalDevice, &ImageViewCreateInfo, nullptr, &TextureImageView ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create Texture Image View, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void CreateTextureSempler()
    {
        VkSamplerCreateInfo SamplerCreateInfo{};
        SamplerCreateInfo.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;

        SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        SamplerCreateInfo.anisotropyEnable = VK_TRUE;
        SamplerCreateInfo.maxAnisotropy    = PhysicalDeviceProperties.limits.maxSamplerAnisotropy; // User setting
        SamplerCreateInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        SamplerCreateInfo.compareEnable = VK_FALSE;
        SamplerCreateInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

        // mipmap wait
        SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerCreateInfo.mipLodBias = 0.0f;
        SamplerCreateInfo.minLod     = 0.0f;
        SamplerCreateInfo.maxLod     = 0.0f;

        VkResult Result{ vkCreateSampler( LogicalDevice, &SamplerCreateInfo, nullptr, &TextureSampler ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create Texture Sampler, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void ImageTransition( VkImage image, VkFormat format, VkImageLayout layout, VkImageLayout NewLayout )
    {
        VkPipelineStageFlags srcStageMask{ 0 };
        VkPipelineStageFlags dstStageMask{ 0 };
        VkImageMemoryBarrier ImageMemoryBarrier{};
        ImageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageMemoryBarrier.image               = image;
        ImageMemoryBarrier.oldLayout           = layout;
        ImageMemoryBarrier.newLayout           = NewLayout;
        ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        if( layout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
        {
            ImageMemoryBarrier.srcAccessMask = 0;
            ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStageMask                     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStageMask                     = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if( layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
        {
            ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStageMask                     = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask                     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            _CriticalThrow( std::format( "Unsupported layout transition, old layout: {}, new layout: {}", std::to_string( layout ), std::to_string( NewLayout ) ) );
        }
        ImageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageMemoryBarrier.subresourceRange.layerCount     = 1;
        ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        ImageMemoryBarrier.subresourceRange.levelCount     = 1;
        ImageMemoryBarrier.subresourceRange.baseMipLevel   = 0;

        VkCommandBuffer commandBuffer{ BeginSingleTimeCommand() };

        vkCmdPipelineBarrier( commandBuffer, srcStageMask, dstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &ImageMemoryBarrier );
        EndSingleTimeCommand( commandBuffer );
    }

    void CreateVertexBuffer()
    {
        size_t len_vB{ 0 };
        std::vector<Vertex> vBp;
        std::vector<VkBufferCopy> vBpC;
        size_t len_viB{ 0 };
        std::vector<uint32_t> viBp;
        std::vector<VkBufferCopy> viBpC;
        for( auto Path{ PathsToModel.begin() }; Path != PathsToModel.end(); Path++ )
        {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            std::vector<Vertex> mVertecies;
            std::vector<uint32_t> mIndecies;
            VkBufferCopy sVerteciesCopy{};
            VkBufferCopy sIndeciesCopy{};
            if( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, std::format( "{}../{}", PATH_PREFIX, Path->first ).c_str() ) )
            {
                _CriticalThrow( std::format( "Failed to Load model {}:\nwarning:\t{}\nerror:\t{}.", std::format( "{}../{}", PATH_PREFIX, Path->first ).c_str(), warn, err ) );
            }
            if( warn.length() )
                SPDLOG_WARN( "Warn with load model {}:{}", Path->first, warn );
            if( err.length() )
                SPDLOG_ERROR( "Error with load model {}:{}", Path->first, err );
            Model mData;
            std::unordered_map<Vertex, uint32_t> Indecies{};
            if( !Models.empty() ) mData.VerteciesOffset = Models[ ( Path - 1 )->second ].VerteciesOffset + static_cast<uint32_t>( Models[ ( Path - 1 )->second ].ModelVertecies.size() );
            for( const auto &shape : shapes )
            {
                for( const auto &index : shape.mesh.indices )
                {
                    mIndecies.push_back( index.vertex_index + mData.VerteciesOffset );
                    viBp.push_back( index.vertex_index + mData.VerteciesOffset );
                }
                for( size_t vert_i{ 0 }; vert_i < attrib.vertices.size(); vert_i += 3 )
                {
                    Vertex mVert{};
                    mVert.coordinate = {
                        attrib.vertices[ vert_i ],
                        attrib.vertices[ vert_i + 1 ],
                        attrib.vertices[ vert_i + 2 ] };
                    mVert.color = {
                        1.f,
                        1.f,
                        !strcmp( Path->second, "test" ) ? 1.f : .0f,
                        1.f };
                    switch( vert_i / 3 )
                    {
                        case 0:
                            mVert.texture = { 0.f, 1.f };
                            break;
                        case 1:
                            mVert.texture = { 0.f, 0.f };
                            break;
                        case 2:
                            mVert.texture = { 1.f, 1.f };
                            break;

                        default:
                            mVert.texture = { 1.f, 0.f };
                            break;
                    }
                    mVertecies.push_back( mVert );
                    vBp.push_back( mVert );
                }
            }
            mData.ModelVertecies          = mVertecies;
            mData.ModelVerteciesIndices   = mIndecies;
            len_vB += sVerteciesCopy.size = sizeof( Vertex ) * mData.ModelVertecies.size();
            len_viB += sIndeciesCopy.size = sizeof( uint32_t ) * mData.ModelVerteciesIndices.size();
            if( !Models.empty() )
            {
                sVerteciesCopy.dstOffset = sVerteciesCopy.srcOffset = vBpC.back().size + vBpC.back().srcOffset;
                sIndeciesCopy.dstOffset = sIndeciesCopy.srcOffset = viBpC.back().size + viBpC.back().srcOffset;
                mData.IndeciesOffset                              = Models[ ( Path - 1 )->second ].IndeciesOffset + static_cast<uint32_t>( Models[ ( Path - 1 )->second ].ModelVerteciesIndices.size() );
            }
            vBpC.push_back( sVerteciesCopy );
            VerteciesOffSets.push_back( sVerteciesCopy.dstOffset );
            viBpC.push_back( sIndeciesCopy );
            Models[ Path->second ] = mData;
        }
        void *data;
        VkBuffer TransferBuffer;
        VkDeviceMemory TransferBufferMemory;
        CreateBuffer( len_vB, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, {}, TransferBuffer, TransferBufferMemory, 0 );
        vkMapMemory( LogicalDevice, TransferBufferMemory, 0, len_vB, 0, &data );
        memcpy( data, vBp.data(), static_cast<size_t>( len_vB ) );
        vkUnmapMemory( LogicalDevice, TransferBufferMemory );
        CreateBuffer( len_vB, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, {}, VertexBuffer, VertexBufferMemory, 0 );
        TransferDataBetweenBuffers( TransferBuffer, VertexBuffer, vBpC );
        vkDestroyBuffer( LogicalDevice, TransferBuffer, nullptr );
        vkFreeMemory( LogicalDevice, TransferBufferMemory, nullptr );

        CreateBuffer( len_viB, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, {}, TransferBuffer, TransferBufferMemory, 0 );

        vkMapMemory( LogicalDevice, TransferBufferMemory, 0, len_viB, 0, &data );
        memcpy( data, viBp.data(), len_viB );
        vkUnmapMemory( LogicalDevice, TransferBufferMemory );

        CreateBuffer( len_viB, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, {}, VertexIndecesBuffer, VertexIndecesBufferMemory, 0 );

        TransferDataBetweenBuffers( TransferBuffer, VertexIndecesBuffer, viBpC );

        vkDestroyBuffer( LogicalDevice, TransferBuffer, nullptr );
        vkFreeMemory( LogicalDevice, TransferBufferMemory, nullptr );
    }

    void CreateUniformBuffers()
    {
        UniformBuffers.resize( SwapchainImgs.size() );
        UniformBuffersMemory.resize( SwapchainImgs.size() );
        for( size_t i{ 0 }; i < SwapchainImgs.size(); i++ )
        {
            CreateBuffer( sizeof( UniformBufferObject ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, {}, UniformBuffers[ i ], UniformBuffersMemory[ i ], 0 );
        }
    }

    void CreateDescriptorsPool()
    {
        VkDescriptorPoolSize UBDescriptorPoolSize{};
        UBDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        UBDescriptorPoolSize.descriptorCount = _MaxFramesInFlight;

        VkDescriptorPoolSize SamplerDescriptorPoolSize{};
        SamplerDescriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SamplerDescriptorPoolSize.descriptorCount = _MaxFramesInFlight;

        VkDescriptorPoolSize DescriptorPoolSize[ 2 ]{ UBDescriptorPoolSize, SamplerDescriptorPoolSize };

        VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo{};
        DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        DescriptorPoolCreateInfo.poolSizeCount = sizeof( DescriptorPoolSize ) / sizeof( DescriptorPoolSize[ 0 ] );
        DescriptorPoolCreateInfo.pPoolSizes    = DescriptorPoolSize;
        DescriptorPoolCreateInfo.maxSets       = _MaxFramesInFlight;

        VkResult Result{ vkCreateDescriptorPool( LogicalDevice, &DescriptorPoolCreateInfo, nullptr, &DescriptorPool ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create descriptor pool, error: {}", string_VkResult( Result ) ) );
        }
    }

    void CreateDescriptorSets() // TODO: As normal class method
    {
        std::vector<VkDescriptorSetLayout> DescriptorSetLayouts( _MaxFramesInFlight, DescriptorsSetLayout );
        VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo{};
        DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescriptorSetAllocateInfo.descriptorSetCount = _MaxFramesInFlight;
        DescriptorSetAllocateInfo.pSetLayouts        = DescriptorSetLayouts.data();
        DescriptorSetAllocateInfo.descriptorPool     = DescriptorPool;

        DescriptorSets.resize( _MaxFramesInFlight );
        VkResult Result{ vkAllocateDescriptorSets( LogicalDevice, &DescriptorSetAllocateInfo, DescriptorSets.data() ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Alocate descriptor sets, error: {}.", string_VkResult( Result ) ) );
        }
        for( size_t i{ 0 }; i < _MaxFramesInFlight; i++ )
        {
            VkDescriptorBufferInfo DescriptorBufferInfo{};
            DescriptorBufferInfo.buffer = UniformBuffers[ i ];
            DescriptorBufferInfo.offset = 0;
            DescriptorBufferInfo.range  = sizeof( UniformBufferObject );

            // TODO: As normal class method
            VkWriteDescriptorSet WriteUBDescriptorSet{};
            WriteUBDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            WriteUBDescriptorSet.dstSet          = DescriptorSets[ i ];
            WriteUBDescriptorSet.dstBinding      = 0;
            WriteUBDescriptorSet.dstArrayElement = 0;
            WriteUBDescriptorSet.descriptorCount = 1;
            WriteUBDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            WriteUBDescriptorSet.pBufferInfo     = &DescriptorBufferInfo;

            VkDescriptorImageInfo DescriptorImageInfo{};
            DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DescriptorImageInfo.imageView   = TextureImageView; // TODO: As normal class method
            DescriptorImageInfo.sampler     = TextureSampler;   // TODO: As normal class method

            VkWriteDescriptorSet WriteSamplerDescriptorSet{};
            WriteSamplerDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            WriteSamplerDescriptorSet.dstSet          = DescriptorSets[ i ];
            WriteSamplerDescriptorSet.dstBinding      = 1;
            WriteSamplerDescriptorSet.dstArrayElement = 0;
            WriteSamplerDescriptorSet.descriptorCount = 1;
            WriteSamplerDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            WriteSamplerDescriptorSet.pBufferInfo     = &DescriptorBufferInfo;
            WriteSamplerDescriptorSet.pImageInfo      = &DescriptorImageInfo;

            VkWriteDescriptorSet WriteDescriptorSet[ 2 ]{ WriteUBDescriptorSet, WriteSamplerDescriptorSet };
            vkUpdateDescriptorSets( LogicalDevice, sizeof( WriteDescriptorSet ) / sizeof( WriteDescriptorSet[ 0 ] ), WriteDescriptorSet, 0, nullptr );
        }
    }

    void TransferDataBetweenBuffers( VkBuffer bSrc, VkBuffer bDst, std::vector<VkBufferCopy> &BufferCopyInfo )
    {
        VkCommandBuffer commandBuffer{ BeginSingleTimeCommand() };
        vkCmdCopyBuffer( commandBuffer, bSrc, bDst, static_cast<uint32_t>( BufferCopyInfo.size() ), BufferCopyInfo.data() );
        EndSingleTimeCommand( commandBuffer );
    }

    void TransferBufferToImage( VkBuffer iSrc, VkImage iDst, std::vector<VkBufferImageCopy> &BufferImageCopy )
    {
        VkCommandBuffer commandBuffer{ BeginSingleTimeCommand() };
        vkCmdCopyBufferToImage( commandBuffer, iSrc, iDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>( BufferImageCopy.size() ), BufferImageCopy.data() );
        EndSingleTimeCommand( commandBuffer );
    }

    uint32_t MemoryTypeIndex( uint32_t type, VkMemoryPropertyFlags properties )
    {

        for( uint32_t i{ 0 }; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++ )
        {
            if( ( type & ( 1 << i ) ) && ( ( PhysicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties ) == properties ) ) return i;
        }
        _CriticalThrow( std::format( "Failed to find suitable memory type, type: {}, properties: {}.", type, properties ) );
        return -1;
    }

    void CreateCommandBuffers()
    {
        VkCommandBufferAllocateInfo CommandBufferAllocateInfo{};
        CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CommandBufferAllocateInfo.commandPool        = CommandPool;
        CommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferAllocateInfo.commandBufferCount = _MaxFramesInFlight;
        CommandBuffers.resize( _MaxFramesInFlight );
        VkResult Result{ vkAllocateCommandBuffers( LogicalDevice, &CommandBufferAllocateInfo, CommandBuffers.data() ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to allocate Command buffers, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void RecordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imI )
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo{};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkResult Result{ vkBeginCommandBuffer( commandBuffer, &CommandBufferBeginInfo ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to beging recording in Command buffer, error: {}.", string_VkResult( Result ) ) );
        }

        VkRenderPassBeginInfo RenderPassBeginInfo{};
        RenderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass        = RenderPass;
        RenderPassBeginInfo.framebuffer       = SwapchainFrameBuffers[ imI ];
        RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        RenderPassBeginInfo.renderArea.extent = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
        VkClearValue ClsClr{ 0.68f, .0f, 1.f, 1.f };
        RenderPassBeginInfo.clearValueCount = 1;
        RenderPassBeginInfo.pClearValues    = &ClsClr;

        vkCmdBeginRenderPass( commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
        vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline );

        VkViewport Viewport{};
        Viewport.x        = .0f;
        Viewport.y        = .0f;
        Viewport.width    = static_cast<float>( PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.width );
        Viewport.height   = static_cast<float>( PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent.height );
        Viewport.minDepth = .0f;
        Viewport.maxDepth = 1.f;
        vkCmdSetViewport( commandBuffer, 0, 1, &Viewport );

        VkRect2D Scissor{};
        Scissor.offset = { 0, 0 };
        Scissor.extent = PhysiacalDeviceSwapchainProperties.Capabilities.currentExtent;
        vkCmdSetScissor( commandBuffer, 0, 1, &Scissor );

        const VkBuffer _VertexBuffers[]{ VertexBuffer };
        const VkDeviceSize _Offsets[]{ 0 };

        vkCmdBindVertexBuffers( CommandBuffers[ imI ], 0, 1, _VertexBuffers, VerteciesOffSets.data() );
        vkCmdBindIndexBuffer( CommandBuffers[ imI ], VertexIndecesBuffer, 0, VK_INDEX_TYPE_UINT32 );
        vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescriptorSets[ imI ], 0, nullptr );
        for( const auto &m : Models )
        {
            vkCmdDrawIndexed( commandBuffer, static_cast<uint32_t>( m.second.ModelVerteciesIndices.size() ), 1, m.second.IndeciesOffset, 0, 0 );
        }

        vkCmdEndRenderPass( commandBuffer );

        Result = vkEndCommandBuffer( commandBuffer );
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to record Command buffer, error: {}.", string_VkResult( Result ) ) );
        }
    }

    void CreateDescriptionSetLayout()
    {
        VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingUB{};
        DescriptorSetLayoutBindingUB.binding         = 0;
        DescriptorSetLayoutBindingUB.descriptorCount = 1;
        DescriptorSetLayoutBindingUB.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorSetLayoutBindingUB.stageFlags      = VK_SHADER_STAGE_ALL_GRAPHICS;

        VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingSampler{};
        DescriptorSetLayoutBindingSampler.binding         = 1;
        DescriptorSetLayoutBindingSampler.descriptorCount = 1;
        DescriptorSetLayoutBindingSampler.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorSetLayoutBindingSampler.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> Binds{ DescriptorSetLayoutBindingUB, DescriptorSetLayoutBindingSampler };
        VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo{};
        DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        DescriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>( Binds.size() );
        DescriptorSetLayoutCreateInfo.pBindings    = Binds.data();
        VkResult Result{ vkCreateDescriptorSetLayout( LogicalDevice, &DescriptorSetLayoutCreateInfo, nullptr, &DescriptorsSetLayout ) };
        if( Result != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed to Create descriptor set layout, error {}.", string_VkResult( Result ) ) );
        }
    };

    void CreateVKinstance()
    {
        if( APP_DEBUG && !CheckValidationLayers() )
            _CriticalThrow( "Unavilable validation layer." );

        uint32_t glfwExtensionsCount{ 0 };
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionsCount );
        uint32_t glfwAvailableExtensionsCount{ 0 };
        vkEnumerateInstanceExtensionProperties( nullptr, &glfwAvailableExtensionsCount, nullptr );
        std::vector<VkExtensionProperties> AvailableExtensions( glfwAvailableExtensionsCount );
        vkEnumerateInstanceExtensionProperties( nullptr, &glfwAvailableExtensionsCount, AvailableExtensions.data() );

        VkApplicationInfo AppInfo{};
        AppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName   = TITLE.data();
        AppInfo.applicationVersion = VK_MAKE_VERSION( 0, 0, 1 );
        AppInfo.apiVersion         = VK_API_VERSION_1_0;

        VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
        VkValidationFeaturesEXT features       = {};
        features.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        features.enabledValidationFeatureCount = sizeof( enables ) / sizeof( VkValidationFeatureEnableEXT );
        features.pEnabledValidationFeatures    = enables;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;
        // CreateInfo.pNext = &features;

        DbgMessengerCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        DbgMessengerCreateInfo.pNext           = &features;
        DbgMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        DbgMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        DbgMessengerCreateInfo.pfnUserCallback = DebugCallback;
        DbgMessengerCreateInfo.pUserData       = this;

        if( APP_DEBUG )
        {
            CreateInfo.enabledLayerCount   = uint32_t( ValidationLayers.size() );
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
            CreateInfo.pNext               = &DbgMessengerCreateInfo;
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;
            CreateInfo.pNext             = nullptr;
        }

        std::vector<const char *> Extensions{ glfwExtensions, glfwExtensions + glfwExtensionsCount };
        Extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
        CreateInfo.enabledExtensionCount   = static_cast<uint32_t>( Extensions.size() );
        CreateInfo.ppEnabledExtensionNames = Extensions.data();

        VkResult InstanceCreateCode = vkCreateInstance( &CreateInfo, nullptr, &instance );
        if( InstanceCreateCode != VK_SUCCESS )
        {
            _CriticalThrow( std::format( "Failed create instance, error code: {}.", string_VkResult( InstanceCreateCode ) ) );
        }
    }

    bool CheckValidationLayers()
    {
        uint32_t Count;
        vkEnumerateInstanceLayerProperties( &Count, NULL );
        std::vector<VkLayerProperties> AviableLayers{ Count };
        vkEnumerateInstanceLayerProperties( &Count, AviableLayers.data() );
        for( const char *lName : ValidationLayers )
        {
            bool SupportLayer = false;
            for( const auto &AlName : AviableLayers )
            {
                if( !strcmp( lName, AlName.layerName ) )
                {
                    SupportLayer = true;
                    break;
                }
            }
            if( !SupportLayer )
            {
                SPDLOG_CRITICAL( "Validation layer {} isn't support.", lName );
                return false;
            }
        }
        return true;
    }

    void SetupDebugMessenger()
    {
        if( !APP_DEBUG ) return;
        VkResult CreateDebugUtilsMessengerEXT_result{ CreateDebugUtilsMessengerEXT( instance, &DbgMessengerCreateInfo, nullptr, &DebugMessenger ) };
        if( CreateDebugUtilsMessengerEXT_result != VK_SUCCESS ) _CriticalThrow( std::format( "Failed to setup Debug Messenger, error: {}.", string_VkResult( CreateDebugUtilsMessengerEXT_result ) ) );
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageLevel,
        VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT *CallbackData,
        void *UserData )
    {
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
        switch( MessageLevel )
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                SPDLOG_DEBUG( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                SPDLOG_INFO( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                SPDLOG_WARN( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage );
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                SPDLOG_ERROR( "{}message: {}", ( MessageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ? "" : format( "Type: {}, ", StrMessageType ) ), CallbackData->pMessage );
                break;
        }
        return VK_FALSE;
    }

    // Ext Functions load.

    VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger )
    {
        PFN_vkCreateDebugUtilsMessengerEXT _crtDbgUtMsgrEXT = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
        return ( _crtDbgUtMsgrEXT == nullptr ? VK_ERROR_EXTENSION_NOT_PRESENT : _crtDbgUtMsgrEXT( instance, pCreateInfo, pAllocator, pMessenger ) );
    }

    void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator )
    {
        PFN_vkDestroyDebugUtilsMessengerEXT _dstrDbgUtMsgrEXT = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
        if( _dstrDbgUtMsgrEXT != nullptr ) _dstrDbgUtMsgrEXT( instance, messenger, pAllocator );
    }

    void _CriticalThrow( std::string Error )
    {
        SPDLOG_CRITICAL( Error );
        throw std::runtime_error( Error );
    }
};

static void FramebufferResizeCallback( GLFWwindow *AppPointer, int width, int height )
{
    auto app = reinterpret_cast<App *>( glfwGetWindowUserPointer( AppPointer ) );
    // int a, b;
    // glfwGetFramebufferSize(AppPointer, &a, &b);
    // SPDLOG_CRITICAL("{} {} / {} {} / {} {}", app->WIDTH, app->HEIGHT, width, height, a, b);
    app->SetWindowSize( app->WIDTH, app->HEIGHT );
}
static void WindwoResizeCallback( GLFWwindow *AppPointer, int width, int height )
{
    auto app    = reinterpret_cast<App *>( glfwGetWindowUserPointer( AppPointer ) );
    app->WIDTH  = width;
    app->HEIGHT = height;
    app->SetWindowSize( width, height );
}
