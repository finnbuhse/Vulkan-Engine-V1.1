# Vulkan-Engine-V1.0
--== To Build ==--

You will need the following libraries included in your project:

GLFW - https://www.glfw.org

GLM - https://github.com/g-truc/glm

Vulkan - https://vulkan.lunarg.com

stb_image.h - https://github.com/nothings/stb

Assimp - https://assimp.org/index.php/downloads

PhysX - https://github.com/NVIDIAGameWorks/PhysX

For each of these libraries, you will need to add their include directories, and their binaries if they have any (.lib & .dll files). The linker will take care of the .lib files so long as you have provided the correct directory to the files, while .dll files should be copied to the directory where the executable is built.
