# Vulkan-Engine-V1.1
--==** WARNING **==--
This is a personal project and there is no gurantee it will be compatible with your system hence will take no responsibility in the case of damage resulting from normal or inproper use.

--== To Build ==--
You will need the following libraries included in your project:

GLFW - https://www.glfw.org

GLM - https://github.com/g-truc/glm

Vulkan - https://vulkan.lunarg.com

stb_image.h - https://github.com/nothings/stb

Assimp - https://assimp.org/index.php/downloads

PhysX - https://github.com/NVIDIAGameWorks/PhysX

Freetype - https://github.com/freetype/freetype

For each of these libraries, you will need to add their include directories, and their binaries if they have any (.lib & .dll files). The linker will take care of the .lib files so long as you have provided the correct directory to the files, while .dll files should be copied to the directory where the executable is built.

--== Updates ==--

16/11/2021
- Added rigidbodies in static, dynamic, and kinematic form.
- Added character controllers with lerped velocity. The default keys are WASD and Space to jump, I am planning to make a settings system at which point the movement keys will become configurable. Use these in conjunction with a camera controller dedicated to pitch since yaw controls the entire character controller.
- Added serialization of components and entities. The writeFile and readFile methods enable one to save serialized entities and load them from files.
- Added raycast method to physics system.

11/12/2021
- Updated model loading.
- Updated rigidbodies with applyForce, setTransform, setLinearVelocity, and getLinearVelocity methods.
- Implemented serialization of rigid bodies.
- Added Interactable and Interactor components to enable the ability to interact with entities.
- Added GunItem component to implement/test Interactable component.
- Added GunWeapon component.

06/02/2022
- Added scene manager to allow for automatic garbage collection of entities and scene loading.
- Added mouse button press events to window manager.
- Updated GunWeapon to shoot projectiles.
- Fixed incorrect world position values for transforms.

24/02/2022
- Fixed cleaning of projectiles shot by GunWeapon.
- Added UIText component.

26/02/2022
- Added Transform2D component.
- Added UIButton component.

17/03/2022
- Added Sprite component
- Added capability to rotate sprites/text/buttons.
- Fixed certain button callbacks causing crashes due to render pass.
- Added capability to subscribe to the scene manager to recieve notification of entities being added/removed from the scene.
- Added getWindowWidth, getWindowHeight, enableCursor and disableCursor subroutines to the WindowManager class.

20/05/2022
- Added toggle buttons.
- Added further mouse support to WindowManager.

16/07/2022
- Depricated Gun components, used an easy unideal solution. Repository will consist of mostly core items, along with helpful components such as Interactable and CameraController which are deemed general enough to be suited to a large proportion of their required uses.

- Updated CharacterController for smoother movement.
- Updated interactables for more consistent 'hits' when being interacted with. 
- Updated UIButton and UIText scaling; buttons have separate member variables width and height, while text is scaled souley via Transform2D::scale. If either possess a parent, its scale will not effect the size of the button/text however their positions behave as one would expect from hierarchies.
- Improved efficiency of button press detection/invokation of button callbacks.
- Updated button callbacks to be passed the button's ID as an argument improving the flexibility of what button callbacks are capable of.
- Added rudimentary scene menu. [In progress]
