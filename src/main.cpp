#include "App.h"

namespace
{
    static void GLFWerrorCallback(int code, const char *data)
    {
        SPDLOG_CRITICAL("GLFW ERROR {}: {}", code, data);
    }
    struct GLFWinit
    {
        GLFWinit()
        {
            spdlog::set_level(spdlog::level::trace);
            spdlog::set_pattern("[func %!] [line %#] [%H:%M:%S.%e] [%^%l%$] %v");
            SPDLOG_DEBUG("--- Start logging. ---");
            // exit(EXIT_FAILURE);

            int32_t code{glfwInit()};
            if ( code==GLFW_TRUE )
            {
                SPDLOG_DEBUG("GLFW{} inititialized", glfwGetVersionString());
                glfwSetErrorCallback( GLFWerrorCallback );
            }
            else
            {
                SPDLOG_CRITICAL("GLFW not initialized, errno{}", code);
                SPDLOG_INFO("Exit with code {}.", EXIT_FAILURE);
                exit(EXIT_FAILURE);
            }
        }
        ~GLFWinit()
        {
            glfwTerminate();
            SPDLOG_DEBUG("App closed.");
            SPDLOG_DEBUG("--- Log finish. ---");
            spdlog::shutdown();
        }
    }_;
}

int main(int argc, char *argv[])
{    
    try
    {
        App app{1920, 1080, "HV"};
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        SPDLOG_CRITICAL("{}\n Exit with error code {}.", e.what(), EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    SPDLOG_INFO("Exit with code {}.", EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

// test for map from py:

// auto some_function = [](auto&& x) { cout << x << endl;};
// auto the_tuple = make_tuple(1, 2.1, '3', "four");
// apply([&](auto& ...x){(..., some_function(x));}, the_tuple);
