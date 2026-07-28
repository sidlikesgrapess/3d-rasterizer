Basic 3D renderer in c++ from scratch.

only external library is SDL, to draw pixels, window and handle mouse/keyboard input.
AI was only used in helping me learn, not in writing my code. Boilerplate code of SDL and obj loading was taken from the internet.

Functions:
1. change FOV
2. translation in Z and Y axis
3. change rot in xz and yz axis
4. specular flat shading
5. backface culling
6. fast cpu renders using z buffering and scanline fill
7. Can take any .obj file and render it in real time (without textures)
8. Can Use custom vertex normal data in .obj files

WIP:
1. Cleaner and more abstract code.
2. Matrix optimizations.
3. smooth shading.
4. texture mapping.

Instructions to Build and Run:
1. clone this repo inside a folder
2. Start run.bat

Controls:

- Q/E : change FOV
- Arrow keys : rotate object
- W/S : move forward/backward
- Z/X : move up/down
- B : toggle backface culling
- C : toggle b/w calculating normals and using custom normals.
- 2 : increase specularity
- 1 : decrease specularity
- Left Click and Drag mouse : Move the light source.
- M : increase exposure
- N : decrease exposure
- [main.cpp (Line 196)](main.cpp#L196)  `mesh = OBJLoader("Object/full_model.obj");` change it to any .obj model

