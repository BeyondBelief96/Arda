# Step 0: Setup (3.1–3.2)

**Goal:** replace the concrete `Window` class with the `Device`,
`GraphicsWindow` and `Context` abstractions, backed by OpenGL 3.3. Nothing is
drawn yet. The milestone is a window that opens through `CreateDevice`, runs
a render loop, and closes cleanly.

**Read:** 3.1 "The Need for a Renderer", 3.2 "Bird's-Eye View" (Figures 3.1–3.3, Listings 3.1–3.2).

## OpenGlobe reference

The C# source is in your local clone under `OpenGlobe/Source/Renderer/`.

| File | What to take from it |
|---|---|
| [Renderer/Device.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Device.cs) | `WindowType` enum (lines 21–25), `CreateWindow` (91–104), the limit queries in the static constructor (29–36) |
| [Renderer/GraphicsWindow.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GraphicsWindow.cs) | Events (`Resize`, `UpdateFrame`, `RenderFrame`), the `On*` raisers, `Run`, `Context`, `Width`, `Height` |
| [Renderer/GL3x/GraphicsWindowGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/GraphicsWindowGL3x.cs) | Window and context creation (lines 18–51), the resize and render hooks (53–69), `Dispose` (107–114) |
| [Renderer/Context.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Context.cs) | `MakeCurrent` (line 31). The rest comes in later steps. |
| [Renderer/GL3x/ContextGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ContextGL3x.cs) | Constructor (lines 20–42) and `MakeCurrent` (104–107) |
| [Renderer/GL3x/Names/*.cs](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Renderer/GL3x/Names) | One class per GL object type. `GLHandle.h` replaces all of them. |
| [Renderer/GL3x/Names/FinalizerThreadContextGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Names/FinalizerThreadContextGL3x.cs) | A hidden GL context kept alive for cleanup. Our hidden *share window* plays a similar role. |

## Files

```
renderer/
  CMakeLists.txt             CHECK     already lists every Step 0 file
  include/arda/renderer/
    GraphicsApi.h            NEW       GraphicsApi, WindowType
    Device.h                 NEW       abstract Device, DeviceLimits, CreateDevice, IsGraphicsApiAvailable
    GraphicsWindow.h         REWRITE   abstract GraphicsWindow (replaces the old Window class)
    Context.h                NEW       abstract Context
    Window.h                 DELETE    (already deleted in your working tree)
  src/
    Device.cpp               NEW       Device::CreateGraphicsWindow, CreateDevice, IsGraphicsApiAvailable
    GraphicsWindow.cpp       REWRITE   GraphicsWindow::Run, OnResize (was Window.cpp; git tracks the rename)
    Context.cpp              NEW       Context constructor (grows in later steps)
    GlfwLibrary.h / .cpp     NEW       reference-counted glfwInit/glfwTerminate
    gl/
      GLHandle.h             NEW       RAII owner of a GL object name
      DeviceGL3x.h / .cpp    NEW       hidden share window, glad loading, limits
      GraphicsWindowGL3x.h / .cpp   NEW   a GLFW window with its own GL context
      ContextGL3x.h / .cpp   NEW       MakeCurrent, initial viewport
scene/src/main.cpp           UPDATE
tests/CMakeLists.txt         CHECK     already lists DeviceTests.cpp
tests/src/renderer/DeviceTests.cpp   NEW
```

Your working tree already has most of these files as empty stubs (just
`#pragma once`). You fill them in, in the order this guide gives.

---

## How to read this guide

The guide is the first of nine, so it explains a lot. Three kinds of callout
appear throughout:

> **C++ note — topic:** a C++ language or library feature, explained where it
> is first used. Later guides link back here instead of explaining it again.

> **OpenGL note — topic:** what OpenGL is actually doing when a call is made.

> **Why:** a design decision, especially where arda differs from OpenGlobe's C#.

Each C++ note has a stable link target, such as `00-setup.md#cpp-raii`. The
[index at the end](#c-and-opengl-notes-index) lists them all.

The work is split into numbered parts. Each part says what to type, then
explains it. There are three **build checkpoints** where the library should
compile, and the milestone at the end is running `arda_scene` and
`arda_tests`.

---

## Background: what Chapter 3 is building

Section 3.1 argues for putting a *renderer* layer between the application and
OpenGL. Raw OpenGL is a global state machine: every call changes hidden state
that the next call depends on. That makes code fragile ("who left blending
on?"), hard to port to Direct3D, and hard to multithread. The renderer wraps GL
in a small set of objects with explicit inputs.

Section 3.2 introduces the three objects this step builds:

| Object | What it does | D3D11 equivalent | OpenGlobe C# |
|---|---|---|---|
| `Device` | Creates windows and resources that every window can use (shaders, buffers, textures) | `ID3D11Device` | `static class Device` |
| `GraphicsWindow` | A window on screen. Runs the frame loop and raises resize/update/render events. Owns one `Context`. | a swap chain and its `HWND` | `abstract class GraphicsWindow` |
| `Context` | Issues draw and clear commands into its window, and creates the objects that can't be shared between windows | `ID3D11DeviceContext` | `abstract class Context` |

The usage you are working towards is:

```cpp
auto device = arda::renderer::CreateDevice(arda::renderer::GraphicsApi::OpenGL33);
auto window = device->CreateGraphicsWindow(1280, 720, "Arda");
arda::renderer::Context& context = window->GetContext();
window->SetRenderFrameHandler([&] { /* context.Clear(...), context.Draw(...) */ });
window->Run();
```

Two differences from OpenGlobe shape everything below:

1. **OpenGlobe's `Device` is a `static class`.** It always creates GL objects,
   and its static constructor opens a throwaway 1×1 window just to query GL
   limits. arda's `Device` is an object created by `CreateDevice(api)`. That
   lets you pick OpenGL or Direct3D 11 at runtime, keeps GL initialization in
   a place you control (a static constructor runs "sometime before first
   use"), and lets tests create and destroy devices.
2. **arda is split into public abstract classes and private backends.** The
   public headers in `include/arda/renderer/` describe `Device`,
   `GraphicsWindow` and `Context` without mentioning GL. The GL versions
   (`DeviceGL3x`, `GraphicsWindowGL3x`, `ContextGL3x`) live in `src/gl/`. The
   D3D11 versions come in Step 8. OpenGlobe has the same split
   (`GraphicsWindow` and `GraphicsWindowGL3x`), but in C# the `internal`
   keyword hides the backend. In C++, hiding means keeping the backend headers
   out of `include/`.

> **Why abstract base classes plus backends:** the alternative is `#ifdef` in
> every function, or a compile-time backend choice (one library per API).
> Abstract classes cost one virtual call per operation, which is nothing next
> to the cost of a GL or D3D call, and they let one executable (and one test
> run) use both APIs. That is how Step 8's cross-API test compares GL and
> D3D11 output pixel for pixel.

---

## OpenGL concepts for this step

Read this section before writing code. Every GL and GLFW call in this step
relies on one of these ideas.

### The OpenGL state machine

OpenGL is a *specification*, not a library. Your GPU driver implements it. The
spec describes a big state machine: a set of global variables (the clear
color, the current viewport, whether depth testing is on, which buffer is
bound to which slot...) and functions that either change that state or act
using it.

```cpp
glClearColor(0.0f, 0.0f, 1.0f, 1.0f);   // changes state: the clear color is now blue
glClear(GL_COLOR_BUFFER_BIT);           // acts using state: clears to whatever the clear color is
```

`glClear` has no color parameter. It uses whatever was set last, by any code.
That is the fragility Chapter 3 is designed around. Step 1 hides it behind
`ClearState` and `RenderState` objects that are passed to every call.

### Object names

GL objects (buffers, textures, shaders, programs, vertex arrays,
framebuffers) live inside the driver. You never get a pointer to one. You get
a `GLuint` called a **name**, which is just an ID:

```cpp
GLuint name = 0;
glGenBuffers(1, &name);      // asks the driver for an unused buffer name, e.g. 1
// ... use it ...
glDeleteBuffers(1, &name);   // frees the object; the name can be reused
```

Name `0` is special: it means "no object". Deleting 0 is silently ignored,
and binding 0 unbinds.

**Binding points.** Most GL functions don't take a name. Instead you *bind*
the object to a slot (a *binding point*, or *target*), and later calls act on
whatever is bound there. For example, `glBindBuffer(GL_ARRAY_BUFFER, name)`
followed by `glBufferData(GL_ARRAY_BUFFER, ...)` uploads into that buffer.
Binding points are more global state. They first matter in Step 3, but
`GLHandle.h` in this step is where names are managed.

Because a name is only an integer, forgetting to delete one leaks GPU memory,
and deleting one twice may delete an unrelated object that reused the name.
OpenGlobe wraps each name in a C# class with `Dispose` and a finalizer. arda
uses one C++ template, `GLHandle`, that deletes the name in its destructor
([Part 8](#8-srcglglhandleh)).

### Contexts

All of that state lives in an **OpenGL context**: a driver-side object holding
one complete copy of the GL state machine, plus the objects created in it.
Every GL call acts on the **current context** of the calling thread. There is
no context parameter.

- A thread has at most one current context. You choose it with
  `glfwMakeContextCurrent(window)`.
- A GL call made with no current context does nothing at best and crashes at
  worst. This is the most common GL bug in multi-window code.
- A context can be current on only one thread at a time.

The operating system creates contexts (WGL on Windows, CGL/NSOpenGL on macOS,
GLX or EGL on Linux), and each context is tied to a window's drawing surface.
**GLFW** hides those APIs: `glfwCreateWindow` creates a window *and* a
context for it. That is why arda's `GraphicsWindowGL3x` owns a `ContextGL3x`:
in GL, each window really does come with its own context.

### Shared contexts and the hidden share window

Two contexts normally share nothing. A buffer created in window A's context
is invisible in window B's. That would make `Device` pointless, because the
device is supposed to create resources that every window can use.

GL fixes this with **share groups**. The last argument to `glfwCreateWindow`
is an existing window whose context the new one should share with:

```cpp
GLFWwindow* b = glfwCreateWindow(800, 600, "B", nullptr, /* share = */ a);
```

Contexts in one share group share their **data objects**: buffers, textures,
renderbuffers, samplers, shaders and programs. They do **not** share
**container objects**, which are objects that only hold references to other
objects: vertex array objects (VAOs) and framebuffer objects (FBOs). They also
never share state such as the viewport or the clear color. This is exactly why
the README puts `CreateVertexArray` and `CreateFramebuffer` on `Context`, and
everything else on `Device`.

To share, a context must already exist when the device creates its first
resource, possibly before any window exists. So `DeviceGL3x` creates a
**hidden 1×1 window** whose only job is to own a context. Every real window
passes it as the `share` argument, so all windows land in one share group.
OpenGlobe gets the same effect from OpenTK, which shares every context by
default, and its static `Device` constructor also opens a 1×1 window.

The hidden window also gives the device a context to fall back on: when a
window closes, its context is gone, but the device's context is still there
for deleting shared resources.

### Loading GL functions with glad

On Windows, the system `opengl32.dll` only exports OpenGL **1.1** functions.
Every newer function, such as `glGenBuffers` (GL 1.5) or `glGenVertexArrays`
(GL 3.0), lives in the driver, and you have to ask for its address at runtime:

```cpp
// What glad does for each of hundreds of functions:
glad_glGenBuffers = (PFNGLGENBUFFERSPROC)glfwGetProcAddress("glGenBuffers");
```

**glad** is a generated loader. `<glad/glad.h>` declares a function pointer
for every GL function, plus a macro so that `glGenBuffers(...)` calls through
the pointer. `gladLoadGLLoader(glfwGetProcAddress)` fills every pointer.

Rules that follow from this:

- **A context must be current before you call `gladLoadGLLoader`.** The
  addresses come from the driver behind the current context. With no current
  context, loading fails.
- **Load before the first GL call.** An unloaded pointer is null, and calling
  it crashes with an access violation at address 0.
- **Include `<glad/glad.h>` before `<GLFW/glfw3.h>`.** GLFW includes the
  system `<GL/gl.h>` unless it sees that a GL header was already included.
  glad refuses to compile after `<GL/gl.h>` ("OpenGL header already
  included").
- **glad declares more than you may use.** The vcpkg glad port is generated
  for GL 4.6 *compatibility* (look at the comment at the top of `glad.h`). The
  compiler will happily let you call a GL 4.5 function, but on a 3.3 context
  its pointer may be null. Stick to functions in the 3.3 core spec.
- Load **once per process** in practice. Strictly, WGL function pointers are
  per context, but contexts on the same driver return the same pointers. arda
  loads in the device constructor, while the share context is current.

### Core profile and forward compatibility

GL 3.2 split the API into two **profiles**:

- The **compatibility profile** keeps every deprecated feature back to GL 1.0
  (`glBegin`/`glEnd`, the fixed-function matrix stack, and so on).
- The **core profile** removes them. You must use buffers, vertex arrays and
  shaders. This is the modern API that D3D11 also resembles.

arda asks for a **3.3 core** context, as OpenGlobe does.

A **forward-compatible** context also removes features that were only
*deprecated* in that version (for example, wide lines via `glLineWidth` > 1).
**macOS requires it**: macOS only provides a core context if the forward
compatible flag is set, and without it you get a GL 2.1 context and
`glfwCreateWindow` fails. OpenGlobe sets `ForwardCompatible` on every
platform. arda does the same, so a Windows build fails the same way a Mac build
would if you use something deprecated.

The C# also asks for a *debug* context (`GraphicsContextFlags.Debug`). A debug
context lets the driver do extra validation and report messages. arda asks for
one in Debug builds only, because debug contexts can be slower.

### Double buffering and vsync

A window context has a **default framebuffer** with (at least) two color
buffers:

- the **front buffer**, which the screen is showing, and
- the **back buffer**, where your draw calls go.

`glfwSwapBuffers(window)` presents the back buffer, so the finished frame
appears all at once. Without double buffering you would see each frame being
drawn.

After a swap, the contents of the new back buffer are **undefined**. That is
why every frame starts with a clear (Step 1), and why the Step 0 window shows
garbage or black: nothing clears it yet.

**Vsync.** `glfwSwapInterval(1)` tells the driver to wait for the monitor's
vertical refresh before swapping, so `glfwSwapBuffers` blocks and the loop
runs at the monitor's refresh rate (for example 60 Hz). With `0`, the loop
runs as fast as it can and the image may tear. The swap interval is state of
the **current context**, so it must be called after `glfwMakeContextCurrent`.
Some drivers ignore it (a driver control panel setting can force vsync on or
off).

### Framebuffer size vs window size

GLFW has two sizes for a window:

- **Window size** (`glfwGetWindowSize`), in *screen coordinates*. This is
  what you pass to `glfwCreateWindow`.
- **Framebuffer size** (`glfwGetFramebufferSize`), in *pixels*. This is what
  GL renders into.

On a normal Windows setup they are equal. On a macOS Retina display, a
1280×720 window has a 2560×1440 framebuffer. If you set the viewport from the
window size there, your image fills only the bottom-left quarter.
`glViewport`, `glScissor`, framebuffer attachments and `glReadPixels` all work
in **pixels**, so arda's `GraphicsWindow::Width()` and `Height()` return the
**framebuffer** size, and the resize event fires on
`glfwSetFramebufferSizeCallback`, not the window size callback.

OpenGlobe's `Width`/`Height` return the window size. That was fine in 2010,
before HiDPI screens were common.

### The render loop

A GLFW program runs this loop:

```
while the window should not close:
    process OS events      (glfwPollEvents: input, resize, close button)
    update the simulation  (move the camera, animate)
    render                 (clear, draw)
    present                (glfwSwapBuffers; blocks for vsync)
```

`glfwPollEvents` is what keeps the window responsive. If you stop calling it,
the OS decides the program has hung. It is also where GLFW calls your
callbacks, such as the framebuffer size callback. `glfwWindowShouldClose`
becomes true when the user clicks the close button.

OpenGlobe's `GraphicsWindow.Run(updateRate)` hands this loop to OpenTK's
`GameWindow.Run`, which raises `UpdateFrame` and `RenderFrame` events. arda's
`GraphicsWindow::Run()` writes the loop out itself and calls handlers in the
same order.

---

## Migrating from the old `Window` class

Before this step, the renderer was a single concrete class:

- `include/arda/renderer/Window.h` declared `Window` (already deleted in your
  working tree).
- `src/Window.cpp` implemented it. Git has already recorded it as renamed to
  `src/GraphicsWindow.cpp`, so that file currently holds the **old** `Window`
  code and includes the deleted `Window.h`. It won't compile until Part 6
  replaces it.
- Your working-tree `include/arda/renderer/GraphicsWindow.h` also still
  contains the old `Window` class declaration. Part 4 replaces it.

Every piece of the old code has a new home:

| Old `Window` code | New home | Why it moved |
|---|---|---|
| `glfwInit()` in the constructor | `GlfwLibrary` constructor (Part 7) | GLFW must be initialized once per process, not once per window |
| `glfwTerminate()` in the destructor | `GlfwLibrary` destructor | The old code terminated GLFW when *any* window closed, which destroys every other window too |
| `glfwWindowHint(...)` calls | `DeviceGL3x::ApplyContextHints` (Part 9) | The share window and every real window need identical hints |
| `glfwCreateWindow(...)` | `DeviceGL3x` constructor (share window) and `GraphicsWindowGL3x` constructor (Part 10) | Real windows now pass the share window |
| `glfwMakeContextCurrent` + `gladLoadGLLoader` | `DeviceGL3x` constructor | glad is loaded once, with the share context current |
| `glfwSwapInterval(1)` | `GraphicsWindowGL3x` constructor | Per context |
| `ShouldClose`, `SwapBuffers`, `PollEvents` | `GraphicsWindowGL3x` overrides | Backend-specific |
| `Clear(r, g, b, a)` | `Context::Clear(const ClearState&)` in Step 1 | Clearing is a context operation, and its state becomes an explicit object |
| `glViewport` from `glfwGetFramebufferSize` every frame | `ContextGL3x` constructor, then the resize handler (Step 1) | Only needed when the size changes |
| The `while` loop in `main.cpp` | `GraphicsWindow::Run` (Part 6) | Same loop for every backend |

Migration steps, in order (each is covered in detail below):

1. Confirm `include/arda/renderer/Window.h` is deleted
   (`git status` shows `D renderer/include/arda/renderer/Window.h`).
2. Replace `include/arda/renderer/GraphicsWindow.h` with the abstract class
   (Part 4).
3. Replace the contents of `src/GraphicsWindow.cpp` with `Run` and
   `OnResize` (Part 6). Keep the file name so git keeps its history.
4. Write the GL backend (Parts 8–11), which is where the old GLFW code goes.
5. Update `scene/src/main.cpp` (Part 13). It currently includes `Window.h`, so
   `arda_scene` won't build until then. Build only the `arda_renderer` target
   at the checkpoints before that.
6. Search for leftovers: `git grep -n "Window.h\|renderer::Window"` should
   find nothing outside the docs.

---

## Build order at a glance

| Part | File(s) | Checkpoint |
|---|---|---|
| 1 | CMake files (check only) | |
| 2 | `GraphicsApi.h` | |
| 3 | `Device.h` | |
| 4 | `GraphicsWindow.h` | |
| 5 | `Context.h` | |
| 6 | `Context.cpp`, `GraphicsWindow.cpp` | |
| 7 | `GlfwLibrary.h` / `.cpp` | **Checkpoint 1:** `arda_renderer` compiles |
| 8 | `gl/GLHandle.h` | |
| 9 | `gl/DeviceGL3x.h` / `.cpp` | |
| 10 | `gl/GraphicsWindowGL3x.h` / `.cpp` | |
| 11 | `gl/ContextGL3x.h` / `.cpp` | **Checkpoint 2:** `arda_renderer` compiles with the GL backend |
| 12 | `Device.cpp` | **Checkpoint 3:** `arda_renderer` compiles completely |
| 13 | `scene/src/main.cpp` | **Milestone, part 1:** the window opens |
| 14 | `tests/src/renderer/DeviceTests.cpp` | **Milestone, part 2:** `arda_tests` passes |

To build only the library (Windows, `vs` preset):

```sh
cmake --preset vs                                           # only if build/vs doesn't exist yet
cmake --build build/vs --config Debug --target arda_renderer
```

`arda_renderer` is a **static library**, so it builds even while some
declared functions have no definition yet. Missing definitions only show up
as linker errors when an executable (`arda_scene`, `arda_tests`) links
against it. The checkpoints use this to compile partway.

---

## 1. Check the CMake files

The CMake files in your working tree already list every Step 0 file, so
there is nothing to type. Read them anyway, because every later step adds to
them.

### Top-level `CMakeLists.txt` (relevant part)

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)     # provides the target: glfw
find_package(glad CONFIG REQUIRED)      # provides the target: glad::glad
find_package(doctest CONFIG REQUIRED)   # provides the target: doctest::doctest

add_subdirectory(core)
add_subdirectory(renderer)
add_subdirectory(scene)
add_subdirectory(tools/viz)

enable_testing()
add_subdirectory(tests)
```

vcpkg installs glfw3, glad and doctest (listed in `vcpkg.json`), and
`find_package` finds them through the vcpkg toolchain file set in
`CMakePresets.json`.

### `renderer/CMakeLists.txt`

This is the file as it is now. Step 0 needs no changes.

```cmake
# Headers are listed for IDE project-tree visibility only; CMake compiles
# just the .cpp sources. Keep this list current when adding headers.
add_library(arda_renderer
    src/Context.cpp
    src/Device.cpp
    src/GlfwLibrary.cpp
    src/GraphicsWindow.cpp
    include/arda/renderer/Context.h
    include/arda/renderer/Device.h
    include/arda/renderer/GraphicsApi.h
    include/arda/renderer/GraphicsWindow.h
    src/GlfwLibrary.h
)

# PUBLIC include: the public headers under include/arda/renderer.
# PRIVATE src: backend sources include their siblings as "gl/DeviceGL3x.h".
target_include_directories(arda_renderer PUBLIC include PRIVATE src)

# PUBLIC: renderer's headers use core's types, so consumers need core too.
# PRIVATE: glfw appears only in .cpp files, so consumers never see it.
target_link_libraries(arda_renderer PUBLIC arda_core PRIVATE glfw)

option(ARDA_RENDERER_GL    "Build the OpenGL 3.3 backend"  ON)
option(ARDA_RENDERER_D3D11 "Build the Direct3D 11 backend" OFF)

if(ARDA_RENDERER_GL)
    target_sources(arda_renderer PRIVATE
        src/gl/ContextGL3x.cpp
        src/gl/DeviceGL3x.cpp
        src/gl/GraphicsWindowGL3x.cpp
        src/gl/ContextGL3x.h
        src/gl/DeviceGL3x.h
        src/gl/GLHandle.h
        src/gl/GraphicsWindowGL3x.h
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

What each piece does:

- **`add_library(arda_renderer ...)`** with no `STATIC`/`SHARED` keyword
  builds a static library (unless `BUILD_SHARED_LIBS` is on). Headers in the
  list are only there so Visual Studio shows them in Solution Explorer.
- **`target_include_directories(... PUBLIC include PRIVATE src)`**: anyone
  who links `arda_renderer` gets `renderer/include` on their include path, so
  `#include <arda/renderer/Device.h>` works in `scene/`. Only the renderer's
  own sources get `renderer/src`, so `#include "gl/DeviceGL3x.h"` works there
  and nowhere else.
- **`PRIVATE glfw` and `PRIVATE glad::glad`**: only the renderer's sources
  get the GLFW and glad include paths. The scene can't include them even by
  accident. (Because `arda_renderer` is static, CMake still adds the glfw and
  glad libraries to the final link of `arda_scene`.) The README's
  ["Why glad, GLFW and D3D headers are private"](../README.md#why-glad-glfw-and-d3d-headers-are-private)
  explains why.
- **`option(...)`** creates a cached ON/OFF setting. Change it with
  `cmake --preset vs -DARDA_RENDERER_GL=OFF`.
- **`target_compile_definitions(... PRIVATE ARDA_HAS_GL=1)`** passes
  `-DARDA_HAS_GL=1` (`/DARDA_HAS_GL=1` on MSVC) when compiling the renderer's
  sources. `Device.cpp` tests it with `#if ARDA_HAS_GL` (Part 12).

> **Why the D3D11 option is here already:** `CreateDevice` and
> `IsGraphicsApiAvailable` already know about `Direct3D11`. Putting the
> option in now means Step 8 only adds sources.

### `tests/CMakeLists.txt`

Also already correct:

```cmake
add_executable(arda_tests
    src/main.cpp
    src/EllipsoidTests.cpp
    src/renderer/DeviceTests.cpp
)
target_link_libraries(arda_tests PRIVATE arda_core arda_renderer doctest::doctest)

add_test(NAME arda_tests COMMAND arda_tests)
```

`tests/src/main.cpp` defines `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, which makes
doctest generate `main()`. Each test file only includes `<doctest/doctest.h>`
and writes `TEST_CASE`s. `tests/src/renderer/DeviceTests.cpp` exists but is
empty until Part 14. Since `arda_tests` links `arda_renderer`, it won't link
until Checkpoint 3.

---

## 2. `include/arda/renderer/GraphicsApi.h`

Two small enums used across the public API. Replace the stub with:

```cpp
#pragma once

namespace arda::renderer {

enum class GraphicsApi {
    OpenGL33,
    Direct3D11,
};

enum class WindowType {
    Default,
    Hidden,   // for tests; OpenGlobe has FullScreen here instead
};

} // namespace arda::renderer
```

`WindowType` comes from `Device.cs` (lines 21–25). OpenGlobe's second value
is `FullScreen`. arda needs `Hidden` more: tests create windows that should
never appear on screen. A fullscreen mode can be added later with
`glfwGetPrimaryMonitor`.

(Your current stub has `# pragma once` with a space, and a stray `;` after
the namespace's closing brace. Both happen to be legal, but write it as
above.)

<a id="cpp-pragma-once"></a>
> **C++ note — `#pragma once` and include guards:** `#include` pastes the
> named file's text in place. If `Device.h` and `Context.h` both include
> `GraphicsApi.h`, and `main.cpp` includes both, the enums would be defined
> twice, which is an error. `#pragma once` tells the compiler to skip the
> file if it was already included in this translation unit (one `.cpp` plus
> everything it includes). The portable, standard alternative is an include
> guard:
> ```cpp
> #ifndef ARDA_RENDERER_GRAPHICSAPI_H
> #define ARDA_RENDERER_GRAPHICSAPI_H
> // ... contents ...
> #endif
> ```
> Every major compiler supports `#pragma once`, and it can't go wrong by
> copy-pasting a guard name, so arda uses it everywhere. C# has nothing like
> this because it has no textual includes: the compiler sees every file of a
> project at once.

<a id="cpp-nested-namespaces"></a>
> **C++ note — nested namespaces:** `namespace arda::renderer { ... }` (C++17)
> is shorthand for `namespace arda { namespace renderer { ... } }`. It works
> like a C# `namespace OpenGlobe.Renderer`, with two differences. A C++
> namespace is opened with braces and can be reopened in any file to add more
> names (every `.cpp` reopens it to define functions). And code inside a nested
> namespace sees names from the enclosing ones without qualification: inside
> `arda::renderer::gl`, the name `Device` finds `arda::renderer::Device`.
> The closing comment `} // namespace arda::renderer` is only a convention.

<a id="cpp-enum-class"></a>
> **C++ note — `enum class`:** a *scoped enumeration*. Its values must be
> qualified (`GraphicsApi::OpenGL33`, like C#), and it doesn't convert to
> `int` implicitly. The older plain `enum` leaks its value names into the
> enclosing namespace and converts to `int` silently:
> ```cpp
> enum Color { Red };           // plain: 'Red' is now a name in this namespace; int x = Red; compiles
> enum class Api { OpenGL33 };  // scoped: must write Api::OpenGL33; int y = Api::OpenGL33; is an error
> int z = static_cast<int>(Api::OpenGL33);   // explicit conversion is fine
> ```
> Use `enum class` for everything. The trailing comma after the last
> enumerator is allowed and keeps diffs small when a value is added.

---

## 3. `include/arda/renderer/Device.h`

The device is the root object: it creates windows now, and shaders, buffers
and textures in later steps. Replace the stub with:

```cpp
#pragma once

#include <arda/renderer/GraphicsApi.h>

#include <memory>
#include <string>

namespace arda::renderer {

class GraphicsWindow;

struct DeviceLimits {
    int maximumNumberOfVertexAttributes = 0;
    int numberOfTextureUnits = 0;
    int maximumNumberOfColorAttachments = 0;
};

// Creates resources that can be used by every window (3.2, Listing 3.1).
class Device {
public:
    virtual ~Device() = default;

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    virtual GraphicsApi Api() const = 0;

    const DeviceLimits& Limits() const { return m_limits; }

    // Not named CreateWindow: <windows.h> defines CreateWindow as a macro.
    std::unique_ptr<GraphicsWindow> CreateGraphicsWindow(
        int width, int height, const std::string& title, WindowType type = WindowType::Default);

protected:
    Device() = default;

    // Backends call this once their API is initialized.
    void SetLimits(const DeviceLimits& limits) { m_limits = limits; }

    virtual std::unique_ptr<GraphicsWindow> DoCreateGraphicsWindow(
        int width, int height, const std::string& title, WindowType type) = 0;

private:
    DeviceLimits m_limits;
};

bool IsGraphicsApiAvailable(GraphicsApi api);

// Throws std::runtime_error if the API was not compiled in.
std::unique_ptr<Device> CreateDevice(GraphicsApi api);

} // namespace arda::renderer
```

How it maps to `Device.cs`:

| C# | C++ |
|---|---|
| `static class Device` | abstract `class Device`, created by `CreateDevice(api)` |
| `static int MaximumNumberOfVertexAttributes { get; }` (and two more) | `Limits().maximumNumberOfVertexAttributes`, filled by the backend with `SetLimits` |
| `CreateWindow(width, height)`, `(..., title)`, `(..., title, windowType)` | one `CreateGraphicsWindow` with a default `type` argument (and `title` required) |
| returns `GraphicsWindow` (garbage collected, `IDisposable`) | returns `std::unique_ptr<GraphicsWindow>` |

C# overloads `CreateWindow` three times because C# only gained default
arguments later. C++ has always had default arguments, so one function is
enough.

> **Why `CreateGraphicsWindow` and not `CreateWindow`:** `<windows.h>`
> contains `#define CreateWindow CreateWindowW` (or `CreateWindowA`). The
> preprocessor replaces every `CreateWindow` token after that line, including
> `device->CreateWindow(...)` and the declaration in this header, so the call
> ends up looking for a function called `CreateWindowW`. Whether it breaks
> depends on include order, which makes it a nasty bug to track down. Step 8
> includes `<windows.h>` (through `d3d11.h`), so the name is avoided from the
> start. The same hazard applies to `min`, `max`, `near`, `far`, `DrawText`,
> `LoadImage` and `GetObject`.

> **Why `Limits()` instead of three getters:** the three values are produced
> together by the backend and read together by later steps
> (`device.Limits().numberOfTextureUnits` in Step 5, and so on), so a struct
> is simpler. Step 8 fills it with C++20 designated initializers
> (`SetLimits({.maximumNumberOfVertexAttributes = ..., ...})`), which works
> because `DeviceLimits` is a plain aggregate. Step 1 explains aggregates.

The C++ features in this header, in the order they appear:

<a id="cpp-forward-declarations"></a>
> **C++ note — forward declarations vs includes:** `class GraphicsWindow;`
> says "a class with this name exists" without defining it. After a forward
> declaration, the type is *incomplete*. You can:
> - declare pointers and references to it (`GraphicsWindow*`, `GraphicsWindow&`),
> - name it in function declarations, including as a return type,
> - use it as the `T` in `std::unique_ptr<T>` or `std::shared_ptr<T>` members and return types.
>
> You can't: create one, store one by value, call its member functions, use
> `sizeof`, or *destroy* one through a `unique_ptr`. Anything that needs its
> size or its members needs the full definition, which means `#include`.
>
> Prefer a forward declaration in headers whenever it's enough. Every
> `#include` in a header is compiled again by every file that includes that
> header, and it drags in that header's includes too. Here, `Device.h` doesn't
> need to know what a `GraphicsWindow` contains, only that one exists.
>
> **Pitfall:** code that *destroys* the `unique_ptr<GraphicsWindow>` returned
> by `CreateGraphicsWindow` needs the full `GraphicsWindow` definition, because
> destroying it calls its destructor. If `main.cpp` includes only `Device.h`,
> you get an error like "can't delete an incomplete type" (MSVC C2027 or
> C4150). The fix is `#include <arda/renderer/GraphicsWindow.h>` in the file
> that uses the window.
>
> C# has no equivalent, because the compiler sees all types in a project and
> referenced assemblies at once.

<a id="cpp-class-vs-struct"></a>
> **C++ note — `class` vs `struct`:** in C++ they are the same thing, with one
> difference: members of a `struct` are `public` by default, and members of a
> `class` are `private` by default. That is unlike C#, where `struct` means a
> value type and `class` means a reference type. In C++, *every* type is a
> value type unless you use pointers or references. arda uses `struct` for
> plain data with public fields (`DeviceLimits`, and `RenderState` in Step 1)
> and `class` for types with invariants and private members (`Device`). The
> `= 0` after each field is a *default member initializer*, which Step 1
> covers.

<a id="cpp-virtual-functions"></a>
> **C++ note — `virtual`, pure virtual, and abstract classes:** in C++,
> member functions are *non-virtual* by default. A call is bound at compile
> time from the static type, just like a non-`virtual` C# method. `virtual`
> makes the call dispatch at runtime on the object's real type, just like C#
> `virtual`. `= 0` makes a function *pure virtual*: it has no implementation in
> this class, which is C#'s `abstract`. A class with at least one pure
> virtual function is *abstract* and can't be instantiated, like a C#
> `abstract class`:
> ```cpp
> class Shape { public: virtual double Area() const = 0; };   // C#: public abstract double Area();
> Shape s;              // error: Shape is abstract
> Shape* p = &circle;   // fine: points at a derived object
> p->Area();            // calls Circle::Area at runtime
> ```
> The `const` after `Api()` is covered in the next note. `override` and
> `final`, which go on the derived side, come in [Part 9](#cpp-override-final).
>
> **Pitfall:** don't call a virtual function from a constructor or destructor.
> While `Device`'s constructor runs, the object is only a `Device` (the
> `DeviceGL3x` part doesn't exist yet), so the call goes to `Device`'s
> version, or crashes if it is pure. C# dispatches to the derived override
> even from a base constructor, so this is a real difference.

<a id="cpp-const-member-functions"></a>
> **C++ note — `const` member functions:** `GraphicsApi Api() const` promises
> that calling `Api()` doesn't modify the object. Inside it, `this` points to
> a `const Device`. Only `const` member functions can be called through a
> `const Device&`. `Limits() const` returns `const DeviceLimits&`: a reference
> to the member, without copying it, which the caller can read but not
> change. C# has no direct equivalent (C# `readonly` members on structs are
> the closest).

<a id="cpp-virtual-destructors"></a>
> **C++ note — virtual destructors:** `CreateDevice` returns a
> `std::unique_ptr<Device>` that actually points at a `DeviceGL3x`. When it
> goes out of scope, it runs `delete` on a `Device*`. If `~Device()` is not
> virtual, only `Device`'s destructor runs. `DeviceGL3x`'s destructor never
> runs (so its window is never destroyed), and the behaviour is undefined. The
> rule: **a class meant to be used through a base pointer needs a public
> virtual destructor.** `virtual ~Device() = default;` asks the compiler for
> the normal destructor, made virtual. C# never needs this, because the
> garbage collector knows every object's real type.

<a id="cpp-deleted-copy"></a>
> **C++ note — deleted copy operations and the rule of 0/3/5:** in C#,
> `var b = a;` copies a *reference*, and both names refer to one object. In
> C++, `Device b = a;` would copy the *object*, calling the copy constructor,
> which the compiler writes for you by copying every member. For a device,
> that would mean two objects that both think they own the same hidden window,
> and both would destroy it. `= delete` removes the copy constructor and
> copy assignment, so such code fails to compile:
> ```cpp
> Device(const Device&)            = delete;   // no: Device b = a;
> Device& operator=(const Device&) = delete;   // no: b = a;
> ```
> Deleting copy also stops the compiler from generating move operations, so
> `Device` can't be moved either. It stays at one address for its whole life,
> which matters when a pointer to it is handed to a C library (Part 10).
>
> The general rules:
> - **Rule of zero:** if every member manages itself (`std::string`,
>   `std::vector`, `std::unique_ptr`), write none of the five special
>   functions (destructor, copy constructor, copy assignment, move
>   constructor, move assignment). The generated ones are correct. Prefer this.
> - **Rule of three/five:** if you write any of them because the class owns a
>   raw resource, you almost always need to think about all five. `GLHandle`
>   ([Part 8](#8-srcglglhandleh)) is the example: it defines the destructor
>   and the two moves, and deletes the two copies.
> - **Polymorphic base classes** (`Device`, `GraphicsWindow`, `Context`):
>   virtual destructor, deleted copies. Copying through a base class would
>   *slice* the object, copying only the base part.

<a id="cpp-protected-constructors"></a>
> **C++ note — protected constructors:** `Device() = default;` is
> `protected`, so only derived classes can call it. `Device` is already
> abstract, so this doesn't change what compiles today, but it states the
> intent and keeps it true if the class ever stops being abstract.
> `SetLimits` is protected for the same reason: backends set limits, users
> only read them. C#'s `protected` means the same thing.

<a id="cpp-nvi"></a>
> **C++ note — the non-virtual interface (NVI) pattern:**
> `CreateGraphicsWindow` is public and **non-virtual**. It checks its
> arguments and then calls `DoCreateGraphicsWindow`, which is protected and
> pure virtual. Backends override only the `Do*` function:
> ```cpp
> std::unique_ptr<GraphicsWindow> Device::CreateGraphicsWindow(int w, int h, ...) {
>     if (w <= 0 || h <= 0) throw std::invalid_argument("...");   // written once, for every backend
>     return DoCreateGraphicsWindow(w, h, ...);                    // backend work
> }
> ```
> The benefits: validation lives in one place instead of being copied into
> every backend. The base class controls what happens before and after the
> backend call. And overloads stay together: in C++, declaring *one* overload
> of a name in a derived class hides *all* base-class overloads of that name,
> which surprises C# programmers. With NVI, backends override `Do*` names that
> have no overloads. Every `Device` and `Context` operation in later steps
> follows this pattern (`CreateShaderProgram` → `DoCreateShaderProgram`, `Draw`
> → `DoDraw`).

<a id="cpp-unique-ptr"></a>
> **C++ note — `std::unique_ptr` and `std::make_unique`:** C++ has no garbage
> collector. An object created with `new` lives until someone calls `delete`.
> `std::unique_ptr<T>` (from `<memory>`) is a smart pointer that *owns* one
> object and deletes it in its own destructor:
> ```cpp
> {
>     std::unique_ptr<Device> device = CreateDevice(GraphicsApi::OpenGL33);
>     device->Api();                    // use it like a pointer
> }                                     // device goes out of scope: the DeviceGL3x is deleted here
> ```
> "Unique" means one owner at a time. It can't be copied, only *moved*
> (ownership transfers and the source becomes null). Returning one from a
> function moves it automatically. `std::make_unique<T>(args...)` creates the
> object and the pointer in one step. A `unique_ptr<Derived>` converts
> implicitly to a `unique_ptr<Base>`, which is how `DeviceGL3x` is returned as
> a `Device`.
>
> Why `unique_ptr` for devices and windows: there is exactly one owner (the
> app), and the moment of destruction matters, because the window must close
> before the device. The README's ownership rules use `std::shared_ptr` for
> resources such as buffers, which several objects keep alive. Step 3 covers
> `shared_ptr`. Compared with C#: the `using` statement's `Dispose` runs at
> the end of a scope. A `unique_ptr` does the same, automatically, for every
> scope exit, including exceptions.

---

## 4. `include/arda/renderer/GraphicsWindow.h`

Replace the whole file (it currently holds the old `Window` class) with the
abstract window:

```cpp
#pragma once

#include <functional>
#include <utility>

namespace arda::renderer {

class Context;

// A window to draw into. Owns the Context used to draw into it (3.2).
class GraphicsWindow {
public:
    using Handler = std::function<void()>;

    virtual ~GraphicsWindow() = default;

    GraphicsWindow(const GraphicsWindow&)            = delete;
    GraphicsWindow& operator=(const GraphicsWindow&) = delete;

    // Runs until the window is closed. Calls the resize handler once
    // before the first frame, then the update and render handlers every frame.
    void Run();

    void SetResizeHandler(Handler handler)      { m_onResize = std::move(handler); }
    void SetUpdateFrameHandler(Handler handler) { m_onUpdateFrame = std::move(handler); }
    void SetRenderFrameHandler(Handler handler) { m_onRenderFrame = std::move(handler); }

    virtual Context& GetContext() = 0;
    virtual int Width() const = 0;    // framebuffer size in pixels
    virtual int Height() const = 0;

    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;

protected:
    GraphicsWindow() = default;

    // Backends call this when the framebuffer size has changed.
    void OnResize();

private:
    Handler m_onResize;
    Handler m_onUpdateFrame;
    Handler m_onRenderFrame;
};

} // namespace arda::renderer
```

How it maps to `GraphicsWindow.cs`:

| C# | C++ |
|---|---|
| `public event GraphicsHandler Resize;` (and `UpdateFrame`, `RenderFrame`) | `SetResizeHandler(Handler)` (and the other two) |
| `PreRenderFrame`, `PostRenderFrame` events | left out; nothing in Chapter 3 uses them |
| `protected virtual void OnResize()` raises the event | `protected void OnResize()` calls the handler if set |
| `public abstract void Run(double updateRate)` | non-virtual `void Run()`, implemented once in the base class |
| `abstract Context Context { get; }` | `virtual Context& GetContext() = 0` |
| `abstract int Width { get; }`, `Height` | `virtual int Width() const = 0`, `Height()`. Framebuffer pixels, not window size. |
| `Mouse`, `Keyboard` | later (input is not needed for Chapter 3) |
| (inside OpenTK) | `ShouldClose`, `PollEvents`, `SwapBuffers`: the loop's building blocks, implemented per backend |

> **Why the window header must not include GLFW:** the old `Window` stored a
> `GLFWwindow*`, so its header forward-declared `struct GLFWwindow;`. That was
> harmless, but it tied the public class to GLFW. The new public class stores
> no GLFW pointer at all. Only `GraphicsWindowGL3x` (a private header) does,
> and Step 8's `GraphicsWindowD3D11` stores an `HWND`-backed swap chain. If
> this header included `<GLFW/glfw3.h>`, every file that includes it
> (`main.cpp`, the tests) would need GLFW's include path, so CMake would have to
> link GLFW `PUBLIC`. GLFW's and GL's macros (`APIENTRY`, `GL_*`, and so on)
> would leak into scene code, and `glfw3.h` would pull in the system
> `<GL/gl.h>`, which then breaks any later `#include <glad/glad.h>` in the same
> file. Keep public headers free of GL, GLFW and D3D.

> **Why `Run()` is non-virtual:** the loop is the same for every backend. It
> is only a sequence of calls to the virtual building blocks. Writing it once
> in the base class is the *template method* pattern, a close cousin of NVI.
> OpenGlobe delegates `Run` to OpenTK's `GameWindow.Run`, so the backend owned
> the loop there. arda uses GLFW for both backends, so there is nothing
> backend-specific left in the loop.

> **Why one handler per event instead of C# multicast events:** a C# `event`
> can have many subscribers (`+=`). Every OpenGlobe example subscribes exactly
> once per event, so a single `std::function` per event is enough and keeps
> the code simple. Setting a handler again replaces the previous one. If you
> ever need several, a `std::vector<Handler>` is a small change.

> **Why no `updateRate`:** `Run(double updateRate)` asks OpenTK for a fixed
> number of updates per second. arda's loop is paced by vsync instead: one
> update and one render per displayed frame. A fixed-timestep update loop can
> be added later without changing any other API.

<a id="cpp-std-function-lambdas"></a>
> **C++ note — `std::function` and lambdas (basics):** `std::function<void()>`
> (from `<functional>`) holds *any* callable that takes no arguments and
> returns nothing: a free function, a lambda, or an object with `operator()`.
> It is the C++ counterpart of a C# delegate type like
> `delegate void GraphicsHandler()`. An empty `std::function` converts to
> `false`, which is how `OnResize` checks "is anyone subscribed?" (C#:
> `if (handler != null)`). Calling an empty one throws
> `std::bad_function_call`.
>
> A **lambda** is an inline anonymous function, like a C# lambda:
> ```cpp
> int frames = 0;
> window->SetRenderFrameHandler([&frames] { ++frames; });   // C#: window.RenderFrame += () => ++frames;
> ```
> The part in `[]` is the **capture list**, which says which local variables
> the lambda can use and how:
> - `[&frames]` captures `frames` *by reference*. The lambda uses the
>   original variable, so `++frames` changes it.
> - `[frames]` captures a *copy* made when the lambda is created.
> - `[&]` captures every used variable by reference. `[=]` captures copies.
>
> C# lambdas always capture variables "by reference" and the runtime keeps them
> alive as long as the lambda. C++ doesn't: a lambda that captured by
> reference must not be called after the captured variable is destroyed. In
> arda's `main`, handlers capture locals of `main` that outlive `Run()`, so
> `[&]` is safe. Step 1 covers the lifetime pitfalls in depth.
>
> `using Handler = std::function<void()>;` is a type alias, like C#'s
> `using Handler = System.Action;`. Part 8 covers `using` aliases for
> templates.

> **C++ note — `std::move` in the setters:** `SetResizeHandler(Handler handler)`
> takes its argument *by value*, and `std::move(handler)` then moves it into
> the member. The caller's lambda is moved (or copied, if the caller passes an
> lvalue) exactly once into the parameter, then moved again into the member.
> Moving a `std::function` just transfers its internal pointer. This
> "take by value, then move" idiom is the standard way to write a setter that
> stores its argument. Moves are explained in [Part 8](#cpp-move-semantics).

---

## 5. `include/arda/renderer/Context.h`

In Step 0, a context can only make itself current. Steps 1, 3, 5 and 6 add
`Clear`, `Draw`, texture units, framebuffers and vertex arrays. Replace the
stub with:

```cpp
#pragma once

namespace arda::renderer {

class Device;

// Issues rendering commands for one window (3.2, Listing 3.2).
// Steps 1, 3, 5 and 6 add Clear, Draw, texture units and framebuffers.
class Context {
public:
    virtual ~Context() = default;

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    Device& GetDevice() const { return m_device; }

    virtual void MakeCurrent() = 0;

protected:
    explicit Context(Device& device);

private:
    Device& m_device;
};

} // namespace arda::renderer
```

OpenGlobe's `Context` has no device member, because its `Device` is static
and reachable from anywhere. arda's device is an object, so each context keeps
a reference to the device that created it. Later steps use it:
`Context::CreateVertexArray(mesh, ...)` calls
`GetDevice().CreateMeshBuffers(...)` (Step 3), and automatic uniforms read
`context.GetDevice().ClipDepthRange()` (Step 4).

`MakeCurrent` comes straight from `Context.cs` line 31. For GL, it makes this
window's context current on the calling thread. For D3D11 (Step 8), where
there is only one device context, it records which window is active.

<a id="cpp-reference-members"></a>
> **C++ note — reference members:** `Device& m_device;` is a *reference*, not
> a pointer. A reference must be bound when it is created and can never be
> rebound or be null. So it must be initialized in the constructor's member
> initializer list (Part 6), and the class can't be copy-assigned (there is
> no way to "reassign" a reference). That fits a context: it belongs to one
> device forever. It also documents that the context doesn't *own* the
> device, it only refers to it. The device must outlive the context, which the
> destruction order in `main` guarantees (Part 13).
>
> `GetDevice() const` returns `Device&` (non-const) from a const function.
> That is allowed: `const` on a member function makes the *members*
> read-only, and for a reference member that means the reference itself
> can't be rebound. It doesn't make the referred-to object const. Pointers work
> the same way. This is sometimes called *shallow* constness.

<a id="cpp-explicit"></a>
> **C++ note — `explicit` constructors:** a constructor that can be called
> with one argument is also an *implicit conversion* unless it is marked
> `explicit`. Without `explicit`, a `Device&` could silently turn into a
> `Context` wherever a `Context` is expected. Here the constructor is also
> protected and the class is abstract, so the risk is small, but the rule of
> thumb is: **mark every single-argument constructor `explicit`** unless you
> want the conversion. C# has no implicit constructor conversions, so C# code
> never needed this. You'll see it again on `GLHandle(GLuint)`, where it
> matters more: without it, `BufferName b = 5;` would compile.

---

## 6. `src/Context.cpp` and `src/GraphicsWindow.cpp`

These are the API-agnostic parts of the two classes.

### `src/Context.cpp`

```cpp
#include <arda/renderer/Context.h>

namespace arda::renderer {

Context::Context(Device& device) : m_device(device) {}

} // namespace arda::renderer
```

The `: m_device(device)` part is the **member initializer list**. It
initializes members *before* the constructor body runs. A reference member
can only be set here. [Part 9](#cpp-member-order) explains the order in which
members are initialized.

### `src/GraphicsWindow.cpp`

Replace everything in the file (the old `Window` code) with:

```cpp
#include <arda/renderer/Context.h>
#include <arda/renderer/GraphicsWindow.h>

namespace arda::renderer {

void GraphicsWindow::Run() {
    // With several windows, the most recently created one has its context current.
    GetContext().MakeCurrent();

    // Lets the app set the viewport and aspect ratio before the first frame.
    // OpenTK's GameWindow does the same.
    OnResize();

    while (!ShouldClose()) {
        PollEvents();   // may call OnResize if the framebuffer size changed
        if (m_onUpdateFrame) {
            m_onUpdateFrame();
        }
        if (m_onRenderFrame) {
            m_onRenderFrame();
        }
        SwapBuffers();
    }
}

void GraphicsWindow::OnResize() {
    if (m_onResize) {
        m_onResize();
    }
}

} // namespace arda::renderer
```

`Run` is exactly the loop from [The render loop](#the-render-loop).
`GraphicsWindowGL3x.cs` does the same work in `OnRenderFrame` (lines 63–69):
raise the render event, then `SwapBuffers`.

`Run` calls pure virtual functions (`ShouldClose`, `PollEvents`,
`SwapBuffers`, `GetContext`) from the base class. That is fine, because `Run`
is only ever called on a fully constructed `GraphicsWindowGL3x`, so the calls
reach the backend's overrides.

`#include <arda/renderer/Context.h>` is needed here, but not in
`GraphicsWindow.h`: the header only mentions `Context&`, while this file
*calls* `MakeCurrent()` on it, which needs the full class.

> **Why `Run` makes the context current:** creating a window makes its
> context current (Part 10). If the app creates two windows and runs the
> first, the second window's context would otherwise be current, and every
> draw would go to the wrong window. Making it current once at the start
> costs nothing.

---

## 7. `src/GlfwLibrary.h` and `src/GlfwLibrary.cpp`

GLFW has to be initialized with `glfwInit()` before any other GLFW call, and
shut down with `glfwTerminate()` at the end. `glfwTerminate` destroys **every**
remaining window. The old `Window` class called both, so closing one window
would have destroyed all the others. With a device that owns a hidden window
and creates many real ones, and with Step 8 where a GL device and a D3D11
device (which also uses GLFW) can be alive together, initialization has to be
counted: the first user initializes, the last one terminates.

`GlfwLibrary` is a tiny class whose only job is that. It lives in `src/`, not
`src/gl/`, because the D3D11 backend uses it too.

### `src/GlfwLibrary.h`

```cpp
#pragma once

namespace arda::renderer {

// Calls glfwInit when the first instance is created and glfwTerminate when the last one is destroyed.
class GlfwLibrary {
public:
    GlfwLibrary();
    ~GlfwLibrary();

    GlfwLibrary(const GlfwLibrary&)            = delete;
    GlfwLibrary& operator=(const GlfwLibrary&) = delete;
};

} // namespace arda::renderer
```

### `src/GlfwLibrary.cpp`

```cpp
#include "GlfwLibrary.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <mutex>
#include <stdexcept>

namespace arda::renderer {

namespace {
std::mutex s_mutex;
int s_count = 0;

void ErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}
} // namespace

GlfwLibrary::GlfwLibrary() {
    std::lock_guard lock(s_mutex);
    if (s_count == 0) {
        glfwSetErrorCallback(&ErrorCallback);   // allowed before glfwInit, and reports glfwInit's own errors
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("glfwInit failed");
        }
    }
    ++s_count;
}

GlfwLibrary::~GlfwLibrary() {
    std::lock_guard lock(s_mutex);
    if (--s_count == 0) {
        glfwTerminate();
    }
}

} // namespace arda::renderer
```

This file doesn't include glad, because it makes no GL calls, so including
`<GLFW/glfw3.h>` alone is fine here. (GLFW will include the system
`<GL/gl.h>` in this translation unit, which does no harm when glad isn't also
included.)

The error callback matters: when `glfwCreateWindow` fails, GLFW returns
`nullptr` and says *why* only through this callback. For example: "WGL: The
driver does not appear to support OpenGL" or "Requested OpenGL version 3.3,
got version 2.1".

<a id="cpp-raii"></a>
> **C++ note — RAII (Resource Acquisition Is Initialization):** the most
> important idea in C++. *Tie every resource to the lifetime of an object:*
> the constructor acquires it and the destructor releases it. Destructors
> run automatically and deterministically when an object goes out of scope,
> whether by `return`, reaching the closing brace, or an exception unwinding
> the stack. So a resource owned by an object can't leak, and can't be released
> twice.
> ```cpp
> {
>     GlfwLibrary glfw;       // glfwInit()
>     // ... if anything here throws, glfw's destructor still runs ...
> }                           // glfwTerminate()
> ```
> In C#, the garbage collector frees memory whenever it likes, so non-memory
> resources need `IDisposable` plus a `using` block (or a finalizer, which
> runs at some unknown time on another thread). That is why OpenGlobe has
> `Disposable`, finalizers and `FinalizerThreadContextGL3x`. RAII replaces all
> of that. In this step: `GlfwLibrary` owns GLFW initialization,
> `DeviceGL3x` owns the share window, `GraphicsWindowGL3x` owns a GLFW window,
> `GLHandle` owns a GL name, and `unique_ptr` owns heap objects.

<a id="cpp-anonymous-namespaces"></a>
> **C++ note — anonymous namespaces and internal linkage:** names declared at
> namespace scope in a `.cpp` file are normally visible to the *linker* from
> every other `.cpp` file (*external linkage*). If two files both defined a
> global `int s_count`, linking would fail with a duplicate symbol, or worse,
> two functions with the same signature could silently clash. Wrapping them
> in `namespace { ... }` gives them *internal linkage*: they are private to
> this `.cpp` file, like C#'s `private static` fields of a class, but at file
> level. Use an anonymous namespace for every helper that isn't declared in a
> header. (The older way is the `static` keyword on each global, which does
> the same thing.)
>
> `s_mutex` and `s_count` are *static storage duration* variables: they exist
> for the whole program, like C# static fields. `s_count` starts at 0 before
> any code runs.

<a id="cpp-mutex"></a>
> **C++ note — `std::mutex`, `std::lock_guard` and CTAD:** a `std::mutex`
> (from `<mutex>`) allows only one thread at a time into a section of code.
> `std::lock_guard` is RAII for a mutex: its constructor locks and its
> destructor unlocks, so the mutex is released even if `glfwInit` fails and the
> constructor throws. It is the C++ version of C#'s `lock (s_mutex) { ... }`.
>
> `std::lock_guard lock(s_mutex);` doesn't say `std::lock_guard<std::mutex>`.
> C++17 **class template argument deduction** (CTAD) deduces the template
> argument from the constructor argument, the way `var` deduces a type in C#.
> (Before C++17 you had to write `std::lock_guard<std::mutex> lock(s_mutex);`.)
>
> Is the mutex needed? GLFW requires `glfwInit` and window creation on the main
> thread, so today every device is created on one thread anyway. The mutex
> makes the counter correct regardless of that, and costs nothing. Chapter 10
> (multithreading) creates resources on other threads, but not devices.

<a id="cpp-exceptions"></a>
> **C++ note — exceptions:** C++ exceptions work much like C#'s:
> ```cpp
> throw std::runtime_error("glfwInit failed");     // C#: throw new Exception("...");
>
> try {
>     auto device = CreateDevice(GraphicsApi::OpenGL33);
> } catch (const std::exception& error) {          // C#: catch (Exception error)
>     std::fprintf(stderr, "%s\n", error.what());  // C#: error.Message
> }
> ```
> Differences to know:
> - You throw by **value** (no `new`) and catch by **const reference**
>   (`const std::exception&`). Catching by value would copy and *slice* a
>   derived exception down to the base class.
> - The standard exceptions are in `<stdexcept>`, all derived from
>   `std::exception`. arda uses `std::runtime_error` for failures outside the
>   caller's control (no driver, `glfwInit` failed) and
>   `std::invalid_argument` for bad arguments (a zero width), similar to C#'s
>   `InvalidOperationException` and `ArgumentException`.
>   (OpenGlobe throws `ArgumentOutOfRangeException` for a negative width.)
> - There is no `finally`. RAII destructors do that job.
> - `catch (...)` catches anything, and a bare `throw;` inside a catch block
>   re-throws the current exception. Part 10 uses both.
> - **If a constructor throws, the object never existed, and its destructor
>   does not run.** Members and base classes that were already fully
>   constructed *are* destroyed. So anything a constructor acquired "by hand"
>   before the throw must be released by hand, or, better, be held by a member
>   that is itself RAII. Parts 9 and 10 deal with exactly this.
> - An exception must not pass through C code, such as a GLFW callback. Part
>   10 is designed around that.

---

### Checkpoint 1

Build the library:

```sh
cmake --build build/vs --config Debug --target arda_renderer
```

It should compile. `Device.cpp` and the `gl/` files are still empty stubs,
which compile to nothing. Typical errors at this point:

- *"cannot open include file `arda/renderer/Window.h`"*: `GraphicsWindow.cpp`
  still has the old code. Redo Part 6.
- *"use of undefined type `arda::renderer::Context`"* in `GraphicsWindow.cpp`:
  the `#include <arda/renderer/Context.h>` is missing.

Don't build `arda_scene` or `arda_tests` yet. `main.cpp` still includes
`Window.h`, and nothing defines `CreateDevice` yet.

---

## 8. `src/gl/GLHandle.h`

Nothing in Step 0 uses this yet, but it belongs with the backend's
foundations: Steps 2, 3, 5 and 6 build every GL object on it.

OpenGlobe has one class per kind of GL name (`BufferNameGL3x`,
`TextureNameGL3x`, `VertexArrayNameGL3x`, ...). Each one is the same 40 lines:
generate the name in the constructor, and delete it in `Dispose` and in a
finalizer. The finalizer runs on the .NET finalizer thread, which has no GL
context, so `FinalizerThreadContextGL3x` makes a hidden context current there
first. C++ destructors run on the thread that destroys the object, at a known
time, so all of that collapses into one class template:

```cpp
#pragma once

#include <glad/glad.h>

#include <utility>

namespace arda::renderer::gl {

// Owns one GL object name and deletes it on destruction. Move-only.
// A context from the object's share group must be current when it is destroyed.
template <typename Deleter>
class GLHandle {
public:
    GLHandle() = default;
    explicit GLHandle(GLuint value) : m_value(value) {}
    ~GLHandle() { Reset(); }

    GLHandle(GLHandle&& other) noexcept : m_value(std::exchange(other.m_value, 0)) {}
    GLHandle& operator=(GLHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            m_value = std::exchange(other.m_value, 0);
        }
        return *this;
    }

    GLHandle(const GLHandle&)            = delete;
    GLHandle& operator=(const GLHandle&) = delete;

    GLuint Get() const noexcept { return m_value; }

    void Reset() noexcept {
        if (m_value != 0) {
            Deleter{}(m_value);
            m_value = 0;
        }
    }

private:
    GLuint m_value = 0;
};

struct BufferDeleter      { void operator()(GLuint n) const { glDeleteBuffers(1, &n); } };
struct VertexArrayDeleter { void operator()(GLuint n) const { glDeleteVertexArrays(1, &n); } };
struct TextureDeleter     { void operator()(GLuint n) const { glDeleteTextures(1, &n); } };
struct SamplerDeleter     { void operator()(GLuint n) const { glDeleteSamplers(1, &n); } };
struct FramebufferDeleter { void operator()(GLuint n) const { glDeleteFramebuffers(1, &n); } };
struct ShaderDeleter      { void operator()(GLuint n) const { glDeleteShader(n); } };
struct ProgramDeleter     { void operator()(GLuint n) const { glDeleteProgram(n); } };

using BufferName      = GLHandle<BufferDeleter>;
using VertexArrayName = GLHandle<VertexArrayDeleter>;
using TextureName     = GLHandle<TextureDeleter>;
using SamplerName     = GLHandle<SamplerDeleter>;
using FramebufferName = GLHandle<FramebufferDeleter>;
using ShaderName      = GLHandle<ShaderDeleter>;
using ProgramName     = GLHandle<ProgramDeleter>;

inline BufferName CreateBufferName() {
    GLuint name = 0;
    glGenBuffers(1, &name);
    return BufferName(name);
}

inline VertexArrayName CreateVertexArrayName() {
    GLuint name = 0;
    glGenVertexArrays(1, &name);
    return VertexArrayName(name);
}

inline TextureName CreateTextureName() {
    GLuint name = 0;
    glGenTextures(1, &name);
    return TextureName(name);
}

inline SamplerName CreateSamplerName() {
    GLuint name = 0;
    glGenSamplers(1, &name);
    return SamplerName(name);
}

inline FramebufferName CreateFramebufferName() {
    GLuint name = 0;
    glGenFramebuffers(1, &name);
    return FramebufferName(name);
}

} // namespace arda::renderer::gl
```

There are no `CreateShaderName` or `CreateProgramName` helpers because shaders
and programs don't use the `glGen*` pattern. `glCreateShader(type)` and
`glCreateProgram()` return the name directly, so Step 2 writes
`ShaderName(glCreateShader(GL_VERTEX_SHADER))`. Their delete functions also
take a single name, not a count and an array, which is why `ShaderDeleter` and
`ProgramDeleter` look different.

> **OpenGL note — `glGen*` vs `glCreate*`, and what a fresh name is:** in GL
> 3.3, `glGenBuffers` only *reserves* a name. The driver creates the actual
> object the first time the name is bound (`glBindBuffer`). So a name from
> `CreateBufferName` isn't a buffer until Step 3 binds it. (GL 4.5 added
> `glCreateBuffers`, which creates the object immediately, but that isn't
> available in 3.3. glad declares it anyway, so the compiler won't stop you.)
> All the `glDelete*` functions ignore name 0, so the `m_value != 0` check in
> `Reset` is only there to avoid a pointless driver call.

> **OpenGL note — deleting needs a current context:** a `glDelete*` call acts
> on the current context, like every GL call. Deleting a *shared* object
> (buffer, texture, sampler, shader, program) works from any context in the
> share group. Deleting a *container* object (vertex array, framebuffer) must
> happen in the context that created it, otherwise the call deletes whatever
> object has that number in the current context, or nothing. That is why
> `ContextGL3x` calls `MakeCurrent()` before creating vertex arrays and
> framebuffers (Steps 3 and 6), and why a window must be destroyed only after
> the vertex arrays and framebuffers created through its context.

<a id="cpp-move-semantics"></a>
> **C++ note — move semantics, `std::exchange` and `noexcept` moves:** a
> `GLHandle` must never be copied: two copies would both delete the same
> name. But it should be *movable*, so it can be returned from
> `CreateBufferName()`, stored in a `std::vector`, or put in a
> `std::optional` (Step 2). A **move** transfers ownership: the new object
> takes the name and the old one is left empty (0), so only one of them
> deletes it.
>
> `GLHandle&&` is an *rvalue reference*: a reference to an object that is
> about to disappear (a temporary, or something passed through `std::move`).
> Overloading on it gives the move constructor and move assignment:
> ```cpp
> BufferName a = CreateBufferName();   // a owns name 1 (the return value is moved, or elided)
> BufferName b = std::move(a);         // move constructor: b owns 1, a holds 0
> b = CreateBufferName();              // move assignment: b deletes 1, then owns the new name
> ```
> `std::move` doesn't move anything by itself. It is a cast to `T&&` that says
> "I'm done with this object, you may take its contents". After a move, the
> source must still be safe to destroy, which is why it's set to 0.
>
> `std::exchange(x, 0)` (from `<utility>`) sets `x` to 0 and returns the old
> value in one expression. It is exactly "take the value and leave 0 behind".
>
> The move assignment `Reset()`s first, so the name it already owned is
> deleted rather than leaked. The `this != &other` check makes `a = std::move(a)`
> harmless: without it, `Reset()` would delete the name and then take 0 from
> itself.
>
> **`noexcept`** promises the function never throws. Mark move operations
> `noexcept` whenever they can't throw: `std::vector` only moves elements
> when it grows if the move constructor is `noexcept`. Otherwise it copies
> them to keep its strong exception guarantee, and a move-only type then
> fails to compile in some operations. Destructors are implicitly
> `noexcept`, and `Reset` is marked too because the destructor calls it.
>
> This class follows the **rule of five** from [Part 3](#cpp-deleted-copy):
> it writes the destructor and both moves, and deletes both copies.

<a id="cpp-class-templates"></a>
> **C++ note — class templates, function-object deleters and `using`
> aliases:** `template <typename Deleter> class GLHandle` is a *class
> template*: a recipe that the compiler turns into a separate class for each
> `Deleter` it is used with. It is like a C# generic `GLHandle<TDeleter>`, with
> one big difference: C++ templates are expanded at compile time for each
> type, and the template's code only has to compile for the types it's actually
> used with. There is no `where` constraint needed to call `Deleter{}(m_value)`
> (C++20 *concepts* can add constraints, which Step 3 covers). Because the
> compiler must see the template's code wherever it is used, **templates are
> written entirely in the header**. There is no `GLHandle.cpp`.
>
> Each deleter is a **function object**: a struct with an `operator()`, so an
> instance can be called like a function. `Deleter{}` creates an (empty)
> instance, and `Deleter{}(m_value)` calls it. The compiler inlines the call,
> so a `BufferName` compiles down to exactly a `GLuint` plus a direct
> `glDeleteBuffers` call, with no function pointer and no extra storage.
> `std::unique_ptr<T, Deleter>` uses the same technique, which Step 5 uses for
> stb_image.
>
> `using BufferName = GLHandle<BufferDeleter>;` gives each instantiation a
> readable name. This is the modern form of `typedef`. The names match
> OpenGlobe's `BufferNameGL3x` and friends.
>
> Why not `std::unique_ptr` with a custom deleter? `unique_ptr` holds a
> *pointer*, and a GL name is an integer. Wrapping an integer handle in a small
> move-only class is the usual solution.

> **C++ note — `inline` functions in headers:** `CreateBufferName` and the
> other helpers are defined in a header, so every `.cpp` that includes it
> gets a copy. `inline` tells the linker these copies are the same function,
> instead of reporting a duplicate symbol. Step 1 covers `inline` helpers in
> more detail. (Member functions defined inside a class body, like `Get()`,
> are implicitly `inline`.)

> **Why one template instead of eight classes:** OpenGlobe's name classes
> differ only in the `Gen`/`Delete` calls. A template captures the shared
> part (ownership, move, reset) once, and each deleter is one line. The
> finalizer machinery isn't needed because destructors run at a known time
> on the destroying thread. The one rule that remains is that a suitable
> context must be current then, as the OpenGL note above explains.

---

## 9. `src/gl/DeviceGL3x.h` and `src/gl/DeviceGL3x.cpp`

`DeviceGL3x` does three things when it's constructed:

1. creates the hidden 1×1 **share window** and makes its context current,
2. loads the GL functions with **glad**,
3. queries the GL **limits** (`Device.cs` lines 33–35).

And one when a window is requested: create a `GraphicsWindowGL3x` that shares
with the hidden window.

### `src/gl/DeviceGL3x.h`

```cpp
#pragma once

#include "GlfwLibrary.h"

#include <arda/renderer/Device.h>

struct GLFWwindow;

namespace arda::renderer::gl {

class DeviceGL3x final : public Device {
public:
    DeviceGL3x();
    ~DeviceGL3x() override;

    GraphicsApi Api() const override { return GraphicsApi::OpenGL33; }

    // Every window shares GL objects with this hidden window's context.
    GLFWwindow* ShareWindow() const { return m_shareWindow; }

    // Sets the GLFW hints for a GL 3.3 core context. Call before every glfwCreateWindow.
    static void ApplyContextHints(WindowType type);

protected:
    std::unique_ptr<GraphicsWindow> DoCreateGraphicsWindow(
        int width, int height, const std::string& title, WindowType type) override;

private:
    GlfwLibrary m_glfw;   // declared first so GLFW is initialized before the window is created
    GLFWwindow* m_shareWindow = nullptr;
};

} // namespace arda::renderer::gl
```

Notes on the header:

- `#include "GlfwLibrary.h"` with quotes: the compiler looks first in the
  including file's own folder (`src/gl/`), doesn't find it, then searches the
  include path, where `src/` (added by `PRIVATE src` in CMake) has it. Angle
  brackets (`<arda/renderer/Device.h>`) skip the file's own folder. The
  convention in arda: angle brackets for public headers and libraries,
  quotes for private `src/` headers.
- `struct GLFWwindow;` is a forward declaration at *global* scope, because
  GLFW declares `GLFWwindow` in the global namespace. Declaring it inside
  `namespace arda::renderer::gl` would declare a *different*, unrelated type
  `arda::renderer::gl::GLFWwindow`, and passing it to GLFW would fail to
  compile.
- `ApplyContextHints` is a `static` member function: it belongs to the class,
  not to an instance, like a C# `static` method. `GraphicsWindowGL3x` calls it
  as `DeviceGL3x::ApplyContextHints(type)`. Step 1 covers static member
  functions.
- `DoCreateGraphicsWindow` stays `protected` in the derived class, so users
  still have to go through `Device::CreateGraphicsWindow` and its checks.

<a id="cpp-override-final"></a>
> **C++ note — `override` and `final`:** `override` after a member function
> says "this overrides a virtual function in a base class". If it doesn't
> (a typo in the name, a missing `const`, a different parameter type), the
> compiler reports an error, instead of silently creating a new, unrelated
> function that is never called. In C#, `override` is required. In C++ it is
> optional but you should **always** write it. (`virtual` on the derived
> function is then redundant. Leave it off.)
>
> `final` on the class (`class DeviceGL3x final : public Device`) forbids
> deriving from `DeviceGL3x`, like C#'s `sealed`. It documents that the
> backend classes are leaves, and it lets the compiler turn virtual calls on a
> `DeviceGL3x&` into direct calls.
>
> `~DeviceGL3x() override;` is legal because the base destructor is virtual,
> and it makes the compiler confirm that. `: public Device` is public
> inheritance, which is what C#'s `: Device` means. (C++ also has `private`
> and `protected` inheritance, which arda doesn't use.)

<a id="cpp-member-order"></a>
> **C++ note — member initialization and destruction order:** members are
> initialized **in the order they are declared in the class**, not the order
> they appear in the constructor's initializer list, and destroyed in
> **reverse** declaration order after the destructor's body has run. Base
> classes are initialized before any member and destroyed after all of them.
> For `DeviceGL3x`:
>
> | Construction | Destruction |
> |---|---|
> | 1. `Device` base (`m_limits`) | 4. `~DeviceGL3x()` body: `glfwDestroyWindow(m_shareWindow)` |
> | 2. `m_glfw`: `glfwInit()` | 5. `m_shareWindow`: nothing (a raw pointer) |
> | 3. `m_shareWindow = nullptr` | 6. `m_glfw`: `glfwTerminate()` |
> | 4. constructor body: create the window, load glad | 7. `Device` base |
>
> So `m_glfw` must be declared *before* any member whose initialization
> calls GLFW. Today `m_shareWindow` starts as `nullptr` and is only set in the
> body, so nothing breaks yet, but if a later change initialized a member with
> a GLFW call and that member were declared above `m_glfw`, it would run
> before `glfwInit`. On the way out, reverse order means `m_glfw` goes last,
> after everything that might still use GLFW. Compilers warn when the
> initializer list order disagrees with the declaration order (`-Wreorder` on
> GCC and Clang), because the list order is misleading: it is never the order
> that actually runs.
>
> Constructor failure uses the same rules. If the body throws after
> `m_glfw` is constructed, `m_glfw` is destroyed (so GLFW is terminated), but
> `~DeviceGL3x()` does **not** run, because the object never finished
> constructing. That is why the constructor destroys the share window itself
> before throwing when glad fails to load.

### `src/gl/DeviceGL3x.cpp`

```cpp
#include "gl/DeviceGL3x.h"
#include "gl/GraphicsWindowGL3x.h"

// glad must come before GLFW: glfw3.h includes the system GL header unless one was already included.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace arda::renderer::gl {

void DeviceGL3x::ApplyContextHints(WindowType type) {
    // Hints are global GLFW state that persists between glfwCreateWindow calls,
    // so start from the defaults every time.
    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Required for a core profile on macOS. OpenGlobe requests it on every platform
    // (GraphicsContextFlags.ForwardCompatible), so Windows catches the same mistakes.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

#ifndef NDEBUG
    // OpenGlobe always asks for a debug context (GraphicsContextFlags.Debug). Debug builds only here.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    // OpenGlobe's GraphicsMode(24, 24, 8): 24-bit depth, 8-bit stencil.
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    glfwWindowHint(GLFW_VISIBLE, type == WindowType::Hidden ? GLFW_FALSE : GLFW_TRUE);
}

DeviceGL3x::DeviceGL3x() {
    // m_glfw has already called glfwInit (it is constructed before this body runs).

    ApplyContextHints(WindowType::Hidden);
    m_shareWindow = glfwCreateWindow(1, 1, "arda share context", nullptr, nullptr);
    if (m_shareWindow == nullptr) {
        // GLFW's error callback has printed the reason.
        throw std::runtime_error("DeviceGL3x: could not create an OpenGL 3.3 core context");
    }

    // glad needs a current context, and resources created through the device
    // are created while some context of this share group is current.
    glfwMakeContextCurrent(m_shareWindow);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        // The destructor won't run for a constructor that throws, so clean up here.
        glfwDestroyWindow(m_shareWindow);
        throw std::runtime_error("DeviceGL3x: gladLoadGL failed");
    }

    // Device.cs static constructor, lines 33-35.
    DeviceLimits limits;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &limits.maximumNumberOfVertexAttributes);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &limits.numberOfTextureUnits);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &limits.maximumNumberOfColorAttachments);
    SetLimits(limits);

    // Step 4 adds: InitializeCommon();
}

DeviceGL3x::~DeviceGL3x() {
    glfwDestroyWindow(m_shareWindow);
    // m_glfw is destroyed next and calls glfwTerminate if this was the last user.
}

std::unique_ptr<GraphicsWindow> DeviceGL3x::DoCreateGraphicsWindow(
    int width, int height, const std::string& title, WindowType type) {
    return std::make_unique<GraphicsWindowGL3x>(*this, width, height, title, type);
}

} // namespace arda::renderer::gl
```

Walking through it:

- **`glfwDefaultWindowHints()`.** GLFW window hints are yet another global
  state machine: a hint stays set until changed. Resetting first means each
  window gets exactly the hints listed, whatever code ran before (Step 8's
  D3D11 window sets `GLFW_CLIENT_API = GLFW_NO_API`, which would otherwise
  leak into the next GL window and create a window with no context).
- **Context hints.** Version 3.3, core profile and forward compatible, as
  explained in [Core profile and forward compatibility](#core-profile-and-forward-compatibility).
  If the driver can't provide it, `glfwCreateWindow` returns `nullptr`.
- **Depth and stencil bits.** These describe the *default framebuffer* of the
  window: a 24-bit depth buffer and 8-bit stencil buffer. 24/8 are GLFW's
  defaults too, but they are written out to match OpenGlobe's
  `GraphicsMode(24, 24, 8)` and to make the dependency visible: depth testing
  in Step 1 only works if the window *has* a depth buffer.
- **`glfwCreateWindow(1, 1, ..., nullptr, nullptr)`.** The fourth argument is a
  monitor for fullscreen (none). The fifth is the share window (none: this
  *is* the share window). Sharing must match: every window that shares with
  it uses the same hints, via `ApplyContextHints`.
- **`glfwMakeContextCurrent` then `gladLoadGLLoader`.** The order matters, as
  explained in [Loading GL functions with glad](#loading-gl-functions-with-glad).
- **`glGetIntegerv(pname, &value)`** reads one piece of integer state from the
  current context. The three limits are implementation-dependent, and the
  GL 3.3 spec guarantees minimums: 16 vertex attributes, 48 combined texture
  units (16 per shader stage × 3 stages), and 8 color attachments. The tests
  check these minimums.
- **`GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`**, not
  `GL_MAX_TEXTURE_IMAGE_UNITS`, matches the C# (line 34). The combined value is
  the number of texture units that exist. The per-stage one is how many a
  fragment shader can use at once. Step 5 creates one `TextureUnit` per unit.
- **`std::make_unique<GraphicsWindowGL3x>(*this, ...)`** passes the device by
  reference (`*this` dereferences the `this` pointer). The resulting
  `unique_ptr<GraphicsWindowGL3x>` converts to `unique_ptr<GraphicsWindow>`
  on return.

> **OpenGL note — why the share context stays current after construction:**
> once the device exists, *some* context must be current before any GL call,
> including creating a shader in Step 2 before any window exists. The share
> context is that context. After a window is created, its context becomes
> current instead (Part 10). Either one works for creating shared resources,
> because they are in the same share group.

> **C++ note — `reinterpret_cast` for the loader:** `glfwGetProcAddress` has the
> type `GLFWglproc (*)(const char*)`, where `GLFWglproc` is `void (*)()`.
> glad wants `GLADloadproc`, which is `void* (*)(const char*)`. The two
> function pointer types differ only in the return type, and on every platform
> GL runs on, both return the same address in the same register.
> `reinterpret_cast` says "treat these bits as the other type", which is the
> documented way to connect the two libraries. `reinterpret_cast` is the
> most dangerous cast, since the compiler checks nothing, so keep it to
> boundaries with C libraries like this one. `static_cast`, the safe everyday
> cast, comes in [Part 10](#cpp-static-cast).

<a id="cpp-preprocessor"></a>
> **C++ note — preprocessor `#if` for build configuration:** before compiling,
> the preprocessor removes code based on macros. `#ifndef NDEBUG ... #endif`
> keeps the debug context hint only when `NDEBUG` is *not* defined. CMake
> defines `NDEBUG` in Release builds (it is also what turns `assert` off). The
> removed code isn't compiled at all, so it can even call functions that don't
> exist in that configuration. The forms you'll see in arda:
> ```cpp
> #ifdef __APPLE__          // defined by the compiler on macOS
> #ifndef NDEBUG            // Debug builds
> #if ARDA_HAS_GL           // from target_compile_definitions (Part 12)
> #ifdef _WIN32             // Windows (Step 8)
> ```
> `#if X` tests a value, and an undefined `X` counts as 0, so
> `#if ARDA_HAS_D3D11` is false when that macro was never defined. `#ifdef X`
> only tests whether `X` is defined at all. Use `#if` for 0/1 feature
> switches.
>
> C# has `#if DEBUG`, which works the same way but can only test whether a
> symbol is defined. Prefer normal C++ code (`if constexpr`, virtual
> functions) where possible. Use the preprocessor only for code that must not
> even be compiled in some configurations: another platform's API, or a backend
> that wasn't built.

---

## 10. `src/gl/GraphicsWindowGL3x.h` and `src/gl/GraphicsWindowGL3x.cpp`

This is where most of the old `Window` code ends up. The GL window owns a GLFW
window (and so its GL context) and the `ContextGL3x` that draws into it.

It maps to `GraphicsWindowGL3x.cs`:

| C# | C++ |
|---|---|
| `new GameWindow(width, height, new GraphicsMode(24, 24, 8), title, flags, ..., 3, 3, ForwardCompatible \| Debug)` | `ApplyContextHints(type)` + `glfwCreateWindow(..., device.ShareWindow())` |
| `_gameWindow.MakeCurrent()` | `glfwMakeContextCurrent(m_handle)` |
| `_gameWindow.Resize += OnResize` | `glfwSetFramebufferSizeCallback` + `PollEvents` calling `OnResize` |
| `_context = new ContextGL3x(_gameWindow, width, height)` | `m_context = std::make_unique<ContextGL3x>(device, m_handle, m_width, m_height)` |
| `_gameWindow.SwapBuffers()` | `glfwSwapBuffers(m_handle)` |
| `Dispose` → `_gameWindow.Dispose()` | destructor: destroy the context, then `glfwDestroyWindow` |
| `width < 0` → `ArgumentOutOfRangeException` | `Device::CreateGraphicsWindow` rejects `<= 0` before the backend is called |

### `src/gl/GraphicsWindowGL3x.h`

```cpp
#pragma once

#include <arda/renderer/GraphicsApi.h>
#include <arda/renderer/GraphicsWindow.h>

#include <memory>
#include <string>

struct GLFWwindow;

namespace arda::renderer::gl {

class DeviceGL3x;
class ContextGL3x;

class GraphicsWindowGL3x final : public GraphicsWindow {
public:
    GraphicsWindowGL3x(DeviceGL3x& device, int width, int height, const std::string& title, WindowType type);
    ~GraphicsWindowGL3x() override;

    Context& GetContext() override;
    int Width() const override { return m_width; }
    int Height() const override { return m_height; }

    bool ShouldClose() const override;
    void PollEvents() override;
    void SwapBuffers() override;

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    DeviceGL3x& m_device;
    GLFWwindow* m_handle = nullptr;
    std::unique_ptr<ContextGL3x> m_context;
    int m_width = 0;
    int m_height = 0;
    bool m_resizePending = false;   // set by the GLFW callback, handled in PollEvents
};

} // namespace arda::renderer::gl
```

`ContextGL3x` is only forward-declared, but the class stores a
`std::unique_ptr<ContextGL3x>`. That works because the `unique_ptr` only
needs the complete type where the pointer is *destroyed* (in
`~GraphicsWindowGL3x`, and in the constructor's cleanup if it throws) and
where it is *dereferenced* (`GetContext`). All of those are defined in the
`.cpp`, which includes `ContextGL3x.h`. That's why the destructor is
declared here but defined in the `.cpp`, even though it could look trivial.
If it were `= default` in the header, every file including this header would
need the full `ContextGL3x`.

### `src/gl/GraphicsWindowGL3x.cpp`

```cpp
#include "gl/GraphicsWindowGL3x.h"
#include "gl/ContextGL3x.h"
#include "gl/DeviceGL3x.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace arda::renderer::gl {

GraphicsWindowGL3x::GraphicsWindowGL3x(
    DeviceGL3x& device, int width, int height, const std::string& title, WindowType type)
    : m_device(device) {
    DeviceGL3x::ApplyContextHints(type);
    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, device.ShareWindow());
    if (m_handle == nullptr) {
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, &FramebufferSizeCallback);
    glfwGetFramebufferSize(m_handle, &m_width, &m_height);   // may differ from width/height on high-DPI screens

    glfwMakeContextCurrent(m_handle);
    glfwSwapInterval(1);   // vsync; applies to the current context

    // ContextGL3x makes GL calls, so this window's context must be current first.
    // If it throws, this constructor never finishes, so the destructor won't
    // destroy the window. Do it here.
    try {
        m_context = std::make_unique<ContextGL3x>(device, m_handle, m_width, m_height);
    } catch (...) {
        glfwDestroyWindow(m_handle);
        throw;
    }
}

GraphicsWindowGL3x::~GraphicsWindowGL3x() {
    const bool wasCurrent = glfwGetCurrentContext() == m_handle;

    // Destroy the context while its window (and GL context) still exist.
    m_context.reset();
    glfwDestroyWindow(m_handle);   // if its context was current, no context is current now

    // Keep a context of the share group current, so resources destroyed after
    // this window (buffers, textures, programs) can still be deleted.
    if (wasCurrent) {
        glfwMakeContextCurrent(m_device.ShareWindow());
    }
}

Context& GraphicsWindowGL3x::GetContext() {
    return *m_context;
}

bool GraphicsWindowGL3x::ShouldClose() const {
    return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

void GraphicsWindowGL3x::PollEvents() {
    glfwPollEvents();   // calls FramebufferSizeCallback for any window whose size changed

    // Raise the event here, in C++ code, not inside the GLFW callback: an
    // exception thrown by the app's resize handler must not pass through GLFW's C code.
    if (m_resizePending) {
        m_resizePending = false;
        OnResize();
    }
}

void GraphicsWindowGL3x::SwapBuffers() {
    glfwSwapBuffers(m_handle);
}

void GraphicsWindowGL3x::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<GraphicsWindowGL3x*>(glfwGetWindowUserPointer(window));
    if (width == 0 || height == 0) {
        return;   // minimized; keep the last real size so aspect ratios stay finite
    }
    self->m_width = width;
    self->m_height = height;
    self->m_resizePending = true;
}

} // namespace arda::renderer::gl
```

What each GLFW call does, in constructor order:

1. **`ApplyContextHints(type)`**: same hints as the share window, which
   sharing requires. Only `GLFW_VISIBLE` differs between hidden and normal
   windows.
2. **`glfwCreateWindow(width, height, title, nullptr, share)`**: creates the OS
   window and its GL context, in the share window's share group. `width` and
   `height` are in screen coordinates. `title.c_str()` gives GLFW the
   `const char*` it wants: a `std::string` keeps a null-terminated buffer
   internally, and `c_str()` exposes it.
3. **`glfwSetWindowUserPointer(m_handle, this)`**: stores a pointer to this C++
   object inside the GLFW window, so the static callback can find it again.
4. **`glfwSetFramebufferSizeCallback`**: GLFW will call
   `FramebufferSizeCallback` during `glfwPollEvents` whenever this window's
   framebuffer size changes.
5. **`glfwGetFramebufferSize`**: the starting size in *pixels* (see
   [Framebuffer size vs window size](#framebuffer-size-vs-window-size)).
6. **`glfwMakeContextCurrent(m_handle)`**: the new context becomes current,
   which the next two calls require.
7. **`glfwSwapInterval(1)`**: vsync for this context
   ([Double buffering and vsync](#double-buffering-and-vsync)).
8. **`ContextGL3x`**: made last, because its constructor sets the viewport of
   the current context.

> **Why the resize event is raised from `PollEvents`, not from the callback:**
> the GLFW callback runs *inside* `glfwPollEvents`, which is C code. If the
> app's resize handler throws (for example `SetViewport` in Step 1 throws
> `std::invalid_argument`), the exception would unwind through GLFW's C stack
> frames. That is undefined behaviour: it may skip GLFW's internal cleanup, or
> terminate the program. So the callback only records the new size and sets a
> flag. `PollEvents` raises the event after `glfwPollEvents` returns, in
> ordinary C++ code, and an exception reaches `main`'s `catch` normally. The
> app sees the same thing either way: `OnResize` runs after events are
> processed and before the frame's update and render. (With several windows,
> each one's flag is checked when *its* `PollEvents` runs.)

> **Why minimized windows are ignored:** on Windows, minimizing reports a
> 0×0 framebuffer. A resize handler that computes `width / height` for an
> aspect ratio would divide by zero. Keeping the last real size means
> `Width()` and `Height()` are never 0 after construction. Rendering while
> minimized is harmless: the output simply isn't shown.

> **Why the destructor order is written out:** `~GraphicsWindowGL3x()` runs
> its body first, and then destroys the members (`m_context` among them). If
> the body only called `glfwDestroyWindow`, `m_context` would be destroyed
> *after* its window and GL context were gone. `ContextGL3x` has nothing to
> clean up in Step 0, but it gains state in later steps, so `m_context.reset()`
> destroys it first, while everything it refers to still exists. See
> [member destruction order](#cpp-member-order).

> **OpenGL note — what happens to the current context on destroy:** when a
> window whose context is current is destroyed, GLFW makes no context current
> on that thread. Any later GL call, such as a `BufferName` being destroyed at
> the end of `main`, would then run with no context. The destructor avoids
> this by switching to the device's share context, but only if this window's
> context was the current one. If another window's context is current, it is
> left alone, so that window keeps drawing into itself.

<a id="cpp-c-callbacks"></a>
> **C++ note — C callbacks with user pointers:** GLFW is a C library. It takes
> callbacks as plain function pointers:
> `void (*)(GLFWwindow*, int, int)`. A C function pointer can't point to a
> *non-static* member function, because a member function needs a `this`
> object, and C has no way to pass one. (C# delegates bundle the target object
> with the method, so this problem doesn't come up there.)
>
> The standard solution has three parts:
> 1. Make the callback a **`static` member function** (or a free function).
>    It has no `this`, so it converts to an ordinary function pointer. As a
>    member, it can still access private members such as `m_width`.
> 2. Store `this` in the C library's **user pointer** slot for the window:
>    `glfwSetWindowUserPointer(m_handle, this)`. GLFW stores it as `void*` and
>    never looks at it.
> 3. In the callback, get it back with `glfwGetWindowUserPointer(window)` and
>    cast it to the real type.
>
> The pointer must stay valid for as long as GLFW might call back. That's why
> `GraphicsWindow` is non-copyable and non-movable
> ([deleted copies](#cpp-deleted-copy)): the object never changes address
> while the GLFW window exists, and the window is destroyed in the object's
> destructor. The object is also heap-allocated by `make_unique`, so it stays
> put however the `unique_ptr` itself is moved around.

<a id="cpp-static-cast"></a>
> **C++ note — `static_cast`:** C++ has named casts instead of C#'s single
> `(T)x`, so each cast says how dangerous it is:
> - **`static_cast<T>(x)`**: conversions the compiler can check are
>   plausible: numeric conversions (`static_cast<int>(3.7)`, enum to integer),
>   `void*` back to the original pointer type, and base-to-derived pointer
>   casts where *you* guarantee the real type (no runtime check, unlike C#'s
>   `(Derived)b`, which throws if wrong).
> - **`dynamic_cast`**: base-to-derived with a runtime check (Step 2).
> - **`reinterpret_cast`**: reinterpret the bits (Part 9's glad loader).
> - **`const_cast`**: removes `const`. Almost never needed.
>
> Here, `static_cast<GraphicsWindowGL3x*>(glfwGetWindowUserPointer(window))`
> turns the `void*` back into the pointer that was stored. That is correct
> only because the constructor stored exactly a `GraphicsWindowGL3x*`. If it had
> stored a `GraphicsWindow*` (the base), the cast would have to go back to
> `GraphicsWindow*`. The rule for `void*`: cast back to exactly the type you
> converted from. Backends use `static_cast` for base-to-derived downcasts all
> the time (`static_cast<VertexArrayGL3x&>(...)` in Step 3), following the same
> rule: only when the real type is known.
>
> Avoid C-style casts `(T)x` in C++. They silently pick whichever of the
> above works, including `reinterpret_cast` and `const_cast`.

---

## 11. `src/gl/ContextGL3x.h` and `src/gl/ContextGL3x.cpp`

In Step 0 the GL context only remembers its GLFW window (for `MakeCurrent`)
and sets the initial viewport. Step 1 adds the cached render state and
`Clear`.

### `src/gl/ContextGL3x.h`

```cpp
#pragma once

#include <arda/renderer/Context.h>

struct GLFWwindow;

namespace arda::renderer::gl {

class ContextGL3x final : public Context {
public:
    ContextGL3x(Device& device, GLFWwindow* window, int width, int height);

    void MakeCurrent() override;

private:
    GLFWwindow* m_window;
};

} // namespace arda::renderer::gl
```

### `src/gl/ContextGL3x.cpp`

```cpp
#include "gl/ContextGL3x.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace arda::renderer::gl {

ContextGL3x::ContextGL3x(Device& device, GLFWwindow* window, int width, int height)
    : Context(device), m_window(window) {
    glViewport(0, 0, width, height);   // Step 1 replaces this with SetViewport
}

void ContextGL3x::MakeCurrent() {
    glfwMakeContextCurrent(m_window);
}

} // namespace arda::renderer::gl
```

- **`: Context(device), m_window(window)`** calls the protected base-class
  constructor first (base classes are always initialized before members,
  whatever order you write them in), then initializes `m_window`.
- **`ContextGL3x` doesn't own the `GLFWwindow`.** `GraphicsWindowGL3x` owns it
  and destroys the context before the window. The raw pointer here is a
  non-owning reference to it. In arda, a raw pointer (`T*`) or reference
  always means "not owned"; ownership is always a smart pointer or an RAII
  member.
- **The constructor takes `Device&`, not `DeviceGL3x&`.** `GraphicsWindowGL3x`
  passes its `DeviceGL3x&`, which converts to the base `Device&` implicitly.
  Nothing in `ContextGL3x` needs GL-specific device functions yet.

> **OpenGL note — the viewport:** after the vertex shader, positions are in
> *normalized device coordinates* (NDC), where x and y run from -1 to 1 across
> the drawable area. `glViewport(x, y, width, height)` sets how NDC maps to
> framebuffer pixels: -1 maps to `x` and +1 to `x + width`, with y measured
> from the **bottom** of the window. A new context's viewport is already the
> window's size, but the viewport is *not* updated when the window resizes.
> After a resize, drawing would stay in the old rectangle. So the viewport is
> set explicitly here, and from the resize handler in Step 1
> (`context.SetViewport({0, 0, window->Width(), window->Height()})`). The
> width and height passed here are the **framebuffer** size in pixels.
> `ContextGL3x.cs` does the same with `Viewport = new Rectangle(0, 0, width, height)` (line 40).

---

### Checkpoint 2

```sh
cmake --build build/vs --config Debug --target arda_renderer
```

The whole GL backend should now compile. `Device.cpp` is still empty, so
`CreateDevice` has no definition yet, but a static library doesn't need one.

If you see *"OpenGL header already included, remove this include, glad
already provides it"*, a file includes `<GLFW/glfw3.h>` before
`<glad/glad.h>`. Swap them. If you see errors about
`arda::renderer::gl::GLFWwindow`, the `struct GLFWwindow;` forward
declaration ended up inside the namespace.

---

## 12. `src/Device.cpp`

The last piece: the public device's non-virtual function, and the factory
that picks a backend.

```cpp
#include <arda/renderer/Device.h>
#include <arda/renderer/GraphicsWindow.h>

#if ARDA_HAS_GL
#include "gl/DeviceGL3x.h"
#endif

#include <stdexcept>

namespace arda::renderer {

std::unique_ptr<GraphicsWindow> Device::CreateGraphicsWindow(
    int width, int height, const std::string& title, WindowType type) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("CreateGraphicsWindow: width and height must be greater than zero");
    }
    return DoCreateGraphicsWindow(width, height, title, type);
}

bool IsGraphicsApiAvailable(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL33:
#if ARDA_HAS_GL
        return true;
#else
        return false;
#endif
    case GraphicsApi::Direct3D11:
#if ARDA_HAS_D3D11
        return true;
#else
        return false;
#endif
    }
    return false;
}

std::unique_ptr<Device> CreateDevice(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL33:
#if ARDA_HAS_GL
        return std::make_unique<gl::DeviceGL3x>();
#else
        break;
#endif
    case GraphicsApi::Direct3D11:
        // Step 8: return std::make_unique<d3d11::DeviceD3D11>();
        break;
    }
    throw std::runtime_error("CreateDevice: this graphics API was not compiled in");
}

} // namespace arda::renderer
```

Details:

- **`CreateGraphicsWindow`** is the NVI wrapper from
  [Part 3](#cpp-nvi). It rejects `0` as well as negative sizes, because
  `glfwCreateWindow` fails on a zero size (OpenGlobe only rejects negative
  ones, and OpenTK coped with 0). The check runs *before* the backend, so the
  test in Part 14 can check it without a GPU doing anything.
- **`Device.cpp` is the only public-side file that includes a backend
  header**, and only when that backend is compiled in. `gl/DeviceGL3x.h` is
  found through the `PRIVATE src` include path.
- **The `switch` has no `default:`**. Every enumerator has a `case`, so if a
  third `GraphicsApi` is ever added, compilers warn about the missing case
  (`-Wswitch`, MSVC C4062 at `/W4`). Step 1 covers this. The `return false;`
  after the switch is still needed, because an `enum class` variable *can*
  hold a value that isn't one of the enumerators (`static_cast<GraphicsApi>(7)`),
  and falling off the end of a non-void function is undefined behaviour.
- **In `CreateDevice`, the `case OpenGL33:` with GL disabled does `break`**,
  so control goes to the `throw` at the end. With GL enabled, `return` leaves
  the function, so the code never falls through into the next case.
- `ARDA_HAS_D3D11` isn't defined anywhere yet, so `#if ARDA_HAS_D3D11` is
  false (an undefined macro counts as 0 in `#if`).

> **Why a free function `CreateDevice` instead of a constructor:** the caller
> asks for an API, and gets a base-class pointer to whichever backend class
> implements it. The caller can't name `gl::DeviceGL3x`, because it lives in a
> private header. This is the *factory function* pattern, and it's the only
> place outside `src/gl/` that knows the GL class exists. `IsGraphicsApiAvailable`
> lets callers (and Step 8's tests) check first instead of catching the
> exception.

---

### Checkpoint 3

```sh
cmake --build build/vs --config Debug --target arda_renderer
```

The library is complete. `arda_scene` still won't build until the next part.

---

## 13. `scene/src/main.cpp`

Replace the old loop with the device, window and `Run`:

```cpp
#include <arda/renderer/Device.h>
#include <arda/renderer/GraphicsWindow.h>

#include <cstdio>
#include <exception>

int main() {
    using namespace arda::renderer;
    try {
        // Declared first so it is destroyed last: windows and resources must go before the device.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(1280, 720, "Arda");

        int frames = 0;
        window->SetResizeHandler([&] {
            std::printf("framebuffer: %d x %d\n", window->Width(), window->Height());
        });
        window->SetRenderFrameHandler([&frames] {
            ++frames;   // Step 1 clears the window here
        });

        window->Run();
        std::printf("rendered %d frames\n", frames);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

The old `main.cpp` also created `Ellipsoid::WGS84()` without using it. You can
keep the `arda/core` includes and that line if you like: `arda_scene` still
gets `arda_core` through `arda_renderer`'s `PUBLIC` link.

`using namespace arda::renderer;` inside `main` lets you write `CreateDevice`
instead of `arda::renderer::CreateDevice`. It is scoped to the function. Never
put `using namespace` at file scope in a header, because it would apply to
every file that includes it.

**Destruction order** is the reverse of declaration order for local variables
too. When `try`'s block ends (normally or by exception), `window` is destroyed
first (its context, then its GLFW window), then `device` (the share window,
then `glfwTerminate`). If you declared `window` in an outer scope that outlives
`device`, `glfwTerminate` would destroy the GLFW window first, and the
window's destructor would then pass a dangling handle to `glfwDestroyWindow`.

### Milestone, part 1: run it

```sh
./run.sh
```

(or `cmake --build build/vs --config Debug --target arda_scene` and run
`build/vs/bin/Debug/arda_scene.exe`).

You should see:

- a 1280×720 window titled "Arda". Its contents are black or garbage, because
  nothing clears the back buffer until Step 1.
- `framebuffer: 1280 x 720` printed once at startup (on a Mac with a Retina
  display, `2560 x 1440`),
- another line each time you resize the window, and none when you minimize it,
- after closing the window, `rendered N frames`, where N is roughly your
  monitor's refresh rate times the number of seconds it was open (vsync), and
  exit code 0.

---

## 14. Tests: `tests/src/renderer/DeviceTests.cpp`

`tests/CMakeLists.txt` already lists this file (Part 1). Fill it in:

```cpp
#include <doctest/doctest.h>

#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/GraphicsWindow.h>

#include <stdexcept>

using namespace arda::renderer;

TEST_CASE("CreateDevice creates an OpenGL 3.3 device with the GL 3.3 minimum limits") {
    REQUIRE(IsGraphicsApiAvailable(GraphicsApi::OpenGL33));
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK(device->Api() == GraphicsApi::OpenGL33);
    CHECK(device->Limits().maximumNumberOfVertexAttributes >= 16);
    CHECK(device->Limits().numberOfTextureUnits >= 48);
    CHECK(device->Limits().maximumNumberOfColorAttachments >= 8);
}

TEST_CASE("CreateDevice throws for an API that was not compiled in") {
    if (IsGraphicsApiAvailable(GraphicsApi::Direct3D11)) {
        MESSAGE("Direct3D 11 is compiled in; nothing to check");
        return;
    }
    CHECK_THROWS_AS(CreateDevice(GraphicsApi::Direct3D11), std::runtime_error);
}

TEST_CASE("A hidden window reports its size and owns a context for its device") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);

    // Framebuffer pixels: 64 x 32, or larger on a high-DPI display.
    CHECK(window->Width() >= 64);
    CHECK(window->Height() >= 32);
    CHECK(&window->GetContext().GetDevice() == device.get());
    CHECK_FALSE(window->ShouldClose());
}

TEST_CASE("CreateGraphicsWindow rejects sizes that are not positive") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK_THROWS_AS(device->CreateGraphicsWindow(0, 32, "bad"), std::invalid_argument);
    CHECK_THROWS_AS(device->CreateGraphicsWindow(32, 0, "bad"), std::invalid_argument);
    CHECK_THROWS_AS(device->CreateGraphicsWindow(-1, 32, "bad"), std::invalid_argument);
}

TEST_CASE("A frame can be run by hand on a hidden window") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(16, 16, "test", WindowType::Hidden);

    // The pieces of Run(), without the loop.
    window->GetContext().MakeCurrent();
    window->PollEvents();
    window->SwapBuffers();
    CHECK_FALSE(window->ShouldClose());
}

TEST_CASE("One device can own several windows, each with its own context") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto first = device->CreateGraphicsWindow(16, 16, "first", WindowType::Hidden);
    auto second = device->CreateGraphicsWindow(16, 16, "second", WindowType::Hidden);

    CHECK(&first->GetContext() != &second->GetContext());

    first.reset();   // closing one window must not affect the other
    second->GetContext().MakeCurrent();
    second->PollEvents();
    second->SwapBuffers();
    CHECK_FALSE(second->ShouldClose());
}

TEST_CASE("Two devices can be alive at the same time") {
    auto first = CreateDevice(GraphicsApi::OpenGL33);
    auto second = CreateDevice(GraphicsApi::OpenGL33);

    first.reset();   // GLFW must stay initialized for the second device
    auto window = second->CreateGraphicsWindow(16, 16, "test", WindowType::Hidden);
    CHECK(window->Width() >= 16);
}
```

What each test covers:

| Test | Checks |
|---|---|
| Limits | glad loaded (otherwise `glGetIntegerv` would crash) and the spec minimums |
| Not compiled in | the `throw` at the end of `CreateDevice` |
| Hidden window | framebuffer size, the context's device reference |
| Rejects sizes | the NVI validation in `Device::CreateGraphicsWindow` |
| Frame by hand | `MakeCurrent`, `PollEvents`, `SwapBuffers` on a real context |
| Several windows | the share window as `share` argument, and the destructor keeping a context current |
| Two devices | `GlfwLibrary` reference counting: without it, destroying `first` would call `glfwTerminate` and the next `glfwCreateWindow` would fail |

About doctest: `CHECK` records a failure and continues; `REQUIRE` stops the
test case on failure (use it when later lines would crash). `CHECK_THROWS_AS(expr, type)`
passes if `expr` throws `type` or something derived from it. Each
`TEST_CASE` runs separately, so every one creates its own device, and each
device initializes and terminates GLFW. That also checks that GLFW can be
initialized again after termination.

### Milestone, part 2: run the tests

```sh
./run.sh -t arda_tests
```

All test cases should pass, including the existing `EllipsoidTests`.

> **Why these tests won't run on CI:** they create real GL contexts, so they
> need a GPU driver and a desktop session. A headless build agent (or a
> Windows service, or a Remote Desktop session on some older drivers) can't
> create one, and every renderer test fails at `CreateDevice`. If you add CI
> later, split the renderer tests into their own executable (say
> `arda_renderer_tests`) and run only the core tests on the build agent.

---

## What happens at startup and shutdown

Now that every piece exists, here is the whole sequence for `main.cpp`. Use it
with a debugger: set a breakpoint on each line and step through.

**Startup:**

1. `CreateDevice(OpenGL33)` → `std::make_unique<gl::DeviceGL3x>()`.
2. `Device` base constructed (`m_limits` zeroed).
3. `m_glfw` constructed → `GlfwLibrary()` → first user, so
   `glfwSetErrorCallback` and `glfwInit()`.
4. `DeviceGL3x` body: `ApplyContextHints(Hidden)`, then `glfwCreateWindow(1, 1, ...)`
   creates the hidden share window and **context #1**.
5. `glfwMakeContextCurrent(share)` → context #1 is current.
6. `gladLoadGLLoader` fills every GL function pointer.
7. `glGetIntegerv` ×3 → `SetLimits`.
8. `device->CreateGraphicsWindow(1280, 720, "Arda")` → size check →
   `DoCreateGraphicsWindow` → `make_unique<GraphicsWindowGL3x>`.
9. `ApplyContextHints(Default)`, then `glfwCreateWindow(..., share)` creates the
   visible window and **context #2**, sharing with #1.
10. User pointer, framebuffer size callback, `glfwGetFramebufferSize`.
11. `glfwMakeContextCurrent(window)` → context #2 is current. `glfwSwapInterval(1)`.
12. `ContextGL3x` constructed → `glViewport(0, 0, 1280, 720)` on context #2.
13. `main` sets the handlers.
14. `Run()` → `MakeCurrent()` (context #2) → `OnResize()` → the resize
    handler prints the size.
15. Loop: `glfwPollEvents` → (maybe the callback sets `m_resizePending` →
    `OnResize`) → update handler (none) → render handler → `glfwSwapBuffers`
    (waits for vsync).

**Shutdown** (after the close button):

1. `glfwWindowShouldClose` returns true, and `Run` returns.
2. The `try` block ends, and `window` is destroyed first:
   `~GraphicsWindowGL3x` → context #2 was current → `m_context.reset()` →
   `glfwDestroyWindow` (context #2 gone) → `glfwMakeContextCurrent(share)`
   (context #1 current). The `GraphicsWindow` base is destroyed (the
   handlers are released).
3. `device` is destroyed: `~DeviceGL3x` → `glfwDestroyWindow(share)` (context
   #1 gone). `m_glfw` is destroyed → last user → `glfwTerminate()`. The
   `Device` base is destroyed.

In later steps, resources (buffers, programs, textures) are declared after
the window in `main`, so they are destroyed before it, while context #2 is
still current.

---

## Common problems

| Symptom | Cause and fix |
|---|---|
| `fatal: DeviceGL3x: could not create an OpenGL 3.3 core context` plus a GLFW error line | The driver can't make a 3.3 core context. Update the GPU driver. In a VM or over some Remote Desktop setups, only GL 1.1 (Microsoft's software renderer) is available. |
| Crash with an access violation at address `0x00000000` on the first GL call | glad wasn't loaded, or no context was current when it was loaded. Check the order in `DeviceGL3x`'s constructor. Also check that the call isn't a GL 4.x function (null on a 3.3 context). |
| Crash or GL error in a destructor at the end of `main` | No context is current, or the device was destroyed before a window or resource. Declare the device first in `main`. |
| "OpenGL header already included, remove this include, glad already provides it" | `<GLFW/glfw3.h>` came before `<glad/glad.h>` in that `.cpp`. |
| C2027 / "can't delete an incomplete type `GraphicsWindow`" | The file destroying the window doesn't include `<arda/renderer/GraphicsWindow.h>`. See [forward declarations](#cpp-forward-declarations). |
| `LNK2019 unresolved external symbol ... CreateDevice` | `src/Device.cpp` is empty or missing from `renderer/CMakeLists.txt`. |
| `CreateDevice: this graphics API was not compiled in` for OpenGL | `ARDA_HAS_GL` isn't defined: reconfigure with `-DARDA_RENDERER_GL=ON`. |
| The window shows black or garbage | Expected in Step 0. Nothing clears the back buffer until Step 1. |
| The image only fills a quarter of the window (macOS) | Something used the window size for the viewport instead of the framebuffer size. |
| Frame count far above the refresh rate | The driver overrides vsync (check the GPU control panel), or `glfwSwapInterval` was called with no current context. |
| A compile error mentioning `CreateWindowW` | `<windows.h>` got included and something is named `CreateWindow`. Use `CreateGraphicsWindow`. |

---

## D3D11 check

How Step 8 implements the same interfaces, to confirm nothing GL-specific
leaked into the public headers:

- **`DeviceD3D11`** holds the `ID3D11Device` and its immediate context, and
  fills `DeviceLimits` from D3D11 constants with `SetLimits`. It has a
  `GlfwLibrary m_glfw` member too, since its windows are GLFW windows. There's
  no share window: all D3D11 resources belong to the device, not to a context.
- **`GraphicsWindowD3D11`** uses the same GLFW calls, but with
  `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`, so GLFW creates no GL
  context. It creates a DXGI swap chain for the window's `HWND` from
  `glfwGetWin32Window`. `SwapBuffers` calls `IDXGISwapChain::Present(1, 0)`,
  where `1` is vsync, like `glfwSwapInterval(1)`. On resize it must release
  its back-buffer views and call `ResizeBuffers`. The deferred-resize pattern
  from Part 10 (record the size in the callback, act on it in `PollEvents`)
  applies there too.
- **`ContextD3D11::MakeCurrent`** records which window is active, since D3D11
  has one device context for all windows.
- **Shared GLFW code:** `GlfwLibrary`, the user-pointer callback technique and
  the framebuffer-size handling are the same.
- **Nothing in the public headers changes.** `GraphicsApi::Direct3D11`,
  `IsGraphicsApiAvailable` and the `CreateDevice` `case` are already in place.
  Step 8 only adds the `#if ARDA_HAS_D3D11` branch in `Device.cpp`.

---

## Checklist

- [ ] CMake: `renderer/CMakeLists.txt` and `tests/CMakeLists.txt` list every Step 0 file (they already do)
- [ ] Public headers: `GraphicsApi.h`, `Device.h`, `GraphicsWindow.h` (the old `Window` class replaced), `Context.h`
- [ ] `src/Context.cpp`, `src/GraphicsWindow.cpp` (old `Window` code replaced), `src/GlfwLibrary.h` / `.cpp`
- [ ] **Checkpoint 1:** `arda_renderer` builds
- [ ] `src/gl/GLHandle.h`
- [ ] `DeviceGL3x`: share window, context hints, glad, limits
- [ ] `GraphicsWindowGL3x`: shared context, user pointer, deferred resize, destructor order
- [ ] `ContextGL3x`: `MakeCurrent`, initial viewport
- [ ] **Checkpoint 2:** `arda_renderer` builds with the GL backend
- [ ] `src/Device.cpp`: `CreateGraphicsWindow`, `IsGraphicsApiAvailable`, `CreateDevice`
- [ ] **Checkpoint 3:** `arda_renderer` builds completely
- [ ] `Window.h` is gone and `git grep -n "Window.h\|renderer::Window"` finds nothing outside the docs
- [ ] `scene/src/main.cpp` uses `CreateDevice` and `Run`
- [ ] `tests/src/renderer/DeviceTests.cpp`
- [ ] **Milestone:** the window opens, prints its framebuffer size, closes cleanly with exit code 0, and `arda_tests` passes

---

## C++ and OpenGL notes index

Later guides link to these instead of explaining them again.

**C++ notes**

| Topic | Link |
|---|---|
| `#pragma once` and include guards | [`#cpp-pragma-once`](#cpp-pragma-once) |
| Nested namespaces | [`#cpp-nested-namespaces`](#cpp-nested-namespaces) |
| `enum class` | [`#cpp-enum-class`](#cpp-enum-class) |
| Forward declarations vs includes | [`#cpp-forward-declarations`](#cpp-forward-declarations) |
| `class` vs `struct` | [`#cpp-class-vs-struct`](#cpp-class-vs-struct) |
| `virtual`, pure virtual, abstract classes | [`#cpp-virtual-functions`](#cpp-virtual-functions) |
| `const` member functions | [`#cpp-const-member-functions`](#cpp-const-member-functions) |
| Virtual destructors | [`#cpp-virtual-destructors`](#cpp-virtual-destructors) |
| Deleted copy operations, rule of 0/3/5 | [`#cpp-deleted-copy`](#cpp-deleted-copy) |
| Protected constructors | [`#cpp-protected-constructors`](#cpp-protected-constructors) |
| Non-virtual interface pattern | [`#cpp-nvi`](#cpp-nvi) |
| `std::unique_ptr`, `std::make_unique` | [`#cpp-unique-ptr`](#cpp-unique-ptr) |
| `std::function` and lambdas | [`#cpp-std-function-lambdas`](#cpp-std-function-lambdas) |
| Reference members | [`#cpp-reference-members`](#cpp-reference-members) |
| `explicit` constructors | [`#cpp-explicit`](#cpp-explicit) |
| RAII | [`#cpp-raii`](#cpp-raii) |
| Anonymous namespaces, internal linkage | [`#cpp-anonymous-namespaces`](#cpp-anonymous-namespaces) |
| `std::mutex`, `std::lock_guard`, CTAD | [`#cpp-mutex`](#cpp-mutex) |
| Exceptions | [`#cpp-exceptions`](#cpp-exceptions) |
| Move semantics, `std::exchange`, `noexcept` | [`#cpp-move-semantics`](#cpp-move-semantics) |
| Class templates, function-object deleters, `using` aliases | [`#cpp-class-templates`](#cpp-class-templates) |
| `override` and `final` | [`#cpp-override-final`](#cpp-override-final) |
| Member initialization and destruction order | [`#cpp-member-order`](#cpp-member-order) |
| Preprocessor `#if` for build configuration | [`#cpp-preprocessor`](#cpp-preprocessor) |
| C callbacks with user pointers | [`#cpp-c-callbacks`](#cpp-c-callbacks) |
| `static_cast` (and the other casts) | [`#cpp-static-cast`](#cpp-static-cast) |

**OpenGL concepts**

| Topic | Link |
|---|---|
| The state machine | [`#the-opengl-state-machine`](#the-opengl-state-machine) |
| Object names and binding points | [`#object-names`](#object-names) |
| Contexts | [`#contexts`](#contexts) |
| Shared contexts and the share window | [`#shared-contexts-and-the-hidden-share-window`](#shared-contexts-and-the-hidden-share-window) |
| glad | [`#loading-gl-functions-with-glad`](#loading-gl-functions-with-glad) |
| Core profile, forward compatibility | [`#core-profile-and-forward-compatibility`](#core-profile-and-forward-compatibility) |
| Double buffering and vsync | [`#double-buffering-and-vsync`](#double-buffering-and-vsync) |
| Framebuffer size vs window size | [`#framebuffer-size-vs-window-size`](#framebuffer-size-vs-window-size) |
| The render loop | [`#the-render-loop`](#the-render-loop) |
