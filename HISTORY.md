# Change History

Logic and structural changes since commit `e313d3b` ("deleted").

## 2026-11-07

1. **User:** Changed the loaded model from `Object/full_model.obj` to `Object/bmw.obj`.

2. **AI:** Split `main.cpp` into header files under `src/`. Logic is unchanged.
   - `main.cpp` now contains only `main()`.
   - New headers: `SDLHeaders.h`, `Config.h`, `Display.h`, `Color.h`, `Vec3.h`, `Mesh.h`, `ObjLoader.h`, `Transform.h`, `Projection.h`, `Shading.h`, `Framebuffer.h`, `Engine3D.h`.
   - The pixel and z buffers, and all draw and fill functions, moved into a `Framebuffer` struct. `Engine3D` owns it as `fb`.
   - Helpers that didn't use engine state became free functions: transforms, `screen()`, shading, culling and `OBJLoader`.
   - Removed `using namespace std;` and added `std::` prefixes instead.

3. **User:** Added a `rotate_xy` stub to `Transform.h`.

4. **AI:** Filled in `rotate_xy`, which rotates in the xy plane and leaves z unchanged.

5. **AI:** Added `translate_x_triangle` and `rotate_xy_triangle` to `Transform.h`.

6. **AI:** Switched the transforms to matrices.
   - Added `Mat4.h`: `Mat4`, `translation()`, `rotation_xz/yz/xy()` and `transform_triangle()`.
   - `Engine3D` now builds one matrix per frame instead of chaining the `translate_*` and `rotate_*_triangle` calls on every triangle.
   - The old `*_triangle` functions in `Transform.h` are now unused.

7. **AI:** Added world space and a camera.
   - Added `Camera.h`: world `position`, `yaw`, `pitch`, `fov` and `view_matrix()`.
   - Added `WorldTransform.h`: an object's world `position`, `rot_xz/yz/xy` and `matrix()`.
   - In `Engine3D`, the members `angle`, `angle2`, `z_offset`, `y_offset` and `fov` were replaced by `object` and `camera`.
   - Rendering now uses `view * model`, and the light direction is also transformed by `view`.
   - The controls behave the same as before. No keys move the camera yet.

8. **AI:** Moved one-press keys to an event handler.
   - Added `Engine3D::HandleEvent(e)`. `main()` now passes it every SDL event.
   - C, B, 1, 2, M and N moved there from `OnUserUpdate`. All `SDL_Delay(200)` calls were removed, so the frame no longer freezes.
   - C and B ignore held-key repeats. Holding 1, 2, M or N keeps stepping at the OS key-repeat rate.

9. **AI:** Made the light a plain world direction.
   - Removed the hack that rotated the light by the object's angles, and removed the `z_dir` member.
   - Left-click now maps the mouse to a direction seen from the camera, converted to world with the new `Camera::to_world_dir()`.
   - Each frame the light goes through `view`, like the mesh.
   - Behavior change: rotating the object now changes its shading, because the light stays fixed in the world instead of turning with the object.

10. **AI:** Added a menu in a second SDL window.
    - Added Dear ImGui v1.91.5, the core files plus the SDL2 and SDL_Renderer backends, in `headers/imgui/`.
    - Added `src/Menu.h`. It lists Car and Camera. Clicking one shows its transform fields: position, the rotations, and FOV for the camera. Editing a field changes `engine.object` or `engine.camera` directly.
    - `main()` creates the menu, sends it events and draws it every frame. Closing either window quits.
    - `Engine3D` now stores the main `window`. Keys and left-click only act when the main window has focus, so using the menu doesn't move the car or the light.
    - `run.bat` now also compiles the ImGui files. Added `compile_flags.txt` so the IDE finds the ImGui headers.

11. **AI:** Moved the FPS counter from the main window title to the top of the menu. It still updates once per second.

12. **AI:** Added a floor grid and x/y/z axes. They're fixed in the world and not listed in the menu.
    - Added `src/Lines.h`: a `Line` struct, `make_grid()`, `make_axes()` and `draw_lines()`.
    - The grid is on the xz plane at y = 0, spans ±10 and has a line every unit.
    - The axes span ±10: X red, Y green, Z blue.
    - Lines go through the view matrix, are cut off at z = 0.1 if they pass behind the camera, and are drawn with `DrawLine`.
    - They're drawn before the car and don't use the z-buffer, so the car is always drawn over them.
    - The default camera position changed from (0, 0, 0) to (0, 1, 0), so the floor isn't seen edge-on. The car is unchanged.

13. **AI:** Commented every keybind in `Engine3D.h` and added a Controls list to the menu.

14. **AI:** Made the light an object.
    - Added `src/Light.h`: a `Light` with a world `position`. It's still a directional light that shines from its position toward the world origin.
    - The default position is (-3, 3, 3), which gives the same direction as the old `light_dir`.
    - The `Engine3D` member `light_dir` was replaced by `light`.
    - Left-drag still sets the direction from the camera's view. It now moves the light to match, keeping its distance from the origin.
    - The light is drawn as a yellow circle, 10 px in radius, with the new `draw_circle()` in `Lines.h`. It's drawn last, so it's always visible.
    - Added Light to the menu list, with a position field.

15. **AI:** Removed all keyboard controls. Everything is now controlled from the menu.
    - Removed `Engine3D::HandleEvent` and its call in `main()`.
    - Removed the keyboard-polling code and the `keystate`, `dz` and `dy` members.
    - Car position and rotation, and camera FOV, were already menu fields.
    - Added a Settings section to the menu: checkboxes for "calculate normals" and "backface cull", and drag fields for specularity and exposure (0 to 10). It replaces the Controls list.
    - Left mouse in the main window still moves the light.
    - Behavior change: car rotation is no longer timed by `fElapsedTime`, and the old W-key limit on how close the car could get (z > -0.99) is gone.

16. **User:** Changed the default view.
    - The car starts at (0, 0, 0).
    - The camera starts at (-2, 3, -2), with yaw -0.78 and pitch 0.70.

17. **AI:** Performance pass 1. The output is pixel-identical, and FPS went from about 20 to 31 (50 → 32 ms per frame).
    - `screen()` caches `1/tan(fov)` in statics and only recomputes it when the FOV changes.
    - `Shade_triangle` computes `pow(x, 8)` as three multiplies. It caches `log10(exposure…)` in statics and only recomputes it when exposure changes.
    - `Shade_triangle` and `isTriangleCulled` now take the triangle by `const&` instead of copying it.
    - `transformed_tri` is now an `Engine3D` member. It's cleared and reserved each frame instead of reallocated.
    - Note: the static caches aren't thread-safe. They'll need changing if rendering goes multithreaded.

18. **AI:** Added Gouraud and Phong shading as options.
    - `Shading.h`:
      - The lighting formula moved out of `Shade_triangle` into a new `Shade_normal()`, which lights one normal. Flat shading's output is unchanged (verified pixel-identical).
      - Added `face_normal()` and `vertex_normal()`. `vertex_normal()` returns the `.obj` normal flipped to match `face_normal`, or the face normal if the file has none.
      - Added the `ShadingMode` enum: `SHADING_FLAT`, `SHADING_GOURAUD` and `SHADING_PHONG`.
    - `Framebuffer.h`: added `Fill_triangle_smooth()`, a bounding-box fill. It gives each pixel three corner weights and calls back to get the pixel's color.
      - It draws half a pixel past each edge to cover small cracks between triangles. The scanline fill hides those cracks the same way.
    - `Engine3D`: added a `shading` member, which picks the mode per triangle.
      - Gouraud lights the three corners and blends the colors across the triangle.
      - Phong blends the three normals and lights every pixel.
    - Menu: added a flat / gouraud / phong radio switch. The checkbox is renamed "calculate normals (flat only)".
    - Speed on `bmw.obj`: flat about 32 FPS, Gouraud about 17, Phong about 18.
    - Backface culling was checked and is correct (on = back faces removed), so it wasn't changed.

19. **AI:** Performance pass 2: Gouraud and Phong now shade each pixel only once, and there's a render scale option.
    - Deferred shading:
      - `Fill_triangle_deferred()` replaced `Fill_triangle_smooth()`. It writes only depth plus the winning triangle and weights, into the new `tri_id`, `w0_buffer` and `w1_buffer`.
      - After all the triangles are drawn, `Engine3D` colors each pixel once from its winning triangle.
      - Corner normals and Gouraud corner colors are computed lazily, only for triangles that win a pixel. They're stored in `corner_normals`, `corner_colors` and `corners_ready`.
    - The smooth fill's bounding box no longer adds a 1 px border. That border let pixels beyond sharp corners get drawn. A few thousand pixels changed, and no cracks came back.
    - Render scale:
      - `Framebuffer` now has a runtime `width` and `height` and a `Resize()` method.
      - `screen()` takes the buffer size. `Draw()` recreates the texture when the size changes, and SDL stretches it to the window.
      - Added a "render scale" slider (0.25 to 1) to the menu. The light circle's radius scales with it.
    - Flat output is unchanged (pixel-identical). Gouraud differs by tiny rounding in the blend weights.
    - Speed, same CPU state: Gouraud default 199 → 111 ms, Phong default 172 → 136 ms, Phong close-up 644 → 443 ms. Render scale 0.5 roughly halves the time again.
