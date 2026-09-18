# Step 1: State management (3.3)

**Goal:** describe render state as plain data that is passed to each call,
instead of setting global state, and implement `Context::Clear` and the
viewport. The GL context keeps a cached copy of the GL state and skips GL
calls for state that hasn't changed.

**Read:** 3.3.1–3.3.5 (Listings 3.3–3.6). 3.3.6 (sorting by state) is
optional. This guide explains why arda doesn't do it.

**Milestone:** `Context::Clear` clears the window to a color you choose every
frame, the viewport follows the window size, and the new tests pass.

**What you'll learn in this step**

- **OpenGL:** why GL is a state machine; what each fixed-function stage does
  (primitive restart, face culling, polygon mode, program point size,
  scissor, stencil, depth test, depth range, blending, write masks); how
  `glClear` and `glViewport` work; and why a CPU-side copy of the GL state is
  worth keeping.
- **C++:** aggregates and default member initializers, designated
  initializers, defaulted `operator==`, bit-flag operators for `enum class`,
  `switch` without `default` and the warnings that catch a missing case,
  static member functions, pass by `const&` or by value, `inline` functions
  in headers, lambda capture lifetimes, and two naming traps (`near`/`far`
  macros, and a field with the same name as its type).

## OpenGlobe reference

| File | What to take from it |
|---|---|
| [RenderState/*.cs](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Renderer/RenderState) | Every state class, its enums and its defaults (set in each constructor) |
| [ClearState.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/ClearState.cs) | `ClearState` |
| [Context.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Context.cs) | `ClearBuffers` enum (lines 20–27), `Clear`, `Viewport` |
| [DrawState.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/DrawState.cs) | `DrawState` (only `RenderState` is used in this step) |
| [GL3x/ContextGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ContextGL3x.cs) | Constructor (lines 20–42), `ForceApplyRenderState` (44–98), `Viewport` (124–144), `Clear` (152–180), the `Apply*` methods (226–472), `Enable` (474–484), `ApplyRenderState` (534–547) |
| [GL3x/TypeConverterGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/TypeConverterGL3x.cs) | `To(ClearBuffers)`, `To(DepthTestFunction)`, `To(CullFace)`, `To(WindingOrder)`, `To(RasterizationMode)`, `To(StencilOperation)`, `To(StencilTestFunction)`, `To(BlendEquation)`, `To(SourceBlendingFactor)`, `To(DestinationBlendingFactor)` |
| [Core/Geometry/Mesh.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Geometry/Mesh.cs) | `WindingOrder` (lines 29–33), needed by `FacetCulling` |

## Files

```
core/
  CMakeLists.txt                 UPDATE  list WindingOrder.h
  include/arda/core/geometry/
    WindingOrder.h               NEW
renderer/
  CMakeLists.txt                 UPDATE  new headers, TypeConverterGL3x.cpp, warnings
  include/arda/renderer/
    Rectangle.h                  NEW
    Color.h                      NEW
    ClearState.h                 NEW   ClearBuffers, ClearState
    DrawState.h                  NEW   (renderState only for now)
    Context.h                    UPDATE  Clear, GetViewport, SetViewport
    renderstate/
      Blending.h                 NEW   SourceBlendingFactor, DestinationBlendingFactor, BlendEquation, Blending
      ColorMask.h                NEW
      DepthRange.h               NEW
      DepthTest.h                NEW   DepthTestFunction, DepthTest
      FacetCulling.h             NEW   CullFace, FacetCulling
      PrimitiveRestart.h         NEW
      ScissorTest.h              NEW
      StencilTest.h              NEW   StencilOperation, StencilTestFunction, StencilTestFace, StencilTest
      RenderState.h              NEW   ProgramPointSize, RasterizationMode, RenderState
  src/
    Context.cpp                  UPDATE  SetViewport
    gl/
      TypeConverterGL3x.h / .cpp NEW
      ContextGL3x.h / .cpp       UPDATE  cached state, ForceApplyRenderState, Apply*, DoClear, DoSetViewport
scene/src/main.cpp               UPDATE  clear every frame, set the viewport on resize
tests/
  CMakeLists.txt                 UPDATE  new test files, link glad
  src/renderer/RenderStateTests.cpp   NEW   plain-data tests (no GPU)
  src/renderer/ContextGL3xTests.cpp   NEW   hidden-window GL tests
```

## How this guide is organized

The sections are numbered in the order you build them. Each checkpoint is a
point where everything so far compiles, so you can stop and test.

1. [Background: OpenGL is a state machine](#1-background-opengl-is-a-state-machine)
   (reading only).
2. [`WindingOrder` in core](#2-windingorder-in-core).
3. [`Rectangle` and `Color`](#3-rectangle-and-color).
4. [The render state headers](#4-the-render-state-headers), one GL stage at
   a time, ending with `RenderState` itself.
5. [`ClearState`](#5-clearstate).
6. [`DrawState`](#6-drawstate).
7. [**Checkpoint A**](#7-checkpoint-a-the-plain-data-tests): the plain-data
   tests build and pass.
8. [The GL type converter](#8-the-gl-type-converter), then
   **Checkpoint B**: the library builds.
9. [`Context`: `Clear` and the viewport](#9-context-clear-and-the-viewport).
10. [`ContextGL3x`](#10-contextgl3x), then **Checkpoint C**: the library
    builds again. Sections 9 and 10 must be done together.
11. [`scene/src/main.cpp`](#11-scenesrcmaincpp), then **Checkpoint D**: the
    window clears.
12. [The tests](#12-tests), the [CMake files](#cmake) in full, and
    **Checkpoint E**: the milestone.

---

## 1. Background: OpenGL is a state machine

Before writing any code, it helps to know what problem Section 3.3 solves.

> **OpenGL note — the state machine:** an OpenGL context is a large set of
> variables: "is depth testing on?", "which comparison does it use?", "which
> blend factors?", "what is the clear color?", and hundreds more. Each has a
> default value when the context is created. Functions like `glEnable`,
> `glDepthFunc` and `glClearColor` don't draw anything. They change one of
> those variables, and the new value stays until something changes it again.
> A draw call (`glDrawArrays`, `glDrawElements`) or a clear (`glClear`) then
> runs using whatever the variables hold at that moment.
>
> This state is **per context**. In arda every window has its own context
> (Step 0). Objects such as buffers and textures are shared between contexts
> in the same share group, but *state* is not. Enabling the depth test in
> one window's context does nothing to another window's.

### The problem with global state (3.3.1)

Because state persists, a piece of code that changes state affects every
draw that comes after it, including draws in completely unrelated code.
Suppose the code that draws billboards turns on blending and turns off
culling, and forgets to turn them back. The terrain drawn next is now
blended and not culled. The bug shows up in the terrain code, a long way from
the line that caused it, and it depends on draw order, so reordering two
unrelated draws can make it come and go.

The common fixes are all unsatisfying:

- **Restore state after every draw.** Every draw then pays for twice the
  state changes, and one forgotten restore brings the bug back.
- **Set all state before every draw.** Correct, but most of those GL calls
  set a value that is already set, and each GL call has a CPU cost.
- **Push and pop state** with `glPushAttrib`/`glPopAttrib`. These were
  removed from the core profile in GL 3.1, and Direct3D never had them.

### Render state as data (3.3.2)

The book's answer is to stop exposing GL's global state. Instead, every draw
call receives a complete description of the fixed-function state it wants, a
`RenderState`, as part of its `DrawState`:

```cpp
// Step 3 adds Draw. This is how it will look:
DrawState drawState;
drawState.renderState.facetCulling.enabled = false;   // only this draw is unculled
context.Draw(PrimitiveType::Triangles, drawState, sceneState);
```

A draw can't "leak" state into the next one, because the next draw brings
its own complete `RenderState`. Anything it doesn't mention has a default
value, not "whatever the last draw left behind".

> **Why:** *render state is passed per draw instead of set globally (3.3).*
> Each draw is described completely by its arguments, so draw order can't
> change what a draw looks like. Code that draws something can be read and
> tested on its own. It also maps directly onto Direct3D 11, where state is
> grouped into objects that are bound per draw (see the
> [D3D11 check](#d3d11-check)).

### Syncing GL with the render state (3.3.3)

If every draw carries a complete `RenderState`, the context has to make GL
match it before drawing. Doing that with one GL call per field would be
correct but wasteful: most consecutive draws share most of their state.

So the context keeps its own copy of the GL state, `m_renderState`. Before
each draw it compares the requested state with that copy, field by field,
and only calls GL for fields that differ. After each GL call it updates the
copy. This is called **state shadowing** or a **state cache**.

The cache only works if one rule is never broken:

> **The cached state must always equal the real GL state.**

If they ever disagree, the context will skip a GL call that was actually
needed, and the draw will be wrong with no error anywhere. Three things
follow from the rule, and you'll see each one in the code:

1. **Sync once at startup.** A new GL context has GL's defaults, and arda's
   defaults are different (for example, the depth test is on). The
   constructor calls `ForceApplyRenderState`, which sets every GL value
   without checking the cache, so both start out equal.
2. **Update the cache only together with GL.** Every `Apply*` method writes
   the cache in the same branch that calls GL.
3. **Nothing else may change this state.** All GL state changes go through
   the context. If you later add code that calls `glEnable` directly (a
   debug overlay library, for example), you must force the state again
   afterwards.

> **OpenGL note — why cache on the CPU instead of asking GL:** you could
> query the current value with `glGetIntegerv` or `glIsEnabled` before each
> change. Don't. GL commands are queued and executed later by the driver,
> often on another thread. A query may make the CPU wait until the queue
> catches up, which is far slower than the redundant call it avoids. A plain
> C++ comparison costs almost nothing.

### Sorting by state (3.3.6), and why arda doesn't

The book also describes sorting draws so that draws with the same state run
together, which reduces state changes further. It's optional, and arda leaves
it out for now:

- the cache already removes redundant calls, so sorting only saves the
  changes that are really needed;
- sorting needs every draw of a frame collected before any runs, which is a
  different API shape (a render queue);
- it conflicts with orders that matter, such as drawing transparent objects
  back to front.

It can be added later on top of `DrawState`, because `RenderState` has
`operator==` and is easy to compare.

### Where each piece of state acts in the pipeline

Here is where the state in this step acts, in pipeline order. Keep it in
mind as you write each header in [part 4](#4-the-render-state-headers).

```
vertex shader ── writes gl_Position (and gl_PointSize if ProgramPointSize is enabled)
      │
primitive assembly ── PrimitiveRestart: a special index starts a new strip
      │
clipping, perspective divide ─► normalized device coordinates (NDC), x, y, z in [-1, 1]
      │
viewport transform ── Viewport (x, y) and DepthRange (z)  ─► window coordinates
      │
face culling ── FacetCulling: discard triangles facing away
      │
rasterization ── RasterizationMode: fill, outline or corner points
      │
fragment shader
      │
per-fragment tests ── ScissorTest ─► StencilTest ─► DepthTest
      │
blending ── Blending
      │
write masks ── ColorMask, depthMask ─► framebuffer
```

`glClear` bypasses almost all of this. It writes directly to the buffers, but
the scissor test and the write masks still apply to it. That matters in
[part 10.5](#105-doclear).

---

## 2. `WindingOrder` in core

`FacetCulling` needs to say which winding order counts as the front of a
triangle. OpenGlobe defines `WindingOrder` next to `Mesh` in `Core`, because
meshes need it too (Step 3 uses it for `Mesh::frontFaceWindingOrder`). arda
does the same, in `arda::core::geometry`.

### `core/include/arda/core/geometry/WindingOrder.h`

```cpp
#pragma once

namespace arda::core::geometry {

// The order in which a triangle's vertices appear on screen, used to decide
// which side of the triangle is the front (Mesh.cs).
enum class WindingOrder {
    Clockwise,
    Counterclockwise,
};

} // namespace arda::core::geometry
```

This uses `#pragma once`, a nested namespace and `enum class`, all covered in
[Step 0](00-setup.md). The trailing comma after the last enumerator is
allowed, and it keeps diffs to one line when you add a value.

### `core/CMakeLists.txt`

Core's `CMakeLists.txt` lists its headers so that they appear in the IDE.
Add the new one:

```cmake
# Headers are listed for IDE project-tree visibility only; CMake compiles
# just the .cpp sources. Keep this list current when adding headers.
add_library(arda_core
    src/Geodetic2D.cpp
    src/Geodetic3D.cpp
    src/Trig.cpp
    src/geometry/Ellipsoid.cpp
    include/arda/core/Geodetic2D.h
    include/arda/core/Geodetic3D.h
    include/arda/core/Math.h
    include/arda/core/Trig.h
    include/arda/core/geometry/Ellipsoid.h
    include/arda/core/geometry/Vector2.h
    include/arda/core/geometry/Vector3.h
    include/arda/core/geometry/Vector4.h
    include/arda/core/geometry/WindingOrder.h
)
target_include_directories(arda_core PUBLIC include)
```

---

## 3. `Rectangle` and `Color`

OpenGlobe uses `System.Drawing.Rectangle` for the viewport and scissor box,
and `System.Drawing.Color` for clear and blend colors. C++ has no standard
equivalents, so arda defines two small structs.

### `renderer/include/arda/renderer/Rectangle.h`

```cpp
#pragma once

namespace arda::renderer {

// A rectangle in window coordinates, in pixels, with the origin at the
// bottom-left corner (the GL convention). Replaces System.Drawing.Rectangle.
struct Rectangle {
    int left = 0;
    int bottom = 0;
    int width = 0;
    int height = 0;

    int Right() const { return left + width; }
    int Top() const { return bottom + height; }

    bool operator==(const Rectangle&) const = default;
};

} // namespace arda::renderer
```

`System.Drawing.Rectangle` has `X`, `Y`, `Left`, `Top`, `Bottom` and so on,
and OpenGlobe passes `rectangle.Bottom` to `GL.Scissor` as if `Y` pointed up.
Naming the fields `left` and `bottom` makes the convention explicit.
`Right()` and `Top()` are `const` member functions: the `const` after the
parameter list promises they don't modify the object, so they can be called
on a `const Rectangle&`.

> **OpenGL note — window coordinates start at the bottom-left:** GL puts
> pixel (0, 0) at the *bottom-left* of the window, with y pointing up. Most
> windowing systems (and Direct3D) put it at the top-left with y pointing
> down. The README settles this once: `Rectangle` is always bottom-left, and
> the D3D11 backend converts it in Step 8.

This struct introduces three C++ features used by every file in this step.

<a id="cpp-aggregates"></a>

> **C++ note — aggregates and default member initializers:** `Rectangle` is
> an *aggregate*: a class with no user-declared constructors, no private or
> protected data members, no virtual functions and no virtual, private or
> protected base classes. Aggregates can be initialized with braces, member by
> member in declaration order:
>
> ```cpp
> Rectangle r{0, 0, 1280, 720};   // left, bottom, width, height
> Rectangle empty;                // every member uses its default: {0, 0, 0, 0}
> Rectangle partial{10, 20};      // left = 10, bottom = 20, the rest use defaults
> ```
>
> The `= 0` after each field is a *default member initializer*. It is used
> whenever a constructor or initializer doesn't give that member a value.
> Since C++14 an aggregate can have them, which is what makes these structs
> both convenient and safe. Without the `= 0`, `Rectangle r;` would leave the
> fields *uninitialized*: they would hold whatever bytes were in memory, and
> reading them would be undefined behaviour. Every field of every state
> struct in this step has an initializer for that reason.
>
> Compared with C#: OpenGlobe's state types are mostly `class`es with a
> constructor that sets each property. In C++ the defaults move onto the
> fields and the constructor disappears. One big difference is that these
> structs are **values**. In C#, `drawState.RenderState = renderState` makes
> both variables refer to the same object. In C++ it copies every field, and
> changing one copy doesn't change the other. That's what the renderer wants:
> the context's cached copy can never be modified from outside.

<a id="cpp-defaulted-equality"></a>

> **C++ note — defaulted `operator==` (C++20):** writing
> `bool operator==(const Rectangle&) const = default;` asks the compiler to
> generate a comparison that compares each member, in declaration order,
> with that member's own `==`. In C++20 the compiler also rewrites `a != b`
> as `!(a == b)`, so you get `!=` for free. It only works if every member
> can be compared, which is why each nested state struct declares its own
> defaulted `==`. `RenderState`'s `==` calls `DepthTest`'s, which compares a
> `bool` and an enum.
>
> In C#, a `class` compares by reference unless you override `Equals`.
> OpenGlobe's `ColorMask` struct writes `Equals`, `==`, `!=` and
> `GetHashCode` by hand (about 40 lines), and its other classes compare field
> by field inside each `Apply*` method. Here, one line per struct does it.
>
> Two details to know: `float` members compare exactly (which is what a
> cache wants: "is it the same value I sent GL?"), and a defaulted `==` does
> not stop a struct from being an aggregate.

<a id="cpp-designated-initializers"></a>

> **C++ note — designated initializers (C++20):** you can name the members
> you initialize:
>
> ```cpp
> const Rectangle viewport{.left = 0, .bottom = 0, .width = 1280, .height = 720};
> const Rectangle square{.width = 256, .height = 256};   // left and bottom use defaults
> ```
>
> The rules are stricter than C#'s object initializers
> (`new Rectangle { Width = 256 }`): the designators must appear in
> **declaration order**, you can't mix designated and positional values in
> one initializer, and you can't write a nested path like
> `.depthTest.enabled = false`. You nest braces instead:
>
> ```cpp
> RenderState state{.facetCulling = {.enabled = false}, .depthTest = {.enabled = false}};
> ```
>
> Members you leave out get their default member initializers, so the
> `facetCulling` above is still `Back`/`Counterclockwise`. Designated
> initializers need `/std:c++20` on MSVC (CMake's `CMAKE_CXX_STANDARD 20`
> already sets it). This guide uses them where the names make code clearer,
> and plain assignments elsewhere.
>
> GCC's `-Wextra` includes `-Wmissing-field-initializers`, which warns
> about every member left out of a designated initializer, even though
> leaving them out is the point. The code is correct. If the warning is
> noisy in your own targets, add `-Wno-missing-field-initializers`.

### `renderer/include/arda/renderer/Color.h`

```cpp
#pragma once

namespace arda::renderer {

// An RGBA color with each channel in [0, 1]. Replaces System.Drawing.Color,
// which stores bytes. GL and D3D both take floats.
struct Color {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    bool operator==(const Color&) const = default;
};

} // namespace arda::renderer
```

`System.Drawing.Color` stores 8-bit channels and converts to floats when
OpenTK passes it to GL. arda stores floats directly, so the value in the
cache is exactly the value GL was given.

---

## 4. The render state headers

`RenderState` (Listing 3.3) is a collection of smaller structs, one per
fixed-function stage. Each gets its own header in `renderstate/`, following
the same pattern:

- the enums the stage needs, with the same names and order as OpenGlobe;
- a struct whose defaults come from the OpenGlobe constructor;
- a defaulted `operator==`.

The C# classes have PascalCase properties (`DepthTest.Enabled`). arda's
structs have camelCase fields (`depthTest.enabled`), as the README requires.
[Section 4.1](#41-depth-test-listing-34) explains why that is more than
style.

### 4.1 Depth test (Listing 3.4)

> **OpenGL note — the depth test:** the depth buffer stores one depth value
> per pixel, the depth of the nearest surface drawn there so far. When the
> depth test is enabled, GL compares each new fragment's depth with the
> stored value using the depth function. With `GL_LESS`, the fragment passes
> if it is *closer* than what is already there; otherwise it is discarded.
> A passing fragment's depth is written to the depth buffer (if the depth
> mask allows it, see [4.8](#48-color-mask-and-depth-mask)).
>
> - `glEnable(GL_DEPTH_TEST)` / `glDisable(GL_DEPTH_TEST)` turns it on and
>   off. GL's default is **off**.
> - `glDepthFunc(func)` chooses the comparison: `GL_NEVER`, `GL_LESS`,
>   `GL_EQUAL`, `GL_LEQUAL`, `GL_GREATER`, `GL_NOTEQUAL`, `GL_GEQUAL` or
>   `GL_ALWAYS`. GL's default is `GL_LESS`.
>
> **Pitfall:** when the depth test is disabled, GL also stops *writing* the
> depth buffer, even with the depth mask on. If you want to write depth
> without testing it, enable the test and use `Always`.
>
> **Pitfall:** the test needs a depth buffer. Step 0 asks GLFW for 24 depth
> bits. Without them, the test silently passes everything.

#### `renderer/include/arda/renderer/renderstate/DepthTest.h`

```cpp
#pragma once

namespace arda::renderer {

// How an incoming fragment's depth is compared with the stored depth.
enum class DepthTestFunction {
    Never,
    Less,
    Equal,
    LessThanOrEqual,
    Greater,
    NotEqual,
    GreaterThanOrEqual,
    Always,
};

// DepthTest.cs (Listing 3.4)
struct DepthTest {
    bool enabled = true;   // GL's default is disabled (3.3.2)
    DepthTestFunction function = DepthTestFunction::Less;

    bool operator==(const DepthTest&) const = default;
};

} // namespace arda::renderer
```

<a id="cpp-camelcase-fields"></a>

> **C++ note — why fields are camelCase (a name can't change meaning inside
> a class):** in C#, `public DepthTest DepthTest { get; set; }` is normal. C#
> has a special rule (sometimes called the "Color Color" rule) that works
> out whether `DepthTest` means the type or the property. C++ has no such
> rule. Inside a class, once a member named `DepthTest` is declared, the name
> `DepthTest` means the *member* for the rest of the class:
>
> ```cpp
> struct RenderState {
>     DepthTest DepthTest;   // declares a member that hides the type
>     DepthTest other;       // error: 'DepthTest' does not name a type
> };
> ```
>
> The standard goes further: if a name used in a class would mean something
> different when looked up again after the whole class has been read, the
> program is ill-formed (no diagnostic required). Compilers differ in what
> they report. Older GCC versions reject even the single line
> `DepthTest DepthTest;` with "declaration ... changes meaning of
> 'DepthTest'", while others accept that line and fail only at the next use.
> The same problem shows up inside member function bodies, where
> `DepthTest copy = other.DepthTest;` doesn't compile.
>
> The README avoids all of this with one convention: types are PascalCase,
> fields are camelCase, so `DepthTest depthTest;` never clashes.

### 4.2 Facet culling

> **OpenGL note — face culling and winding order:** after the viewport
> transform, GL knows where each triangle's corners are on screen. It
> computes the triangle's signed area. If the vertices go around
> counterclockwise on screen, the triangle is *front-facing* by default,
> otherwise *back-facing*. For a closed mesh such as a globe, the back-facing
> triangles are always hidden behind front-facing ones, so discarding them
> before rasterization roughly halves the fragment work.
>
> - `glEnable(GL_CULL_FACE)` turns culling on. GL's default is **off**.
> - `glCullFace(GL_BACK | GL_FRONT | GL_FRONT_AND_BACK)` chooses what is
>   discarded. Default `GL_BACK`. `GL_FRONT_AND_BACK` discards every
>   triangle (lines and points are still drawn).
> - `glFrontFace(GL_CCW | GL_CW)` chooses which winding is the front.
>   Default `GL_CCW`.
>
> **Pitfall:** if a mesh's triangles are wound the other way from what
> `frontFaceWindingOrder` says, culling removes the side you wanted to see
> and the object looks inside out or disappears. When "nothing draws",
> disabling culling is one of the first things to try.

#### `renderer/include/arda/renderer/renderstate/FacetCulling.h`

```cpp
#pragma once

#include <arda/core/geometry/WindingOrder.h>

namespace arda::renderer {

enum class CullFace {
    Front,
    Back,
    FrontAndBack,
};

// FacetCulling.cs
struct FacetCulling {
    bool enabled = true;   // GL's default is disabled (3.3.2)
    CullFace face = CullFace::Back;
    core::geometry::WindingOrder frontFaceWindingOrder = core::geometry::WindingOrder::Counterclockwise;

    bool operator==(const FacetCulling&) const = default;
};

} // namespace arda::renderer
```

Inside `namespace arda::renderer`, the name `core::geometry::WindingOrder`
is found by looking outward to `arda::core::geometry`, so you don't have to
write `arda::` again.

### 4.3 Primitive restart

> **OpenGL note — primitive restart:** strips (`GL_TRIANGLE_STRIP`,
> `GL_LINE_STRIP`) share vertices between consecutive primitives, which
> saves memory, but one draw call normally produces one connected strip.
> With primitive restart enabled, an indexed draw treats one special index
> value as "end this strip and start a new one". Many strips then fit in a
> single draw, which the book uses for terrain and for tessellated
> ellipsoids.
>
> - `glEnable(GL_PRIMITIVE_RESTART)` turns it on (GL 3.1+). Default **off**.
> - `glPrimitiveRestartIndex(index)` sets the special value. Default 0.
>
> It applies to indexed draws (`glDrawElements` and friends).

OpenGlobe's `PrimitiveRestart` has an `Index` property. arda removes it:
Direct3D 11 only supports the maximum value of the index type (0xFFFF for
16-bit indices, 0xFFFFFFFF for 32-bit), so the README settles on always
using that. The GL backend sets it at draw time from the index buffer's type
(Step 3, `ApplyPrimitiveRestartIndex`).

#### `renderer/include/arda/renderer/renderstate/PrimitiveRestart.h`

```cpp
#pragma once

namespace arda::renderer {

// PrimitiveRestart.cs, without Index. The restart index is always the maximum
// value of the index type (0xFFFF or 0xFFFFFFFF), because Direct3D 11 only
// supports those values. Step 3 sets it at draw time from the index buffer.
struct PrimitiveRestart {
    bool enabled = false;

    bool operator==(const PrimitiveRestart&) const = default;
};

} // namespace arda::renderer
```

### 4.4 Scissor test

> **OpenGL note — the scissor test:** a rectangle in window coordinates.
> When enabled, fragments outside it are discarded. It's cheap and is used
> for split screens, UI panels and limiting work to a region.
>
> - `glEnable(GL_SCISSOR_TEST)`. Default **off**.
> - `glScissor(x, y, width, height)`: bottom-left corner and size. The
>   default box is the window's initial size.
>
> **Pitfall:** unlike the viewport, the scissor test also limits `glClear`.
> A leftover scissor box makes a clear only clear part of the window. See
> [10.5](#105-doclear) for how arda avoids this.

#### `renderer/include/arda/renderer/renderstate/ScissorTest.h`

```cpp
#pragma once

#include <arda/renderer/Rectangle.h>

namespace arda::renderer {

// ScissorTest.cs
struct ScissorTest {
    bool enabled = false;
    Rectangle rectangle;   // {0, 0, 0, 0}

    bool operator==(const ScissorTest&) const = default;
};

} // namespace arda::renderer
```

### 4.5 Stencil test

> **OpenGL note — the stencil test:** the stencil buffer stores a small
> integer (8 bits in arda, from Step 0's `GLFW_STENCIL_BITS`) per pixel.
> The stencil test compares a *reference value* with the stored value, and
> then updates the stored value depending on what happened. It is used for
> masking: draw a shape into the stencil buffer, then draw something else
> only where (or only where not) the shape was.
>
> For each fragment:
>
> 1. **Test:** `(reference & mask) FUNC (stored & mask)`, for example with
>    `GL_LESS` it passes if `(reference & mask) < (stored & mask)`.
> 2. **Update** the stored value with one of three operations, depending on
>    the outcome:
>    - *stencil fail*: the stencil test failed;
>    - *depth fail*: the stencil test passed but the depth test failed;
>    - *depth pass*: both passed.
>
> The operations are `GL_KEEP` (leave it), `GL_ZERO`, `GL_REPLACE` (write the
> reference value), `GL_INCR`/`GL_DECR` (add or subtract 1, clamping at the
> ends), `GL_INCR_WRAP`/`GL_DECR_WRAP` (wrapping around) and `GL_INVERT`
> (flip the bits).
>
> Front-facing and back-facing triangles can have different settings (used
> for shadow volumes), so the calls take a face:
>
> - `glEnable(GL_STENCIL_TEST)`. Default **off**.
> - `glStencilFuncSeparate(face, func, reference, mask)`. Defaults
>   `GL_ALWAYS`, 0, all ones.
> - `glStencilOpSeparate(face, stencilFail, depthFail, depthPass)`. Defaults
>   all `GL_KEEP`.
>
> Points and lines count as front-facing.

The `mask` here is the *comparison* mask. The stencil *write* mask
(`glStencilMask`) is a separate piece of state that OpenGlobe never changes
from GL's default of all ones. arda doesn't expose it either.

#### `renderer/include/arda/renderer/renderstate/StencilTest.h`

OpenGlobe splits this into `StencilTestFace.cs` and `StencilTest.cs`. arda
keeps both in one header, since `StencilTest` is the only user of
`StencilTestFace`.

```cpp
#pragma once

namespace arda::renderer {

// What to do to the stored stencil value (StencilTestFace.cs).
enum class StencilOperation {
    Zero,
    Invert,
    Keep,
    Replace,
    Increment,
    Decrement,
    IncrementWrap,
    DecrementWrap,
};

// How the reference value is compared with the stored stencil value.
enum class StencilTestFunction {
    Never,
    Less,
    Equal,
    LessThanOrEqual,
    Greater,
    NotEqual,
    GreaterThanOrEqual,
    Always,
};

// The stencil settings for one facing, front or back (StencilTestFace.cs).
struct StencilTestFace {
    StencilOperation stencilFailOperation = StencilOperation::Keep;
    StencilOperation depthFailStencilPassOperation = StencilOperation::Keep;
    StencilOperation depthPassStencilPassOperation = StencilOperation::Keep;
    StencilTestFunction function = StencilTestFunction::Always;
    int referenceValue = 0;
    int mask = ~0;   // all bits set

    bool operator==(const StencilTestFace&) const = default;
};

// StencilTest.cs
struct StencilTest {
    bool enabled = false;
    StencilTestFace frontFace;
    StencilTestFace backFace;

    bool operator==(const StencilTest&) const = default;
};

} // namespace arda::renderer
```

`~0` is the bitwise complement of the `int` 0, so every bit is 1 (the value
is -1). GL takes the mask as a `GLuint`, and the backend converts it with
`static_cast<GLuint>`, which gives 0xFFFFFFFF.

### 4.6 Depth range

> **OpenGL note — the depth range:** after the perspective divide, a
> vertex's depth is in normalized device coordinates, z in [-1, 1] on GL.
> The viewport transform maps that to the value stored in the depth buffer:
>
> ```
> z_window = z_ndc * (far - near) / 2 + (far + near) / 2
> ```
>
> `glDepthRange(near, far)` sets `near` and `far`, default 0 and 1, so the
> whole [-1, 1] range maps to [0, 1]. Changing it squeezes a draw into part
> of the depth buffer. For example, drawing a sky with the range (1, 1)
> places it behind everything. GL clamps both values to [0, 1]. OpenGlobe
> throws instead, so a bad value is noticed rather than silently changed.

#### `renderer/include/arda/renderer/renderstate/DepthRange.h`

```cpp
#pragma once

namespace arda::renderer {

// DepthRange.cs. Maps normalized device depth to window depth. Both values
// must be in [0, 1].
struct DepthRange {
    // Not "near" and "far": <windows.h> defines both as empty macros.
    double nearValue = 0.0;
    double farValue = 1.0;

    bool operator==(const DepthRange&) const = default;
};

} // namespace arda::renderer
```

<a id="cpp-macro-names"></a>

> **C++ note — macro names (`near`, `far`, `min`, `max`, `CreateWindow`):**
> the C# class calls these `Near` and `Far`, and the natural C++ names would
> be `near` and `far`. But `<windows.h>` (through `<minwindef.h>`) contains
> `#define near` and `#define far`, left over from 16-bit Windows pointers.
> The preprocessor runs before the compiler and replaces every `near` token
> with nothing, so in any file that includes `<windows.h>` first,
>
> ```cpp
> double near = 0.0;
> ```
>
> becomes `double = 0.0;` and fails with a confusing error pointing at your
> header. Macros ignore namespaces and scopes, so putting the struct in
> `arda::renderer` doesn't help.
>
> arda's public headers never include `<windows.h>`, but the files that
> include *them* might: Step 8's D3D11 backend includes `<d3d11.h>`, which
> includes `<windows.h>`, and then `RenderState.h`. The safe rule is to
> avoid names that are known macros:
>
> - `near`, `far`: use `nearValue`, `farValue`;
> - `min`, `max`: also macros in `<windows.h>` unless `NOMINMAX` is defined,
>   which is one reason `BlendEquation` says `Minimum` and `Maximum`;
> - `CreateWindow`: a macro for `CreateWindowA`/`CreateWindowW`, which is
>   why the device's function is `CreateGraphicsWindow` (Step 0);
> - on Linux, X11's `<X11/X.h>` defines `None` and `Always` as macros. arda
>   still uses `Always` as an enumerator because the renderer never includes
>   X11 headers in a file that also includes these headers.
>
> `<wingdi.h>` also declares a *function* called `Rectangle` in the global
> namespace. That one is not a macro, so namespaces do keep it apart from
> `arda::renderer::Rectangle`. Code inside `arda::renderer` finds arda's
> `Rectangle` first.

### 4.7 Blending

> **OpenGL note — blending:** without blending, a fragment's color replaces
> the color already in the framebuffer. With blending, GL combines the
> fragment's color (the *source*) with the stored color (the *destination*):
>
> ```
> result = source * sourceFactor  EQUATION  destination * destinationFactor
> ```
>
> The classic transparency setup is `SourceAlpha` and `OneMinusSourceAlpha`
> with `Add`: `result = src * src.a + dst * (1 - src.a)`.
>
> - `glEnable(GL_BLEND)`. Default **off**.
> - `glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha)` sets the
>   factors separately for the color and alpha channels. Defaults `GL_ONE`,
>   `GL_ZERO`, which just writes the source.
> - `glBlendEquationSeparate(rgbEquation, alphaEquation)`: `GL_FUNC_ADD`,
>   `GL_FUNC_SUBTRACT` (src − dst), `GL_FUNC_REVERSE_SUBTRACT` (dst − src),
>   `GL_MIN` or `GL_MAX`. Default `GL_FUNC_ADD`. `GL_MIN` and `GL_MAX` ignore
>   the factors.
> - `glBlendColor(r, g, b, a)` sets a constant color, used by the
>   `Constant*` factors. Default (0, 0, 0, 0).
>
> **Pitfall:** notice the argument order of `glBlendFuncSeparate`: source
> RGB, *destination RGB*, source alpha, destination alpha. It's easy to pass
> the two source factors first.

OpenGlobe has two factor enums, because older GL versions allowed some
factors on only one side: `SourceAlphaSaturate` is a source-only factor, and
`SourceColor` and `OneMinusSourceColor` appear only as destination factors.
arda keeps the same split.

#### `renderer/include/arda/renderer/renderstate/Blending.h`

```cpp
#pragma once

#include <arda/renderer/Color.h>

namespace arda::renderer {

enum class SourceBlendingFactor {
    Zero,
    One,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    DestinationColor,
    OneMinusDestinationColor,
    SourceAlphaSaturate,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
};

enum class DestinationBlendingFactor {
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha,
    DestinationColor,
    OneMinusDestinationColor,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
};

enum class BlendEquation {
    Add,
    Minimum,
    Maximum,
    Subtract,
    ReverseSubtract,
};

// Blending.cs. The defaults (One, Zero, Add) write the source color unchanged.
struct Blending {
    bool enabled = false;
    SourceBlendingFactor sourceRGBFactor = SourceBlendingFactor::One;
    SourceBlendingFactor sourceAlphaFactor = SourceBlendingFactor::One;
    DestinationBlendingFactor destinationRGBFactor = DestinationBlendingFactor::Zero;
    DestinationBlendingFactor destinationAlphaFactor = DestinationBlendingFactor::Zero;
    BlendEquation rgbEquation = BlendEquation::Add;
    BlendEquation alphaEquation = BlendEquation::Add;
    Color color;   // the constant color used by the Constant* factors: (0, 0, 0, 0)

    bool operator==(const Blending&) const = default;
};

} // namespace arda::renderer
```

### 4.8 Color mask and depth mask

> **OpenGL note — write masks:** masks decide which buffers a fragment that
> passed every test is allowed to write. They don't affect testing.
>
> - `glColorMask(r, g, b, a)`: each `GL_TRUE` channel is written. Default
>   all `GL_TRUE`. Masking alpha is common when the window's alpha channel
>   must stay at 1.
> - `glDepthMask(flag)`: `GL_FALSE` keeps the depth *test* running but stops
>   depth *writes*. The usual use is transparent objects: they are hidden by
>   opaque objects in front of them, but they don't hide what is drawn after
>   them. Default `GL_TRUE`.
>
> **Pitfall:** the masks also apply to `glClear`. With the depth mask off,
> `glClear(GL_DEPTH_BUFFER_BIT)` does nothing, and the next frame's depth
> test compares against the previous frame's depth. That's a classic bug
> with a global-state renderer. [10.5](#105-doclear) shows how arda rules it
> out.

`ColorMask` gets its own struct. The depth mask is a single `bool` and lives
directly in `RenderState`, as in OpenGlobe.

#### `renderer/include/arda/renderer/renderstate/ColorMask.h`

```cpp
#pragma once

namespace arda::renderer {

// ColorMask.cs. true means the channel is written.
struct ColorMask {
    bool red = true;
    bool green = true;
    bool blue = true;
    bool alpha = true;

    bool operator==(const ColorMask&) const = default;
};

} // namespace arda::renderer
```

The C# `ColorMask` is an immutable struct with a four-argument constructor
and read-only properties. Here it is a mutable aggregate like the others,
so `ColorMask{true, true, true, false}` or
`ColorMask{.alpha = false}` both work.

### 4.9 `RenderState` (Listing 3.3)

Two stages don't need a struct of their own.

> **OpenGL note — program point size:** when drawing `GL_POINTS`, each point
> is a square of some size in pixels. By default the size comes from
> `glPointSize` (1 pixel). With `glEnable(GL_PROGRAM_POINT_SIZE)`, the vertex
> shader sets it per vertex by writing `gl_PointSize`. The book uses it for
> point-based rendering. Default **off**. If you enable it and the shader
> doesn't write `gl_PointSize`, the size is undefined.

> **OpenGL note — polygon mode (rasterization mode):**
> `glPolygonMode(GL_FRONT_AND_BACK, mode)` chooses how triangles are
> rasterized: `GL_FILL` (normal, the default), `GL_LINE` (only edges, a
> wireframe) or `GL_POINT` (only corners). It only affects triangles, and it
> happens after culling. It's mostly a debugging aid.
>
> **Pitfall:** in a core profile the first argument *must* be
> `GL_FRONT_AND_BACK`. Passing `GL_FRONT` or `GL_BACK` (as older tutorials
> do) is a `GL_INVALID_ENUM` error, and GL ignores the call. GL doesn't
> report errors unless you ask with `glGetError`, so the only symptom is
> that nothing changes.

#### `renderer/include/arda/renderer/renderstate/RenderState.h`

```cpp
#pragma once

#include <arda/renderer/renderstate/Blending.h>
#include <arda/renderer/renderstate/ColorMask.h>
#include <arda/renderer/renderstate/DepthRange.h>
#include <arda/renderer/renderstate/DepthTest.h>
#include <arda/renderer/renderstate/FacetCulling.h>
#include <arda/renderer/renderstate/PrimitiveRestart.h>
#include <arda/renderer/renderstate/ScissorTest.h>
#include <arda/renderer/renderstate/StencilTest.h>

namespace arda::renderer {

// When enabled, the vertex shader sets the point size with gl_PointSize.
// GL only: Direct3D 11 always draws one-pixel points and ignores it.
enum class ProgramPointSize {
    Enabled,
    Disabled,
};

// How triangles are rasterized. Direct3D 11 has no Point mode and throws for it.
enum class RasterizationMode {
    Point,
    Line,
    Fill,
};

// RenderState.cs (Listing 3.3). Configures the fixed-function parts of the
// pipeline for one draw call. The fields are in the same order as the C# class.
struct RenderState {
    PrimitiveRestart primitiveRestart;
    FacetCulling facetCulling;
    ProgramPointSize programPointSize = ProgramPointSize::Disabled;
    RasterizationMode rasterizationMode = RasterizationMode::Fill;
    ScissorTest scissorTest;
    StencilTest stencilTest;
    DepthTest depthTest;
    DepthRange depthRange;
    Blending blending;
    ColorMask colorMask;
    bool depthMask = true;

    bool operator==(const RenderState&) const = default;
};

} // namespace arda::renderer
```

The struct members without an `=` (`primitiveRestart`, `facetCulling` and so
on) are still initialized: each is a struct whose own members have
defaults. Only fields of built-in type (`bool`, `int`, `float`, enums) need
an explicit initializer.

`ProgramPointSize` is an enum rather than a `bool` because OpenGlobe made it
one. It's kept so ported code reads the same.

> **Why:** *OpenGlobe's defaults differ from GL's.* GL was designed so that
> a fresh context draws *something* whatever you do: no depth test, no
> culling. A virtual globe almost always wants both, so OpenGlobe makes
> "depth test on, back faces culled" the defaults (3.3.2). A default-built
> `RenderState` is then right for opaque geometry, and code only mentions
> what it changes. This is also why the context must sync GL with its cache
> at startup. The two sets of defaults disagree, so a cache that simply
> *assumed* GL's state would be wrong from the start.
>
> | State | GL default | `RenderState` default |
> |---|---|---|
> | Depth test | off, `GL_LESS` | **on**, `Less` |
> | Face culling | off, `GL_BACK`, `GL_CCW` | **on**, `Back`, `Counterclockwise` |
> | Everything else | | same as GL |

Setting up billboard render state (3.3.2), which is blended and not culled,
now looks like this:

```cpp
RenderState renderState;
renderState.facetCulling.enabled = false;
renderState.blending.enabled = true;
renderState.blending.sourceRGBFactor = SourceBlendingFactor::SourceAlpha;
renderState.blending.sourceAlphaFactor = SourceBlendingFactor::SourceAlpha;
renderState.blending.destinationRGBFactor = DestinationBlendingFactor::OneMinusSourceAlpha;
renderState.blending.destinationAlphaFactor = DestinationBlendingFactor::OneMinusSourceAlpha;
```

or, with designated initializers (in declaration order):

```cpp
const RenderState renderState{
    .facetCulling = {.enabled = false},
    .blending = {
        .enabled = true,
        .sourceRGBFactor = SourceBlendingFactor::SourceAlpha,
        .sourceAlphaFactor = SourceBlendingFactor::SourceAlpha,
        .destinationRGBFactor = DestinationBlendingFactor::OneMinusSourceAlpha,
        .destinationAlphaFactor = DestinationBlendingFactor::OneMinusSourceAlpha,
    },
};
```

Nothing global changed. The billboards' draw carries this state, and the
next draw carries its own.

---

## 5. `ClearState`

`ClearState` (Listing 3.6) describes a clear the same way `RenderState`
describes a draw: which buffers to clear, and the values to clear them to.

> **OpenGL note — clearing:** clearing takes two kinds of GL call.
>
> - `glClearColor(r, g, b, a)`, `glClearDepth(depth)` and
>   `glClearStencil(s)` set the *clear values*. Like everything else, these
>   are state variables, with defaults (0, 0, 0, 0), 1.0 and 0.
> - `glClear(mask)` fills the buffers named in `mask`
>   (`GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT`)
>   with the current clear values.
>
> `glClear` ignores the viewport, the depth test, blending and the shaders.
> It is affected by the scissor test and by the color, depth and stencil
> write masks. Clearing depth to 1.0, the farthest value, is what makes
> `Less` work at the start of a frame: everything drawn is closer than 1.0.
>
> Clear every buffer you use at the start of each frame, even if you're
> about to draw over every pixel. GPUs, especially tiled mobile GPUs, use
> the clear as a hint that the old contents aren't needed.

#### `renderer/include/arda/renderer/ClearState.h`

```cpp
#pragma once

#include <arda/renderer/Color.h>

namespace arda::renderer {

// Which buffers Context::Clear clears (Context.cs). The values are bit flags,
// so they can be combined with |.
enum class ClearBuffers : unsigned {
    ColorBuffer = 1,
    DepthBuffer = 2,
    StencilBuffer = 4,
    ColorAndDepthBuffer = ColorBuffer | DepthBuffer,
    All = ColorBuffer | DepthBuffer | StencilBuffer,
};

constexpr ClearBuffers operator|(ClearBuffers a, ClearBuffers b) {
    return static_cast<ClearBuffers>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

// True if any bit of flag is set in value.
constexpr bool HasFlag(ClearBuffers value, ClearBuffers flag) {
    return (static_cast<unsigned>(value) & static_cast<unsigned>(flag)) != 0;
}

// ClearState.cs (Listing 3.6), without the scissor test and the write masks.
// Clears always ignore those (see the README's portability table).
struct ClearState {
    ClearBuffers buffers = ClearBuffers::All;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};   // OpenGlobe's default is white
    float depth = 1.0f;
    int stencil = 0;
};

} // namespace arda::renderer
```

<a id="cpp-flag-enums"></a>

> **C++ note — bit-flag operators for `enum class`:** in C#, a `[Flags]`
> enum supports `|` and `&` out of the box. A C++ `enum class` supports
> neither. That's deliberate: scoped enums don't convert to integers
> implicitly, so you can't accidentally add a `ClearBuffers` to an `int`. For
> a flag enum you add the operators you need yourself:
>
> ```cpp
> constexpr ClearBuffers operator|(ClearBuffers a, ClearBuffers b) {
>     return static_cast<ClearBuffers>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
> }
> ```
>
> - Casting to the *underlying type* (`: unsigned`, declared on the enum)
>   gives plain integers to combine. Casting back is allowed even when the
>   result, like 3 or 6, isn't a named enumerator. Any value that fits in
>   the underlying type is a valid `ClearBuffers`.
> - The operator is a free function in the same namespace as the enum. When
>   you write `a | b`, the compiler looks for `operator|` in the namespaces
>   of the arguments' types (argument-dependent lookup), so it's found even
>   from code in another namespace.
> - `constexpr` means the function *can* run at compile time when its
>   arguments are constants. Then the result can be used where C++ needs a
>   constant, such as `static_assert` or a `case` label:
>
>   ```cpp
>   static_assert(HasFlag(ClearBuffers::DepthBuffer | ClearBuffers::StencilBuffer,
>                         ClearBuffers::StencilBuffer));
>   ```
>
>   With runtime arguments it's an ordinary function call. `constexpr`
>   functions are implicitly `inline` (see
>   [the `inline` note](#cpp-inline) below), which is why they can be
>   defined in a header.
> - `HasFlag` replaces C#'s `(mask & ClearBuffers.ColorBuffer) != 0` and
>   `Enum.HasFlag`. arda doesn't define `operator&` because nothing needs
>   it yet.
>
> One subtle point explains why `ColorAndDepthBuffer = ColorBuffer | DepthBuffer`
> compiles even though it comes before `operator|` is declared. *Inside* the
> braces of an enum with a fixed underlying type, the enumerators have the
> underlying type (`unsigned`), so that `|` is the built-in integer one.
> After the closing brace they have type `ClearBuffers`, and then only the
> custom operator works.

> **Why:** *clears ignore the scissor test and masks.* OpenGlobe's
> `ClearState` also has `ScissorTest`, `ColorMask`, `DepthMask` and stencil
> masks, because `glClear` respects them. Direct3D 11's
> `ClearRenderTargetView` and `ClearDepthStencilView` always clear the whole
> view and ignore all of them. A scissored or masked clear would be
> impossible to implement on D3D11 without drawing a quad, so arda removes
> the fields (README, portability table). The GL backend resets the scissor
> test and masks before `glClear`, so both backends behave the same. A
> partial clear, if you need one later, can be done by drawing a
> full-viewport quad with the right render state.

`ClearState` has no `operator==`. Nothing compares whole clear states; the
GL context caches the three values individually.

---

## 6. `DrawState`

`DrawState` (Listing 3.5) bundles everything a draw call needs. In this
step it only has the render state. Step 3 adds `shaderProgram` and
`vertexArray`.

#### `renderer/include/arda/renderer/DrawState.h`

```cpp
#pragma once

#include <arda/renderer/renderstate/RenderState.h>

namespace arda::renderer {

// DrawState.cs (Listing 3.5). Everything a draw call needs.
// Step 3 adds shaderProgram and vertexArray.
struct DrawState {
    RenderState renderState;
};

} // namespace arda::renderer
```

OpenGlobe's `DrawState` has a constructor that takes all three parts. An
aggregate doesn't need one: Step 3 fills the fields directly, or uses
`DrawState{renderState, shaderProgram, vertexArray}`.

---

## 7. Checkpoint A: the plain-data tests

Everything so far is header-only, and a header is only compiled when a
`.cpp` file includes it. The first test file includes them all, so now is
the time to write it: type in
[`RenderStateTests.cpp`](#testssrcrendererrenderstatetestscpp) from the
Tests section.

Then add it, and the new headers, to CMake:

- in `tests/CMakeLists.txt`, add `src/renderer/RenderStateTests.cpp` to
  `add_executable(arda_tests ...)`;
- in `renderer/CMakeLists.txt`, add the new headers to the header list (the
  [complete file](#renderercmakeliststxt) is in the CMake section).

Build and run the tests:

```
./run.sh -t arda_tests
```

All the `RenderState` tests should pass. If one of the default checks fails,
compare that field's initializer with the OpenGlobe constructor.

---

## 8. The GL type converter

Public headers use arda's own enums. Only the GL backend knows GL's values.
OpenGlobe's `TypeConverterGL3x` is a static class of overloaded `To` methods.
In C++, a set of overloaded free functions in the backend's namespace does
the same job. A static class is a C# workaround for a language with no free
functions, and C++ doesn't need it.

The name is `ToGL` rather than `To`. It reads better at the call site
(`glDepthFunc(ToGL(depthTest.function))`), and Step 8's `ToD3D` functions can
then live next to these without confusion.

### `renderer/src/gl/TypeConverterGL3x.h`

```cpp
#pragma once

#include <arda/core/geometry/WindingOrder.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/renderstate/RenderState.h>

#include <glad/glad.h>

namespace arda::renderer::gl {

// GL has its own boolean type (an unsigned char). Converting explicitly keeps
// compilers quiet and makes the GL_TRUE/GL_FALSE intent visible.
inline GLboolean ToGLBoolean(bool value) {
    return value ? GL_TRUE : GL_FALSE;
}

// TypeConverterGL3x.cs. Each function throws std::invalid_argument for a
// value that isn't one of the enum's enumerators.
GLbitfield ToGL(ClearBuffers buffers);
GLenum ToGL(DepthTestFunction function);
GLenum ToGL(StencilTestFunction function);
GLenum ToGL(StencilOperation operation);
GLenum ToGL(CullFace face);
GLenum ToGL(core::geometry::WindingOrder windingOrder);
GLenum ToGL(RasterizationMode mode);
GLenum ToGL(SourceBlendingFactor factor);
GLenum ToGL(DestinationBlendingFactor factor);
GLenum ToGL(BlendEquation equation);

} // namespace arda::renderer::gl
```

This is a private header in `src/gl/`, so it may include glad. Scene code
never sees it (README, "Why glad, GLFW and D3D headers are private").

<a id="cpp-inline"></a>

> **C++ note — `inline` functions in headers:** a header is pasted into
> every `.cpp` that includes it. If a header *defines* an ordinary function,
> each of those `.cpp` files gets its own definition, and the linker fails
> with "multiple definition" (the one-definition rule). Marking the function
> `inline` tells the linker that identical definitions in several files are
> expected, and to keep one.
>
> ```cpp
> // In a header:
> GLboolean ToGLBoolean(bool value) { ... }          // link error once two .cpp files include it
> inline GLboolean ToGLBoolean(bool value) { ... }   // fine
> ```
>
> Despite the name, `inline` today is about linking, not speed. The
> optimizer decides on its own whether to inline a call. Some functions are
> inline without the keyword: member functions defined inside the class body
> (`Rectangle::Right`), `constexpr` functions (`HasFlag`), and templates. Put
> small helpers like `ToGLBoolean` in the header as `inline`, and put
> anything larger in the `.cpp` (like the `ToGL` functions) so a change to
> it doesn't recompile every file that includes the header.

### `renderer/src/gl/TypeConverterGL3x.cpp`

Each conversion is a `switch` with one `case` per enumerator and **no
`default:`**, followed by a `throw`. The note after the code explains why.
The GL constants are the ones listed in the OpenGL notes of
[part 4](#4-the-render-state-headers). The C# names map onto them
directly, for example OpenTK's `DepthFunction.Lequal` is `GL_LEQUAL` and
`StencilOp.Incr` is `GL_INCR`.

```cpp
#include "gl/TypeConverterGL3x.h"

#include <stdexcept>

namespace arda::renderer::gl {

// A flags enum can't be converted with a switch: any combination of bits is
// valid. Test each bit instead.
GLbitfield ToGL(ClearBuffers buffers) {
    GLbitfield mask = 0;
    if (HasFlag(buffers, ClearBuffers::ColorBuffer)) {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (HasFlag(buffers, ClearBuffers::DepthBuffer)) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (HasFlag(buffers, ClearBuffers::StencilBuffer)) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    return mask;
}

GLenum ToGL(DepthTestFunction function) {
    switch (function) {
    case DepthTestFunction::Never:              return GL_NEVER;
    case DepthTestFunction::Less:               return GL_LESS;
    case DepthTestFunction::Equal:              return GL_EQUAL;
    case DepthTestFunction::LessThanOrEqual:    return GL_LEQUAL;
    case DepthTestFunction::Greater:            return GL_GREATER;
    case DepthTestFunction::NotEqual:           return GL_NOTEQUAL;
    case DepthTestFunction::GreaterThanOrEqual: return GL_GEQUAL;
    case DepthTestFunction::Always:             return GL_ALWAYS;
    }
    throw std::invalid_argument("Invalid DepthTestFunction");
}

GLenum ToGL(StencilTestFunction function) {
    switch (function) {
    case StencilTestFunction::Never:              return GL_NEVER;
    case StencilTestFunction::Less:               return GL_LESS;
    case StencilTestFunction::Equal:              return GL_EQUAL;
    case StencilTestFunction::LessThanOrEqual:    return GL_LEQUAL;
    case StencilTestFunction::Greater:            return GL_GREATER;
    case StencilTestFunction::NotEqual:           return GL_NOTEQUAL;
    case StencilTestFunction::GreaterThanOrEqual: return GL_GEQUAL;
    case StencilTestFunction::Always:             return GL_ALWAYS;
    }
    throw std::invalid_argument("Invalid StencilTestFunction");
}

GLenum ToGL(StencilOperation operation) {
    switch (operation) {
    case StencilOperation::Zero:          return GL_ZERO;
    case StencilOperation::Invert:        return GL_INVERT;
    case StencilOperation::Keep:          return GL_KEEP;
    case StencilOperation::Replace:       return GL_REPLACE;
    case StencilOperation::Increment:     return GL_INCR;
    case StencilOperation::Decrement:     return GL_DECR;
    case StencilOperation::IncrementWrap: return GL_INCR_WRAP;
    case StencilOperation::DecrementWrap: return GL_DECR_WRAP;
    }
    throw std::invalid_argument("Invalid StencilOperation");
}

GLenum ToGL(CullFace face) {
    switch (face) {
    case CullFace::Front:        return GL_FRONT;
    case CullFace::Back:         return GL_BACK;
    case CullFace::FrontAndBack: return GL_FRONT_AND_BACK;
    }
    throw std::invalid_argument("Invalid CullFace");
}

GLenum ToGL(core::geometry::WindingOrder windingOrder) {
    switch (windingOrder) {
    case core::geometry::WindingOrder::Clockwise:        return GL_CW;
    case core::geometry::WindingOrder::Counterclockwise: return GL_CCW;
    }
    throw std::invalid_argument("Invalid WindingOrder");
}

GLenum ToGL(RasterizationMode mode) {
    switch (mode) {
    case RasterizationMode::Point: return GL_POINT;
    case RasterizationMode::Line:  return GL_LINE;
    case RasterizationMode::Fill:  return GL_FILL;
    }
    throw std::invalid_argument("Invalid RasterizationMode");
}

GLenum ToGL(SourceBlendingFactor factor) {
    switch (factor) {
    case SourceBlendingFactor::Zero:                     return GL_ZERO;
    case SourceBlendingFactor::One:                      return GL_ONE;
    case SourceBlendingFactor::SourceAlpha:              return GL_SRC_ALPHA;
    case SourceBlendingFactor::OneMinusSourceAlpha:      return GL_ONE_MINUS_SRC_ALPHA;
    case SourceBlendingFactor::DestinationAlpha:         return GL_DST_ALPHA;
    case SourceBlendingFactor::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
    case SourceBlendingFactor::DestinationColor:         return GL_DST_COLOR;
    case SourceBlendingFactor::OneMinusDestinationColor: return GL_ONE_MINUS_DST_COLOR;
    case SourceBlendingFactor::SourceAlphaSaturate:      return GL_SRC_ALPHA_SATURATE;
    case SourceBlendingFactor::ConstantColor:            return GL_CONSTANT_COLOR;
    case SourceBlendingFactor::OneMinusConstantColor:    return GL_ONE_MINUS_CONSTANT_COLOR;
    case SourceBlendingFactor::ConstantAlpha:            return GL_CONSTANT_ALPHA;
    case SourceBlendingFactor::OneMinusConstantAlpha:    return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    throw std::invalid_argument("Invalid SourceBlendingFactor");
}

GLenum ToGL(DestinationBlendingFactor factor) {
    switch (factor) {
    case DestinationBlendingFactor::Zero:                     return GL_ZERO;
    case DestinationBlendingFactor::One:                      return GL_ONE;
    case DestinationBlendingFactor::SourceColor:              return GL_SRC_COLOR;
    case DestinationBlendingFactor::OneMinusSourceColor:      return GL_ONE_MINUS_SRC_COLOR;
    case DestinationBlendingFactor::SourceAlpha:              return GL_SRC_ALPHA;
    case DestinationBlendingFactor::OneMinusSourceAlpha:      return GL_ONE_MINUS_SRC_ALPHA;
    case DestinationBlendingFactor::DestinationAlpha:         return GL_DST_ALPHA;
    case DestinationBlendingFactor::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
    case DestinationBlendingFactor::DestinationColor:         return GL_DST_COLOR;
    case DestinationBlendingFactor::OneMinusDestinationColor: return GL_ONE_MINUS_DST_COLOR;
    case DestinationBlendingFactor::ConstantColor:            return GL_CONSTANT_COLOR;
    case DestinationBlendingFactor::OneMinusConstantColor:    return GL_ONE_MINUS_CONSTANT_COLOR;
    case DestinationBlendingFactor::ConstantAlpha:            return GL_CONSTANT_ALPHA;
    case DestinationBlendingFactor::OneMinusConstantAlpha:    return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    throw std::invalid_argument("Invalid DestinationBlendingFactor");
}

GLenum ToGL(BlendEquation equation) {
    switch (equation) {
    case BlendEquation::Add:             return GL_FUNC_ADD;
    case BlendEquation::Minimum:         return GL_MIN;
    case BlendEquation::Maximum:         return GL_MAX;
    case BlendEquation::Subtract:        return GL_FUNC_SUBTRACT;
    case BlendEquation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    }
    throw std::invalid_argument("Invalid BlendEquation");
}

} // namespace arda::renderer::gl
```

<a id="cpp-switch-without-default"></a>

> **C++ note — `switch` without `default`, and the warnings that catch a
> missing case:** when a `switch` over an enum has no `default:` and misses
> an enumerator, compilers can warn:
>
> - GCC and Clang: `-Wswitch`, part of `-Wall`: "enumeration value
>   'Minimum' not handled in switch".
> - MSVC: warning C4062 ("enumerator ... in switch of enum ... is not
>   handled"). It is **off by default, even at `/W4`**, so arda turns it on
>   with `/w44062` ("report C4062 at level 4").
>
> That warning is the reason to leave `default:` out. If someone later adds
> `DepthTestFunction::Approximately`, every converter that forgot it is
> flagged at compile time. A `default:` would silence the warning and turn
> the mistake into a runtime error, or worse, a wrong GL value.
>
> The `throw` *after* the switch is still needed, for two reasons. First, an
> `enum class` variable can hold values that aren't enumerators:
> `static_cast<DepthTestFunction>(42)` is legal, and so is reading a
> corrupted or uninitialized value. Second, without a statement after the
> switch, the compiler sees a path where the function reaches its end
> without returning, which is undefined behaviour for a non-`void` function,
> and it warns ("not all control paths return a value", C4715). This matches
> OpenGlobe, which throws `ArgumentException` after each switch.
>
> Each `case` returns, so there is no fall-through. In a `switch` whose
> cases *don't* return, remember that C++ (unlike C#) silently falls through
> to the next case unless you write `break`.

### Turn on the warnings

Add warning flags to `arda_renderer` so the switch checks actually run. Put
this after `add_library(arda_renderer ...)` in `renderer/CMakeLists.txt` (the
[complete file](#renderercmakeliststxt) is below):

```cmake
# Warnings, including a missing enumerator in a switch (-Wswitch is in -Wall;
# MSVC's C4062 is off by default, so /w44062 enables it at level 4).
if(MSVC)
    target_compile_options(arda_renderer PRIVATE /W4 /w44062)
else()
    target_compile_options(arda_renderer PRIVATE -Wall -Wextra)
endif()
```

`PRIVATE` means the flags apply to the renderer's own sources only, not to
the scene or the tests. `/W4` may show a few warnings in your Step 0 code,
such as an unused parameter. They're worth reading and fixing, but they
don't stop the build.

### Checkpoint B

Add `src/gl/TypeConverterGL3x.cpp` (and the `.h`) to the GL sources in
`renderer/CMakeLists.txt` and build `arda_renderer`. Nothing calls the
converters yet, but they must compile without warnings. To see the switch
warning in action, temporarily delete one `case` line and rebuild.

---

## 9. `Context`: `Clear` and the viewport

`Context` gets two new abilities: clearing, and a viewport. Both follow the
non-virtual interface pattern from Step 0 (see
[Step 0](00-setup.md), "non-virtual interface"): a public non-virtual
function does the shared work, then calls a protected pure virtual `Do*`
function that each backend implements.

> **OpenGL note — the viewport:** `glViewport(x, y, width, height)` sets the
> rectangle of the window, in pixels from the bottom-left, that normalized
> device coordinates map onto. NDC x = -1 goes to the rectangle's left edge,
> +1 to its right edge, and likewise for y:
>
> ```
> x_window = x + (x_ndc + 1) * width / 2
> y_window = y + (y_ndc + 1) * height / 2
> ```
>
> When a context is first made current on a window, GL sets the viewport to
> the window's size. **It never changes it again.** When the window is
> resized, the viewport keeps its old size, and the image is drawn into a
> corner or cut off. That's why the scene sets the viewport in its resize
> handler.
>
> Two pitfalls:
>
> - Use the **framebuffer** size, not the window size. On high-DPI screens
>   they differ (a 1280×720 window can have a 2560×1440 framebuffer).
>   `GraphicsWindow::Width()` and `Height()` already return the framebuffer
>   size (Step 0).
> - The viewport does **not** limit `glClear`. A clear always covers the
>   whole buffer (unless scissored). So in this step you can't see the
>   viewport working yet. You'll see it in Step 3, when triangles are drawn.

### `renderer/include/arda/renderer/Context.h`

This is the whole file after this step. The new parts are the two
includes, `Clear`, `GetViewport`, `SetViewport`, `DoClear`, `DoSetViewport`
and `m_viewport`.

```cpp
#pragma once

#include <arda/renderer/ClearState.h>
#include <arda/renderer/Rectangle.h>

namespace arda::renderer {

class Device;

// Issues rendering commands for one window (3.2, Listing 3.2).
// Steps 3, 5 and 6 add Draw, texture units and framebuffers.
class Context {
public:
    virtual ~Context() = default;

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    Device& GetDevice() const { return m_device; }

    virtual void MakeCurrent() = 0;

    // Clears the buffers named by clearState.buffers (3.3.5). The scissor
    // test and the write masks never affect a clear.
    void Clear(const ClearState& clearState) { DoClear(clearState); }

    // The part of the window (or framebuffer) that draws map to, in pixels,
    // with the origin at the bottom-left.
    const Rectangle& GetViewport() const { return m_viewport; }

    // Throws std::invalid_argument if the width or height is negative.
    void SetViewport(const Rectangle& viewport);

protected:
    explicit Context(Device& device);

    virtual void DoClear(const ClearState& clearState) = 0;

    // Called only when the viewport actually changes.
    virtual void DoSetViewport(const Rectangle& viewport) = 0;

private:
    Device& m_device;
    Rectangle m_viewport;   // {0, 0, 0, 0} until the backend's constructor sets it
};

} // namespace arda::renderer
```

C#'s `Viewport` is a property with a getter and setter. C++ has no
properties, so it becomes `GetViewport` and `SetViewport` (README naming
conventions). `Clear` has no validation, so it simply forwards. It is still
non-virtual, so that later steps can add shared checks without touching the
backends.

<a id="cpp-const-ref-vs-value"></a>

> **C++ note — pass by `const&` or by value:** in C#, a `class` argument is
> always passed as a reference, and a `struct` argument is always copied. In
> C++ *you* choose, per parameter:
>
> ```cpp
> void SetViewport(const Rectangle& viewport);   // no copy; can't modify the caller's object
> void ApplyDepthMask(bool depthMask);           // copied; a bool is cheaper to copy than to point at
> ```
>
> The usual rule:
>
> - **By value** for things that are cheap to copy: built-in types (`bool`,
>   `int`, `float`, `double`), enums, pointers, and very small structs.
> - **By `const&`** for anything larger or that owns memory: `RenderState`
>   (over 100 bytes), `ClearState`, `std::string`. The function sees the
>   caller's object directly and promises not to change it.
> - **By `&` (non-const)** only when the function is meant to modify the
>   argument, like `ApplyStencil`'s `current` parameter in
>   [10.4](#104-the-apply-methods).
>
> `Rectangle` (16 bytes) is on the border. arda passes it by `const&` to
> match the other state structs. Either choice is fine.
>
> Returning by `const&`, as `GetViewport` does, avoids a copy too. The
> caller must not keep the reference longer than the `Context` exists, which
> is easy here.
>
> A `const&` parameter also binds to a temporary:
> `ApplyScissorTest(ScissorTest{})` creates a default `ScissorTest` that
> lives until the end of that statement. [10.5](#105-doclear) uses this.

### `renderer/src/Context.cpp`

```cpp
#include <arda/renderer/Context.h>

#include <stdexcept>

namespace arda::renderer {

Context::Context(Device& device) : m_device(device) {}

// ContextGL3x.cs Viewport setter (lines 124-144), moved to the base class so
// every backend gets the same validation and the same redundancy check.
void Context::SetViewport(const Rectangle& viewport) {
    if (viewport.width < 0 || viewport.height < 0) {
        throw std::invalid_argument("SetViewport: width and height must be greater than or equal to zero");
    }

    if (viewport != m_viewport) {
        m_viewport = viewport;
        DoSetViewport(viewport);
    }
}

} // namespace arda::renderer
```

`viewport != m_viewport` uses the `!=` that C++20 derives from
`Rectangle`'s defaulted `==`. The viewport is part of the cached state too:
the base class remembers the last value it passed down, so resize events that
don't change the size cost nothing.

OpenGlobe throws `ArgumentOutOfRangeException`. arda uses
`std::invalid_argument` for all bad arguments, as Step 0 does (exceptions are
covered in [Step 0](00-setup.md)). The check happens *before* the cache is
updated, so a bad value leaves the context unchanged.

A zero width or height is allowed. That's what a minimized window reports
on Windows, and `glViewport(0, 0, 0, 0)` is valid.

---

## 10. `ContextGL3x`

This is the heart of the step: the GL context's state cache, and a
faithful port of `ContextGL3x.cs`. Adding `DoClear` and `DoSetViewport` to
`Context` made them pure virtual, so `ContextGL3x` won't compile (it's
abstract until it overrides them). Write this whole section before
building again.

### 10.1 The header

```cpp
#pragma once

#include <arda/renderer/Color.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/renderstate/RenderState.h>

#include <glad/glad.h>

struct GLFWwindow;

namespace arda::renderer::gl {

class ContextGL3x final : public Context {
public:
    // The window's GL context must be current when this is constructed.
    ContextGL3x(Device& device, GLFWwindow* window, int width, int height);

    void MakeCurrent() override;

protected:
    void DoClear(const ClearState& clearState) override;
    void DoSetViewport(const Rectangle& viewport) override;

private:
    // Sets every piece of GL state in renderState, ignoring the cache.
    static void ForceApplyRenderState(const RenderState& renderState);
    static void ForceApplyRenderStateStencil(GLenum face, const StencilTestFace& test);

    // Each Apply* method changes only the GL state that differs from the cache,
    // then updates the cache. ApplyRenderState is called by DoDraw in Step 3.
    void ApplyRenderState(const RenderState& renderState);
    void ApplyPrimitiveRestart(const PrimitiveRestart& primitiveRestart);
    void ApplyFacetCulling(const FacetCulling& facetCulling);
    void ApplyProgramPointSize(ProgramPointSize programPointSize);
    void ApplyRasterizationMode(RasterizationMode rasterizationMode);
    void ApplyScissorTest(const ScissorTest& scissorTest);
    void ApplyStencilTest(const StencilTest& stencilTest);
    static void ApplyStencil(GLenum face, StencilTestFace& current, const StencilTestFace& test);
    void ApplyDepthTest(const DepthTest& depthTest);
    void ApplyDepthRange(const DepthRange& depthRange);
    void ApplyBlending(const Blending& blending);
    void ApplyColorMask(const ColorMask& colorMask);
    void ApplyDepthMask(bool depthMask);

    GLFWwindow* m_window;

    // A copy of this context's GL state. It must always equal the real GL
    // state (3.3.3): change one only together with the other.
    RenderState m_renderState;
    Color m_clearColor;
    float m_clearDepth = 1.0f;
    int m_clearStencil = 0;
};

} // namespace arda::renderer::gl
```

Compared with Step 0, the header now includes `<glad/glad.h>`, because
`GLenum` appears in the declarations. That's fine in a private header.

> **OpenGL note — include glad before GLFW:** glad replaces the system's GL
> header, and it stops with an error if a GL header was already included.
> GLFW includes the system GL header unless told not to. So in every `.cpp`
> file, glad (directly, or through a header like this one) must come before
> `<GLFW/glfw3.h>`. Including a file's own header first, as all of arda's
> `.cpp` files do, gets this right automatically.

The `Apply*` names and parameters mirror the C# methods one for one, so you
can read the two side by side. In C#, `ApplyStencil` takes the cached
`StencilTestFace` object and modifies it through the reference. Here the
same thing is spelled `StencilTestFace& current`.

<a id="cpp-static-member-functions"></a>

> **C++ note — static member functions:** a `static` member function
> belongs to the class, not to an object. It has no `this` pointer, so it
> can't read or write non-static members like `m_renderState`. It can still
> use the class's private members and other static functions, and it is
> called like a normal function from inside the class
> (`ForceApplyRenderState(m_renderState)`) or as
> `ContextGL3x::ForceApplyRenderState(...)` from outside, if it were public.
> It's the same as C#'s `static`.
>
> OpenGlobe makes `ForceApplyRenderState` and `ApplyStencil` static, and arda
> keeps that because it documents something useful. `ForceApplyRenderState`
> *cannot* touch the cache. It only pushes a given state to GL, and the
> caller is responsible for making that state the cache. `ApplyStencil`
> modifies only the cache face it's handed, not the whole context. The
> compiler enforces both promises.

The cached members use default member initializers again. `m_clearColor`,
`m_clearDepth` and `m_clearStencil` start at GL's documented defaults, and
the constructor then reads the real values from GL anyway.

### 10.2 The constructor and `ForceApplyRenderState`

`ContextGL3x.cpp` is built from the code blocks in sections 10.2 to 10.6.
Type them into the file **in order**. Together they are the complete file.

```cpp
#include "gl/ContextGL3x.h"
#include "gl/TypeConverterGL3x.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace arda::renderer::gl {

namespace {

// ContextGL3x.cs Enable (lines 474-484). glEnable and glDisable take the
// same "capability" enums, so one helper covers every on/off switch.
void Enable(GLenum capability, bool enable) {
    if (enable) {
        glEnable(capability);
    } else {
        glDisable(capability);
    }
}

} // namespace

// ContextGL3x.cs constructor (lines 20-42).
ContextGL3x::ContextGL3x(Device& device, GLFWwindow* window, int width, int height)
    : Context(device), m_window(window) {
    // Start the clear-value cache from what GL actually holds.
    GLfloat clearColor[4] = {};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    m_clearColor = Color{clearColor[0], clearColor[1], clearColor[2], clearColor[3]};
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &m_clearDepth);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &m_clearStencil);

    // GL's defaults differ from RenderState's defaults (the depth test and
    // culling are on), so push the cached state to GL once, unconditionally.
    ForceApplyRenderState(m_renderState);

    // The base class caches the viewport as {0, 0, 0, 0}, but GL starts with
    // the window's size. Make GL match the cache first, so SetViewport's
    // redundancy check can never skip a needed call, then set the real size.
    glViewport(0, 0, 0, 0);
    SetViewport(Rectangle{0, 0, width, height});
}

void ContextGL3x::MakeCurrent() {
    glfwMakeContextCurrent(m_window);
}
```

The constructor follows the C# order: read the clear values, sync the
render state, set the viewport.

- **Reading the clear values** uses `glGet*` once, at startup, where a
  pipeline stall doesn't matter. OpenGlobe does the same (lines 22–30). The
  window's constructor made this context current before creating the
  `ContextGL3x`, so the queries read this context's values. `GLint` is
  `int` in glad, so `&m_clearStencil` can be passed directly.
- **`ForceApplyRenderState(m_renderState)`** makes GL equal the cache, which
  holds a default `RenderState`.
- **The viewport:** OpenGlobe sets `Viewport` straight away. arda first
  calls `glViewport(0, 0, 0, 0)`. Without that, GL would hold the window's
  size while the base class's cache held {0, 0, 0, 0}. If the first
  viewport requested were {0, 0, 0, 0} (a window whose framebuffer has no
  pixels, such as a minimized one), `SetViewport` would see "no change" and
  skip it, leaving GL at the old size. This is the same rule as the render state: make GL equal the
  cache before trusting the cache.
- **Calling `SetViewport` from a constructor.** `SetViewport` calls the
  virtual `DoSetViewport`. Inside a constructor, a virtual call goes to the
  version of the class currently being constructed. Here that is
  `ContextGL3x`, which is what we want. (Calling it from `Context`'s
  constructor would *not* reach `ContextGL3x`, because that part of the
  object doesn't exist yet.)

`Enable` sits in an anonymous namespace (see [Step 0](00-setup.md)), so it
is private to this file. OpenGlobe's version is a `protected static` method.

> **Why:** *`ForceApplyRenderState` runs at startup.* The cache begins as a
> default `RenderState`, with depth test on and culling on. A new GL context
> has both off. If the constructor skipped the sync, the first draw asking
> for the depth test would compare its state with the cache, see "already
> enabled", and skip `glEnable(GL_DEPTH_TEST)`. The depth test would stay off
> for as long as every draw wanted it on, with no error anywhere. Forcing
> every value once makes the cache true, and from then on every `Apply*` call
> keeps it true.
>
> Forcing costs a few dozen GL calls once per context, so there's no reason
> to be clever about it. OpenGlobe could instead have *queried* GL into the
> cache. Forcing is simpler, and it doesn't depend on the query returning
> exactly the value the cache would store.

Next comes `ForceApplyRenderState` (C# lines 46–98). It sets every value
without looking at the cache.

```cpp

// ContextGL3x.cs ForceApplyRenderState (lines 46-85). Sets every value,
// ignoring the cache.
void ContextGL3x::ForceApplyRenderState(const RenderState& renderState) {
    // The restart index is set at draw time in Step 3, so only the enable is forced.
    Enable(GL_PRIMITIVE_RESTART, renderState.primitiveRestart.enabled);

    Enable(GL_CULL_FACE, renderState.facetCulling.enabled);
    glCullFace(ToGL(renderState.facetCulling.face));
    glFrontFace(ToGL(renderState.facetCulling.frontFaceWindingOrder));

    Enable(GL_PROGRAM_POINT_SIZE, renderState.programPointSize == ProgramPointSize::Enabled);
    glPolygonMode(GL_FRONT_AND_BACK, ToGL(renderState.rasterizationMode));

    Enable(GL_SCISSOR_TEST, renderState.scissorTest.enabled);
    const Rectangle& rectangle = renderState.scissorTest.rectangle;
    glScissor(rectangle.left, rectangle.bottom, rectangle.width, rectangle.height);

    Enable(GL_STENCIL_TEST, renderState.stencilTest.enabled);
    ForceApplyRenderStateStencil(GL_FRONT, renderState.stencilTest.frontFace);
    ForceApplyRenderStateStencil(GL_BACK, renderState.stencilTest.backFace);

    Enable(GL_DEPTH_TEST, renderState.depthTest.enabled);
    glDepthFunc(ToGL(renderState.depthTest.function));

    glDepthRange(renderState.depthRange.nearValue, renderState.depthRange.farValue);

    const Blending& blending = renderState.blending;
    Enable(GL_BLEND, blending.enabled);
    glBlendFuncSeparate(
        ToGL(blending.sourceRGBFactor),
        ToGL(blending.destinationRGBFactor),
        ToGL(blending.sourceAlphaFactor),
        ToGL(blending.destinationAlphaFactor));
    glBlendEquationSeparate(ToGL(blending.rgbEquation), ToGL(blending.alphaEquation));
    glBlendColor(blending.color.red, blending.color.green, blending.color.blue, blending.color.alpha);

    glDepthMask(ToGLBoolean(renderState.depthMask));
    const ColorMask& colorMask = renderState.colorMask;
    glColorMask(
        ToGLBoolean(colorMask.red),
        ToGLBoolean(colorMask.green),
        ToGLBoolean(colorMask.blue),
        ToGLBoolean(colorMask.alpha));
}

// ContextGL3x.cs ForceApplyRenderStateStencil (lines 87-98).
void ContextGL3x::ForceApplyRenderStateStencil(GLenum face, const StencilTestFace& test) {
    glStencilOpSeparate(face,
        ToGL(test.stencilFailOperation),
        ToGL(test.depthFailStencilPassOperation),
        ToGL(test.depthPassStencilPassOperation));

    glStencilFuncSeparate(face,
        ToGL(test.function),
        test.referenceValue,
        static_cast<GLuint>(test.mask));
}
```

A few things to notice:

- Values are forced **even for disabled stages**: the cull face, scissor
  box, stencil functions and blend factors are all set although those
  stages start disabled. The cache stores them, so GL must hold them too.
- The only difference from the C# is the primitive restart index. C# sets
  it to `Index` (0), which is also GL's default. arda has no index field,
  and Step 3 sets the index before each indexed draw. Step 3's
  `m_primitiveRestartDatatype` starts empty, so its first indexed draw always
  sets it.
- `const Rectangle& rectangle = ...` and `const Blending& blending = ...`
  are local references, only a shorter name for a member. No copy is made.

### 10.3 `ApplyRenderState`

`ApplyRenderState` (C# lines 534–547) calls each `Apply*` method in the
order of the `RenderState` fields. Nothing calls it in this step. Step 3's
`ApplyBeforeDraw` calls it before every draw.

```cpp

// ContextGL3x.cs ApplyRenderState (lines 534-547). Called before every draw
// (Step 3). Only the differences from the cache reach GL.
void ContextGL3x::ApplyRenderState(const RenderState& renderState) {
    ApplyPrimitiveRestart(renderState.primitiveRestart);
    ApplyFacetCulling(renderState.facetCulling);
    ApplyProgramPointSize(renderState.programPointSize);
    ApplyRasterizationMode(renderState.rasterizationMode);
    ApplyScissorTest(renderState.scissorTest);
    ApplyStencilTest(renderState.stencilTest);
    ApplyDepthTest(renderState.depthTest);
    ApplyDepthRange(renderState.depthRange);
    ApplyBlending(renderState.blending);
    ApplyColorMask(renderState.colorMask);
    ApplyDepthMask(renderState.depthMask);
}
```

### 10.4 The `Apply*` methods

Every `Apply*` method has the same shape, taken from `ApplyDepthTest` in
Section 3.3.3:

1. If the *enabled* flag differs from the cache, call `glEnable` or
   `glDisable`, and update the cached flag.
2. **Only if the stage is enabled,** compare each of its other values with
   the cache, call GL for the ones that differ, and update those cached
   values.

Step 2 is an optimization. The settings of a disabled stage have no effect,
so there's no point sending them. The cache stays correct: it still holds
the old values, which are exactly what GL still holds. When a later draw
enables the stage, step 2 runs and brings any stale values up to date.

> **Pitfall:** in every branch, the GL call and the cache update go
> together. Updating the cache without calling GL, or calling GL in a
> branch that doesn't update the cache, breaks the invariant, and the bug
> only shows up several draws later.

Primitive restart (C# lines 226–242). Only the enable flag is here; the
index belongs to Step 3.

```cpp

// ContextGL3x.cs ApplyPrimitiveRestart (lines 226-242), without the index:
// Step 3's ApplyPrimitiveRestartIndex sets it from the index buffer's type.
void ContextGL3x::ApplyPrimitiveRestart(const PrimitiveRestart& primitiveRestart) {
    if (m_renderState.primitiveRestart.enabled != primitiveRestart.enabled) {
        Enable(GL_PRIMITIVE_RESTART, primitiveRestart.enabled);
        m_renderState.primitiveRestart.enabled = primitiveRestart.enabled;
    }
}
```

Facet culling (C# lines 244–266). The cull face and the winding order are
separate GL calls, so they're compared separately.

```cpp

// ContextGL3x.cs ApplyFacetCulling (lines 244-266).
void ContextGL3x::ApplyFacetCulling(const FacetCulling& facetCulling) {
    if (m_renderState.facetCulling.enabled != facetCulling.enabled) {
        Enable(GL_CULL_FACE, facetCulling.enabled);
        m_renderState.facetCulling.enabled = facetCulling.enabled;
    }

    if (facetCulling.enabled) {
        if (m_renderState.facetCulling.face != facetCulling.face) {
            glCullFace(ToGL(facetCulling.face));
            m_renderState.facetCulling.face = facetCulling.face;
        }

        if (m_renderState.facetCulling.frontFaceWindingOrder != facetCulling.frontFaceWindingOrder) {
            glFrontFace(ToGL(facetCulling.frontFaceWindingOrder));
            m_renderState.facetCulling.frontFaceWindingOrder = facetCulling.frontFaceWindingOrder;
        }
    }
}
```

Program point size and rasterization mode (C# lines 268–284) are single
values with no "enabled" gate.

```cpp

// ContextGL3x.cs ApplyProgramPointSize (lines 268-275).
void ContextGL3x::ApplyProgramPointSize(ProgramPointSize programPointSize) {
    if (m_renderState.programPointSize != programPointSize) {
        Enable(GL_PROGRAM_POINT_SIZE, programPointSize == ProgramPointSize::Enabled);
        m_renderState.programPointSize = programPointSize;
    }
}

// ContextGL3x.cs ApplyRasterizationMode (lines 277-284). A core profile only
// accepts GL_FRONT_AND_BACK here.
void ContextGL3x::ApplyRasterizationMode(RasterizationMode rasterizationMode) {
    if (m_renderState.rasterizationMode != rasterizationMode) {
        glPolygonMode(GL_FRONT_AND_BACK, ToGL(rasterizationMode));
        m_renderState.rasterizationMode = rasterizationMode;
    }
}
```

The scissor test (C# lines 286–318) validates its rectangle first. GL would
report a negative size as `GL_INVALID_VALUE` and ignore the call, so the
cache would disagree with GL. Throwing *before* touching anything keeps
both unchanged. The rectangle is compared as a whole, using `Rectangle`'s
`operator!=`.

```cpp

// ContextGL3x.cs ApplyScissorTest (lines 286-318).
void ContextGL3x::ApplyScissorTest(const ScissorTest& scissorTest) {
    const Rectangle& rectangle = scissorTest.rectangle;

    if (rectangle.width < 0 || rectangle.height < 0) {
        throw std::invalid_argument(
            "renderState.scissorTest.rectangle width and height must be greater than or equal to zero");
    }

    if (m_renderState.scissorTest.enabled != scissorTest.enabled) {
        Enable(GL_SCISSOR_TEST, scissorTest.enabled);
        m_renderState.scissorTest.enabled = scissorTest.enabled;
    }

    if (scissorTest.enabled) {
        if (m_renderState.scissorTest.rectangle != rectangle) {
            glScissor(rectangle.left, rectangle.bottom, rectangle.width, rectangle.height);
            m_renderState.scissorTest.rectangle = rectangle;
        }
    }
}
```

The stencil test (C# lines 320–364) applies each face separately.
`ApplyStencil` compares the three operations as a group, because
`glStencilOpSeparate` sets all three at once, and the function, reference
and mask as another group for `glStencilFuncSeparate`. `current` is a
reference to the cached face inside `m_renderState`, so assigning to it
updates the cache.

```cpp

// ContextGL3x.cs ApplyStencilTest (lines 320-333).
void ContextGL3x::ApplyStencilTest(const StencilTest& stencilTest) {
    if (m_renderState.stencilTest.enabled != stencilTest.enabled) {
        Enable(GL_STENCIL_TEST, stencilTest.enabled);
        m_renderState.stencilTest.enabled = stencilTest.enabled;
    }

    if (stencilTest.enabled) {
        ApplyStencil(GL_FRONT, m_renderState.stencilTest.frontFace, stencilTest.frontFace);
        ApplyStencil(GL_BACK, m_renderState.stencilTest.backFace, stencilTest.backFace);
    }
}

// ContextGL3x.cs ApplyStencil (lines 335-364). current is the cached face
// and is updated in place.
void ContextGL3x::ApplyStencil(GLenum face, StencilTestFace& current, const StencilTestFace& test) {
    if (current.stencilFailOperation != test.stencilFailOperation ||
        current.depthFailStencilPassOperation != test.depthFailStencilPassOperation ||
        current.depthPassStencilPassOperation != test.depthPassStencilPassOperation) {
        glStencilOpSeparate(face,
            ToGL(test.stencilFailOperation),
            ToGL(test.depthFailStencilPassOperation),
            ToGL(test.depthPassStencilPassOperation));

        current.stencilFailOperation = test.stencilFailOperation;
        current.depthFailStencilPassOperation = test.depthFailStencilPassOperation;
        current.depthPassStencilPassOperation = test.depthPassStencilPassOperation;
    }

    if (current.function != test.function ||
        current.referenceValue != test.referenceValue ||
        current.mask != test.mask) {
        glStencilFuncSeparate(face,
            ToGL(test.function),
            test.referenceValue,
            static_cast<GLuint>(test.mask));

        current.function = test.function;
        current.referenceValue = test.referenceValue;
        current.mask = test.mask;
    }
}
```

The depth test (C# lines 366–382) is the example from Section 3.3.3.

```cpp

// ContextGL3x.cs ApplyDepthTest (lines 366-382), the example in Section 3.3.3.
void ContextGL3x::ApplyDepthTest(const DepthTest& depthTest) {
    if (m_renderState.depthTest.enabled != depthTest.enabled) {
        Enable(GL_DEPTH_TEST, depthTest.enabled);
        m_renderState.depthTest.enabled = depthTest.enabled;
    }

    if (depthTest.enabled) {
        if (m_renderState.depthTest.function != depthTest.function) {
            glDepthFunc(ToGL(depthTest.function));
            m_renderState.depthTest.function = depthTest.function;
        }
    }
}
```

The depth range (C# lines 384–408) has no enable flag. It validates both
values, then compares them as a pair because one call sets both. The check
is written `!(value >= 0.0 && value <= 1.0)` rather than
`value < 0.0 || value > 1.0`. Every comparison with NaN is false, so the
second form would let a NaN through, and the first rejects it.

```cpp

// ContextGL3x.cs ApplyDepthRange (lines 384-408).
void ContextGL3x::ApplyDepthRange(const DepthRange& depthRange) {
    // Written so that NaN fails the check too.
    if (!(depthRange.nearValue >= 0.0 && depthRange.nearValue <= 1.0)) {
        throw std::invalid_argument("renderState.depthRange.nearValue must be between zero and one");
    }
    if (!(depthRange.farValue >= 0.0 && depthRange.farValue <= 1.0)) {
        throw std::invalid_argument("renderState.depthRange.farValue must be between zero and one");
    }

    if (m_renderState.depthRange != depthRange) {
        glDepthRange(depthRange.nearValue, depthRange.farValue);
        m_renderState.depthRange = depthRange;
    }
}
```

Blending (C# lines 410–454) groups its values the same way the GL calls
do: four factors, two equations, one color.

```cpp

// ContextGL3x.cs ApplyBlending (lines 410-454).
void ContextGL3x::ApplyBlending(const Blending& blending) {
    Blending& current = m_renderState.blending;

    if (current.enabled != blending.enabled) {
        Enable(GL_BLEND, blending.enabled);
        current.enabled = blending.enabled;
    }

    if (blending.enabled) {
        if (current.sourceRGBFactor != blending.sourceRGBFactor ||
            current.destinationRGBFactor != blending.destinationRGBFactor ||
            current.sourceAlphaFactor != blending.sourceAlphaFactor ||
            current.destinationAlphaFactor != blending.destinationAlphaFactor) {
            glBlendFuncSeparate(
                ToGL(blending.sourceRGBFactor),
                ToGL(blending.destinationRGBFactor),
                ToGL(blending.sourceAlphaFactor),
                ToGL(blending.destinationAlphaFactor));

            current.sourceRGBFactor = blending.sourceRGBFactor;
            current.destinationRGBFactor = blending.destinationRGBFactor;
            current.sourceAlphaFactor = blending.sourceAlphaFactor;
            current.destinationAlphaFactor = blending.destinationAlphaFactor;
        }

        if (current.rgbEquation != blending.rgbEquation ||
            current.alphaEquation != blending.alphaEquation) {
            glBlendEquationSeparate(ToGL(blending.rgbEquation), ToGL(blending.alphaEquation));

            current.rgbEquation = blending.rgbEquation;
            current.alphaEquation = blending.alphaEquation;
        }

        if (current.color != blending.color) {
            const Color& color = blending.color;
            glBlendColor(color.red, color.green, color.blue, color.alpha);
            current.color = color;
        }
    }
}
```

`Blending& current = m_renderState.blending;` is a non-const reference
used as a short name for the cached struct. Assigning through it writes the
cache. The C# spells out `_renderState.Blending.` on every line.

The two masks (C# lines 456–472) are compared as whole values.

```cpp

// ContextGL3x.cs ApplyColorMask (lines 456-463).
void ContextGL3x::ApplyColorMask(const ColorMask& colorMask) {
    if (m_renderState.colorMask != colorMask) {
        glColorMask(
            ToGLBoolean(colorMask.red),
            ToGLBoolean(colorMask.green),
            ToGLBoolean(colorMask.blue),
            ToGLBoolean(colorMask.alpha));
        m_renderState.colorMask = colorMask;
    }
}

// ContextGL3x.cs ApplyDepthMask (lines 465-472).
void ContextGL3x::ApplyDepthMask(bool depthMask) {
    if (m_renderState.depthMask != depthMask) {
        glDepthMask(ToGLBoolean(depthMask));
        m_renderState.depthMask = depthMask;
    }
}
```

That's all eleven `Apply*` methods, one for each `RenderState` field.

### 10.5 `DoClear`

`DoClear` ports `Clear` (C# lines 152–180), with one change: it resets the
scissor test and write masks to their defaults instead of taking them from
the `ClearState`.

```cpp

// ContextGL3x.cs Clear (lines 152-180).
void ContextGL3x::DoClear(const ClearState& clearState) {
    // Step 6 adds: ApplyFramebuffer();

    // glClear respects the scissor test and the write masks. Direct3D 11's
    // clears don't, so reset them here (through the cache) to match it.
    ApplyScissorTest(ScissorTest{});   // disabled
    ApplyColorMask(ColorMask{});       // every channel written
    ApplyDepthMask(true);
    // The stencil write mask is never changed from GL's default (all ones).

    if (m_clearColor != clearState.color) {
        const Color& color = clearState.color;
        glClearColor(color.red, color.green, color.blue, color.alpha);
        m_clearColor = color;
    }

    if (m_clearDepth != clearState.depth) {
        glClearDepth(clearState.depth);
        m_clearDepth = clearState.depth;
    }

    if (m_clearStencil != clearState.stencil) {
        glClearStencil(clearState.stencil);
        m_clearStencil = clearState.stencil;
    }

    glClear(ToGL(clearState.buffers));
}
```

- The resets go **through the `Apply*` methods**, so they update the cache.
  They cost nothing when the state is already the default, which is the
  usual case. The next draw's `ApplyRenderState` turns the scissor test or
  masks back on if it wants them.
- `ScissorTest{}` and `ColorMask{}` are temporaries holding the default
  values. They bind to the `const&` parameters and are destroyed at the end
  of the statement (see the
  [pass-by-reference note](#cpp-const-ref-vs-value)).
- The clear values are cached like everything else: `glClearColor` is only
  called when the color changes. The comparison uses `Color`'s defaulted
  `!=`.
- `glClearDepth` takes a `GLdouble`. The `float` converts implicitly
  without loss.

> **Why:** *resetting instead of respecting.* With GL alone, respecting the
> scissor and masks would be more flexible. arda doesn't, because of
> portability (README, "Clears" row). If `DoClear` left them alone, a clear
> right after a scissored draw would clear only the scissor box on GL but
> the whole target on D3D11, and the same program would look different on
> each backend. Resetting makes GL behave like D3D11, and it also removes the
> classic "depth mask left off, so depth never clears" bug from
> [4.8](#48-color-mask-and-depth-mask).

### 10.6 `DoSetViewport`

```cpp

// ContextGL3x.cs Viewport setter (lines 131-143). Context::SetViewport has
// already validated the rectangle and skipped unchanged values.
void ContextGL3x::DoSetViewport(const Rectangle& viewport) {
    glViewport(viewport.left, viewport.bottom, viewport.width, viewport.height);
}

} // namespace arda::renderer::gl
```

That closes the namespace, and `ContextGL3x.cpp` is complete.

### 10.7 Two rules for using the context

These aren't enforced by the code, so keep them in mind.

- **Make the right context current.** GL calls go to whichever context is
  current on the calling thread. `ContextGL3x` assumes it's current when you
  call `Clear` or `SetViewport`, like OpenGlobe does. With one window that
  is always true. With two windows, call `context.MakeCurrent()` before using
  a context. Otherwise the GL calls change the *other* context's state,
  while *this* context's cache records them, and both are now wrong.
- **Never change GL state behind the context's back.** Everything in
  `RenderState`, the clear values and the viewport must be changed only
  through `Context`. If you add code that calls GL directly, such as a
  debugging overlay, it must restore every value it touched before the next
  `Clear` or `Draw`.

### Checkpoint C

Build `arda_renderer`. It should compile without warnings. If the compiler
says `ContextGL3x` is abstract, a signature in the header doesn't exactly
match the base class. The `override` keyword makes the compiler point at
the mismatched one.

---

## 11. `scene/src/main.cpp`

The scene now clears the window every frame, animates the clear color so
you can see the frames, and keeps the viewport equal to the window size.

```cpp
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/GraphicsWindow.h>

#include <cmath>
#include <cstdio>
#include <exception>

int main() {
    using namespace arda::renderer;
    try {
        // Declared first so it is destroyed last.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(1280, 720, "Arda");
        Context& context = window->GetContext();

        ClearState clearState;
        clearState.color = {0.02f, 0.05f, 0.12f, 1.0f};

        int frame = 0;

        // OpenGlobe's OnResize: keep the viewport equal to the framebuffer size.
        window->SetResizeHandler([&] {
            context.SetViewport({0, 0, window->Width(), window->Height()});
            const Rectangle& viewport = context.GetViewport();
            std::printf("viewport: %d x %d\n", viewport.width, viewport.height);
        });

        // OpenGlobe's OnUpdateFrame: change the scene, but don't draw.
        window->SetUpdateFrameHandler([&] {
            ++frame;
            const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(frame) * 0.02f);
            clearState.color.blue = 0.12f + 0.3f * pulse;
        });

        // OpenGlobe's OnRenderFrame: draw.
        window->SetRenderFrameHandler([&] {
            context.Clear(clearState);
        });

        window->Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

- `context.SetViewport({0, 0, window->Width(), window->Height()})`: the
  braces build a `Rectangle` in place, because the parameter type is known.
- The scene only includes public headers. It can't call GLFW, which is a
  private dependency of the renderer, so it counts frames instead of reading
  a clock.

<a id="cpp-lambda-capture"></a>

> **C++ note — lambda captures and lifetimes:** a lambda's `[...]` says
> which local variables it can use. `[&]` captures every local it mentions
> **by reference**: the lambda stores a reference to `clearState`, not a
> copy. That's why the update handler's changes are seen by the render
> handler. They're both looking at the same variable. (The basics of
> lambdas and `std::function` are in [Step 0](00-setup.md).)
>
> A reference capture doesn't keep the variable alive. If the lambda runs
> after the variable is destroyed, it reads freed stack memory, which is
> undefined behaviour. It usually crashes or reads garbage, and sometimes
> appears to work. In C# this can't happen, because a captured variable
> lives as long as the closure does.
>
> Here it's safe: the handlers are stored in `window`, they only run inside
> `window->Run()`, and `Run()` returns before `clearState`, `frame`,
> `context` and `window` go out of scope. It stops being safe when the
> lambda outlives the function that created it:
>
> ```cpp
> void InstallHandlers(GraphicsWindow& window, Context& context) {
>     ClearState clearState;                    // lives until InstallHandlers returns
>     window.SetRenderFrameHandler([&] {
>         context.Clear(clearState);            // dangling once InstallHandlers returns
>     });
> }
> ```
>
> The other choices each have a cost. `[=]` (or `[clearState]`) captures a
> *copy* made when the lambda is created. It's safe, but changes made later
> to the original are invisible, and the copy can't be modified unless the
> lambda is declared `mutable`. Naming each capture (`[&clearState, &context]`)
> documents exactly what the lambda depends on, which helps as handlers grow.

### Checkpoint D

Run the scene (`./run.sh`). The window should pulse slowly between dark and
lighter blue, and resizing it should print the new size. Two experiments
show the cache at work:

- Set a breakpoint on the `glClearColor` line in `DoClear`. It's hit every
  frame, because the color changes every frame. Now comment out the update
  handler's color change: the breakpoint is hit once and never again.
- Resize the window and watch the console. `DoSetViewport` runs only when
  the size really changes.

The whole window stays filled when you resize, but that proves less than it
seems: `glClear` ignores the viewport, so it would fill the window even with
a stale viewport. The viewport test in the next section checks it properly.

---

## 12. Tests

Two test files: one for the plain data types, which needs no GPU, and one
that creates a hidden window and checks what actually reached GL.

### `tests/src/renderer/RenderStateTests.cpp`

```cpp
#include <doctest/doctest.h>

#include <arda/renderer/ClearState.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/renderstate/RenderState.h>

using namespace arda::renderer;
using arda::core::geometry::WindingOrder;

// ClearBuffers operators are constexpr, so they can be checked at compile time.
static_assert(HasFlag(ClearBuffers::All, ClearBuffers::StencilBuffer));
static_assert((ClearBuffers::ColorBuffer | ClearBuffers::DepthBuffer) == ClearBuffers::ColorAndDepthBuffer);

TEST_CASE("RenderState defaults match OpenGlobe") {
    const RenderState renderState;

    CHECK_FALSE(renderState.primitiveRestart.enabled);

    CHECK(renderState.facetCulling.enabled);
    CHECK(renderState.facetCulling.face == CullFace::Back);
    CHECK(renderState.facetCulling.frontFaceWindingOrder == WindingOrder::Counterclockwise);

    CHECK(renderState.programPointSize == ProgramPointSize::Disabled);
    CHECK(renderState.rasterizationMode == RasterizationMode::Fill);

    CHECK_FALSE(renderState.scissorTest.enabled);
    CHECK(renderState.scissorTest.rectangle == Rectangle{});

    CHECK_FALSE(renderState.stencilTest.enabled);

    CHECK(renderState.depthTest.enabled);
    CHECK(renderState.depthTest.function == DepthTestFunction::Less);

    CHECK(renderState.depthRange.nearValue == 0.0);
    CHECK(renderState.depthRange.farValue == 1.0);

    CHECK_FALSE(renderState.blending.enabled);

    CHECK(renderState.colorMask == ColorMask{true, true, true, true});
    CHECK(renderState.depthMask);
}

TEST_CASE("StencilTestFace defaults match OpenGlobe") {
    const StencilTestFace face;
    CHECK(face.stencilFailOperation == StencilOperation::Keep);
    CHECK(face.depthFailStencilPassOperation == StencilOperation::Keep);
    CHECK(face.depthPassStencilPassOperation == StencilOperation::Keep);
    CHECK(face.function == StencilTestFunction::Always);
    CHECK(face.referenceValue == 0);
    CHECK(face.mask == ~0);

    const StencilTest stencilTest;
    CHECK(stencilTest.frontFace == face);
    CHECK(stencilTest.backFace == face);
}

TEST_CASE("Blending defaults write the source color unchanged") {
    const Blending blending;
    CHECK(blending.sourceRGBFactor == SourceBlendingFactor::One);
    CHECK(blending.sourceAlphaFactor == SourceBlendingFactor::One);
    CHECK(blending.destinationRGBFactor == DestinationBlendingFactor::Zero);
    CHECK(blending.destinationAlphaFactor == DestinationBlendingFactor::Zero);
    CHECK(blending.rgbEquation == BlendEquation::Add);
    CHECK(blending.alphaEquation == BlendEquation::Add);
    CHECK(blending.color == Color{0.0f, 0.0f, 0.0f, 0.0f});
}

TEST_CASE("Defaulted operator== compares every nested field") {
    const RenderState a;
    RenderState b;
    CHECK(a == b);

    b.stencilTest.backFace.mask = 0xFF;   // three levels deep
    CHECK(a != b);

    b = a;                                // copies every field
    CHECK(a == b);

    b.blending.color.alpha = 0.5f;
    CHECK_FALSE(a == b);
}

TEST_CASE("Copies are independent values") {
    DrawState drawState;
    RenderState renderState = drawState.renderState;   // a copy, not a reference
    renderState.depthTest.enabled = false;
    CHECK(drawState.renderState.depthTest.enabled);
}

TEST_CASE("Designated initializers leave unnamed fields at their defaults") {
    const RenderState renderState{
        .facetCulling = {.enabled = false},
        .depthTest = {.function = DepthTestFunction::LessThanOrEqual},
    };
    CHECK_FALSE(renderState.facetCulling.enabled);
    CHECK(renderState.facetCulling.face == CullFace::Back);   // not named, so default
    CHECK(renderState.depthTest.enabled);                     // not named, so default
    CHECK(renderState.depthTest.function == DepthTestFunction::LessThanOrEqual);
}

TEST_CASE("ClearBuffers flags combine") {
    const ClearBuffers depthAndStencil = ClearBuffers::DepthBuffer | ClearBuffers::StencilBuffer;
    CHECK(HasFlag(depthAndStencil, ClearBuffers::DepthBuffer));
    CHECK(HasFlag(depthAndStencil, ClearBuffers::StencilBuffer));
    CHECK_FALSE(HasFlag(depthAndStencil, ClearBuffers::ColorBuffer));

    CHECK(HasFlag(ClearBuffers::ColorAndDepthBuffer, ClearBuffers::ColorBuffer));
    CHECK_FALSE(HasFlag(ClearBuffers::ColorAndDepthBuffer, ClearBuffers::StencilBuffer));
}

TEST_CASE("ClearState defaults match OpenGlobe") {
    const ClearState clearState;
    CHECK(clearState.buffers == ClearBuffers::All);
    CHECK(clearState.color == Color{1.0f, 1.0f, 1.0f, 1.0f});
    CHECK(clearState.depth == 1.0f);
    CHECK(clearState.stencil == 0);
}

TEST_CASE("Rectangle edges") {
    const Rectangle rectangle{.left = 10, .bottom = 20, .width = 30, .height = 40};
    CHECK(rectangle.Right() == 40);
    CHECK(rectangle.Top() == 60);
    CHECK(rectangle == Rectangle{10, 20, 30, 40});
}
```

`static_assert` checks a condition at compile time. If it fails, the file
doesn't compile. It works here only because `operator|` and `HasFlag` are
`constexpr`.

### `tests/src/renderer/ContextGL3xTests.cpp`

These tests need a GPU and a desktop session, like Step 0's device tests.
They create a hidden window, whose constructor makes its context current,
and then use GL queries to check that the context left GL in the state it
claims. The queries are fine in a test, where speed doesn't matter.

The tests call GL directly, so the test executable links glad (see
[the tests CMake file](#testscmakeliststxt)). glad's function pointers are
global variables in one static library, and the device already loaded them,
so the test can call `glGetIntegerv` without loading anything itself.

```cpp
// glad first: it must come before anything that might include a GL header.
#include <glad/glad.h>

#include <doctest/doctest.h>

#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/GraphicsWindow.h>

#include <array>
#include <cstdint>
#include <stdexcept>

using namespace arda::renderer;

namespace {

bool IsEnabled(GLenum capability) {
    return glIsEnabled(capability) == GL_TRUE;
}

GLint GetInteger(GLenum name) {
    GLint value = 0;
    glGetIntegerv(name, &value);
    return value;
}

// Reads one pixel from the window's back buffer, where Clear wrote.
std::array<std::uint8_t, 4> ReadPixel(int x, int y) {
    std::array<std::uint8_t, 4> pixel{};
    glReadBuffer(GL_BACK);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    return pixel;
}

} // namespace

TEST_CASE("A new context applies RenderState's defaults to GL") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);
    window->GetContext().MakeCurrent();

    // These two differ from GL's defaults, so they prove ForceApplyRenderState ran.
    CHECK(IsEnabled(GL_DEPTH_TEST));
    CHECK(IsEnabled(GL_CULL_FACE));

    CHECK(GetInteger(GL_DEPTH_FUNC) == GL_LESS);
    CHECK(GetInteger(GL_CULL_FACE_MODE) == GL_BACK);
    CHECK(GetInteger(GL_FRONT_FACE) == GL_CCW);
    CHECK_FALSE(IsEnabled(GL_PRIMITIVE_RESTART));
    CHECK_FALSE(IsEnabled(GL_PROGRAM_POINT_SIZE));
    CHECK_FALSE(IsEnabled(GL_SCISSOR_TEST));
    CHECK_FALSE(IsEnabled(GL_STENCIL_TEST));
    CHECK_FALSE(IsEnabled(GL_BLEND));
    CHECK(GetInteger(GL_BLEND_SRC_RGB) == GL_ONE);
    CHECK(GetInteger(GL_BLEND_DST_RGB) == GL_ZERO);
    CHECK(GetInteger(GL_BLEND_EQUATION_RGB) == GL_FUNC_ADD);

    GLboolean depthMask = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    CHECK(depthMask == GL_TRUE);

    std::array<GLboolean, 4> colorMask{};
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask.data());
    CHECK(colorMask == std::array<GLboolean, 4>{GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE});

    std::array<GLfloat, 2> depthRange{};
    glGetFloatv(GL_DEPTH_RANGE, depthRange.data());
    CHECK(depthRange[0] == 0.0f);
    CHECK(depthRange[1] == 1.0f);

    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("The viewport starts at the window size and round-trips through GL") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);
    Context& context = window->GetContext();
    context.MakeCurrent();

    const Rectangle initial{0, 0, window->Width(), window->Height()};
    CHECK(context.GetViewport() == initial);

    const Rectangle viewport{.left = 10, .bottom = 20, .width = 30, .height = 40};
    context.SetViewport(viewport);
    CHECK(context.GetViewport() == viewport);

    std::array<GLint, 4> glViewportValue{};
    glGetIntegerv(GL_VIEWPORT, glViewportValue.data());
    CHECK(glViewportValue == std::array<GLint, 4>{10, 20, 30, 40});

    // A bad viewport throws and leaves the cached (and GL) viewport unchanged.
    CHECK_THROWS_AS(context.SetViewport({0, 0, -1, 10}), std::invalid_argument);
    CHECK(context.GetViewport() == viewport);

    // Zero-sized viewports (a minimized window) are allowed.
    context.SetViewport({0, 0, 0, 0});
    glGetIntegerv(GL_VIEWPORT, glViewportValue.data());
    CHECK(glViewportValue == std::array<GLint, 4>{0, 0, 0, 0});
}

TEST_CASE("Clear sends the clear values to GL and resets the scissor test and masks") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);
    Context& context = window->GetContext();
    context.MakeCurrent();

    ClearState clearState;
    clearState.color = {0.25f, 0.5f, 0.75f, 1.0f};
    clearState.depth = 0.5f;
    clearState.stencil = 3;
    context.Clear(clearState);

    std::array<GLfloat, 4> clearColor{};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor.data());
    CHECK(clearColor == std::array<GLfloat, 4>{0.25f, 0.5f, 0.75f, 1.0f});

    GLfloat clearDepth = 0.0f;
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    CHECK(clearDepth == 0.5f);
    CHECK(GetInteger(GL_STENCIL_CLEAR_VALUE) == 3);

    CHECK_FALSE(IsEnabled(GL_SCISSOR_TEST));
    GLboolean depthMask = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    CHECK(depthMask == GL_TRUE);

    CHECK(glGetError() == GL_NO_ERROR);
}

// Reads back the default framebuffer of a hidden window. Most desktop drivers
// allow this. If this test fails on your machine while the others pass, see
// the note after this file.
TEST_CASE("Clear fills the back buffer, and only the buffers asked for") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);
    Context& context = window->GetContext();
    context.MakeCurrent();

    ClearState red;
    red.color = {1.0f, 0.0f, 0.0f, 1.0f};
    context.Clear(red);
    CHECK(ReadPixel(0, 0) == std::array<std::uint8_t, 4>{255, 0, 0, 255});
    CHECK(ReadPixel(window->Width() - 1, window->Height() - 1) == std::array<std::uint8_t, 4>{255, 0, 0, 255});

    // Clearing only depth and stencil must leave the color alone.
    ClearState green;
    green.buffers = ClearBuffers::DepthBuffer | ClearBuffers::StencilBuffer;
    green.color = {0.0f, 1.0f, 0.0f, 1.0f};
    context.Clear(green);
    CHECK(ReadPixel(0, 0) == std::array<std::uint8_t, 4>{255, 0, 0, 255});

    // Clearing color now uses the green that the previous Clear already sent to GL.
    green.buffers = ClearBuffers::ColorBuffer;
    context.Clear(green);
    CHECK(ReadPixel(0, 0) == std::array<std::uint8_t, 4>{0, 255, 0, 255});
}
```

Notes on these tests:

- **Each test creates its own device and window,** declared device first,
  so each starts from a fresh context and cleans up after itself.
- **`glGetError() == GL_NO_ERROR`** catches invalid GL calls. GL records
  the first error and ignores the bad call. The most likely one here is
  `glPolygonMode` with `GL_FRONT` from [4.9](#49-renderstate-listing-33).
- **Reading pixels from a hidden window.** GL's *pixel ownership test*
  lets a driver leave pixels undefined when they aren't visible on screen.
  Desktop drivers on Windows usually keep the back buffer of a hidden GLFW
  window, so the last test normally passes there. If it fails for you while the rest pass,
  that's the reason. Step 6 checks the same thing more robustly by clearing a
  texture through a framebuffer.
- The last test also shows the cache doing its job. The second `Clear` sent
  green to GL even though it didn't clear color, so the third `Clear` skips
  `glClearColor`, and green still comes out.

---

## CMake

These are the complete files after this step.

### `renderer/CMakeLists.txt`

```cmake
# Headers are listed for IDE project-tree visibility only; CMake compiles
# just the .cpp sources. Keep this list current when adding headers.
add_library(arda_renderer
    src/Context.cpp
    src/Device.cpp
    src/GlfwLibrary.cpp
    src/GraphicsWindow.cpp
    include/arda/renderer/ClearState.h
    include/arda/renderer/Color.h
    include/arda/renderer/Context.h
    include/arda/renderer/Device.h
    include/arda/renderer/DrawState.h
    include/arda/renderer/GraphicsApi.h
    include/arda/renderer/GraphicsWindow.h
    include/arda/renderer/Rectangle.h
    include/arda/renderer/renderstate/Blending.h
    include/arda/renderer/renderstate/ColorMask.h
    include/arda/renderer/renderstate/DepthRange.h
    include/arda/renderer/renderstate/DepthTest.h
    include/arda/renderer/renderstate/FacetCulling.h
    include/arda/renderer/renderstate/PrimitiveRestart.h
    include/arda/renderer/renderstate/RenderState.h
    include/arda/renderer/renderstate/ScissorTest.h
    include/arda/renderer/renderstate/StencilTest.h
    src/GlfwLibrary.h
)

# PUBLIC include: the public headers under include/arda/renderer.
# PRIVATE src: backend sources include their siblings as "gl/DeviceGL3x.h".
target_include_directories(arda_renderer PUBLIC include PRIVATE src)

# PUBLIC: renderer's headers use core's types, so consumers need core too.
# PRIVATE: glfw appears only in .cpp files, so consumers never see it.
target_link_libraries(arda_renderer PUBLIC arda_core PRIVATE glfw)

# Warnings, including a missing enumerator in a switch (-Wswitch is in -Wall;
# MSVC's C4062 is off by default, so /w44062 enables it at level 4).
if(MSVC)
    target_compile_options(arda_renderer PRIVATE /W4 /w44062)
else()
    target_compile_options(arda_renderer PRIVATE -Wall -Wextra)
endif()

option(ARDA_RENDERER_GL    "Build the OpenGL 3.3 backend"  ON)
option(ARDA_RENDERER_D3D11 "Build the Direct3D 11 backend" OFF)

if(ARDA_RENDERER_GL)
    target_sources(arda_renderer PRIVATE
        src/gl/ContextGL3x.cpp
        src/gl/DeviceGL3x.cpp
        src/gl/GraphicsWindowGL3x.cpp
        src/gl/TypeConverterGL3x.cpp
        src/gl/ContextGL3x.h
        src/gl/DeviceGL3x.h
        src/gl/GLHandle.h
        src/gl/GraphicsWindowGL3x.h
        src/gl/TypeConverterGL3x.h
    )
    target_link_libraries(arda_renderer PRIVATE glad::glad)
    target_compile_definitions(arda_renderer PRIVATE ARDA_HAS_GL=1)
endif()

if(ARDA_RENDERER_D3D11)
    if(NOT WIN32)
        message(FATAL_ERROR "ARDA_RENDERER_D3D11 requires Windows")
    endif()
    # Step 8 adds sources here.
endif()
```

### `tests/CMakeLists.txt`

```cmake
add_executable(arda_tests
    src/main.cpp
    src/EllipsoidTests.cpp
    src/renderer/ContextGL3xTests.cpp
    src/renderer/DeviceTests.cpp
    src/renderer/RenderStateTests.cpp
)

# glad: ContextGL3xTests.cpp calls GL directly to check the state the context set.
target_link_libraries(arda_tests PRIVATE arda_core arda_renderer glad::glad doctest::doctest)

add_test(NAME arda_tests COMMAND arda_tests)
```

`core/CMakeLists.txt` is in [part 2](#corecmakeliststxt).

### Checkpoint E: the milestone

Build everything and run `./run.sh -t arda_tests`. Every test should pass,
and the scene should show the pulsing clear color from Checkpoint D.

---

## D3D11 check

Nothing in this step's public headers needs to change for Direct3D 11.
Step 8 covers the implementation in full. In outline:

- **State objects:** D3D11 groups fixed-function state into immutable
  objects, created once and bound per draw. `ContextD3D11::ApplyRenderState`
  looks up (or creates) the object for each group of fields in a cache owned
  by the device, keyed on those fields:

  | D3D11 object | `RenderState` fields |
  |---|---|
  | `ID3D11DepthStencilState` | `depthTest`, `depthMask`, `stencilTest` |
  | `ID3D11BlendState` | `blending`, `colorMask` |
  | `ID3D11RasterizerState` | `facetCulling`, `rasterizationMode`, `scissorTest.enabled` |

  `blending.color`, the stencil reference value, the scissor rectangle and
  `depthRange` aren't part of those objects. They're passed to
  `OMSetBlendState`, `OMSetDepthStencilState`, `RSSetScissorRects` and the
  viewport's `MinDepth`/`MaxDepth`.
- **Per-draw state is a natural fit.** Because `RenderState` is complete
  data compared with `operator==`, the D3D11 backend can use it as a cache
  key. A global-state API would have been much harder to map onto immutable
  state objects.
- **Clear:** `DoClear` calls `ClearRenderTargetView` if the color flag is
  set, and `ClearDepthStencilView` with `D3D11_CLEAR_DEPTH` and/or
  `D3D11_CLEAR_STENCIL`. Neither is affected by scissor or masks, which is
  why `ClearState` leaves them out and the GL backend resets them.
- **Viewport:** `DoSetViewport` converts from the bottom-left origin. For
  the window, `TopLeftY = backBufferHeight - (bottom + height)`. (Render
  targets in Step 6 follow a different rule because of the Y flip; see
  [Step 8](08-direct3d11.md).)
- **Names:** `DepthRange::nearValue`/`farValue` compile in files that include
  `<d3d11.h>` (and so `<windows.h>`), which `near`/`far` wouldn't.

## Checklist

- [ ] `WindingOrder.h` in core, listed in `core/CMakeLists.txt`
- [ ] `Rectangle.h`, `Color.h`
- [ ] The 9 `renderstate/` headers: `DepthTest`, `FacetCulling`,
      `PrimitiveRestart`, `ScissorTest`, `StencilTest`, `DepthRange`,
      `Blending`, `ColorMask`, `RenderState`
- [ ] `ClearState.h` (with `operator|` and `HasFlag`), `DrawState.h`
- [ ] **Checkpoint A:** `RenderStateTests.cpp` passes
- [ ] `TypeConverterGL3x` for all ten conversions, with no `default:` cases
- [ ] Warnings on for `arda_renderer` (`/W4 /w44062` or `-Wall -Wextra`)
- [ ] **Checkpoint B:** the library builds without warnings
- [ ] `Context`: `Clear`, `GetViewport`, `SetViewport`, `DoClear`, `DoSetViewport`, `m_viewport`
- [ ] `ContextGL3x`: constructor (clear-value query, `ForceApplyRenderState`,
      viewport), `ForceApplyRenderStateStencil`, `ApplyRenderState`, all 11
      `Apply*` methods plus `ApplyStencil`, `DoClear`, `DoSetViewport`
- [ ] **Checkpoint C:** the library builds again
- [ ] `scene/src/main.cpp` clears every frame and sets the viewport on resize
- [ ] **Checkpoint D:** the window pulses, and resizing prints the new size
- [ ] `ContextGL3xTests.cpp` and the tests CMake update (link `glad::glad`)
- [ ] **Milestone:** the window clears to your color every frame, the
      viewport follows the window, and `arda_tests` passes

## What later steps rely on

- **Step 3** adds `shaderProgram` and `vertexArray` to `DrawState`, calls
  `ApplyRenderState(drawState.renderState)` from `ApplyBeforeDraw`, sets the
  primitive restart index at draw time, and adds `ToGL` overloads to
  `TypeConverterGL3x`. `core::geometry::WindingOrder` is reused by `Mesh`.
- **Step 6** adds `ApplyFramebuffer()` at the top of `DoClear` and calls
  `SetViewport` when rendering into a texture.
- **Step 8** implements `DoClear` and `DoSetViewport` for D3D11 and turns
  `RenderState` into D3D11 state objects.
