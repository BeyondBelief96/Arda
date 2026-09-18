# Step 7: Putting it all together, `Chapter03Triangle` (3.8)

**Goal:** port the book's example program, `Triangle.cs`, to
`scene/src/main.cpp`. It uses only the public renderer types and makes no GL
calls. After Step 8, the same program runs on Direct3D 11 when you change one
argument.

**Read:** 3.8 "Putting It All Together: Rendering a Triangle" (Listings
3.30–3.32).

This step adds no new renderer code. It's a review: every line of the example
exercises something from Steps 0–4. For each piece, this guide says what it
is, which step built it, and what happens inside the renderer when it runs.
If you've finished Steps 0–4, you can build this now. Steps 5 and 6 are only
needed for the "Try This" exercises at the end.

## OpenGlobe reference

| File | What to take from it |
|---|---|
| [Examples/Chapter03/Triangle/Triangle.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Examples/Chapter03/Triangle/Triangle.cs) | The entire example |

## Files

```
scene/
  src/main.cpp            REWRITE   the Triangle example
  shaders/                OPTIONAL  shader source files; see "Loading shaders from files"
  CMakeLists.txt          OPTIONAL  copies shaders/ next to the executable
```

---

## 1. The C# program, section by section

`Triangle.cs` is a single class, `Triangle`, that implements `IDisposable`.
Its constructor sets up everything, `OnResize` and `OnRenderFrame` are
event handlers, and `Dispose` releases GPU resources. `Main` creates one and
runs it:

```csharp
static void Main()
{
    using (Triangle example = new Triangle())
    {
        example.Run(30.0);
    }
}
```

The C++ port keeps this shape: a `Triangle` class with a constructor,
`OnResize`, `OnRenderFrame` and `Run`. Keeping the book's structure makes
it easy to compare the two side by side. It also shows the main difference
between the languages: C++ has no `Dispose`, because the destructor does
that job automatically.

The constructor does six things, in this order:

1. Creates a window, and hooks up the resize and render handlers.
2. Compiles a shader program and sets its `u_color` uniform.
3. Builds a `Mesh` with three positions and three indices.
4. Creates a vertex array from the mesh for this window's context.
5. Creates a `RenderState` with culling and depth testing off, and bundles
   it with the program and the vertex array into a `DrawState`.
6. Zooms the camera so the triangle fills the view.

---

## 2. The complete `scene/src/main.cpp`

Replace `scene/src/main.cpp` with the program below, then read the
walkthrough in section 3.

```cpp
#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>

#include <cstdio>
#include <exception>
#include <memory>
#include <string_view>

namespace {

using namespace arda::renderer;
using arda::core::Vector3;
using arda::core::geometry::IndicesUnsignedShort;
using arda::core::geometry::Mesh;
using arda::core::geometry::PrimitiveType;
using arda::core::geometry::VertexAttributeFloatVector3;

struct ShaderSources {
    std::string_view vertex;
    std::string_view fragment;
};

// Listing 3.31. Each backend compiles its own shader language, so the
// application picks the source that matches the device.
ShaderSources TriangleShaders(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL33:
        return {
            R"(#version 330

               layout(location = og_positionVertexLocation) in vec4 position;
               uniform mat4 og_modelViewPerspectiveMatrix;

               void main()
               {
                   gl_Position = og_modelViewPerspectiveMatrix * position;
               })",
            R"(#version 330

               out vec3 fragmentColor;
               uniform vec3 u_color;

               void main()
               {
                   fragmentColor = u_color;
               })",
        };
    case GraphicsApi::Direct3D11:
        return {
            R"(float4x4 og_modelViewPerspectiveMatrix;

               float4 main(float4 position : position0) : SV_Position
               {
                   return mul(og_modelViewPerspectiveMatrix, position);
               })",
            R"(float3 u_color;

               void main(out float4 fragmentColor : SV_Target0)
               {
                   fragmentColor = float4(u_color, 1.0);
               })",
        };
    }
    return {};
}

// The book's Chapter 3 example (Listings 3.30-3.32).
class Triangle {
public:
    explicit Triangle(GraphicsApi api);

    // The handlers capture `this`, so a Triangle must never be copied or moved.
    Triangle(const Triangle&)            = delete;
    Triangle& operator=(const Triangle&) = delete;

    void Run() { m_window->Run(); }

private:
    void OnResize();
    void OnRenderFrame();

    // Declaration order is construction order; destruction is the reverse.
    // The device is created first and destroyed last, after everything it made.
    std::unique_ptr<Device> m_device;
    std::unique_ptr<GraphicsWindow> m_window;
    SceneState m_sceneState;
    ClearState m_clearState;
    DrawState m_drawState;   // destroyed first: releases the vertex array and program
};

Triangle::Triangle(GraphicsApi api)
    : m_device(CreateDevice(api)),
      m_window(m_device->CreateGraphicsWindow(800, 600, "Chapter 3:  Triangle")) {
    m_window->SetResizeHandler([this] { OnResize(); });
    m_window->SetRenderFrameHandler([this] { OnRenderFrame(); });

    // Shader program (3.4)
    const ShaderSources sources = TriangleShaders(api);
    std::shared_ptr<ShaderProgram> sp = m_device->CreateShaderProgram(sources.vertex, sources.fragment);
    sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));

    // Mesh (3.5.6): an isosceles right triangle in the xz plane with legs of length 1.
    Mesh mesh;

    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(0.0f, 0.0f, 0.0f);
    positions.emplace_back(1.0f, 0.0f, 0.0f);
    positions.emplace_back(0.0f, 0.0f, 1.0f);

    auto indices = std::make_unique<IndicesUnsignedShort>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);

    // Vertex array (3.5.3): uploads the mesh and matches "position" to the shader's input.
    std::shared_ptr<VertexArray> va =
        m_window->GetContext().CreateVertexArray(mesh, sp->VertexAttributes(), BufferHint::StaticDraw);

    // Render state (3.3). Culling and depth testing are on by default in arda,
    // unlike GL. A single triangle needs neither.
    RenderState renderState;
    renderState.facetCulling.enabled = false;
    renderState.depthTest.enabled = false;

    m_drawState = DrawState{renderState, sp, va};

    // Camera (3.4.5): back away until a sphere of radius 1 around the target fits the view.
    m_sceneState.camera.ZoomToTarget(1.0);
}

void Triangle::OnResize() {
    m_window->GetContext().SetViewport({0, 0, m_window->Width(), m_window->Height()});
    m_sceneState.camera.aspectRatio = m_window->Width() / static_cast<double>(m_window->Height());
}

// Listing 3.32
void Triangle::OnRenderFrame() {
    Context& context = m_window->GetContext();
    context.Clear(m_clearState);
    context.Draw(PrimitiveType::Triangles, m_drawState, m_sceneState);
}

} // namespace

int main() {
    try {
        Triangle example(GraphicsApi::OpenGL33);   // Step 8: GraphicsApi::Direct3D11
        example.Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

**Build and run:**

```sh
./run.sh
```

You should see a red right triangle on a white background (`ClearState`'s
default color is white, like OpenGlobe's). The right angle sits at the
center of the window, one leg points right and the other points up. Resize the
window: the triangle keeps its shape and stays centered. Minimize and restore
it: nothing throws. Close it: the program exits with code 0 and prints
nothing.

If something is wrong, see [Troubleshooting](#7-troubleshooting).

---

## 3. Walkthrough

### 3.1 The includes and the anonymous namespace

Every header is public (`arda/core/...` or `arda/renderer/...`). None is
under `src/`, and nothing includes glad, GLFW or `<d3d11.h>`. That's the whole
point of Chapter 3: the application talks to the *renderer*, not to a graphics
API. `scene`'s `CMakeLists.txt` links only `arda_renderer`, and it can't see
`renderer/src` even if it tried. See the README's "Why glad, GLFW and D3D
headers are private".

`ShaderSources`, `TriangleShaders` and `Triangle` are in an anonymous
namespace. They're used only in this file, so they get internal linkage
(see [Step 0: anonymous namespaces](00-setup.md#cpp-anonymous-namespaces)).
`main` must stay outside it, because the linker looks for a global `main`.

The `using` declarations are inside the anonymous namespace, not at file
scope, so they don't leak into anything else. In a header you would never
write `using namespace`. In a `.cpp` file that's only a style choice, but
naming exactly what you use (`using arda::core::geometry::Mesh;`) makes it
clear where each name comes from.

### 3.2 `TriangleShaders`: one program, two shader languages

C# had one `vs` and one `fs` string, because OpenGlobe only had GL. arda
can have a GL device or a D3D11 device, and each compiles its own language.
The README's portability table settles this: `CreateShaderProgram` takes
source code in the backend's language, and the application chooses the source
based on `device->Api()`. Here, the constructor's `api` parameter serves the
same purpose.

The GLSL is Listing 3.31, unchanged:

- `#version 330` is allowed. The GL backend's prelude already begins with
  `#version 330`, so it comments out your copy (see
  [Step 2: `#version` and the prelude](02-shaders.md)).
- `og_positionVertexLocation` is a built-in `#define` from the prelude,
  which the renderer defines as `0` (Step 2). `layout(location = ...)` fixes
  the attribute's location, so `VertexArray` slot 0 feeds `position`.
- `og_modelViewPerspectiveMatrix` is a *draw automatic uniform* (Step 4).
  The application never sets it. Before each draw, the renderer computes
  `perspective × view × model` from `SceneState` and uploads it.
- `in vec4 position` receives a 3-component attribute. GL fills in the
  missing `w` with 1, which is exactly what a position needs.
- `u_color` is an ordinary uniform, set once in the constructor.

The HLSL follows the conventions in the README's portability table:

- Loose global variables (`og_modelViewPerspectiveMatrix`, `u_color`) are
  collected into the `$Globals` constant buffer. The D3D11 backend uploads it
  whenever a uniform in it has changed.
- `position : position0` is semantic name `position` with semantic index 0.
  Index 0 is the attribute location, the same slot as
  `og_positionVertexLocation`.
- `out float4 fragmentColor : SV_Target0` is render target 0, the same as
  GLSL's `out vec3 fragmentColor`. HLSL reflection doesn't report output
  names, so the D3D11 backend finds `fragmentColor` by reading the
  `name : SV_TargetN` pattern in the source (Step 8). Writing the output as
  a named `out` parameter, rather than a return value, keeps
  `FragmentOutputLocation("fragmentColor")` working on both APIs. HLSL
  outputs are `float4`, so the color gets an alpha of 1.
- `mul(M, v)` is matrix × column vector, which matches GLSL's `M * v`. It's
  correct because the D3D11 backend compiles with column-major matrix packing,
  so both backends upload the same 16 floats (Step 8).

Both entry points are named `main`, which the D3D11 backend uses as the
entry point name.

> **C++ note — raw string literals:** `R"( ... )"` is a raw string: backslashes
> and newlines are kept as typed, so the shader can be pasted in as it is. See
> [Step 2](02-shaders.md). The string literal lives for the whole program
> (it's in the executable's read-only data), so returning a
> `std::string_view` that points at it is safe. Returning a `string_view` into
> a *local* `std::string` would dangle.

> **C++ note — the trailing `return {};`:** every enumerator of `GraphicsApi`
> has a `case` that returns, but C++ doesn't assume an enum holds only its
> named values. `static_cast<GraphicsApi>(7)` is legal. Without a final
> `return`, the compiler warns that control can reach the end of a non-void
> function. The `switch` still has no `default:`, so if you add a
> `GraphicsApi` value you'll get a missing-case warning (see
> [Step 1](01-state-management.md#cpp-switch-without-default)). An empty
> source then fails in `CreateShaderProgram` with a compile error, which is
> a clear enough signal.

### 3.3 The `Triangle` class and its member order

| C# | C++ | Why |
|---|---|---|
| `private readonly GraphicsWindow _window;` | `std::unique_ptr<GraphicsWindow> m_window;` | The class owns the window exclusively (see [Step 0: `unique_ptr`](00-setup.md#cpp-unique-ptr)) |
| *(the static `Device` class)* | `std::unique_ptr<Device> m_device;` | arda's device is an object, so a GL and a D3D11 device can exist side by side |
| `private readonly SceneState _sceneState;` | `SceneState m_sceneState;` | A plain value. No heap allocation needed |
| `private readonly ClearState _clearState;` | `ClearState m_clearState;` | A plain value |
| `private readonly DrawState _drawState;` | `DrawState m_drawState;` | A value holding two `shared_ptr`s |
| `Dispose()` | *(nothing: the implicit destructor)* | Members are destroyed in reverse order |

**Order is the important part.** C++ constructs members in the order
they're *declared* in the class, and destroys them in the reverse order (see
[Step 0: member initialization order](00-setup.md#cpp-member-order)). This
program relies on that order:

1. `m_device` is constructed first. Its constructor starts GLFW, creates the
   hidden share context, and loads GL.
2. `m_window` is next. Its initializer calls `m_device->CreateGraphicsWindow`,
   so the device must already exist, which it does because it's declared
   first. If you swapped the two declarations, `m_device` would still be
   null when `m_window` is initialized, and the program would crash.
   Compilers warn about this (`-Wreorder`; MSVC C5038) when the initializer
   list is written in a different order from the declarations.
3. `m_sceneState`, `m_clearState` and `m_drawState` are default-constructed.
   The constructor body then fills them in.

At destruction, the order reverses:

1. `m_drawState` is destroyed first. Its `shared_ptr`s release the vertex
   array and the shader program. They're the last owners, so their
   destructors run and delete the GL objects (`glDeleteVertexArrays`,
   `glDeleteBuffers` for the vertex and index buffers the array kept alive,
   and `glDeleteProgram`). This happens while the window's context and the
   device still exist.
2. `m_window` is destroyed. It destroys its `Context`, then the GLFW
   window.
3. `m_device` is destroyed last. It destroys the hidden share window and
   terminates GLFW.

That's exactly C#'s `Dispose`, written out by hand in the book:

```csharp
_drawState.VertexArray.Dispose();
_drawState.ShaderProgram.Dispose();
_window.Dispose();
```

In C++, the compiler writes it for you, and it runs even when the
constructor throws partway through. If `CreateShaderProgram` throws a
compile error, for example, the members already constructed (`m_device` and
`m_window`) are destroyed in reverse order before the exception leaves the
constructor. C# needed the `using` block in `Main` for that. In C++ it's
RAII (see [Step 0: RAII](00-setup.md#cpp-raii)).

> **Why the device is a member here and not in the C#:** OpenGlobe's `Device`
> is a static class, so it's always there. arda's `CreateDevice` returns an
> object, so *something* must own it, and it must outlive every window and
> resource. Making it the first member of the class that owns everything
> else guarantees that.

> **C++ note — why `Triangle` can't be copied or moved:** the constructor
> registers `[this] { OnResize(); }` with the window. That lambda stores the
> address of *this* `Triangle`. If a `Triangle` were copied or moved, the
> window's handlers would still point at the old object, which may no
> longer exist. Deleting the copy operations also suppresses the implicit
> move operations, because a class with a user-declared copy constructor
> gets no implicit move constructor. So `Triangle` is pinned in place, and
> `main` constructs it directly as a local. See
> [Step 0: deleted copy operations](00-setup.md#cpp-deleted-copy) and
> [Step 1: lambda capture lifetime](01-state-management.md#cpp-lambda-capture).

> **C++ note — `explicit` on a one-argument constructor:** without it,
> `Triangle t = GraphicsApi::OpenGL33;` would compile as an implicit
> conversion. See [Step 0](00-setup.md#cpp-explicit).

### 3.4 The constructor, line by line

**The initializer list.**

```cpp
: m_device(CreateDevice(api)),
  m_window(m_device->CreateGraphicsWindow(800, 600, "Chapter 3:  Triangle")) {
```

- `CreateDevice` (Step 0) switches on `api` and returns a
  `std::unique_ptr<Device>` that points at a `DeviceGL3x`. It throws
  `std::runtime_error` if that backend wasn't compiled in.
- `CreateGraphicsWindow` (Step 0) is the non-virtual public function. It
  validates the size, then calls the backend's `DoCreateGraphicsWindow`. The
  GL window is created with the device's hidden window as its share context,
  so a shader program created through the device can be used in this
  window's context. Its constructor also creates the `ContextGL3x`, which
  forces GL's state to match `RenderState`'s defaults (Step 1).
- The window's context is current after construction. The shader and
  buffer creation that follows happens on it.

**The handlers.**

```cpp
m_window->SetResizeHandler([this] { OnResize(); });
m_window->SetRenderFrameHandler([this] { OnRenderFrame(); });
```

These replace C#'s `_window.Resize += OnResize;`. A C# event can have many
subscribers. `GraphicsWindow` stores a single `std::function<void()>` per
event, which is all this program needs (see
[Step 0: `std::function` and lambdas](00-setup.md#cpp-std-function-lambdas)).

Setting them before anything else matches the C#. The order doesn't really
matter, because no handler runs until `Run()`.

**The shader program.**

```cpp
std::shared_ptr<ShaderProgram> sp = m_device->CreateShaderProgram(sources.vertex, sources.fragment);
sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));
```

What happens inside (Steps 2 and 4):

1. `ShaderObjectGL3x` prepends the prelude (`#version 330`, the `og_*`
   defines and constants) to each source, compiles it and throws
   `CouldNotCreateVideoCardResourceException` with the compile log on failure.
2. `ShaderProgramGL3x` links the two shaders and throws with the link log
   on failure.
3. It asks GL for the active attributes (`position` at location 0) and
   the active uniforms (`og_modelViewPerspectiveMatrix`, a `mat4`, and
   `u_color`, a `vec3`), and creates one `UniformGL3x<T>` for each.
4. It checks every uniform name against the device's automatic-uniform
   factories. `og_modelViewPerspectiveMatrix` matches, so the program
   creates a `ModelViewPerspectiveMatrixUniform` that sets it on every draw.
   `u_color` doesn't match, so the application sets it.

`Get<Vector3<float>>("u_color")` finds the uniform and checks its type. If the
shader had declared `uniform vec4 u_color;`, this would throw instead of
silently writing the wrong type. C# needed a cast here:
`((Uniform<Vector3F>)sp.Uniforms["u_color"]).Value = ...`. `SetValue` only
stores the value and marks the uniform dirty. Nothing is sent to GL until the
first draw.

`auto` would have been fine instead of `std::shared_ptr<ShaderProgram>`.
It's spelled out here so you can see that the program is shared: the
`DrawState` will own it too.

**The mesh.**

```cpp
Mesh mesh;
auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
```

`Mesh` lives in `core` and knows nothing about GPUs (Step 3, Part B). It's
just geometry: named vertex attributes, optional indices, a primitive type
and a winding order.

- `Add<VertexAttributeFloatVector3>("position", 3)` constructs the
  attribute inside the collection and returns a reference to it. The name
  `"position"` must match the shader's input name. That's how
  `CreateVertexArray` pairs mesh data with shader inputs. The `3` is a
  capacity hint for `reserve`.
- `auto&` matters. Plain `auto` would *copy* the value vector, and the
  `emplace_back` calls would fill the copy while the mesh stayed empty.
- `emplace_back(0.0f, 0.0f, 0.0f)` constructs a `Vector3<float>` directly
  in the vector's storage, without a temporary.

The C# creates the attribute, adds it, and fills it afterwards. Filling
through a reference works the same way, because the collection owns the
attribute and the reference points into it.

```cpp
auto indices = std::make_unique<IndicesUnsignedShort>(3);
indices->AddTriangle(0, 1, 2);
mesh.indices = std::move(indices);
```

`IndicesUnsignedShort` stores 16-bit indices, which are enough for up to 65,535 vertices.
The mesh owns its indices through a `std::unique_ptr<IndicesBase>`, so
ownership moves in with `std::move`. After the move, `indices` is null.
C#'s `new TriangleIndicesUnsignedShort(0, 1, 2)` wrapper struct becomes three
arguments.

`mesh.primitiveType` and `mesh.frontFaceWindingOrder` already default to
`Triangles` and `Counterclockwise` (Step 3), so the C# didn't need to set them
and neither does this code.

**The vertex array.**

```cpp
std::shared_ptr<VertexArray> va =
    m_window->GetContext().CreateVertexArray(mesh, sp->VertexAttributes(), BufferHint::StaticDraw);
```

This is `Context.cs` lines 33–50 (Step 3, Part B):

1. `Device::CreateMeshBuffers` walks the shader's vertex attributes. For
   `position` at location 0, it finds the mesh attribute with the same name,
   creates a `VertexBuffer`, copies the three `Vector3<float>`s into it and
   describes it with a `VertexBufferAttribute` (3 floats, not normalized).
   It also creates an `IndexBuffer` of three `uint16_t`s.
2. `Context::CreateVertexArray` creates an empty `VertexArrayGL3x` and
   assigns the attribute to slot 0 and the index buffer. The VAO is configured
   later, on the first draw, because the vertex array only records the changes
   now and applies them in `Clean`.

The vertex array is created by the *context*, not the device, because GL
vertex array objects are not shared between contexts (3.5.3). The buffers
inside it *are* shared, which is why the device creates them.

`BufferHint::StaticDraw` tells the driver the data is written once and drawn
many times, so it can put it in GPU memory.

The mesh is a local. Once the data is on the GPU, the CPU copy isn't needed,
and `mesh` is destroyed at the end of the constructor. The GPU buffers live
on, owned by the vertex array.

**The render state and draw state.**

```cpp
RenderState renderState;
renderState.facetCulling.enabled = false;
renderState.depthTest.enabled = false;

m_drawState = DrawState{renderState, sp, va};
```

`RenderState` is a plain struct with OpenGlobe's defaults (Step 1). The
book turns off two of them here:

- **Facet culling:** on by default, discarding back faces. The triangle's
  winding as seen from the camera is what decides which side is the front.
  With culling off, both sides draw, so it doesn't matter.
- **Depth testing:** on by default. There's only one triangle, so nothing
  can hide it.

`DrawState{renderState, sp, va}` is aggregate initialization in field order:
`renderState`, `shaderProgram`, `vertexArray` (see
[Step 1: aggregates](01-state-management.md#cpp-aggregates)). It replaces
C#'s `new DrawState(renderState, sp, va)`. The `shared_ptr`s are copied, so the
program and vertex array now have two owners each: the locals `sp` and `va`,
and `m_drawState`. When the constructor returns, the locals are destroyed,
and `m_drawState` is the only owner.

Designated initializers would make the fields explicit:

```cpp
m_drawState = DrawState{
    .renderState = renderState,
    .shaderProgram = sp,
    .vertexArray = va,
};
```

Both forms are fine. The designated form survives a reordering of
`DrawState`'s fields, and it's a compile error if the order is wrong.

**The camera.**

```cpp
m_sceneState.camera.ZoomToTarget(1.0);
```

The default `Camera` (Step 4) sits at (0, −1, 0), looks at the origin and
has +z up. So world x is screen right, world z is screen up, and the xz-plane
triangle faces the camera. `ZoomToTarget(1.0)` moves the eye back along the
view direction until a sphere of radius 1 around the target fits in the field
of view. The triangle's vertices are all within 1 of the origin, so it fits.

### 3.5 `OnResize`

```cpp
void Triangle::OnResize() {
    m_window->GetContext().SetViewport({0, 0, m_window->Width(), m_window->Height()});
    m_sceneState.camera.aspectRatio = m_window->Width() / static_cast<double>(m_window->Height());
}
```

`GraphicsWindow::Run` calls this once before the first frame, then again from
`PollEvents` whenever the framebuffer size changes (Step 0).

- **The viewport** maps clip space to window pixels. `Rectangle{0, 0, w, h}`
  is left, bottom, width, height, with the origin at the bottom-left (the GL
  convention; D3D11 converts it). `Context::SetViewport` skips the GL call
  if the viewport hasn't changed (Step 1).
- **The aspect ratio** is what the perspective projection uses to keep a
  square looking square. It must be the viewport's width over height.
  `Width()`/`Height()` are the *framebuffer* size in pixels, so on a
  high-DPI display they can differ from the window size in screen
  coordinates. Using the framebuffer size for both keeps them consistent.
- `static_cast<double>` is needed because `800 / 600` in integer arithmetic
  is `1`. Casting one operand makes the division floating-point.

You don't need a zero-size guard. Step 0's `GraphicsWindowGL3x` ignores the
0×0 size Windows reports for a minimized window, so the handler never sees a
zero height. (Step 4's milestone had a guard; with Step 0's filtering, it
never triggers.)

### 3.6 `OnRenderFrame` and what happens inside `Draw`

```cpp
void Triangle::OnRenderFrame() {
    Context& context = m_window->GetContext();
    context.Clear(m_clearState);
    context.Draw(PrimitiveType::Triangles, m_drawState, m_sceneState);
}
```

This is Listing 3.32, and it's the reason Chapter 3 exists: two lines draw
a frame, and nothing in them mentions a graphics API.

Here's what those two calls do on the first frame, in the GL backend:

**`context.Clear(m_clearState)`** (Steps 1 and 6):

1. `ApplyFramebuffer()` binds the default framebuffer (the window), because
   no `Framebuffer` has been set on the context.
2. The scissor test is disabled and the color and depth masks are set to all
   on, so the clear affects the whole window, as it does on D3D11. The cached
   state already matches, so no GL calls are made.
3. `glClearColor(1, 1, 1, 1)` runs on the first frame only. After that, the
   cached clear color matches and the call is skipped.
4. `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)`.

**`context.Draw(PrimitiveType::Triangles, m_drawState, m_sceneState)`**
(Steps 3–6):

1. `Context::Draw` is non-virtual. It calls `VerifyDraw`, which checks that
   the draw state has a shader program and a vertex array, and that a depth
   test isn't enabled against a framebuffer with no depth attachment.
2. It calls the backend's `DoDraw`, which calls `ApplyBeforeDraw`:
   - `ApplyRenderState`: compares each part of `renderState` with the
     cached GL state. On the first frame, `GL_CULL_FACE` and `GL_DEPTH_TEST`
     are disabled. On every frame after that, nothing is called.
   - `ApplyVertexArray`: binds the VAO. On the first frame, `Clean` runs
     `glBindBuffer`, `glVertexAttribPointer` and `glEnableVertexAttribArray` for slot 0,
     and binds the index buffer to the VAO.
   - `ApplyShaderProgram`: `glUseProgram` if a different program was last
     used, then `Clean`:
     - The draw automatic uniforms run first.
       `ModelViewPerspectiveMatrixUniform` reads
       `m_sceneState.ModelViewPerspectiveMatrix(clipDepth)` (with the device's
       clip-depth range), converts it
       from double to float and calls `SetValue`. The uniform becomes dirty
       only if the matrix changed.
     - Dirty uniforms upload: `glUniformMatrix4fv` for the matrix and
       `glUniform3f` for `u_color` on the first frame. On later frames,
       neither has changed, so nothing is uploaded.
   - `CleanTextureUnits`: there are no textures, so nothing happens.
   - `ApplyFramebuffer`: still the window, so nothing happens.
3. `glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, nullptr)`.

After the first frame, a steady-state frame makes about three GL calls:
`glClear`, the VAO bind and `glDrawRangeElements`. Every other call was
skipped because the renderer's cached state already matched. That's the
payoff of Section 3.3.3's state shadowing and Section 3.4.4's lazy uniforms.

Then `GraphicsWindow::Run` calls `SwapBuffers`, which presents the back
buffer and waits for vsync.

> **Why `Draw` takes the primitive type separately from the `DrawState`:**
> the same vertex array can be drawn in different ways. Try This 1 below
> draws it as a line loop, without changing the draw state. Step 3's
> Milestone 3B passes `mesh.primitiveType` instead, which lets the mesh
> decide. Both are valid.

### 3.7 `main` and `Run`

```cpp
int main() {
    try {
        Triangle example(GraphicsApi::OpenGL33);
        example.Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

- `Triangle example(...)` is C#'s `using (Triangle example = new Triangle())`.
  The object is a local, and its destructor runs when the `try` block ends,
  either normally or through an exception.
- If the constructor throws (for example, because the shader doesn't
  compile), the `catch` prints the error and `main` returns 1. By then,
  everything the constructor had already created has been destroyed.
- `Run()` loops until the window is closed: poll events, update, render,
  swap. C#'s `Run(30.0)` passed an update rate for the `UpdateFrame` event.
  arda's `Run` has no fixed-rate update yet, and the example doesn't use
  `UpdateFrame`. Frames are paced by vsync.

---

## 4. C# to C++ at a glance

| C# (`Triangle.cs`) | C++ (`main.cpp`) | Step |
|---|---|---|
| `Device.CreateWindow(800, 600, ...)` | `m_device->CreateGraphicsWindow(800, 600, ...)` | 0 |
| `_window.Resize += OnResize` | `m_window->SetResizeHandler([this] { OnResize(); })` | 0 |
| `_window.RenderFrame += OnRenderFrame` | `m_window->SetRenderFrameHandler([this] { OnRenderFrame(); })` | 0 |
| `new ClearState()` | `ClearState m_clearState;` | 1 |
| `new RenderState()`, `.FacetCulling.Enabled = false` | `RenderState renderState;`, `.facetCulling.enabled = false` | 1 |
| `Device.CreateShaderProgram(vs, fs)` | `m_device->CreateShaderProgram(vs, fs)` | 2 |
| `((Uniform<Vector3F>)sp.Uniforms["u_color"]).Value = ...` | `sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(...)` | 2 |
| `new VertexAttributeFloatVector3("position", 3)` + `mesh.Attributes.Add(...)` | `mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3)` | 3 |
| `new IndicesUnsignedShort(3)`, `mesh.Indices = indices` | `std::make_unique<IndicesUnsignedShort>(3)`, `mesh.indices = std::move(indices)` | 3 |
| `indices.AddTriangle(new TriangleIndicesUnsignedShort(0, 1, 2))` | `indices->AddTriangle(0, 1, 2)` | 3 |
| `_window.Context.CreateVertexArray(mesh, sp.VertexAttributes, BufferHint.StaticDraw)` | `m_window->GetContext().CreateVertexArray(mesh, sp->VertexAttributes(), BufferHint::StaticDraw)` | 3 |
| `new DrawState(renderState, sp, va)` | `DrawState{renderState, sp, va}` | 3 |
| `new SceneState()`, `.Camera.ZoomToTarget(1)` | `SceneState m_sceneState;`, `.camera.ZoomToTarget(1.0)` | 4 |
| `new Rectangle(0, 0, w, h)` | `Rectangle{0, 0, w, h}` (origin at the bottom-left) | 1 |
| `_sceneState.Camera.AspectRatio = ...` | `m_sceneState.camera.aspectRatio = ...` | 4 |
| `context.Draw(PrimitiveType.Triangles, _drawState, _sceneState)` | `context.Draw(PrimitiveType::Triangles, m_drawState, m_sceneState)` | 3 |
| `Dispose()` | The implicit destructor, in reverse declaration order | 0 |
| `using (...) { example.Run(30.0); }` | `Triangle example(...); example.Run();` | 0 |

---

## 5. Loading shaders from files (optional)

The book embeds shader files in the assembly as resources (3.4.1). Shaders
in their own files are easier to edit: your editor can highlight GLSL, and
the line numbers in compile errors match the file. Here's a small, complete
version.

### 5.1 The files

Create `scene/shaders/` with four files. They contain the same code as
`TriangleShaders`:

`scene/shaders/triangle.vs.glsl`:

```glsl
#version 330

layout(location = og_positionVertexLocation) in vec4 position;
uniform mat4 og_modelViewPerspectiveMatrix;

void main()
{
    gl_Position = og_modelViewPerspectiveMatrix * position;
}
```

`scene/shaders/triangle.fs.glsl`:

```glsl
#version 330

out vec3 fragmentColor;
uniform vec3 u_color;

void main()
{
    fragmentColor = u_color;
}
```

`scene/shaders/triangle.vs.hlsl`:

```hlsl
float4x4 og_modelViewPerspectiveMatrix;

float4 main(float4 position : position0) : SV_Position
{
    return mul(og_modelViewPerspectiveMatrix, position);
}
```

`scene/shaders/triangle.fs.hlsl`:

```hlsl
float3 u_color;

void main(out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(u_color, 1.0);
}
```

### 5.2 Copy them next to the executable

In `scene/CMakeLists.txt`, after `add_executable(arda_scene ...)`:

```cmake
# Copy the shaders next to the executable after every build, so the program
# finds them no matter which directory it's started from.
add_custom_command(TARGET arda_scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:arda_scene>/shaders
    COMMENT "Copying shaders")
```

`$<TARGET_FILE_DIR:arda_scene>` is a *generator expression*. CMake replaces it
at build time with the directory that holds the executable, including the
`Debug`/`Release` subdirectory on multi-config generators such as Visual Studio.

A `POST_BUILD` command only runs when the target is rebuilt. If you change
only a shader, the executable is up to date and nothing is copied. To copy
shaders on every build, use a custom target instead:

```cmake
add_custom_target(arda_scene_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:arda_scene>/shaders)
add_dependencies(arda_scene arda_scene_shaders)
```

### 5.3 Load them

Add this to the anonymous namespace in `main.cpp`, replacing `ShaderSources` and
`TriangleShaders`:

```cpp
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

enum class ShaderStage { Vertex, Fragment };

// Reads shaders/<name>.<vs|fs>.<glsl|hlsl> from the executable's directory.
// The extension depends on the device's API.
std::string LoadShader(GraphicsApi api, std::string_view name, ShaderStage stage) {
    std::string fileName(name);
    fileName += stage == ShaderStage::Vertex ? ".vs" : ".fs";
    fileName += api == GraphicsApi::OpenGL33 ? ".glsl" : ".hlsl";

    const std::filesystem::path path = std::filesystem::path("shaders") / fileName;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open shader file: " + path.string());
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}
```

In the constructor:

```cpp
const std::string vs = LoadShader(api, "triangle", ShaderStage::Vertex);
const std::string fs = LoadShader(api, "triangle", ShaderStage::Fragment);
std::shared_ptr<ShaderProgram> sp = m_device->CreateShaderProgram(vs, fs);
```

`vs` and `fs` are `std::string`s. They convert implicitly to the
`std::string_view` parameters, and they live until the end of the constructor,
which is longer than `CreateShaderProgram` needs them.

> **Why `std::ios::binary`:** in text mode on Windows, `\r\n` becomes `\n`.
> That's harmless for shaders, but binary mode reads the file exactly as
> stored on every platform, which is what you want for source you pass to
> another compiler.

> **Why the path is relative:** `"shaders/triangle.vs.glsl"` is resolved
> against the *current working directory*, not the executable's directory.
> `run.sh` starts the program from the repository root, so the lookup would
> fail there. To be robust, build the path from the executable's location.
> There's no portable standard way to find it, but `GetModuleFileNameW` on
> Windows and `/proc/self/exe` on Linux both work. Or pass the shader
> directory as a command-line argument. Chapter 3 doesn't need either: the
> string literals in section 2 work everywhere.

To embed the files in the executable instead, like OpenGlobe does, a build
step can turn each file into a header containing a raw string literal (for
example with CMake's `file(READ ...)` and `configure_file`). C++26's `#embed`
will make that a one-liner once compilers support it.

---

## 6. "Try This" exercises from 3.8

Each exercise changes only `main.cpp`.

### Try This 1: a line loop instead of triangles

In `OnRenderFrame`, change the primitive type:

```cpp
context.Draw(PrimitiveType::LineLoop, m_drawState, m_sceneState);
```

You now see the triangle's outline: `GL_LINE_LOOP` connects vertices 0→1→2
and back to 0. The vertex array and indices are unchanged. Only the
primitive assembly stage interprets them differently.

Then try `PrimitiveType::Points`. Three single pixels appear at the corners.
They're hard to see at 1 pixel. To make them bigger, enable
`renderState.programPointSize = ProgramPointSize::Enabled` in the constructor, and
write `gl_PointSize = 10.0;` in the vertex shader.

On D3D11, `LineLoop` throws, because Direct3D 11 has no line-loop topology
(see the README's portability table). The portable way to draw the outline is
a `LineStrip` with the first index repeated:

```cpp
// In the constructor, replace AddTriangle with four indices:
auto indices = std::make_unique<IndicesUnsignedShort>(4);
indices->Values().push_back(0);
indices->Values().push_back(1);
indices->Values().push_back(2);
indices->Values().push_back(0);
mesh.indices = std::move(indices);

// In OnRenderFrame:
context.Draw(PrimitiveType::LineStrip, m_drawState, m_sceneState);
```

### Try This 2: wireframe through render state

The line loop changes the primitive. A different approach keeps the
triangles and changes how they're filled:

```cpp
renderState.rasterizationMode = RasterizationMode::Line;
```

This is `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` (Step 1). For one triangle,
the result looks the same as the line loop. For a mesh, it draws every
triangle's edges, which is a useful debugging view. D3D11 supports `Line`
(`D3D11_FILL_WIREFRAME`) but not `Point`.

### Try This 3: draw an explicit range

`Context::Draw` has a second overload that draws `count` indices starting
at `offset` (Step 3):

```cpp
context.Draw(PrimitiveType::Triangles, 0, 3, m_drawState, m_sceneState);
```

This draws the same triangle. With `(PrimitiveType::Triangles, 0, 2, ...)`,
nothing appears, because two indices don't make a triangle. With
`(PrimitiveType::Lines, 0, 2, ...)`, the first edge appears. Ranges let
one vertex array hold several objects, drawn with separate calls.

### Try This 4: change the color every frame

This one tests the dirty-uniform path. Add a frame counter as a member, then
set the color in `OnRenderFrame`:

```cpp
// In the class:
int m_frame = 0;

// In OnRenderFrame, before Draw:
const float t = static_cast<float>(m_frame++ % 120) / 119.0f;
m_drawState.shaderProgram->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, t, 0.0f));
```

The triangle cycles from red to yellow. Put a breakpoint in
`UniformGL3x<Vector3<float>>::Clean`: it's hit once per frame now. Before the
change, it was hit once in total.

Looking the uniform up by name every frame costs a hash lookup. To avoid it,
store a pointer to the uniform: `Uniform<Vector3<float>>* m_color` set in the
constructor. The uniform lives as long as the program, which the draw state
keeps alive.

### Try This 5: orbit the camera

Use the update handler (Step 0), which runs before the render handler every
frame:

```cpp
// In the constructor:
m_window->SetUpdateFrameHandler([this] {
    m_sceneState.camera.eye = m_sceneState.camera.eye.RotateAboutAxis(0.01, Vector3<double>::UnitZ());
});
```

The eye rotates around the z axis, so the triangle turns edge-on and then
shows its back. Now turn facet culling back on
(`renderState.facetCulling.enabled = true`) and watch the back side
disappear. The front face is the side where the vertices go 0→1→2
counterclockwise, which is `Mesh::frontFaceWindingOrder`'s default.
`RotateAboutAxis` and `UnitZ` are core's `Vector3` functions. See Step 4's
milestone.

### Try This 6: a textured triangle

This is the milestone for [Step 5](05-textures.md): add texture coordinates to
the mesh, load an image into a `Texture2D`, bind it and a sampler to texture
unit 0, and sample it in the fragment shader. Step 5 has the complete program.

### Try This 7: render to a texture

This is the milestone for [Step 6](06-framebuffers.md): draw the triangle into
a `Framebuffer` with a color texture attached, then draw that texture on a
full-window quad. Step 6 has the complete program.

---

## 7. Troubleshooting

| Symptom | Likely cause |
|---|---|
| White window, no triangle | The camera isn't looking at the triangle: check `ZoomToTarget` and that the triangle is in the xz plane (y = 0). Facet culling was left on and you're seeing the back face. Or the uniform matrix never uploaded: check that `ShaderProgramGL3x::Clean` runs the draw automatic uniforms before cleaning dirty uniforms. |
| Black triangle | `u_color` was never set (GL's initial value is 0), or `SetValue` was called on a different program. |
| `No uniform named 'u_color'` | The fragment shader doesn't use `u_color`, so the linker removed it. |
| `Shader requires vertex attribute "position", which is not present in mesh.` | The mesh attribute name and the shader input name differ. |
| Triangle stretched when the window is resized | `OnResize` isn't updating `camera.aspectRatio`, or it uses integer division. |
| Triangle fills only part of the window after resizing | The viewport isn't updated in `OnResize`. |
| Crash at exit | Something that holds GPU resources outlives the window or device. For example, a `std::shared_ptr<ShaderProgram>` stored in a global. Keep all resources in `Triangle`, declared after the device and window. |
| `Shader program validation failed` in a debug build | No vertex array is bound, or a sampler uniform's texture unit has no texture (Step 5). Read the log in the message. |
| `CreateDevice: this graphics API was not compiled in` | `ARDA_RENDERER_GL` is off, or you passed `Direct3D11` before building Step 8. |

---

## 8. Running on Direct3D 11

Once Step 8 is done, change one line in `main`:

```cpp
Triangle example(GraphicsApi::Direct3D11);
```

Nothing else changes. `TriangleShaders` returns the HLSL, and every renderer
call goes through `DeviceD3D11`, `GraphicsWindowD3D11` and `ContextD3D11`.
The picture should be identical, pixel for pixel. Step 8's cross-API tests
check exactly that.

To choose the API at run time, read a command-line argument:

```cpp
#include <cstring>

int main(int argc, char* argv[]) {
    const GraphicsApi api = (argc > 1 && std::strcmp(argv[1], "d3d11") == 0)
                                ? GraphicsApi::Direct3D11
                                : GraphicsApi::OpenGL33;
    if (!IsGraphicsApiAvailable(api)) {
        std::fprintf(stderr, "That graphics API was not compiled in.\n");
        return 1;
    }
    try {
        Triangle example(api);
        example.Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

Then `./run.sh -- d3d11` runs the D3D11 version (everything after `--` is
passed to the program).

---

## Checklist

- [ ] `main.cpp` includes only `arda/core/...` and `arda/renderer/...`
      headers, and makes no GL calls
- [ ] `Triangle` declares `m_device` first and `m_drawState` last, and
      deletes its copy operations
- [ ] The red triangle appears on a white background
- [ ] It keeps its shape and stays centered when the window is resized, and
      minimizing doesn't throw
- [ ] Closing the window exits with code 0, with no errors printed
- [ ] A deliberately broken shader (for example, a missing `;`) prints the
      compile log and exits with code 1
- [ ] At least Try This 1, 4 and 5 done
- [ ] (After Step 8) `GraphicsApi::Direct3D11` shows the same picture
