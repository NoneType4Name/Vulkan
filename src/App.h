#ifdef _DEBUG
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
const bool APP_DEBUG = true;
#else
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
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
#include <utility>
#include <spdlog/spdlog.h>
#include "vulkan.h"

const uint16_t DEFAULT_WIDTH{ 800 };
const uint16_t DEFAULT_HEIGHT{ 600 };

template <typename... Args>
void CriticalLogsCatch( spdlog::format_string_t<Args...> fmt, Args &&...args )
{
    spdlog::default_logger_raw()->critical( fmt, std::forward<Args>( args )... );
    throw std::runtime_error( "View log." );
}
template <typename T>
void CriticalLogsCatch( const T &msg )
{
    spdlog::default_logger_raw()->critical( msg );
    throw std::runtime_error( msg );
}

#define TRACE_CALLBACK    spdlog::trace
#define DEBUG_CALLBACK    spdlog::debug
#define INFO_CALLBACK     spdlog::info
#define WARN_CALLBACK     spdlog::warn
#define ERROR_CALLBACK    spdlog::error
#define CRITICAL_CALLBACK CriticalLogsCatch

namespace
{
struct _initialize
{
    _initialize()
    {
#ifdef _DEBUG
        try
        {
#endif
            spdlog::set_level( APP_DEBUG ? spdlog::level::trace : spdlog::level::critical );
            spdlog::set_pattern( "[%H:%M:%S.%e] [%^%l%$] %v" );
            DEBUG_CALLBACK( "--- Start logging. ---" );
#ifdef _DEBUG
        }
        catch( const std::exception &logInitE )
        {
            std::cerr << "Logger initialize error:\t" << logInitE.what() << "\n.";
            exit( EXIT_FAILURE );
        }
#endif

        if( glfwInit() )
        {
            DEBUG_CALLBACK( "GLFW{} inititialized.", glfwGetVersionString() );
            glfwSetErrorCallback( []( int code, const char *data )
                                  { ERROR_CALLBACK( "GLFW ERROR {}: {}", code, data ); } );
        }
        else
        {
            CRITICAL_CALLBACK( "GLFW not initialized." );
            INFO_CALLBACK( "Exit with code {}.", EXIT_FAILURE );
            exit( EXIT_FAILURE );
        }
    }
    ~_initialize()
    {
        glfwTerminate();
        DEBUG_CALLBACK( "App closed." );
        DEBUG_CALLBACK( "--- Log finish. ---" );
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

static void FramebufferResizeCallback( GLFWwindow *, int, int );
static void WindwoResizeCallback( GLFWwindow *, int, int );

class App
{
  public:
    uint16_t WIDTH;
    uint16_t HEIGHT;
    uint16_t DISPLAY_WIDTH;
    uint16_t DISPLAY_HEIGHT;
    std::string TITLE;
    std::vector<std::pair<const char *, const char *>> &Models;
    App( uint16_t width, uint16_t height, const char *title, std::vector<std::pair<const char *, const char *>> &models ) : WIDTH{ width }, HEIGHT{ height }, TITLE{ title }, Models{ models }
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
        static VulkanInstance VkApi{ glfwGetWin32Window( window ), GetModuleHandle( nullptr ), { []( const char *data )
                                                                                                 { TRACE_CALLBACK( data ); },
                                                                                                 []( const char *data )
                                                                                                 { DEBUG_CALLBACK( data ); },
                                                                                                 []( const char *data )
                                                                                                 { INFO_CALLBACK( data ); },
                                                                                                 []( const char *data )
                                                                                                 { WARN_CALLBACK( data ); },
                                                                                                 []( const char *data )
                                                                                                 { ERROR_CALLBACK( data ); },
                                                                                                 []( const char *data )
                                                                                                 { CRITICAL_CALLBACK( data ); } } };

        Vulkan = &VkApi;
    };
    ~App()
    {
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
        // ReCreateSwapChain();
    }

  private:
    GLFWwindow *window;
    VulkanInstance *Vulkan;
    void GetScreenResolution( uint16_t &width, uint16_t &height )
    {
        auto Monitor = glfwGetPrimaryMonitor();
        glfwGetVideoMode( Monitor );
        auto mode = glfwGetVideoMode( Monitor );
        width     = mode->width;
        height    = mode->height;
    }
};

static void FramebufferResizeCallback( GLFWwindow *AppPointer, int width, int height )
{
    auto app = reinterpret_cast<App *>( glfwGetWindowUserPointer( AppPointer ) );
    app->SetWindowSize( app->WIDTH, app->HEIGHT );
}
static void WindwoResizeCallback( GLFWwindow *AppPointer, int width, int height )
{
    auto app    = reinterpret_cast<App *>( glfwGetWindowUserPointer( AppPointer ) );
    app->WIDTH  = width;
    app->HEIGHT = height;
    app->SetWindowSize( width, height );
}
