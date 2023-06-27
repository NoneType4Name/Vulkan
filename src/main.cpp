#include "App.h"

// const Model Triangle{
//     { { -.5f, -.5f, .0f }, { 1.f, .0f, .0f, 1.f } },
//     // { { .0f, -0.5f, .0f }, { 1.f, .0f, .0f, 1.f } },
//     { { .5f, -.5f, .0f }, { .0f, 1.f, .0f, 1.f } },
//     { { .5f, .5f, .0f }, { .0f, 1.f, .0f, 1.f } },
//     { { -.5f, .5f, .0f }, { .0f, 0.f, 1.f, 1.f } } },
//     { 0, 1, 2, 2, 3, 1 };

// static const Model Models{};

int main( int argc, char *argv[] )
{
    // AppData Data{};
    // Data.Models = &Models;
    std::vector<std::pair<const char *, const char *>> ModelsPaths{
        { "models/triangle.obj", "triangle" } };
    try
    {
        // App app{ 0, 0, "HV", &Data };
        App app{ 0, 0, "HV", ModelsPaths };
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << std::endl;
        SPDLOG_CRITICAL( "{}\n Exit with error code {}.", e.what(), EXIT_FAILURE );
        return EXIT_FAILURE;
    }
    SPDLOG_INFO( "Exit with code {}.", EXIT_SUCCESS );
    return EXIT_SUCCESS;
}

// test for map from py:

// auto some_function = [](auto&& x) { cout << x << endl;};
// auto the_tuple = make_tuple(1, 2.1, '3', "four");
// apply([&](auto& ...x){(..., some_function(x));}, the_tuple);
