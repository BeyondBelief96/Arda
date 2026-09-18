# Step 8: Direct3D 11 backend

**Goal:** implement `src/d3d11/` behind the same public headers, so that
every earlier milestone runs with `CreateDevice(GraphicsApi::Direct3D11)`.
You build it in the same order as Steps 0–6 and check each milestone on the
way: an empty window, a clear color, the shader tests, the first triangle,
the triangle through a camera, the textured triangle, render-to-texture,
and finally Step 7's `Triangle` example with one argument changed.

**Read again:** 3.5.5, 3.6.5, 3.7.2, and the "false sense of portability"
note in 3.1.

OpenGlobe has no D3D implementation. This step is arda's own: it proves the
abstraction by implementing every interface a second time, on an API with a
very different shape. OpenGlobe's `GL3x` folder is still the structural
model, because every GL class you wrote has a D3D11 twin with the same job.
Microsoft's documentation is the reference for the API calls:

- [Direct3D 11 graphics](https://learn.microsoft.com/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
- [DXGI flip model](https://learn.microsoft.com/windows/win32/direct3ddxgi/dxgi-flip-model)
- [`D3DCompile`](https://learn.microsoft.com/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile)
- [`ID3D11ShaderReflection`](https://learn.microsoft.com/windows/win32/api/d3d11shader/nn-d3d11shader-id3d11shaderreflection)
- [Packing rules for constant variables](https://learn.microsoft.com/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
- [Using the debug layer](https://learn.microsoft.com/windows/win32/direct3d11/using-the-debug-layer-to-test-apps)

## How to read this guide

- **Callouts.** Three kinds appear throughout:
  - `> **D3D11 note — <topic>:**` explains how Direct3D 11 works and how it
    differs from what you did in GL.
  - `> **Why:**` explains a design decision, usually one row of the
    [README's portability table](../README.md#portability-decisions).
  - `> **C++ note — <topic>:**` explains a language or library feature.
    Features covered in earlier guides get a one-line reminder with a link
    (the [index in Step 0](00-setup.md#c-and-opengl-notes-index) lists them).
- **Build order.** The public headers are finished; you wrote them in
  Steps 0–6. So `DeviceD3D11` and `ContextD3D11` must override *every* pure
  virtual function before anything compiles. Step 0 below therefore gives you
  the final headers, plus `.cpp` files where the later functions are
  temporary stubs. Each later section replaces stubs with the real
  functions, in full, and says which `#include` lines to add.
- **One public change.** `Context` gains `ClipSpaceTransform()` for the
  render-to-texture flip. You make that change first, in
  [Before you start](#before-you-start-the-one-public-change), and prove with
  the GL tests that it changes nothing on GL. Everything else lives in
  `src/d3d11/`.

---

## The Direct3D 11 mental model

Before any code, here is how the pieces of D3D11 line up with what you
already built for GL.

| Concept | OpenGL 3.3 | Direct3D 11 | arda |
|---|---|---|---|
| Creates resources | The current context | `ID3D11Device` (no "current", callable from any thread) | `Device` |
| Issues commands, holds bindings | The current context | `ID3D11DeviceContext` (the *immediate* context) | `Context` |
| Window surface | Default framebuffer, `glfwSwapBuffers` | DXGI swap chain, `Present` | `GraphicsWindow` |
| Pipeline state | A global state machine, one `glEnable` at a time | Immutable state *objects*, created up front | `RenderState` |
| Object handle | `GLuint` name | COM interface pointer | `GLHandle` / `ComPtr` |
| Where a draw writes | Framebuffer object | Render target and depth-stencil *views* | `Framebuffer` |
| Vertex input binding | Attribute location | Input layout + semantic | `VertexArray` |
| Loose uniforms | `glUniform*` | The `$Globals` constant buffer | `Uniform<T>` |
| Texture binding | Texture unit N | Shader resource view in `tN` + sampler in `sN` | `TextureUnits` |
| Error reporting | `glGetError`, debug output | `HRESULT` + the debug layer | exceptions |

> **D3D11 note — device vs. immediate context:** GL has one object, the
> context, that both creates resources and draws. D3D11 splits it in two.
> `ID3D11Device` creates buffers, textures, shaders and state objects. It is
> free-threaded, and nothing has to be "current" to use it.
> `ID3D11DeviceContext` holds the pipeline bindings (which shaders, which
> buffers, which render targets) and issues `Draw` and `Clear`. Each device
> has exactly one *immediate* context, whose commands go to the GPU. (There
> are also *deferred* contexts for recording command lists on other threads;
> arda doesn't need them.) This is the split the book's `Device`/`Context`
> design copies (3.2), so the mapping is direct: `DeviceD3D11` owns an
> `ID3D11Device`, and each `ContextD3D11` issues commands through the
> immediate context.

> **D3D11 note — one immediate context, many windows:** in GL, each window
> has its own context with its own copy of the state, which is why
> `ContextGL3x` keeps its own cached `RenderState`. In D3D11, every window
> shares the device's single immediate context. `ContextD3D11` still caches
> what it applied, so it can skip redundant calls, but it must throw that
> cache away whenever a *different* `ContextD3D11` has used the immediate
> context since. The device remembers which context used it last (the
> "active" context) for exactly this reason.

---

## Files

```
renderer/
  CMakeLists.txt                                   UPDATE  D3D11 sources, libraries, definitions
  include/arda/renderer/Context.h                  UPDATE  ClipSpaceTransform, FlipsRenderTargetY
  src/
    Context.cpp                                    UPDATE  ClipSpaceTransform
    Device.cpp                                     UPDATE  CreateDevice for Direct3D11
    shaders/automaticuniforms/
      ModelViewPerspectiveMatrixUniform.h          UPDATE  applies ClipSpaceTransform
      StandardDrawAutomaticUniforms.cpp            UPDATE  applies ClipSpaceTransform; adds og_clipSpaceFlipY
    d3d11/
      D3D11Common.h                                NEW     Windows header setup, ComPtr, ThrowIfFailed, strings, MappedSubresource
      TypeConverterD3D11.h / .cpp                  NEW     every arda enum to its D3D11/DXGI value
      DeviceD3D11.h / .cpp                         NEW     Step 0
      GraphicsWindowD3D11.h / .cpp                 NEW     Step 0: swap chain, back buffer, window depth buffer
      ContextD3D11.h / .cpp                        NEW     Steps 0, 1, 3, 5, 6
      StateCacheD3D11.h / .cpp                     NEW     Step 1: depth-stencil, blend and rasterizer state objects
      shaders/HlslPrelude.h / .cpp                 NEW     Step 2
      shaders/ConstantBufferD3D11.h / .cpp         NEW     Step 2: CPU copy of one stage's $Globals
      shaders/UniformD3D11.h / .cpp                NEW     Step 2
      shaders/ShaderProgramD3D11.h / .cpp          NEW     Step 2
      buffers/BufferD3D11.h / .cpp                 NEW     Step 3
      buffers/VertexBufferD3D11.h                  NEW     Step 3
      buffers/IndexBufferD3D11.h                   NEW     Step 3
      vertexarray/VertexArrayD3D11.h / .cpp        NEW     Step 3: buffer binding and the input layout cache
      buffers/PixelBuffersD3D11.h                  NEW     Step 5: WritePixelBufferD3D11, ReadPixelBufferD3D11
      textures/PixelConversionD3D11.h / .cpp       NEW     Step 5: pixel-buffer data <-> DXGI texel layout
      textures/Texture2DD3D11.h / .cpp             NEW     Step 5
      textures/TextureSamplerD3D11.h / .cpp        NEW     Step 5
      framebuffer/FramebufferD3D11.h / .cpp        NEW     Step 6
tests/
  CMakeLists.txt                                   UPDATE
  src/renderer/DeviceTests.cpp                     UPDATE  Step 0 on D3D11: a Direct3D 11 device test
  src/renderer/HlslShaderTests.cpp                 NEW     Step 2 on D3D11
  src/renderer/TestApis.h                          NEW     AvailableApis(), for running tests on every backend
  src/renderer/CrossApiTests.cpp                   NEW     every backend renders the same pixels
```

`scene/src/main.cpp` needs no change: Step 7 already picks the API from the
command line (`arda_scene d3d11`).

---

## Before you start: the one public change

Direct3D and GL disagree about which way is up *in a render target*. The full
explanation is in [Step 4 on D3D11](#step-4-on-d3d11-the-render-target-y-flip);
the fix is to flip clip-space y while a framebuffer is bound on D3D11. Every
automatic uniform that produces clip coordinates needs to know whether to
flip, so `Context` gets a public function that returns the flip as a matrix,
and a protected virtual that backends override. Make the change now, while
only the GL backend exists: the GL tests prove it changes nothing there.

### `include/arda/renderer/Context.h`

Add, in the `public:` section after the Step 6 framebuffer functions:

```cpp
    // Step 8: multiply this into every matrix that produces clip coordinates.
    // The identity, except on D3D11 while a framebuffer is bound, where it
    // flips y so render targets end up the same way up as on GL.
    core::Matrix4<double> ClipSpaceTransform() const;
```

and in the `protected:` section:

```cpp
    // Step 8: true while clip-space y must be flipped. GL never flips.
    virtual bool FlipsRenderTargetY() const { return false; }
```

`Context.h` already includes `SceneState.h`, which includes `Matrix4.h`, so
no new include is needed.

### `src/Context.cpp`

```cpp
core::Matrix4<double> Context::ClipSpaceTransform() const {
    if (!FlipsRenderTargetY()) {
        return core::Matrix4<double>::Identity();
    }
    return core::Matrix4<double>(1.0,  0.0, 0.0, 0.0,
                                 0.0, -1.0, 0.0, 0.0,
                                 0.0,  0.0, 1.0, 0.0,
                                 0.0,  0.0, 0.0, 1.0);
}
```

### `src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h`

Change `Set`:

```cpp
    void Set(Context& context, const DrawState&, const SceneState& sceneState) override {
        const core::ClipDepth clipDepth = context.GetDevice().ClipDepthRange();
        // Multiply in double, convert once at the end.
        m_uniform.SetValue((context.ClipSpaceTransform() * sceneState.ModelViewPerspectiveMatrix(clipDepth)).Cast<float>());
    }
```

### `src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.cpp`

Every other automatic uniform that produces clip coordinates gets the same
treatment. Add a helper to the anonymous namespace, next to `ClipDepthOf`:

```cpp
// Step 8: the flip for render targets on D3D11 (identity on GL), applied to
// a matrix whose result is in clip coordinates.
Matrix4F ToClip(const Context& context, const Matrix4D& matrix) {
    return (context.ClipSpaceTransform() * matrix).Cast<float>();
}
```

and replace these five registrations, and the viewport transformation, in
`AddStandardDrawAutomaticUniformFactories`:

```cpp
    Register<Matrix4F>(factories, "og_perspectiveMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return ToClip(c, s.PerspectiveMatrix(ClipDepthOf(c)));
        });
    Register<Matrix4F>(factories, "og_orthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return ToClip(c, s.OrthographicMatrix(ClipDepthOf(c)));
        });
    Register<Matrix4F>(factories, "og_modelViewOrthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return ToClip(c, s.ModelViewOrthographicMatrix(ClipDepthOf(c)));
        });
    Register<Matrix4F>(factories, "og_modelViewPerspectiveMatrixRelativeToEye",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return ToClip(c, s.ModelViewPerspectiveMatrixRelativeToEye(ClipDepthOf(c)));
        });
    Register<Matrix4F>(factories, "og_viewportOrthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState&) {
            return ToClip(c, SceneState::ComputeViewportOrthographicMatrix(c.GetViewport(), ClipDepthOf(c)));
        });
    Register<Matrix4F>(factories, "og_viewportTransformationMatrix",
        [](const Context& c, const DrawState& d, const SceneState&) {
            const DepthRange& depthRange = d.renderState.depthRange;
            // NDC -> window: undo the flip first (it is its own inverse), so the
            // result is arda's bottom-left window coordinates on both APIs.
            return (SceneState::ComputeViewportTransformationMatrix(
                        c.GetViewport(), depthRange.nearValue, depthRange.farValue, ClipDepthOf(c)) *
                    c.ClipSpaceTransform()).Cast<float>();
        });
```

and register one new uniform, for shaders that write clip coordinates
without any of the matrices above (a fullscreen quad drawn into a
framebuffer, for example):

```cpp
    // Step 8: 1 normally, -1 while clip-space y is flipped. Multiply it into
    // gl_Position.y / SV_Position.y when a shader builds clip coordinates itself.
    Register<float>(factories, "og_clipSpaceFlipY",
        [](const Context& c, const DrawState&, const SceneState&) {
            return static_cast<float>(c.ClipSpaceTransform()(1, 1));
        });
```

> **Why the matrix goes on the left, except for one:** a clip-space matrix
> `M` maps something to clip coordinates, so the flip has to happen *after*
> it: `F · M`. `og_viewportTransformationMatrix` goes the other way (from
> NDC to window coordinates), so the flip has to be undone *before* it:
> `V · F`. The flip matrix `F` is its own inverse, which is why the same
> matrix works for both.

> **Why a function on `Context` and not a flag in each uniform:** whether
> to flip depends on the backend *and* on context state (the current
> framebuffer). The automatic uniforms already receive the `Context`. A
> single public function keeps the backends' knowledge in the backends: the
> uniforms never ask "am I on D3D11?", they just multiply by whatever the
> context returns.

**Checkpoint:** build with only the GL backend and run `arda_tests`. Every
test still passes, because `ClipSpaceTransform()` is the identity on GL.

---

## CMake

`renderer/CMakeLists.txt`: fill in the `ARDA_RENDERER_D3D11` block.

```cmake
if(ARDA_RENDERER_D3D11)
    if(NOT WIN32)
        message(FATAL_ERROR "ARDA_RENDERER_D3D11 requires Windows")
    endif()
    target_sources(arda_renderer PRIVATE
        src/d3d11/TypeConverterD3D11.cpp
        src/d3d11/DeviceD3D11.cpp
        src/d3d11/GraphicsWindowD3D11.cpp
        src/d3d11/ContextD3D11.cpp
        src/d3d11/StateCacheD3D11.cpp
        src/d3d11/shaders/HlslPrelude.cpp
        src/d3d11/shaders/ConstantBufferD3D11.cpp
        src/d3d11/shaders/UniformD3D11.cpp
        src/d3d11/shaders/ShaderProgramD3D11.cpp
        src/d3d11/buffers/BufferD3D11.cpp
        src/d3d11/vertexarray/VertexArrayD3D11.cpp
        src/d3d11/textures/PixelConversionD3D11.cpp
        src/d3d11/textures/Texture2DD3D11.cpp
        src/d3d11/textures/TextureSamplerD3D11.cpp
        src/d3d11/framebuffer/FramebufferD3D11.cpp
        # Headers, listed for the IDE's project tree only.
        src/d3d11/D3D11Common.h
        src/d3d11/TypeConverterD3D11.h
        src/d3d11/DeviceD3D11.h
        src/d3d11/GraphicsWindowD3D11.h
        src/d3d11/ContextD3D11.h
        src/d3d11/StateCacheD3D11.h
        src/d3d11/shaders/HlslPrelude.h
        src/d3d11/shaders/ConstantBufferD3D11.h
        src/d3d11/shaders/UniformD3D11.h
        src/d3d11/shaders/ShaderProgramD3D11.h
        src/d3d11/buffers/BufferD3D11.h
        src/d3d11/buffers/VertexBufferD3D11.h
        src/d3d11/buffers/IndexBufferD3D11.h
        src/d3d11/buffers/PixelBuffersD3D11.h
        src/d3d11/vertexarray/VertexArrayD3D11.h
        src/d3d11/textures/PixelConversionD3D11.h
        src/d3d11/textures/Texture2DD3D11.h
        src/d3d11/textures/TextureSamplerD3D11.h
        src/d3d11/framebuffer/FramebufferD3D11.h
    )
    # d3d11:       D3D11CreateDevice and the ID3D11* interfaces
    # dxgi:        swap chains and adapters (IDXGIFactory2, IDXGISwapChain1)
    # d3dcompiler: D3DCompile and D3DReflect (loads d3dcompiler_47.dll, part of Windows 10 and later)
    # dxguid:      interface and GUID definitions, e.g. IID_ID3D11ShaderReflection, WKPDID_D3DDebugObjectName
    target_link_libraries(arda_renderer PRIVATE d3d11 dxgi d3dcompiler dxguid)
    target_compile_definitions(arda_renderer PRIVATE
        ARDA_HAS_D3D11=1
        WIN32_LEAN_AND_MEAN   # leave rarely used APIs (sockets, COM automation, ...) out of <windows.h>
        NOMINMAX              # stop <windows.h> from defining min and max as macros
    )
endif()
```

Add the sources in the order you write them if you prefer to compile after
every file; the list above is the finished state.

Configure with `-DARDA_RENDERER_D3D11=ON`, or add a preset to
`CMakePresets.json` that sets the cache variable:

```json
{
    "name": "windows-d3d11",
    "inherits": "windows-debug",
    "cacheVariables": { "ARDA_RENDERER_D3D11": "ON" }
}
```

(`inherits` names whatever your existing Windows debug preset is called.)
Keep `ARDA_RENDERER_GL` on: both backends compile into one library, which is
what lets the tests compare them.

`tests/CMakeLists.txt`: add the two new test files.

```cmake
add_executable(arda_tests
    # ...existing files...
    src/renderer/HlslShaderTests.cpp
    src/renderer/CrossApiTests.cpp
)
```

> **Why `PRIVATE`:** exactly as with glad and GLFW (see the README's
> [Why glad, GLFW and D3D headers are private](../README.md#why-glad-glfw-and-d3d-headers-are-private)),
> only `arda_renderer`'s own sources see `<d3d11.h>`. The scene and the tests
> never include a Windows header, so `min`, `max`, `near`, `far` and
> `CreateWindow` can't leak into them. The system libraries are linked
> `PRIVATE` too; CMake still adds them to the final link of every executable
> that uses the static `arda_renderer`.

## `src/Device.cpp`

Replace the Step 8 placeholder in `CreateDevice`:

```cpp
#if ARDA_HAS_GL
#include "gl/DeviceGL3x.h"
#endif
#if ARDA_HAS_D3D11
#include "d3d11/DeviceD3D11.h"
#endif

// ...

std::unique_ptr<Device> CreateDevice(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::OpenGL33:
#if ARDA_HAS_GL
        return std::make_unique<gl::DeviceGL3x>();
#else
        break;
#endif
    case GraphicsApi::Direct3D11:
#if ARDA_HAS_D3D11
        return std::make_unique<d3d11::DeviceD3D11>();
#else
        break;
#endif
    }
    throw std::runtime_error("CreateDevice: this graphics API was not compiled in");
}
```

`IsGraphicsApiAvailable` already checks `ARDA_HAS_D3D11` (Step 0).

---

## `src/d3d11/D3D11Common.h`

Every D3D11 source includes this header first. It sets up the Windows
headers and holds the small helpers every other file uses.

```cpp
#pragma once

#ifndef _WIN32
#error "The Direct3D 11 backend only builds on Windows."
#endif

// CMake defines these for the whole target. The fallbacks keep this header
// correct if it is ever compiled some other way. They must come before the
// first #include of <windows.h> in the translation unit.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace arda::renderer::d3d11 {

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// The system's text for an HRESULT, e.g. "The parameter is incorrect."
inline std::string HResultMessage(HRESULT hr) {
    char* buffer = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(hr), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),   // with ALLOCATE_BUFFER, this argument is really a char**
        0, nullptr);
    std::string message = length != 0 ? std::string(buffer, length) : std::string("unknown error");
    LocalFree(buffer);   // LocalFree(nullptr) is harmless
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' ')) {
        message.pop_back();
    }
    return message;
}

// Thrown by ThrowIfFailed. Keeps the HRESULT so callers can test for specific codes.
class D3D11Error : public std::runtime_error {
public:
    D3D11Error(HRESULT hr, std::string_view what)
        : std::runtime_error(std::format("{} failed: {} (HRESULT 0x{:08X})",
                                         what, HResultMessage(hr), static_cast<std::uint32_t>(hr))),
          m_result(hr) {}

    HRESULT Result() const { return m_result; }

private:
    HRESULT m_result;
};

inline void ThrowIfFailed(HRESULT hr, std::string_view what) {
    if (FAILED(hr)) {
        throw D3D11Error(hr, what);
    }
}

// UTF-8 (what arda uses everywhere) to UTF-16 (what Windows' "W" functions take).
inline std::wstring Utf8ToWide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                           text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length == 0) {
        throw std::invalid_argument("Utf8ToWide: the text is not valid UTF-8");
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

// UTF-16 to UTF-8, e.g. for DXGI_ADAPTER_DESC::Description.
inline std::string WideToUtf8(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           text.data(), static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    if (length == 0) {
        throw std::invalid_argument("WideToUtf8: the text is not valid UTF-16");
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

// Names an object for the debug layer, PIX and RenderDoc. Does nothing in release builds.
inline void SetDebugName([[maybe_unused]] ID3D11DeviceChild* object, [[maybe_unused]] std::string_view name) {
#ifndef NDEBUG
    if (object != nullptr) {
        object->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.data());
    }
#endif
}

// Maps a resource for CPU access, and unmaps it when the scope ends, even if
// an exception is thrown in between.
class MappedSubresource {
public:
    MappedSubresource(ID3D11DeviceContext* context, ID3D11Resource* resource, UINT subresource, D3D11_MAP mapType)
        : m_context(context), m_resource(resource), m_subresource(subresource) {
        ThrowIfFailed(context->Map(resource, subresource, mapType, 0, &m_mapped), "ID3D11DeviceContext::Map");
    }

    ~MappedSubresource() { m_context->Unmap(m_resource, m_subresource); }

    MappedSubresource(const MappedSubresource&)            = delete;
    MappedSubresource& operator=(const MappedSubresource&) = delete;

    std::byte* Data() const { return static_cast<std::byte*>(m_mapped.pData); }
    UINT RowPitch() const { return m_mapped.RowPitch; }   // bytes from one texture row to the next

private:
    ID3D11DeviceContext* m_context;
    ID3D11Resource* m_resource;
    UINT m_subresource;
    D3D11_MAPPED_SUBRESOURCE m_mapped{};
};

} // namespace arda::renderer::d3d11
```

<a id="cpp-windows-h"></a>

> **C++ note — `_WIN32`, `WIN32_LEAN_AND_MEAN` and `NOMINMAX`:** `<windows.h>`
> is a C header from before namespaces, and it defines hundreds of macros.
> Two of them break ordinary C++: `min` and `max` are function-like macros,
> so the preprocessor rewrites `std::min(a, b)` and
> `std::numeric_limits<float>::max()` into garbage before the compiler sees
> them. `NOMINMAX` turns them off. `WIN32_LEAN_AND_MEAN` skips rarely used
> subsystems, which makes builds faster and removes more macro names. Both
> must be defined *before* the first `#include <windows.h>` in each `.cpp`
> file, and any header can pull `<windows.h>` in indirectly (`<d3d11.h>` and
> `<GLFW/glfw3native.h>` both do). Setting them with
> `target_compile_definitions` puts them on every compiler command line, so
> the order of includes stops mattering. The `#error` on `_WIN32`, which
> every Windows compiler defines (for 64-bit targets too), turns an
> accidental non-Windows build into one clear message instead of hundreds of
> missing-header errors. (This is the same macro problem that made
> `DepthRange` use `nearValue` and `Device` use `CreateGraphicsWindow`; see
> [Step 1](01-state-management.md#cpp-macro-names).)

<a id="cpp-com"></a>

> **C++ note — COM and reference counting:** every D3D11 and DXGI object is
> a COM object. You never `new` or `delete` one. A factory function
> (`CreateBuffer`, `D3D11CreateDevice`, ...) hands you an *interface pointer*
> whose reference count is already 1. Every COM interface derives from
> `IUnknown`, which has three functions:
> - `AddRef()` increments the count. Call it when you store another copy of
>   the pointer.
> - `Release()` decrements it. The object destroys itself when it reaches 0.
> - `QueryInterface(iid, &out)` asks the object for a *different* interface
>   it also implements, for example an `ID3D11Device` for its `IDXGIDevice`.
>   On success it `AddRef`s and writes the new pointer; on failure it returns
>   `E_NOINTERFACE`. The `iid` is a GUID naming the interface; the
>   `IID_PPV_ARGS(&p)` macro fills in both arguments from `p`'s type.
>
> It is the same idea as `std::shared_ptr`, except that the count lives
> inside the object and you call the functions yourself. Forgetting one
> `Release` leaks the object; one `Release` too many destroys it while it's
> still in use. D3D11 objects also hold an internal reference to their
> device, so releasing a texture after the device's last external reference
> is safe.

<a id="cpp-comptr"></a>

> **C++ note — `Microsoft::WRL::ComPtr`:** `ComPtr<T>` from `<wrl/client.h>`
> is the RAII wrapper that calls `AddRef` and `Release` for you, just as
> `GLHandle` calls `glDelete*` ([Step 0](00-setup.md#cpp-raii)). Copying a
> `ComPtr` `AddRef`s, destroying one `Release`s, and moving one transfers
> ownership with no count change. The functions you will use:
>
> | Function | What it does | Use it for |
> |---|---|---|
> | `Get()` | Returns the raw `T*`, count unchanged | Passing one object to a D3D call |
> | `GetAddressOf()` | Returns `T**` to the stored pointer, **without** releasing it | An *input* array of one: `OMSetRenderTargets(1, rtv.GetAddressOf(), ...)` |
> | `ReleaseAndGetAddressOf()` | Releases the current object, then returns `T**` | *Output* parameters that create a new object |
> | `&ptr` | Same as `ReleaseAndGetAddressOf()` | Output parameters: `CreateBuffer(&desc, nullptr, &buffer)` |
> | `As(&other)` | `QueryInterface` for `other`'s interface type | `IDXGIDevice` from `ID3D11Device` |
> | `Reset()` | Releases and becomes null | Dropping a reference early (swap chain resizing) |
>
> **The `&` pitfall:** `&ptr` *releases* whatever `ptr` held. That is right
> for outputs and a silent bug for inputs:
> `context->OMSetRenderTargets(1, &m_backBufferView, dsv)` compiles, but it
> releases the view (possibly destroying it) and then binds null. Use
> `GetAddressOf()` whenever the D3D function *reads* the pointer.

<a id="cpp-hresult"></a>

> **C++ note — `HRESULT` and `ThrowIfFailed`:** D3D reports errors by
> returning an `HRESULT`, a 32-bit code. Negative values are failures, which
> is what the `FAILED(hr)` macro tests. Zero or positive values are success,
> and some successes carry information: `DXGI_STATUS_OCCLUDED` from `Present`
> means "the window is hidden", not "error". Checking every call by hand is
> noisy and easy to forget, so every call goes through `ThrowIfFailed`,
> which turns a failure into an exception, like the rest of arda's error
> handling ([Step 0](00-setup.md#cpp-exceptions)). The message includes the
> call's name, the system's text for the code (`FormatMessageA` looks it
> up) and the code in hex, so `E_INVALIDARG` shows up as
> `CreateTexture2D failed: The parameter is incorrect. (HRESULT 0x80070057)`.
> The hex code is what you search for when the text is unhelpful. The
> `static_cast<std::uint32_t>` matters: `HRESULT` is a signed `long`, and
> formatting a negative number with `{:08X}` prints `-7FF8FFA9`.
> `D3D11Error` derives from `std::runtime_error`
> ([custom exceptions](02-shaders.md#cpp-custom-exceptions)) and keeps the
> code, so a caller can test `error.Result() == DXGI_ERROR_DEVICE_REMOVED`.

<a id="cpp-wide-strings"></a>

> **C++ note — wide strings:** Windows uses UTF-16 internally. Its API
> functions come in two versions: `FooA` (8-bit "ANSI" code page) and `FooW`
> (UTF-16, `wchar_t`, `std::wstring`). arda stores all text as UTF-8
> `std::string`, so text crossing into or out of a `W` function must be
> converted with `MultiByteToWideChar` / `WideCharToMultiByte`. Both are
> called twice: once with a null output buffer to learn the length, then
> again to fill a string of that size. `MB_ERR_INVALID_CHARS` makes invalid
> input an error instead of silently replacing it. D3D11 itself mostly takes
> `char` strings (`D3DCompile`, semantic names), so this backend needs the
> conversion in only two places: the adapter name in `DXGI_ADAPTER_DESC`,
> which is `WCHAR[128]`, and `D3DCompileFromFile` if you load shaders from
> files, because its path parameter is `LPCWSTR`:
> ```cpp
> const std::wstring path = Utf8ToWide("shaders/triangle.vs.hlsl");
> D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
>                    "main", "vs_5_0", flags, 0, &code, &errors);
> ```
> (`std::filesystem::path::wstring()` does the same conversion for paths.)

> **C++ note — RAII for Map/Unmap:** `MappedSubresource` is the RAII idea
> from [Step 0](00-setup.md#cpp-raii) applied to an *action* instead of an
> object: the constructor maps and the destructor unmaps. If a conversion in
> between throws, the destructor still runs during stack unwinding, so a
> resource is never left mapped (the GPU can't use a mapped resource). It is
> the same kind of scope guard as Step 6's `ScopedRenderTarget`.

---

## Step 0 on D3D11: device, window and swap chain

This step mirrors [Step 0](00-setup.md): a device, a window that owns a
context, and nothing drawn yet. Three classes, and the D3D11 concepts that
come with them: device creation, the debug layer, DXGI swap chains, and
views.

### D3D11 concepts for this step

> **D3D11 note — feature levels:** one D3D11 API runs on hardware of
> several generations. A *feature level* is a guaranteed set of
> capabilities. `D3D_FEATURE_LEVEL_11_0` guarantees geometry shaders,
> shader model 5.0 (`vs_5_0`/`ps_5_0`), 16 samplers and 128 texture slots
> per stage, 8 render targets, 16384×16384 textures, and `R32` index
> buffers. That covers everything GL 3.3 gives you. You pass a list of
> acceptable levels to `D3D11CreateDevice`, and it picks the highest one the
> GPU supports; arda passes only `11_0`, so creation fails cleanly on older
> hardware instead of producing a device that can't compile `vs_5_0`.

> **D3D11 note — the debug layer:** GL reports mistakes through
> `glGetError`, which you must poll. D3D11 returns an `HRESULT` from
> creation calls, but most pipeline calls (`OMSetRenderTargets`, `Draw`,
> ...) return `void` and silently ignore bad arguments. The *debug layer*,
> requested with `D3D11_CREATE_DEVICE_DEBUG`, validates every call and
> prints messages such as
> `D3D11 ERROR: ID3D11DeviceContext::DrawIndexed: Vertex Shader - Pixel Shader linkage error ...`
> to the debugger's Output window. It is the single most useful D3D11
> debugging tool; keep it on in every debug build. It is an optional Windows
> component ("Graphics Tools" under Settings > System > Optional features).
> Without it, device creation with the flag fails with
> `DXGI_ERROR_SDK_COMPONENT_MISSING`, which `DeviceD3D11` handles by trying
> again without the flag. `ID3D11InfoQueue` controls the layer: arda asks it
> to break into the debugger on errors, but only when a debugger is attached
> (a break with no debugger attached crashes the process).

> **D3D11 note — WARP:** `D3D_DRIVER_TYPE_WARP` is Microsoft's software
> rasterizer. It implements feature level 11_0 on the CPU, slowly but
> correctly. `DeviceD3D11` falls back to it when no GPU driver can create a
> device, which lets the tests run in a virtual machine or on a CI runner
> that has no GPU. `AdapterDescription()` then reports
> "Microsoft Basic Render Driver".

### `src/d3d11/DeviceD3D11.h`

This is the Step 0 version. Step 1 adds the state cache (three lines).

```cpp
#pragma once

#include "GlfwLibrary.h"
#include "d3d11/D3D11Common.h"

#include <arda/renderer/Device.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace arda::renderer::d3d11 {

class ContextD3D11;

class DeviceD3D11 final : public Device {
public:
    DeviceD3D11();
    ~DeviceD3D11() override;

    GraphicsApi Api() const override { return GraphicsApi::Direct3D11; }

    ID3D11Device* Native() const { return m_device.Get(); }
    ID3D11DeviceContext* Immediate() const { return m_immediate.Get(); }

    // The GPU's name, for example for a log line. "Microsoft Basic Render Driver" means WARP.
    const std::string& AdapterDescription() const { return m_adapterDescription; }

    // The ContextD3D11 whose state the shared immediate context holds.
    // nullptr means "unknown": every context must re-apply its state.
    ContextD3D11* ActiveContext() const { return m_activeContext; }
    void SetActiveContext(ContextD3D11* context) { m_activeContext = context; }

protected:
    std::unique_ptr<GraphicsWindow> DoCreateGraphicsWindow(
        int width, int height, const std::string& title, WindowType type) override;

    std::shared_ptr<ShaderProgram> DoCreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view geometryShaderSource,
        std::string_view fragmentShaderSource) override;

    std::shared_ptr<VertexBuffer> DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;
    std::shared_ptr<IndexBuffer> DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;

    std::shared_ptr<WritePixelBuffer> DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                               std::size_t sizeInBytes) override;
    std::shared_ptr<Texture2D> DoCreateTexture2D(const Texture2DDescription& description) override;
    std::shared_ptr<TextureSampler> DoCreateTexture2DSampler(const TextureSamplerDescription& description) override;

private:
    GlfwLibrary m_glfw;   // GLFW still creates the windows; declared first so it outlives them
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_immediate;
    std::string m_adapterDescription;
    ContextD3D11* m_activeContext = nullptr;
};

} // namespace arda::renderer::d3d11
```

- **No hidden share window.** `DeviceGL3x` needs a hidden window because a
  GL context can't exist without one, and resources are created "in" the
  current context. `ID3D11Device` needs no window and no "current" state,
  which is exactly what the book's `Device` abstraction wanted in the first
  place (3.2).
- **`GlfwLibrary`** is the same reference-counted `glfwInit`/`glfwTerminate`
  holder as in [Step 0](00-setup.md). A GL device and a D3D11 device can be
  alive together (the cross-API tests do this); the count keeps GLFW
  initialized until both are gone.
- **`Native()` and `Immediate()`** return raw pointers without changing the
  reference count. The device owns the objects; callers only borrow them.

### `src/d3d11/DeviceD3D11.cpp` (starter)

The constructor and destructor are final. The `DoCreate*` functions are
stubs that later steps replace.

```cpp
#include "d3d11/DeviceD3D11.h"
#include "d3d11/GraphicsWindowD3D11.h"

#include <iterator>
#include <stdexcept>
#include <string>

namespace arda::renderer::d3d11 {

namespace {

// One attempt at creating the device and its immediate context.
HRESULT CreateDeviceAndContext(D3D_DRIVER_TYPE driverType, UINT flags,
                               ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& immediate) {
    const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL obtained{};
    return D3D11CreateDevice(
        nullptr,        // the default adapter (the GPU driving the primary monitor)
        driverType,
        nullptr,        // no software rasterizer DLL
        flags,
        featureLevels, static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(), &obtained, immediate.ReleaseAndGetAddressOf());
}

// TEMPORARY: removed once every stub below is replaced.
[[noreturn]] void NotImplementedYet(const char* what) {
    throw std::logic_error(std::string(what) + " is not implemented yet on Direct3D 11");
}

} // namespace

DeviceD3D11::DeviceD3D11() {
    UINT flags = 0;
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_HARDWARE, flags, m_device, m_immediate);
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
        // The debug layer isn't installed. Run without it rather than not at all.
        flags &= ~static_cast<UINT>(D3D11_CREATE_DEVICE_DEBUG);
        hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_HARDWARE, flags, m_device, m_immediate);
    }
    if (FAILED(hr)) {
        // No GPU driver could do feature level 11_0: fall back to the software rasterizer.
        hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_WARP, flags, m_device, m_immediate);
    }
    ThrowIfFailed(hr, "D3D11CreateDevice");

#ifndef NDEBUG
    // Stop in the debugger at the first debug-layer error, at the call that caused it.
    ComPtr<ID3D11InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue)) && IsDebuggerPresent()) {
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif

    // Device -> DXGI device -> adapter, to read the GPU's name.
    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(m_device.As(&dxgiDevice), "QueryInterface(IDXGIDevice)");
    ComPtr<IDXGIAdapter> adapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(&adapter), "IDXGIDevice::GetAdapter");
    DXGI_ADAPTER_DESC adapterDesc{};
    ThrowIfFailed(adapter->GetDesc(&adapterDesc), "IDXGIAdapter::GetDesc");
    m_adapterDescription = WideToUtf8(adapterDesc.Description);

    // Feature level 11_0 guarantees these, so they are constants rather than
    // queries. Texture units are limited by samplers (16), not by texture
    // slots (128), because each unit is a texture *and* a sampler.
    SetLimits(DeviceLimits{
        .maximumNumberOfVertexAttributes = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,   // 32
        .numberOfTextureUnits = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,                // 16
        .maximumNumberOfColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,    // 8
    });

    // Automatic uniforms and the four common samplers (Steps 4 and 5).
    InitializeCommon();
}

DeviceD3D11::~DeviceD3D11() {
    // The base class's samplers, released while the device is certainly alive
    // (the same order as DeviceGL3x; D3D11 would also tolerate the reverse).
    ReleaseCommon();

    // Unbind everything, and let the driver destroy released objects now.
    // Windows (and their contexts) must already be gone: see "Destruction order".
    if (m_immediate) {
        m_immediate->ClearState();
        m_immediate->Flush();
    }
}

std::unique_ptr<GraphicsWindow> DeviceD3D11::DoCreateGraphicsWindow(
    int width, int height, const std::string& title, WindowType type) {
    return std::make_unique<GraphicsWindowD3D11>(*this, width, height, title, type);
}

// --- Stubs. Each later step replaces one or more of these. ------------------

std::shared_ptr<ShaderProgram> DeviceD3D11::DoCreateShaderProgram(std::string_view, std::string_view,
                                                                  std::string_view) {
    NotImplementedYet("CreateShaderProgram (Step 2)");
}

std::shared_ptr<VertexBuffer> DeviceD3D11::DoCreateVertexBuffer(BufferHint, std::size_t) {
    NotImplementedYet("CreateVertexBuffer (Step 3)");
}

std::shared_ptr<IndexBuffer> DeviceD3D11::DoCreateIndexBuffer(BufferHint, std::size_t) {
    NotImplementedYet("CreateIndexBuffer (Step 3)");
}

std::shared_ptr<WritePixelBuffer> DeviceD3D11::DoCreateWritePixelBuffer(PixelBufferHint, std::size_t) {
    NotImplementedYet("CreateWritePixelBuffer (Step 5)");
}

std::shared_ptr<Texture2D> DeviceD3D11::DoCreateTexture2D(const Texture2DDescription&) {
    NotImplementedYet("CreateTexture2D (Step 5)");
}

std::shared_ptr<TextureSampler> DeviceD3D11::DoCreateTexture2DSampler(const TextureSamplerDescription&) {
    // Not a throwing stub: InitializeCommon creates the four common samplers
    // in the constructor. Until Step 5, Device::Samplers() holds nulls.
    return nullptr;
}

} // namespace arda::renderer::d3d11
```

- **`DXGI_ERROR_SDK_COMPONENT_MISSING`** is how `D3D11CreateDevice` says the
  debug layer isn't installed. Nothing else in the code depends on the flag,
  so falling back is safe.
- **`m_device.As(&dxgiDevice)`** is `QueryInterface`: the D3D11 device object
  also implements `IDXGIDevice`, the DXGI view of the same GPU. DXGI (the
  DirectX Graphics Infrastructure) is the layer below D3D that knows about
  adapters, monitors and swap chains. It is shared by D3D10, 11 and 12.
- **Designated initializers** (`.numberOfTextureUnits = ...`), from
  [Step 1](01-state-management.md#cpp-designated-initializers), keep each
  constant next to the field it fills.
- **`[[noreturn]]`** on `NotImplementedYet` tells the compiler it never
  returns, so the stubs don't need a `return` after the call
  ([Step 2](02-shaders.md#cpp-noreturn)).

> **Why `ReleaseCommon` is called here:** `Device::m_samplers` is a base-class
> member, so it is destroyed *after* `~DeviceD3D11()` and after `m_device`.
> On GL that would delete sampler objects with no context current, which is
> why [Step 5](05-textures.md) added `ReleaseCommon`. On D3D11 it is harmless
> either way, because each sampler state holds an internal reference to its
> device (the COM note above). Calling it anyway keeps both backends' shutdown
> order the same, which makes the code easier to reason about.

### D3D11 concepts: swap chains and views

> **D3D11 note — DXGI swap chains:** in GL, `glfwCreateWindow` gives you a
> default framebuffer and `glfwSwapBuffers` shows it. D3D11 has no default
> framebuffer. A *swap chain* is a small set of textures (the *buffers*)
> owned by DXGI and attached to one window. You render into the current
> *back buffer*; `Present` hands it to the Windows compositor to show and
> gives you the next one. Creating it takes a `DXGI_SWAP_CHAIN_DESC1`:
> - **`Format`**: `DXGI_FORMAT_R8G8B8A8_UNORM`, 8 bits per channel, the same
>   as GL's default framebuffer.
> - **`SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD`**: the *flip model*. The
>   compositor displays your buffer directly instead of copying it (the old
>   "blt model"), which is faster, required for variable refresh rate, and
>   the only model Microsoft recommends since Windows 10. "Discard" means the
>   back buffer's contents are undefined after `Present`, so you must clear
>   or redraw every pixel each frame, which arda does anyway.
> - **`BufferCount = 2`**: flip-model swap chains need at least two buffers,
>   one being shown while you draw into the other. Three reduces stalls at
>   the cost of a frame of latency.
> - **`SampleDesc.Count = 1`**: flip-model swap chains can't be multisampled.
>   (MSAA would render into a separate multisampled texture and resolve into
>   the back buffer.)
> - **`Width = Height = 0`**: take the size of the window's client area.
>
> **`Present(syncInterval, flags)`** is `glfwSwapBuffers`. A sync interval
> of 1 waits for the next vertical blank, which is `glfwSwapInterval(1)`
> (vsync). 0 presents immediately.

> **D3D11 note — resources and views:** in GL, a texture is one object, and
> you attach it to a framebuffer or bind it to a texture unit directly.
> D3D11 separates the *resource* (the memory: `ID3D11Texture2D`,
> `ID3D11Buffer`) from the *views* that say how a pipeline stage may use it:
> - a **render target view** (`ID3D11RenderTargetView`, RTV) lets the output
>   merger write color to it,
> - a **depth-stencil view** (`ID3D11DepthStencilView`, DSV) lets it be the
>   depth buffer,
> - a **shader resource view** (`ID3D11ShaderResourceView`, SRV) lets
>   shaders sample it (Step 5).
>
> The pipeline only ever binds views. A view can also reinterpret the
> resource: a depth texture created as `R32_TYPELESS` is seen as `D32_FLOAT`
> through its DSV and as `R32_FLOAT` through its SRV (Step 5). Views are
> created once and cached; creating them is cheap, but not free.

> **D3D11 note — the window's depth buffer:** GL's default framebuffer comes
> with the depth and stencil buffers you asked for with `GLFW_DEPTH_BITS`
> and `GLFW_STENCIL_BITS`. A DXGI swap chain has only color buffers. The
> window creates its own `D24_UNORM_S8_UINT` texture (24-bit depth, 8-bit
> stencil, the same as the GL hints) and a DSV for it, and must recreate
> both whenever the window is resized.

### `src/d3d11/GraphicsWindowD3D11.h`

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/GraphicsApi.h>
#include <arda/renderer/GraphicsWindow.h>

#include <memory>
#include <string>

struct GLFWwindow;

namespace arda::renderer::d3d11 {

class ContextD3D11;
class DeviceD3D11;

class GraphicsWindowD3D11 final : public GraphicsWindow {
public:
    GraphicsWindowD3D11(DeviceD3D11& device, int width, int height, const std::string& title, WindowType type);
    ~GraphicsWindowD3D11() override;

    Context& GetContext() override;
    int Width() const override { return m_width; }
    int Height() const override { return m_height; }

    bool ShouldClose() const override;
    void PollEvents() override;
    void SwapBuffers() override;

    // For ContextD3D11: what "no framebuffer" means on this window.
    ID3D11RenderTargetView* BackBufferView() const { return m_backBufferView.Get(); }
    ID3D11DepthStencilView* DepthStencilView() const { return m_depthStencilView.Get(); }
    int BackBufferHeight() const { return m_backBufferHeight; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    void CreateSwapChain();
    void CreateSizeDependentResources();    // back buffer view, depth buffer and its view
    void ReleaseSizeDependentResources();
    void ResizeSwapChain();                 // to m_width x m_height

    DeviceD3D11& m_device;
    GLFWwindow* m_handle = nullptr;

    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_backBufferView;
    ComPtr<ID3D11Texture2D> m_depthStencilTexture;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    std::unique_ptr<ContextD3D11> m_context;

    int m_width = 0;              // the window's framebuffer size, never 0 after construction
    int m_height = 0;
    int m_backBufferWidth = 0;    // the swap chain's size, which follows m_width/m_height in PollEvents
    int m_backBufferHeight = 0;
    bool m_resizePending = false; // set by the GLFW callback, handled in PollEvents
};

} // namespace arda::renderer::d3d11
```

As in `GraphicsWindowGL3x.h`, `ContextD3D11` is only forward-declared, and
the destructor is defined in the `.cpp`, where the `unique_ptr` can see the
complete type ([Step 0](00-setup.md#cpp-unique-ptr)).

### `src/d3d11/GraphicsWindowD3D11.cpp`

```cpp
#include "d3d11/GraphicsWindowD3D11.h"
#include "d3d11/ContextD3D11.h"
#include "d3d11/DeviceD3D11.h"

// No GL header: GLFW would otherwise include <GL/gl.h>.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// glfwGetWin32Window. This header includes <windows.h>, which is why CMake
// defines NOMINMAX and WIN32_LEAN_AND_MEAN for the whole target.
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>

namespace arda::renderer::d3d11 {

GraphicsWindowD3D11::GraphicsWindowD3D11(
    DeviceD3D11& device, int width, int height, const std::string& title, WindowType type)
    : m_device(device) {
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // a plain window: no GL context
    glfwWindowHint(GLFW_VISIBLE, type == WindowType::Hidden ? GLFW_FALSE : GLFW_TRUE);

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_handle == nullptr) {
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, &FramebufferSizeCallback);
    glfwGetFramebufferSize(m_handle, &m_width, &m_height);

    // If anything below throws, the destructor won't run (the object was never
    // fully constructed), so clean up here.
    try {
        CreateSwapChain();
        CreateSizeDependentResources();
        m_context = std::make_unique<ContextD3D11>(device, *this, m_backBufferWidth, m_backBufferHeight);
    } catch (...) {
        m_context.reset();
        ReleaseSizeDependentResources();
        m_swapChain.Reset();
        glfwDestroyWindow(m_handle);
        throw;
    }
}

GraphicsWindowD3D11::~GraphicsWindowD3D11() {
    // The immediate context may still have this window's back buffer and depth
    // buffer bound. Unbind everything, and tell every context to re-apply its state.
    ID3D11DeviceContext* immediate = m_device.Immediate();
    immediate->ClearState();
    m_device.SetActiveContext(nullptr);

    m_context.reset();
    ReleaseSizeDependentResources();
    m_swapChain.Reset();       // the swap chain refers to the HWND, so release it first
    immediate->Flush();        // let the driver destroy the released objects now

    glfwDestroyWindow(m_handle);
}

Context& GraphicsWindowD3D11::GetContext() {
    return *m_context;
}

bool GraphicsWindowD3D11::ShouldClose() const {
    return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

void GraphicsWindowD3D11::PollEvents() {
    glfwPollEvents();   // calls FramebufferSizeCallback for any window whose size changed

    // As in GraphicsWindowGL3x: resize here, in C++ code, not inside GLFW's C callback,
    // so that an exception from ResizeBuffers or from the app's handler reaches main.
    if (m_resizePending) {
        m_resizePending = false;
        ResizeSwapChain();
        OnResize();
    }
}

void GraphicsWindowD3D11::SwapBuffers() {
    const HRESULT hr = m_swapChain->Present(1, 0);   // sync interval 1: vsync, like glfwSwapInterval(1)

    // A flip-model Present unbinds the back buffer from the output merger.
    m_context->ForgetRenderTargets();

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // The GPU was reset, its driver updated, or it hung. The reason is more useful than hr.
        ThrowIfFailed(m_device.Native()->GetDeviceRemovedReason(), "Present (device removed)");
    }
    ThrowIfFailed(hr, "IDXGISwapChain::Present");   // DXGI_STATUS_OCCLUDED (minimized) is a success
}

void GraphicsWindowD3D11::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<GraphicsWindowD3D11*>(glfwGetWindowUserPointer(window));
    if (width == 0 || height == 0) {
        return;   // minimized: keep the last real size, and the swap chain as it is
    }
    self->m_width = width;
    self->m_height = height;
    self->m_resizePending = true;
}

void GraphicsWindowD3D11::CreateSwapChain() {
    // The swap chain must be created by the DXGI factory that created the
    // device: device -> DXGI device -> adapter -> factory.
    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(m_device.Native()->QueryInterface(IID_PPV_ARGS(&dxgiDevice)), "QueryInterface(IDXGIDevice)");
    ComPtr<IDXGIAdapter> adapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(&adapter), "IDXGIDevice::GetAdapter");
    ComPtr<IDXGIFactory2> factory;
    ThrowIfFailed(adapter->GetParent(IID_PPV_ARGS(&factory)), "IDXGIAdapter::GetParent(IDXGIFactory2)");

    const HWND hwnd = glfwGetWin32Window(m_handle);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = 0;                                   // 0 x 0: the window's client area
    desc.Height = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;                        // flip model: no MSAA back buffers
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;                             // flip model: at least 2
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = 0;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(m_device.Native(), hwnd, &desc,
                                                  nullptr,    // windowed, not fullscreen
                                                  nullptr,    // any output (monitor)
                                                  &m_swapChain),
                  "IDXGIFactory2::CreateSwapChainForHwnd");

    // DXGI would otherwise switch to exclusive fullscreen on Alt+Enter, behind GLFW's back.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
}

void GraphicsWindowD3D11::CreateSizeDependentResources() {
    ID3D11Device* device = m_device.Native();

    // A render target view of back buffer 0. With the flip model, "buffer 0"
    // always means the current back buffer, so one view serves every frame.
    ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "IDXGISwapChain::GetBuffer");
    ThrowIfFailed(device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferView),
                  "CreateRenderTargetView(back buffer)");
    SetDebugName(m_backBufferView.Get(), "window back buffer");

    D3D11_TEXTURE2D_DESC backBufferDesc{};
    backBuffer->GetDesc(&backBufferDesc);
    m_backBufferWidth = static_cast<int>(backBufferDesc.Width);
    m_backBufferHeight = static_cast<int>(backBufferDesc.Height);

    // The depth/stencil buffer GL gets from GLFW_DEPTH_BITS and GLFW_STENCIL_BITS.
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = backBufferDesc.Width;
    depthDesc.Height = backBufferDesc.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ThrowIfFailed(device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilTexture),
                  "CreateTexture2D(window depth buffer)");
    ThrowIfFailed(device->CreateDepthStencilView(m_depthStencilTexture.Get(), nullptr, &m_depthStencilView),
                  "CreateDepthStencilView(window depth buffer)");
    SetDebugName(m_depthStencilView.Get(), "window depth buffer");
}

void GraphicsWindowD3D11::ReleaseSizeDependentResources() {
    m_backBufferView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilTexture.Reset();
}

void GraphicsWindowD3D11::ResizeSwapChain() {
    if (m_width == m_backBufferWidth && m_height == m_backBufferHeight) {
        return;
    }

    // ResizeBuffers fails unless every reference to the old back buffer is
    // gone, including the immediate context's binding of its view.
    ID3D11DeviceContext* immediate = m_device.Immediate();
    immediate->ClearState();
    m_device.SetActiveContext(nullptr);   // ClearState reset every binding: all contexts must re-apply
    ReleaseSizeDependentResources();
    immediate->Flush();                   // D3D11 destroys released objects lazily; make it happen now

    ThrowIfFailed(m_swapChain->ResizeBuffers(0,   // keep the buffer count
                                             static_cast<UINT>(m_width), static_cast<UINT>(m_height),
                                             DXGI_FORMAT_UNKNOWN,   // keep the format
                                             0),
                  "IDXGISwapChain::ResizeBuffers");
    CreateSizeDependentResources();
}

} // namespace arda::renderer::d3d11
```

What each part does, compared with `GraphicsWindowGL3x`:

1. **`GLFW_CLIENT_API = GLFW_NO_API`** makes GLFW create a plain Win32
   window with no GL context, so no `glfwMakeContextCurrent` and no
   `glfwSwapInterval`. Everything else about the GLFW window (the user
   pointer, the size callback, `glfwPollEvents`) is the same.
2. **`GLFW_INCLUDE_NONE`** stops `<GLFW/glfw3.h>` from including the system
   `<GL/gl.h>`. The D3D11 backend doesn't need GL, and doesn't link it.
3. **`glfwGetWin32Window`** returns the window's `HWND`, the Win32 handle DXGI
   attaches the swap chain to.
4. **The factory walk** (`IDXGIDevice` → `GetAdapter` → `GetParent`) finds
   the `IDXGIFactory2` that owns the device's adapter. A swap chain made by a
   different factory fails with `DXGI_ERROR_INVALID_CALL`.
5. **The resize protocol** is the same as GL's (the callback records the
   size; `PollEvents` acts on it, and ignores 0×0), with one D3D11-specific
   step in the middle: `ResizeSwapChain`.

> **D3D11 note — resizing a swap chain:** `ResizeBuffers` reallocates the
> back buffers, so it fails with `DXGI_ERROR_INVALID_CALL` while anything
> still refers to them. That includes your RTV, *and* the immediate
> context's binding of that RTV, *and* objects you released that the driver
> hasn't destroyed yet (D3D11 destroys objects lazily). Hence the order:
> `ClearState` (unbind everything), release the views, `Flush` (destroy them
> now), `ResizeBuffers`, recreate the views and the depth buffer. After
> `ClearState`, every cached binding in every `ContextD3D11` is wrong, so
> the device's active context is reset to "unknown".

> **D3D11 note — `Present` unbinds the back buffer:** with the flip model,
> the buffer you just presented belongs to the compositor, and the runtime
> removes it from the output merger. The next frame must bind the back
> buffer view again before it clears or draws. `SwapBuffers` tells the
> context to forget its cached render targets, so its next `Clear` rebinds
> them.

> **D3D11 note — device removed:** a D3D11 device can be lost: the driver
> was updated, the GPU hung and Windows reset it (TDR), or the app hit a
> driver bug. Every call keeps "working" but does nothing, and `Present`
> returns `DXGI_ERROR_DEVICE_REMOVED`. `GetDeviceRemovedReason` tells you
> why (`DXGI_ERROR_DEVICE_HUNG` usually means a shader ran too long).
> Recovering means recreating every resource; arda just throws, which
> `main` reports.

> **C++ note — exceptions in constructors:** if a constructor throws, the
> object never existed, so its destructor never runs. Members that were
> already constructed *are* destroyed (the `ComPtr`s release themselves),
> but the raw `GLFWwindow*` isn't a member with a destructor. That is why
> the constructor catches, destroys the window, and rethrows with `throw;`,
> exactly like `GraphicsWindowGL3x` ([Step 0](00-setup.md#cpp-exceptions)).

### `src/d3d11/ContextD3D11.h`

This is the final header, with every member the later steps use. Only
forward declarations are needed for the other backend classes, so it
compiles now.

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/Context.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace arda::renderer::d3d11 {

class DeviceD3D11;
class GraphicsWindowD3D11;
class ShaderProgramD3D11;
class Texture2DD3D11;

class ContextD3D11 final : public Context {
public:
    ContextD3D11(DeviceD3D11& device, GraphicsWindowD3D11& window, int width, int height);
    ~ContextD3D11() override;

    // Makes this the context whose state the device's immediate context holds.
    void MakeCurrent() override;

    // For GraphicsWindowD3D11.
    void ForgetRenderTargets() { m_renderTargets.reset(); }   // after Present unbound the back buffer
    void InvalidateCachedState();                              // the immediate context's state is unknown

protected:
    void DoClear(const ClearState& clearState) override;
    void DoSetViewport(const Rectangle& viewport) override;

    std::shared_ptr<VertexArray> DoCreateVertexArray() override;
    std::shared_ptr<Framebuffer> DoCreateFramebuffer() override;

    void DoDraw(core::geometry::PrimitiveType primitiveType,
                const DrawState& drawState, const SceneState& sceneState) override;
    void DoDrawRange(core::geometry::PrimitiveType primitiveType, int offset, int count,
                     const DrawState& drawState, const SceneState& sceneState) override;

    // Step 4 on D3D11: flip clip-space y while a framebuffer is bound.
    bool FlipsRenderTargetY() const override;

private:
    // What OMSetRenderTargets was last called with. Raw pointers are safe to
    // compare: the immediate context holds a reference to each bound view, so
    // a bound view can't be destroyed and replaced by a new one at the same address.
    struct RenderTargetBinding {
        std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> views{};
        UINT count = 0;
        ID3D11DepthStencilView* depthStencil = nullptr;

        bool operator==(const RenderTargetBinding&) const = default;
    };

    ID3D11DeviceContext* Immediate() const;

    // Called first by every Clear and Draw.
    void EnsureActive();

    // Step 1: render targets, render state and the viewport (3.3)
    void ApplyFramebuffer();
    void ApplyRenderState(const RenderState& renderState);
    void ApplyScissorTest(const ScissorTest& scissorTest);
    void ApplyViewport(const DepthRange& depthRange);
    int ToTopLeftY(int bottom, int height) const;   // arda's bottom-left y to D3D's top-left y

    // Step 3: drawing (3.5)
    void ApplyBeforeDraw(core::geometry::PrimitiveType primitiveType,
                         const DrawState& drawState, const SceneState& sceneState);
    void ApplyPrimitiveTopology(core::geometry::PrimitiveType primitiveType);
    void ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState);
    void ApplyVertexArray(const DrawState& drawState);

    // Step 5: texture units (3.6)
    void CleanTextureUnits();

    // Step 6: the render target / shader resource hazard (3.7)
    void UnbindShaderResourcesUsedAsRenderTargets();
    bool IsRenderTarget(const Texture2DD3D11& texture) const;

    DeviceD3D11& m_deviceD3D11;   // the base class's m_device is private and typed Device&
    GraphicsWindowD3D11& m_window;

    // Cached copy of what this context last applied to the immediate context.
    // std::nullopt means "unknown": the next draw applies it unconditionally.
    std::optional<ID3D11DepthStencilState*> m_depthStencilState;
    std::optional<UINT> m_stencilReference;
    std::optional<ID3D11BlendState*> m_blendState;
    std::optional<std::array<float, 4>> m_blendFactor;
    std::optional<ID3D11RasterizerState*> m_rasterizerState;
    std::optional<std::array<LONG, 4>> m_scissorRectangle;    // left, top, right, bottom
    std::optional<std::array<float, 6>> m_appliedViewport;    // D3D11_VIEWPORT's six fields, in order
    std::optional<D3D11_PRIMITIVE_TOPOLOGY> m_primitiveTopology;
    std::optional<ID3D11InputLayout*> m_inputLayout;
    std::optional<RenderTargetBinding> m_renderTargets;
    bool m_depthStencilHasStencil = false;   // for Clear: only clear stencil if there is one

    // weak_ptr for the same reason as in ContextGL3x: a destroyed object's
    // address can be reused by a new one, which must still be bound.
    std::weak_ptr<VertexArray> m_boundVertexArray;
    std::weak_ptr<ShaderProgram> m_boundShaderProgram;

    // Step 5 and 6: the view bound in each texture unit's slot, the textures
    // bound as render targets, and units to bind again at the next draw.
    std::vector<ID3D11ShaderResourceView*> m_boundShaderResources;
    std::vector<const Texture2DD3D11*> m_renderTargetTextures;
    std::vector<int> m_textureUnitsToRebind;
};

} // namespace arda::renderer::d3d11
```

> **Why a cache at all:** a D3D11 call like `OMSetBlendState` is cheaper
> than a GL call because it sets a whole prebuilt object, but it still goes
> through the runtime and the driver. The cache mirrors `ContextGL3x`'s
> cached `RenderState` (3.3.3): compare with what was last applied, and skip
> the call if nothing changed. The difference is what gets cached: GL caches
> individual values, D3D11 caches *which object* is bound.

> **C++ note — `std::optional` as "unknown":** each cached value is a
> `std::optional` ([Step 2](02-shaders.md#cpp-optional)). An empty optional
> means "we don't know what's bound", and `m_blendState != blendState`
> compares an `optional<T>` with a plain `T`: an empty optional is never
> equal to a value, so the call is made. Invalidating the whole cache is then
> just `reset()` on every member. The alternative, a sentinel such as a
> made-up pointer value, is easy to get wrong and impossible for an
> `std::array` of floats.

> **C++ note — `LONG` and `float` arrays instead of D3D structs:**
> `D3D11_VIEWPORT` and `D3D11_RECT` have no `operator==`, and C++ can't
> default one for a type you don't own. Storing their fields in a
> `std::array`, which does have `operator==`, gives a correct comparison for
> free. Comparing the structs with `std::memcmp` would also work here (both
> have no padding), but see the padding note in Step 1 on D3D11 for why that
> habit is dangerous.

### `src/d3d11/ContextD3D11.cpp` (starter)

The constructor, destructor, `MakeCurrent`, the active-context logic and
two final one-liners. The other overrides are stubs until their steps.

```cpp
#include "d3d11/ContextD3D11.h"
#include "d3d11/DeviceD3D11.h"
#include "d3d11/GraphicsWindowD3D11.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>

namespace arda::renderer::d3d11 {

using core::geometry::PrimitiveType;

namespace {
// TEMPORARY: removed once every stub below is replaced.
[[noreturn]] void NotImplementedYet(const char* what) {
    throw std::logic_error(std::string(what) + " is not implemented yet on Direct3D 11");
}
} // namespace

ContextD3D11::ContextD3D11(DeviceD3D11& device, GraphicsWindowD3D11& window, int width, int height)
    : Context(device),
      m_deviceD3D11(device),
      m_window(window),
      m_boundShaderResources(static_cast<std::size_t>(GetTextureUnits().Count()), nullptr) {
    SetViewport(Rectangle{0, 0, width, height});
}

ContextD3D11::~ContextD3D11() {
    if (m_deviceD3D11.ActiveContext() == this) {
        m_deviceD3D11.SetActiveContext(nullptr);
    }
}

ID3D11DeviceContext* ContextD3D11::Immediate() const {
    return m_deviceD3D11.Immediate();
}

void ContextD3D11::MakeCurrent() {
    EnsureActive();
}

void ContextD3D11::EnsureActive() {
    if (m_deviceD3D11.ActiveContext() != this) {
        // Another context (another window) used the immediate context since
        // this one did, or ClearState reset it. Nothing in the cache can be trusted.
        m_deviceD3D11.SetActiveContext(this);
        InvalidateCachedState();
    }
}

void ContextD3D11::InvalidateCachedState() {
    m_depthStencilState.reset();
    m_stencilReference.reset();
    m_blendState.reset();
    m_blendFactor.reset();
    m_rasterizerState.reset();
    m_scissorRectangle.reset();
    m_appliedViewport.reset();
    m_primitiveTopology.reset();
    m_inputLayout.reset();
    m_renderTargets.reset();
    m_boundVertexArray.reset();
    m_boundShaderProgram.reset();

    // Every texture unit must be bound again, whether or not it changed.
    std::fill(m_boundShaderResources.begin(), m_boundShaderResources.end(), nullptr);
    m_textureUnitsToRebind.resize(m_boundShaderResources.size());
    std::iota(m_textureUnitsToRebind.begin(), m_textureUnitsToRebind.end(), 0);   // 0, 1, 2, ...
}

void ContextD3D11::DoSetViewport(const Rectangle&) {
    // Nothing to do yet. D3D11's viewport depends on the render target's height
    // and on the draw's depth range, so ApplyViewport sets it before each draw.
}

bool ContextD3D11::FlipsRenderTargetY() const {
    return GetFramebuffer() != nullptr;
}

// --- Stubs. Later steps replace these. ---------------------------------------

void ContextD3D11::DoClear(const ClearState&) {
    NotImplementedYet("Context::Clear (Step 1)");
}

std::shared_ptr<VertexArray> ContextD3D11::DoCreateVertexArray() {
    NotImplementedYet("Context::CreateVertexArray (Step 3)");
}

std::shared_ptr<Framebuffer> ContextD3D11::DoCreateFramebuffer() {
    NotImplementedYet("Context::CreateFramebuffer (Step 6)");
}

void ContextD3D11::DoDraw(PrimitiveType, const DrawState&, const SceneState&) {
    NotImplementedYet("Context::Draw (Step 3)");
}

void ContextD3D11::DoDrawRange(PrimitiveType, int, int, const DrawState&, const SceneState&) {
    NotImplementedYet("Context::Draw (Step 3)");
}

} // namespace arda::renderer::d3d11
```

`std::fill` (from `<algorithm>`) sets every element to one value, and
`std::iota` (from `<numeric>`) fills a range with increasing values.

> **C++ note — a base-class reference and a derived-class reference to the
> same object:** `Context` keeps a `Device&`, private to `Context`.
> `ContextD3D11` needs the `DeviceD3D11` interface (`Immediate()`,
> `ActiveContext()`), so it keeps its own `DeviceD3D11&` to the same object.
> The alternative, `static_cast<DeviceD3D11&>(GetDevice())` at every use,
> is correct ([Step 0](00-setup.md#cpp-static-cast)) but noisy. The member is
> named `m_deviceD3D11` so it can't be confused with the base class's.

### Destruction order

The rule from the README applies with more force here: **destroy windows
and resources before the device.** A `ContextD3D11` refers to its
`DeviceD3D11`, and `~GraphicsWindowD3D11` calls `m_device.Immediate()`. In
`main`, declaring the device first (so it is destroyed last) is enough. COM
itself would tolerate a texture outliving the device; arda's references
would not.

### Milestone: a D3D11 device and window

Add a test case to `tests/src/renderer/DeviceTests.cpp`:

```cpp
TEST_CASE("CreateDevice creates a Direct3D 11 device with feature level 11_0 limits") {
    if (!IsGraphicsApiAvailable(GraphicsApi::Direct3D11)) {
        MESSAGE("Direct3D 11 backend not compiled; skipping");
        return;
    }
    auto device = CreateDevice(GraphicsApi::Direct3D11);
    CHECK(device->Api() == GraphicsApi::Direct3D11);
    CHECK(device->ClipDepthRange() == arda::core::ClipDepth::ZeroToOne);
    CHECK(device->Limits().maximumNumberOfVertexAttributes == 32);
    CHECK(device->Limits().numberOfTextureUnits == 16);
    CHECK(device->Limits().maximumNumberOfColorAttachments == 8);

    auto window = device->CreateGraphicsWindow(64, 32, "test", WindowType::Hidden);
    CHECK(window->Width() > 0);    // in pixels, so not necessarily 64 x 32 on a high-DPI screen
    CHECK(window->Height() > 0);
    window->PollEvents();
    window->SwapBuffers();   // Present on a hidden window succeeds (possibly with DXGI_STATUS_OCCLUDED)
}
```

Run it under the debugger once and look at the Output window. A clean run
prints no `D3D11 ERROR` or `D3D11 WARNING` lines.

**Checklist for this step:** the device test passes, a visible window opens
and closes cleanly with `GraphicsApi::Direct3D11`, and resizing it prints no
debug-layer errors.

---

## Step 1 on D3D11: state objects, clears and the viewport

This step mirrors [Step 1](01-state-management.md). `RenderState` and
`ClearState` are unchanged. What changes is how a `RenderState` reaches the
API.

> **D3D11 note — immutable state objects vs. GL's state machine:** GL has
> dozens of independent switches: `glEnable(GL_DEPTH_TEST)`,
> `glDepthFunc`, `glBlendFuncSeparate`, ... Each call changes one value, and
> the driver has to re-validate the combination at the next draw. D3D11
> groups related fixed-function settings into three *state objects*, each
> built from a description struct and **immutable** once created:
>
> | Object | Created from | Bound with | arda fields |
> |---|---|---|---|
> | `ID3D11DepthStencilState` | `D3D11_DEPTH_STENCIL_DESC` | `OMSetDepthStencilState(state, stencilRef)` | `depthTest`, `depthMask`, `stencilTest` |
> | `ID3D11BlendState` | `D3D11_BLEND_DESC` | `OMSetBlendState(state, blendFactor, sampleMask)` | `blending`, `colorMask` |
> | `ID3D11RasterizerState` | `D3D11_RASTERIZER_DESC` | `RSSetState(state)` | `facetCulling`, `rasterizationMode`, `scissorTest.enabled` |
>
> The driver validates each object once, at creation. Binding one is then a
> single cheap call. A few values are *not* in any object, because they
> change often: the stencil reference value and the blend color are
> arguments of the bind calls, the scissor rectangle goes to
> `RSSetScissorRects`, and the depth range is part of the viewport. This
> design is why the book's `RenderState` is a plain value passed to every
> draw (3.3.2): it maps naturally onto "look up the object for these
> values".

> **D3D11 note — clears:** D3D11 has no `glClearColor` state and no
> `glClear` bitmask. `ClearRenderTargetView(view, color)` clears one color
> view, and `ClearDepthStencilView(view, flags, depth, stencil)` clears the
> depth and/or stencil of one depth view. Both clear the *whole* view: the
> viewport, the scissor rectangle and the write masks never affect them.

> **Why `ClearState` has no scissor or masks (portability table, "Clears"):**
> GL's `glClear` *does* honor the scissor test and the write masks. D3D11
> has no way to honor them short of drawing a quad. Keeping them out of
> `ClearState` means both backends behave like D3D11, and the GL backend
> resets them before `glClear` ([Step 1](01-state-management.md)). On D3D11
> there is nothing to do.

> **D3D11 note — the viewport's origin is top-left:** in GL, window
> coordinate (0, 0) is the bottom-left pixel and y grows upward. In D3D11,
> (0, 0) is the top-left pixel and y grows downward. That applies to
> `D3D11_VIEWPORT::TopLeftY`, to scissor rectangles, and to `SV_Position`
> in pixel shaders. The clip-space convention is the same in both APIs
> (+y is up in normalized device coordinates); only the mapping from NDC to
> pixel rows is mirrored.

> **Why `Rectangle` stays bottom-left (portability table, "Viewport
> origin"):** the public API has to pick one convention, and the book, the
> automatic uniforms (`og_viewport`) and every OpenGlobe example use GL's.
> The D3D11 backend converts: for the window,
> `TopLeftY = backBufferHeight - (bottom + height)`. While a framebuffer is
> bound, no conversion is needed at all, because of the render-target flip
> ([Step 4 on D3D11](#step-4-on-d3d11-the-render-target-y-flip) explains
> why). `ToTopLeftY` holds both rules in one place.

### `src/d3d11/TypeConverterD3D11.h`

Like `TypeConverterGL3x`, this is the only file that knows D3D11's enum
values. It contains every conversion the backend needs, including the ones
used in Steps 3 and 5, so it is written once.

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/renderstate/RenderState.h>
#include <arda/renderer/textures/TextureFormat.h>
#include <arda/renderer/textures/TextureSampler.h>
#include <arda/renderer/vertexarray/ComponentDatatype.h>

namespace arda::renderer::d3d11 {

// Step 1: render state. Each function throws std::invalid_argument for a
// value that isn't an enumerator, or that D3D11 can't express.
D3D11_COMPARISON_FUNC ToD3D(DepthTestFunction function);
D3D11_COMPARISON_FUNC ToD3D(StencilTestFunction function);
D3D11_STENCIL_OP ToD3D(StencilOperation operation);
D3D11_CULL_MODE ToD3D(CullFace face);                   // throws for FrontAndBack
D3D11_FILL_MODE ToD3D(RasterizationMode mode);          // throws for Point
D3D11_BLEND ToD3D(SourceBlendingFactor factor);
D3D11_BLEND ToD3D(DestinationBlendingFactor factor);
D3D11_BLEND ToD3DAlpha(SourceBlendingFactor factor);    // for the alpha factors: no *_COLOR values
D3D11_BLEND ToD3DAlpha(DestinationBlendingFactor factor);
D3D11_BLEND_OP ToD3D(BlendEquation equation);
UINT8 ToD3D(const ColorMask& colorMask);                // a D3D11_COLOR_WRITE_ENABLE_* bit mask

// Step 3: vertex data
D3D11_PRIMITIVE_TOPOLOGY ToD3D(core::geometry::PrimitiveType type);   // throws for LineLoop, TriangleFan
DXGI_FORMAT ToDxgiFormat(IndexBufferDatatype datatype);
DXGI_FORMAT ToDxgiFormat(ComponentDatatype datatype, int numberOfComponents, bool normalize);

// Step 5: textures and samplers
struct DxgiTextureFormats {
    DXGI_FORMAT texture;          // the ID3D11Texture2D
    DXGI_FORMAT target;           // its render target view, or its depth-stencil view
    DXGI_FORMAT shaderResource;   // its shader resource view
};

// Throws InsufficientVideoCardException for the formats DXGI doesn't have.
DxgiTextureFormats ToDxgiFormats(TextureFormat format);

D3D11_FILTER ToD3DFilter(TextureMinificationFilter minification, TextureMagnificationFilter magnification,
                         bool anisotropic);
bool UsesMipmaps(TextureMinificationFilter minification);
D3D11_TEXTURE_ADDRESS_MODE ToD3D(TextureWrap wrap);

} // namespace arda::renderer::d3d11
```

### `src/d3d11/TypeConverterD3D11.cpp`

Every case is written out, and each `switch` has no `default:`, so the
compiler warns when an enumerator is added and not converted
([Step 1](01-state-management.md#cpp-switch-without-default)).

```cpp
#include "d3d11/TypeConverterD3D11.h"

#include <arda/renderer/Exceptions.h>

#include <format>
#include <stdexcept>

namespace arda::renderer::d3d11 {

using core::geometry::PrimitiveType;

// --- Step 1: render state ---------------------------------------------------

D3D11_COMPARISON_FUNC ToD3D(DepthTestFunction function) {
    switch (function) {
    case DepthTestFunction::Never:              return D3D11_COMPARISON_NEVER;
    case DepthTestFunction::Less:               return D3D11_COMPARISON_LESS;
    case DepthTestFunction::Equal:              return D3D11_COMPARISON_EQUAL;
    case DepthTestFunction::LessThanOrEqual:    return D3D11_COMPARISON_LESS_EQUAL;
    case DepthTestFunction::Greater:            return D3D11_COMPARISON_GREATER;
    case DepthTestFunction::NotEqual:           return D3D11_COMPARISON_NOT_EQUAL;
    case DepthTestFunction::GreaterThanOrEqual: return D3D11_COMPARISON_GREATER_EQUAL;
    case DepthTestFunction::Always:             return D3D11_COMPARISON_ALWAYS;
    }
    throw std::invalid_argument("Invalid DepthTestFunction");
}

D3D11_COMPARISON_FUNC ToD3D(StencilTestFunction function) {
    switch (function) {
    case StencilTestFunction::Never:              return D3D11_COMPARISON_NEVER;
    case StencilTestFunction::Less:               return D3D11_COMPARISON_LESS;
    case StencilTestFunction::Equal:              return D3D11_COMPARISON_EQUAL;
    case StencilTestFunction::LessThanOrEqual:    return D3D11_COMPARISON_LESS_EQUAL;
    case StencilTestFunction::Greater:            return D3D11_COMPARISON_GREATER;
    case StencilTestFunction::NotEqual:           return D3D11_COMPARISON_NOT_EQUAL;
    case StencilTestFunction::GreaterThanOrEqual: return D3D11_COMPARISON_GREATER_EQUAL;
    case StencilTestFunction::Always:             return D3D11_COMPARISON_ALWAYS;
    }
    throw std::invalid_argument("Invalid StencilTestFunction");
}

D3D11_STENCIL_OP ToD3D(StencilOperation operation) {
    switch (operation) {
    case StencilOperation::Zero:          return D3D11_STENCIL_OP_ZERO;
    case StencilOperation::Invert:        return D3D11_STENCIL_OP_INVERT;
    case StencilOperation::Keep:          return D3D11_STENCIL_OP_KEEP;
    case StencilOperation::Replace:       return D3D11_STENCIL_OP_REPLACE;
    // GL_INCR and GL_DECR clamp; D3D11 calls those INCR_SAT and DECR_SAT.
    case StencilOperation::Increment:     return D3D11_STENCIL_OP_INCR_SAT;
    case StencilOperation::Decrement:     return D3D11_STENCIL_OP_DECR_SAT;
    // GL_INCR_WRAP and GL_DECR_WRAP wrap around; D3D11 calls those plain INCR and DECR.
    case StencilOperation::IncrementWrap: return D3D11_STENCIL_OP_INCR;
    case StencilOperation::DecrementWrap: return D3D11_STENCIL_OP_DECR;
    }
    throw std::invalid_argument("Invalid StencilOperation");
}

D3D11_CULL_MODE ToD3D(CullFace face) {
    switch (face) {
    case CullFace::Front: return D3D11_CULL_FRONT;
    case CullFace::Back:  return D3D11_CULL_BACK;
    case CullFace::FrontAndBack:
        throw std::invalid_argument(
            "CullFace::FrontAndBack is not supported by Direct3D 11. To draw no triangles, skip the draw call.");
    }
    throw std::invalid_argument("Invalid CullFace");
}

D3D11_FILL_MODE ToD3D(RasterizationMode mode) {
    switch (mode) {
    case RasterizationMode::Point:
        throw std::invalid_argument(
            "RasterizationMode::Point is not supported by Direct3D 11. Draw with PrimitiveType::Points instead.");
    case RasterizationMode::Line: return D3D11_FILL_WIREFRAME;
    case RasterizationMode::Fill: return D3D11_FILL_SOLID;
    }
    throw std::invalid_argument("Invalid RasterizationMode");
}

D3D11_BLEND ToD3D(SourceBlendingFactor factor) {
    switch (factor) {
    case SourceBlendingFactor::Zero:                     return D3D11_BLEND_ZERO;
    case SourceBlendingFactor::One:                      return D3D11_BLEND_ONE;
    case SourceBlendingFactor::SourceAlpha:              return D3D11_BLEND_SRC_ALPHA;
    case SourceBlendingFactor::OneMinusSourceAlpha:      return D3D11_BLEND_INV_SRC_ALPHA;
    case SourceBlendingFactor::DestinationAlpha:         return D3D11_BLEND_DEST_ALPHA;
    case SourceBlendingFactor::OneMinusDestinationAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
    case SourceBlendingFactor::DestinationColor:         return D3D11_BLEND_DEST_COLOR;
    case SourceBlendingFactor::OneMinusDestinationColor: return D3D11_BLEND_INV_DEST_COLOR;
    case SourceBlendingFactor::SourceAlphaSaturate:      return D3D11_BLEND_SRC_ALPHA_SAT;
    // D3D11 has one constant, the blend factor; BlendFactor() in StateCacheD3D11
    // fills it with the color or with the alpha, depending on the factors used.
    case SourceBlendingFactor::ConstantColor:            return D3D11_BLEND_BLEND_FACTOR;
    case SourceBlendingFactor::OneMinusConstantColor:    return D3D11_BLEND_INV_BLEND_FACTOR;
    case SourceBlendingFactor::ConstantAlpha:            return D3D11_BLEND_BLEND_FACTOR;
    case SourceBlendingFactor::OneMinusConstantAlpha:    return D3D11_BLEND_INV_BLEND_FACTOR;
    }
    throw std::invalid_argument("Invalid SourceBlendingFactor");
}

D3D11_BLEND ToD3D(DestinationBlendingFactor factor) {
    switch (factor) {
    case DestinationBlendingFactor::Zero:                     return D3D11_BLEND_ZERO;
    case DestinationBlendingFactor::One:                      return D3D11_BLEND_ONE;
    case DestinationBlendingFactor::SourceColor:              return D3D11_BLEND_SRC_COLOR;
    case DestinationBlendingFactor::OneMinusSourceColor:      return D3D11_BLEND_INV_SRC_COLOR;
    case DestinationBlendingFactor::SourceAlpha:              return D3D11_BLEND_SRC_ALPHA;
    case DestinationBlendingFactor::OneMinusSourceAlpha:      return D3D11_BLEND_INV_SRC_ALPHA;
    case DestinationBlendingFactor::DestinationAlpha:         return D3D11_BLEND_DEST_ALPHA;
    case DestinationBlendingFactor::OneMinusDestinationAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
    case DestinationBlendingFactor::DestinationColor:         return D3D11_BLEND_DEST_COLOR;
    case DestinationBlendingFactor::OneMinusDestinationColor: return D3D11_BLEND_INV_DEST_COLOR;
    case DestinationBlendingFactor::ConstantColor:            return D3D11_BLEND_BLEND_FACTOR;
    case DestinationBlendingFactor::OneMinusConstantColor:    return D3D11_BLEND_INV_BLEND_FACTOR;
    case DestinationBlendingFactor::ConstantAlpha:            return D3D11_BLEND_BLEND_FACTOR;
    case DestinationBlendingFactor::OneMinusConstantAlpha:    return D3D11_BLEND_INV_BLEND_FACTOR;
    }
    throw std::invalid_argument("Invalid DestinationBlendingFactor");
}

namespace {

// SrcBlendAlpha and DestBlendAlpha may not use the *_COLOR values; creating the
// blend state fails if they do. For the alpha channel, "source color" and
// "source alpha" are the same number anyway, so use the alpha versions.
D3D11_BLEND ColorFactorToAlphaFactor(D3D11_BLEND blend) {
    switch (blend) {
    case D3D11_BLEND_SRC_COLOR:      return D3D11_BLEND_SRC_ALPHA;
    case D3D11_BLEND_INV_SRC_COLOR:  return D3D11_BLEND_INV_SRC_ALPHA;
    case D3D11_BLEND_DEST_COLOR:     return D3D11_BLEND_DEST_ALPHA;
    case D3D11_BLEND_INV_DEST_COLOR: return D3D11_BLEND_INV_DEST_ALPHA;
    default:                         return blend;   // a D3D enum: only these four need changing
    }
}

} // namespace

D3D11_BLEND ToD3DAlpha(SourceBlendingFactor factor) {
    return ColorFactorToAlphaFactor(ToD3D(factor));
}

D3D11_BLEND ToD3DAlpha(DestinationBlendingFactor factor) {
    return ColorFactorToAlphaFactor(ToD3D(factor));
}

D3D11_BLEND_OP ToD3D(BlendEquation equation) {
    switch (equation) {
    case BlendEquation::Add:             return D3D11_BLEND_OP_ADD;
    case BlendEquation::Minimum:         return D3D11_BLEND_OP_MIN;
    case BlendEquation::Maximum:         return D3D11_BLEND_OP_MAX;
    case BlendEquation::Subtract:        return D3D11_BLEND_OP_SUBTRACT;
    case BlendEquation::ReverseSubtract: return D3D11_BLEND_OP_REV_SUBTRACT;
    }
    throw std::invalid_argument("Invalid BlendEquation");
}

UINT8 ToD3D(const ColorMask& colorMask) {
    UINT mask = 0;
    if (colorMask.red)   mask |= D3D11_COLOR_WRITE_ENABLE_RED;
    if (colorMask.green) mask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    if (colorMask.blue)  mask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    if (colorMask.alpha) mask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    return static_cast<UINT8>(mask);
}

// --- Step 3: vertex data ----------------------------------------------------

D3D11_PRIMITIVE_TOPOLOGY ToD3D(PrimitiveType type) {
    switch (type) {
    case PrimitiveType::Points:                 return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveType::Lines:                  return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveType::LineLoop:
        throw std::invalid_argument(
            "PrimitiveType::LineLoop is not supported by Direct3D 11. Use LineStrip and repeat the first index.");
    case PrimitiveType::LineStrip:              return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveType::Triangles:              return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveType::TriangleStrip:          return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveType::TriangleFan:
        throw std::invalid_argument(
            "PrimitiveType::TriangleFan is not supported by Direct3D 11. Use Triangles with explicit indices.");
    case PrimitiveType::LinesAdjacency:         return D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
    case PrimitiveType::LineStripAdjacency:     return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    case PrimitiveType::TrianglesAdjacency:     return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
    case PrimitiveType::TriangleStripAdjacency: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
    }
    throw std::invalid_argument("Invalid PrimitiveType");
}

DXGI_FORMAT ToDxgiFormat(IndexBufferDatatype datatype) {
    switch (datatype) {
    case IndexBufferDatatype::UnsignedShort: return DXGI_FORMAT_R16_UINT;
    case IndexBufferDatatype::UnsignedInt:   return DXGI_FORMAT_R32_UINT;
    }
    throw std::invalid_argument("Invalid IndexBufferDatatype");
}

DXGI_FORMAT ToDxgiFormat(ComponentDatatype datatype, int numberOfComponents, bool normalize) {
    if (numberOfComponents < 1 || numberOfComponents > 4) {
        throw std::invalid_argument("numberOfComponents must be between one and four");
    }

    // One row per component count: [numberOfComponents - 1].
    // DXGI_FORMAT_UNKNOWN marks a combination DXGI has no format for.
    auto pick = [numberOfComponents](DXGI_FORMAT one, DXGI_FORMAT two, DXGI_FORMAT three, DXGI_FORMAT four) {
        const DXGI_FORMAT formats[] = {one, two, three, four};
        return formats[numberOfComponents - 1];
    };

    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    switch (datatype) {
    case ComponentDatatype::Byte:
        format = normalize
            ? pick(DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_SNORM)
            : pick(DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_SINT);
        break;
    case ComponentDatatype::UnsignedByte:
        format = normalize
            ? pick(DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM)
            : pick(DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UINT);
        break;
    case ComponentDatatype::Short:
        format = normalize
            ? pick(DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_SNORM)
            : pick(DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_SINT);
        break;
    case ComponentDatatype::UnsignedShort:
        format = normalize
            ? pick(DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_UNORM)
            : pick(DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_UINT);
        break;
    case ComponentDatatype::Int:
        // DXGI has no normalized 32-bit integer formats.
        format = normalize
            ? DXGI_FORMAT_UNKNOWN
            : pick(DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32B32_SINT, DXGI_FORMAT_R32G32B32A32_SINT);
        break;
    case ComponentDatatype::UnsignedInt:
        format = normalize
            ? DXGI_FORMAT_UNKNOWN
            : pick(DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32B32_UINT, DXGI_FORMAT_R32G32B32A32_UINT);
        break;
    case ComponentDatatype::Float:
        format = pick(DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT,
                      DXGI_FORMAT_R32G32B32A32_FLOAT);
        break;
    case ComponentDatatype::HalfFloat:
        format = pick(DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN,
                      DXGI_FORMAT_R16G16B16A16_FLOAT);
        break;
    }

    if (format == DXGI_FORMAT_UNKNOWN) {
        throw std::invalid_argument(std::format(
            "Direct3D 11 has no vertex format for {} components of this ComponentDatatype{}. "
            "8- and 16-bit attributes need 1, 2 or 4 components; pad to 4.",
            numberOfComponents, normalize ? " (normalized)" : ""));
    }
    return format;
}

// --- Step 5: textures and samplers ------------------------------------------

DxgiTextureFormats ToDxgiFormats(TextureFormat format) {
    // Color formats: the resource, its render target view and its shader
    // resource view all use the same format.
    auto same = [](DXGI_FORMAT f) { return DxgiTextureFormats{f, f, f}; };
    auto unsupported = [format](const char* why) -> DxgiTextureFormats {
        throw InsufficientVideoCardException(
            std::format("TextureFormat {} is not supported by Direct3D 11: {}", static_cast<int>(format), why));
    };

    switch (format) {
    // 3-channel normalized and float formats have no DXGI equivalent. They are
    // stored with 4 channels, and Texture2DD3D11 pads the data (alpha = 1).
    case TextureFormat::RedGreenBlue8:         return same(DXGI_FORMAT_R8G8B8A8_UNORM);
    case TextureFormat::RedGreenBlue16:        return same(DXGI_FORMAT_R16G16B16A16_UNORM);
    case TextureFormat::RedGreenBlueAlpha8:    return same(DXGI_FORMAT_R8G8B8A8_UNORM);
    case TextureFormat::RedGreenBlue10A2:      return same(DXGI_FORMAT_R10G10B10A2_UNORM);
    case TextureFormat::RedGreenBlueAlpha16:   return same(DXGI_FORMAT_R16G16B16A16_UNORM);

    // Depth formats: a typeless resource, a D* depth-stencil view, and an R* shader resource view.
    case TextureFormat::Depth16:
        return {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_R16_UNORM};
    case TextureFormat::Depth24:   // DXGI has no 24-bit depth without stencil; the stencil bits go unused
        return {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS};

    case TextureFormat::Red8:                  return same(DXGI_FORMAT_R8_UNORM);
    case TextureFormat::Red16:                 return same(DXGI_FORMAT_R16_UNORM);
    case TextureFormat::RedGreen8:             return same(DXGI_FORMAT_R8G8_UNORM);
    case TextureFormat::RedGreen16:            return same(DXGI_FORMAT_R16G16_UNORM);
    case TextureFormat::Red16f:                return same(DXGI_FORMAT_R16_FLOAT);
    case TextureFormat::Red32f:                return same(DXGI_FORMAT_R32_FLOAT);
    case TextureFormat::RedGreen16f:           return same(DXGI_FORMAT_R16G16_FLOAT);
    case TextureFormat::RedGreen32f:           return same(DXGI_FORMAT_R32G32_FLOAT);
    case TextureFormat::Red8i:                 return same(DXGI_FORMAT_R8_SINT);
    case TextureFormat::Red8ui:                return same(DXGI_FORMAT_R8_UINT);
    case TextureFormat::Red16i:                return same(DXGI_FORMAT_R16_SINT);
    case TextureFormat::Red16ui:               return same(DXGI_FORMAT_R16_UINT);
    case TextureFormat::Red32i:                return same(DXGI_FORMAT_R32_SINT);
    case TextureFormat::Red32ui:               return same(DXGI_FORMAT_R32_UINT);
    case TextureFormat::RedGreen8i:            return same(DXGI_FORMAT_R8G8_SINT);
    case TextureFormat::RedGreen8ui:           return same(DXGI_FORMAT_R8G8_UINT);
    case TextureFormat::RedGreen16i:           return same(DXGI_FORMAT_R16G16_SINT);
    case TextureFormat::RedGreen16ui:          return same(DXGI_FORMAT_R16G16_UINT);
    case TextureFormat::RedGreen32i:           return same(DXGI_FORMAT_R32G32_SINT);
    case TextureFormat::RedGreen32ui:          return same(DXGI_FORMAT_R32G32_UINT);
    case TextureFormat::RedGreenBlueAlpha32f:  return same(DXGI_FORMAT_R32G32B32A32_FLOAT);
    // R32G32B32_FLOAT exists, but it can't be a render target or be filtered on
    // every GPU; storing 4 channels keeps RGB32f usable everywhere RGBA32f is.
    case TextureFormat::RedGreenBlue32f:       return same(DXGI_FORMAT_R32G32B32A32_FLOAT);
    case TextureFormat::RedGreenBlueAlpha16f:  return same(DXGI_FORMAT_R16G16B16A16_FLOAT);
    case TextureFormat::RedGreenBlue16f:       return same(DXGI_FORMAT_R16G16B16A16_FLOAT);
    case TextureFormat::Depth24Stencil8:
        return {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS};
    case TextureFormat::Red11fGreen11fBlue10f: return same(DXGI_FORMAT_R11G11B10_FLOAT);
    case TextureFormat::RedGreenBlue9E5:       return same(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
    case TextureFormat::SRedGreenBlue8:        return same(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    case TextureFormat::SRedGreenBlue8Alpha8:  return same(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    case TextureFormat::Depth32f:
        return {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT};
    case TextureFormat::Depth32fStencil8:
        return {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS};
    case TextureFormat::RedGreenBlueAlpha32ui: return same(DXGI_FORMAT_R32G32B32A32_UINT);
    case TextureFormat::RedGreenBlue32ui:      return same(DXGI_FORMAT_R32G32B32_UINT);
    case TextureFormat::RedGreenBlueAlpha16ui: return same(DXGI_FORMAT_R16G16B16A16_UINT);
    case TextureFormat::RedGreenBlue16ui:      return unsupported("there is no 3-channel 16-bit integer format");
    case TextureFormat::RedGreenBlueAlpha8ui:  return same(DXGI_FORMAT_R8G8B8A8_UINT);
    case TextureFormat::RedGreenBlue8ui:       return unsupported("there is no 3-channel 8-bit integer format");
    case TextureFormat::RedGreenBlueAlpha32i:  return same(DXGI_FORMAT_R32G32B32A32_SINT);
    case TextureFormat::RedGreenBlue32i:       return same(DXGI_FORMAT_R32G32B32_SINT);
    case TextureFormat::RedGreenBlueAlpha16i:  return same(DXGI_FORMAT_R16G16B16A16_SINT);
    case TextureFormat::RedGreenBlue16i:       return unsupported("there is no 3-channel 16-bit integer format");
    case TextureFormat::RedGreenBlueAlpha8i:   return same(DXGI_FORMAT_R8G8B8A8_SINT);
    case TextureFormat::RedGreenBlue8i:        return unsupported("there is no 3-channel 8-bit integer format");
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

D3D11_FILTER ToD3DFilter(TextureMinificationFilter minification, TextureMagnificationFilter magnification,
                         bool anisotropic) {
    if (anisotropic) {
        return D3D11_FILTER_ANISOTROPIC;   // anisotropic minification, magnification and mip selection
    }

    // GL names a minification filter <within a level>Mipmap<between levels>.
    D3D11_FILTER_TYPE minType = D3D11_FILTER_TYPE_POINT;
    D3D11_FILTER_TYPE mipType = D3D11_FILTER_TYPE_POINT;
    switch (minification) {
    case TextureMinificationFilter::Nearest:              minType = D3D11_FILTER_TYPE_POINT;  mipType = D3D11_FILTER_TYPE_POINT;  break;
    case TextureMinificationFilter::Linear:               minType = D3D11_FILTER_TYPE_LINEAR; mipType = D3D11_FILTER_TYPE_POINT;  break;
    case TextureMinificationFilter::NearestMipmapNearest: minType = D3D11_FILTER_TYPE_POINT;  mipType = D3D11_FILTER_TYPE_POINT;  break;
    case TextureMinificationFilter::LinearMipmapNearest:  minType = D3D11_FILTER_TYPE_LINEAR; mipType = D3D11_FILTER_TYPE_POINT;  break;
    case TextureMinificationFilter::NearestMipmapLinear:  minType = D3D11_FILTER_TYPE_POINT;  mipType = D3D11_FILTER_TYPE_LINEAR; break;
    case TextureMinificationFilter::LinearMipmapLinear:   minType = D3D11_FILTER_TYPE_LINEAR; mipType = D3D11_FILTER_TYPE_LINEAR; break;
    default: throw std::invalid_argument("Invalid TextureMinificationFilter");
    }

    D3D11_FILTER_TYPE magType = D3D11_FILTER_TYPE_POINT;
    switch (magnification) {
    case TextureMagnificationFilter::Nearest: magType = D3D11_FILTER_TYPE_POINT;  break;
    case TextureMagnificationFilter::Linear:  magType = D3D11_FILTER_TYPE_LINEAR; break;
    default: throw std::invalid_argument("Invalid TextureMagnificationFilter");
    }

    // The last argument is the reduction type: 0 is the standard (non-comparison) filter.
    return static_cast<D3D11_FILTER>(D3D11_ENCODE_BASIC_FILTER(minType, magType, mipType, 0));
}

bool UsesMipmaps(TextureMinificationFilter minification) {
    return minification != TextureMinificationFilter::Nearest && minification != TextureMinificationFilter::Linear;
}

D3D11_TEXTURE_ADDRESS_MODE ToD3D(TextureWrap wrap) {
    switch (wrap) {
    case TextureWrap::Clamp:          return D3D11_TEXTURE_ADDRESS_CLAMP;    // GL_CLAMP_TO_EDGE
    case TextureWrap::Repeat:         return D3D11_TEXTURE_ADDRESS_WRAP;     // GL_REPEAT
    case TextureWrap::MirroredRepeat: return D3D11_TEXTURE_ADDRESS_MIRROR;   // GL_MIRRORED_REPEAT
    }
    throw std::invalid_argument("Invalid TextureWrap");
}

} // namespace arda::renderer::d3d11
```

- **`unsupported` returns `DxgiTextureFormats` but always throws.** The
  explicit return type (`-> DxgiTextureFormats`) lets `return unsupported(...)`
  compile in a function that must return a value.
- **Two switches with `default:`** in `ToD3DFilter`: those assign to local
  variables and `break`, so without a `default` the compiler couldn't tell
  that the variables are always set. The warning for a missing enumerator is
  lost there; the other switches keep it.

> **Why the stencil operations look swapped:** GL's `GL_INCR` *clamps* at
> the maximum and `GL_INCR_WRAP` wraps to 0. D3D11's `D3D11_STENCIL_OP_INCR`
> *wraps*, and `D3D11_STENCIL_OP_INCR_SAT` clamps. Same for decrement. The
> names match across the APIs, the meanings don't, which makes this a
> classic porting bug: the stencil shadow volumes of a later chapter would
> silently break.

> **Why FrontAndBack, Point mode, LineLoop and TriangleFan throw:** D3D11
> has no equivalent. The portability table keeps `LineLoop` and
> `TriangleFan` in `PrimitiveType` because the book uses them, and D3D11
> throws with a message that says what to use instead. Failing loudly beats
> drawing something different.

### `src/d3d11/StateCacheD3D11.h`

Creating a state object for every draw would be slow, and D3D11 also limits
how many unique state objects can exist (4096 of each kind). So the cache
creates each distinct combination once, keeps it in an `std::unordered_map`,
and returns the same object whenever the same settings are requested again.
The map's key holds exactly the `RenderState` fields that go into the
object, nothing more.

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/core/geometry/WindingOrder.h>
#include <arda/renderer/renderstate/RenderState.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace arda::renderer::d3d11 {

// Mixes value's hash into seed (the boost::hash_combine formula).
template <typename T>
void HashCombine(std::size_t& seed, const T& value) {
    seed ^= std::hash<T>{}(value) + static_cast<std::size_t>(0x9e3779b97f4a7c15ull) + (seed << 6) + (seed >> 2);
}

// The fields of one stencil face that go into D3D11_DEPTH_STENCILOP_DESC.
struct StencilFaceKey {
    StencilOperation stencilFail = StencilOperation::Keep;
    StencilOperation depthFail = StencilOperation::Keep;
    StencilOperation pass = StencilOperation::Keep;
    StencilTestFunction function = StencilTestFunction::Always;

    bool operator==(const StencilFaceKey&) const = default;
};

// Everything in a D3D11_DEPTH_STENCIL_DESC. The stencil reference value is
// not here: it is an argument of OMSetDepthStencilState.
struct DepthStencilKey {
    bool depthTestEnabled = true;
    DepthTestFunction depthFunction = DepthTestFunction::Less;
    bool depthWrite = true;
    bool stencilEnabled = false;
    std::uint8_t stencilReadMask = 0xFF;
    StencilFaceKey front;
    StencilFaceKey back;

    bool operator==(const DepthStencilKey&) const = default;
};

// Everything in a D3D11_BLEND_DESC. The blend color is not here: it is an
// argument of OMSetBlendState.
struct BlendKey {
    bool enabled = false;
    SourceBlendingFactor sourceRGB = SourceBlendingFactor::One;
    SourceBlendingFactor sourceAlpha = SourceBlendingFactor::One;
    DestinationBlendingFactor destinationRGB = DestinationBlendingFactor::Zero;
    DestinationBlendingFactor destinationAlpha = DestinationBlendingFactor::Zero;
    BlendEquation rgbEquation = BlendEquation::Add;
    BlendEquation alphaEquation = BlendEquation::Add;
    ColorMask colorMask;

    bool operator==(const BlendKey&) const = default;
};

// Everything in a D3D11_RASTERIZER_DESC. The scissor rectangle is not here:
// it is set with RSSetScissorRects.
struct RasterizerKey {
    bool cullEnabled = true;
    CullFace cullFace = CullFace::Back;
    core::geometry::WindingOrder frontFace = core::geometry::WindingOrder::Counterclockwise;
    RasterizationMode rasterizationMode = RasterizationMode::Fill;
    bool scissorEnabled = false;

    bool operator==(const RasterizerKey&) const = default;
};

// Build the keys from a RenderState. They validate what D3D11 can't express
// (throwing std::invalid_argument), and normalize fields that don't matter,
// so that, for example, every disabled blend state shares one object.
DepthStencilKey MakeDepthStencilKey(const RenderState& renderState);
BlendKey MakeBlendKey(const RenderState& renderState);
RasterizerKey MakeRasterizerKey(const RenderState& renderState, bool flipFrontFace);

// The dynamic values that go to the bind calls.
UINT StencilReference(const StencilTest& stencilTest);
std::array<float, 4> BlendFactor(const Blending& blending);

} // namespace arda::renderer::d3d11

// std::unordered_map needs std::hash<Key>. Specializations of std templates
// must be declared in namespace std, before the maps below use them.
namespace std {

template <>
struct hash<arda::renderer::d3d11::StencilFaceKey> {
    std::size_t operator()(const arda::renderer::d3d11::StencilFaceKey& key) const noexcept {
        using arda::renderer::d3d11::HashCombine;
        std::size_t seed = 0;
        HashCombine(seed, key.stencilFail);
        HashCombine(seed, key.depthFail);
        HashCombine(seed, key.pass);
        HashCombine(seed, key.function);
        return seed;
    }
};

template <>
struct hash<arda::renderer::d3d11::DepthStencilKey> {
    std::size_t operator()(const arda::renderer::d3d11::DepthStencilKey& key) const noexcept {
        using arda::renderer::d3d11::HashCombine;
        std::size_t seed = 0;
        HashCombine(seed, key.depthTestEnabled);
        HashCombine(seed, key.depthFunction);
        HashCombine(seed, key.depthWrite);
        HashCombine(seed, key.stencilEnabled);
        HashCombine(seed, key.stencilReadMask);
        HashCombine(seed, key.front);   // uses hash<StencilFaceKey> above
        HashCombine(seed, key.back);
        return seed;
    }
};

template <>
struct hash<arda::renderer::d3d11::BlendKey> {
    std::size_t operator()(const arda::renderer::d3d11::BlendKey& key) const noexcept {
        using arda::renderer::d3d11::HashCombine;
        std::size_t seed = 0;
        HashCombine(seed, key.enabled);
        HashCombine(seed, key.sourceRGB);
        HashCombine(seed, key.sourceAlpha);
        HashCombine(seed, key.destinationRGB);
        HashCombine(seed, key.destinationAlpha);
        HashCombine(seed, key.rgbEquation);
        HashCombine(seed, key.alphaEquation);
        HashCombine(seed, key.colorMask.red);
        HashCombine(seed, key.colorMask.green);
        HashCombine(seed, key.colorMask.blue);
        HashCombine(seed, key.colorMask.alpha);
        return seed;
    }
};

template <>
struct hash<arda::renderer::d3d11::RasterizerKey> {
    std::size_t operator()(const arda::renderer::d3d11::RasterizerKey& key) const noexcept {
        using arda::renderer::d3d11::HashCombine;
        std::size_t seed = 0;
        HashCombine(seed, key.cullEnabled);
        HashCombine(seed, key.cullFace);
        HashCombine(seed, key.frontFace);
        HashCombine(seed, key.rasterizationMode);
        HashCombine(seed, key.scissorEnabled);
        return seed;
    }
};

} // namespace std

namespace arda::renderer::d3d11 {

// Owned by DeviceD3D11: state objects are device resources, shared by every window.
class StateCacheD3D11 {
public:
    explicit StateCacheD3D11(ID3D11Device* device) : m_device(device) {}

    StateCacheD3D11(const StateCacheD3D11&)            = delete;
    StateCacheD3D11& operator=(const StateCacheD3D11&) = delete;

    // Each returns the one object for these settings, creating it on first use.
    // The cache keeps the object alive; callers only borrow the pointer.
    ID3D11DepthStencilState* DepthStencil(const DepthStencilKey& key);
    ID3D11BlendState* Blend(const BlendKey& key);
    ID3D11RasterizerState* Rasterizer(const RasterizerKey& key);

private:
    ID3D11Device* m_device;   // the DeviceD3D11 that owns this cache outlives it
    std::unordered_map<DepthStencilKey, ComPtr<ID3D11DepthStencilState>> m_depthStencilStates;
    std::unordered_map<BlendKey, ComPtr<ID3D11BlendState>> m_blendStates;
    std::unordered_map<RasterizerKey, ComPtr<ID3D11RasterizerState>> m_rasterizerStates;
};

} // namespace arda::renderer::d3d11
```

<a id="cpp-std-hash"></a>

> **C++ note — `std::hash` specializations:** `std::unordered_map<K, V>`
> hashes its keys with `std::hash<K>`, a class template the standard library
> provides for built-in types, strings, smart pointers and every `enum`
> ([Step 2](02-shaders.md#cpp-unordered-map)). For your own `K` you have two
> choices: pass a hasher as the third template argument
> (`std::unordered_map<K, V, MyHash>`), or *specialize* `std::hash<K>`. A
> specialization is a second definition of the template, used only for that
> type: `template <> struct hash<DepthStencilKey> { ... };`. The standard
> allows adding specializations to namespace `std` for program-defined types
> (and nothing else). It must be declared before the first use of
> `std::unordered_map<DepthStencilKey, ...>`, which is why the header closes
> arda's namespace, opens `std`, then reopens arda's. The `operator()` is
> `const` and `noexcept`: containers call it through a const hasher, and a
> hash that can throw would make `insert` unsafe. The map also needs
> `operator==`, to tell apart keys whose hashes collide; the defaulted one
> compares every field ([Step 1](01-state-management.md#cpp-defaulted-equality)).

<a id="cpp-hash-combine"></a>

> **C++ note — combining hashes:** a struct's hash must depend on every
> field that `operator==` compares, and equal keys must hash equally.
> XOR-ing the field hashes (`h1 ^ h2`) is the obvious approach and a bad
> one: it is symmetric (swapping two fields gives the same hash), and two
> equal fields cancel out to 0. `HashCombine` is the formula from Boost:
> each step shifts and adds the running seed, so order matters and equal
> values don't cancel. `0x9e3779b97f4a7c15` is 2^64 divided by the golden
> ratio, a constant with well-mixed bits. The `static_cast<std::size_t>`
> truncates it harmlessly on a 32-bit build. Hashing is member by member,
> through `std::hash` of each field's own type, so the key's layout in
> memory never matters.

<a id="cpp-padding"></a>

> **C++ note — why not `std::memcmp` and a hash of the raw bytes:** a
> tempting shortcut is to hash `sizeof(DepthStencilKey)` bytes and compare
> keys with `std::memcmp`. Don't. The compiler inserts *padding* between
> fields to align them ([Step 3](03-vertex-data.md) covers `sizeof` and
> alignment): `DepthStencilKey` starts with a `bool` followed by a 4-byte
> enum, so there are three padding bytes after it. Padding bytes have
> indeterminate values. Two keys with identical fields can differ in their
> padding, so `memcmp` says "different", the cache misses, and a new state
> object is created every frame until D3D11's 4096-object limit makes
> `CreateDepthStencilState` fail. Floats add a second trap: `0.0f` and
> `-0.0f` compare equal but have different bytes, while a NaN has identical
> bytes but compares unequal. Defaulted `operator==` and member-wise hashing
> avoid all of it, which is also why `RenderTargetBinding` and the cache
> keys use `= default`.

> **D3D11 note — the runtime also deduplicates:** if you call
> `CreateBlendState` twice with identical descriptions, D3D11 returns the
> *same* object the second time, with its reference count increased. So the
> cache isn't needed for correctness. It avoids building a description,
> calling into the runtime and hashing a 50-byte struct for every draw, and
> it keeps one reference to each object alive so the runtime's copy isn't
> destroyed and recreated as draws come and go.

### `src/d3d11/StateCacheD3D11.cpp`

```cpp
#include "d3d11/StateCacheD3D11.h"
#include "d3d11/TypeConverterD3D11.h"

#include <stdexcept>
#include <utility>

namespace arda::renderer::d3d11 {

using core::geometry::WindingOrder;

namespace {

StencilFaceKey ToKey(const StencilTestFace& face) {
    return StencilFaceKey{
        .stencilFail = face.stencilFailOperation,
        .depthFail = face.depthFailStencilPassOperation,
        .pass = face.depthPassStencilPassOperation,
        .function = face.function,
    };
}

D3D11_DEPTH_STENCILOP_DESC ToD3D(const StencilFaceKey& key) {
    D3D11_DEPTH_STENCILOP_DESC desc{};
    desc.StencilFailOp = ToD3D(key.stencilFail);
    desc.StencilDepthFailOp = ToD3D(key.depthFail);
    desc.StencilPassOp = ToD3D(key.pass);
    desc.StencilFunc = ToD3D(key.function);
    return desc;
}

template <typename Factor>
bool UsesConstantColor(Factor factor) {
    return factor == Factor::ConstantColor || factor == Factor::OneMinusConstantColor;
}

template <typename Factor>
bool UsesConstantAlpha(Factor factor) {
    return factor == Factor::ConstantAlpha || factor == Factor::OneMinusConstantAlpha;
}

} // namespace

DepthStencilKey MakeDepthStencilKey(const RenderState& renderState) {
    DepthStencilKey key;
    key.depthTestEnabled = renderState.depthTest.enabled;
    // The function doesn't matter when the test is off; normalizing it shares one object.
    key.depthFunction = renderState.depthTest.enabled ? renderState.depthTest.function : DepthTestFunction::Less;
    key.depthWrite = renderState.depthMask;

    const StencilTest& stencil = renderState.stencilTest;
    key.stencilEnabled = stencil.enabled;
    if (stencil.enabled) {
        // GL has a compare mask and a reference value per face; D3D11 has one of each.
        if (stencil.frontFace.mask != stencil.backFace.mask ||
            stencil.frontFace.referenceValue != stencil.backFace.referenceValue) {
            throw std::invalid_argument(
                "Direct3D 11 uses one stencil mask and one reference value for both faces: "
                "renderState.stencilTest.frontFace and backFace must have the same mask and referenceValue.");
        }
        key.stencilReadMask = static_cast<std::uint8_t>(stencil.frontFace.mask & 0xFF);
        key.front = ToKey(stencil.frontFace);
        key.back = ToKey(stencil.backFace);
    }
    return key;
}

BlendKey MakeBlendKey(const RenderState& renderState) {
    BlendKey key;
    key.colorMask = renderState.colorMask;

    const Blending& blending = renderState.blending;
    key.enabled = blending.enabled;
    if (blending.enabled) {   // otherwise keep the defaults, so every disabled state is equal
        key.sourceRGB = blending.sourceRGBFactor;
        key.sourceAlpha = blending.sourceAlphaFactor;
        key.destinationRGB = blending.destinationRGBFactor;
        key.destinationAlpha = blending.destinationAlphaFactor;
        key.rgbEquation = blending.rgbEquation;
        key.alphaEquation = blending.alphaEquation;
    }
    return key;
}

RasterizerKey MakeRasterizerKey(const RenderState& renderState, bool flipFrontFace) {
    RasterizerKey key;

    const FacetCulling& culling = renderState.facetCulling;
    key.cullEnabled = culling.enabled;
    if (culling.enabled) {
        ToD3D(culling.face);   // throws for FrontAndBack, here rather than at object creation
        key.cullFace = culling.face;
    }

    // Flipping clip-space y (Step 4 on D3D11) mirrors every triangle, which
    // reverses its winding on screen. Reverse the front face to match.
    key.frontFace = culling.frontFaceWindingOrder;
    if (flipFrontFace) {
        key.frontFace = key.frontFace == WindingOrder::Counterclockwise ? WindingOrder::Clockwise
                                                                        : WindingOrder::Counterclockwise;
    }

    ToD3D(renderState.rasterizationMode);   // throws for Point
    key.rasterizationMode = renderState.rasterizationMode;
    key.scissorEnabled = renderState.scissorTest.enabled;
    return key;
}

UINT StencilReference(const StencilTest& stencilTest) {
    return stencilTest.enabled ? static_cast<UINT>(stencilTest.frontFace.referenceValue) & 0xFFu : 0u;
}

std::array<float, 4> BlendFactor(const Blending& blending) {
    if (!blending.enabled) {
        return {1.0f, 1.0f, 1.0f, 1.0f};   // unused; a fixed value keeps the cache comparison stable
    }

    const bool color = UsesConstantColor(blending.sourceRGBFactor) || UsesConstantColor(blending.sourceAlphaFactor) ||
                       UsesConstantColor(blending.destinationRGBFactor) ||
                       UsesConstantColor(blending.destinationAlphaFactor);
    const bool alpha = UsesConstantAlpha(blending.sourceRGBFactor) || UsesConstantAlpha(blending.sourceAlphaFactor) ||
                       UsesConstantAlpha(blending.destinationRGBFactor) ||
                       UsesConstantAlpha(blending.destinationAlphaFactor);

    const Color& c = blending.color;
    if (color && alpha) {
        throw std::invalid_argument(
            "Direct3D 11 has one blend constant: a blend state can't use both ConstantColor and ConstantAlpha factors.");
    }
    if (alpha) {
        return {c.alpha, c.alpha, c.alpha, c.alpha};   // GL's constant alpha, as D3D's per-channel factor
    }
    return {c.red, c.green, c.blue, c.alpha};
}

ID3D11DepthStencilState* StateCacheD3D11::DepthStencil(const DepthStencilKey& key) {
    if (auto it = m_depthStencilStates.find(key); it != m_depthStencilStates.end()) {
        return it->second.Get();
    }

    D3D11_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = key.depthTestEnabled ? TRUE : FALSE;
    desc.DepthWriteMask = key.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = ToD3D(key.depthFunction);
    desc.StencilEnable = key.stencilEnabled ? TRUE : FALSE;
    desc.StencilReadMask = key.stencilReadMask;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;   // 0xFF, like GL's untouched glStencilMask
    desc.FrontFace = ToD3D(key.front);
    desc.BackFace = ToD3D(key.back);

    // Create first, insert second: if creation throws, the map is unchanged.
    ComPtr<ID3D11DepthStencilState> state;
    ThrowIfFailed(m_device->CreateDepthStencilState(&desc, &state), "CreateDepthStencilState");
    return m_depthStencilStates.emplace(key, std::move(state)).first->second.Get();
}

ID3D11BlendState* StateCacheD3D11::Blend(const BlendKey& key) {
    if (auto it = m_blendStates.find(key); it != m_blendStates.end()) {
        return it->second.Get();
    }

    D3D11_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;   // RenderTarget[0] applies to every render target, as in GL 3.3

    D3D11_RENDER_TARGET_BLEND_DESC& target = desc.RenderTarget[0];
    target.BlendEnable = key.enabled ? TRUE : FALSE;
    target.SrcBlend = ToD3D(key.sourceRGB);
    target.DestBlend = ToD3D(key.destinationRGB);
    target.BlendOp = ToD3D(key.rgbEquation);
    target.SrcBlendAlpha = ToD3DAlpha(key.sourceAlpha);
    target.DestBlendAlpha = ToD3DAlpha(key.destinationAlpha);
    target.BlendOpAlpha = ToD3D(key.alphaEquation);
    target.RenderTargetWriteMask = ToD3D(key.colorMask);

    ComPtr<ID3D11BlendState> state;
    ThrowIfFailed(m_device->CreateBlendState(&desc, &state), "CreateBlendState");
    return m_blendStates.emplace(key, std::move(state)).first->second.Get();
}

ID3D11RasterizerState* StateCacheD3D11::Rasterizer(const RasterizerKey& key) {
    if (auto it = m_rasterizerStates.find(key); it != m_rasterizerStates.end()) {
        return it->second.Get();
    }

    D3D11_RASTERIZER_DESC desc{};
    desc.FillMode = ToD3D(key.rasterizationMode);
    desc.CullMode = key.cullEnabled ? ToD3D(key.cullFace) : D3D11_CULL_NONE;
    desc.FrontCounterClockwise = key.frontFace == WindingOrder::Counterclockwise ? TRUE : FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;   // clip against the near and far planes, as GL always does
    desc.ScissorEnable = key.scissorEnabled ? TRUE : FALSE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    ComPtr<ID3D11RasterizerState> state;
    ThrowIfFailed(m_device->CreateRasterizerState(&desc, &state), "CreateRasterizerState");
    return m_rasterizerStates.emplace(key, std::move(state)).first->second.Get();
}

} // namespace arda::renderer::d3d11
```

- **`if (auto it = ...; it != end)`** is an `if` with an initializer: `it`
  exists only inside the `if` and its `else`.
- **Create, then insert.** A version that does
  `auto [it, inserted] = map.try_emplace(key)` first and creates the object
  into `it->second` leaves an empty `ComPtr` in the map if creation throws,
  and the next lookup returns `nullptr`. Creating into a local first avoids
  that.
- **`BlendFactor` and `StencilReference`** return the values that aren't
  in the objects. `ContextD3D11` passes them to the bind calls.
- **`UsesConstantColor` is a function template** over the factor type, so
  one function works for both `SourceBlendingFactor` and
  `DestinationBlendingFactor`, which both have enumerators with these names.

> **Why the stencil check throws:** GL lets front and back faces use
> different compare masks and reference values (`glStencilFuncSeparate`).
> D3D11 has one `StencilReadMask` and one reference value for both.
> Silently using the front face's values would give different results on
> the two backends; the exception names the fields to fix. The defaults
> (both faces `mask = ~0`, `referenceValue = 0`) always pass.

### `DeviceD3D11`: the state cache

State objects are device resources, shared by every window, so the device
owns the cache. Add to `src/d3d11/DeviceD3D11.h`, next to the other
includes:

```cpp
#include "d3d11/StateCacheD3D11.h"
```

in the `public:` section, after `Immediate()`:

```cpp
    StateCacheD3D11& StateCache() { return *m_stateCache; }
```

and in the `private:` section, after `m_immediate`:

```cpp
    std::unique_ptr<StateCacheD3D11> m_stateCache;   // created once m_device exists
```

In `src/d3d11/DeviceD3D11.cpp`, create it right after
`ThrowIfFailed(hr, "D3D11CreateDevice");`:

```cpp
    m_stateCache = std::make_unique<StateCacheD3D11>(m_device.Get());
```

Members are destroyed in reverse order of declaration, so the cache (and
its state objects) goes before `m_immediate` and `m_device`
([Step 0](00-setup.md#cpp-member-order)).

### `ContextD3D11`: clear, render state and viewport

Add `#include "d3d11/StateCacheD3D11.h"` to `ContextD3D11.cpp`, and replace
the `DoClear` stub with these functions.

```cpp
void ContextD3D11::DoClear(const ClearState& clearState) {
    EnsureActive();
    ApplyFramebuffer();   // clears write to the current render targets, like draws
    const RenderTargetBinding& binding = *m_renderTargets;

    if (HasFlag(clearState.buffers, ClearBuffers::ColorBuffer)) {
        const Color& c = clearState.color;
        const float color[4] = {c.red, c.green, c.blue, c.alpha};
        for (UINT i = 0; i < binding.count; ++i) {
            if (binding.views[i] != nullptr) {   // an empty attachment point
                Immediate()->ClearRenderTargetView(binding.views[i], color);
            }
        }
    }

    UINT depthStencilFlags = 0;
    if (HasFlag(clearState.buffers, ClearBuffers::DepthBuffer)) {
        depthStencilFlags |= D3D11_CLEAR_DEPTH;
    }
    if (HasFlag(clearState.buffers, ClearBuffers::StencilBuffer) && m_depthStencilHasStencil) {
        depthStencilFlags |= D3D11_CLEAR_STENCIL;
    }
    if (depthStencilFlags != 0 && binding.depthStencil != nullptr) {
        Immediate()->ClearDepthStencilView(binding.depthStencil, depthStencilFlags,
                                           clearState.depth, static_cast<UINT8>(clearState.stencil));
    }
}

// Step 1 version: only the window's back buffer. Step 6 replaces this function.
void ContextD3D11::ApplyFramebuffer() {
    if (GetFramebuffer()) {
        throw std::logic_error("Framebuffers are not implemented yet on Direct3D 11 (Step 6)");
    }

    RenderTargetBinding binding;
    binding.views[0] = m_window.BackBufferView();
    binding.count = 1;
    binding.depthStencil = m_window.DepthStencilView();
    m_depthStencilHasStencil = true;   // the window's depth buffer is D24_UNORM_S8_UINT

    if (m_renderTargets != binding) {
        Immediate()->OMSetRenderTargets(binding.count, binding.views.data(), binding.depthStencil);
        m_renderTargets = binding;
    }
}

void ContextD3D11::ApplyRenderState(const RenderState& renderState) {
    StateCacheD3D11& cache = m_deviceD3D11.StateCache();

    ID3D11DepthStencilState* depthStencil = cache.DepthStencil(MakeDepthStencilKey(renderState));
    const UINT stencilReference = StencilReference(renderState.stencilTest);
    if (m_depthStencilState != depthStencil || m_stencilReference != stencilReference) {
        Immediate()->OMSetDepthStencilState(depthStencil, stencilReference);
        m_depthStencilState = depthStencil;
        m_stencilReference = stencilReference;
    }

    ID3D11BlendState* blend = cache.Blend(MakeBlendKey(renderState));
    const std::array<float, 4> blendFactor = BlendFactor(renderState.blending);
    if (m_blendState != blend || m_blendFactor != blendFactor) {
        Immediate()->OMSetBlendState(blend, blendFactor.data(), 0xFFFFFFFFu);   // all samples
        m_blendState = blend;
        m_blendFactor = blendFactor;
    }

    ID3D11RasterizerState* rasterizer = cache.Rasterizer(MakeRasterizerKey(renderState, FlipsRenderTargetY()));
    if (m_rasterizerState != rasterizer) {
        Immediate()->RSSetState(rasterizer);
        m_rasterizerState = rasterizer;
    }

    ApplyScissorTest(renderState.scissorTest);
    ApplyViewport(renderState.depthRange);

    // Nothing to apply for the rest:
    // - programPointSize: D3D11 always rasterizes points as one pixel.
    // - primitiveRestart: D3D11 always restarts strips at the maximum index (see ApplyPrimitiveTopology).
}

void ContextD3D11::ApplyScissorTest(const ScissorTest& scissorTest) {
    const Rectangle& r = scissorTest.rectangle;
    if (r.width < 0 || r.height < 0) {
        throw std::invalid_argument("renderState.scissorTest.rectangle width and height must be >= 0");
    }
    if (!scissorTest.enabled) {
        return;   // ScissorEnable is FALSE in the rasterizer state, so the rectangle is ignored
    }

    const LONG top = ToTopLeftY(r.bottom, r.height);
    const std::array<LONG, 4> rectangle = {r.left, top, r.Right(), top + r.height};
    if (m_scissorRectangle != rectangle) {
        const D3D11_RECT d3dRectangle{rectangle[0], rectangle[1], rectangle[2], rectangle[3]};
        Immediate()->RSSetScissorRects(1, &d3dRectangle);
        m_scissorRectangle = rectangle;
    }
}

void ContextD3D11::ApplyViewport(const DepthRange& depthRange) {
    if (depthRange.nearValue < 0.0 || depthRange.nearValue > 1.0 ||
        depthRange.farValue < 0.0 || depthRange.farValue > 1.0) {
        throw std::invalid_argument("renderState.depthRange values must be in [0, 1]");
    }

    const Rectangle& v = GetViewport();
    const std::array<float, 6> viewport = {
        static_cast<float>(v.left),
        static_cast<float>(ToTopLeftY(v.bottom, v.height)),
        static_cast<float>(v.width),
        static_cast<float>(v.height),
        static_cast<float>(depthRange.nearValue),   // GL's glDepthRange is the viewport's depth range in D3D
        static_cast<float>(depthRange.farValue),
    };
    if (m_appliedViewport != viewport) {
        const D3D11_VIEWPORT d3dViewport{viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]};
        Immediate()->RSSetViewports(1, &d3dViewport);
        m_appliedViewport = viewport;
    }
}

int ContextD3D11::ToTopLeftY(int bottom, int height) const {
    if (GetFramebuffer()) {
        // While a framebuffer is bound, clip-space y is flipped (Step 4 on D3D11),
        // so D3D's rows from the top are exactly GL's rows from the bottom.
        return bottom;
    }
    return m_window.BackBufferHeight() - (bottom + height);
}
```

- **`D3D11_VIEWPORT` field order** is `TopLeftX, TopLeftY, Width, Height,
  MinDepth, MaxDepth`, which is the order of the cached array.
- **`MinDepth`/`MaxDepth`** do what `glDepthRange` does. D3D11 has no
  separate call for it.
- **The viewport is applied at draw time, not in `DoSetViewport`**, because
  the conversion needs the render target's height and the draw's depth
  range. Clears don't need it; they ignore the viewport.
- **`0xFFFFFFFFu`** as `OMSetBlendState`'s sample mask means "write every
  sample", which is what GL does without `GL_SAMPLE_MASK`.

### Milestone: the clear color

The render-frame handler from [Step 1](01-state-management.md), with the
device created as Direct3D 11:

```cpp
auto device = CreateDevice(GraphicsApi::Direct3D11);
auto window = device->CreateGraphicsWindow(1280, 720, "Arda (Direct3D 11)");
Context& context = window->GetContext();

ClearState clearState;
clearState.color = {0.02f, 0.05f, 0.12f, 1.0f};

window->SetResizeHandler([&] { context.SetViewport({0, 0, window->Width(), window->Height()}); });
window->SetRenderFrameHandler([&] { context.Clear(clearState); });
window->Run();
```

(Use it as a temporary `main`, or put it behind the command-line switch
from Step 7.) The window fills with the clear color, stays filled while you
resize it, and the debug output stays empty.

`ClearRenderTargetView` takes the view as an argument, so a clear works even
when the view isn't bound. A missing `ForgetRenderTargets` in `SwapBuffers`
therefore doesn't show up yet. It shows up in Step 3: the first frame draws
the triangle, and every later frame draws nothing, because the cache
believes the back buffer is still bound after `Present` unbound it.

---

## Step 2 on D3D11: HLSL, reflection and constant buffers

This step mirrors [Step 2](02-shaders.md). The public side
(`ShaderProgram`, `Uniform<T>`, `UniformCollection`,
`ShaderVertexAttributeCollection`) is unchanged. `ShaderProgramD3D11`
compiles HLSL instead of GLSL and fills the same collections from D3D11's
shader reflection.

### HLSL conventions

These are the portability table's decisions, as they look in a shader.

```hlsl
// Loose globals are uniforms. The compiler packs them into a constant buffer named $Globals.
float4x4 og_modelViewPerspectiveMatrix;
float3 u_color;

// Vertex shader inputs: semantic name = attribute name, semantic index = location.
// "position0" is semantic "position", index 0 (VertexLocations::Position).
// "textureCoordinate3" is semantic "textureCoordinate", index 3 (VertexLocations::TextureCoordinate).
float4 main(float4 position : position0) : SV_Position
{
    return mul(og_modelViewPerspectiveMatrix, position);   // M * v, as in GLSL
}

// Pixel shader outputs: SV_TargetN writes color attachment N. Declare the
// output as a named out parameter (or struct member), so that
// FragmentOutputLocation("fragmentColor") can find it.
void main(out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(u_color, 1.0);
}

// Texture unit N is texture register tN and sampler register sN (Step 5).
Texture2D og_texture0 : register(t0);
SamplerState og_sampler0 : register(s0);
```

> **Why each backend takes its own shader language (portability table,
> "Shader language"):** GLSL and HLSL differ in more than syntax: matrix
> multiplication, built-in names, how inputs and outputs are declared,
> texture and sampler objects. A translator (SPIRV-Cross, glslang,
> ShaderConductor) could hide that, but it's a big dependency and a second
> thing to debug. The book's renderer takes source in the backend's
> language, and so does arda: `CreateShaderProgram` compiles whatever it is
> given, and the application picks the source with `device->Api()`, as
> Step 7's `TriangleShaders` does.

> **Why semantics carry the attribute name and location (portability
> table, "Vertex input names"):** GL identifies a vertex input by name and
> location (`layout(location = 0) in vec4 position`). D3D11 identifies it by
> a *semantic*: a name and an index (`position0` is name `position`, index
> 0). The mapping `name : nameN` means reflection gives the backend both
> values arda needs: `SemanticName` becomes `ShaderVertexAttribute::name`,
> which `CreateMeshBuffers` matches against mesh attribute names, and
> `SemanticIndex` becomes `location`, which selects the vertex array slot.
> The parameter name (`position` before the colon) is ignored by D3D; only
> the semantic matters.
>
> **Pitfall:** the semantic index must be a literal. `og_positionVertexLocation`
> can't be pasted into a semantic, so write the numbers from
> `VertexLocations.h` by hand: `position0`, `normal2`, `textureCoordinate3`,
> `color4`. And a name that ends in a digit is split at the digits:
> `color1` is `color` index 1, so attribute names used as semantics must not
> end in a digit.

> **Why `SV_TargetN` (portability table, "Fragment outputs"):** GL's
> `layout(location = N) out` and D3D's `SV_TargetN` both write color
> attachment N, so a framebuffer set up with
> `FragmentOutputLocation(...)` works on both. What D3D11's reflection does
> *not* provide is the output's name: an output parameter is reported as
> "semantic `SV_Target`, index 0", and the variable name is gone after
> compilation. `ShaderProgramD3D11` recovers the names by scanning the pixel
> shader's source for `name : SV_TargetN` (an `out` parameter or a struct
> member), and also accepts the semantic itself
> (`FragmentOutputLocation("SV_Target0")`). A shader that only returns a
> value (`float4 main() : SV_Target0`) has no output name, which is fine
> unless the application asks for one.

> **Why column-major, and what the compile flag really does (portability
> table, "Matrix layout"):** a 4×4 matrix in a constant buffer occupies four
> 16-byte registers. *Packing* decides what each register holds: a column
> (`column_major`) or a row (`row_major`). HLSL's default is already
> `column_major`, the same as GLSL and as `core::Matrix4::Data()`, so the 64
> bytes GL gets from `glUniformMatrix4fv(..., GL_FALSE, m.Data())` are the
> same 64 bytes D3D11 needs. The README's table says HLSL defaults to
> row-major; it doesn't. The confusion comes from DirectXMath, whose
> `XMMATRIX` is stored row by row and is used with row vectors
> (`mul(v, M)`), so D3D samples transpose matrices before uploading.
> `D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR` makes arda's choice explicit, so it
> doesn't depend on a default or on a `/Zpr` build flag. Packing and
> multiplication order are separate decisions: with column-major packing,
> `mul(M, v)` treats `v` as a column vector and computes `M · v`, exactly
> like GLSL's `M * v`. Writing `mul(v, M)` computes `Mᵀ · v`, a different
> result. A `row_major` keyword in a shader overrides the flag for that
> variable; `ShaderProgramD3D11` rejects it, because arda's matrices are
> column-major bytes.

> **Why `$Globals` and not explicit constant buffers (portability table,
> "Loose uniforms"):** D3D11 has no `glUniform*`; all shader constants live
> in *constant buffers*. HLSL still accepts loose global variables, and the
> compiler packs them into an implicit constant buffer called `$Globals`
> (one per stage). Keeping `Uniform<T>` means the book's code
> (`sp.Uniforms["u_color"]`), the automatic uniforms (3.4.5) and the tests
> work unchanged. `UniformD3D11<T>` writes into a CPU copy of `$Globals`, and
> the program uploads each stage's copy once before a draw if anything
> changed. Explicit `cbuffer` blocks are ignored, just as the GL backend
> ignores uniform blocks.

> **D3D11 note — no link step:** a GL program is compiled per stage and then
> *linked*, and the linker matches vertex outputs to fragment inputs by name.
> D3D11 has no program object. Each stage is compiled on its own
> (`D3DCompile` with target `vs_5_0`, `gs_5_0` or `ps_5_0`), created as its
> own object (`ID3D11VertexShader`, ...), and bound on its own
> (`VSSetShader`, ...). Vertex outputs meet pixel inputs by *semantic*, and
> a mismatch is only reported by the debug layer at draw time.
> `ShaderProgramD3D11` groups the stages into one arda program, and checks
> the one thing a linker would have caught: that every pixel shader input is
> written by the previous stage.

> **D3D11 note — shader reflection:** `D3DReflect` wraps a compiled shader in
> an `ID3D11ShaderReflection`, which describes it: input parameters (the
> *input signature*), output parameters, constant buffers and their
> variables with byte offsets and types, and resource bindings (which
> textures, samplers and constant buffers sit in which registers). It is
> D3D11's `glGetActiveAttrib` and `glGetActiveUniform`. Variables the shader
> never reads are still listed, but without the `D3D_SVF_USED` flag; arda
> skips them, the same way GL only reports *active* uniforms.

### Constant buffers and the packing rules

The compiler places each `$Globals` variable at a byte offset, following
HLSL's packing rules. Reflection reports every offset, so arda never
computes one, but you need the rules to understand what reflection returns
and to avoid a classic bug if you ever mirror a constant buffer in a C++
struct.

1. A constant buffer is an array of 16-byte **registers** (`float4`s).
2. Variables are placed in declaration order, each at the next offset that
   fits its size and alignment.
3. **A variable may not straddle a 16-byte boundary.** If it doesn't fit in
   what is left of the current register, it moves to the next one.
4. **Matrices and arrays always start a new register.** A column-major matrix
   takes one register per column; each array element takes a whole register.
5. `bool` is 4 bytes, like `int` and `float`.
6. The buffer's total size is rounded up to a multiple of 16.

For example:

```hlsl
float    u_a;   // offset   0   register 0, x
float3   u_b;   // offset   4   register 0, yzw: 12 bytes fit in the 12 that are left
float2   u_c;   // offset  16   register 1, xy
float3   u_d;   // offset  32   16 + 8 + 12 = 36 would cross 32, so it starts register 2
float4x4 u_m;   // offset  48   registers 3-6: a matrix starts a new register
float    u_e;   // offset 112   register 7, x
                // size: 128 (116, rounded up to a multiple of 16)
```

And the non-square matrices, the reason `UniformD3D11` can't just copy
`Data()` for every matrix type:

```hlsl
float3x3 u_normalMatrix;   // column-major: 3 registers, each holding one column of 3 floats.
                           // Bytes 0-11, 16-27, 32-43: 44 bytes, with a 4-byte gap after each column.
```

`core::Matrix3<float>` stores its 9 floats contiguously (36 bytes), so its
columns must be scattered to offsets 0, 16 and 32. `Matrix4<float>` (and any
matrix with 4 rows) fills its registers completely, so its bytes can be
copied unchanged.

<a id="cpp-alignas"></a>

> **C++ note — `alignas(16)` and mirroring a constant buffer in C++:** many
> D3D11 programs declare explicit constant buffers and a C++ struct with the
> same layout, then copy the whole struct with one `memcpy`:
> ```cpp
> // HLSL:  cbuffer PerDraw : register(b1) { float4x4 world; float3 lightDirection; float intensity; float3 color; };
> struct alignas(16) PerDrawConstants {
>     float world[16];            // offset  0
>     float lightDirection[3];    // offset 64
>     float intensity;            // offset 76: shares lightDirection's register, as in HLSL
>     alignas(16) float color[3]; // offset 80: HLSL starts a new register; alignas makes C++ agree
> };
> static_assert(sizeof(PerDrawConstants) == 96);   // a multiple of 16, as CreateBuffer requires
> static_assert(offsetof(PerDrawConstants, color) == 80);
> ```
> `alignas(16)` on the *struct* makes its alignment 16, which also rounds
> `sizeof` up to a multiple of 16 (a constant buffer's `ByteWidth` must be
> one). `alignas(16)` on a *member* forces that member to a 16-byte offset,
> which is how you reproduce rule 3 by hand. Without it, `float3 a; float3 b;`
> sits at offsets 0 and 12 in C++ but 0 and 16 in HLSL, and every field after
> it is read from the wrong place: the most common constant-buffer bug. The
> `static_assert`s ([Step 3](03-vertex-data.md)) turn such a mismatch into a
> compile error. arda avoids the whole problem by writing each uniform at
> the offset reflection reports, but it's worth knowing the struct approach
> when you read other D3D11 code.

> **D3D11 note — updating a constant buffer: `Map` with
> `D3D11_MAP_WRITE_DISCARD` vs. `UpdateSubresource`:** a buffer created with
> `D3D11_USAGE_DYNAMIC` and `D3D11_CPU_ACCESS_WRITE` can be *mapped*: `Map`
> returns a CPU pointer, you write, and `Unmap` hands the memory back.
> `WRITE_DISCARD` tells the driver you'll overwrite the whole buffer, so
> instead of waiting for the GPU to finish the draws that still read the old
> contents, it gives you a *fresh* piece of memory and quietly swaps it in
> (this is called *renaming*). That is exactly the pattern for a buffer
> rewritten every frame, and it is why `ConstantBufferD3D11` keeps a full CPU
> copy: `DISCARD` needs you to write every byte, not only the ones that
> changed. `UpdateSubresource` is the alternative for `D3D11_USAGE_DEFAULT`
> resources: it copies from a CPU pointer through a driver-managed staging
> area. It is simpler and fine for data that changes rarely, which is how
> arda's vertex buffers and textures are updated (Steps 3 and 5).

### `src/d3d11/shaders/HlslPrelude.h` and `.cpp`

The HLSL twin of `GlslPrelude()`, built from the same table.

```cpp
// src/d3d11/shaders/HlslPrelude.h
#pragma once

#include <string>

namespace arda::renderer::d3d11 {

// A #define for each built-in define, and a static const float for each
// built-in constant (the HLSL version of GlslPrelude). Built once.
const std::string& HlslPrelude();

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/shaders/HlslPrelude.cpp
#include "d3d11/shaders/HlslPrelude.h"
#include "shaders/BuiltinConstants.h"

#include <format>

namespace arda::renderer::d3d11 {

namespace {

std::string BuildHlslPrelude() {
    std::string prelude = "// arda HLSL prelude\n";
    for (const BuiltinDefine& define : BuiltinDefines()) {
        prelude += std::format("#define {} {}\n", define.name, define.value);
    }
    for (const BuiltinConstant& constant : BuiltinConstants()) {
        // "static const", not "const": see the pitfall below.
        prelude += std::format("static const float {} = {:.17g};\n", constant.name, constant.value);
    }
    return prelude;
}

} // namespace

const std::string& HlslPrelude() {
    static const std::string prelude = BuildHlslPrelude();
    return prelude;
}

} // namespace arda::renderer::d3d11
```

`HlslPrelude()` is a function-local static, built once, thread-safely, on
first use ([Step 2](02-shaders.md#cpp-function-local-static)).

> **Pitfall — `const` globals in HLSL are uniforms:** in GLSL, a global
> `const float og_pi = 3.14;` is a compile-time constant. In HLSL, every
> global variable is implicitly `extern uniform`, and `const` only means
> "the shader can't assign to it". A global `const float og_pi = 3.14;`
> would become a `$Globals` variable, show up in `Uniforms()`, and read as
> **0** unless the application uploads a value: HLSL initializers on globals
> are ignored at run time. `static const` makes it a true constant, which is
> what the prelude writes. The same rule is why arda's HLSL uniforms never
> have initializers.

### `src/d3d11/shaders/ConstantBufferD3D11.h` and `.cpp`

One stage's `$Globals`: a CPU copy, the GPU buffer, and where to bind it.

```cpp
// src/d3d11/shaders/ConstantBufferD3D11.h
#pragma once

#include "d3d11/D3D11Common.h"

#include <cstddef>
#include <span>
#include <vector>

namespace arda::renderer::d3d11 {

enum class ShaderStage {
    Vertex,
    Geometry,
    Pixel,
};

class ConstantBufferD3D11 {
public:
    // sizeInBytes and bindPoint come from reflection ($Globals' size and its bN register).
    ConstantBufferD3D11(ID3D11Device* device, ShaderStage stage, UINT sizeInBytes, UINT bindPoint);

    ConstantBufferD3D11(const ConstantBufferD3D11&)            = delete;
    ConstantBufferD3D11& operator=(const ConstantBufferD3D11&) = delete;

    // Copies bytes into the CPU copy at offset and marks the buffer dirty.
    // Throws std::logic_error if they don't fit (a reflection or packing bug).
    void Write(UINT offset, std::span<const std::byte> bytes);

    // If anything was written since the last upload, copies the whole CPU copy to the GPU.
    void Upload(ID3D11DeviceContext* context);

    // Binds the buffer to its register in its stage.
    void Bind(ID3D11DeviceContext* context) const;

private:
    ShaderStage m_stage;
    UINT m_bindPoint;
    std::vector<std::byte> m_data;   // the CPU copy, zero-initialized like GL's uniforms
    ComPtr<ID3D11Buffer> m_buffer;
    bool m_dirty = true;             // upload the zeros once, before the first draw
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/shaders/ConstantBufferD3D11.cpp
#include "d3d11/shaders/ConstantBufferD3D11.h"

#include <cstring>
#include <stdexcept>

namespace arda::renderer::d3d11 {

ConstantBufferD3D11::ConstantBufferD3D11(ID3D11Device* device, ShaderStage stage, UINT sizeInBytes, UINT bindPoint)
    : m_stage(stage),
      m_bindPoint(bindPoint),
      m_data((static_cast<std::size_t>(sizeInBytes) + 15) / 16 * 16) {   // ByteWidth must be a multiple of 16
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(m_data.size());
    desc.Usage = D3D11_USAGE_DYNAMIC;               // rewritten often: Map with WRITE_DISCARD
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &m_buffer), "CreateBuffer($Globals)");
    SetDebugName(m_buffer.Get(), "$Globals");
}

void ConstantBufferD3D11::Write(UINT offset, std::span<const std::byte> bytes) {
    if (static_cast<std::size_t>(offset) + bytes.size() > m_data.size()) {
        throw std::logic_error("ConstantBufferD3D11::Write: the value doesn't fit in $Globals");
    }
    std::memcpy(m_data.data() + offset, bytes.data(), bytes.size());
    m_dirty = true;
}

void ConstantBufferD3D11::Upload(ID3D11DeviceContext* context) {
    if (!m_dirty) {
        return;
    }
    MappedSubresource mapped(context, m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD);
    std::memcpy(mapped.Data(), m_data.data(), m_data.size());   // DISCARD: every byte must be written
    m_dirty = false;
}

void ConstantBufferD3D11::Bind(ID3D11DeviceContext* context) const {
    // GetAddressOf, not &: these functions *read* an array of buffer pointers (see the ComPtr note).
    switch (m_stage) {
    case ShaderStage::Vertex:   context->VSSetConstantBuffers(m_bindPoint, 1, m_buffer.GetAddressOf()); return;
    case ShaderStage::Geometry: context->GSSetConstantBuffers(m_bindPoint, 1, m_buffer.GetAddressOf()); return;
    case ShaderStage::Pixel:    context->PSSetConstantBuffers(m_bindPoint, 1, m_buffer.GetAddressOf()); return;
    }
    throw std::invalid_argument("Invalid ShaderStage");
}

} // namespace arda::renderer::d3d11
```

`GetAddressOf()` is called on a `const ComPtr` here, which returns
`T* const*`. The `XXSetConstantBuffers` functions take
`ID3D11Buffer* const*`, so it fits without a cast.

### `src/d3d11/shaders/UniformD3D11.h` and `.cpp`

The D3D11 twin of `UniformGL3x<T>`. One uniform may live in two stages'
`$Globals` (a `u_color` declared in both the vertex and the pixel shader),
so it keeps a list of *targets*: a constant buffer and a byte offset.

```cpp
// src/d3d11/shaders/UniformD3D11.h
#pragma once

#include "Cleanable.h"
#include "d3d11/shaders/ConstantBufferD3D11.h"

#include <arda/core/geometry/Matrix.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/shaders/Uniform.h>

#include <string>
#include <utility>
#include <vector>

namespace arda::renderer::d3d11 {

// One overload per uniform value type: each writes the value in HLSL's
// constant buffer layout at offset. They are UploadUniform's D3D11 twins.
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, int value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, float value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, bool value);

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<float>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<float>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<float>& value);

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<int>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<int>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<int>& value);

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<bool>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<bool>& value);
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<bool>& value);

// Writes a column-major matrix: each column starts a new 16-byte register.
void WriteColumnMajorMatrix(ConstantBufferD3D11& buffer, UINT offset, const float* values, int columns, int rows);

inline void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Matrix4<float>& value) {
    WriteColumnMajorMatrix(buffer, offset, value.Data(), 4, 4);
}

template <int Columns, int Rows>
void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Matrix<float, Columns, Rows>& value) {
    WriteColumnMajorMatrix(buffer, offset, value.Data(), Columns, Rows);
}

// The part of UniformD3D11<T> that doesn't depend on T, so ShaderProgramD3D11
// can add a second stage's target without knowing the uniform's type.
class UniformTargetsD3D11 {
public:
    virtual ~UniformTargetsD3D11() = default;

    void AddTarget(ConstantBufferD3D11& buffer, UINT offset) { m_targets.push_back({&buffer, offset}); }

protected:
    struct Target {
        ConstantBufferD3D11* buffer;   // owned by the program, which owns this uniform too
        UINT offset;
    };
    std::vector<Target> m_targets;
};

// A uniform in a D3D11 program. SetValue only stores the value and tells the
// program it changed (3.4.4, the "delayed technique"). Clean() writes it into
// the CPU copy of each stage's $Globals; the program uploads those afterwards.
template <typename T>
class UniformD3D11 final : public Uniform<T>, public UniformTargetsD3D11, public ICleanable {
public:
    UniformD3D11(std::string name, UniformType type, ICleanableObserver& observer)
        : Uniform<T>(std::move(name), type), m_observer(observer) {
        m_observer.NotifyDirty(*this);   // write the initial (zero) value before the first draw
    }

    const T& Value() const override { return m_value; }

    void SetValue(const T& value) override {
        if (!m_dirty && m_value != value) {
            m_dirty = true;
            m_observer.NotifyDirty(*this);
        }
        m_value = value;
    }

    void Clean() override {
        for (const Target& target : m_targets) {
            WriteUniform(*target.buffer, target.offset, m_value);
        }
        m_dirty = false;
    }

private:
    T m_value{};
    bool m_dirty = true;
    ICleanableObserver& m_observer;
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/shaders/UniformD3D11.cpp
#include "d3d11/shaders/UniformD3D11.h"

#include <cstdint>
#include <span>

namespace arda::renderer::d3d11 {

namespace {

// Writes n 4-byte values. HLSL's int, float and bool are all 4 bytes.
template <typename T, std::size_t N>
void WriteArray(ConstantBufferD3D11& buffer, UINT offset, const T (&values)[N]) {
    static_assert(sizeof(T) == 4);
    buffer.Write(offset, std::as_bytes(std::span(values)));
}

std::int32_t ToHlslBool(bool value) {
    return value ? 1 : 0;   // an HLSL bool is a 32-bit value; any nonzero bit pattern is true
}

} // namespace

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, int value) {
    const std::int32_t values[] = {value};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, float value) {
    const float values[] = {value};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, bool value) {
    const std::int32_t values[] = {ToHlslBool(value)};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<float>& v) {
    const float values[] = {v.X(), v.Y()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<float>& v) {
    const float values[] = {v.X(), v.Y(), v.Z()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<float>& v) {
    const float values[] = {v.X(), v.Y(), v.Z(), v.W()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<int>& v) {
    const std::int32_t values[] = {v.X(), v.Y()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<int>& v) {
    const std::int32_t values[] = {v.X(), v.Y(), v.Z()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<int>& v) {
    const std::int32_t values[] = {v.X(), v.Y(), v.Z(), v.W()};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector2<bool>& v) {
    const std::int32_t values[] = {ToHlslBool(v.X()), ToHlslBool(v.Y())};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector3<bool>& v) {
    const std::int32_t values[] = {ToHlslBool(v.X()), ToHlslBool(v.Y()), ToHlslBool(v.Z())};
    WriteArray(buffer, offset, values);
}

void WriteUniform(ConstantBufferD3D11& buffer, UINT offset, const core::Vector4<bool>& v) {
    const std::int32_t values[] = {ToHlslBool(v.X()), ToHlslBool(v.Y()), ToHlslBool(v.Z()), ToHlslBool(v.W())};
    WriteArray(buffer, offset, values);
}

void WriteColumnMajorMatrix(ConstantBufferD3D11& buffer, UINT offset, const float* values, int columns, int rows) {
    for (int column = 0; column < columns; ++column) {
        const std::span<const float> columnValues(values + column * rows, static_cast<std::size_t>(rows));
        buffer.Write(offset + static_cast<UINT>(column) * 16u, std::as_bytes(columnValues));
    }
}

} // namespace arda::renderer::d3d11
```

- **Components are written one by one** (`v.X(), v.Y()`), not by copying
  the `Vector3` object. The bytes are the same, but this doesn't depend on
  `Vector3`'s layout, and it converts `bool` components to HLSL's 4-byte
  `bool` on the way.
- **`const T (&values)[N]`** is a reference to an array, which keeps the
  size `N` in the type, so `std::span(values)` knows how many elements there
  are ([Step 2](02-shaders.md#cpp-span)).
- **`UniformD3D11<T>` has three bases.** `Uniform<T>` is the public
  interface, `ICleanable` is the dirty-list interface, and
  `UniformTargetsD3D11` is the non-template part the program needs
  ([multiple interfaces](02-shaders.md#cpp-multiple-interfaces)). Inside the
  template, `m_targets` and `Target` are found without `this->`, because
  `UniformTargetsD3D11` is not a dependent base; `Uniform<T>` is, which is
  why its constructor is named with `<T>`
  ([two-phase lookup](02-shaders.md#cpp-two-phase-lookup)).

### `src/d3d11/shaders/ShaderProgramD3D11.h`

```cpp
#pragma once

#include "Cleanable.h"
#include "d3d11/D3D11Common.h"
#include "d3d11/shaders/ConstantBufferD3D11.h"

#include <arda/renderer/shaders/ShaderProgram.h>

#include <d3d11shader.h>   // ID3D11ShaderReflection

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace arda::renderer::d3d11 {

class DeviceD3D11;
class UniformTargetsD3D11;

class ShaderProgramD3D11 final : public ShaderProgram, public ICleanableObserver {
public:
    // An empty geometryShaderSource means there is no geometry shader.
    // Throws CouldNotCreateVideoCardResourceException if a stage doesn't
    // compile, or if the pixel shader reads an input the previous stage doesn't write.
    ShaderProgramD3D11(const DeviceD3D11& device,
                       std::string_view vertexShaderSource,
                       std::string_view geometryShaderSource,
                       std::string_view pixelShaderSource);

    // The compilers' warnings. Empty when there are none.
    std::string Log() const override { return m_log; }

    // Accepts the output's variable name (found in the source) or its semantic, e.g. "SV_Target0".
    int FragmentOutputLocation(std::string_view name) const override;

    // ICleanableObserver: a uniform's value changed since the last draw.
    void NotifyDirty(ICleanable& value) override { m_dirtyUniforms.push_back(&value); }

    // Sets the shaders and binds the constant buffers. Only needed when a different program was bound.
    void Bind(ID3D11DeviceContext* context) const;

    // Before each draw: sets the draw automatic uniforms, writes dirty uniforms
    // into the CPU copies of $Globals, and uploads the copies that changed.
    void Clean(Context& context, const DrawState& drawState, const SceneState& sceneState);

    // For input layouts (Step 3): CreateInputLayout checks a layout against this bytecode.
    ID3DBlob* VertexShaderCode() const { return m_vertexShaderCode.Get(); }

    // Identifies the vertex shader's input signature. Programs with the same key
    // can share input layouts.
    const std::string& InputSignatureKey() const { return m_inputSignatureKey; }

private:
    void ReflectVertexInputs(ID3D11ShaderReflection& reflection);
    void ReflectGlobals(ShaderStage stage, ID3D11ShaderReflection& reflection);
    void ReflectPixelOutputs(ID3D11ShaderReflection& reflection, std::string_view pixelShaderSource);
    void AddUniformTarget(const std::string& name, UniformType type, ConstantBufferD3D11& buffer, UINT offset);

    const DeviceD3D11& m_device;

    ComPtr<ID3DBlob> m_vertexShaderCode;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11GeometryShader> m_geometryShader;   // null when there is no geometry shader
    ComPtr<ID3D11PixelShader> m_pixelShader;

    // unique_ptr so each buffer keeps its address: uniforms point to them.
    std::vector<std::unique_ptr<ConstantBufferD3D11>> m_constantBuffers;

    // Only used while reflecting: finds a uniform already created for another stage.
    std::map<std::string, UniformTargetsD3D11*, std::less<>> m_uniformTargets;

    std::map<std::string, int, std::less<>> m_fragmentOutputs;   // name or semantic -> location
    std::vector<ICleanable*> m_dirtyUniforms;                    // points into m_uniforms
    std::string m_inputSignatureKey;
    std::string m_log;
};

} // namespace arda::renderer::d3d11
```

> **Why `std::vector<std::unique_ptr<ConstantBufferD3D11>>`:** each
> `UniformD3D11` stores a `ConstantBufferD3D11*`. A
> `std::vector<ConstantBufferD3D11>` would move its elements to new memory
> when it grows (the vertex shader's buffer is added before the pixel
> shader's), leaving those pointers dangling. With `unique_ptr` elements,
> the vector moves pointers around, and the buffers themselves never move
> ([Step 2](02-shaders.md#cpp-vector-unique-ptr)).

### `src/d3d11/shaders/ShaderProgramD3D11.cpp`

```cpp
#include "d3d11/shaders/ShaderProgramD3D11.h"
#include "d3d11/DeviceD3D11.h"
#include "d3d11/shaders/HlslPrelude.h"
#include "d3d11/shaders/UniformD3D11.h"

#include <arda/core/geometry/Matrix.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/Exceptions.h>

#include <d3dcompiler.h>

#include <bit>
#include <format>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <utility>

namespace arda::renderer::d3d11 {

namespace {

struct CompiledShader {
    ComPtr<ID3DBlob> code;
    std::string warnings;
};

std::string BlobToString(ID3DBlob* blob) {
    if (blob == nullptr) {
        return {};
    }
    std::string text(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
    while (!text.empty() && text.back() == '\0') {
        text.pop_back();   // compiler messages end with a null terminator
    }
    return text;
}

// target is "vs_5_0", "gs_5_0" or "ps_5_0". stageName appears in error messages:
// "pixel shader(3,5): error X3004: undeclared identifier 'u_colr'".
CompiledShader Compile(std::string_view source, const char* target, const char* stageName) {
    // "#line 1" after the prelude makes error line numbers refer to the user's source.
    std::string fullSource = HlslPrelude();
    fullSource += "#line 1\n";
    fullSource += source;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
#ifndef NDEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;   // readable in PIX and RenderDoc
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ComPtr<ID3DBlob> code;
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(fullSource.data(), fullSource.size(),
                                  stageName,   // the "file name" in messages
                                  nullptr,     // no macros beyond the prelude
                                  nullptr,     // no #include support
                                  "main", target, flags, 0, &code, &errors);
    std::string log = BlobToString(errors.Get());
    if (FAILED(hr)) {
        throw CouldNotCreateVideoCardResourceException(std::format(
            "Could not compile the {}. Compile log:\n\n{}", stageName, log.empty() ? HResultMessage(hr) : log));
    }
    return {std::move(code), std::move(log)};
}

ComPtr<ID3D11ShaderReflection> Reflect(ID3DBlob* code) {
    ComPtr<ID3D11ShaderReflection> reflection;
    ThrowIfFailed(D3DReflect(code->GetBufferPointer(), code->GetBufferSize(), IID_PPV_ARGS(&reflection)),
                  "D3DReflect");
    return reflection;
}

ShaderVertexAttributeType ToShaderVertexAttributeType(const D3D11_SIGNATURE_PARAMETER_DESC& parameter) {
    // Mask has one bit per component the input declares: 0b0111 is xyz, a float3.
    const int components = std::popcount(static_cast<unsigned>(parameter.Mask));
    using Type = ShaderVertexAttributeType;
    if (components >= 1 && components <= 4) {
        switch (parameter.ComponentType) {
        case D3D_REGISTER_COMPONENT_FLOAT32: {
            constexpr Type types[] = {Type::Float, Type::FloatVector2, Type::FloatVector3, Type::FloatVector4};
            return types[components - 1];
        }
        case D3D_REGISTER_COMPONENT_SINT32:
        case D3D_REGISTER_COMPONENT_UINT32: {   // arda has no unsigned attribute types
            constexpr Type types[] = {Type::Int, Type::IntVector2, Type::IntVector3, Type::IntVector4};
            return types[components - 1];
        }
        default:
            break;
        }
    }
    throw CouldNotCreateVideoCardResourceException(
        std::format("Unsupported vertex shader input type for '{}'", parameter.SemanticName));
}

// HLSL's floatRxC has R rows and C columns; arda's (and GLSL's) FloatMatrixCR has C columns and R rows.
std::optional<UniformType> ToMatrixUniformType(UINT columns, UINT rows) {
    if (columns == 2 && rows == 2) return UniformType::FloatMatrix22;
    if (columns == 3 && rows == 3) return UniformType::FloatMatrix33;
    if (columns == 4 && rows == 4) return UniformType::FloatMatrix44;
    if (columns == 2 && rows == 3) return UniformType::FloatMatrix23;
    if (columns == 2 && rows == 4) return UniformType::FloatMatrix24;
    if (columns == 3 && rows == 2) return UniformType::FloatMatrix32;
    if (columns == 3 && rows == 4) return UniformType::FloatMatrix34;
    if (columns == 4 && rows == 2) return UniformType::FloatMatrix42;
    if (columns == 4 && rows == 3) return UniformType::FloatMatrix43;
    return std::nullopt;
}

UniformType ToUniformType(const std::string& name, const D3D11_SHADER_TYPE_DESC& type) {
    if (type.Elements != 0) {
        throw CouldNotCreateVideoCardResourceException("Uniform arrays are not supported: " + name);
    }

    switch (type.Class) {
    case D3D_SVC_SCALAR:
    case D3D_SVC_VECTOR: {
        const UINT n = type.Columns;   // a vector is 1 row by n columns; a scalar is 1 by 1
        switch (type.Type) {
        case D3D_SVT_FLOAT: {
            constexpr UniformType types[] = {UniformType::Float, UniformType::FloatVector2,
                                             UniformType::FloatVector3, UniformType::FloatVector4};
            if (n >= 1 && n <= 4) return types[n - 1];
            break;
        }
        case D3D_SVT_INT: {
            constexpr UniformType types[] = {UniformType::Int, UniformType::IntVector2,
                                             UniformType::IntVector3, UniformType::IntVector4};
            if (n >= 1 && n <= 4) return types[n - 1];
            break;
        }
        case D3D_SVT_BOOL: {
            constexpr UniformType types[] = {UniformType::Bool, UniformType::BoolVector2,
                                             UniformType::BoolVector3, UniformType::BoolVector4};
            if (n >= 1 && n <= 4) return types[n - 1];
            break;
        }
        default:
            break;
        }
        break;
    }
    case D3D_SVC_MATRIX_COLUMNS:
        if (type.Type == D3D_SVT_FLOAT) {
            if (const auto matrixType = ToMatrixUniformType(type.Columns, type.Rows)) {
                return *matrixType;
            }
        }
        break;
    case D3D_SVC_MATRIX_ROWS:
        throw CouldNotCreateVideoCardResourceException(
            "Uniform '" + name + "' is declared row_major. arda uploads column-major matrices; remove row_major.");
    default:
        break;
    }
    throw CouldNotCreateVideoCardResourceException("Unsupported uniform type for '" + name + "'");
}

// Creates a UniformD3D11<T> and returns it twice: as the owning base pointer
// the UniformCollection stores, and as the target list the program adds to.
template <typename T>
std::pair<std::unique_ptr<UniformBase>, UniformTargetsD3D11*> MakeUniform(
    const std::string& name, UniformType type, ICleanableObserver& observer) {
    auto uniform = std::make_unique<UniformD3D11<T>>(name, type, observer);
    UniformTargetsD3D11* targets = uniform.get();
    return {std::move(uniform), targets};
}

// The same C++ value types as UniformGL3x uses, so Uniforms().Get<T>() behaves identically.
std::pair<std::unique_ptr<UniformBase>, UniformTargetsD3D11*> CreateUniform(
    const std::string& name, UniformType type, ICleanableObserver& observer) {
    using namespace arda::core;
    switch (type) {
    case UniformType::Int:           return MakeUniform<int>(name, type, observer);
    case UniformType::Float:         return MakeUniform<float>(name, type, observer);
    case UniformType::Bool:          return MakeUniform<bool>(name, type, observer);
    case UniformType::FloatVector2:  return MakeUniform<Vector2<float>>(name, type, observer);
    case UniformType::FloatVector3:  return MakeUniform<Vector3<float>>(name, type, observer);
    case UniformType::FloatVector4:  return MakeUniform<Vector4<float>>(name, type, observer);
    case UniformType::IntVector2:    return MakeUniform<Vector2<int>>(name, type, observer);
    case UniformType::IntVector3:    return MakeUniform<Vector3<int>>(name, type, observer);
    case UniformType::IntVector4:    return MakeUniform<Vector4<int>>(name, type, observer);
    case UniformType::BoolVector2:   return MakeUniform<Vector2<bool>>(name, type, observer);
    case UniformType::BoolVector3:   return MakeUniform<Vector3<bool>>(name, type, observer);
    case UniformType::BoolVector4:   return MakeUniform<Vector4<bool>>(name, type, observer);
    case UniformType::FloatMatrix22: return MakeUniform<Matrix2<float>>(name, type, observer);
    case UniformType::FloatMatrix33: return MakeUniform<Matrix3<float>>(name, type, observer);
    case UniformType::FloatMatrix44: return MakeUniform<Matrix4<float>>(name, type, observer);
    case UniformType::FloatMatrix23: return MakeUniform<Matrix23<float>>(name, type, observer);
    case UniformType::FloatMatrix24: return MakeUniform<Matrix24<float>>(name, type, observer);
    case UniformType::FloatMatrix32: return MakeUniform<Matrix32<float>>(name, type, observer);
    case UniformType::FloatMatrix34: return MakeUniform<Matrix34<float>>(name, type, observer);
    case UniformType::FloatMatrix42: return MakeUniform<Matrix42<float>>(name, type, observer);
    case UniformType::FloatMatrix43: return MakeUniform<Matrix43<float>>(name, type, observer);
    default:
        // Samplers are never loose globals in HLSL, so ToUniformType can't produce them.
        throw CouldNotCreateVideoCardResourceException("Unsupported uniform type for '" + name + "'");
    }
}

// The previous stage's user-defined outputs, as "semantic:index" strings.
std::set<std::string> UserOutputs(ID3D11ShaderReflection& reflection) {
    D3D11_SHADER_DESC desc{};
    ThrowIfFailed(reflection.GetDesc(&desc), "ID3D11ShaderReflection::GetDesc");
    std::set<std::string> outputs;
    for (UINT i = 0; i < desc.OutputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC parameter{};
        ThrowIfFailed(reflection.GetOutputParameterDesc(i, &parameter), "GetOutputParameterDesc");
        outputs.insert(std::format("{}:{}", parameter.SemanticName, parameter.SemanticIndex));
    }
    return outputs;
}

// D3D11's stand-in for a link step: every user-defined pixel shader input must be
// written by the previous stage. (System values like SV_Position and SV_IsFrontFace
// are provided by the rasterizer.) Semantics are case-insensitive in HLSL, but
// reflection reports them as written; write each one the same way in both stages.
void CheckLinkage(ID3D11ShaderReflection& previousStage, ID3D11ShaderReflection& pixelShader) {
    const std::set<std::string> outputs = UserOutputs(previousStage);
    D3D11_SHADER_DESC desc{};
    ThrowIfFailed(pixelShader.GetDesc(&desc), "ID3D11ShaderReflection::GetDesc");
    for (UINT i = 0; i < desc.InputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC parameter{};
        ThrowIfFailed(pixelShader.GetInputParameterDesc(i, &parameter), "GetInputParameterDesc");
        if (parameter.SystemValueType != D3D_NAME_UNDEFINED) {
            continue;
        }
        const std::string input = std::format("{}:{}", parameter.SemanticName, parameter.SemanticIndex);
        if (!outputs.contains(input)) {
            throw CouldNotCreateVideoCardResourceException(std::format(
                "Could not link shader program: the pixel shader reads {}{}, which the previous stage doesn't write.",
                parameter.SemanticName, parameter.SemanticIndex));
        }
    }
}

} // namespace

ShaderProgramD3D11::ShaderProgramD3D11(const DeviceD3D11& device,
                                       std::string_view vertexShaderSource,
                                       std::string_view geometryShaderSource,
                                       std::string_view pixelShaderSource)
    : m_device(device) {
    ID3D11Device* d3d = device.Native();

    // Compile and create each stage. D3D11 has no program object to link them into.
    CompiledShader vertex = Compile(vertexShaderSource, "vs_5_0", "vertex shader");
    ThrowIfFailed(d3d->CreateVertexShader(vertex.code->GetBufferPointer(), vertex.code->GetBufferSize(),
                                          nullptr, &m_vertexShader),
                  "CreateVertexShader");
    m_vertexShaderCode = vertex.code;

    std::optional<CompiledShader> geometry;
    if (!geometryShaderSource.empty()) {
        geometry = Compile(geometryShaderSource, "gs_5_0", "geometry shader");
        ThrowIfFailed(d3d->CreateGeometryShader(geometry->code->GetBufferPointer(), geometry->code->GetBufferSize(),
                                                nullptr, &m_geometryShader),
                      "CreateGeometryShader");
    }

    CompiledShader pixel = Compile(pixelShaderSource, "ps_5_0", "pixel shader");
    ThrowIfFailed(d3d->CreatePixelShader(pixel.code->GetBufferPointer(), pixel.code->GetBufferSize(),
                                         nullptr, &m_pixelShader),
                  "CreatePixelShader");

    m_log = vertex.warnings + (geometry ? geometry->warnings : std::string()) + pixel.warnings;

    // Reflect: fill the base class's collections, as FindVertexAttributes and FindUniforms do in GL.
    const ComPtr<ID3D11ShaderReflection> vertexReflection = Reflect(vertex.code.Get());
    ReflectVertexInputs(*vertexReflection.Get());
    ReflectGlobals(ShaderStage::Vertex, *vertexReflection.Get());

    ComPtr<ID3D11ShaderReflection> geometryReflection;
    if (geometry) {
        geometryReflection = Reflect(geometry->code.Get());
        ReflectGlobals(ShaderStage::Geometry, *geometryReflection.Get());
    }

    const ComPtr<ID3D11ShaderReflection> pixelReflection = Reflect(pixel.code.Get());
    ReflectGlobals(ShaderStage::Pixel, *pixelReflection.Get());
    ReflectPixelOutputs(*pixelReflection.Get(), pixelShaderSource);

    CheckLinkage(geometry ? *geometryReflection.Get() : *vertexReflection.Get(), *pixelReflection.Get());

    m_uniformTargets.clear();   // only needed while reflecting
    InitializeAutomaticUniforms(device);
}

int ShaderProgramD3D11::FragmentOutputLocation(std::string_view name) const {
    const auto it = m_fragmentOutputs.find(name);
    if (it == m_fragmentOutputs.end()) {
        throw std::out_of_range("No fragment output named '" + std::string(name) +
                                "'. On Direct3D 11, name outputs as 'out float4 name : SV_TargetN' "
                                "or ask for the semantic, e.g. 'SV_Target0'.");
    }
    return it->second;
}

void ShaderProgramD3D11::Bind(ID3D11DeviceContext* context) const {
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->GSSetShader(m_geometryShader.Get(), nullptr, 0);   // nullptr unbinds a previous program's
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    for (const auto& buffer : m_constantBuffers) {
        buffer->Bind(context);
    }
}

void ShaderProgramD3D11::Clean(Context& context, const DrawState& drawState, const SceneState& sceneState) {
    SetDrawAutomaticUniforms(context, drawState, sceneState);

    for (ICleanable* uniform : m_dirtyUniforms) {
        uniform->Clean();   // writes into the CPU copies
    }
    m_dirtyUniforms.clear();

    ID3D11DeviceContext* immediate = m_device.Immediate();
    for (const auto& buffer : m_constantBuffers) {
        buffer->Upload(immediate);   // only the ones that were written
    }
}

void ShaderProgramD3D11::ReflectVertexInputs(ID3D11ShaderReflection& reflection) {
    D3D11_SHADER_DESC shaderDesc{};
    ThrowIfFailed(reflection.GetDesc(&shaderDesc), "ID3D11ShaderReflection::GetDesc");

    for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC parameter{};
        ThrowIfFailed(reflection.GetInputParameterDesc(i, &parameter), "GetInputParameterDesc");

        // Everything CreateInputLayout checks a layout against.
        m_inputSignatureKey += std::format("{}{}:{}:{};", parameter.SemanticName, parameter.SemanticIndex,
                                           static_cast<int>(parameter.ComponentType),
                                           static_cast<int>(parameter.Mask));

        if (parameter.SystemValueType != D3D_NAME_UNDEFINED) {
            continue;   // SV_VertexID, SV_InstanceID: generated by the input assembler, not read from buffers
        }

        const std::string name = parameter.SemanticName;
        if (m_vertexAttributes.Contains(name)) {
            // Two inputs with one semantic name: an array or matrix input (texcoord0, texcoord1, ...),
            // or a name ending in a digit. Either way, the name no longer identifies one attribute.
            throw CouldNotCreateVideoCardResourceException(
                "Vertex shader input semantic '" + name + "' is used more than once. "
                "Matrix and array vertex inputs are not supported.");
        }
        m_vertexAttributes.Add({name, static_cast<int>(parameter.SemanticIndex),
                                ToShaderVertexAttributeType(parameter), 1});
    }
}

void ShaderProgramD3D11::ReflectGlobals(ShaderStage stage, ID3D11ShaderReflection& reflection) {
    D3D11_SHADER_DESC shaderDesc{};
    ThrowIfFailed(reflection.GetDesc(&shaderDesc), "ID3D11ShaderReflection::GetDesc");

    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D11_SHADER_INPUT_BIND_DESC binding{};
        ThrowIfFailed(reflection.GetResourceBindingDesc(i, &binding), "GetResourceBindingDesc");
        if (binding.Type != D3D_SIT_CBUFFER || std::string_view(binding.Name) != "$Globals") {
            continue;   // textures, samplers, and explicit cbuffers (not supported, like GL uniform blocks)
        }

        // Never null: an unknown name returns a dummy object whose GetDesc fails.
        ID3D11ShaderReflectionConstantBuffer* globals = reflection.GetConstantBufferByName("$Globals");
        D3D11_SHADER_BUFFER_DESC bufferDesc{};
        ThrowIfFailed(globals->GetDesc(&bufferDesc), "ID3D11ShaderReflectionConstantBuffer::GetDesc");

        ConstantBufferD3D11& buffer = *m_constantBuffers.emplace_back(
            std::make_unique<ConstantBufferD3D11>(m_device.Native(), stage, bufferDesc.Size, binding.BindPoint));

        for (UINT v = 0; v < bufferDesc.Variables; ++v) {
            ID3D11ShaderReflectionVariable* variable = globals->GetVariableByIndex(v);
            D3D11_SHADER_VARIABLE_DESC variableDesc{};
            ThrowIfFailed(variable->GetDesc(&variableDesc), "ID3D11ShaderReflectionVariable::GetDesc");
            if ((variableDesc.uFlags & D3D_SVF_USED) == 0) {
                continue;   // declared but never read: not an active uniform, as in GL
            }

            D3D11_SHADER_TYPE_DESC typeDesc{};
            ThrowIfFailed(variable->GetType()->GetDesc(&typeDesc), "ID3D11ShaderReflectionType::GetDesc");

            const std::string name = variableDesc.Name;
            AddUniformTarget(name, ToUniformType(name, typeDesc), buffer, variableDesc.StartOffset);
        }
    }
}

void ShaderProgramD3D11::AddUniformTarget(const std::string& name, UniformType type,
                                          ConstantBufferD3D11& buffer, UINT offset) {
    if (auto it = m_uniformTargets.find(name); it != m_uniformTargets.end()) {
        // Already created for an earlier stage: the same uniform, one more place to write it.
        if (m_uniforms[name].Datatype() != type) {
            throw CouldNotCreateVideoCardResourceException(
                "Uniform '" + name + "' has different types in different shader stages");
        }
        it->second->AddTarget(buffer, offset);
        return;
    }

    auto [uniform, targets] = CreateUniform(name, type, *this);
    targets->AddTarget(buffer, offset);
    m_uniformTargets.emplace(name, targets);
    m_uniforms.Add(std::move(uniform));
}

void ShaderProgramD3D11::ReflectPixelOutputs(ID3D11ShaderReflection& reflection, std::string_view pixelShaderSource) {
    D3D11_SHADER_DESC shaderDesc{};
    ThrowIfFailed(reflection.GetDesc(&shaderDesc), "ID3D11ShaderReflection::GetDesc");

    // 1. The semantics themselves: "SV_Target0" -> 0.
    std::set<int> targets;
    for (UINT i = 0; i < shaderDesc.OutputParameters; ++i) {
        D3D11_SIGNATURE_PARAMETER_DESC parameter{};
        ThrowIfFailed(reflection.GetOutputParameterDesc(i, &parameter), "GetOutputParameterDesc");
        if (parameter.SystemValueType == D3D_NAME_TARGET) {
            const int index = static_cast<int>(parameter.SemanticIndex);
            targets.insert(index);
            m_fragmentOutputs.emplace(std::format("SV_Target{}", index), index);
        }
    }

    // 2. The names in the source: "out float4 fragmentColor : SV_Target0" or a
    //    struct member "float4 color : SV_Target1;". Reflection doesn't keep them.
    static const std::regex pattern(R"(([A-Za-z_]\w*)\s*:\s*SV_Target(\d*))", std::regex::icase);
    const char* begin = pixelShaderSource.data();
    const char* end = begin + pixelShaderSource.size();
    for (std::cregex_iterator it(begin, end, pattern), last; it != last; ++it) {
        const std::cmatch& match = *it;
        const int index = match[2].length() == 0 ? 0 : std::stoi(match[2].str());   // "SV_Target" means 0
        if (targets.contains(index)) {   // only outputs the compiled shader really has
            m_fragmentOutputs.emplace(match[1].str(), index);
        }
    }
}

} // namespace arda::renderer::d3d11
```

A walk through the less obvious parts:

- **`Compile`** puts the prelude first and `#line 1` after it, so an error in
  line 3 of your shader is reported as `pixel shader(3,5)`, not
  `pixel shader(20,5)`. (GL keeps line numbers by passing the prelude as a
  separate source string; D3DCompile takes one string, and `#line` does the
  same job.) `D3DCOMPILE_ENABLE_STRICTNESS` rejects deprecated D3D9 syntax.
  `D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION` keeps debug builds
  readable in graphics debuggers.
- **The vertex shader's bytecode is kept** (`m_vertexShaderCode`):
  `CreateInputLayout` validates each input layout against it (Step 3). The
  other stages' bytecode can be dropped once the shader objects exist.
- **`GetConstantBufferByName` never returns null.** For a name that doesn't
  exist it returns a placeholder object whose `GetDesc` fails. That's why
  the code finds `$Globals` through the resource bindings first, which also
  gives its register (`BindPoint`, usually 0 for `b0`).
- **`AddUniformTarget`** creates each uniform once. A uniform declared in
  both stages gets a second target, and its type must match.
- **`CheckLinkage`** stands in for GL's linker, as explained above.
- **`Log()`** returns warnings. Errors are in the exception.

<a id="cpp-popcount"></a>

> **C++ note — `std::popcount`:** `<bit>` (C++20) has functions for bit
> manipulation that used to need compiler intrinsics. `std::popcount(x)`
> counts the 1 bits of an *unsigned* integer; it doesn't accept signed
> types, which is why `parameter.Mask` (a `BYTE`) is cast to `unsigned`
> first. A mask of `0b0111` (x, y and z) gives 3, a `float3` input.
> [Step 3](03-vertex-data.md) used `std::bit_cast` from the same header.

<a id="cpp-regex"></a>

> **C++ note — `std::regex`:** `<regex>` provides regular expressions.
> `std::regex` compiles a pattern once, which is why it is a function-local
> `static const` here. `std::cregex_iterator` walks every match in a range of
> `const char*` (the `c` stands for C string; `sregex_iterator` is the
> `std::string` version), and a default-constructed iterator marks the end.
> Each match is a `std::cmatch`: `match[0]` is the whole match, and
> `match[1]`, `match[2]` are the parenthesized groups. `std::regex::icase`
> makes the match case-insensitive, like HLSL semantics. The pattern is a
> raw string literal, so `\w` and `\s` need no double backslashes
> ([Step 2](02-shaders.md#cpp-raw-string-literals)). `std::regex` is slow
> compared with a hand-written scanner, but it runs once per program
> creation, where compiling the shader costs far more.

> **C++ note — returning two views of one object:** `MakeUniform` returns
> `std::pair<std::unique_ptr<UniformBase>, UniformTargetsD3D11*>`: the owning
> pointer, typed as the base the `UniformCollection` stores, and a
> non-owning pointer to a *different base* of the same object. The raw
> pointer is taken before the `unique_ptr` is moved into the pair; moving a
> `unique_ptr` doesn't move the object it points to, so it stays valid. The
> caller unpacks it with structured bindings:
> `auto [uniform, targets] = CreateUniform(...)`
> ([Step 2](02-shaders.md#cpp-structured-bindings)).

### `DeviceD3D11`: create programs

In `src/d3d11/DeviceD3D11.cpp`, add `#include "d3d11/shaders/ShaderProgramD3D11.h"`
and replace the `DoCreateShaderProgram` stub:

```cpp
std::shared_ptr<ShaderProgram> DeviceD3D11::DoCreateShaderProgram(
    std::string_view vertexShaderSource, std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource) {
    return std::make_shared<ShaderProgramD3D11>(*this, vertexShaderSource, geometryShaderSource,
                                                fragmentShaderSource);
}
```

The program takes `const DeviceD3D11&` first, mirroring `ShaderProgramGL3x`'s
`const Device&`: it needs the device for the automatic uniform registries
(`InitializeAutomaticUniforms`) and to create its shaders and constant
buffers (`Native()` is a `const` function that returns a non-const
`ID3D11Device*`).

### Tests: `tests/src/renderer/HlslShaderTests.cpp`

The HLSL versions of `ShaderProgramTests`, plus the rules that only exist on
D3D11. A device is enough, no window: D3D11 compiles shaders without any
"current" state.

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/Exceptions.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <memory>
#include <stdexcept>

using namespace arda::renderer;
using arda::core::Vector3;

namespace {

// nullptr when the D3D11 backend isn't compiled in; the test then returns early.
std::unique_ptr<Device> CreateD3D11Device() {
    if (!IsGraphicsApiAvailable(GraphicsApi::Direct3D11)) {
        return nullptr;
    }
    return CreateDevice(GraphicsApi::Direct3D11);
}

constexpr const char* kVertexShader = R"(
float4x4 u_modelViewPerspective;
float4 main(float4 position : position0) : SV_Position
{
    return mul(u_modelViewPerspective, position);
})";

constexpr const char* kPixelShader = R"(
float3 u_color;
void main(out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(u_color * og_oneOverPi, 1.0);   // uses a built-in constant
})";

} // namespace

TEST_CASE("HLSL: ShaderProgram finds attributes, uniforms and outputs") {
    auto device = CreateD3D11Device();
    if (!device) {
        MESSAGE("Direct3D 11 backend not compiled; skipping");
        return;
    }
    auto sp = device->CreateShaderProgram(kVertexShader, kPixelShader);

    REQUIRE(sp->VertexAttributes().Contains("position"));
    CHECK(sp->VertexAttributes()["position"].location == VertexLocations::Position);
    CHECK(sp->VertexAttributes()["position"].type == ShaderVertexAttributeType::FloatVector4);

    CHECK(sp->Uniforms().Size() == 2);   // og_oneOverPi is static const, not a uniform
    CHECK(sp->Uniforms()["u_color"].Datatype() == UniformType::FloatVector3);
    CHECK(sp->Uniforms()["u_modelViewPerspective"].Datatype() == UniformType::FloatMatrix44);

    auto& color = sp->Uniforms().Get<Vector3<float>>("u_color");
    color.SetValue(Vector3<float>(1, 0, 0));
    CHECK(color.Value() == Vector3<float>(1, 0, 0));
    CHECK_THROWS_AS(sp->Uniforms().Get<float>("u_color"), std::invalid_argument);

    CHECK(sp->FragmentOutputLocation("fragmentColor") == 0);
    CHECK(sp->FragmentOutputLocation("SV_Target0") == 0);
    CHECK_THROWS_AS(sp->FragmentOutputLocation("missing"), std::out_of_range);
}

TEST_CASE("HLSL: compile errors throw with the log") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    CHECK_THROWS_AS(device->CreateShaderProgram("this is not hlsl", kPixelShader),
                    CouldNotCreateVideoCardResourceException);
}

TEST_CASE("HLSL: a uniform used by two stages is one uniform") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    auto sp = device->CreateShaderProgram(
        R"(float u_scale;
           float4 main(float4 position : position0) : SV_Position { return position * u_scale; })",
        R"(float u_scale;
           float4 main() : SV_Target0 { return float4(u_scale, 0.0, 0.0, 1.0); })");
    CHECK(sp->Uniforms().Size() == 1);
    CHECK(sp->Uniforms()["u_scale"].Datatype() == UniformType::Float);
}

TEST_CASE("HLSL: unused globals are not active uniforms") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    auto sp = device->CreateShaderProgram(
        R"(float4 u_unused;
           float4 main(float4 position : position0) : SV_Position { return position; })",
        "float4 main() : SV_Target0 { return float4(1.0, 1.0, 1.0, 1.0); }");
    CHECK_FALSE(sp->Uniforms().Contains("u_unused"));
}

TEST_CASE("HLSL: non-square matrices map to GLSL's naming") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    // HLSL float3x2: 3 rows, 2 columns. GLSL mat2x3 / arda FloatMatrix23: 2 columns, 3 rows.
    auto sp = device->CreateShaderProgram(
        R"(float3x2 u_m;
           float4 main(float4 position : position0) : SV_Position
           {
               return float4(mul(u_m, position.xy), 1.0);
           })",
        "float4 main() : SV_Target0 { return float4(1.0, 1.0, 1.0, 1.0); }");
    CHECK(sp->Uniforms()["u_m"].Datatype() == UniformType::FloatMatrix23);
}

TEST_CASE("HLSL: row_major matrices are rejected") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    CHECK_THROWS_AS(device->CreateShaderProgram(
                        R"(row_major float4x4 u_m;
                           float4 main(float4 position : position0) : SV_Position { return mul(u_m, position); })",
                        "float4 main() : SV_Target0 { return float4(1.0, 1.0, 1.0, 1.0); }"),
                    CouldNotCreateVideoCardResourceException);
}

TEST_CASE("HLSL: a pixel shader input the vertex shader doesn't write fails to link") {
    auto device = CreateD3D11Device();
    if (!device) {
        return;
    }
    CHECK_THROWS_AS(device->CreateShaderProgram(
                        "float4 main(float4 position : position0) : SV_Position { return position; }",
                        "float4 main(float2 uv : TEXCOORD0) : SV_Target0 { return float4(uv, 0.0, 1.0); }"),
                    CouldNotCreateVideoCardResourceException);
}
```

> **D3D11 note — compiler dead-code elimination changes what you see:** the
> `float3x2` test multiplies the matrix into the result on purpose. A uniform
> the shader declares but whose value never reaches an output is removed by
> the optimizer, so reflection reports it without `D3D_SVF_USED`, and arda
> skips it, like GL's inactive uniforms. When a uniform you expect is
> "missing" from `Uniforms()`, check that it affects the output.

**Milestone:** `HlslShaderTests` pass, and `ShaderProgramTests` still pass
on GL. Also try a deliberate typo (`u_colr`) and read the exception: the
line and column refer to your source, thanks to `#line 1`.

---

## Step 3 on D3D11: buffers, input layouts and the first triangle

This step mirrors [Step 3](03-vertex-data.md). `VertexBuffer`, `IndexBuffer`,
`VertexArray`, `Mesh` and `CreateMeshBuffers` are unchanged; only the
backend classes are new. Read 3.5.5 ("Vertex Data in Direct3D") again first.

### D3D11 concepts for this step

> **D3D11 note — buffers and usage:** every D3D11 buffer is an
> `ID3D11Buffer`, created with a `D3D11_BUFFER_DESC`. Where GL has one
> usage *hint* (`GL_STATIC_DRAW`, ...) that the driver may ignore, D3D11 has
> a binding *contract*:
>
> | `Usage` | GPU | CPU | Updated with |
> |---|---|---|---|
> | `D3D11_USAGE_DEFAULT` | read and write | none | `UpdateSubresource`, or GPU copies |
> | `D3D11_USAGE_DYNAMIC` | read only | write | `Map(WRITE_DISCARD / WRITE_NO_OVERWRITE)` |
> | `D3D11_USAGE_IMMUTABLE` | read only | none | only the initial data at creation |
> | `D3D11_USAGE_STAGING` | copy source/destination only | read and/or write | `Map(READ / WRITE)` |
>
> `BindFlags` say where the buffer may be bound (`D3D11_BIND_VERTEX_BUFFER`,
> `D3D11_BIND_INDEX_BUFFER`, `D3D11_BIND_CONSTANT_BUFFER`, ...). The runtime
> enforces both: binding a buffer somewhere its flags don't allow, or
> mapping a `DEFAULT` buffer, fails.

> **D3D11 note — staging resources for readback:** the CPU can't read a
> `DEFAULT` resource. To read one back (`CopyToSystemMemory`, and textures in
> Step 5), you create a `STAGING` resource of the same size with
> `D3D11_CPU_ACCESS_READ`, ask the GPU to copy into it
> (`CopySubresourceRegion` or `CopyResource`), then `Map(D3D11_MAP_READ)` it.
> `Map` waits until the GPU has actually done the copy, which stalls the CPU
> until every earlier command has run, the same stall `glGetBufferSubData`
> causes. Fine for tests and screenshots, bad in a frame loop.

> **D3D11 note — input layouts vs. GL attribute locations:** in GL, a
> vertex array object records "attribute location N reads this buffer, with
> this format, stride and offset", and the shader declares
> `layout(location = N) in vec4 position`. D3D11 splits the same
> information in two:
> - The **input layout** (`ID3D11InputLayout`) is an array of
>   `D3D11_INPUT_ELEMENT_DESC`: for each element, a semantic (name and
>   index), a `DXGI_FORMAT`, which *input slot* (vertex buffer binding) it
>   reads from, and its byte offset within a vertex. It is created *against
>   a vertex shader's bytecode*, and creation fails if an element the shader
>   reads is missing or has an incompatible type.
> - The **vertex buffer bindings** (`IASetVertexBuffers`): for each input
>   slot, a buffer, a stride and a starting offset.
>
> Because an input layout depends on both the vertex data *and* the vertex
> shader, it can't be created when the vertex array is, nor when the program
> is. `VertexArrayD3D11` creates it at the first draw that uses a given
> program, and caches it per vertex shader *input signature*: two programs
> whose vertex shaders read the same inputs share the layout.

> **Why each attribute gets its own input slot:** arda's
> `VertexBufferAttribute` names a buffer, an offset and a stride per
> attribute, like `glVertexAttribPointer`. D3D11's natural model is one slot
> per buffer, with several elements at different `AlignedByteOffset`s. Using
> slot N for location N, with `AlignedByteOffset = 0` and the attribute's
> offset passed to `IASetVertexBuffers`, maps GL's model directly, whether
> attributes share a buffer (interleaved) or not. D3D11 has 32 slots, which
> is `DeviceLimits::maximumNumberOfVertexAttributes`.

### `src/d3d11/buffers/BufferD3D11.h` and `.cpp`

The helper shared by vertex and index buffers, like `BufferGL3x`.

```cpp
// src/d3d11/buffers/BufferD3D11.h
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/buffers/BufferHint.h>

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace arda::renderer::d3d11 {

class DeviceD3D11;

// One ID3D11Buffer with a fixed size (BufferGL3x's twin). bindFlags is
// D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER.
class BufferD3D11 {
public:
    BufferD3D11(const DeviceD3D11& device, UINT bindFlags, BufferHint usageHint, std::size_t sizeInBytes,
                std::string_view debugName);

    BufferD3D11(const BufferD3D11&)            = delete;
    BufferD3D11& operator=(const BufferD3D11&) = delete;

    // The caller has already checked that the range fits.
    void CopyFromSystemMemory(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes);
    void CopyToSystemMemory(std::span<std::byte> bytes, std::size_t offsetInBytes) const;

    std::size_t SizeInBytes() const { return m_sizeInBytes; }
    BufferHint UsageHint() const { return m_usageHint; }
    ID3D11Buffer* Native() const { return m_buffer.Get(); }

private:
    // Stream/Dynamic *Draw buffers are rewritten often: DYNAMIC usage and Map.
    // Everything else: DEFAULT usage and UpdateSubresource.
    static bool IsDynamic(BufferHint usageHint);

    const DeviceD3D11& m_device;
    ComPtr<ID3D11Buffer> m_buffer;
    BufferHint m_usageHint;
    std::size_t m_sizeInBytes;
    std::vector<std::byte> m_shadowCopy;   // dynamic buffers only: the whole contents, in system memory
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/buffers/BufferD3D11.cpp
#include "d3d11/buffers/BufferD3D11.h"
#include "d3d11/DeviceD3D11.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace arda::renderer::d3d11 {

BufferD3D11::BufferD3D11(const DeviceD3D11& device, UINT bindFlags, BufferHint usageHint, std::size_t sizeInBytes,
                         std::string_view debugName)
    : m_device(device), m_usageHint(usageHint), m_sizeInBytes(sizeInBytes) {
    if (sizeInBytes > std::numeric_limits<UINT>::max()) {
        throw std::invalid_argument("Direct3D 11 buffers are limited to 4 GB (a UINT byte width)");
    }

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(sizeInBytes);
    desc.BindFlags = bindFlags;
    if (IsDynamic(usageHint)) {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        m_shadowCopy.resize(sizeInBytes);   // zero-initialized, like the DEFAULT buffers in practice
    } else {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
    }

    ThrowIfFailed(device.Native()->CreateBuffer(&desc, nullptr, &m_buffer), "CreateBuffer");
    SetDebugName(m_buffer.Get(), debugName);
}

bool BufferD3D11::IsDynamic(BufferHint usageHint) {
    return usageHint == BufferHint::StreamDraw || usageHint == BufferHint::DynamicDraw;
}

void BufferD3D11::CopyFromSystemMemory(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) {
    if (bytes.empty()) {
        return;
    }
    ID3D11DeviceContext* immediate = m_device.Immediate();

    if (!m_shadowCopy.empty()) {
        // WRITE_DISCARD gives fresh memory, so every byte must be written: update
        // the shadow copy, then upload all of it.
        std::memcpy(m_shadowCopy.data() + destinationOffsetInBytes, bytes.data(), bytes.size());
        MappedSubresource mapped(immediate, m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD);
        std::memcpy(mapped.Data(), m_shadowCopy.data(), m_shadowCopy.size());
        return;
    }

    // For a buffer, the box is a byte range: [left, right) with top/bottom and front/back set to [0, 1).
    const D3D11_BOX box{
        static_cast<UINT>(destinationOffsetInBytes), 0, 0,
        static_cast<UINT>(destinationOffsetInBytes + bytes.size()), 1, 1,
    };
    immediate->UpdateSubresource(m_buffer.Get(), 0, &box, bytes.data(), 0, 0);
}

void BufferD3D11::CopyToSystemMemory(std::span<std::byte> bytes, std::size_t offsetInBytes) const {
    if (bytes.empty()) {
        return;
    }

    if (!m_shadowCopy.empty()) {
        std::memcpy(bytes.data(), m_shadowCopy.data() + offsetInBytes, bytes.size());   // no GPU round trip
        return;
    }

    // DEFAULT buffers can't be mapped. Copy the range into a staging buffer the CPU can read.
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(bytes.size());
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;   // staging resources can't be bound anywhere
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Buffer> staging;
    ThrowIfFailed(m_device.Native()->CreateBuffer(&desc, nullptr, &staging), "CreateBuffer(staging)");

    ID3D11DeviceContext* immediate = m_device.Immediate();
    const D3D11_BOX box{
        static_cast<UINT>(offsetInBytes), 0, 0,
        static_cast<UINT>(offsetInBytes + bytes.size()), 1, 1,
    };
    immediate->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, m_buffer.Get(), 0, &box);

    MappedSubresource mapped(immediate, staging.Get(), 0, D3D11_MAP_READ);   // waits for the copy
    std::memcpy(bytes.data(), mapped.Data(), bytes.size());
}

} // namespace arda::renderer::d3d11
```

> **Why a shadow copy for dynamic buffers:** GL's `glBufferSubData` can
> update any byte range of any buffer at any time; the driver makes it safe.
> D3D11's fast path for often-changing data, `Map` with `WRITE_DISCARD`,
> throws the old contents away, so a partial update like
> `CopyFromSystemMemory(values, 64)` would leave bytes 0-63 undefined. The
> alternative, `WRITE_NO_OVERWRITE`, keeps the old contents but makes *you*
> promise not to touch bytes the GPU may still be reading, a promise
> `VertexBuffer`'s interface can't make. Keeping the whole buffer in system
> memory costs its size in RAM and gives GL's semantics exactly, with the
> fast `DISCARD` path. It also makes `CopyToSystemMemory` free for dynamic
> buffers. Static buffers use `UpdateSubresource`, which handles byte ranges
> on its own.

> **D3D11 note — `UpdateSubresource`:** it copies from your pointer into
> driver memory right away, then schedules a GPU copy into the resource, so
> you can reuse your memory as soon as it returns. The `D3D11_BOX` selects
> the destination region; for a buffer, only `left` and `right` (bytes)
> matter, and `top`/`bottom` and `front`/`back` must be 0 and 1. The two
> pitch arguments are for textures and are ignored for buffers.

### `src/d3d11/buffers/VertexBufferD3D11.h` and `IndexBufferD3D11.h`

Thin wrappers, as in GL. Everything is inline, so there are no `.cpp` files.

```cpp
// src/d3d11/buffers/VertexBufferD3D11.h
#pragma once

#include "d3d11/buffers/BufferD3D11.h"

#include <arda/renderer/buffers/VertexBuffer.h>

namespace arda::renderer::d3d11 {

class VertexBufferD3D11 final : public VertexBuffer {
public:
    VertexBufferD3D11(const DeviceD3D11& device, BufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(device, D3D11_BIND_VERTEX_BUFFER, usageHint, sizeInBytes, "vertex buffer") {}

    ID3D11Buffer* Native() const { return m_buffer.Native(); }

    std::size_t SizeInBytes() const override { return m_buffer.SizeInBytes(); }
    BufferHint UsageHint() const override { return m_buffer.UsageHint(); }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        m_buffer.CopyFromSystemMemory(bytes, destinationOffsetInBytes);
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        m_buffer.CopyToSystemMemory(bytes, offsetInBytes);
    }

private:
    BufferD3D11 m_buffer;
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/buffers/IndexBufferD3D11.h
#pragma once

#include "d3d11/buffers/BufferD3D11.h"

#include <arda/renderer/buffers/IndexBuffer.h>

namespace arda::renderer::d3d11 {

class IndexBufferD3D11 final : public IndexBuffer {
public:
    IndexBufferD3D11(const DeviceD3D11& device, BufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(device, D3D11_BIND_INDEX_BUFFER, usageHint, sizeInBytes, "index buffer") {}

    ID3D11Buffer* Native() const { return m_buffer.Native(); }

    std::size_t SizeInBytes() const override { return m_buffer.SizeInBytes(); }
    BufferHint UsageHint() const override { return m_buffer.UsageHint(); }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        m_buffer.CopyFromSystemMemory(bytes, destinationOffsetInBytes);
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        m_buffer.CopyToSystemMemory(bytes, offsetInBytes);
    }

private:
    BufferD3D11 m_buffer;
};

} // namespace arda::renderer::d3d11
```

Unlike GL, binding an index buffer never touches a vertex array, so there's
no "unbind the vertex array first" dance: nothing is bound until a draw.

### `src/d3d11/vertexarray/VertexArrayD3D11.h` and `.cpp`

D3D11 has no vertex array object. The base class already stores the
attributes, the index buffer and the dirty flags, so `VertexArrayD3D11`
only binds them and owns the input layout cache.

```cpp
// src/d3d11/vertexarray/VertexArrayD3D11.h
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/vertexarray/VertexArray.h>

#include <string>
#include <unordered_map>

namespace arda::renderer::d3d11 {

class DeviceD3D11;
class ShaderProgramD3D11;

class VertexArrayD3D11 final : public VertexArray {
public:
    VertexArrayD3D11(const DeviceD3D11& device, int maximumNumberOfAttributes)
        : VertexArray(maximumNumberOfAttributes), m_device(device) {}

    // Applies the changes the base class recorded. Returns true if the buffers
    // must be bound again, even if this vertex array is already bound.
    bool Clean();

    // IASetVertexBuffers for every slot (null for empty ones) and IASetIndexBuffer.
    void BindBuffers(ID3D11DeviceContext* context) const;

    // The input layout for this vertex array and this program's vertex shader.
    // Created on first use; nullptr if the shader reads no vertex buffers at all.
    ID3D11InputLayout* InputLayoutFor(const ShaderProgramD3D11& program);

private:
    // Everything an input layout depends on from this side: each attribute's location and format.
    std::string FormatKey() const;

    const DeviceD3D11& m_device;
    std::string m_formatKey;
    DXGI_FORMAT m_indexFormat = DXGI_FORMAT_UNKNOWN;   // what BindBuffers binds the index buffer as

    // Keyed by ShaderProgramD3D11::InputSignatureKey(). Cleared when FormatKey() changes.
    std::unordered_map<std::string, ComPtr<ID3D11InputLayout>> m_inputLayouts;
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/vertexarray/VertexArrayD3D11.cpp
#include "d3d11/vertexarray/VertexArrayD3D11.h"
#include "d3d11/DeviceD3D11.h"
#include "d3d11/TypeConverterD3D11.h"
#include "d3d11/buffers/IndexBufferD3D11.h"
#include "d3d11/buffers/VertexBufferD3D11.h"
#include "d3d11/shaders/ShaderProgramD3D11.h"

#include <format>
#include <stdexcept>
#include <utility>
#include <vector>

namespace arda::renderer::d3d11 {

bool VertexArrayD3D11::Clean() {
    bool changed = false;

    if (m_attributesDirty) {
        for (AttributeSlot& slot : m_slots) {
            slot.dirty = false;   // D3D11 rebinds every slot at once, so per-slot flags aren't needed
        }
        m_attributesDirty = false;
        changed = true;

        // A new buffer with the same format keeps the layouts; a new format invalidates them.
        std::string formatKey = FormatKey();
        if (formatKey != m_formatKey) {
            m_formatKey = std::move(formatKey);
            m_inputLayouts.clear();
        }
    }

    // The index type is set by the index buffer's last CopyFromSystemMemory, so
    // it can change without SetIndexBuffer being called.
    const DXGI_FORMAT indexFormat = m_indexBuffer ? ToDxgiFormat(m_indexBuffer->Datatype()) : DXGI_FORMAT_UNKNOWN;
    if (m_indexBufferDirty || indexFormat != m_indexFormat) {
        m_indexBufferDirty = false;
        m_indexFormat = indexFormat;
        changed = true;
    }

    return changed;
}

void VertexArrayD3D11::BindBuffers(ID3D11DeviceContext* context) const {
    const std::size_t count = m_slots.size();
    std::vector<ID3D11Buffer*> buffers(count, nullptr);
    std::vector<UINT> strides(count, 0);
    std::vector<UINT> offsets(count, 0);

    for (std::size_t location = 0; location < count; ++location) {
        if (const std::optional<VertexBufferAttribute>& attribute = m_slots[location].attribute) {
            buffers[location] = static_cast<const VertexBufferD3D11&>(*attribute->vertexBuffer).Native();
            strides[location] = static_cast<UINT>(attribute->strideInBytes);
            offsets[location] = static_cast<UINT>(attribute->offsetInBytes);   // slot N starts at the attribute's offset
        }
    }
    context->IASetVertexBuffers(0, static_cast<UINT>(count), buffers.data(), strides.data(), offsets.data());

    if (m_indexBuffer) {
        context->IASetIndexBuffer(static_cast<const IndexBufferD3D11&>(*m_indexBuffer).Native(), m_indexFormat, 0);
    } else {
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    }
}

ID3D11InputLayout* VertexArrayD3D11::InputLayoutFor(const ShaderProgramD3D11& program) {
    const std::string& key = program.InputSignatureKey();
    if (auto it = m_inputLayouts.find(key); it != m_inputLayouts.end()) {
        return it->second.Get();
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
    for (const ShaderVertexAttribute& shaderAttribute : program.VertexAttributes()) {
        const int location = shaderAttribute.location;
        if (location < 0 || location >= MaximumNumberOfAttributes()) {
            throw std::invalid_argument(std::format(
                "Vertex shader input '{}' uses location {}, but Direct3D 11 has input slots 0 to {}",
                shaderAttribute.name, location, MaximumNumberOfAttributes() - 1));
        }
        const std::optional<VertexBufferAttribute>& attribute = Attribute(location);
        if (!attribute) {
            // GL would silently read (0, 0, 0, 1); D3D11 refuses to create the layout.
            throw std::invalid_argument(std::format(
                "The vertex array has no attribute at location {}, which the shader reads as '{}'",
                location, shaderAttribute.name));
        }

        D3D11_INPUT_ELEMENT_DESC element{};
        element.SemanticName = shaderAttribute.name.c_str();   // lives as long as the program
        element.SemanticIndex = static_cast<UINT>(location);
        element.Format = ToDxgiFormat(attribute->componentDatatype, attribute->numberOfComponents,
                                      attribute->normalize);
        element.InputSlot = static_cast<UINT>(location);       // one slot per attribute
        element.AlignedByteOffset = 0;                         // the offset is in IASetVertexBuffers
        element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        element.InstanceDataStepRate = 0;
        elements.push_back(element);
    }

    ComPtr<ID3D11InputLayout> layout;
    if (!elements.empty()) {   // a shader that reads no vertex buffers (e.g. only SV_VertexID) needs no layout
        ID3DBlob* code = program.VertexShaderCode();
        ThrowIfFailed(m_device.Native()->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()),
                                                           code->GetBufferPointer(), code->GetBufferSize(),
                                                           &layout),
                      "CreateInputLayout");
    }
    return m_inputLayouts.emplace(key, std::move(layout)).first->second.Get();
}

std::string VertexArrayD3D11::FormatKey() const {
    std::string key;
    for (std::size_t location = 0; location < m_slots.size(); ++location) {
        if (const std::optional<VertexBufferAttribute>& attribute = m_slots[location].attribute) {
            key += std::format("{}:{}:{}:{};", location, static_cast<int>(attribute->componentDatatype),
                               attribute->numberOfComponents, attribute->normalize ? 1 : 0);
        }
    }
    return key;
}

} // namespace arda::renderer::d3d11
```

- **`if (const auto& x = ...)`** declares a reference inside the condition;
  the `if` tests the `std::optional`'s `operator bool`.
- **`BindBuffers` binds all 32 slots in one call.** Empty slots get a null
  buffer. That is simpler than tracking which slots changed, and one call
  with 32 entries costs about the same as one with 1.
- **The input layout cache has two levels of key:** `m_formatKey` (this
  vertex array's formats) invalidates the whole map when attributes change
  format, and the program's input signature selects the entry. A vertex
  array drawn with two different programs holds two layouts.

> **Why the cache lives in the vertex array:** the layout depends on the
> pair (vertex formats, vertex shader signature). The vertex array changes
> formats rarely and is drawn with few programs, so a small map per vertex
> array stays small and is freed with the vertex array. Keying on the
> *signature* rather than the program means programs that share a vertex
> shader layout (common with a shader cache) share the layout too.

> **D3D11 note — formats and shader types must agree:** GL converts
> integer vertex data to float for a `vec4` input unless you use
> `glVertexAttribIPointer`. D3D11 does not convert: an `R8G8B8A8_UINT`
> element feeds a `uint4` input, and an `R8G8B8A8_UNORM` element (a
> *normalized* `UnsignedByte` attribute) feeds a `float4`. So non-normalized
> integer attributes need `int`/`uint` inputs in HLSL. And DXGI has no
> 3-component 8- or 16-bit formats, so `ToDxgiFormat` throws for those.
> That includes `VertexAttributeRGB` (3 bytes per vertex) and
> `VertexAttributeHalfFloatVector3` meshes: use `VertexAttributeRGBA` and
> 4-component half floats for meshes meant for both APIs. A 3-component
> `Float` attribute is fine (`R32G32B32_FLOAT`), and a `float4` input fed
> from it gets w = 1, exactly as in GL.

### `ContextD3D11`: vertex arrays and drawing

In `src/d3d11/ContextD3D11.cpp`, add these includes:

```cpp
#include "d3d11/TypeConverterD3D11.h"
#include "d3d11/shaders/ShaderProgramD3D11.h"
#include "d3d11/vertexarray/VertexArrayD3D11.h"
```

replace the `DoCreateVertexArray`, `DoDraw` and `DoDrawRange` stubs, and add
the `Apply*` functions:

```cpp
std::shared_ptr<VertexArray> ContextD3D11::DoCreateVertexArray() {
    // D3D11 vertex data isn't tied to a context; the context creates vertex arrays
    // only because the interface says so (GL's vertex array objects are per context).
    return std::make_shared<VertexArrayD3D11>(m_deviceD3D11, GetDevice().Limits().maximumNumberOfVertexAttributes);
}

void ContextD3D11::DoDraw(PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(primitiveType, drawState, sceneState);

    const VertexArray& vertexArray = *drawState.vertexArray;
    if (const auto& indexBuffer = vertexArray.GetIndexBuffer()) {
        Immediate()->DrawIndexed(static_cast<UINT>(indexBuffer->Count()), 0, 0);
    } else {
        Immediate()->Draw(static_cast<UINT>(vertexArray.MaximumArrayIndex() + 1), 0);
    }
}

void ContextD3D11::DoDrawRange(PrimitiveType primitiveType, int offset, int count,
                               const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(primitiveType, drawState, sceneState);

    // Both take counts and starting positions in elements (indices or vertices), not bytes.
    if (drawState.vertexArray->GetIndexBuffer()) {
        Immediate()->DrawIndexed(static_cast<UINT>(count), static_cast<UINT>(offset), 0);
    } else {
        Immediate()->Draw(static_cast<UINT>(count), static_cast<UINT>(offset));
    }
}

void ContextD3D11::ApplyBeforeDraw(PrimitiveType primitiveType, const DrawState& drawState,
                                   const SceneState& sceneState) {
    EnsureActive();
    ApplyFramebuffer();                       // first: it unbinds textures that are about to be rendered into
    ApplyRenderState(drawState.renderState);  // the viewport conversion depends on the render target
    ApplyPrimitiveTopology(primitiveType);
    ApplyShaderProgram(drawState, sceneState);
    ApplyVertexArray(drawState);              // after the program: the input layout depends on its vertex shader
    CleanTextureUnits();                      // after the framebuffer: a render target can't also be read
}

void ContextD3D11::ApplyPrimitiveTopology(PrimitiveType primitiveType) {
    const D3D11_PRIMITIVE_TOPOLOGY topology = ToD3D(primitiveType);   // throws for LineLoop and TriangleFan
    if (m_primitiveTopology != topology) {
        Immediate()->IASetPrimitiveTopology(topology);
        m_primitiveTopology = topology;
    }
}

void ContextD3D11::ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState) {
    auto& program = static_cast<ShaderProgramD3D11&>(*drawState.shaderProgram);
    if (m_boundShaderProgram.lock() != drawState.shaderProgram) {
        program.Bind(Immediate());
        m_boundShaderProgram = drawState.shaderProgram;
    }
    program.Clean(*this, drawState, sceneState);
}

void ContextD3D11::ApplyVertexArray(const DrawState& drawState) {
    auto& vertexArray = static_cast<VertexArrayD3D11&>(*drawState.vertexArray);
    const auto& program = static_cast<const ShaderProgramD3D11&>(*drawState.shaderProgram);

    const bool buffersChanged = vertexArray.Clean();   // always call it: it also updates the layout cache
    if (buffersChanged || m_boundVertexArray.lock() != drawState.vertexArray) {
        vertexArray.BindBuffers(Immediate());
        m_boundVertexArray = drawState.vertexArray;
    }

    ID3D11InputLayout* layout = vertexArray.InputLayoutFor(program);
    if (m_inputLayout != layout) {
        Immediate()->IASetInputLayout(layout);
        m_inputLayout = layout;
    }
}

// TEMPORARY until Step 5 replaces it.
void ContextD3D11::CleanTextureUnits() {}
```

- **The order in `ApplyBeforeDraw` matters**, and differs from GL's. The
  framebuffer comes first, because the viewport and scissor conversions
  depend on the render target, and because D3D11 refuses to read a texture
  that is bound as a render target (Step 6). The vertex array comes after
  the program, because the input layout is created against the program's
  vertex shader.
- **`static_cast` to the backend types** is the same downcast `ContextGL3x`
  does ([Step 0](00-setup.md#cpp-static-cast)): a D3D11 context only ever
  sees D3D11 objects.
- **`m_boundShaderProgram` and `m_boundVertexArray` are `std::weak_ptr`s**,
  for the reason explained in [Step 3](03-vertex-data.md): a new object
  created at a destroyed object's address must still be bound.

> **Why `PrimitiveRestart` has only `enabled` (portability table, "Primitive
> restart"):** D3D11's strip topologies *always* restart at index `0xFFFF`
> (16-bit indices) or `0xFFFFFFFF` (32-bit) in an indexed draw. There is no
> switch and no other index. The GL backend fixes the index to the same
> values, so both behave alike whenever restart is enabled. The one
> difference that remains: with `enabled = false`, GL treats `0xFFFF` as an
> ordinary vertex index in a strip, and D3D11 still cuts the strip there.
> Don't put the maximum index in a strip's index buffer unless you want a
> restart. List topologies ignore the value on both APIs.

### `DeviceD3D11`: create buffers

In `src/d3d11/DeviceD3D11.cpp`, add

```cpp
#include "d3d11/buffers/IndexBufferD3D11.h"
#include "d3d11/buffers/VertexBufferD3D11.h"
```

and replace the two buffer stubs:

```cpp
std::shared_ptr<VertexBuffer> DeviceD3D11::DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<VertexBufferD3D11>(*this, usageHint, sizeInBytes);
}

std::shared_ptr<IndexBuffer> DeviceD3D11::DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<IndexBufferD3D11>(*this, usageHint, sizeInBytes);
}
```

### Milestones 3A and 3B on D3D11

Use Step 3's `main` with the device created as Direct3D 11 and these
shaders. Everything else, including the `std::array` of positions, the
`uint16_t` indices and the `Mesh` of Milestone 3B, is unchanged.

```cpp
const char* vertexShader = R"(
float4 main(float3 position : position0) : SV_Position
{
    return float4(position, 1.0);
})";

const char* pixelShader = R"(
float3 u_color;
void main(out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(u_color, 1.0);
})";

auto sp = device->CreateShaderProgram(vertexShader, pixelShader);
sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1, 0, 0));
```

- **`float3 position : position0`** matches the 3-float `Vector3<float>`
  vertex data. `float4 position : position0` would work too: the input
  assembler fills the missing w with 1.
- **The triangle appears with the same orientation as on GL.** Drawing to
  the window needs no flip: NDC +y is up on both APIs, and D3D11's top-left
  viewport origin is handled by `ToTopLeftY`.

Things to try, as in Step 3:

- `PrimitiveType::LineLoop` now throws with a message. Use `LineStrip` with
  the indices `{0, 1, 2, 0}`.
- `context.Draw(PrimitiveType::Triangles, 0, 3, drawState, sceneState)`
  draws through `DoDrawRange`.
- Remove the index buffer, so the draw uses `Draw` instead of `DrawIndexed`.
- Rename the shader input to `positio0`. With a `Mesh` (3B),
  `CreateMeshBuffers` throws, because no mesh attribute has that name. With
  raw buffers (3A) it still draws: the input layout takes its semantic from
  the shader, and only the location (the semantic index) selects the
  vertex array slot, exactly as only the location matters in GL. Now change
  it to `position1`: `InputLayoutFor` throws, because the vertex array has
  nothing at location 1.
- Comment out `ForgetRenderTargets()` in `SwapBuffers`. The triangle shows
  for one frame and disappears.

The vertex data tests (`VertexDataTests`) are GL-only as written. Loop them
over both APIs as shown in [Tests](#tests) at the end of this guide.

---

## Step 4 on D3D11: the render-target Y flip

[Step 4](04-automatic-uniforms.md) is almost entirely API-agnostic: the
camera, `SceneState`, the automatic uniform registries and the shader cache
work on D3D11 unchanged, because `ShaderProgramD3D11` calls
`InitializeAutomaticUniforms` and `SetDrawAutomaticUniforms` exactly as the
GL program does. Two things differ between the APIs, and both show up in the
matrices.

### Clip-space depth

> **Why `Device::ClipDepthRange()` (portability table, "Clip-space
> depth"):** after the perspective divide, GL keeps points whose depth is in
> [-1, 1] and D3D keeps [0, 1]. A projection matrix decides which range the
> near and far planes land in, so it has to know. That is why
> `CreatePerspectiveFieldOfView` and every `SceneState` projection take a
> `core::ClipDepth`, and the automatic uniforms pass
> `context.GetDevice().ClipDepthRange()`, which is `ZeroToOne` on D3D11.
> **Pitfall:** hand-build a GL projection (`ClipDepth::NegativeOneToOne`)
> and use it on D3D11, and the NDC range [-1, 0) is clipped away. Because
> perspective depth is non-linear, that half of NDC is only a thin slab in
> front of the camera: everything nearer than about twice the near-plane
> distance. So the bug hides until something comes close to the camera and
> vanishes. It also halves the depth buffer's usable precision. Always
> build projections through `ClipDepthRange()`.

### The flip

You already made the public change in
[Before you start](#before-you-start-the-one-public-change), and Step 0's
starter `ContextD3D11.cpp` already has the final override:

```cpp
bool ContextD3D11::FlipsRenderTargetY() const {
    return GetFramebuffer() != nullptr;
}
```

Here is why it exists, and everything it touches.

> **D3D11 note — which way is up in a render target (portability table,
> "Render-to-texture orientation"):** both APIs agree that texture
> coordinate v = 0 is row 0 of a texture's memory, and that the first row
> you upload *is* row 0. So an uploaded image samples identically on both
> (Step 5). They disagree about *rendering*:
> - GL maps NDC y = -1 (the bottom of the view) to window row 0, and a
>   framebuffer's row 0 is its texture's row 0. The bottom of the image lands
>   in row 0.
> - D3D11 maps NDC y = +1 (the top of the view) to render-target row 0,
>   because its window coordinates start at the top-left. The *top* of the
>   image lands in row 0.
>
> So a texture rendered by D3D11 is stored upside down compared with GL.
> Sample it with the texture coordinates that work on GL, and the picture is
> mirrored; read it back with `CopyToBuffer`, and row 0 is the top.

> **Why flip in clip space, and not somewhere else:** there are three
> places the difference could be fixed.
> 1. **In every shader that samples a render target** (`1.0 - uv.y`). But
>    the shader doesn't know whether a texture was rendered or loaded, and
>    the fix would have to be written in every HLSL shader, forever.
> 2. **When reading pixels back.** That fixes `CopyToBuffer` and `Save`, but
>    not sampling, which is what render-to-texture is for.
> 3. **While rendering into a framebuffer, flip clip-space y** (multiply by
>    `diag(1, -1, 1, 1)`). D3D11 then maps what *would* have been the top of
>    the view to the *bottom* of the target, so row 0 holds the bottom of the
>    image, exactly as on GL. Every consumer downstream (sampling, readback,
>    `Save`, the next render pass) sees identical memory on both APIs.
>
> arda does 3, in one place: `Context::ClipSpaceTransform()`, multiplied into
> every automatic uniform that produces clip coordinates. The window's back
> buffer is never flipped; nothing reads it back, and on-screen the image is
> already the right way up.

The flip has four consequences, and the backend handles each one:

| Consequence | Where it's handled |
|---|---|
| Clip coordinates are mirrored in y | `ClipSpaceTransform()` in the automatic uniforms (Before you start) |
| Every triangle's winding reverses on screen, so front faces look like back faces | `MakeRasterizerKey(renderState, FlipsRenderTargetY())` swaps the front-face winding (Step 1) |
| The viewport and scissor no longer need the top-left conversion: D3D row r is now GL row r | `ToTopLeftY` returns `bottom` unchanged while a framebuffer is bound (Step 1) |
| A shader that writes clip coordinates itself (no matrix) isn't flipped | Multiply by the `og_clipSpaceFlipY` automatic uniform (Before you start) |

> **Why the viewport isn't converted while a framebuffer is bound:** take a
> 32×32 framebuffer and a viewport `{0, 0, 16, 16}`, the bottom-left
> quadrant in arda's (GL's) convention, which is memory rows 0-15. With the
> flip, the bottom of the view (y = -1, flipped to +1) lands at
> `TopLeftY`, and D3D11's `TopLeftY` counts memory rows from row 0. So
> `TopLeftY = bottom = 0` puts the viewport in rows 0-15, where GL puts it.
> The window formula, `targetHeight - (bottom + height) = 16`, would put it
> in rows 16-31, the wrong half. The cross-API tests check exactly this case.

> **D3D11 note — `SV_Position` in pixel shaders:** a pixel shader's
> `SV_Position.xy` is the pixel's window position, counted from the top-left
> on D3D11 and from the bottom-left (`gl_FragCoord`) on GL. While a
> framebuffer is bound, the flip makes the two numerically equal; on the
> window, `SV_Position.y = height - gl_FragCoord.y`. Shaders that compare
> pixel positions with `og_viewportTransformationMatrix` output must take
> this into account on the window (see the D3D11 check in
> [Step 4](04-automatic-uniforms.md)). The flip also negates screen-space
> derivatives in y (`ddy` in HLSL, `dFdy` in GLSL) while a framebuffer is
> bound; only shaders that use their sign care.

### Milestone: the triangle through a camera

Step 4's milestone, with the device created as Direct3D 11 and this vertex
shader (the pixel shader is Step 3's):

```hlsl
float4x4 og_modelViewPerspectiveMatrix;

float4 main(float4 position : position0) : SV_Position
{
    return mul(og_modelViewPerspectiveMatrix, position);
}
```

The xz-plane triangle appears with x to the right and z up, the same as on
GL, and keeps its shape when the window is resized. Things to check:

- **Swap to `mul(position, og_modelViewPerspectiveMatrix)`.** The triangle
  disappears or is distorted: that computes `Mᵀ · v`.
- **Declare `row_major float4x4 og_modelViewPerspectiveMatrix;`.**
  `CreateShaderProgram` throws (Step 2 on D3D11).
- **Set `sceneState.camera.perspectiveNearPlaneDistance = 1.0`** and move
  the eye to 1.5 units from the triangle. It is still drawn: with the right
  clip depth range, everything past the near plane survives.

---

## Step 5 on D3D11: textures, samplers and texture units

This step mirrors [Step 5](05-textures.md). Read 3.6.5 ("Textures in
Direct3D") again first. `Texture2D`, the pixel buffers, `TextureSampler`,
`TextureUnits` and `Image` are unchanged.

### D3D11 concepts for this step

> **D3D11 note — shader resource views and samplers vs. GL texture units:**
> a GL texture unit is one slot that holds a texture *and*, since GL 3.3, a
> sampler object; a `sampler2D` uniform names the unit. D3D11 keeps the two
> apart, per shader stage:
> - **Texture registers `t0`–`t127`** hold shader resource views
>   (`VSSetShaderResources`, `PSSetShaderResources`, ...). In HLSL, a
>   `Texture2D` object is bound to one: `Texture2D og_texture0 : register(t0);`
> - **Sampler registers `s0`–`s15`** hold `ID3D11SamplerState` objects
>   (`PSSetSamplers`, ...). In HLSL: `SamplerState og_sampler0 : register(s0);`
> - The shader combines them at the call site:
>   `og_texture0.Sample(og_sampler0, uv)`. Any texture can be sampled with
>   any sampler.
>
> Bindings are per stage: a texture bound only with `PSSetShaderResources`
> is invisible to the vertex shader. GL's units are shared by every stage.

> **Why texture unit N is `tN` + `sN` (portability table, "Sampler
> binding"):** arda's `TextureUnit` pairs one texture with one sampler, as
> GL does. Binding unit N's view to `tN` and its sampler to `sN`, in the
> vertex, geometry and pixel stages, gives D3D11 shaders GL's model. The
> `og_textureN` link automatic uniforms (which set a `sampler2D` uniform to
> N) have nothing to do on D3D11: HLSL textures aren't loose globals, so
> they are never in `Uniforms()`, and the names simply never match. The
> `register(tN)` in the shader does their job. Sixteen sampler registers is
> why `DeviceLimits::numberOfTextureUnits` is 16.

> **D3D11 note — no "last texture unit" trick:** `Texture2DGL3x` binds a
> texture to the last unit before `glTexSubImage2D`, because GL edits
> whatever is *bound*, and `ContextGL3x` rebinds that unit before every draw
> (3.6.4). D3D11 functions take the resource as an argument
> (`UpdateSubresource(texture, ...)`, `GenerateMips(view)`), so creating or
> updating a texture never disturbs a binding.

> **D3D11 note — creating textures:** a `D3D11_TEXTURE2D_DESC` fixes
> everything up front, like `Texture2DDescription`: size, format, mip count
> (`MipLevels = 0` means the full chain), `Usage`, and `BindFlags` naming
> every way the texture will be used (`SHADER_RESOURCE`, `RENDER_TARGET`,
> `DEPTH_STENCIL`). A texture created without `RENDER_TARGET` can never be
> rendered into. Not every format supports every use on every GPU:
> `CheckFormatSupport` answers per format (`R9G9B9E5_SHAREDEXP` can be
> sampled but not rendered to, for example). `GenerateMips` needs
> `RENDER_TARGET` (it renders each level from the one above), the
> `D3D11_RESOURCE_MISC_GENERATE_MIPS` flag, and a format with
> `D3D11_FORMAT_SUPPORT_MIP_AUTOGEN`.

> **D3D11 note — typeless formats for depth textures:** a depth texture
> must be written through a depth-stencil view (`D32_FLOAT`) and read
> through a shader resource view (`R32_FLOAT`). A view may only reinterpret
> its resource within the same *format family*, and only if the resource
> was created *typeless*: `R32_TYPELESS` can be viewed as `D32_FLOAT` or
> `R32_FLOAT`; a `D32_FLOAT` resource can't be viewed as anything else. So
> every depth format in `ToDxgiFormats` is a typeless resource plus two view
> formats. `R24_UNORM_X8_TYPELESS` means "the 24 depth bits as a normalized
> value, ignoring the 8 stencil bits".

> **D3D11 note — no pixel buffer objects:** GL's pixel buffers are GPU
> buffers that `glTexSubImage2D` and `glGetTexImage` read from and write to,
> possibly asynchronously. D3D11 has no equivalent: `UpdateSubresource`
> copies from CPU memory, and readback goes through a staging texture. So
> the D3D11 pixel buffers are plain system memory, and the conversion to and
> from the texture's layout happens on the CPU.

> **Why RGB8 becomes RGBA8 on D3D11 (portability table, "24-bit RGB
> textures"):** DXGI has no 3-channel 8- or 16-bit format. GPUs fetch texels
> in power-of-two sizes, and a 3-byte texel isn't one. GL accepts `GL_RGB8`
> and most drivers quietly store it as RGBA8. `Texture2DD3D11` does the same
> explicitly: `RedGreenBlue8` is an `R8G8B8A8_UNORM` texture, and the upload
> pads each pixel with alpha = 1. Reading it back as `RedGreenBlue` drops the
> padding again, so a round trip returns the original bytes. The same
> applies to `RedGreenBlue16`, `RedGreenBlue16f`, `SRedGreenBlue8` and
> `RedGreenBlue32f`. The 3-channel 8- and 16-bit *integer* formats throw
> `InsufficientVideoCardException`; they would need integer padding, and no
> example uses them.

### `src/d3d11/buffers/PixelBuffersD3D11.h`

```cpp
#pragma once

#include <arda/renderer/buffers/ReadPixelBuffer.h>
#include <arda/renderer/buffers/WritePixelBuffer.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace arda::renderer::d3d11 {

// D3D11 has no pixel buffer objects: UpdateSubresource reads system memory directly.
class WritePixelBufferD3D11 final : public WritePixelBuffer {
public:
    WritePixelBufferD3D11(PixelBufferHint usageHint, std::size_t sizeInBytes)
        : m_usageHint(usageHint), m_bytes(sizeInBytes) {}

    std::span<const std::byte> Bytes() const { return m_bytes; }

    std::size_t SizeInBytes() const override { return m_bytes.size(); }
    PixelBufferHint UsageHint() const override { return m_usageHint; }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        if (!bytes.empty()) {   // memcpy with a null pointer is undefined, even for 0 bytes
            std::memcpy(m_bytes.data() + destinationOffsetInBytes, bytes.data(), bytes.size());
        }
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        if (!bytes.empty()) {
            std::memcpy(bytes.data(), m_bytes.data() + offsetInBytes, bytes.size());
        }
    }

private:
    PixelBufferHint m_usageHint;
    std::vector<std::byte> m_bytes;
};

// Texture2DD3D11::DoCopyToBuffer fills one of these from a mapped staging texture.
class ReadPixelBufferD3D11 final : public ReadPixelBuffer {
public:
    ReadPixelBufferD3D11(PixelBufferHint usageHint, std::vector<std::byte> bytes)
        : m_usageHint(usageHint), m_bytes(std::move(bytes)) {}

    std::size_t SizeInBytes() const override { return m_bytes.size(); }
    PixelBufferHint UsageHint() const override { return m_usageHint; }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        if (!bytes.empty()) {
            std::memcpy(m_bytes.data() + destinationOffsetInBytes, bytes.data(), bytes.size());
        }
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        if (!bytes.empty()) {
            std::memcpy(bytes.data(), m_bytes.data() + offsetInBytes, bytes.size());
        }
    }

private:
    PixelBufferHint m_usageHint;
    std::vector<std::byte> m_bytes;
};

} // namespace arda::renderer::d3d11
```

The range checks are done by the public templates in `WritePixelBuffer` and
`ReadPixelBuffer`, as for the GL versions. `ReadPixelBufferD3D11` takes its
vector by value and moves it in: the texture builds the bytes, then hands
them over without a copy ([sink parameters](06-framebuffers.md)).

### `src/d3d11/textures/PixelConversionD3D11.h` and `.cpp`

GL converts between the pixel buffer's layout (`ImageFormat`,
`ImageDatatype`, row alignment) and the texture's internal format for you,
in both directions. D3D11 doesn't: `UpdateSubresource` copies bytes that must
already be in the texture's DXGI layout, and a mapped staging texture gives
you that layout back. This module does GL's conversions on the CPU. It
converts every value through a `double` in [0, 1] (normalized formats) or
the value itself (integer formats), which is exact for every combination
arda uses.

```cpp
// src/d3d11/textures/PixelConversionD3D11.h
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/textures/ImageFormat.h>
#include <arda/renderer/textures/TextureFormat.h>

#include <cstddef>
#include <span>
#include <vector>

namespace arda::renderer::d3d11 {

// How the DXGI format chosen by ToDxgiFormats stores one component (or texel).
enum class TexelKind {
    UNorm8, UNorm16, Float16, Float32,
    SInt8, UInt8, SInt16, UInt16, SInt32, UInt32,
    Packed32,              // R10G10B10A2_UNORM, R11G11B10_FLOAT, R9G9B9E5_SHAREDEXP: copied as is
    Depth16, Depth24Stencil8, Depth32f, Depth32fStencil8,
};

struct TexelLayout {
    TexelKind kind;
    int channels;        // stored channels: 4 for the padded RGB formats, 1 for depth
    int bytesPerTexel;
};

// Must agree with ToDxgiFormats. Throws InsufficientVideoCardException for the same formats.
TexelLayout TexelLayoutFor(TextureFormat format);

struct PackedTexels {
    std::vector<std::byte> bytes;   // tightly packed rows
    UINT rowPitch = 0;              // bytes per row, for UpdateSubresource
};

// Pixel-buffer data (rows padded to rowAlignment) -> the texture's DXGI layout.
// Throws std::invalid_argument for combinations GL would also reject, and for
// depth textures, which D3D11 can't update from the CPU.
PackedTexels PackTexels(std::span<const std::byte> source, int width, int height,
                        ImageFormat imageFormat, ImageDatatype datatype, int rowAlignment,
                        TextureFormat textureFormat);

// Mapped texels (rows rowPitch bytes apart) -> pixel-buffer layout, rows padded to rowAlignment.
std::vector<std::byte> UnpackTexels(const std::byte* texels, UINT rowPitch, int width, int height,
                                    TextureFormat textureFormat,
                                    ImageFormat imageFormat, ImageDatatype datatype, int rowAlignment);

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/textures/PixelConversionD3D11.cpp
#include "d3d11/textures/PixelConversionD3D11.h"

#include <arda/core/Half.h>
#include <arda/renderer/Exceptions.h>
#include <arda/renderer/textures/TextureUtility.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace arda::renderer::d3d11 {

namespace {

// --- Reading and writing unaligned values ------------------------------------

template <typename T>
T Load(const std::byte* p) {
    T value;
    std::memcpy(&value, p, sizeof(T));   // pixel data has no alignment guarantee
    return value;
}

template <typename T>
void Store(std::byte* p, T value) {
    std::memcpy(p, &value, sizeof(T));
}

// [0, 1] (or [-1, 1] for signed T) to the full integer range, rounded.
template <typename T>
T ToNormalized(double value) {
    constexpr double maximum = static_cast<double>(std::numeric_limits<T>::max());
    const double minimum = std::is_signed_v<T> ? -1.0 : 0.0;
    return static_cast<T>(std::llround(std::clamp(value, minimum, 1.0) * maximum));
}

// The integer range of T, rounded and clamped.
template <typename T>
T ToInteger(double value) {
    const double low = static_cast<double>(std::numeric_limits<T>::lowest());
    const double high = static_cast<double>(std::numeric_limits<T>::max());
    return static_cast<T>(std::llround(std::clamp(value, low, high)));
}

// --- The pixel-buffer side: ImageFormat and ImageDatatype --------------------

struct ChannelMap {
    int count = 0;
    std::array<int, 4> toRgba{};   // channel i of a pixel is RGBA component toRgba[i]
    bool integer = false;          // an *Integer format: values are not normalized
};

ChannelMap ChannelMapFor(ImageFormat format) {
    switch (format) {
    case ImageFormat::DepthComponent:           return {1, {0, 0, 0, 0}, false};   // depth is component 0
    case ImageFormat::Red:                      return {1, {0, 0, 0, 0}, false};
    case ImageFormat::Green:                    return {1, {1, 0, 0, 0}, false};
    case ImageFormat::Blue:                     return {1, {2, 0, 0, 0}, false};
    case ImageFormat::RedGreen:                 return {2, {0, 1, 0, 0}, false};
    case ImageFormat::RedGreenBlue:             return {3, {0, 1, 2, 0}, false};
    case ImageFormat::RedGreenBlueAlpha:        return {4, {0, 1, 2, 3}, false};
    case ImageFormat::BlueGreenRed:             return {3, {2, 1, 0, 0}, false};
    case ImageFormat::BlueGreenRedAlpha:        return {4, {2, 1, 0, 3}, false};
    case ImageFormat::RedInteger:               return {1, {0, 0, 0, 0}, true};
    case ImageFormat::GreenInteger:             return {1, {1, 0, 0, 0}, true};
    case ImageFormat::BlueInteger:              return {1, {2, 0, 0, 0}, true};
    case ImageFormat::RedGreenInteger:          return {2, {0, 1, 0, 0}, true};
    case ImageFormat::RedGreenBlueInteger:      return {3, {0, 1, 2, 0}, true};
    case ImageFormat::RedGreenBlueAlphaInteger: return {4, {0, 1, 2, 3}, true};
    case ImageFormat::BlueGreenRedInteger:      return {3, {2, 1, 0, 0}, true};
    case ImageFormat::BlueGreenRedAlphaInteger: return {4, {2, 1, 0, 3}, true};
    case ImageFormat::StencilIndex:
    case ImageFormat::DepthStencil:
        throw std::invalid_argument(
            "Direct3D 11: stencil data can't be copied between textures and pixel buffers");
    }
    throw std::invalid_argument("Invalid ImageFormat");
}

// One component of a pixel buffer, as a double. Normalized unless `integer` (GL's rules).
double ReadComponent(const std::byte* p, ImageDatatype datatype, bool integer) {
    switch (datatype) {
    case ImageDatatype::UnsignedByte: {
        const double v = Load<std::uint8_t>(p);
        return integer ? v : v / 255.0;
    }
    case ImageDatatype::Byte: {
        const double v = Load<std::int8_t>(p);
        return integer ? v : std::max(v / 127.0, -1.0);
    }
    case ImageDatatype::UnsignedShort: {
        const double v = Load<std::uint16_t>(p);
        return integer ? v : v / 65535.0;
    }
    case ImageDatatype::Short: {
        const double v = Load<std::int16_t>(p);
        return integer ? v : std::max(v / 32767.0, -1.0);
    }
    case ImageDatatype::UnsignedInt: {
        const double v = Load<std::uint32_t>(p);
        return integer ? v : v / 4294967295.0;
    }
    case ImageDatatype::Int: {
        const double v = Load<std::int32_t>(p);
        return integer ? v : std::max(v / 2147483647.0, -1.0);
    }
    case ImageDatatype::Float:     return Load<float>(p);
    case ImageDatatype::HalfFloat: return core::HalfBitsToFloat(Load<std::uint16_t>(p));
    default:
        break;   // the packed types: only copied as is, for the matching packed texture format
    }
    throw std::invalid_argument("Direct3D 11: this packed ImageDatatype doesn't match the texture's format");
}

void WriteComponent(std::byte* p, ImageDatatype datatype, bool integer, double value) {
    switch (datatype) {
    case ImageDatatype::UnsignedByte:
        Store(p, integer ? ToInteger<std::uint8_t>(value) : ToNormalized<std::uint8_t>(value));
        return;
    case ImageDatatype::Byte:
        Store(p, integer ? ToInteger<std::int8_t>(value) : ToNormalized<std::int8_t>(value));
        return;
    case ImageDatatype::UnsignedShort:
        Store(p, integer ? ToInteger<std::uint16_t>(value) : ToNormalized<std::uint16_t>(value));
        return;
    case ImageDatatype::Short:
        Store(p, integer ? ToInteger<std::int16_t>(value) : ToNormalized<std::int16_t>(value));
        return;
    case ImageDatatype::UnsignedInt:
        Store(p, integer ? ToInteger<std::uint32_t>(value) : ToNormalized<std::uint32_t>(value));
        return;
    case ImageDatatype::Int:
        Store(p, integer ? ToInteger<std::int32_t>(value) : ToNormalized<std::int32_t>(value));
        return;
    case ImageDatatype::Float:
        Store(p, static_cast<float>(value));
        return;
    case ImageDatatype::HalfFloat:
        Store(p, core::FloatToHalfBits(static_cast<float>(value)));
        return;
    default:
        break;
    }
    throw std::invalid_argument("Direct3D 11: this packed ImageDatatype doesn't match the texture's format");
}

// --- The texture side: TexelKind --------------------------------------------

bool IsIntegerKind(TexelKind kind) {
    return kind == TexelKind::SInt8 || kind == TexelKind::UInt8 || kind == TexelKind::SInt16 ||
           kind == TexelKind::UInt16 || kind == TexelKind::SInt32 || kind == TexelKind::UInt32;
}

bool IsDepthKind(TexelKind kind) {
    return kind == TexelKind::Depth16 || kind == TexelKind::Depth24Stencil8 ||
           kind == TexelKind::Depth32f || kind == TexelKind::Depth32fStencil8;
}

// Component c of the texel at p: normalized for UNorm and depth kinds, the value for the others.
double ReadTexelComponent(const std::byte* p, TexelKind kind, int c) {
    switch (kind) {
    case TexelKind::UNorm8:           return Load<std::uint8_t>(p + c) / 255.0;
    case TexelKind::UNorm16:          return Load<std::uint16_t>(p + 2 * c) / 65535.0;
    case TexelKind::Float16:          return core::HalfBitsToFloat(Load<std::uint16_t>(p + 2 * c));
    case TexelKind::Float32:          return Load<float>(p + 4 * c);
    case TexelKind::SInt8:            return Load<std::int8_t>(p + c);
    case TexelKind::UInt8:            return Load<std::uint8_t>(p + c);
    case TexelKind::SInt16:           return Load<std::int16_t>(p + 2 * c);
    case TexelKind::UInt16:           return Load<std::uint16_t>(p + 2 * c);
    case TexelKind::SInt32:           return Load<std::int32_t>(p + 4 * c);
    case TexelKind::UInt32:           return Load<std::uint32_t>(p + 4 * c);
    case TexelKind::Depth16:          return Load<std::uint16_t>(p) / 65535.0;
    case TexelKind::Depth24Stencil8:  return (Load<std::uint32_t>(p) & 0xFFFFFFu) / 16777215.0;   // depth: low 24 bits
    case TexelKind::Depth32f:
    case TexelKind::Depth32fStencil8: return Load<float>(p);   // the float comes first; stencil follows
    case TexelKind::Packed32:         break;
    }
    throw std::logic_error("ReadTexelComponent: unexpected TexelKind");
}

void WriteTexelComponent(std::byte* p, TexelKind kind, int c, double value) {
    switch (kind) {
    case TexelKind::UNorm8:  Store(p + c, ToNormalized<std::uint8_t>(value)); return;
    case TexelKind::UNorm16: Store(p + 2 * c, ToNormalized<std::uint16_t>(value)); return;
    case TexelKind::Float16: Store(p + 2 * c, core::FloatToHalfBits(static_cast<float>(value))); return;
    case TexelKind::Float32: Store(p + 4 * c, static_cast<float>(value)); return;
    case TexelKind::SInt8:   Store(p + c, ToInteger<std::int8_t>(value)); return;
    case TexelKind::UInt8:   Store(p + c, ToInteger<std::uint8_t>(value)); return;
    case TexelKind::SInt16:  Store(p + 2 * c, ToInteger<std::int16_t>(value)); return;
    case TexelKind::UInt16:  Store(p + 2 * c, ToInteger<std::uint16_t>(value)); return;
    case TexelKind::SInt32:  Store(p + 4 * c, ToInteger<std::int32_t>(value)); return;
    case TexelKind::UInt32:  Store(p + 4 * c, ToInteger<std::uint32_t>(value)); return;
    case TexelKind::Packed32:
    case TexelKind::Depth16:
    case TexelKind::Depth24Stencil8:
    case TexelKind::Depth32f:
    case TexelKind::Depth32fStencil8:
        break;
    }
    throw std::logic_error("WriteTexelComponent: unexpected TexelKind");
}

// The one pixel-buffer layout that matches each packed DXGI format bit for bit.
bool MatchesPackedLayout(TextureFormat format, ImageFormat imageFormat, ImageDatatype datatype) {
    switch (format) {
    case TextureFormat::RedGreenBlue10A2:
        return imageFormat == ImageFormat::RedGreenBlueAlpha && datatype == ImageDatatype::UnsignedInt2101010Reversed;
    case TextureFormat::Red11fGreen11fBlue10f:
        return imageFormat == ImageFormat::RedGreenBlue && datatype == ImageDatatype::UnsignedInt10F11F11FReversed;
    case TextureFormat::RedGreenBlue9E5:
        return imageFormat == ImageFormat::RedGreenBlue && datatype == ImageDatatype::UnsignedInt5999Reversed;
    default:
        return false;
    }
}

void CheckIntegerMatch(const ChannelMap& map, TexelKind kind) {
    // GL rejects the same combinations: integer textures need *Integer formats, and vice versa.
    if (map.integer != IsIntegerKind(kind)) {
        throw std::invalid_argument(
            "Integer textures need an *Integer ImageFormat, and other textures a non-integer one");
    }
}

} // namespace

TexelLayout TexelLayoutFor(TextureFormat format) {
    auto layout = [](TexelKind kind, int channels) {
        int componentSize = 4;
        switch (kind) {
        case TexelKind::UNorm8: case TexelKind::SInt8: case TexelKind::UInt8:     componentSize = 1; break;
        case TexelKind::UNorm16: case TexelKind::Float16:
        case TexelKind::SInt16: case TexelKind::UInt16: case TexelKind::Depth16: componentSize = 2; break;
        case TexelKind::Depth32fStencil8:                                          componentSize = 8; break;
        default:                                                                   componentSize = 4; break;
        }
        return TexelLayout{kind, channels, componentSize * channels};
    };
    auto unsupported = [format]() -> TexelLayout {
        throw InsufficientVideoCardException(
            "TextureFormat " + std::to_string(static_cast<int>(format)) + " is not supported by Direct3D 11");
    };

    switch (format) {
    case TextureFormat::RedGreenBlue8:         return layout(TexelKind::UNorm8, 4);    // padded
    case TextureFormat::RedGreenBlue16:        return layout(TexelKind::UNorm16, 4);   // padded
    case TextureFormat::RedGreenBlueAlpha8:    return layout(TexelKind::UNorm8, 4);
    case TextureFormat::RedGreenBlue10A2:      return layout(TexelKind::Packed32, 1);
    case TextureFormat::RedGreenBlueAlpha16:   return layout(TexelKind::UNorm16, 4);
    case TextureFormat::Depth16:               return layout(TexelKind::Depth16, 1);
    case TextureFormat::Depth24:               return layout(TexelKind::Depth24Stencil8, 1);
    case TextureFormat::Red8:                  return layout(TexelKind::UNorm8, 1);
    case TextureFormat::Red16:                 return layout(TexelKind::UNorm16, 1);
    case TextureFormat::RedGreen8:             return layout(TexelKind::UNorm8, 2);
    case TextureFormat::RedGreen16:            return layout(TexelKind::UNorm16, 2);
    case TextureFormat::Red16f:                return layout(TexelKind::Float16, 1);
    case TextureFormat::Red32f:                return layout(TexelKind::Float32, 1);
    case TextureFormat::RedGreen16f:           return layout(TexelKind::Float16, 2);
    case TextureFormat::RedGreen32f:           return layout(TexelKind::Float32, 2);
    case TextureFormat::Red8i:                 return layout(TexelKind::SInt8, 1);
    case TextureFormat::Red8ui:                return layout(TexelKind::UInt8, 1);
    case TextureFormat::Red16i:                return layout(TexelKind::SInt16, 1);
    case TextureFormat::Red16ui:               return layout(TexelKind::UInt16, 1);
    case TextureFormat::Red32i:                return layout(TexelKind::SInt32, 1);
    case TextureFormat::Red32ui:               return layout(TexelKind::UInt32, 1);
    case TextureFormat::RedGreen8i:            return layout(TexelKind::SInt8, 2);
    case TextureFormat::RedGreen8ui:           return layout(TexelKind::UInt8, 2);
    case TextureFormat::RedGreen16i:           return layout(TexelKind::SInt16, 2);
    case TextureFormat::RedGreen16ui:          return layout(TexelKind::UInt16, 2);
    case TextureFormat::RedGreen32i:           return layout(TexelKind::SInt32, 2);
    case TextureFormat::RedGreen32ui:          return layout(TexelKind::UInt32, 2);
    case TextureFormat::RedGreenBlueAlpha32f:  return layout(TexelKind::Float32, 4);
    case TextureFormat::RedGreenBlue32f:       return layout(TexelKind::Float32, 4);   // padded
    case TextureFormat::RedGreenBlueAlpha16f:  return layout(TexelKind::Float16, 4);
    case TextureFormat::RedGreenBlue16f:       return layout(TexelKind::Float16, 4);   // padded
    case TextureFormat::Depth24Stencil8:       return layout(TexelKind::Depth24Stencil8, 1);
    case TextureFormat::Red11fGreen11fBlue10f: return layout(TexelKind::Packed32, 1);
    case TextureFormat::RedGreenBlue9E5:       return layout(TexelKind::Packed32, 1);
    case TextureFormat::SRedGreenBlue8:        return layout(TexelKind::UNorm8, 4);    // padded
    case TextureFormat::SRedGreenBlue8Alpha8:  return layout(TexelKind::UNorm8, 4);
    case TextureFormat::Depth32f:              return layout(TexelKind::Depth32f, 1);
    case TextureFormat::Depth32fStencil8:      return layout(TexelKind::Depth32fStencil8, 1);
    case TextureFormat::RedGreenBlueAlpha32ui: return layout(TexelKind::UInt32, 4);
    case TextureFormat::RedGreenBlue32ui:      return layout(TexelKind::UInt32, 3);
    case TextureFormat::RedGreenBlueAlpha16ui: return layout(TexelKind::UInt16, 4);
    case TextureFormat::RedGreenBlue16ui:      return unsupported();
    case TextureFormat::RedGreenBlueAlpha8ui:  return layout(TexelKind::UInt8, 4);
    case TextureFormat::RedGreenBlue8ui:       return unsupported();
    case TextureFormat::RedGreenBlueAlpha32i:  return layout(TexelKind::SInt32, 4);
    case TextureFormat::RedGreenBlue32i:       return layout(TexelKind::SInt32, 3);
    case TextureFormat::RedGreenBlueAlpha16i:  return layout(TexelKind::SInt16, 4);
    case TextureFormat::RedGreenBlue16i:       return unsupported();
    case TextureFormat::RedGreenBlueAlpha8i:   return layout(TexelKind::SInt8, 4);
    case TextureFormat::RedGreenBlue8i:        return unsupported();
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

PackedTexels PackTexels(std::span<const std::byte> source, int width, int height,
                        ImageFormat imageFormat, ImageDatatype datatype, int rowAlignment,
                        TextureFormat textureFormat) {
    const TexelLayout layout = TexelLayoutFor(textureFormat);
    if (IsDepthKind(layout.kind)) {
        throw std::invalid_argument(
            "Direct3D 11 can't copy CPU data into a depth texture (UpdateSubresource rejects depth-stencil "
            "resources). Render the depth values instead.");
    }

    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    const std::size_t sourceRowPitch = TextureUtility::RequiredSizeInBytes(width, 1, imageFormat, datatype, rowAlignment);
    const std::size_t destinationRowPitch = w * static_cast<std::size_t>(layout.bytesPerTexel);

    PackedTexels result;
    result.rowPitch = static_cast<UINT>(destinationRowPitch);
    result.bytes.resize(destinationRowPitch * h);

    if (layout.kind == TexelKind::Packed32) {
        if (!MatchesPackedLayout(textureFormat, imageFormat, datatype)) {
            throw std::invalid_argument("Direct3D 11: packed textures accept only their own packed ImageDatatype");
        }
        for (std::size_t y = 0; y < h; ++y) {
            std::memcpy(result.bytes.data() + y * destinationRowPitch, source.data() + y * sourceRowPitch,
                        destinationRowPitch);
        }
        return result;
    }

    const ChannelMap map = ChannelMapFor(imageFormat);
    CheckIntegerMatch(map, layout.kind);
    const auto componentSize = static_cast<std::size_t>(TextureUtility::SizeInBytes(datatype));

    for (std::size_t y = 0; y < h; ++y) {
        const std::byte* sourceRow = source.data() + y * sourceRowPitch;
        std::byte* destinationRow = result.bytes.data() + y * destinationRowPitch;
        for (std::size_t x = 0; x < w; ++x) {
            // Missing channels default to (0, 0, 0, 1), as in GL. That default is the RGB padding.
            std::array<double, 4> rgba = {0.0, 0.0, 0.0, 1.0};
            for (int c = 0; c < map.count; ++c) {
                const std::byte* component = sourceRow + (x * static_cast<std::size_t>(map.count) +
                                                          static_cast<std::size_t>(c)) * componentSize;
                rgba[static_cast<std::size_t>(map.toRgba[static_cast<std::size_t>(c)])] =
                    ReadComponent(component, datatype, map.integer);
            }
            std::byte* texel = destinationRow + x * static_cast<std::size_t>(layout.bytesPerTexel);
            for (int c = 0; c < layout.channels; ++c) {
                WriteTexelComponent(texel, layout.kind, c, rgba[static_cast<std::size_t>(c)]);
            }
        }
    }
    return result;
}

std::vector<std::byte> UnpackTexels(const std::byte* texels, UINT rowPitch, int width, int height,
                                    TextureFormat textureFormat,
                                    ImageFormat imageFormat, ImageDatatype datatype, int rowAlignment) {
    const TexelLayout layout = TexelLayoutFor(textureFormat);

    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    const std::size_t destinationRowPitch = TextureUtility::RequiredSizeInBytes(width, 1, imageFormat, datatype, rowAlignment);
    std::vector<std::byte> result(TextureUtility::RequiredSizeInBytes(width, height, imageFormat, datatype, rowAlignment));

    if (layout.kind == TexelKind::Packed32) {
        if (!MatchesPackedLayout(textureFormat, imageFormat, datatype)) {
            throw std::invalid_argument("Direct3D 11: packed textures can only be read as their own packed ImageDatatype");
        }
        for (std::size_t y = 0; y < h; ++y) {
            std::memcpy(result.data() + y * destinationRowPitch, texels + y * rowPitch, w * 4);
        }
        return result;
    }

    const ChannelMap map = ChannelMapFor(imageFormat);
    if (!IsDepthKind(layout.kind)) {
        CheckIntegerMatch(map, layout.kind);
    }
    const auto componentSize = static_cast<std::size_t>(TextureUtility::SizeInBytes(datatype));

    for (std::size_t y = 0; y < h; ++y) {
        const std::byte* sourceRow = texels + y * rowPitch;   // RowPitch may be larger than width * texel size
        std::byte* destinationRow = result.data() + y * destinationRowPitch;
        for (std::size_t x = 0; x < w; ++x) {
            std::array<double, 4> rgba = {0.0, 0.0, 0.0, 1.0};
            const std::byte* texel = sourceRow + x * static_cast<std::size_t>(layout.bytesPerTexel);
            for (int c = 0; c < layout.channels; ++c) {
                rgba[static_cast<std::size_t>(c)] = ReadTexelComponent(texel, layout.kind, c);
            }
            for (int c = 0; c < map.count; ++c) {
                std::byte* component = destinationRow + (x * static_cast<std::size_t>(map.count) +
                                                         static_cast<std::size_t>(c)) * componentSize;
                WriteComponent(component, datatype, map.integer,
                               rgba[static_cast<std::size_t>(map.toRgba[static_cast<std::size_t>(c)])]);
            }
        }
    }
    return result;
}

} // namespace arda::renderer::d3d11
```

- **`Load` and `Store` use `std::memcpy`,** not a cast like
  `*reinterpret_cast<const float*>(p)`. Pixel data has no alignment
  guarantee, and reading a `float` through a pointer to bytes that aren't a
  `float` object is undefined behaviour. The compiler turns a fixed-size
  `memcpy` into one load or store, so it costs nothing.
- **Why a `double` in the middle:** every 8-, 16- and 32-bit integer and
  every `float` fits in a `double` exactly, so an unsigned-byte round trip
  (`b / 255.0 * 255.0`, then rounded) returns `b` exactly, and integer
  textures keep their values.
- **The padded channel** comes from the `{0, 0, 0, 1}` default: an RGB
  source never writes component 3, so alpha is stored as 1 (255).
- **What still throws,** with the reason in the message: uploading to depth
  textures, stencil data (`DepthStencil`/`StencilIndex`), packed datatypes
  that don't match the texture's packed format, and integer/non-integer
  mixes (GL rejects those too).

### `src/d3d11/textures/Texture2DD3D11.h`

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/textures/Texture2D.h>

#include <memory>

namespace arda::renderer::d3d11 {

class DeviceD3D11;

class Texture2DD3D11 final : public Texture2D {
public:
    // Throws InsufficientVideoCardException for formats (or mipmap generation) the GPU can't do.
    Texture2DD3D11(const DeviceD3D11& device, const Texture2DDescription& description);

    ID3D11Texture2D* Native() const { return m_texture.Get(); }
    ID3D11ShaderResourceView* ShaderResourceView() const { return m_shaderResourceView.Get(); }

    // For FramebufferD3D11. Created on first use: most textures are never render targets.
    ID3D11RenderTargetView* RenderTargetView();
    ID3D11DepthStencilView* DepthStencilView();
    bool HasStencil() const;   // the depth view has stencil bits (so Clear may clear them)

protected:
    void DoCopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset, int width, int height,
                          ImageFormat format, ImageDatatype datatype, int rowAlignment) override;
    std::shared_ptr<ReadPixelBuffer> DoCopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                    int rowAlignment) const override;

private:
    const DeviceD3D11& m_device;
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;   // color formats, on first use
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;   // depth formats, on first use
};

} // namespace arda::renderer::d3d11
```

### `src/d3d11/textures/Texture2DD3D11.cpp`

```cpp
#include "d3d11/textures/Texture2DD3D11.h"
#include "d3d11/DeviceD3D11.h"
#include "d3d11/TypeConverterD3D11.h"
#include "d3d11/buffers/PixelBuffersD3D11.h"
#include "d3d11/textures/PixelConversionD3D11.h"

#include <arda/renderer/Exceptions.h>

#include <stdexcept>
#include <utility>

namespace arda::renderer::d3d11 {

Texture2DD3D11::Texture2DD3D11(const DeviceD3D11& device, const Texture2DDescription& description)
    : Texture2D(description), m_device(device) {
    ID3D11Device* d3d = device.Native();
    const DxgiTextureFormats formats = ToDxgiFormats(description.textureFormat);   // throws for missing formats
    const bool depth = description.DepthRenderable();

    // Ask what this GPU can do with the format, instead of letting CreateTexture2D fail.
    UINT support = 0;
    if (FAILED(d3d->CheckFormatSupport(formats.target, &support)) ||
        (support & D3D11_FORMAT_SUPPORT_TEXTURE2D) == 0) {
        throw InsufficientVideoCardException("This GPU doesn't support the texture's format as a 2D texture");
    }
    const bool renderable = depth ? (support & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0
                                  : (support & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0;
    if (description.generateMipmaps && (depth || (support & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) == 0)) {
        throw InsufficientVideoCardException("Direct3D 11 can't generate mipmaps for this texture format");
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(description.width);
    desc.Height = static_cast<UINT>(description.height);
    desc.MipLevels = description.generateMipmaps ? 0 : 1;   // 0: the full chain, down to 1x1
    desc.ArraySize = 1;
    desc.Format = formats.texture;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (renderable) {
        desc.BindFlags |= depth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
    }
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = description.generateMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

    ThrowIfFailed(d3d->CreateTexture2D(&desc, nullptr, &m_texture), "CreateTexture2D");
    SetDebugName(m_texture.Get(), "Texture2D");

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
    viewDesc.Format = formats.shaderResource;
    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MostDetailedMip = 0;
    viewDesc.Texture2D.MipLevels = static_cast<UINT>(-1);   // every level
    ThrowIfFailed(d3d->CreateShaderResourceView(m_texture.Get(), &viewDesc, &m_shaderResourceView),
                  "CreateShaderResourceView");
}

ID3D11RenderTargetView* Texture2DD3D11::RenderTargetView() {
    if (!m_renderTargetView) {
        D3D11_TEXTURE2D_DESC desc{};
        m_texture->GetDesc(&desc);
        if ((desc.BindFlags & D3D11_BIND_RENDER_TARGET) == 0) {
            throw InsufficientVideoCardException("This texture's format can't be a render target on this GPU");
        }
        D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
        viewDesc.Format = ToDxgiFormats(Description().textureFormat).target;
        viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;   // render into level 0, like glFramebufferTexture(..., 0)
        ThrowIfFailed(m_device.Native()->CreateRenderTargetView(m_texture.Get(), &viewDesc, &m_renderTargetView),
                      "CreateRenderTargetView");
    }
    return m_renderTargetView.Get();
}

ID3D11DepthStencilView* Texture2DD3D11::DepthStencilView() {
    if (!m_depthStencilView) {
        D3D11_TEXTURE2D_DESC desc{};
        m_texture->GetDesc(&desc);
        if ((desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) == 0) {
            throw InsufficientVideoCardException("This texture's format can't be a depth buffer on this GPU");
        }
        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc{};
        viewDesc.Format = ToDxgiFormats(Description().textureFormat).target;
        viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        viewDesc.Flags = 0;   // writable depth and stencil
        viewDesc.Texture2D.MipSlice = 0;
        ThrowIfFailed(m_device.Native()->CreateDepthStencilView(m_texture.Get(), &viewDesc, &m_depthStencilView),
                      "CreateDepthStencilView");
    }
    return m_depthStencilView.Get();
}

bool Texture2DD3D11::HasStencil() const {
    const DXGI_FORMAT format = ToDxgiFormats(Description().textureFormat).target;
    return format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
}

void Texture2DD3D11::DoCopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset,
                                      int width, int height, ImageFormat format, ImageDatatype datatype,
                                      int rowAlignment) {
    const auto& buffer = static_cast<const WritePixelBufferD3D11&>(pixelBuffer);

    // Convert to the texture's DXGI layout (RGB -> RGBA padding happens here).
    const PackedTexels texels = PackTexels(buffer.Bytes(), width, height, format, datatype, rowAlignment,
                                           Description().textureFormat);

    // Row 0 of the data is row yOffset of the texture. Both APIs call that the
    // bottom row when sampling (v = 0), so no flip is needed.
    const D3D11_BOX box{
        static_cast<UINT>(xOffset), static_cast<UINT>(yOffset), 0,
        static_cast<UINT>(xOffset + width), static_cast<UINT>(yOffset + height), 1,
    };
    ID3D11DeviceContext* immediate = m_device.Immediate();
    immediate->UpdateSubresource(m_texture.Get(), 0, &box, texels.bytes.data(), texels.rowPitch, 0);

    if (Description().generateMipmaps) {
        immediate->GenerateMips(m_shaderResourceView.Get());   // rebuilds every level below 0
    }
}

std::shared_ptr<ReadPixelBuffer> Texture2DD3D11::DoCopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                                int rowAlignment) const {
    ID3D11DeviceContext* immediate = m_device.Immediate();

    // A CPU-readable copy of level 0.
    D3D11_TEXTURE2D_DESC desc{};
    m_texture->GetDesc(&desc);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> staging;
    ThrowIfFailed(m_device.Native()->CreateTexture2D(&desc, nullptr, &staging), "CreateTexture2D(staging)");

    // Whole subresource (a null box): depth-stencil resources can only be copied whole.
    immediate->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, m_texture.Get(), 0, nullptr);

    const Texture2DDescription& d = Description();
    MappedSubresource mapped(immediate, staging.Get(), 0, D3D11_MAP_READ);   // waits for the GPU
    std::vector<std::byte> pixels = UnpackTexels(mapped.Data(), mapped.RowPitch(), d.width, d.height,
                                                 d.textureFormat, format, datatype, rowAlignment);
    return std::make_shared<ReadPixelBufferD3D11>(PixelBufferHint::Stream, std::move(pixels));
}

} // namespace arda::renderer::d3d11
```

- **The staging texture copies the description,** then changes what a
  staging resource must have: no bind flags, CPU read access, one mip
  level, no misc flags. The format (typeless for depth) stays the same,
  which `CopySubresourceRegion` requires.
- **`mapped.RowPitch()`** is the distance between rows in the mapped
  memory. The driver may pad rows (to 256 bytes on many GPUs), so a 3-pixel
  RGBA8 row can be 256 bytes apart, not 12. Using `width * 4` instead is a
  classic bug that works for some sizes and garbles others.
- **`GenerateMips`** takes the shader resource view, which covers every
  level; it renders each level from the one above.
- **`static_cast<UINT>(-1)`** is the documented way to say "all mip levels"
  in a view description.

> **D3D11 note — sRGB:** `SRedGreenBlue8Alpha8` is `R8G8B8A8_UNORM_SRGB`.
> Sampling converts from sRGB to linear on both APIs. *Writing* differs: a
> D3D11 render target view with an `_SRGB` format always converts linear to
> sRGB on write, while GL only does so when `GL_FRAMEBUFFER_SRGB` is
> enabled, which arda never enables. Rendering into an sRGB framebuffer
> texture therefore gives different bytes on the two APIs.

### `src/d3d11/textures/TextureSamplerD3D11.h` and `.cpp`

```cpp
// src/d3d11/textures/TextureSamplerD3D11.h
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/textures/TextureSampler.h>

namespace arda::renderer::d3d11 {

class TextureSamplerD3D11 final : public TextureSampler {
public:
    TextureSamplerD3D11(ID3D11Device* device, const TextureSamplerDescription& description);

    ID3D11SamplerState* Native() const { return m_sampler.Get(); }

private:
    ComPtr<ID3D11SamplerState> m_sampler;
};

} // namespace arda::renderer::d3d11
```

```cpp
// src/d3d11/textures/TextureSamplerD3D11.cpp
#include "d3d11/textures/TextureSamplerD3D11.h"
#include "d3d11/TypeConverterD3D11.h"

#include <algorithm>

namespace arda::renderer::d3d11 {

TextureSamplerD3D11::TextureSamplerD3D11(ID3D11Device* device, const TextureSamplerDescription& description)
    : TextureSampler(description) {
    // Device::CreateTexture2DSampler has already rejected values below 1.
    const bool anisotropic = description.maximumAnisotropy > 1.0f;

    D3D11_SAMPLER_DESC desc{};
    desc.Filter = ToD3DFilter(description.minificationFilter, description.magnificationFilter, anisotropic);
    desc.AddressU = ToD3D(description.wrapS);
    desc.AddressV = ToD3D(description.wrapT);
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;   // unused for 2D textures
    desc.MipLODBias = 0.0f;
    // Clamped like the GL backend clamps to GL_MAX_TEXTURE_MAX_ANISOTROPY: CreateSamplerState rejects values above 16.
    desc.MaxAnisotropy = anisotropic
        ? static_cast<UINT>(std::min(description.maximumAnisotropy, static_cast<float>(D3D11_MAX_MAXANISOTROPY)))
        : 1u;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;   // not a comparison (shadow) sampler
    desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0.0f;
    desc.MinLOD = 0.0f;
    // GL's Nearest and Linear minification filters read only level 0. D3D11 has
    // no "no mipmaps" filter, so limit the level range instead.
    desc.MaxLOD = UsesMipmaps(description.minificationFilter) ? D3D11_FLOAT32_MAX : 0.0f;

    ThrowIfFailed(device->CreateSamplerState(&desc, &m_sampler), "CreateSamplerState");
}

} // namespace arda::renderer::d3d11
```

> **D3D11 note — filters:** GL names a minification filter
> `<within a level>_MIPMAP_<between levels>` and has a separate
> magnification filter. D3D11 encodes minification, magnification and mip
> filtering into one `D3D11_FILTER` value (`D3D11_ENCODE_BASIC_FILTER`
> builds it). The one GL concept with no D3D11 filter is "no mipmapping":
> `GL_LINEAR` as a minification filter ignores the mip levels. `MaxLOD = 0`
> gives the same result. Anisotropic filtering is core in D3D11 (up to
> 16×), so unlike the GL backend it never throws
> `InsufficientVideoCardException`.

### `ContextD3D11`: texture units

In `src/d3d11/ContextD3D11.cpp`, add

```cpp
#include "d3d11/textures/Texture2DD3D11.h"
#include "d3d11/textures/TextureSamplerD3D11.h"
```

and replace the temporary `CleanTextureUnits` from Step 3 with these two
functions:

```cpp
void ContextD3D11::CleanTextureUnits() {
    TextureUnits& units = GetTextureUnits();

    // The units that changed since the last draw, plus the ones this context
    // must bind again: all of them after InvalidateCachedState, and those
    // whose texture was a render target (Step 6).
    std::vector<int> indices = units.TakeDirtyUnits();   // throws if a unit has a texture but no sampler
    indices.insert(indices.end(), m_textureUnitsToRebind.begin(), m_textureUnitsToRebind.end());
    m_textureUnitsToRebind.clear();

    for (const int index : indices) {
        const TextureUnit& unit = units[index];

        ID3D11ShaderResourceView* view = nullptr;
        if (const std::shared_ptr<Texture2D>& texture = unit.GetTexture()) {
            const auto& textureD3D11 = static_cast<const Texture2DD3D11&>(*texture);
            if (IsRenderTarget(textureD3D11)) {
                // Reading a texture that is being rendered into is undefined in GL and
                // refused by D3D11. Leave the slot empty until the framebuffer changes.
                m_textureUnitsToRebind.push_back(index);
            } else {
                view = textureD3D11.ShaderResourceView();
            }
        }

        ID3D11SamplerState* sampler = unit.GetSampler()
            ? static_cast<const TextureSamplerD3D11&>(*unit.GetSampler()).Native()
            : nullptr;

        // Unit N is register tN and sN, in every stage, like a GL texture unit.
        const auto slot = static_cast<UINT>(index);
        Immediate()->VSSetShaderResources(slot, 1, &view);
        Immediate()->GSSetShaderResources(slot, 1, &view);
        Immediate()->PSSetShaderResources(slot, 1, &view);
        Immediate()->VSSetSamplers(slot, 1, &sampler);
        Immediate()->GSSetSamplers(slot, 1, &sampler);
        Immediate()->PSSetSamplers(slot, 1, &sampler);
        m_boundShaderResources[static_cast<std::size_t>(index)] = view;
    }
}

bool ContextD3D11::IsRenderTarget(const Texture2DD3D11& texture) const {
    return std::find(m_renderTargetTextures.begin(), m_renderTargetTextures.end(), &texture) !=
           m_renderTargetTextures.end();
}
```

`m_renderTargetTextures` stays empty until Step 6 fills it in
`ApplyFramebuffer`, so `IsRenderTarget` is always false for now.

- **`&view` is fine here:** `view` is a plain `ID3D11ShaderResourceView*`
  local, not a `ComPtr`, so `&view` is an ordinary pointer to it. The
  functions read an array of one.
- **`m_boundShaderResources`** records what each slot holds. Step 6 uses it
  to unbind a texture before it becomes a render target.

### `DeviceD3D11`: textures and samplers

In `src/d3d11/DeviceD3D11.cpp`, add

```cpp
#include "d3d11/buffers/PixelBuffersD3D11.h"
#include "d3d11/textures/Texture2DD3D11.h"
#include "d3d11/textures/TextureSamplerD3D11.h"
```

and replace the last three stubs:

```cpp
std::shared_ptr<WritePixelBuffer> DeviceD3D11::DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                                        std::size_t sizeInBytes) {
    return std::make_shared<WritePixelBufferD3D11>(usageHint, sizeInBytes);
}

std::shared_ptr<Texture2D> DeviceD3D11::DoCreateTexture2D(const Texture2DDescription& description) {
    return std::make_shared<Texture2DD3D11>(*this, description);
}

std::shared_ptr<TextureSampler> DeviceD3D11::DoCreateTexture2DSampler(const TextureSamplerDescription& description) {
    return std::make_shared<TextureSamplerD3D11>(m_device.Get(), description);
}
```

With no stubs left in `DeviceD3D11.cpp`, delete `NotImplementedYet` from its
anonymous namespace. `Device::Samplers()` now holds four real samplers.

### Milestone: the textured triangle

Step 5's milestone with the device created as Direct3D 11 and these
shaders. The mesh, the image and the texture unit code are unchanged.

```hlsl
// Vertex shader
float4x4 og_modelViewPerspectiveMatrix;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 textureCoordinate : TEXCOORD0;
};

VertexOutput main(float4 position : position0, float2 textureCoordinate : textureCoordinate3)
{
    VertexOutput output;
    output.position = mul(og_modelViewPerspectiveMatrix, position);
    output.textureCoordinate = textureCoordinate;
    return output;
}
```

```hlsl
// Pixel shader
Texture2D og_texture0 : register(t0);
SamplerState og_sampler0 : register(s0);

struct VertexOutput
{
    float4 position : SV_Position;
    float2 textureCoordinate : TEXCOORD0;
};

void main(VertexOutput input, out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(og_texture0.Sample(og_sampler0, input.textureCoordinate).rgb, 1.0);
}
```

- **The same struct in both stages.** D3D11 matches a pixel shader's inputs
  to the vertex shader's outputs by semantic *and* by register position, so
  declaring the identical struct in both is the simplest way to keep them
  compatible. `TEXCOORD0` is just a name here; any semantic works, as long
  as both stages use the same one.
- **`textureCoordinate3`:** the mesh attribute named `textureCoordinate`, at
  `VertexLocations::TextureCoordinate` (3).
- **The image has the same orientation as on GL**, with the same texture
  coordinates: the first uploaded row is v = 0 in both APIs.

Also try `TextureFormat::RedGreenBlue8` with a 3-channel image (the padding
path), and a 2×1 texture read back with
`CopyToBuffer(ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 1)`:
the bytes match what you uploaded.

---

## Step 6 on D3D11: framebuffers

This step mirrors [Step 6](06-framebuffers.md). Read 3.7.2 ("Framebuffers in
Direct3D") again first. `Framebuffer` and its attachment rules are
unchanged.

### D3D11 concepts for this step

> **D3D11 note — no framebuffer object:** GL's framebuffer object is a
> container you attach textures to and then bind. D3D11 has nothing to
> create: `OMSetRenderTargets(count, renderTargetViews, depthStencilView)`
> binds up to eight render target views and one depth-stencil view directly.
> So `FramebufferD3D11` is just the list of views to pass. There is also no
> `glDrawBuffers`: slot N of the array receives `SV_TargetN`, and a null
> entry does what `GL_NONE` does. There is no completeness check either;
> mistakes show up when a view is created, or as debug-layer messages at
> draw time.

> **D3D11 note — attachment sizes:** GL 3.3 lets a framebuffer's
> attachments have different sizes and renders to their intersection.
> D3D11's debug layer reports an error when the bound views' sizes differ,
> and drivers may misbehave. `FramebufferD3D11::Clean` throws instead, which
> makes the rule the same on every machine. Keep a framebuffer's attachments
> the same size if you want portable code.

> **D3D11 note — a resource can't be read and written at once (the
> feedback loop):** GL leaves it *undefined* to sample a texture that is
> attached to the bound framebuffer (3.7). D3D11 enforces a rule instead: a
> resource may not be bound as a shader input and as an output at the same
> time. If you bind it as a render target, the runtime silently unbinds it
> from every shader stage (with a debug-layer warning). If you bind a view
> as a shader resource while its texture is a render target, the runtime
> binds *null* instead. Either way, arda's cached copy of the bindings would
> then be wrong, and a texture that is later valid to read would stay
> unbound. So `ContextD3D11` handles both directions itself:
> - `ApplyFramebuffer` unbinds any shader resource view whose texture is
>   about to become a render target, and remembers to bind that unit again.
> - `CleanTextureUnits` (Step 5) binds null for a texture that is currently
>   a render target, and retries after the framebuffer changes.
>
> That is also why `ApplyBeforeDraw` applies the framebuffer before the
> texture units.

### `src/d3d11/framebuffer/FramebufferD3D11.h`

```cpp
#pragma once

#include "d3d11/D3D11Common.h"

#include <arda/renderer/framebuffer/Framebuffer.h>

#include <cstddef>
#include <span>
#include <vector>

namespace arda::renderer::d3d11 {

class Texture2DD3D11;

// FramebufferGL3x's twin. D3D11 has no framebuffer object, so this is only the
// views to pass to OMSetRenderTargets, rebuilt when an attachment changes.
class FramebufferD3D11 final : public Framebuffer {
public:
    explicit FramebufferD3D11(int maximumNumberOfColorAttachments)
        : Framebuffer(maximumNumberOfColorAttachments),
          m_renderTargetViews(static_cast<std::size_t>(maximumNumberOfColorAttachments), nullptr) {}

    // Applies changed attachments. Throws std::invalid_argument if the attachments
    // have different sizes, and InsufficientVideoCardException if a texture's
    // format can't be rendered to on this GPU.
    void Clean();

    // Slot N is color attachment N, up to the highest one that is set. Empty slots are null.
    std::span<ID3D11RenderTargetView* const> RenderTargetViews() const {
        return {m_renderTargetViews.data(), m_renderTargetCount};
    }

    ID3D11DepthStencilView* DepthStencilView() const { return m_depthStencilView; }   // null if none
    bool DepthStencilHasStencil() const { return m_depthStencilHasStencil; }

    // Every attached texture, for the feedback-loop checks in ContextD3D11.
    const std::vector<const Texture2DD3D11*>& AttachedTextures() const { return m_attachedTextures; }

private:
    // Raw pointers: each attached texture owns its views, and the base class's
    // attachments keep the textures alive.
    std::vector<ID3D11RenderTargetView*> m_renderTargetViews;
    std::size_t m_renderTargetCount = 0;
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
    bool m_depthStencilHasStencil = false;
    std::vector<const Texture2DD3D11*> m_attachedTextures;
};

} // namespace arda::renderer::d3d11
```

### `src/d3d11/framebuffer/FramebufferD3D11.cpp`

```cpp
#include "d3d11/framebuffer/FramebufferD3D11.h"
#include "d3d11/textures/Texture2DD3D11.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace arda::renderer::d3d11 {

void FramebufferD3D11::Clean() {
    if (!m_colorAttachmentsDirty && !m_depthAttachment.dirty && !m_depthStencilAttachment.dirty) {
        return;
    }

    // Rebuild everything into locals first, and commit only if nothing threw,
    // so a failed Clean leaves the previous (valid) state and the dirty flags.
    std::vector<ID3D11RenderTargetView*> views(m_renderTargetViews.size(), nullptr);
    std::size_t count = 0;
    std::vector<const Texture2DD3D11*> attached;
    std::optional<std::pair<int, int>> size;

    auto checkSize = [&size](const Texture2D& texture) {
        const std::pair<int, int> textureSize{texture.Description().width, texture.Description().height};
        if (!size) {
            size = textureSize;
        } else if (*size != textureSize) {
            throw std::invalid_argument(
                "Direct3D 11 requires every attachment of a framebuffer to have the same width and height");
        }
    };

    for (std::size_t i = 0; i < m_colorAttachments.size(); ++i) {
        if (const std::shared_ptr<Texture2D>& texture = m_colorAttachments[i].texture) {
            auto& textureD3D11 = static_cast<Texture2DD3D11&>(*texture);
            checkSize(textureD3D11);
            views[i] = textureD3D11.RenderTargetView();
            count = i + 1;
            attached.push_back(&textureD3D11);
        }
    }

    // Framebuffer.h: a depth/stencil attachment takes precedence over a depth attachment.
    ID3D11DepthStencilView* depthView = nullptr;
    bool hasStencil = false;
    const std::shared_ptr<Texture2D>& depth =
        m_depthStencilAttachment.texture ? m_depthStencilAttachment.texture : m_depthAttachment.texture;
    if (depth) {
        auto& textureD3D11 = static_cast<Texture2DD3D11&>(*depth);
        checkSize(textureD3D11);
        depthView = textureD3D11.DepthStencilView();
        hasStencil = textureD3D11.HasStencil();
        attached.push_back(&textureD3D11);
    }

    // Commit.
    m_renderTargetViews = std::move(views);
    m_renderTargetCount = count;
    m_depthStencilView = depthView;
    m_depthStencilHasStencil = hasStencil;
    m_attachedTextures = std::move(attached);
    for (Attachment& attachment : m_colorAttachments) {
        attachment.dirty = false;
    }
    m_colorAttachmentsDirty = false;
    m_depthAttachment.dirty = false;
    m_depthStencilAttachment.dirty = false;
}

} // namespace arda::renderer::d3d11
```

- **Rebuild, don't patch.** `FramebufferGL3x::Clean` attaches only the dirty
  attachments, because each GL attach call is a change to a GL object.
  Here, the "object" is nine pointers, so rebuilding them all is simpler
  and cheaper than tracking which changed.
- **Commit at the end** is the *strong exception guarantee*: if
  `RenderTargetView()` throws for an unrenderable format, the framebuffer
  is exactly as it was, still dirty, and the error repeats at the next use
  instead of leaving half-applied state behind.
- **`std::optional<std::pair<int, int>>`** holds "no size seen yet" or the
  first attachment's size ([Step 2](02-shaders.md#cpp-optional)).

### `ContextD3D11`: framebuffers

In `src/d3d11/ContextD3D11.cpp`, add

```cpp
#include "d3d11/framebuffer/FramebufferD3D11.h"

#include <span>
```

replace the `DoCreateFramebuffer` stub, replace the Step 1 version of
`ApplyFramebuffer` with the final one, and add
`UnbindShaderResourcesUsedAsRenderTargets`:

```cpp
std::shared_ptr<Framebuffer> ContextD3D11::DoCreateFramebuffer() {
    return std::make_shared<FramebufferD3D11>(GetDevice().Limits().maximumNumberOfColorAttachments);
}

void ContextD3D11::ApplyFramebuffer() {
    RenderTargetBinding binding;

    if (const std::shared_ptr<Framebuffer>& framebuffer = GetFramebuffer()) {
        auto& framebufferD3D11 = static_cast<FramebufferD3D11&>(*framebuffer);
        framebufferD3D11.Clean();   // creates views for new attachments

        const std::span<ID3D11RenderTargetView* const> views = framebufferD3D11.RenderTargetViews();
        std::copy(views.begin(), views.end(), binding.views.begin());
        binding.count = static_cast<UINT>(views.size());
        binding.depthStencil = framebufferD3D11.DepthStencilView();
        m_depthStencilHasStencil = framebufferD3D11.DepthStencilHasStencil();
        m_renderTargetTextures = framebufferD3D11.AttachedTextures();
    } else {
        binding.views[0] = m_window.BackBufferView();
        binding.count = 1;
        binding.depthStencil = m_window.DepthStencilView();
        m_depthStencilHasStencil = true;   // the window's D24_UNORM_S8_UINT
        m_renderTargetTextures.clear();
    }

    if (m_renderTargets != binding) {
        // A texture about to be written must not stay bound for reading.
        UnbindShaderResourcesUsedAsRenderTargets();
        Immediate()->OMSetRenderTargets(binding.count, binding.views.data(), binding.depthStencil);
        m_renderTargets = binding;
    }
}

void ContextD3D11::UnbindShaderResourcesUsedAsRenderTargets() {
    for (std::size_t slot = 0; slot < m_boundShaderResources.size(); ++slot) {
        ID3D11ShaderResourceView* bound = m_boundShaderResources[slot];
        if (bound == nullptr) {
            continue;
        }
        const bool becomesRenderTarget =
            std::any_of(m_renderTargetTextures.begin(), m_renderTargetTextures.end(),
                        [bound](const Texture2DD3D11* texture) { return texture->ShaderResourceView() == bound; });
        if (!becomesRenderTarget) {
            continue;
        }

        ID3D11ShaderResourceView* nullView = nullptr;
        const auto index = static_cast<UINT>(slot);
        Immediate()->VSSetShaderResources(index, 1, &nullView);
        Immediate()->GSSetShaderResources(index, 1, &nullView);
        Immediate()->PSSetShaderResources(index, 1, &nullView);
        m_boundShaderResources[slot] = nullptr;
        m_textureUnitsToRebind.push_back(static_cast<int>(slot));   // bind it again once it's safe
    }
}
```

- **The binding is compared as a whole.** `RenderTargetBinding`'s defaulted
  `operator==` compares all eight view pointers, the count and the depth
  view, so switching between a framebuffer and the window, or changing an
  attachment, triggers exactly one `OMSetRenderTargets`.
- **Comparing view pointers is safe** because the immediate context holds a
  reference to every bound view: a bound view can't be destroyed, so its
  address can't be reused by a new view while the comparison matters.
- **`std::any_of` with a lambda** asks "does any attached texture own this
  view?". The lambda captures `bound` by value; see
  [lambda captures](01-state-management.md#cpp-lambda-capture).

> **Why the Step 6 milestone clears unit 0 before pass 1 anyway:** the
> milestone's `unit0.SetTexture(nullptr)` before rendering into
> `colorTexture` is what portable code should do: on GL, sampling the
> texture you are rendering into is undefined. On D3D11 the context would
> also cope without it, but the program would then behave differently on
> the two backends, which is what the whole abstraction exists to prevent.

### Milestone: render to a texture, then show it

Step 6's milestone with the device created as Direct3D 11. The triangle
program is Step 4 on D3D11's (camera vertex shader plus the Step 3 pixel
shader with its named `fragmentColor` output, so
`FragmentOutputLocation("fragmentColor")` finds location 0). The fullscreen
quad:

```hlsl
// Vertex shader
struct VertexOutput
{
    float4 position : SV_Position;
    float2 textureCoordinate : TEXCOORD0;
};

VertexOutput main(float2 position : position0)
{
    VertexOutput output;
    output.textureCoordinate = position * 0.5 + 0.5;   // [-1, 1] -> [0, 1]
    output.position = float4(position, 0.0, 1.0);
    return output;
}
```

```hlsl
// Pixel shader
Texture2D og_texture0 : register(t0);
SamplerState og_sampler0 : register(s0);

struct VertexOutput
{
    float4 position : SV_Position;
    float2 textureCoordinate : TEXCOORD0;
};

void main(VertexOutput input, out float4 fragmentColor : SV_Target0)
{
    fragmentColor = float4(og_texture0.Sample(og_sampler0, input.textureCoordinate).rgb, 1.0);
}
```

What to check:

- **The triangle on the quad is the right way up**, exactly as on GL. The
  quad's bottom edge (clip y = -1) samples v = 0, which is row 0 of the
  texture, and the render-target flip put the bottom of the triangle's
  image in row 0.
- **`render-to-texture.png` is the right way up.** `Texture2D::Save` reads
  the texture back through `DoCopyToBuffer`; the flip made the rows match GL.
- **Culling works in both passes.** Enable facet culling for the triangle
  (`triangleState.renderState.facetCulling.enabled = true`): it still
  appears in pass 1, because `MakeRasterizerKey` reversed the front face
  while the framebuffer was bound. Without that reversal, the triangle
  would be culled in the texture but not on the window.
- **Comment out the flip** (make `FlipsRenderTargetY` return `false`): the
  triangle appears upside down on the quad. That one change is the whole
  difference between the APIs.
- **The debug output stays clean.** A warning like
  `Resource being set to OM RenderTarget slot 0 is still bound on input!`
  means a texture was still bound for reading when it became a render
  target, the case `UnbindShaderResourcesUsedAsRenderTargets` handles.

---

## Step 7 on D3D11: `Chapter03Triangle`, unchanged

[Step 7](07-triangle-example.md) already contains the HLSL in
`TriangleShaders` and the command-line switch. Run

```
arda_scene d3d11
```

and the same red triangle appears, in the same place, the same way up, and
it keeps its shape when the window is resized or minimized. Nothing in
`main.cpp` changes; that is the milestone for this whole step. The only
lines that know which API is running are the ones that choose the shader
source, which is the "false sense of portability" note in 3.1 made
concrete: the renderer abstracts the API calls, not the shading language.

---

## Tests

The earlier test files create `GraphicsApi::OpenGL33` devices. This step
adds a small helper so any test can run on every compiled backend, and a
test file that renders the same scenes on each backend and compares pixels.

### `tests/src/renderer/TestApis.h`

```cpp
#pragma once

#include <arda/renderer/Device.h>

#include <vector>

namespace arda::renderer::test {

// Every backend compiled into this build, in a fixed order.
inline std::vector<GraphicsApi> AvailableApis() {
    std::vector<GraphicsApi> apis;
    for (const GraphicsApi api : {GraphicsApi::OpenGL33, GraphicsApi::Direct3D11}) {
        if (IsGraphicsApiAvailable(api)) {
            apis.push_back(api);
        }
    }
    return apis;
}

inline const char* Name(GraphicsApi api) {
    return api == GraphicsApi::OpenGL33 ? "OpenGL 3.3" : "Direct3D 11";
}

} // namespace arda::renderer::test
```

To run an existing API-agnostic test on both backends, wrap its body in a
loop. `INFO` adds the API's name to any failure message from inside the
loop:

```cpp
#include "TestApis.h"

TEST_CASE("VertexBuffer copies to and from system memory") {
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        auto device = CreateDevice(api);
        // ...the original test body...
    }
}
```

The vertex data, texture, framebuffer and automatic uniform tests from
Steps 3–6 don't depend on the shading language and can be looped as they
are. The shader tests need HLSL sources; `HlslShaderTests` (Step 2 on
D3D11) covers them. `#include "TestApis.h"` with quotes finds the header
next to the test file, with no extra include directory.

### `tests/src/renderer/CrossApiTests.cpp`

Each test renders into a small framebuffer texture and reads the pixels
back. Reading back from a framebuffer, rather than from the window, is what
makes the tests exact: both APIs produce the same rows in the same order.
They catch most portability bugs: a missing Y flip, a missing winding
reversal, the wrong viewport conversion, the wrong clip depth range, the
wrong matrix packing, and constant buffer offsets.

```cpp
#include "TestApis.h"

#include <doctest/doctest.h>

#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/Texture2D.h>

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>

using namespace arda::renderer;
using namespace arda::core::geometry;
using arda::core::Vector2;
using arda::core::Vector3;

namespace {

constexpr int kSize = 32;

struct ShaderSources {
    const char* vertex;
    const char* fragment;
};

ShaderSources Pick(GraphicsApi api, ShaderSources glsl, ShaderSources hlsl) {
    return api == GraphicsApi::OpenGL33 ? glsl : hlsl;
}

// Step 7's triangle, with a vec4 output so the render target's alpha is defined.
ShaderSources TriangleShaders(GraphicsApi api) {
    return Pick(api,
        {R"(#version 330
            layout(location = og_positionVertexLocation) in vec4 position;
            uniform mat4 og_modelViewPerspectiveMatrix;
            void main() { gl_Position = og_modelViewPerspectiveMatrix * position; })",
         R"(#version 330
            out vec4 fragmentColor;
            uniform vec3 u_color;
            void main() { fragmentColor = vec4(u_color, 1.0); })"},
        {R"(float4x4 og_modelViewPerspectiveMatrix;
            float4 main(float4 position : position0) : SV_Position
            {
                return mul(og_modelViewPerspectiveMatrix, position);
            })",
         R"(float3 u_color;
            void main(out float4 fragmentColor : SV_Target0) { fragmentColor = float4(u_color, 1.0); })"});
}

constexpr const char* kQuadVertexGlsl = R"(#version 330
    layout(location = og_positionVertexLocation) in vec2 position;
    void main() { gl_Position = vec4(position, 0.0, 1.0); })";

constexpr const char* kQuadVertexHlsl = R"(
    float4 main(float2 position : position0) : SV_Position { return float4(position, 0.0, 1.0); })";

// A clip-space quad in one flat color.
ShaderSources ColorQuadShaders(GraphicsApi api) {
    return Pick(api,
        {kQuadVertexGlsl,
         R"(#version 330
            out vec4 fragmentColor;
            uniform vec3 u_color;
            void main() { fragmentColor = vec4(u_color, 1.0); })"},
        {kQuadVertexHlsl,
         R"(float3 u_color;
            void main(out float4 fragmentColor : SV_Target0) { fragmentColor = float4(u_color, 1.0); })"});
}

// Three uniforms whose HLSL offsets exercise the packing rules:
// u_offset at 0, u_color at 16 (8 + 12 would cross a register), u_scale at 28.
ShaderSources PackingShaders(GraphicsApi api) {
    return Pick(api,
        {kQuadVertexGlsl,
         R"(#version 330
            uniform vec2 u_offset;
            uniform vec3 u_color;
            uniform float u_scale;
            out vec4 fragmentColor;
            void main() { fragmentColor = vec4(u_color * u_scale + vec3(u_offset, 0.0), 1.0); })"},
        {kQuadVertexHlsl,
         R"(float2 u_offset;
            float3 u_color;
            float u_scale;
            void main(out float4 fragmentColor : SV_Target0)
            {
                fragmentColor = float4(u_color * u_scale + float3(u_offset, 0.0), 1.0);
            })"});
}

// Step 7's mesh: a right triangle in the xz plane, legs of length 1, counterclockwise from the camera.
Mesh TriangleMesh() {
    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(0.0f, 0.0f, 0.0f);
    positions.emplace_back(1.0f, 0.0f, 0.0f);
    positions.emplace_back(0.0f, 0.0f, 1.0f);
    auto indices = std::make_unique<IndicesUnsignedShort>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);
    return mesh;
}

// Covers the whole viewport, counterclockwise.
Mesh FullscreenQuadMesh() {
    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector2>("position", 4).Values();
    positions.emplace_back(-1.0f, -1.0f);
    positions.emplace_back(1.0f, -1.0f);
    positions.emplace_back(1.0f, 1.0f);
    positions.emplace_back(-1.0f, 1.0f);
    auto indices = std::make_unique<IndicesUnsignedShort>(6);
    indices->AddTriangle(0, 1, 2);
    indices->AddTriangle(0, 2, 3);
    mesh.indices = std::move(indices);
    return mesh;
}

struct Scene {
    ShaderSources shaders;
    std::function<Mesh()> createMesh;
    std::function<void(ShaderProgram&)> setUniforms;
    Rectangle viewport{0, 0, kSize, kSize};
};

// Renders the scene into a kSize x kSize RGBA8 framebuffer texture, with back-face
// culling on, and returns its pixels: 4 bytes each, bottom row first.
std::vector<std::uint8_t> Render(GraphicsApi api, const Scene& scene) {
    // Declared in this order so resources are destroyed before the window, and the window before the device.
    auto device = CreateDevice(api);
    auto window = device->CreateGraphicsWindow(kSize, kSize, "cross-API test", WindowType::Hidden);
    Context& context = window->GetContext();

    auto program = device->CreateShaderProgram(scene.shaders.vertex, scene.shaders.fragment);
    scene.setUniforms(*program);

    DrawState drawState;
    drawState.renderState.depthTest.enabled = false;   // no depth attachment
    drawState.renderState.facetCulling.enabled = true; // tests the winding reversal on D3D11
    drawState.shaderProgram = program;
    drawState.vertexArray = context.CreateVertexArray(scene.createMesh(), program->VertexAttributes(),
                                                      BufferHint::StaticDraw);

    auto color = device->CreateTexture2D(Texture2DDescription{kSize, kSize, TextureFormat::RedGreenBlueAlpha8, false});
    auto framebuffer = context.CreateFramebuffer();
    framebuffer->SetColorAttachment(0, color);

    SceneState sceneState;
    sceneState.camera.ZoomToTarget(1.0);   // the default aspect ratio, 1, matches the square target

    ClearState clearState;
    clearState.color = {0.0f, 0.0f, 1.0f, 1.0f};   // blue background

    context.SetFramebuffer(framebuffer);
    context.SetViewport(scene.viewport);
    context.Clear(clearState);
    context.Draw(PrimitiveType::Triangles, drawState, sceneState);
    context.SetFramebuffer(nullptr);

    return color->CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1)
        ->CopyToSystemMemory<std::uint8_t>();
}

struct Rgba {
    int red, green, blue, alpha;
};

// (x, y) with (0, 0) at the bottom-left, like Rectangle.
Rgba Pixel(const std::vector<std::uint8_t>& pixels, int x, int y) {
    const std::size_t i = (static_cast<std::size_t>(y) * kSize + static_cast<std::size_t>(x)) * 4;
    return {pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
}

bool IsColor(Rgba pixel, int red, int green, int blue, int tolerance = 1) {
    return std::abs(pixel.red - red) <= tolerance && std::abs(pixel.green - green) <= tolerance &&
           std::abs(pixel.blue - blue) <= tolerance && pixel.alpha == 255;
}

Scene TriangleScene(GraphicsApi api) {
    return Scene{
        TriangleShaders(api),
        TriangleMesh,
        [](ShaderProgram& program) {
            program.Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));
        },
    };
}

} // namespace

TEST_CASE("Each backend renders the triangle the right way up") {
    // The right angle is at the target, the center of the image. The legs run
    // right (+x) and up (+z), about 15.5 pixels long at this zoom.
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        const auto pixels = Render(api, TriangleScene(api));
        REQUIRE(pixels.size() == static_cast<std::size_t>(kSize * kSize * 4));

        CHECK(IsColor(Pixel(pixels, 20, 20), 255, 0, 0));   // inside, up and to the right of the corner
        CHECK(IsColor(Pixel(pixels, 10, 10), 0, 0, 255));   // below and left of the corner
        CHECK(IsColor(Pixel(pixels, 22, 10), 0, 0, 255));   // below the horizontal leg: catches a Y flip
        CHECK(IsColor(Pixel(pixels, 10, 22), 0, 0, 255));   // left of the vertical leg: catches an X flip
        CHECK(IsColor(Pixel(pixels, 26, 26), 0, 0, 255));   // beyond the hypotenuse
    }
}

TEST_CASE("OpenGL and Direct3D 11 render the same image") {
    if (!IsGraphicsApiAvailable(GraphicsApi::OpenGL33) || !IsGraphicsApiAvailable(GraphicsApi::Direct3D11)) {
        MESSAGE("Both backends are needed; skipping");
        return;
    }

    const auto gl = Render(GraphicsApi::OpenGL33, TriangleScene(GraphicsApi::OpenGL33));
    const auto d3d = Render(GraphicsApi::Direct3D11, TriangleScene(GraphicsApi::Direct3D11));
    REQUIRE(gl.size() == d3d.size());

    int differentPixels = 0;
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const Rgba a = Pixel(gl, x, y);
            const Rgba b = Pixel(d3d, x, y);
            if (!IsColor(b, a.red, a.green, a.blue, 2)) {
                ++differentPixels;
            }
        }
    }
    // Both APIs follow the same top-left fill rule, so the images should match
    // exactly. Allow a few pixels on the hypotenuse, where a sample point can
    // sit exactly on the edge and rounding differs between drivers.
    CHECK(differentPixels <= 4);
}

TEST_CASE("Uniforms reach the shader at the offsets reflection reports") {
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        const Scene scene{
            PackingShaders(api),
            FullscreenQuadMesh,
            [](ShaderProgram& program) {
                program.Uniforms().Get<Vector2<float>>("u_offset").SetValue(Vector2<float>(0.0f, 0.25f));
                program.Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.5f));
                program.Uniforms().Get<float>("u_scale").SetValue(0.5f);
            },
        };
        const auto pixels = Render(api, scene);
        // (1, 0, 0.5) * 0.5 + (0, 0.25, 0) = (0.5, 0.25, 0.25)
        CHECK(IsColor(Pixel(pixels, 16, 16), 128, 64, 64));
    }
}

TEST_CASE("A viewport inside a framebuffer covers the same pixels on every backend") {
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        Scene scene{
            ColorQuadShaders(api),
            FullscreenQuadMesh,
            [](ShaderProgram& program) {
                program.Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));
            },
        };
        scene.viewport = Rectangle{0, 0, kSize / 2, kSize / 2};   // the bottom-left quadrant
        const auto pixels = Render(api, scene);

        CHECK(IsColor(Pixel(pixels, 4, 4), 255, 0, 0));     // bottom-left: the quad
        CHECK(IsColor(Pixel(pixels, 4, 28), 0, 0, 255));    // top-left: background
        CHECK(IsColor(Pixel(pixels, 28, 4), 0, 0, 255));    // bottom-right: background
        CHECK(IsColor(Pixel(pixels, 28, 28), 0, 0, 255));   // top-right: background
    }
}

TEST_CASE("RGB8 textures round-trip through RGBA storage") {
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        auto device = CreateDevice(api);

        Image image;
        image.width = 2;
        image.height = 1;
        image.channels = 3;
        image.pixels = {255, 0, 0, 0, 128, 255};

        auto texture = device->CreateTexture2D(image, TextureFormat::RedGreenBlue8, false);
        CHECK(texture->CopyToBuffer(ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 1)
                  ->CopyToSystemMemory<std::uint8_t>() == image.pixels);
        CHECK(texture->CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1)
                  ->CopyToSystemMemory<std::uint8_t>() == std::vector<std::uint8_t>{255, 0, 0, 255, 0, 128, 255, 255});
    }
}

TEST_CASE("Depth textures read back as floats") {
    for (const GraphicsApi api : test::AvailableApis()) {
        INFO("API: " << test::Name(api));
        auto device = CreateDevice(api);
        auto window = device->CreateGraphicsWindow(16, 16, "test", WindowType::Hidden);
        Context& context = window->GetContext();

        auto depth = device->CreateTexture2D(Texture2DDescription{4, 4, TextureFormat::Depth32f, false});
        auto framebuffer = context.CreateFramebuffer();
        framebuffer->SetDepthAttachment(depth);

        ClearState clearState;
        clearState.buffers = ClearBuffers::DepthBuffer;
        clearState.depth = 0.25f;
        context.SetFramebuffer(framebuffer);
        context.Clear(clearState);
        context.SetFramebuffer(nullptr);

        const auto values = depth->CopyToBuffer(ImageFormat::DepthComponent, ImageDatatype::Float, 4)
                                ->CopyToSystemMemory<float>();
        REQUIRE(values.size() == 16);
        CHECK(values[0] == doctest::Approx(0.25f));
        CHECK(values[15] == doctest::Approx(0.25f));
    }
}
```

- **`Render` builds everything from scratch** for each call: a device, a
  hidden window and a framebuffer. That is slow (tens of milliseconds) but
  keeps every test independent, and it also exercises creating and
  destroying a whole device per backend, which catches shutdown-order bugs.
- **`std::function<Mesh()>`** holds the mesh factory. `Mesh` is move-only
  (it owns `unique_ptr`s), so the scene stores *how to make* a mesh rather
  than a mesh, and `Render` makes a fresh one
  ([Step 0](00-setup.md#cpp-std-function-lambdas)).
- **The pixel positions** come from the camera: `ZoomToTarget(1)` places the
  eye so a unit sphere just fits in the 30° field of view, so one unit is
  about 15.5 pixels in a 32-pixel image, and the triangle's corner is at the
  center.
- **A failure names the backend** thanks to `INFO`. If only Direct3D 11
  fails the "right way up" test at pixel (20, 20) while (20, 11) would be
  red, the Y flip is missing; if culling is on and nothing is red, the
  winding reversal is missing; if the viewport test finds red at (4, 28),
  `ToTopLeftY` is converting while a framebuffer is bound.

These tests need a GPU (or WARP) and a desktop session, like the other
renderer tests.

---

## Common pitfalls

| Symptom | Likely cause |
|---|---|
| `std::min` / `numeric_limits::max()` fails to compile in a D3D11 file | `NOMINMAX` isn't defined before `<windows.h>`. Check the CMake definitions. |
| `DXGI_ERROR_SDK_COMPONENT_MISSING` in a debug build, or no debug messages | The Graphics Tools optional feature isn't installed. The device falls back to no debug layer. |
| The triangle shows for one frame, then only the clear color | `SwapBuffers` doesn't call `ForgetRenderTargets`: `Present` unbound the back buffer, and the cache still thinks it's bound. |
| `ResizeBuffers` fails with `DXGI_ERROR_INVALID_CALL` | A reference to the back buffer survived: a view not reset, a binding not cleared (`ClearState`), or no `Flush` after releasing. |
| A view or buffer is released at a strange time; crashes in `OMSetRenderTargets` | `&comPtr` passed to an *input* parameter. Use `GetAddressOf()`. |
| The triangle is missing or distorted | `mul(v, M)` instead of `mul(M, v)`, or a `row_major` matrix (arda rejects those at creation). |
| A uniform always reads 0 | It's declared `const` without `static` (a uniform no one sets), it has an initializer (ignored for globals), or the compiler removed it because it doesn't affect the output. |
| Values from a hand-written `cbuffer` struct are shifted | HLSL packing: a `float3` after a `float2`, or any value that would cross a 16-byte register, starts a new register. Use `alignas(16)` members and `static_assert` offsets. |
| `CreateInputLayout` fails | The vertex array has no attribute at a location the shader reads, or the format doesn't fit the input type (a non-normalized integer attribute needs an `int`/`uint` input). |
| `ToDxgiFormat` throws for a mesh | 3-component 8- or 16-bit attributes (`VertexAttributeRGB`, `HalfFloatVector3`) have no DXGI format. Use 4 components. |
| `CreateBlendState` fails with `E_INVALIDARG` | An alpha blend factor uses a `*_COLOR` value. `ToD3DAlpha` converts them; check it's used for `SrcBlendAlpha`/`DestBlendAlpha`. |
| Stencil counts wrap instead of saturating (or the reverse) | `GL_INCR` is `D3D11_STENCIL_OP_INCR_SAT`; `GL_INCR_WRAP` is `D3D11_STENCIL_OP_INCR`. |
| Render-to-texture output is upside down | `FlipsRenderTargetY` returns false, or a shader writes clip coordinates without a clip-space matrix and without `og_clipSpaceFlipY`. |
| The triangle is culled in the texture but not on the window | The rasterizer key doesn't reverse the front face while a framebuffer is bound. |
| A sub-viewport lands in the wrong half of a framebuffer | `ToTopLeftY` converts to top-left while a framebuffer is bound; it must return `bottom` unchanged there. |
| A texture samples as black after it was rendered into | It was still bound as a render target (the runtime bound null), or the unbinding in `ApplyFramebuffer` didn't queue it for rebinding. |
| Garbled rows when reading back a texture | Rows were copied `width * bytesPerTexel` apart instead of `mapped.RowPitch()` apart. |
| Objects very close to the camera disappear | A projection built with `ClipDepth::NegativeOneToOne` is used on D3D11. |
| `LineLoop`/`TriangleFan` throws | D3D11 has neither. Use `LineStrip` with the first index repeated, or `Triangles`. |

---

## Checklist

- [ ] **Before you start:** `Context::ClipSpaceTransform` and `FlipsRenderTargetY`; the automatic uniforms apply the transform; `og_clipSpaceFlipY`; GL tests still pass
- [ ] CMake: the `ARDA_RENDERER_D3D11` block (sources, `d3d11 dxgi d3dcompiler dxguid`, `NOMINMAX`, `WIN32_LEAN_AND_MEAN`); `CreateDevice(Direct3D11)`
- [ ] `D3D11Common.h`: `ComPtr`, `ThrowIfFailed`/`D3D11Error`, `Utf8ToWide`/`WideToUtf8`, `SetDebugName`, `MappedSubresource`
- [ ] **Step 0:** `DeviceD3D11` (debug layer, WARP fallback, limits, `ReleaseCommon`), `GraphicsWindowD3D11` (flip-model swap chain, window depth buffer, resize in `PollEvents`, `Present`), `ContextD3D11` header and starter; the device test passes
- [ ] **Step 1:** `TypeConverterD3D11` (every case), `StateCacheD3D11` (keys, `std::hash` specializations, three caches), `DoClear`, `ApplyRenderState`, `ApplyScissorTest`, `ApplyViewport`, `ToTopLeftY`; the clear color shows
- [ ] **Step 2:** `HlslPrelude`, `ConstantBufferD3D11`, `UniformD3D11<T>` and `WriteUniform`, `ShaderProgramD3D11` (compile, reflect, link check, `FragmentOutputLocation`); `HlslShaderTests` pass
- [ ] **Step 3:** `BufferD3D11`, `VertexBufferD3D11`, `IndexBufferD3D11`, `VertexArrayD3D11` (input layout cache), `DoDraw`, `DoDrawRange`, `ApplyBeforeDraw`; Milestones 3A and 3B on D3D11
- [ ] **Step 4:** the triangle through the camera on D3D11; you can explain why the flip happens in clip space
- [ ] **Step 5:** pixel buffers, `PixelConversionD3D11`, `Texture2DD3D11` (formats, padding, mipmaps, staging readback), `TextureSamplerD3D11`, `CleanTextureUnits`; the textured triangle on D3D11
- [ ] **Step 6:** `FramebufferD3D11`, the final `ApplyFramebuffer`, `UnbindShaderResourcesUsedAsRenderTargets`; render-to-texture on D3D11 is the right way up, with culling on
- [ ] **Step 7:** `arda_scene d3d11` shows the same triangle as `arda_scene`
- [ ] No `NotImplementedYet` stub is left; `TestApis.h`, `CrossApiTests` pass; the debug output is clean for every milestone

---

## Appendix: the finished files that were built in pieces

`DeviceD3D11` and `ContextD3D11` grew over several sections. These are their
final versions, for checking your assembled files against.

### `src/d3d11/DeviceD3D11.h` (final)

```cpp
#pragma once

#include "GlfwLibrary.h"
#include "d3d11/D3D11Common.h"
#include "d3d11/StateCacheD3D11.h"

#include <arda/renderer/Device.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace arda::renderer::d3d11 {

class ContextD3D11;

class DeviceD3D11 final : public Device {
public:
    DeviceD3D11();
    ~DeviceD3D11() override;

    GraphicsApi Api() const override { return GraphicsApi::Direct3D11; }

    ID3D11Device* Native() const { return m_device.Get(); }
    ID3D11DeviceContext* Immediate() const { return m_immediate.Get(); }
    StateCacheD3D11& StateCache() { return *m_stateCache; }

    // The GPU's name, for example for a log line. "Microsoft Basic Render Driver" means WARP.
    const std::string& AdapterDescription() const { return m_adapterDescription; }

    // The ContextD3D11 whose state the shared immediate context holds.
    // nullptr means "unknown": every context must re-apply its state.
    ContextD3D11* ActiveContext() const { return m_activeContext; }
    void SetActiveContext(ContextD3D11* context) { m_activeContext = context; }

protected:
    std::unique_ptr<GraphicsWindow> DoCreateGraphicsWindow(
        int width, int height, const std::string& title, WindowType type) override;

    std::shared_ptr<ShaderProgram> DoCreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view geometryShaderSource,
        std::string_view fragmentShaderSource) override;

    std::shared_ptr<VertexBuffer> DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;
    std::shared_ptr<IndexBuffer> DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;

    std::shared_ptr<WritePixelBuffer> DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                               std::size_t sizeInBytes) override;
    std::shared_ptr<Texture2D> DoCreateTexture2D(const Texture2DDescription& description) override;
    std::shared_ptr<TextureSampler> DoCreateTexture2DSampler(const TextureSamplerDescription& description) override;

private:
    GlfwLibrary m_glfw;   // GLFW still creates the windows; declared first so it outlives them
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_immediate;
    std::unique_ptr<StateCacheD3D11> m_stateCache;   // created once m_device exists
    std::string m_adapterDescription;
    ContextD3D11* m_activeContext = nullptr;
};

} // namespace arda::renderer::d3d11
```

### `src/d3d11/DeviceD3D11.cpp` (final)

```cpp
#include "d3d11/DeviceD3D11.h"
#include "d3d11/GraphicsWindowD3D11.h"
#include "d3d11/buffers/IndexBufferD3D11.h"
#include "d3d11/buffers/PixelBuffersD3D11.h"
#include "d3d11/buffers/VertexBufferD3D11.h"
#include "d3d11/shaders/ShaderProgramD3D11.h"
#include "d3d11/textures/Texture2DD3D11.h"
#include "d3d11/textures/TextureSamplerD3D11.h"

#include <iterator>

namespace arda::renderer::d3d11 {

namespace {

// One attempt at creating the device and its immediate context.
HRESULT CreateDeviceAndContext(D3D_DRIVER_TYPE driverType, UINT flags,
                               ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& immediate) {
    const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL obtained{};
    return D3D11CreateDevice(
        nullptr,        // the default adapter (the GPU driving the primary monitor)
        driverType,
        nullptr,        // no software rasterizer DLL
        flags,
        featureLevels, static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(), &obtained, immediate.ReleaseAndGetAddressOf());
}

} // namespace

DeviceD3D11::DeviceD3D11() {
    UINT flags = 0;
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_HARDWARE, flags, m_device, m_immediate);
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
        // The debug layer isn't installed. Run without it rather than not at all.
        flags &= ~static_cast<UINT>(D3D11_CREATE_DEVICE_DEBUG);
        hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_HARDWARE, flags, m_device, m_immediate);
    }
    if (FAILED(hr)) {
        // No GPU driver could do feature level 11_0: fall back to the software rasterizer.
        hr = CreateDeviceAndContext(D3D_DRIVER_TYPE_WARP, flags, m_device, m_immediate);
    }
    ThrowIfFailed(hr, "D3D11CreateDevice");

    m_stateCache = std::make_unique<StateCacheD3D11>(m_device.Get());

#ifndef NDEBUG
    // Stop in the debugger at the first debug-layer error, at the call that caused it.
    ComPtr<ID3D11InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue)) && IsDebuggerPresent()) {
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif

    // Device -> DXGI device -> adapter, to read the GPU's name.
    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(m_device.As(&dxgiDevice), "QueryInterface(IDXGIDevice)");
    ComPtr<IDXGIAdapter> adapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(&adapter), "IDXGIDevice::GetAdapter");
    DXGI_ADAPTER_DESC adapterDesc{};
    ThrowIfFailed(adapter->GetDesc(&adapterDesc), "IDXGIAdapter::GetDesc");
    m_adapterDescription = WideToUtf8(adapterDesc.Description);

    SetLimits(DeviceLimits{
        .maximumNumberOfVertexAttributes = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,   // 32
        .numberOfTextureUnits = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,                // 16
        .maximumNumberOfColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,    // 8
    });

    // Automatic uniforms and the four common samplers (Steps 4 and 5).
    InitializeCommon();
}

DeviceD3D11::~DeviceD3D11() {
    ReleaseCommon();
    if (m_immediate) {
        m_immediate->ClearState();
        m_immediate->Flush();
    }
}

std::unique_ptr<GraphicsWindow> DeviceD3D11::DoCreateGraphicsWindow(
    int width, int height, const std::string& title, WindowType type) {
    return std::make_unique<GraphicsWindowD3D11>(*this, width, height, title, type);
}

std::shared_ptr<ShaderProgram> DeviceD3D11::DoCreateShaderProgram(
    std::string_view vertexShaderSource, std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource) {
    return std::make_shared<ShaderProgramD3D11>(*this, vertexShaderSource, geometryShaderSource,
                                                fragmentShaderSource);
}

std::shared_ptr<VertexBuffer> DeviceD3D11::DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<VertexBufferD3D11>(*this, usageHint, sizeInBytes);
}

std::shared_ptr<IndexBuffer> DeviceD3D11::DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<IndexBufferD3D11>(*this, usageHint, sizeInBytes);
}

std::shared_ptr<WritePixelBuffer> DeviceD3D11::DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                                        std::size_t sizeInBytes) {
    return std::make_shared<WritePixelBufferD3D11>(usageHint, sizeInBytes);
}

std::shared_ptr<Texture2D> DeviceD3D11::DoCreateTexture2D(const Texture2DDescription& description) {
    return std::make_shared<Texture2DD3D11>(*this, description);
}

std::shared_ptr<TextureSampler> DeviceD3D11::DoCreateTexture2DSampler(const TextureSamplerDescription& description) {
    return std::make_shared<TextureSamplerD3D11>(m_device.Get(), description);
}

} // namespace arda::renderer::d3d11
```

### `src/d3d11/ContextD3D11.cpp` (final)

```cpp
#include "d3d11/ContextD3D11.h"
#include "d3d11/DeviceD3D11.h"
#include "d3d11/GraphicsWindowD3D11.h"
#include "d3d11/StateCacheD3D11.h"
#include "d3d11/TypeConverterD3D11.h"
#include "d3d11/framebuffer/FramebufferD3D11.h"
#include "d3d11/shaders/ShaderProgramD3D11.h"
#include "d3d11/textures/Texture2DD3D11.h"
#include "d3d11/textures/TextureSamplerD3D11.h"
#include "d3d11/vertexarray/VertexArrayD3D11.h"

#include <algorithm>
#include <numeric>
#include <span>
#include <stdexcept>

namespace arda::renderer::d3d11 {

using core::geometry::PrimitiveType;

// --- Step 0: construction and the active context ------------------------------

ContextD3D11::ContextD3D11(DeviceD3D11& device, GraphicsWindowD3D11& window, int width, int height)
    : Context(device),
      m_deviceD3D11(device),
      m_window(window),
      m_boundShaderResources(static_cast<std::size_t>(GetTextureUnits().Count()), nullptr) {
    SetViewport(Rectangle{0, 0, width, height});
}

ContextD3D11::~ContextD3D11() {
    if (m_deviceD3D11.ActiveContext() == this) {
        m_deviceD3D11.SetActiveContext(nullptr);
    }
}

ID3D11DeviceContext* ContextD3D11::Immediate() const {
    return m_deviceD3D11.Immediate();
}

void ContextD3D11::MakeCurrent() {
    EnsureActive();
}

void ContextD3D11::EnsureActive() {
    if (m_deviceD3D11.ActiveContext() != this) {
        m_deviceD3D11.SetActiveContext(this);
        InvalidateCachedState();
    }
}

void ContextD3D11::InvalidateCachedState() {
    m_depthStencilState.reset();
    m_stencilReference.reset();
    m_blendState.reset();
    m_blendFactor.reset();
    m_rasterizerState.reset();
    m_scissorRectangle.reset();
    m_appliedViewport.reset();
    m_primitiveTopology.reset();
    m_inputLayout.reset();
    m_renderTargets.reset();
    m_boundVertexArray.reset();
    m_boundShaderProgram.reset();

    std::fill(m_boundShaderResources.begin(), m_boundShaderResources.end(), nullptr);
    m_textureUnitsToRebind.resize(m_boundShaderResources.size());
    std::iota(m_textureUnitsToRebind.begin(), m_textureUnitsToRebind.end(), 0);
}

bool ContextD3D11::FlipsRenderTargetY() const {
    return GetFramebuffer() != nullptr;
}

// --- Step 1: clears, render state and the viewport ------------------------------

void ContextD3D11::DoClear(const ClearState& clearState) {
    EnsureActive();
    ApplyFramebuffer();
    const RenderTargetBinding& binding = *m_renderTargets;

    if (HasFlag(clearState.buffers, ClearBuffers::ColorBuffer)) {
        const Color& c = clearState.color;
        const float color[4] = {c.red, c.green, c.blue, c.alpha};
        for (UINT i = 0; i < binding.count; ++i) {
            if (binding.views[i] != nullptr) {
                Immediate()->ClearRenderTargetView(binding.views[i], color);
            }
        }
    }

    UINT depthStencilFlags = 0;
    if (HasFlag(clearState.buffers, ClearBuffers::DepthBuffer)) {
        depthStencilFlags |= D3D11_CLEAR_DEPTH;
    }
    if (HasFlag(clearState.buffers, ClearBuffers::StencilBuffer) && m_depthStencilHasStencil) {
        depthStencilFlags |= D3D11_CLEAR_STENCIL;
    }
    if (depthStencilFlags != 0 && binding.depthStencil != nullptr) {
        Immediate()->ClearDepthStencilView(binding.depthStencil, depthStencilFlags,
                                           clearState.depth, static_cast<UINT8>(clearState.stencil));
    }
}

void ContextD3D11::DoSetViewport(const Rectangle&) {
    // Applied by ApplyViewport before each draw: it needs the target and the depth range.
}

void ContextD3D11::ApplyRenderState(const RenderState& renderState) {
    StateCacheD3D11& cache = m_deviceD3D11.StateCache();

    ID3D11DepthStencilState* depthStencil = cache.DepthStencil(MakeDepthStencilKey(renderState));
    const UINT stencilReference = StencilReference(renderState.stencilTest);
    if (m_depthStencilState != depthStencil || m_stencilReference != stencilReference) {
        Immediate()->OMSetDepthStencilState(depthStencil, stencilReference);
        m_depthStencilState = depthStencil;
        m_stencilReference = stencilReference;
    }

    ID3D11BlendState* blend = cache.Blend(MakeBlendKey(renderState));
    const std::array<float, 4> blendFactor = BlendFactor(renderState.blending);
    if (m_blendState != blend || m_blendFactor != blendFactor) {
        Immediate()->OMSetBlendState(blend, blendFactor.data(), 0xFFFFFFFFu);
        m_blendState = blend;
        m_blendFactor = blendFactor;
    }

    ID3D11RasterizerState* rasterizer = cache.Rasterizer(MakeRasterizerKey(renderState, FlipsRenderTargetY()));
    if (m_rasterizerState != rasterizer) {
        Immediate()->RSSetState(rasterizer);
        m_rasterizerState = rasterizer;
    }

    ApplyScissorTest(renderState.scissorTest);
    ApplyViewport(renderState.depthRange);
}

void ContextD3D11::ApplyScissorTest(const ScissorTest& scissorTest) {
    const Rectangle& r = scissorTest.rectangle;
    if (r.width < 0 || r.height < 0) {
        throw std::invalid_argument("renderState.scissorTest.rectangle width and height must be >= 0");
    }
    if (!scissorTest.enabled) {
        return;
    }

    const LONG top = ToTopLeftY(r.bottom, r.height);
    const std::array<LONG, 4> rectangle = {r.left, top, r.Right(), top + r.height};
    if (m_scissorRectangle != rectangle) {
        const D3D11_RECT d3dRectangle{rectangle[0], rectangle[1], rectangle[2], rectangle[3]};
        Immediate()->RSSetScissorRects(1, &d3dRectangle);
        m_scissorRectangle = rectangle;
    }
}

void ContextD3D11::ApplyViewport(const DepthRange& depthRange) {
    if (depthRange.nearValue < 0.0 || depthRange.nearValue > 1.0 ||
        depthRange.farValue < 0.0 || depthRange.farValue > 1.0) {
        throw std::invalid_argument("renderState.depthRange values must be in [0, 1]");
    }

    const Rectangle& v = GetViewport();
    const std::array<float, 6> viewport = {
        static_cast<float>(v.left),
        static_cast<float>(ToTopLeftY(v.bottom, v.height)),
        static_cast<float>(v.width),
        static_cast<float>(v.height),
        static_cast<float>(depthRange.nearValue),
        static_cast<float>(depthRange.farValue),
    };
    if (m_appliedViewport != viewport) {
        const D3D11_VIEWPORT d3dViewport{viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]};
        Immediate()->RSSetViewports(1, &d3dViewport);
        m_appliedViewport = viewport;
    }
}

int ContextD3D11::ToTopLeftY(int bottom, int height) const {
    if (GetFramebuffer()) {
        return bottom;   // flipped clip space: D3D rows from the top are GL rows from the bottom
    }
    return m_window.BackBufferHeight() - (bottom + height);
}

// --- Step 3: vertex arrays and drawing ------------------------------------------

std::shared_ptr<VertexArray> ContextD3D11::DoCreateVertexArray() {
    return std::make_shared<VertexArrayD3D11>(m_deviceD3D11, GetDevice().Limits().maximumNumberOfVertexAttributes);
}

void ContextD3D11::DoDraw(PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(primitiveType, drawState, sceneState);

    const VertexArray& vertexArray = *drawState.vertexArray;
    if (const auto& indexBuffer = vertexArray.GetIndexBuffer()) {
        Immediate()->DrawIndexed(static_cast<UINT>(indexBuffer->Count()), 0, 0);
    } else {
        Immediate()->Draw(static_cast<UINT>(vertexArray.MaximumArrayIndex() + 1), 0);
    }
}

void ContextD3D11::DoDrawRange(PrimitiveType primitiveType, int offset, int count,
                               const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(primitiveType, drawState, sceneState);

    if (drawState.vertexArray->GetIndexBuffer()) {
        Immediate()->DrawIndexed(static_cast<UINT>(count), static_cast<UINT>(offset), 0);
    } else {
        Immediate()->Draw(static_cast<UINT>(count), static_cast<UINT>(offset));
    }
}

void ContextD3D11::ApplyBeforeDraw(PrimitiveType primitiveType, const DrawState& drawState,
                                   const SceneState& sceneState) {
    EnsureActive();
    ApplyFramebuffer();
    ApplyRenderState(drawState.renderState);
    ApplyPrimitiveTopology(primitiveType);
    ApplyShaderProgram(drawState, sceneState);
    ApplyVertexArray(drawState);
    CleanTextureUnits();
}

void ContextD3D11::ApplyPrimitiveTopology(PrimitiveType primitiveType) {
    const D3D11_PRIMITIVE_TOPOLOGY topology = ToD3D(primitiveType);
    if (m_primitiveTopology != topology) {
        Immediate()->IASetPrimitiveTopology(topology);
        m_primitiveTopology = topology;
    }
}

void ContextD3D11::ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState) {
    auto& program = static_cast<ShaderProgramD3D11&>(*drawState.shaderProgram);
    if (m_boundShaderProgram.lock() != drawState.shaderProgram) {
        program.Bind(Immediate());
        m_boundShaderProgram = drawState.shaderProgram;
    }
    program.Clean(*this, drawState, sceneState);
}

void ContextD3D11::ApplyVertexArray(const DrawState& drawState) {
    auto& vertexArray = static_cast<VertexArrayD3D11&>(*drawState.vertexArray);
    const auto& program = static_cast<const ShaderProgramD3D11&>(*drawState.shaderProgram);

    const bool buffersChanged = vertexArray.Clean();
    if (buffersChanged || m_boundVertexArray.lock() != drawState.vertexArray) {
        vertexArray.BindBuffers(Immediate());
        m_boundVertexArray = drawState.vertexArray;
    }

    ID3D11InputLayout* layout = vertexArray.InputLayoutFor(program);
    if (m_inputLayout != layout) {
        Immediate()->IASetInputLayout(layout);
        m_inputLayout = layout;
    }
}

// --- Step 5: texture units -----------------------------------------------------

void ContextD3D11::CleanTextureUnits() {
    TextureUnits& units = GetTextureUnits();

    std::vector<int> indices = units.TakeDirtyUnits();
    indices.insert(indices.end(), m_textureUnitsToRebind.begin(), m_textureUnitsToRebind.end());
    m_textureUnitsToRebind.clear();

    for (const int index : indices) {
        const TextureUnit& unit = units[index];

        ID3D11ShaderResourceView* view = nullptr;
        if (const std::shared_ptr<Texture2D>& texture = unit.GetTexture()) {
            const auto& textureD3D11 = static_cast<const Texture2DD3D11&>(*texture);
            if (IsRenderTarget(textureD3D11)) {
                m_textureUnitsToRebind.push_back(index);
            } else {
                view = textureD3D11.ShaderResourceView();
            }
        }

        ID3D11SamplerState* sampler = unit.GetSampler()
            ? static_cast<const TextureSamplerD3D11&>(*unit.GetSampler()).Native()
            : nullptr;

        const auto slot = static_cast<UINT>(index);
        Immediate()->VSSetShaderResources(slot, 1, &view);
        Immediate()->GSSetShaderResources(slot, 1, &view);
        Immediate()->PSSetShaderResources(slot, 1, &view);
        Immediate()->VSSetSamplers(slot, 1, &sampler);
        Immediate()->GSSetSamplers(slot, 1, &sampler);
        Immediate()->PSSetSamplers(slot, 1, &sampler);
        m_boundShaderResources[static_cast<std::size_t>(index)] = view;
    }
}

bool ContextD3D11::IsRenderTarget(const Texture2DD3D11& texture) const {
    return std::find(m_renderTargetTextures.begin(), m_renderTargetTextures.end(), &texture) !=
           m_renderTargetTextures.end();
}

// --- Step 6: framebuffers --------------------------------------------------------

std::shared_ptr<Framebuffer> ContextD3D11::DoCreateFramebuffer() {
    return std::make_shared<FramebufferD3D11>(GetDevice().Limits().maximumNumberOfColorAttachments);
}

void ContextD3D11::ApplyFramebuffer() {
    RenderTargetBinding binding;

    if (const std::shared_ptr<Framebuffer>& framebuffer = GetFramebuffer()) {
        auto& framebufferD3D11 = static_cast<FramebufferD3D11&>(*framebuffer);
        framebufferD3D11.Clean();

        const std::span<ID3D11RenderTargetView* const> views = framebufferD3D11.RenderTargetViews();
        std::copy(views.begin(), views.end(), binding.views.begin());
        binding.count = static_cast<UINT>(views.size());
        binding.depthStencil = framebufferD3D11.DepthStencilView();
        m_depthStencilHasStencil = framebufferD3D11.DepthStencilHasStencil();
        m_renderTargetTextures = framebufferD3D11.AttachedTextures();
    } else {
        binding.views[0] = m_window.BackBufferView();
        binding.count = 1;
        binding.depthStencil = m_window.DepthStencilView();
        m_depthStencilHasStencil = true;
        m_renderTargetTextures.clear();
    }

    if (m_renderTargets != binding) {
        UnbindShaderResourcesUsedAsRenderTargets();
        Immediate()->OMSetRenderTargets(binding.count, binding.views.data(), binding.depthStencil);
        m_renderTargets = binding;
    }
}

void ContextD3D11::UnbindShaderResourcesUsedAsRenderTargets() {
    for (std::size_t slot = 0; slot < m_boundShaderResources.size(); ++slot) {
        ID3D11ShaderResourceView* bound = m_boundShaderResources[slot];
        if (bound == nullptr) {
            continue;
        }
        const bool becomesRenderTarget =
            std::any_of(m_renderTargetTextures.begin(), m_renderTargetTextures.end(),
                        [bound](const Texture2DD3D11* texture) { return texture->ShaderResourceView() == bound; });
        if (!becomesRenderTarget) {
            continue;
        }

        ID3D11ShaderResourceView* nullView = nullptr;
        const auto index = static_cast<UINT>(slot);
        Immediate()->VSSetShaderResources(index, 1, &nullView);
        Immediate()->GSSetShaderResources(index, 1, &nullView);
        Immediate()->PSSetShaderResources(index, 1, &nullView);
        m_boundShaderResources[slot] = nullptr;
        m_textureUnitsToRebind.push_back(static_cast<int>(slot));
    }
}

} // namespace arda::renderer::d3d11
```
