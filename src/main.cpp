#include "App.h"

int main( int argc, char *argv[] )
{
    SPDLOG_CRITICAL( "ARGS: {}", argc );
    std::vector<std::pair<const char *, const char *>> ModelsPaths{
        { "models/model.obj", "test" } };
    try
    {
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
