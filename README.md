Basic 3D renderer in c++ from scratch.

All rendering is done on the CPU. External libraries:
- SDL, to draw pixels, windows and handle mouse input.
- Dear ImGui, for the menu window.
- stb_image, to load .png textures.

Boilerplate code of SDL and obj loading was taken from the internet.

Functions:
1. world space with a camera, objects and a light (matrix transforms)
2. move/rotate the object, camera and light from a menu window
3. change FOV
4. flat, gouraud and phong shading (with specular)
5. texture mapping (perspective correct), loaded from .mtl files
6. backface culling
7. fast cpu renders using z buffering and scanline fill
8. Can take any .obj file and render it in real time
9. Can Use custom vertex normal data in .obj files
10. floor grid, x/y/z axes and a circle showing the light
11. render scale for more fps (draws at lower resolution)
12. fps counter in the menu

WIP:
1. Blender style mouse controls for the camera.
2. Import multiple objects.
3. Multithreading.

Instructions to Build and Run:
1. clone this repo inside a folder
2. Start run.bat (takes a few seconds, it also compiles ImGui)

Controls:

Everything is in the menu window:
- Car / Light / Camera : click one to show its position and rotation, drag a value to change it
- flat / gouraud / phong : shading mode
- calculate normals : calculated normals or the .obj normals (flat only)
- backface cull : toggle backface culling
- textures : toggle textures
- render scale : lower = faster but blurrier
- specularity / exposure : drag to change

In the main window:
- Left Click and Drag mouse : Move the light source.

- [src/Engine3D.h (Line 50)](src/Engine3D.h#L50)  `mesh = OBJLoader("Object/full_model.obj");` change it to any .obj model
