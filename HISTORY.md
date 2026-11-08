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

20. **User:** Copied the `full_model` textures into `Object/Textures/`.

21. **AI:** Changed the `map_Kd` texture paths in `Object/full_model.mtl` from absolute `Downloads` paths to `Textures/...`, relative to the `.mtl`.

22. **AI:** Added texture mapping.
    - Added `stb_image.h` v2.30 in `headers/stb/`, pinned to commit `adc1672` (Feb 2025), for loading PNGs. Its implementation is compiled once at the end of `main.cpp`'s includes.
    - Added `src/Texture.h`: a `Texture` struct with nearest-pixel `sample(u, v)`, which wraps outside 0–1 and flips v, plus `load_texture()`.
    - `Mesh.h`:
      - Added a `UV` struct.
      - `Triangle` gained `uv[3]` and `texture`, an index into the new `Mesh::textures`.
      - `transform_triangle` copies both.
    - `ObjLoader.h`:
      - Parses `vt`, `mtllib` and `usemtl`, and the vt index in faces.
      - The new `load_mtl()` reads `newmtl` and `map_Kd`, and loads each image once. Paths are relative to the `.mtl`.
    - `Shading.h`: split `Shade_normal` into `Light_intensity()` and `Shade_intensity()`. The output is unchanged.
    - Texture use by mode:
      - Flat uses the texture color at the triangle's center.
      - Phong uses the texture per pixel.
      - Gouraud now blends the corner light amounts (`corner_light`) instead of the corner colors, then shades each pixel with the texture.
      - Untextured Gouraud on the car changed slightly: mean difference about 1/255, but up to 81/255 on some highlights.
    - Texture coordinates are perspective-corrected: weights are divided by corner depth, then renormalized.
    - Menu: added a "textures" checkbox.
    - `OnUserCreate` now loads `Object/full_model.obj`. The `bmw.obj` line is kept, commented out.
    - Loading takes about 0.7 s (three 2048×2048 textures). Rendering takes about 11–18 ms per frame, depending on mode.

23. **AI:** Updated `README.md` to match the current code, keeping its original style.
    - Updated the library list, the functions list, the WIP list, the controls (now the menu), and the model path link (now `src/Engine3D.h`).
    - Changed the AI note to say that AI also wrote parts of the code, pointing to `HISTORY.md`.

24. **User:** Asked to remove the AI line from `README.md`. **AI:** Removed it. The boilerplate sentence stays.

25. **AI:** Added a Blender-style mouse camera.
    - `Camera` is now an orbit camera with `target` and `distance`. `update_orbit()` computes `position` from them, plus yaw and pitch, at the start of every frame.
    - Brought back `Engine3D::HandleEvent(e)`, which `main()` calls for every event. It only acts on the main window:
      - Middle-drag orbits: x changes yaw, y changes pitch, and pitch is clamped to ±1.55.
      - Shift + middle-drag pans: the target moves along the camera's right and up axes, scaled so the scene follows the mouse.
      - The wheel zooms by multiplying the distance by 0.9 or 1.1.
    - The default view is unchanged. Target (0, 0.62, 0) at distance 3.7 gives a position of about (-2, 3, -2) with the same yaw and pitch.
    - Menu: the camera's position field was replaced by target and distance, because position is now computed. Added the mouse controls to the menu text.
    - `README.md`: added the mouse controls and removed that item from WIP.

26. **AI:** Made rendering multithreaded. The output is pixel-identical to single-threaded on both models, all modes, near and far.
    - Added `src/Parallel.h`:
      - `parallel_for(count, fn)` runs on all cores. Threads keep taking the next job, which spreads the work evenly.
      - `parallel_for_chunks()` hands out ranges of jobs.
    - `Engine3D` now renders in three parallel steps:
      1. **Per-triangle pass:** transform, cull, project, then work out the flat color or the corner data. It fills the new `visible`, `projected` and `flat_colors` arrays. The lazy `corners_ready` was removed.
      2. **Fill:** the screen is split into 32-row strips, and each thread fills only its own rows, so no two threads touch the same pixel. Triangles are still drawn in the same order.
      3. **Gouraud/Phong resolve:** split by the same strips.
    - The static FOV and exposure caches are filled before the threads start, so the threads only read them.
    - `Framebuffer`:
      - `Fill_triangle` and `Fill_triangle_deferred` now take already-projected corners plus a row range (`y0`, `y1`).
      - The flat-top and flat-bottom fills clip their row loop to that range.
    - Speed, comparing runs back to back:
      - Car: flat 43 → 19 ms, Gouraud 71 → 28 ms, Phong 75 → 29 ms.
      - Car close-up: Gouraud 200 → 60 ms, Phong 230 → 66 ms.
      - Character model: about 1.2–2.7× faster.
    - `README.md`: added multithreading to the functions list. Raytraced shadows replace it in the WIP list.

27. **AI:** Added simple ray tracing as a "ray tracing" checkbox in the menu. When it's on, it replaces triangle filling, and the shading options are greyed out.
    - Added `src/RayTracer.h`:
      - `Ray` and `Hit` structs.
      - `ray_triangle()`, the Möller–Trumbore test.
      - `ray_box()`, a slab test.
      - A median-split `BVH`: a box tree with up to 4 triangles per leaf, built once in `OnUserCreate` in the mesh's own coordinates. It has a closest-hit and an any-hit `intersect()`.
    - Added `Mat4::inverse_rigid()`, used to move rays into mesh coordinates instead of moving the mesh.
    - Added `Engine3D::render_raytraced()`. It fires one ray per pixel, in parallel by 32-row strips:
      - **Hits the mesh:** gets a smooth normal from the hit weights, like Phong, and an exact texture coordinate. One shadow ray goes toward the light, and a blocked point gets 0.15 light.
      - **Misses but hits the floor at y = 0 within ±10:** if the mesh blocks the light there, the existing floor pixel is darkened to 60%. The grid and axes are drawn before this, so they stay visible.
    - Rasterized output is unchanged: pixel-identical in all 12 test cases with ray tracing off.
    - Speed with 12 threads:
      - Character model: about 57 ms at full resolution, 20 ms at render scale 0.5.
      - Car: about 395 ms at full resolution, 120 ms at 0.5.
      - BVH build: 8 ms for the character, 144 ms for the car.
    - `README.md`: added ray tracing to the functions and controls. WIP now lists reflections and soft shadows.

28. **User:** asked for antialiasing.
    **AI:** added supersampling antialiasing (SSAA), an "antialiasing (2x2)" checkbox in the menu.
    - When on, the frame is drawn 2x wider and taller, so there are 4 samples per pixel. `Framebuffer::Draw()` then averages each 2x2 block into one shown pixel (in parallel) before sending it to SDL.
    - Works the same for flat, Gouraud, Phong and ray tracing. The light circle is scaled up to keep its size. The grid lines come out thinner and softer.
    - Limited to 2x2: at 2880x1800, 3x3 would need about 1 GB of buffers.
    - Cost is about 3x the frame time (character close-up: Phong 18 → 50 ms, ray tracing 100 → 365 ms). Lowering the render scale offsets it.

29. **User:** asked how to make materials (mainly metallic, glossy and diffuse), then said to implement the suggested plan.
    **AI:** added materials, a new lighting model, reflections in the ray tracer and a material menu.
    - **Material** (new `src/Material.h`): name, color, metallic (0..1) and roughness (0..1). Diffuse = metallic 0, roughness 1. Glossy = metallic 0, low roughness. Metallic = metallic 1, low roughness.
    - **Loading:** `Mesh` has `materials` and every `Triangle` has a `material` index. `load_mtl` now also reads `Kd` (color), `Ns` (converted to roughness the way Blender exports it, 1 - sqrt(Ns/1000)), and `Pr`/`Pm` (roughness/metallic) if present. Triangles with no known material get a default one (green, roughness 0.5), so there's always at least one.
    - **Lighting:** the old `Light_intensity` / `Shade_intensity` / `Shade_normal` / `Shade_triangle` (fixed x^8 highlight and the global specularity) are replaced by two steps in `Shading.h`:
      - `Light_terms(normal, light, view direction, roughness)` gives the diffuse amount (still 0.15..1), a Blinn-Phong highlight whose sharpness comes from roughness, and how much the surface faces the camera.
      - `Shade_material(terms, base color, material, reflection, exposure_scale)` mixes them. Metals have no diffuse and a highlight tinted by their color, while non-metals get a 4% white highlight. Everything reflects more when seen edge-on (Fresnel), rough surfaces less so.
      - Flat works these out once per triangle, Gouraud at the corners and blends the 3 values, and Phong and ray tracing per pixel. The rasterizer has no reflection rays, so mirrors see the grey background.
    - **Ray tracer:** the per-pixel code moved into `trace(ray, light, depth, out)`. Materials with roughness < 0.5 send a reflection ray (up to 2 bounces) and mix what it hits with the background; rougher ones just reflect the background.
    - **Base color:** the material's color is used when there's no texture or textures are off (it used to be fixed green).
    - **Exposure:** `exposure_scale` is computed once per frame in the engine, which replaces the static cache in `Shade_intensity`.
    - **Menu:** the specularity slider is removed. A "Materials" list (click to open) shows each material, and clicking one shows its color, metallic and roughness.
    - Verified on a smooth test sphere: diffuse, glossy and metal look the same in flat, Gouraud, Phong and ray tracing. The rasterized images differ from before, as expected, because the lighting model changed.

30. **User:** asked to support many objects: load every .obj in the `Obj_load` folder, with textures in a folder named `<file name>_tex`.
    **AI:** replaced the single mesh with a list of objects.
    - **`SceneObject`** (new `src/SceneObject.h`): name (the file name), mesh, world transform and BVH.
      - `load_folder("Obj_load")` loads every `.obj` there in name order and builds each object's BVH.
      - It places the objects side by side along x (0.5 apart) and centers the row on x = 0.
    - **Textures:** the `.obj`/`.mtl` loader takes a texture folder (`Obj_load/<name>_tex/`). For each `map_Kd` it first looks there by file name, then falls back to the path the `.mtl` gives.
    - **Rasterizer:** `Engine3D` has `objects` instead of `mesh`/`object`/`bvh`.
      - `tri_refs` lists every object's triangles as one list (object, index), so the 3 parallel steps work as before; each triangle uses its own object's matrix, materials and textures.
      - Per object, the model, inverse and model-view matrices are worked out once per frame.
    - **Ray tracing:** now done in world coordinates.
      - `intersect_scene` moves the ray into each object's coordinates and tests its BVH, keeping the closest hit. Moving/rotating keeps lengths, so distances compare across objects.
      - Normals are moved back to world. Shadows and reflections work between objects.
    - **Menu:** lists Light, Camera, then every object by name. Clicking a name shows its transform. Its arrow opens its materials.
    - Set up `Obj_load/` with copies of `full_model` (textures in `full_model_tex/`) and `monkey`. `Object/` is untouched.
    - `README.md`: how to add models, and multiple objects moved from WIP to the functions.

31. **User:** in ray tracing, the top of a box was lit while the light marker sat inside the box. Asked for 2 light types in `Light.h`: directional and point.
    **AI:** this wasn't a bug: the light was directional (a sun infinitely far away), and the marker only showed its direction. Added a point light next to it.
    - **`Light`** now has `type` (`LIGHT_DIRECTIONAL` / `LIGHT_POINT`), `target` (where a directional light shines to; it used to always be the origin) and `strength` (point only).
      - `direction_at(point)`: one direction for the sun, or from the bulb to the point.
      - `strength_at(point)`: 1 for the sun, or `strength / distance²` for the bulb.
      - `distance_to(point)`: infinite for the sun, the real distance for the bulb.
    - **`Light_terms`** takes the strength. Diffuse is now `max(n·l * strength, 0.15)`: still 0.15..1 for the sun, but a close bulb can go above 1. The highlight is scaled the same way.
    - **Rasterizer:** a copy of the light is moved into camera coordinates (position and target) each frame. Flat, Gouraud and Phong ask it for the direction and strength at the triangle center, corner or pixel.
    - **Ray tracer:** new `in_shadow(point)` is used for surfaces and the floor. The shadow ray only counts blockers closer than the light, so a wall behind a bulb doesn't shadow it. `trace` no longer takes a light direction.
    - **Display and controls:** a directional light draws a yellow line from its position to its target. Left-dragging now moves the light around its target instead of the origin.
    - **Menu:** under Light there's a directional/point switch, then position, plus target (directional) or strength (point).
    - Checked with a point light inside a box: the outside top and walls stay dark, the inside is lit, and in ray tracing the box shadows the floor around it.
