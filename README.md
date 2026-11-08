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
8. Loads every .obj file in the Obj_load folder and renders them in real time
9. Can Use custom vertex normal data in .obj files
10. floor grid, x/y/z axes and a circle showing the light
11. render scale for more fps (draws at lower resolution)
12. fps counter in the menu
13. Blender style mouse camera (orbit, pan, zoom)
14. multithreaded (uses all CPU cores)
15. simple ray tracing with shadows (a checkbox in the menu)
16. antialiasing (2x2 supersampling)
17. materials: diffuse, glossy and metallic (metallic + roughness, read from .mtl files)
18. reflections in the ray tracer (smooth materials)
19. multiple objects, each with its own transform and materials
20. directional light (sun) or point light (bulb, fades with distance)

WIP:
1. Soft shadows and blurry reflections in the ray tracer.

Instructions to Build and Run:
1. clone this repo inside a folder
2. Start run.bat (takes a few seconds, it also compiles ImGui)

Controls:

Everything is in the menu window:
- Light / Camera / objects : click one to show its transform, drag a value to change it (camera: target, distance, yaw, pitch, fov)
- Light : directional (position + target) or point (position + strength)
- the arrow next to an object : shows its materials, click one to change its color (used without a texture), metallic and roughness
- ray tracing : ray traced image with real shadows (greys out the shading options)
- flat / gouraud / phong : shading mode
- calculate normals : calculated normals or the .obj normals (flat only)
- backface cull : toggle backface culling
- textures : toggle textures
- antialiasing (2x2) : smoother edges, about 3x slower
- render scale : lower = faster but blurrier
- exposure : drag to change

In the main window:
- Left Click and Drag mouse : Move the light source.
- Middle Click and Drag : orbit the camera
- Shift + Middle Click and Drag : pan the camera
- Mouse wheel : zoom

Adding models: put the .obj (and its .mtl) in the Obj_load folder. Put its textures in a folder named `<file name>_tex` next to it, e.g. `Obj_load/car.obj` -> `Obj_load/car_tex/`. Objects are placed side by side.
