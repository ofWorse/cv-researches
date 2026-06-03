#include "application.h"
#include <iostream>

int main(void) 
{
    Application app;

    if(!app.init(1280, 720, "CV Research")) 
    {
        std::cerr << "Failed to initialize application\n";
        return -1;
    }

    app.run();

    return 0;
}