# Vulkan-Engine-V1.1
--== To Build ==--

You will need the following libraries included in your project:

GLFW - https://www.glfw.org

GLM - https://github.com/g-truc/glm

Vulkan - https://vulkan.lunarg.com

stb_image.h - https://github.com/nothings/stb

Assimp - https://assimp.org/index.php/downloads

PhysX - https://github.com/NVIDIAGameWorks/PhysX

For each of these libraries, you will need to add their include directories, and their binaries if they have any (.lib & .dll files). The linker will take care of the .lib files so long as you have provided the correct directory to the files, while .dll files should be copied to the directory where the executable is built.

Notes 16/11/2021:
- Added rigidbodies in static, dynamic, and kinematic form.
- Added character controllers with lerped velocity. The default keys are WASD and Space to jump, I am planning to make a settings system at which point the movement keys will become configurable. Use these in conjunction with a camera controller dedicated to pitch since yaw controls the entire character controller.
- Added serialization of components and entities. The writeFile and readFile methods enable one to save serialized entities and load them from files.
- Added raycast method to physics system.
