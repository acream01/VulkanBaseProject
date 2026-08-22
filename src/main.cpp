
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Application.hpp"

//Download from https://github.com/nothings/stb/blob/master/stb_image.h and then add to dependancy path
//Check "TextureMapping/Images" section of vulkan tutorial

// This Vulkan Project was built using https://vulkan-tutorial.com/Introduction as a foundation
// Claire Ogawa and Aidan Ream

// VULKAN INFO AND TIPS
// Vulkan designed for minimal driver overhead
// 
// Vulkan tends to use structs to store/update info rather than function parameters
//      object creation function parameters in Vulkan follow is:
//      1. Pointer to struct with creation info
//      2. Pointer to custom allocator callbacks (we are using default memory allocators)
//      3. Pointer to the variable that stores the handle to the new object

// https://docs.vulkan.org/spec/latest/chapters/extensions.html#extendingvulkan-extensions
// Extensions may define new Vulkan commands, structures, and enumerants. 
// Validation layers are optional components that hook into Vulkan function calls to apply additional operations.
//  - help us with error handling

//Refactoring effort by Aidan Ream

int main() {
    // create instance of sample app
    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n"; // throw exception if app exe fails
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

    //sampleMain(); // TEST MAIN/POPUP WINDOW
}