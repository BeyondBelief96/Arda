# Renderer: Chapter 3 Build Plan

A step-by-step plan for building `arda_renderer` while reading Chapter 3
("Renderer Design") of *3D Engine Design for Virtual Globes*, using
[OpenGlobe's renderer](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Renderer)
as the reference.

The renderer is a set of **abstract base classes** with backend
implementations. **OpenGL 3.3** (`src/gl/`) is built first and
**Direct3D 11** (`src/d3d11/`) comes later.

Each step has its own guide in [`docs/`](docs). A guide has:

- the book sections to read and the OpenGlobe files to compare against
- every file to create or change, **in full**, in the order to build them,
  with checkpoints where the code compiles partway through
- explanations of the OpenGL behind each call, the design choices (and the
  alternatives that were rejected), and the C++ features as they first appear
- CMake changes, including the complete CMake files as of that step, plus
  tests and a milestone to check before moving on
- a **D3D11 check**: how Direct3D 11 implements the same interface

### How to read the guides

The guides use four kinds of callout:

- `> **OpenGL note — topic:**` what GL (or D3D11 in Step 8) is actually doing
- `> **C++ note — topic:**` a language or library feature, explained the first
  time it's used. Later guides link back instead of repeating it. Step 0 has
  an [index of its notes](docs/00-setup.md#c-and-opengl-notes-index), and each
  note has a stable anchor, such as `00-setup.md#cpp-raii`.
- `> **Why:**` the reason for a design decision
- `> **Math note — topic:**` (Step 4) the transforms behind the camera

Type the code in yourself rather than copying whole files. Build at each
checkpoint, and read the notes when something is unfamiliar. The C# names
and line numbers are there so you can read OpenGlobe side by side.

| Step | Guide | Book | Milestone |
|---|---|---|---|
| 0 | [Setup](docs/00-setup.md) | 3.1–3.2 | Window opens through `CreateDevice` |
| 1 | [State management](docs/01-state-management.md) | 3.3 | `Context::Clear` clears the window |
| 2 | [Shaders](docs/02-shaders.md) | 3.4.1–3.4.4 | Test compiles a shader and finds its uniforms |
| 3 | [Vertex data](docs/03-vertex-data.md) | 3.5 | First triangle (raw buffers, then a `Mesh`) |
| 4 | [Automatic uniforms](docs/04-automatic-uniforms.md) | 3.4.5–3.4.6 | Triangle viewed through a `Camera` |
| 5 | [Textures](docs/05-textures.md) | 3.6 | Textured triangle |
| 6 | [Framebuffers](docs/06-framebuffers.md) | 3.7 | Render to texture, then draw it on screen |
| 7 | [Chapter 3 example](docs/07-triangle-example.md) | 3.8 | `Chapter03Triangle` ported |
| 8 | [Direct3D 11 backend](docs/08-direct3d11.md) | 3.5.5, 3.6.5, 3.7.2 | Same milestones on D3D11 |

> **Build order vs. reading order:** read the chapter in order, but build the
> automatic uniforms (3.4.5) *after* vertex data (3.5). A triangle gets on
> screen sooner that way, and 3.4.5 needs a matrix type and a `Camera` first.

---

## Architecture

### Directory layout

```
renderer/
  include/arda/renderer/          Public headers. Never include glad, GLFW, d3d11.h or <windows.h>.
    Device.h  Context.h  GraphicsWindow.h  GraphicsApi.h  Rectangle.h  Color.h  ...
    renderstate/  shaders/  buffers/  vertexarray/  mesh/  textures/  framebuffer/  scene/
  src/                            API-agnostic code: base-class helpers, automatic uniforms,
                                  mesh buffers, Camera, SceneState, ShaderCache, Image
  src/gl/                         OpenGL 3.3 backend. Private headers; may include glad and GLFW.
  src/d3d11/                      Direct3D 11 backend (Step 8). Private headers; may include d3d11.h.
  docs/                           The step guides
```

- **Namespaces:** public types are in `arda::renderer`. The OpenGL backend is
  in `arda::renderer::gl` and the D3D11 backend in `arda::renderer::d3d11`.
- **Class names:** a public abstract class `Foo` has implementations `FooGL3x`
  (in `src/gl/`) and `FooD3D11` (in `src/d3d11/`), matching OpenGlobe's names.

### Backend selection

```cpp
auto device = arda::renderer::CreateDevice(arda::renderer::GraphicsApi::OpenGL33);
auto window = device->CreateGraphicsWindow(1280, 720, "Arda");   // the window owns a Context
arda::renderer::Context& context = window->GetContext();
```

- **`Device`** creates resources that every window can use: shaders, buffers,
  textures and samplers. It is like `ID3D11Device`.
  - `DeviceGL3x` keeps a hidden 1×1 GLFW window. Every real window passes it
    as the `share` argument to `glfwCreateWindow`, so GL objects are shared
    between contexts (3.2).
- **`Context`** issues `Clear` and `Draw`, and creates the objects that can't
  be shared: vertex arrays and framebuffers. It is like `ID3D11DeviceContext`.
- **Destruction order:** destroy windows and resources before the device. In
  `main`, declare the device first so it is destroyed last.

### Class patterns

1. **The non-virtual interface pattern for `Device` and `Context`.** Public
   functions are non-virtual. They validate their arguments, then call a
   protected pure virtual `Do*` function that each backend implements.
   Validation is written once, and public overloads never hide each other in
   derived classes.
   ```cpp
   void Context::Draw(PrimitiveType type, const DrawState& ds, const SceneState& ss) {
       VerifyDraw(ds);        // shared checks
       DoDraw(type, ds, ss);  // backend work
   }
   ```
2. **Base classes hold API-agnostic state.**
   - `VertexArray`, `Framebuffer`, `TextureUnits` and `ShaderProgram` store
     their attributes, attachments and uniform collections in the base class,
     along with "dirty" flags that mark what has changed.
   - Backends only apply dirty items to the GL or D3D API before drawing
     (3.5.4, 3.6.4, 3.7.1).
3. **Typed templates call byte-level virtual functions.** A template can't be
   virtual, so buffers use a non-virtual template
   (`CopyFromSystemMemory(const R& values)`) that forwards to
   `virtual CopyFromSystemMemoryBytes(std::span<const std::byte>)`.
4. **Backends cast other objects to their own types**, for example
   `static_cast<VertexArrayGL3x&>(*drawState.vertexArray)`. This is the same
   downcast OpenGlobe does.
5. **Each backend has a type converter.** Public enums are arda's own. Only
   `src/gl/TypeConverterGL3x` (and later `src/d3d11/TypeConverterD3D11`) knows
   GL or D3D values.

### Naming conventions

| Kind | Convention | Example |
|---|---|---|
| Plain data structs (`RenderState`, `ClearState`, `DrawState`, `Mesh`, ...) | public camelCase fields | `renderState.depthTest.enabled = false;` |
| Classes | PascalCase methods, `m_` private members | `window->ShouldClose()` |
| A getter whose name is also a type | `Get` prefix | `GetContext()`, `GetFramebuffer()`, `GetTextureUnits()` |
| Setters | `Set` prefix | `SetViewport(...)` |
| Class template and its non-template base | `FooBase` + `Foo<T>` | `UniformBase` + `Uniform<T>` |

Two conventions here avoid real compile errors:

- **camelCase fields:** in C++, a field declared as `DepthTest DepthTest;`
  changes what the name `DepthTest` means inside the struct, and GCC rejects
  it. camelCase field names avoid the clash.
- **No function named `CreateWindow`:** `<windows.h>` defines `CreateWindow`
  as a macro, so the device's function is `CreateGraphicsWindow`.

### Ownership

- **Windows:** `Device::CreateGraphicsWindow` returns `std::unique_ptr<GraphicsWindow>`.
- **Resources:** shaders, buffers, vertex arrays, textures, samplers and
  framebuffers are returned as `std::shared_ptr<T>`. Containers
  (`DrawState`, `VertexArray`, `Framebuffer`, `TextureUnit`) keep them alive,
  which does the job of C#'s garbage collector and `Disposable`.
- **GL object IDs:** `src/gl/GLHandle.h` is one move-only RAII type that
  replaces `GL3x/Names/*`.
- **D3D objects:** `Microsoft::WRL::ComPtr<T>`.

### CMake layout (grows each step)

```cmake
add_library(arda_renderer
    src/Device.cpp
    src/Context.cpp
    src/GraphicsWindow.cpp
    # ... API-agnostic sources added by each step
)
target_include_directories(arda_renderer PUBLIC include PRIVATE src)
target_link_libraries(arda_renderer PUBLIC arda_core PRIVATE glfw)

option(ARDA_RENDERER_GL    "Build the OpenGL 3.3 backend"  ON)
option(ARDA_RENDERER_D3D11 "Build the Direct3D 11 backend" OFF)

if(ARDA_RENDERER_GL)
    target_sources(arda_renderer PRIVATE
        src/gl/DeviceGL3x.cpp
        # ... GL sources added by each step
    )
    target_link_libraries(arda_renderer PRIVATE glad::glad)
    target_compile_definitions(arda_renderer PRIVATE ARDA_HAS_GL=1)
endif()

if(ARDA_RENDERER_D3D11)
    # Step 8
endif()
```

---

## Portability decisions

These affect public interfaces, so they are settled here. Each step guide
applies them.

| Topic | GL 3.3 | D3D 11 | Decision |
|---|---|---|---|
| Shader language | GLSL | HLSL | `CreateShaderProgram` takes source in the backend's language. The app picks the source with `device->Api()`. |
| Loose uniforms | `glUniform*` | Loose globals are packed into the `$Globals` constant buffer | Keep `Uniform<T>`. D3D11 writes into a CPU copy of `$Globals` and uploads it before the draw if it changed. |
| Matrix layout | Column-major | Column-major by default too, but `#pragma pack_matrix` or `row_major` can change it | Compile HLSL with `D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR` to make it explicit, so both backends upload the same bytes, and write `mul(M, v)` to match GLSL's `M * v`. |
| Clip-space depth | z in [-1, 1] | z in [0, 1] | `Device::ClipDepthRange()`. Projection functions take a `core::ClipDepth` parameter. |
| Clears | Scissor, color mask and depth mask affect `glClear` | They don't affect clears | `ClearState` has no scissor or mask fields. The GL backend resets them before `glClear` (3.3.5). |
| Primitive restart | Any index | Fixed index, strips only | `PrimitiveRestart` has only `enabled`. The index is the maximum value for the index type. |
| `LineLoop`, `TriangleFan` | Supported | Not supported | Keep them in `PrimitiveType`. D3D11 throws. |
| Vertex input names | Attribute name plus location | Semantic name plus semantic index | HLSL inputs are written `name : nameN`, where N is the location (`float4 position : position0`). D3D11 reads both from reflection. |
| Fragment outputs | `layout(location = N) out` | `SV_TargetN` | Location N in both. |
| Sampler binding | `sampler2D` uniform set to a texture unit | Registers `tN` and `sN` | Texture unit N is `tN`/`sN`. The `og_textureN` link automatics simply never match in HLSL. |
| Viewport origin | Bottom-left | Top-left | `Rectangle` is bottom-left. D3D11 converts it. |
| Render-to-texture orientation | Row 0 at the bottom | Row 0 at the top | While a framebuffer is bound, D3D11 flips clip-space Y (and the front-face winding) through `Context::ClipSpaceTransform()` (Step 8). |
| 3-channel textures | `GL_RGB8`, `GL_RGB16F`, ... | No such DXGI format | D3D11 stores RGB8/16/16f/32f as 4-channel formats and pads with alpha = 1 on upload. 3-channel integer formats throw. |
| 3-component 8/16-bit vertex attributes | Supported | No such DXGI vertex format | D3D11 throws. Use 4 components (e.g. `VertexAttributeRGBA`) for portable meshes. |
| `CullFace::FrontAndBack` | Supported | Not supported | D3D11 throws. |
| Separate front/back stencil masks or reference values | Supported | One mask and one reference for both faces | D3D11 throws if the two faces differ in those fields. |
| Fragment output names | `glGetFragDataLocation` | Reflection has no output names | D3D11 reads `name : SV_TargetN` from the source, or assumes `SV_Target0`. |

---

## Not needed for Chapter 3

| OpenGlobe code | When it's needed |
|---|---|
| `Synchronization/Fence`, `GL3x/Names/FinalizerThreadContextGL3x` | Chapter 10 (multithreading). The second is specific to C#'s garbage collector. |
| `Input/*` | When camera controllers are added (GLFW input works for both backends) |
| `Tools/TextureAtlas`, `Device.CreateBitmapFromText` / `FromPoint` | Chapter 9 (billboards) |
| `Buffers/UniformBuffer`, `Shaders/UniformBlock*` | Not used in the book |
| Most of `Shaders/DrawAutomaticUniforms/*` | Add each one when a later chapter uses it |
| `GL3x/GLSL/BuiltinFunctions.glsl` | Chapters 5 and 7 (emulated doubles, wide lines) |
| `Extensions`, `HighResolutionSnap*`, texture rectangles | Chapter 11 and later |

---

## Why glad, GLFW and D3D headers are private

`#include` copies text. Anything a public header includes is also compiled by
every file that includes that header, such as `scene/src/main.cpp`.

- **If a public header included `<GLFW/glfw3.h>`, `<glad/glad.h>` or
  `<d3d11.h>`,** every user of the renderer would need those include paths,
  and CMake would have to link them as `PUBLIC`. GL macros and `<windows.h>`
  macros (such as `min`, `max` and `CreateWindow`) would reach the scene code.
- **A forward declaration (`struct GLFWwindow;`)** only says the type exists.
  A pointer has the same size whatever it points to, so the compiler can still
  lay out a class that holds one. Only `.cpp` files and private backend
  headers include the real API headers.

**What a forward declaration allows:** declaring pointers or references to the
type, and naming it in function declarations.

**What needs the full definition:** a member stored by value, `sizeof`,
calling functions on it, or accessing its fields.

**How abstract base classes help:** public headers declare only `Context`,
`VertexBuffer` and so on. `ContextGL3x` (which stores `GLuint`s) and
`ContextD3D11` (which stores `ComPtr`s) live in `src/gl/` and `src/d3d11/`.
Callers only ever hold a base-class pointer or reference.

`target_link_libraries(... PRIVATE glfw glad::glad)`: only `arda_renderer`'s
own sources get those include paths. Because `arda_renderer` is a static
library, CMake still adds them to the final link of `arda_scene`
automatically.
