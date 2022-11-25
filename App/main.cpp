#include "App.h"

#include <iostream>

int main(int argc, char** argv)
{
    using namespace app;
    
    try
    {
        // Parse program options
        auto options = AppOptions::FromArgs(argc, argv);

        // Run app
        App app(std::move(options));
        app.Run();
        return 0;
    }
    catch (std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return -1;
    }
}
