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

Notes 11/12/2021
- Updated model loading.
- Updated rigidbodies with applyForce, setTransform, setLinearVelocity, and getLinearVelocity methods.
- Implemented serialization of rigid bodies.
- Added Interactable and Interactor components to enable the ability to interact with entities.
- Added GunItem component to implement/test Interactable component.
- Added GunWeapon component.

Notes 06/02/2022
- Added scene manager to allow for automatic garbage collection of entities and scene loading.
- Added mouse button press events to window manager.
- Updated GunWeapon to shoot projectiles.
- Fixed incorrect world position values for transforms.
