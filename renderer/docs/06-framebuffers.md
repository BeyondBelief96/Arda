# Step 6: Framebuffers (3.7)

**Goal:** framebuffers that hold textures as color, depth or depth/stencil
attachments; `Context::SetFramebuffer`; and draws and clears that go to the
current framebuffer. The milestone renders into a texture, then draws that
texture on screen.

**Read:** 3.7 "Framebuffers", 3.7.1 "GL Renderer Implementation" (Listing
3.29). 3.7.2 (framebuffers in Direct3D) goes with this step's D3D11 check.

By the end of this step you will have:

- a public `Framebuffer` class that stores its attachments and dirty flags in
  API-agnostic code
- `FramebufferGL3x`, which applies only the attachments that changed, sets up
  `glDrawBuffers`, and reports why a framebuffer is incomplete
- `Context::CreateFramebuffer`, `GetFramebuffer` and `SetFramebuffer`, with
  `Clear` and `Draw` both writing to whichever framebuffer is set
- a draw-time check that the depth test has a depth buffer to test against
- tests that render into textures and read the pixels back
- a program that draws the triangle into a texture, then draws that texture
  on a fullscreen quad

## OpenGlobe reference

| File | What to take from it |
|---|---|
| [Framebuffer/Framebuffer.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Framebuffer/Framebuffer.cs), [ColorAttachments.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Framebuffer/ColorAttachments.cs) | The public interface: an indexer for color attachments, `Count`, and the depth and depth/stencil properties |
| [GL3x/Framebuffer/FramebufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Framebuffer/FramebufferGL3x.cs) | `Bind`, `UnBind`, `Clean` (attach dirty attachments, `glDrawBuffers`), `Attach`, and the renderability checks in the setters |
| [GL3x/Framebuffer/ColorAttachmentsGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Framebuffer/ColorAttachmentsGL3x.cs) | Per-attachment dirty flags, the collection-wide `Dirty` flag, `Count` |
| [GL3x/ContextGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ContextGL3x.cs) | `CreateFramebuffer` (114–117), the `Framebuffer` property (146–150), `Clear` calling `ApplyFramebuffer` (154), the depth check in `VerifyDraw` (513–521), `ApplyBeforeDraw` (524–532), `ApplyFramebuffer` (581–610) |
| [Framebuffer/HighResolutionSnap.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Framebuffer/HighResolutionSnap.cs) | How OpenGlobe itself uses a framebuffer: set it and the viewport before a frame, restore both afterwards |
| [Tests/Renderer/Framebuffer/FramebufferTests.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Tests/Renderer/Framebuffer/FramebufferTests.cs), [Tests/TestUtility.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Tests/TestUtility.cs) | The attachment tests, `CreateFramebuffer` and `ValidateColor` (reading a render target back) |

## Files

```
renderer/
  include/arda/renderer/
    framebuffer/Framebuffer.h            NEW
    Context.h                            UPDATE  CreateFramebuffer, GetFramebuffer, SetFramebuffer
    Device.h                             (nothing new; the limit is already in DeviceLimits)
  src/
    framebuffer/Framebuffer.cpp          NEW     setters and validation
    Context.cpp                          UPDATE  depth check in VerifyDraw
    gl/
      framebuffer/FramebufferGL3x.h / .cpp   NEW
      ContextGL3x.h / .cpp               UPDATE  DoCreateFramebuffer, ApplyFramebuffer
  CMakeLists.txt                         UPDATE
tests/
  src/renderer/FramebufferTests.cpp      NEW
  CMakeLists.txt                         UPDATE
scene/src/main.cpp                       UPDATE  render-to-texture milestone
```

---

## Background: what a framebuffer is in OpenGL

Read this section before writing code. Every design decision in this step
follows from how GL handles framebuffers.

### The default framebuffer and framebuffer objects

Everything you have drawn so far went to the **default framebuffer**. It
belongs to the window. GLFW asked the operating system for it when it created
the window: a back color buffer (the one `SwapBuffers` shows), a 24-bit depth
buffer and an 8-bit stencil buffer (the `GLFW_DEPTH_BITS` and
`GLFW_STENCIL_BITS` hints from [Step 0](00-setup.md)). GL doesn't own that
memory, so you can't sample it as a texture.

A **framebuffer object** (FBO) is a GL object that you create, like a vertex
array. It owns no pixel memory. It is a small table of **attachment points**,
and each point either is empty or refers to an image you own:

| Attachment point | Holds | Written by |
|---|---|---|
| `GL_COLOR_ATTACHMENT0` … `GL_COLOR_ATTACHMENTn` | color textures | fragment shader outputs |
| `GL_DEPTH_ATTACHMENT` | a depth texture | the depth test and depth writes |
| `GL_STENCIL_ATTACHMENT` | a stencil image | the stencil test |
| `GL_DEPTH_STENCIL_ATTACHMENT` | a combined depth/stencil texture | shorthand for attaching the same image to both points above |

`n + 1` is `GL_MAX_COLOR_ATTACHMENTS`, which [Step 0](00-setup.md) already
stores in `DeviceLimits::maximumNumberOfColorAttachments`. GL 3.3 guarantees
at least 8.

When an FBO is bound with `glBindFramebuffer(GL_FRAMEBUFFER, name)`, draws
and clears write into its attachments instead of the window. Binding name `0`
returns to the default framebuffer. Rendering into a texture like this is
called **render to texture**. It is how you do shadow maps, post-processing,
picking and off-screen screenshots such as OpenGlobe's `HighResolutionSnap`.

> **OpenGL note — `GL_FRAMEBUFFER`, `GL_DRAW_FRAMEBUFFER` and `GL_READ_FRAMEBUFFER`:**
> GL has two framebuffer bindings: the *draw* framebuffer (where draws and
> clears go) and the *read* framebuffer (what `glReadPixels` and
> `glBlitFramebuffer` read from). Binding to `GL_FRAMEBUFFER` sets both at
> once. This renderer only ever uses `GL_FRAMEBUFFER`, as OpenGlobe does.

### Attaching a texture: `glFramebufferTexture2D`

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, fbo);
glFramebufferTexture2D(GL_FRAMEBUFFER,        // the FBO bound to this target is modified
                       GL_COLOR_ATTACHMENT0,  // which attachment point
                       GL_TEXTURE_2D,         // the texture's target
                       texture,               // texture name, or 0 to detach
                       0);                    // mipmap level to render into
```

This only records a reference to the texture's level 0 in the FBO's table. No
pixels are copied. Passing texture `0` empties the attachment point. Like
most classic GL functions, it edits whichever FBO is **currently bound**, so
the FBO must be bound first. This is the same bind-to-edit pattern you saw
with vertex arrays in [Step 3](03-vertex-data.md).

OpenGlobe calls `glFramebufferTexture` (GL 3.2) instead. It takes no texture
target and can also attach a whole layered texture. For a 2D texture both
calls do the same thing. This guide uses `glFramebufferTexture2D` because it
names the texture target explicitly, and `Texture2DGL3x` already knows its
target (`GL_TEXTURE_2D` now, `GL_TEXTURE_RECTANGLE` in Chapter 11).

### Fragment outputs, draw buffers and color attachments

A fragment shader can write several colors at once:

```glsl
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
```

Each output has a **location** (3.4.3). If you don't write
`layout(location = N)`, the linker assigns one, and
`ShaderProgram::FragmentOutputLocation("name")` from
[Step 2](02-shaders.md) asks GL which one it chose (`glGetFragDataLocation`).

Locations are *not* attachment points. The **draw buffers** table sits in
between. It belongs to the FBO and is set with `glDrawBuffers`:

```
fragment shader output          draw buffers (glDrawBuffers)       FBO attachment points
location 0  ───────────────►    [0] = GL_COLOR_ATTACHMENT0  ───►   color 0: colorTexture
location 1  ───────────────►    [1] = GL_NONE (discarded)          color 1: (empty)
                                                                   depth:   depthTexture
```

Entry `i` says where output location `i` goes. `GL_NONE` throws the value
away. OpenGlobe always fills the table as the identity mapping:
`drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i` if attachment `i` holds a
texture, `GL_NONE` otherwise. So in this renderer:

> **Fragment output location N writes to color attachment N.**

That is why the book attaches textures with
`framebuffer.ColorAttachments[sp.FragmentOutputs["fragmentColor"]] = texture`,
and why the milestone below does the same with `FragmentOutputLocation`. The
portability table in the [README](../README.md) makes the same promise for
D3D11: location N is `SV_TargetN`.

> **OpenGL note — draw buffers are per framebuffer:** the draw-buffers table
> is state of the FBO, not of the context. Setting it on your FBO doesn't
> change the default framebuffer, whose draw buffer stays `GL_BACK`. The same
> is true of the read buffer (`glReadBuffer`) and of course the attachments.
> The viewport, scissor, masks and clear values are **context** state, and
> they don't change when you bind a different framebuffer. Keep this split in
> mind: it explains why `ContextGL3x` caches the viewport but the FBO caches
> its own draw buffers.

### Completeness

Before an FBO can be drawn into, GL checks that its attachments make sense.
`glCheckFramebufferStatus(GL_FRAMEBUFFER)` returns `GL_FRAMEBUFFER_COMPLETE`
or one of these reasons:

| Status | Usual cause |
|---|---|
| `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT` | An attached texture has zero size, was deleted, or has a format that can't go at that attachment point (for example a depth format at a color point) |
| `GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT` | Nothing is attached at all |
| `GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER` | A draw buffer names a color attachment point that is empty |
| `GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER` | The read buffer names a color attachment point that is empty (GL 3.3 still checks this; GL 4.1 dropped the rule) |
| `GL_FRAMEBUFFER_UNSUPPORTED` | The driver can't render to this combination of formats. Legal but not guaranteed formats such as `GL_RGB8` can cause it |
| `GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE` | Attachments have different sample counts |
| `GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS` | Some attachments are layered and some aren't |
| `GL_FRAMEBUFFER_UNDEFINED` | The *default* framebuffer is bound, but the window has no surface |

Drawing into an incomplete FBO doesn't crash. GL records
`GL_INVALID_FRAMEBUFFER_OPERATION` and draws nothing, so an incomplete
framebuffer looks like "my texture is just black". OpenGlobe checks the
status in debug builds and throws. This guide does the same, with a readable
message for every status.

Because the class validates textures when they are attached (color textures
at color points, depth textures at depth points), most incomplete
framebuffers you'll meet are either empty or `UNSUPPORTED`.

### Framebuffer objects are not shared between contexts

[Step 0](00-setup.md) made every window's context share objects with the
device's hidden context. Sharing covers objects that hold *data*: buffers,
textures, samplers, shaders and programs. It does **not** cover *container*
objects, which only refer to other objects: **vertex arrays and framebuffer
objects**. The GL specification lists them as never shared.

So a framebuffer name created while window A's context is current means
nothing in window B's context. Binding it there is `GL_INVALID_OPERATION` at
best, and at worst the same number refers to a different FBO. This is why
vertex arrays ([Step 3](03-vertex-data.md)) and framebuffers are created by
the **Context**, not the Device (3.2). The textures you attach are ordinary
shared objects, so the same texture can be attached to FBOs in several
contexts.

> **OpenGL note — `glGenFramebuffers` only reserves a name:** the FBO itself
> is created the first time the name is bound, in whichever context is
> current at that moment. That's why `DoCreateFramebuffer` calls
> `MakeCurrent()` first, exactly like `DoCreateVertexArray`.

### The feedback loop

If a texture is attached to the bound FBO **and** the current shader can
sample it, the result of drawing is undefined. The GPU might read texels it
is in the middle of writing. GL does not report an error. You just get
garbage, flickering, or a correct image on one GPU and a broken one on
another.

The GL 3.3 rule is about *possibility*: it applies when the texture is bound
to a texture unit that a sampler uniform in the current program refers to,
whether or not the shader actually executes the `texture()` call. The fix is
always the same. Before drawing into a texture, make sure no texture unit that
the program can read holds it. The milestone does this explicitly in both
passes.

Reading a render target with `glGetTexImage` (what `Texture2D::CopyToBuffer`
does) is *not* a feedback loop. GL executes commands in order, so the read
sees the finished draw.

### The viewport follows you, not the framebuffer

`glViewport` maps clip space to pixel coordinates in whatever framebuffer is
bound. It is context state, so binding an FBO does **not** change it. If the
window is 800×600 and you render into a 512×512 texture without changing the
viewport, GL maps clip space to an 800×600 area. Only the lower-left 512×512
of that lands in the texture, so the image appears cropped and off-center.

The rule: **whenever you switch render targets, set the viewport to the
target's size, and set it back afterwards.** OpenGlobe's `HighResolutionSnap`
does exactly this: it saves `context.Viewport`, sets the framebuffer and a
viewport the size of the snapshot, and restores both after the frame. The
camera's aspect ratio must match the target too, or the image is stretched.

### Mipmaps go stale

Rendering writes only to the attached mipmap level (level 0 here). If the
texture has mipmaps, levels 1 and up still hold whatever they held before,
and a sampler with a mipmap minification filter will blend old and new
content. After rendering into a mipmapped texture, you must regenerate its
mipmaps (`glGenerateMipmap`) before sampling it.

This step keeps things simple: render targets are created with
`generateMipmaps = false` and sampled with `linearClamp`, which has no
mipmap filter. If you pair a **mipmapped filter** with a texture that
**has no mipmaps**, the texture is incomplete for sampling and reads as black.
That is a different common pitfall. Neither `Texture2D` nor OpenGlobe exposes
"regenerate mipmaps after rendering" yet. When a later chapter needs it, add
a public `Texture2D::GenerateMipmaps()` that forwards to the backend.

---

## Design: how OpenGlobe models this, and the C++ shape

OpenGlobe splits the public side into two abstract classes:

```csharp
public abstract class Framebuffer : Disposable {
    public abstract ColorAttachments ColorAttachments { get; }
    public abstract Texture2D DepthAttachment { get; set; }
    public abstract Texture2D DepthStencilAttachment { get; set; }
}

public abstract class ColorAttachments {
    public abstract Texture2D this[int index] { get; set; }   // framebuffer.ColorAttachments[0] = texture;
    public abstract int Count { get; }
}
```

All the real work is in `FramebufferGL3x` and `ColorAttachmentsGL3x`: the
setters validate the texture, record it and set dirty flags. `Clean` later
pushes only the dirty attachments to GL. `ContextGL3x.ApplyFramebuffer`
binds the framebuffer and calls `Clean` before every clear and draw.

The C++ port makes three changes.

1. **The attachments and dirty flags live in the base class.** The setters,
   the validation and the count are the same for every backend, so they are
   written once in `Framebuffer`. Backends read the protected members and
   only translate changes to their API. `VertexArray`
   ([Step 3](03-vertex-data.md)) and `TextureUnits` ([Step 5](05-textures.md))
   follow the same pattern.
2. **`ColorAttachments` becomes indexed functions on `Framebuffer`:**
   `SetColorAttachment(index, texture)` and `ColorAttachment(index)`. The
   C++ note on indexers below explains why.
3. **`VerifyDraw`'s depth check moves into the base `Context`.** It uses only
   public information, so every backend gets it for free.

> **Why:** *framebuffers are created by the Context, not the Device (3.7).*
> A GL framebuffer object is a container object and isn't shared between
> contexts (see the background section). If `Device::CreateFramebuffer`
> existed, it would have to guess which context the FBO belongs to. D3D11 has
> no such restriction, because its views are device objects, but the
> interface follows the stricter API so the same code runs on both.

> **Why:** *attachments are applied lazily.* `SetColorAttachment` doesn't
> touch GL. It records the texture and sets a dirty flag. `Clean` applies the
> changes the next time the framebuffer is used. This has several benefits:
>
> - Calling `glFramebufferTexture2D` needs the FBO bound. An eager setter
>   would have to bind it behind the context's back, which breaks the cached
>   binding `ContextGL3x` keeps, or save and restore the old binding every
>   time.
> - The setter can be called while another context is current. Only the
>   context that uses the framebuffer ever makes GL calls for it.
> - Setting three attachments costs three flag writes, followed by one
>   `glDrawBuffers` and one completeness check. Every attachment change
>   makes the driver revalidate the FBO, so batching them is cheaper.
> - D3D11 creates render target views at the same point (Step 8), so the
>   base class serves both backends unchanged.

> **Why:** *the framebuffer is Context state, not part of `DrawState`.* The
> book gives the reason at the end of 3.7: an object that draws itself
> shouldn't need to know where it's drawing. A globe, a billboard set or a
> vector layer fills in a `DrawState` and calls `Draw`. Whether that ends up
> in the window, a shadow map or a high-resolution screenshot is decided by
> whoever sets `context.Framebuffer` around those calls. `HighResolutionSnap`
> relies on this: it redirects a whole frame into a large framebuffer without
> any scene object knowing. `Clear` also needs to know the target, and it
> takes no `DrawState`. The viewport is context state for the same reason.

> **Why:** *textures only, no renderbuffers.* GL also has renderbuffers:
> images that can be attached to an FBO but never sampled. They can be a
> little faster for a depth buffer you never read. OpenGlobe attaches only
> textures, and so does this port. That is one resource type instead of two,
> D3D11 has no renderbuffer equivalent (everything is a texture with bind
> flags), and any attachment can be saved or sampled for debugging.

---

## Step 6.1: The public `Framebuffer` class

### `include/arda/renderer/framebuffer/Framebuffer.h` (Listing 3.29)

This class ports `Framebuffer.cs`, `ColorAttachments.cs`, and the setter half
of `FramebufferGL3x.cs` and `ColorAttachmentsGL3x.cs`.

```cpp
#pragma once

#include <arda/renderer/textures/Texture2D.h>

#include <memory>
#include <vector>

namespace arda::renderer {

// A set of textures to render into (3.7). Create one with Context::CreateFramebuffer,
// then make it the render target with Context::SetFramebuffer.
//
// Attachments are recorded here and applied by the backend the next time the
// framebuffer is used by a Clear or Draw.
class Framebuffer {
public:
    virtual ~Framebuffer() = default;
    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // index is usually program->FragmentOutputLocation("outputName") (3.4.3):
    // fragment output location N writes to color attachment N.
    // Pass nullptr to remove an attachment.
    // Throws std::invalid_argument if the texture isn't color renderable, and
    // std::out_of_range if index is outside [0, MaximumNumberOfColorAttachments()).
    void SetColorAttachment(int index, std::shared_ptr<Texture2D> texture);
    const std::shared_ptr<Texture2D>& ColorAttachment(int index) const;
    int NumberOfColorAttachments() const { return m_colorCount; }   // how many are set
    int MaximumNumberOfColorAttachments() const { return static_cast<int>(m_colorAttachments.size()); }

    // Throws std::invalid_argument if the texture isn't depth renderable.
    void SetDepthAttachment(std::shared_ptr<Texture2D> texture);
    const std::shared_ptr<Texture2D>& DepthAttachment() const { return m_depthAttachment.texture; }

    // Throws std::invalid_argument if the texture isn't depth/stencil renderable.
    // When set, it is used for depth as well, and takes precedence over DepthAttachment().
    void SetDepthStencilAttachment(std::shared_ptr<Texture2D> texture);
    const std::shared_ptr<Texture2D>& DepthStencilAttachment() const { return m_depthStencilAttachment.texture; }

protected:
    explicit Framebuffer(int maximumNumberOfColorAttachments)
        : m_colorAttachments(static_cast<std::size_t>(maximumNumberOfColorAttachments)) {}

    // One attachment point: the texture (or nullptr) and whether the backend
    // still has to apply it (ColorAttachmentGL3x in OpenGlobe).
    struct Attachment {
        std::shared_ptr<Texture2D> texture;
        bool dirty = false;
    };

    // For backends. Indexed by attachment point; the size is fixed at construction.
    std::vector<Attachment> m_colorAttachments;
    bool m_colorAttachmentsDirty = false;   // true if any color attachment is dirty
    Attachment m_depthAttachment;
    Attachment m_depthStencilAttachment;

private:
    int m_colorCount = 0;   // only the setters change it, so it can't drift out of sync
};

} // namespace arda::renderer
```

Read it from the top.

- **The class is non-copyable and abstract.** The constructor is protected,
  so only a backend can create one, through `Context::CreateFramebuffer`.
  Copying is deleted: a copy would share one GL object between two C++
  objects, and both would delete it. This is the rule of 0/3/5 from
  [Step 0](00-setup.md).
- **`m_colorAttachments` is a vector with one entry per attachment point**,
  sized once from the device limit. An empty point holds a null
  `shared_ptr`. The vector never grows or shrinks, so index `i` is always
  attachment point `i`.
- **Each `Attachment` has its own dirty flag**, and
  `m_colorAttachmentsDirty` is a summary flag so `Clean` can skip the color
  loop when nothing changed. This is the dirty-flag pattern from
  [Step 2](02-shaders.md), used twice: once per slot and once for the
  collection.
- **The depth and depth/stencil attachments are separate slots**, as in
  OpenGlobe. A texture can't go in both, because each setter checks a
  different format property.

> **C++ note — a fixed-index collection sized at run time: `std::vector` vs `std::array`:**
> C# writes `new ColorAttachmentGL3x[Device.MaximumNumberOfColorAttachments]`:
> an array whose length is chosen at run time and never changes. C++ has two
> candidates.
>
> - `std::array<Attachment, 8>` has its length baked into the type, so it
>   must be a compile-time constant. The real limit is a GL query answered at
>   run time, and it can be larger than 8. D3D11's limit is a constant (8),
>   but the base class serves both backends.
> - `std::vector<Attachment>` can be sized at run time. Constructing it with
>   a count, `std::vector<Attachment>(n)`, creates `n` value-initialized
>   elements: every `texture` is null and every `dirty` is `false`. If you
>   never call `push_back` or `resize` afterwards, it behaves like a
>   fixed-size array whose size was picked at run time.
>
> `std::array` is the better choice when the size really is a compile-time
> constant, as in `ContextD3D11::ApplyFramebuffer` in
> [Step 8](08-direct3d11.md), which uses
> `std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT>`.
> [Step 5](05-textures.md) covered `std::array` itself.

> **C++ note — `protected` members and nested types:** `Attachment`,
> `m_colorAttachments` and the dirty flags are `protected`. Derived classes
> (`FramebufferGL3x`, and `FramebufferD3D11` in Step 8) can use them, but
> users of `Framebuffer` can't. This is C#'s `internal`/`protected` split
> collapsed into one keyword. `m_colorCount` is `private`, so even a backend
> can't change it. Only the base class's setters update it, and they always
> keep it equal to the number of non-null slots.

### Indexers: why `SetColorAttachment` instead of `colorAttachments[i] = texture`

In C#, `framebuffer.ColorAttachments[0] = texture` calls a *setter* on an
indexer. The setter validates the texture and marks the slot dirty. C++ has
no indexer properties. `operator[]` returns something, and assignment then
acts on that something.

> **C++ note — C# indexers vs `operator[]`, and proxy objects:** if
> `operator[]` returned `std::shared_ptr<Texture2D>&`, then
> `framebuffer[0] = texture` would write straight into the vector. The
> renderability check would never run, the dirty flag would never be set, and
> the count would drift. The only way to intercept the assignment is to
> return a **proxy**, a small object whose `operator=` calls the real setter:
>
> ```cpp
> class ColorAttachmentProxy {
> public:
>     ColorAttachmentProxy& operator=(std::shared_ptr<Texture2D> texture) {
>         m_framebuffer.SetColorAttachment(m_index, std::move(texture));   // validation + dirty flag
>         return *this;
>     }
>     operator const std::shared_ptr<Texture2D>&() const { return m_framebuffer.ColorAttachment(m_index); }
> private:
>     Framebuffer& m_framebuffer;
>     int m_index;
> };
> // ColorAttachmentProxy Framebuffer::operator[](int index) { return {*this, index}; }
> ```
>
> The standard library does this in `std::vector<bool>` and
> `std::bitset::reference`, and it is a well-known source of surprises:
>
> - `auto t = framebuffer[0];` makes `t` a proxy, not a `shared_ptr`, and it
>   dangles if the framebuffer is destroyed.
> - `framebuffer[0]->Description()` doesn't compile, because the proxy has no
>   `operator->`.
> - Implicit conversions make overload resolution and template argument
>   deduction harder to predict.
>
> A plain `SetColorAttachment(i, t)` / `ColorAttachment(i)` pair is what
> `VertexArray::SetAttribute` / `Attribute` already look like. It makes the
> side effects (validation, the dirty flag) visible at the call site. The
> README's naming rule covers this: setters get a `Set` prefix.

> **C++ note — returning `const std::shared_ptr<T>&`:** the getters return a
> *reference* to the stored `shared_ptr`. That costs nothing. Returning by
> value would copy the `shared_ptr`, and a copy is an atomic increment
> followed by an atomic decrement. The caller can still copy it if it wants
> to keep the texture (`auto texture = framebuffer->ColorAttachment(0);`),
> or use it in place (`framebuffer->ColorAttachment(0)->Save("color.png")`).
> The reference is only valid while the framebuffer exists and the slot
> isn't reassigned, so don't store the reference itself. Parameters are the
> opposite case: see the "sink" note in Step 6.2.
> [Step 1](01-state-management.md) covers `const&` vs value for parameters.

---

## Step 6.2: `src/framebuffer/Framebuffer.cpp`

The setters port `ColorAttachmentsGL3x`'s indexer and `FramebufferGL3x`'s
`DepthAttachment` and `DepthStencilAttachment` setters. The error messages
are OpenGlobe's, with C++ member names.

```cpp
#include <arda/renderer/framebuffer/Framebuffer.h>

#include <stdexcept>
#include <utility>

namespace arda::renderer {

void Framebuffer::SetColorAttachment(int index, std::shared_ptr<Texture2D> texture) {
    if (texture && !texture->Description().ColorRenderable()) {
        throw std::invalid_argument(
            "Texture must be color renderable but Description().ColorRenderable() is false.");
    }

    // at() throws std::out_of_range for a negative or too-large index.
    Attachment& attachment = m_colorAttachments.at(static_cast<std::size_t>(index));
    if (attachment.texture == texture) {
        return;   // same texture (or both null): nothing to apply
    }

    if (attachment.texture && !texture) {
        --m_colorCount;
    } else if (!attachment.texture && texture) {
        ++m_colorCount;
    }

    attachment.texture = std::move(texture);
    attachment.dirty = true;
    m_colorAttachmentsDirty = true;
}

const std::shared_ptr<Texture2D>& Framebuffer::ColorAttachment(int index) const {
    return m_colorAttachments.at(static_cast<std::size_t>(index)).texture;
}

void Framebuffer::SetDepthAttachment(std::shared_ptr<Texture2D> texture) {
    if (texture && !texture->Description().DepthRenderable()) {
        throw std::invalid_argument(
            "Texture must be depth renderable but Description().DepthRenderable() is false.");
    }

    if (m_depthAttachment.texture != texture) {
        m_depthAttachment.texture = std::move(texture);
        m_depthAttachment.dirty = true;
    }
}

void Framebuffer::SetDepthStencilAttachment(std::shared_ptr<Texture2D> texture) {
    if (texture && !texture->Description().DepthStencilRenderable()) {
        throw std::invalid_argument(
            "Texture must be depth/stencil renderable but Description().DepthStencilRenderable() is false.");
    }

    if (m_depthStencilAttachment.texture != texture) {
        m_depthStencilAttachment.texture = std::move(texture);
        m_depthStencilAttachment.dirty = true;
    }
}

} // namespace arda::renderer
```

`ColorRenderable`, `DepthRenderable` and `DepthStencilRenderable` come from
`Texture2DDescription` ([Step 5](05-textures.md)):

| Format | Color | Depth | Depth/stencil |
|---|---|---|---|
| `RedGreenBlue8`, `RedGreenBlueAlpha8`, `SRedGreenBlue8Alpha8`, `Red8`, `Red32f`, `RedGreen32f`, `RedGreenBlueAlpha32f` | yes | | |
| `Depth16`, `Depth24`, `Depth32f` | | yes | |
| `Depth24Stencil8`, `Depth32fStencil8` | | yes | yes |

A `Depth24Stencil8` texture is accepted by *both* depth setters, so you can
use it as a plain depth buffer and ignore its stencil bits.

> **C++ note — `std::shared_ptr` comparisons and null:** a `shared_ptr` can
> be compared the way C# compares references.
>
> - `a == b` compares the stored raw pointers (`a.get() == b.get()`). Two
>   `shared_ptr`s to the same texture are equal, even if one is a copy of the
>   other. Two null ones are also equal, so `attachment.texture == texture`
>   covers the "set null on an empty slot" case too.
> - `if (texture)` and `!texture` use `shared_ptr`'s `explicit operator bool`,
>   which is true when it points at something. That is C#'s
>   `texture != null`.
> - Comparing with `nullptr` works as well: `texture == nullptr`.
> - Pointers to base and derived types compare correctly after the usual
>   pointer conversion. A `shared_ptr<Framebuffer>` and a
>   `shared_ptr<FramebufferGL3x>` to the same object are equal.
> - Comparison looks only at the address. It ignores the control block and
>   the use count, which is what you want here.
>
> ```cpp
> std::shared_ptr<Texture2D> a = device->CreateTexture2D(description);
> std::shared_ptr<Texture2D> b = a;    // same texture, use count 2
> std::shared_ptr<Texture2D> none;     // null
> a == b;              // true
> none == nullptr;     // true
> if (!none) { /* empty slot */ }
> ```
>
> [Step 3](03-vertex-data.md) introduced `shared_ptr` ownership.

> **C++ note — sink parameters: take `shared_ptr` by value, then `std::move` it:**
> the setters take `std::shared_ptr<Texture2D> texture` *by value* and move it
> into the slot at the end. A caller passing an lvalue pays for one copy (one
> increment), and a caller passing a temporary pays nothing. Taking
> `const std::shared_ptr<Texture2D>&` would force a copy inside the function
> in every case. This is the "sink argument" idiom: take by value what you
> will keep. The move itself is covered in [Step 0](00-setup.md).
> Note that the early `return` in `SetColorAttachment` happens before the
> move, so `texture` is only moved from on the path that stores it.

> **C++ note — `at()` vs `operator[]`:** `m_colorAttachments.at(i)` checks
> the index and throws `std::out_of_range`. `m_colorAttachments[i]` doesn't
> check, and an out-of-range index is undefined behavior. Public functions
> that take an index from the caller use `at()`. Backend loops that iterate
> `0 .. size()` use `[]`, because the index is correct by construction. A
> negative `int` cast to `std::size_t` becomes a huge number, so `at()`
> catches negative indices too.

> **Why:** *validation happens in the setter, not in `Clean`.* An exception
> from `SetColorAttachment` points at the line that attached the wrong
> texture. The same mistake found in `Clean` would surface later, inside a
> `Draw`, far from its cause. OpenGlobe validates in the setters for the same
> reason.

### Checkpoint: build now

Add the new files to CMake (the IDE header list is optional, but matches the
existing `renderer/CMakeLists.txt`):

```cmake
add_library(arda_renderer
    # ...existing sources...
    src/framebuffer/Framebuffer.cpp
    # ...existing headers...
    include/arda/renderer/framebuffer/Framebuffer.h
)
```

Build. Nothing uses `Framebuffer` yet, but this compiles the header and the
setters on their own, so any typo shows up here rather than mixed in with
backend errors.

---

## Step 6.3: The `Context` interface

### `include/arda/renderer/Context.h` (complete after Step 6)

This is the whole header, as built up by Steps 0, 1, 3, 5 and 6. The lines
marked `// Step 6` are new. If your copy has extra comments from earlier
steps, keep them.

```cpp
#pragma once

#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/framebuffer/Framebuffer.h>     // Step 6
#include <arda/renderer/mesh/MeshBuffers.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderVertexAttribute.h>
#include <arda/renderer/textures/TextureUnits.h>
#include <arda/renderer/vertexarray/VertexArray.h>

#include <memory>

namespace arda::renderer {

class Device;

// Issues rendering commands for one window (3.2, Listing 3.2).
class Context {
public:
    virtual ~Context() = default;

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    Device& GetDevice() const { return m_device; }

    virtual void MakeCurrent() = 0;

    // Step 1: clears and the viewport (3.3)
    void Clear(const ClearState& clearState) { DoClear(clearState); }

    const Rectangle& GetViewport() const { return m_viewport; }
    void SetViewport(const Rectangle& viewport);

    // Step 3: vertex arrays and drawing (3.5)
    std::shared_ptr<VertexArray> CreateVertexArray() { return DoCreateVertexArray(); }
    std::shared_ptr<VertexArray> CreateVertexArray(const MeshBuffers& meshBuffers);
    std::shared_ptr<VertexArray> CreateVertexArray(const core::geometry::Mesh& mesh,
                                                   const ShaderVertexAttributeCollection& shaderAttributes,
                                                   BufferHint usageHint);

    // Draws every vertex (or every index, if the vertex array has an index buffer).
    void Draw(core::geometry::PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState);

    // Draws count vertices or indices, starting at offset.
    void Draw(core::geometry::PrimitiveType primitiveType, int offset, int count,
              const DrawState& drawState, const SceneState& sceneState);

    // Step 5: texture units (3.6)
    TextureUnits& GetTextureUnits() { return *m_textureUnits; }

    // Step 6: framebuffers (3.7)
    // The framebuffer belongs to this context and can only be set on this context.
    [[nodiscard]] std::shared_ptr<Framebuffer> CreateFramebuffer() { return DoCreateFramebuffer(); }

    // The render target for Clear and Draw. nullptr (the default) means the window's own framebuffer.
    // Takes effect at the next Clear or Draw. Remember to set a matching viewport.
    const std::shared_ptr<Framebuffer>& GetFramebuffer() const { return m_framebuffer; }
    void SetFramebuffer(std::shared_ptr<Framebuffer> framebuffer) { m_framebuffer = std::move(framebuffer); }

protected:
    explicit Context(Device& device);

    virtual void DoClear(const ClearState& clearState) = 0;
    virtual void DoSetViewport(const Rectangle& viewport) = 0;

    // Vertex arrays and framebuffers belong to the context that is current when
    // they are created, because GL doesn't share them between contexts (3.2).
    virtual std::shared_ptr<VertexArray> DoCreateVertexArray() = 0;
    virtual std::shared_ptr<Framebuffer> DoCreateFramebuffer() = 0;   // Step 6

    virtual void DoDraw(core::geometry::PrimitiveType primitiveType,
                        const DrawState& drawState, const SceneState& sceneState) = 0;
    virtual void DoDrawRange(core::geometry::PrimitiveType primitiveType, int offset, int count,
                             const DrawState& drawState, const SceneState& sceneState) = 0;

    // Checks shared by every backend. Throws std::invalid_argument.
    void VerifyDraw(const DrawState& drawState) const;

private:
    Device& m_device;
    Rectangle m_viewport;
    std::unique_ptr<TextureUnits> m_textureUnits;
    std::shared_ptr<Framebuffer> m_framebuffer;   // Step 6: null = the window's framebuffer
};

} // namespace arda::renderer
```

The C# property pair `Framebuffer { get; set; }` becomes
`GetFramebuffer()` / `SetFramebuffer()`. Plain `Framebuffer()` would clash
with the type name, which is why the README reserves the `Get` prefix for
getters whose name is also a type. `CreateFramebuffer` follows the
non-virtual interface pattern from [Step 0](00-setup.md): public and
non-virtual, forwarding to the protected virtual `DoCreateFramebuffer`.

`SetFramebuffer` takes any `Framebuffer`. The GL backend `static_cast`s it
to `FramebufferGL3x` when it's applied, so passing a framebuffer created by a
different backend, or by a different window's context, is a programming
error, just as in OpenGlobe, where the cast in the property setter would
throw.

> **C++ note — `[[nodiscard]]`:** marking a function `[[nodiscard]]` asks the
> compiler to warn when a caller ignores its return value:
>
> ```cpp
> context.CreateFramebuffer();                      // warning: discarding return value
> auto framebuffer = context.CreateFramebuffer();   // fine
> ```
>
> A framebuffer that is created and immediately thrown away is always a bug.
> It is created, and then destroyed at the end of the statement. The
> attribute costs nothing at run time and doesn't change the function's
> type, so adding it doesn't affect any caller that uses the result. Good
> candidates are factory functions (`Create*`), functions whose only effect
> is their result, and error codes that must be checked. C# has no direct
> equivalent. The closest is the analyzer warning for an ignored return
> value. You could add `[[nodiscard]]` to `CreateVertexArray` and the
> `Device::Create*` functions for the same reason.

> **C++ note — a shared_ptr member as "current state":** the context holds a
> `shared_ptr`, not a raw pointer, so the framebuffer set on the context
> can't be destroyed while it's set, even if the caller drops its own copy.
> That is also why the framebuffer holds `shared_ptr`s to its textures: an
> attached texture stays alive for as long as it is attached, so GL never
> renders into a deleted texture. Ownership is the README's rule:
> containers keep their contents alive.

### `src/Context.cpp` (complete after Step 6)

The only Step 6 change is the depth check at the end of `VerifyDraw`. The
whole file is shown so you can compare it with yours.

```cpp
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>

#include <stdexcept>

namespace arda::renderer {

using core::geometry::PrimitiveType;

Context::Context(Device& device)
    : m_device(device),
      m_textureUnits(std::make_unique<TextureUnits>(device.Limits().numberOfTextureUnits)) {}

void Context::SetViewport(const Rectangle& viewport) {
    if (viewport.width < 0 || viewport.height < 0) {
        throw std::invalid_argument("SetViewport: width and height must be greater than or equal to zero");
    }
    if (viewport != m_viewport) {
        m_viewport = viewport;
        DoSetViewport(viewport);
    }
}

// Context.cs lines 33-50
std::shared_ptr<VertexArray> Context::CreateVertexArray(const MeshBuffers& meshBuffers) {
    auto vertexArray = CreateVertexArray();
    vertexArray->SetIndexBuffer(meshBuffers.indexBuffer);
    for (std::size_t location = 0; location < meshBuffers.attributes.size(); ++location) {
        if (meshBuffers.attributes[location]) {
            vertexArray->SetAttribute(static_cast<int>(location), meshBuffers.attributes[location]);
        }
    }
    return vertexArray;
}

std::shared_ptr<VertexArray> Context::CreateVertexArray(const core::geometry::Mesh& mesh,
                                                        const ShaderVertexAttributeCollection& shaderAttributes,
                                                        BufferHint usageHint) {
    return CreateVertexArray(GetDevice().CreateMeshBuffers(mesh, shaderAttributes, usageHint));
}

void Context::Draw(PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState) {
    VerifyDraw(drawState);
    DoDraw(primitiveType, drawState, sceneState);
}

void Context::Draw(PrimitiveType primitiveType, int offset, int count,
                   const DrawState& drawState, const SceneState& sceneState) {
    if (offset < 0 || count < 0) {
        throw std::invalid_argument("Draw: offset and count must be >= 0");
    }
    VerifyDraw(drawState);
    DoDrawRange(primitiveType, offset, count, drawState, sceneState);
}

// ContextGL3x.cs VerifyDraw (lines 486-522).
void Context::VerifyDraw(const DrawState& drawState) const {
    if (!drawState.shaderProgram) {
        throw std::invalid_argument("drawState.shaderProgram is null");
    }
    if (!drawState.vertexArray) {
        throw std::invalid_argument("drawState.vertexArray is null");
    }

    // Step 6. With no depth buffer, GL silently behaves as if the depth test
    // were disabled, so a forgotten depth attachment shows up as wrong
    // occlusion, not as an error (3.7).
    if (m_framebuffer && drawState.renderState.depthTest.enabled &&
        !m_framebuffer->DepthAttachment() && !m_framebuffer->DepthStencilAttachment()) {
        throw std::invalid_argument(
            "The depth test is enabled (drawState.renderState.depthTest.enabled) but the context's "
            "framebuffer has no depth or depth/stencil attachment (DepthAttachment() or "
            "DepthStencilAttachment()).");
    }
}

} // namespace arda::renderer
```

> **Why:** *the depth check is in the base `Context`.* OpenGlobe puts it in
> `ContextGL3x.VerifyDraw`, but it only reads public state: the draw state and
> the framebuffer's attachments. Moving it into the base class gives D3D11
> the same check for free. Remember that `RenderState`'s depth test is
> **enabled by default** ([Step 1](01-state-management.md)), unlike GL's. So
> a framebuffer with only a color attachment throws on the first draw, unless
> you add a depth attachment or turn the depth test off. The check doesn't
> apply when no framebuffer is set, because the window always has a depth
> buffer.

---

## Step 6.4: The GL framebuffer

### `src/gl/framebuffer/FramebufferGL3x.h`

```cpp
#pragma once

#include "gl/GLHandle.h"

#include <arda/renderer/framebuffer/Framebuffer.h>

#include <memory>
#include <string_view>

namespace arda::renderer::gl {

// FramebufferGL3x.cs and ColorAttachmentsGL3x.cs. The attachments are stored in
// the Framebuffer base class. This class owns the GL framebuffer object and
// applies attachment changes to it.
class FramebufferGL3x final : public Framebuffer {
public:
    // Call with the owning context current: FBOs are not shared between contexts.
    explicit FramebufferGL3x(int maximumNumberOfColorAttachments)
        : Framebuffer(maximumNumberOfColorAttachments), m_name(CreateFramebufferName()) {}

    void Bind() const { glBindFramebuffer(GL_FRAMEBUFFER, m_name.Get()); }
    static void Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }   // back to the window's framebuffer

    // Applies changed attachments, the draw buffers and the read buffer.
    // The framebuffer must be bound.
    void Clean();

    // A readable explanation of a glCheckFramebufferStatus result.
    [[nodiscard]] static std::string_view StatusMessage(GLenum status);

private:
    static void Attach(GLenum attachPoint, const std::shared_ptr<Texture2D>& texture);

    FramebufferName m_name;
};

} // namespace arda::renderer::gl
```

`FramebufferName` and `CreateFramebufferName` have been in `GLHandle.h`
since [Step 0](00-setup.md). They replace OpenGlobe's
`FramebufferNameGL3x`: `glGenFramebuffers` in `CreateFramebufferName`, and
`glDeleteFramebuffers` in the destructor. Nothing else is needed for cleanup,
so the class follows the rule of zero for its own members. The
`Framebuffer` base class has already deleted copying, and `GLHandle`'s move
operations are never used because the object always lives inside a
`shared_ptr`.

`Clean` is a normal, non-virtual member function. Only `ContextGL3x` calls it,
after it has cast the framebuffer to `FramebufferGL3x&` (see
[Step 2](02-shaders.md) for `static_cast` vs `dynamic_cast`). A virtual
`Clean` on the public base class would expose a backend detail to every
user of the renderer.

### `src/gl/framebuffer/FramebufferGL3x.cpp`

```cpp
#include "gl/framebuffer/FramebufferGL3x.h"
#include "gl/textures/Texture2DGL3x.h"

#include <algorithm>
#include <vector>

namespace arda::renderer::gl {

void FramebufferGL3x::Clean() {
    if (m_colorAttachmentsDirty) {
        // Draw buffer i receives fragment output location i. Point it at
        // color attachment i if that holds a texture, otherwise discard it.
        std::vector<GLenum> drawBuffers(m_colorAttachments.size(), GL_NONE);
        std::size_t numberOfDrawBuffers = 0;   // 1 + the highest attached index
        GLenum readBuffer = GL_NONE;           // the lowest attached index

        for (std::size_t i = 0; i < m_colorAttachments.size(); ++i) {
            Attachment& attachment = m_colorAttachments[i];
            const GLenum attachPoint = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);

            if (attachment.dirty) {
                Attach(attachPoint, attachment.texture);
                attachment.dirty = false;
            }

            if (attachment.texture) {
                drawBuffers[i] = attachPoint;
                numberOfDrawBuffers = i + 1;
                if (readBuffer == GL_NONE) {
                    readBuffer = attachPoint;
                }
            }
        }

        // Only pass entries up to the last attachment: glDrawBuffers fails if n is
        // larger than GL_MAX_DRAW_BUFFERS, which may be smaller than
        // GL_MAX_COLOR_ATTACHMENTS. With no color attachments, pass { GL_NONE }.
        glDrawBuffers(static_cast<GLsizei>(std::max<std::size_t>(numberOfDrawBuffers, 1)), drawBuffers.data());

        // An FBO's read buffer defaults to GL_COLOR_ATTACHMENT0. GL 3.3 reports the
        // FBO incomplete if that point is empty, for example in a depth-only framebuffer.
        glReadBuffer(readBuffer);

        m_colorAttachmentsDirty = false;
    }

    // A depth/stencil texture occupies the depth attachment point too:
    //   "Attaching a level of a texture to GL_DEPTH_STENCIL_ATTACHMENT is equivalent
    //    to attaching that level to both the GL_DEPTH_ATTACHMENT and the
    //    GL_STENCIL_ATTACHMENT attachment points simultaneously."
    // So when both are set, the depth/stencil attachment wins.
    const bool depthStencilChanged = m_depthStencilAttachment.dirty;
    if (m_depthStencilAttachment.dirty) {
        Attach(GL_DEPTH_STENCIL_ATTACHMENT, m_depthStencilAttachment.texture);
        m_depthStencilAttachment.dirty = false;
    }

    // Reattach the depth texture when it changed, or when removing the
    // depth/stencil texture just emptied the depth point.
    if (m_depthAttachment.dirty || depthStencilChanged) {
        if (!m_depthStencilAttachment.texture) {
            Attach(GL_DEPTH_ATTACHMENT, m_depthAttachment.texture);
        }
        m_depthAttachment.dirty = false;
    }
}

void FramebufferGL3x::Attach(GLenum attachPoint, const std::shared_ptr<Texture2D>& texture) {
    if (texture) {
        const auto& textureGL = static_cast<const Texture2DGL3x&>(*texture);
        // Mipmap level 0. OpenGlobe has a "TODO: Mipmap level" here as well.
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, textureGL.Target(), textureGL.Handle(), 0);
    } else {
        // Texture 0 detaches; the texture target is ignored in that case.
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachPoint, GL_TEXTURE_2D, 0, 0);
    }
}

std::string_view FramebufferGL3x::StatusMessage(GLenum status) {
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return "the framebuffer is complete";
    case GL_FRAMEBUFFER_UNDEFINED:
        return "GL_FRAMEBUFFER_UNDEFINED: the default framebuffer is bound, but the window has no surface";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: an attached texture has zero width or height, "
               "was deleted, or has a format that can't be attached at that point";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: nothing is attached; "
               "set at least one color, depth or depth/stencil attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: a draw buffer (glDrawBuffers) names an empty "
               "color attachment point";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: the read buffer (glReadBuffer) names an empty "
               "color attachment point";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "GL_FRAMEBUFFER_UNSUPPORTED: this driver can't render to this combination of formats "
               "(try RedGreenBlueAlpha8 instead of RedGreenBlue8, or a Depth24Stencil8 depth/stencil "
               "attachment instead of separate depth and stencil)";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: the attachments have different numbers of samples";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: some attachments are layered and others are not";
    case 0:
        return "glCheckFramebufferStatus failed: a GL error occurred (check glGetError)";
    default:
        return "unknown framebuffer status";
    }
}

} // namespace arda::renderer::gl
```

Walk through `Clean` once with the milestone's framebuffer: color attachment
0 is `colorTexture`, the depth attachment is `depthTexture`, and all flags
are dirty.

1. The color loop attaches `colorTexture` at `GL_COLOR_ATTACHMENT0` and
   builds `drawBuffers = {GL_COLOR_ATTACHMENT0, GL_NONE, …}` with
   `numberOfDrawBuffers = 1`.
2. `glDrawBuffers(1, {GL_COLOR_ATTACHMENT0})` routes output location 0 to
   `colorTexture`. `glReadBuffer(GL_COLOR_ATTACHMENT0)` keeps the read buffer
   valid.
3. The depth/stencil slot isn't dirty, so step 4 runs only because the depth
   slot is dirty. No depth/stencil texture is set, so `depthTexture` is
   attached at `GL_DEPTH_ATTACHMENT`.
4. Every flag is now clean. The next `Clean` returns almost immediately: two
   `bool` checks.

> **Why:** *three small fixes to OpenGlobe's `Clean`.* The logic is
> `FramebufferGL3x.Clean`'s, with three GL-correctness fixes that the C#
> doesn't have:
>
> - **The `glDrawBuffers` count.** OpenGlobe passes an array with one entry
>   per color attachment point (`GL_MAX_COLOR_ATTACHMENTS`). `glDrawBuffers`
>   accepts at most `GL_MAX_DRAW_BUFFERS` entries, a separate limit that the
>   GL 3.3 specification also sets to at least 8, but it can be smaller than
>   the attachment count. Passing only the entries up to the last attached
>   index is always valid. Trailing `GL_NONE` entries mean the same thing as
>   absent ones. (Attaching at an index at or above `GL_MAX_DRAW_BUFFERS`
>   still can't work. In practice both limits are 8 on current hardware.)
> - **The read buffer.** A new FBO's read buffer is `GL_COLOR_ATTACHMENT0`.
>   GL 3.3 marks the FBO incomplete (`INCOMPLETE_READ_BUFFER`) if that point
>   is empty. That happens with a depth-only framebuffer, as in a shadow map,
>   or when only attachment 1 is used. OpenGlobe never hit this because its
>   framebuffers always have attachment 0. Setting the read buffer to the
>   first attached point, or `GL_NONE`, keeps the FBO complete.
> - **The depth/stencil precedence.** OpenGlobe attaches the depth texture
>   and *then* the depth/stencil texture, relying on the second to overwrite
>   the first. That works when both change in the same `Clean`. But if you
>   later change only the depth attachment, the C# overwrites the depth half
>   of the depth/stencil attachment and leaves its stencil half, mixing two
>   textures. And removing the depth/stencil attachment empties the depth
>   point even when a depth texture is still set. The version above keeps
>   what the getters report and what GL has in agreement: the depth/stencil
>   texture wins while it's set, and the depth texture comes back when it's
>   removed.
>
> None of these changes affect the public interface.

> **C++ note — `auto&` when modifying elements in place:**
> `Attachment& attachment = m_colorAttachments[i];` binds a reference, so
> `attachment.dirty = false` changes the element in the vector. Writing
> `Attachment attachment = …` or `auto attachment = …` would copy the
> element, clear the copy's flag, and leave the vector dirty forever. The
> framebuffer would then be reattached on every draw. C# gets this right
> automatically here only because `_colorAttachments[i].Dirty = false`
> assigns through the array element. In C++, the `&` is what decides
> "modify in place" versus "modify a copy". The same applies to range-for:
> `for (auto& attachment : m_colorAttachments)`.

> **C++ note — `std::string_view` for fixed messages:** `StatusMessage`
> returns a `std::string_view` of a string literal. Literals live for the
> whole program, so the view can never dangle, and nothing is allocated.
> [Step 2](02-shaders.md) covers `string_view` in depth.

> **C++ note — switching on a `GLenum`:** `GLenum` is a plain
> `unsigned int`, and the `GL_*` names are macros. So this `switch` has a
> `default:`, unlike the `switch` statements on arda's own `enum class`es
> ([Step 1](01-state-management.md)). The compiler can't warn about a
> missing case of a type that isn't an enumeration, and GL may return values
> that aren't in the list. Two cases with the same value would not compile,
> which protects against copy-paste mistakes.

> **OpenGL note — `GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS` doesn't exist in GL
> 3.3:** you'll see it in OpenGL ES and in old `EXT_framebuffer_object`
> tutorials. Desktop GL 3.0 dropped the "all attachments must be the same
> size" rule: attachments may have different sizes, and rendering is limited
> to the area they all cover. A glad loader generated for GL 3.3 core doesn't
> define the constant, so don't add a case for it. D3D11 is stricter about
> mismatched sizes (see the D3D11 check), so keep attachments the same size
> anyway.

---

## Step 6.5: `ContextGL3x`

### `src/gl/ContextGL3x.h` (complete after Step 6)

The whole header as built by Steps 0 to 6. The Step 6 lines are marked.

```cpp
#pragma once

#include <arda/renderer/Context.h>
#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/renderstate/RenderState.h>

#include <glad/glad.h>

#include <memory>
#include <optional>

struct GLFWwindow;

namespace arda::renderer::gl {

class ContextGL3x final : public Context {
public:
    ContextGL3x(Device& device, GLFWwindow* window, int width, int height);

    void MakeCurrent() override;

protected:
    void DoClear(const ClearState& clearState) override;
    void DoSetViewport(const Rectangle& viewport) override;

    std::shared_ptr<VertexArray> DoCreateVertexArray() override;
    std::shared_ptr<Framebuffer> DoCreateFramebuffer() override;   // Step 6

    void DoDraw(core::geometry::PrimitiveType primitiveType,
                const DrawState& drawState, const SceneState& sceneState) override;
    void DoDrawRange(core::geometry::PrimitiveType primitiveType, int offset, int count,
                     const DrawState& drawState, const SceneState& sceneState) override;

private:
    // Step 1: render state (3.3)
    static void ForceApplyRenderState(const RenderState& renderState);
    static void ForceApplyRenderStateStencil(GLenum face, const StencilTestFace& test);

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

    // Step 3: drawing (3.5)
    void ApplyBeforeDraw(const DrawState& drawState, const SceneState& sceneState);
    void ApplyVertexArray(VertexArray& vertexArray);
    void ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState);
    void ApplyPrimitiveRestartIndex(IndexBufferDatatype datatype);

    // Step 5: texture units (3.6)
    void CleanTextureUnits();
    void BindTextureUnit(int index);

    // Step 6: binds GetFramebuffer() (or the window's framebuffer) and cleans it (3.7.1).
    void ApplyFramebuffer();

    GLFWwindow* m_window;

    // Cached copy of the GL state. It must always match GL (3.3.3).
    RenderState m_renderState;
    Color m_clearColor;             // GL defaults: (0, 0, 0, 0), depth 1, stencil 0
    float m_clearDepth = 1.0f;
    int m_clearStencil = 0;

    // weak_ptr, not a raw pointer: if the object is destroyed and a new one
    // is created at the same address, the new one must still be bound.
    std::weak_ptr<ShaderProgram> m_boundShaderProgram;
    std::optional<IndexBufferDatatype> m_primitiveRestartDatatype;
    std::weak_ptr<Framebuffer> m_boundFramebuffer;   // Step 6: expired or null = the window's framebuffer
};

} // namespace arda::renderer::gl
```

OpenGlobe keeps two fields: `_setFramebuffer`, what the user asked for, and
`_boundFramebuffer`, what GL has bound. In this port the first one is the
base class's `m_framebuffer`, read through `GetFramebuffer()`. Only the
second is GL-specific, so only it lives in `ContextGL3x`.

> **C++ note — `std::weak_ptr` for "what GL has bound":** a reminder from
> [Step 4](04-automatic-uniforms.md). A `weak_ptr` observes an object without
> keeping it alive. `lock()` returns a `shared_ptr`, which is null if the
> object is gone. `m_boundFramebuffer` must not keep the last framebuffer
> alive. It is only a cache of GL state. It also can't be a raw pointer: if
> the framebuffer were destroyed and a new one allocated at the same
> address, a raw pointer comparison would say "already bound", and the new
> FBO would never be bound (the ABA problem). An expired `weak_ptr` locks to
> null instead, so the comparison stays correct.

### `src/gl/ContextGL3x.cpp` additions

`ContextGL3x.cpp` is long by now, so this section shows only what changes.
Everything else stays as Steps 1, 3 and 5 left it.

**1. Includes.** Add these at the top, next to the existing
`gl/shaders/ShaderProgramGL3x.h` and `gl/vertexarray/VertexArrayGL3x.h`
includes. `Device.h` is probably already there for `DoCreateVertexArray`.

```cpp
#include "gl/framebuffer/FramebufferGL3x.h"

#include <arda/renderer/Device.h>

#include <format>
#include <stdexcept>
```

**2. `DoCreateFramebuffer`.** Put it next to `DoCreateVertexArray`. It ports
`ContextGL3x.CreateFramebuffer`, plus the `MakeCurrent()` call that
`DoCreateVertexArray` already makes. OpenGlobe doesn't make that call and
relies on the caller to have the right context current.

```cpp
std::shared_ptr<Framebuffer> ContextGL3x::DoCreateFramebuffer() {
    MakeCurrent();   // the GL framebuffer object belongs to this context
    return std::make_shared<FramebufferGL3x>(GetDevice().Limits().maximumNumberOfColorAttachments);
}
```

**3. `ApplyFramebuffer`.** Put it after `ApplyShaderProgram`. It ports
`ContextGL3x.ApplyFramebuffer` (lines 581–610) line for line.

```cpp
void ContextGL3x::ApplyFramebuffer() {
    const std::shared_ptr<Framebuffer>& framebuffer = GetFramebuffer();

    // Bind only when the requested target differs from what GL has bound.
    if (m_boundFramebuffer.lock() != framebuffer) {
        if (framebuffer) {
            static_cast<FramebufferGL3x&>(*framebuffer).Bind();
        } else {
            FramebufferGL3x::Unbind();   // restore the window's framebuffer
        }
        m_boundFramebuffer = framebuffer;
    }

    if (framebuffer) {
        // Clean every time: attachments may have changed since the last draw,
        // even if the same framebuffer is still bound.
        static_cast<FramebufferGL3x&>(*framebuffer).Clean();

#ifndef NDEBUG
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(std::format("Framebuffer is incomplete: {} (status 0x{:04X}).",
                                                 FramebufferGL3x::StatusMessage(status), status));
        }
#endif
    }
}
```

**4. `DoClear`.** Replace Step 1's version with this one. The only new line
is the first one.

```cpp
void ContextGL3x::DoClear(const ClearState& clearState) {
    // A clear writes to the framebuffer too, so apply it first (ContextGL3x.cs line 154).
    ApplyFramebuffer();

    // Clears ignore the scissor test and write masks, matching Direct3D 11.
    ApplyScissorTest(ScissorTest{});   // disabled
    ApplyColorMask(ColorMask{});       // all channels on
    ApplyDepthMask(true);
    // The stencil write mask is never changed from GL's default (all ones).

    if (m_clearColor != clearState.color) {
        const Color& c = clearState.color;
        glClearColor(c.red, c.green, c.blue, c.alpha);
        m_clearColor = c;
    }
    if (m_clearDepth != clearState.depth) {
        glClearDepth(clearState.depth);
        m_clearDepth = clearState.depth;
    }
    if (m_clearStencil != clearState.stencil) {
        glClearStencil(clearState.stencil);
        m_clearStencil = clearState.stencil;
    }

    // With a framebuffer bound, this clears every color attachment named in its
    // draw buffers, plus its depth and stencil attachments.
    glClear(ToGL(clearState.buffers));
}
```

**5. `ApplyBeforeDraw`.** Replace Step 5's version. `ApplyFramebuffer()` is
last, as in `ContextGL3x.cs` lines 524–532.

```cpp
void ContextGL3x::ApplyBeforeDraw(const DrawState& drawState, const SceneState& sceneState) {
    ApplyRenderState(drawState.renderState);
    ApplyVertexArray(*drawState.vertexArray);
    ApplyShaderProgram(drawState, sceneState);
    CleanTextureUnits();
    ApplyFramebuffer();   // Step 6
}
```

`DoDraw` and `DoDrawRange` already call `ApplyBeforeDraw`, so they need no
changes.

**6. `DoSetViewport` doesn't change.** It still calls `glViewport`
immediately. The viewport is context state in GL, so it doesn't need to wait
for the framebuffer to be bound. What matters is that both are right by the
time `glClear` or `glDraw*` runs, and they are. Keeping the viewport in step
with the render target is the application's job (see the milestone). The
renderer doesn't guess, as in OpenGlobe.

Some details are worth understanding.

- **Why both `Clear` and `Draw` call `ApplyFramebuffer`.** `SetFramebuffer`
  only records the request. Nothing reaches GL until the next operation that
  writes pixels. If only `Draw` applied it, this sequence would clear the
  *window* and then draw into the *texture*:
  ```cpp
  context.SetFramebuffer(framebuffer);
  context.Clear(clearState);    // must clear the texture
  context.Draw(...);            // must draw into the texture
  ```
- **Restoring the default framebuffer is the same code path.**
  `SetFramebuffer(nullptr)` makes `GetFramebuffer()` null, the cached
  binding differs, and `Unbind()` binds name 0. There is no separate
  "restore" function to forget to call.
- **The binding is cached, cleaning isn't.** Binding is skipped when nothing
  changed, but `Clean` runs every time a framebuffer is set, because the
  application may have changed an attachment of the *same* framebuffer
  between draws. The dirty flags make that nearly free.
- **A framebuffer destroyed while bound.** Deleting a bound FBO makes GL
  fall back to binding 0. Could that happen here? The context's
  `m_framebuffer` keeps the *set* framebuffer alive, so only one that was set,
  bound by a draw, then replaced with `SetFramebuffer` and released before
  the next draw can be destroyed while bound. Then `m_boundFramebuffer` has
  expired and `lock()` returns null. If the new request is also null, nothing
  is bound, and GL is already at 0, so the cache still matches GL. If the new
  request is another framebuffer, it gets bound.
- **Debug-only completeness check.** `glCheckFramebufferStatus` makes the
  driver validate the FBO, which isn't free, and a framebuffer that was
  complete once stays complete until its attachments change. OpenGlobe
  checks only in `DEBUG` builds, and `#ifndef NDEBUG` is the C++ equivalent.
  In a release build an incomplete framebuffer silently draws nothing. If a
  render target is black only in release builds, run the debug build.

> **C++ note — `#ifndef NDEBUG`:** `NDEBUG` ("no debug") is the macro that
> turns off `assert`. CMake defines it for `Release` and `RelWithDebInfo`
> builds, and not for `Debug`. Code inside `#ifndef NDEBUG … #endif` is
> therefore compiled only into debug builds, like C#'s `#if DEBUG`. Keep
> such blocks free of side effects the program depends on. Here the block
> only checks and throws.

> **C++ note — `std::format`:** `std::format` (C++20, `<format>`) builds a
> `std::string` from a format string with `{}` placeholders, like C#'s
> `string.Format` or interpolated strings. `{:04X}` formats an integer as
> upper-case hexadecimal padded to four digits, so status `0x8CD6` prints as
> `0x8CD6`. Format strings are checked at compile time: a placeholder that
> doesn't fit its argument's type is a compile error. [Step 2](02-shaders.md)
> uses it to build the shader prelude.

> **OpenGL note — the order in `ApplyBeforeDraw` doesn't matter to GL:** GL
> reads all of this state when the draw call executes, not when each piece is
> set. `ApplyFramebuffer` could just as well come first. It's last only
> because OpenGlobe has it last, and keeping the order makes the C# easy to
> compare with.

### Checkpoint: build and run

Add the GL files to CMake:

```cmake
if(ARDA_RENDERER_GL)
    target_sources(arda_renderer PRIVATE
        # ...existing GL sources...
        src/gl/framebuffer/FramebufferGL3x.cpp
        src/gl/framebuffer/FramebufferGL3x.h
    )
    # ...
endif()
```

Build and run `arda_tests` and your Step 5 program. Nothing should change:
no framebuffer is set, so `ApplyFramebuffer` binds nothing. The first call
compares an empty `weak_ptr` with a null `shared_ptr`, finds them equal, and
returns. If anything *does* change, `DoClear` or `ApplyBeforeDraw` has lost a
line.

---

## Step 6.6: Tests: `tests/src/renderer/FramebufferTests.cpp`

Framebuffers are created through a context, so these tests use a hidden
window, like OpenGlobe's `FramebufferTests.cs`. The first group ports those
C# tests. The rest render into textures and read the pixels back with
`Texture2D::CopyToBuffer` and `ReadPixelBuffer::CopyToSystemMemory` from
[Step 5](05-textures.md), as OpenGlobe's `TestUtility.ValidateColor` does.

What the tests cover:

| Test | What it proves |
|---|---|
| A new framebuffer is empty | The collection is sized from the device limit, and every slot starts null |
| Attachments check the texture format | The renderability checks and exception types |
| The color attachment count tracks set and removed textures | `NumberOfColorAttachments` (C#'s `EnumerateColorAttachments`) |
| Depth and depth/stencil attachments can be set and removed | The two depth setters (C#'s `DepthAttachment` and `DepthStencilAttachment`) |
| Clearing a framebuffer writes to its texture, not the window | `DoClear` applies the framebuffer, and `SetFramebuffer(nullptr)` restores the window |
| Rendering into a texture puts row 0 at the bottom | Drawing, the viewport, and GL's render-to-texture orientation |
| The depth test needs a depth attachment | The `VerifyDraw` check, and that the depth attachment really is attached |
| Fragment output locations select color attachments | `glDrawBuffers`, multiple render targets, detaching, and the read-buffer fix |
| An incomplete framebuffer is reported | The debug completeness check and its message |

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Indices.h>
#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/framebuffer/Framebuffer.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/textures/Texture2D.h>

#include <cstdint>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace arda::renderer;
using namespace arda::core::geometry;
using arda::core::Vector4;

namespace {

// A device and a hidden window. Members are initialized in declaration order and
// destroyed in reverse, so the window is destroyed before the device.
struct TestWindow {
    std::unique_ptr<Device> device = CreateDevice(GraphicsApi::OpenGL33);
    std::unique_ptr<GraphicsWindow> window = device->CreateGraphicsWindow(16, 16, "test", WindowType::Hidden);
    Context& context = window->GetContext();
};

// Draws a rectangle whose corners are already in clip space.
// u_depth sets the clip-space z of every vertex; u_color is the output color.
constexpr std::string_view SolidVertexShader = R"(
    layout(location = og_positionVertexLocation) in vec2 position;
    uniform float u_depth;
    void main()
    {
        gl_Position = vec4(position, u_depth, 1.0);
    })";

constexpr std::string_view SolidFragmentShader = R"(
    out vec4 fragmentColor;
    uniform vec4 u_color;
    void main()
    {
        fragmentColor = u_color;
    })";

// Two outputs at explicit locations, for the multiple render target test.
constexpr std::string_view TwoOutputFragmentShader = R"(
    layout(location = 0) out vec4 color0;
    layout(location = 1) out vec4 color1;
    void main()
    {
        color0 = vec4(1.0, 0.0, 0.0, 1.0);
        color1 = vec4(0.0, 1.0, 0.0, 1.0);
    })";

// A rectangle spanning the full width, from y = bottom to y = top in clip space.
std::shared_ptr<VertexArray> CreateRectangle(Context& context, const ShaderProgram& program,
                                             float bottom, float top) {
    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector2>("position", 4).Values();
    positions.emplace_back(-1.0f, bottom);
    positions.emplace_back( 1.0f, bottom);
    positions.emplace_back( 1.0f, top);
    positions.emplace_back(-1.0f, top);

    auto indices = std::make_unique<IndicesUnsignedShort>(6);
    indices->AddTriangle(0, 1, 2);
    indices->AddTriangle(0, 2, 3);
    mesh.indices = std::move(indices);

    return context.CreateVertexArray(mesh, program.VertexAttributes(), BufferHint::StaticDraw);
}

std::shared_ptr<Texture2D> CreateColorTexture(Device& device, int width, int height) {
    return device.CreateTexture2D(Texture2DDescription{width, height, TextureFormat::RedGreenBlueAlpha8, false});
}

struct Rgba {
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 0;
    bool operator==(const Rgba&) const = default;
};

// Lets doctest print the values when a CHECK fails.
std::ostream& operator<<(std::ostream& stream, const Rgba& c) {
    return stream << "(" << c.red << ", " << c.green << ", " << c.blue << ", " << c.alpha << ")";
}

constexpr Rgba Black{0, 0, 0, 255};
constexpr Rgba Red{255, 0, 0, 255};
constexpr Rgba Green{0, 255, 0, 255};

// Reads an RGBA8 texture back to system memory (TestUtility.ValidateColor).
// Row 0 of the result is the bottom row of the texture.
std::vector<std::uint8_t> ReadPixels(const Texture2D& texture) {
    return texture.CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1)
        ->CopyToSystemMemory<std::uint8_t>();
}

Rgba PixelAt(const std::vector<std::uint8_t>& pixels, int width, int x, int y) {
    const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 4;
    return {pixels.at(i), pixels.at(i + 1), pixels.at(i + 2), pixels.at(i + 3)};
}

} // namespace

// ---------------------------------------------------------------------------
// Attachments (ports of OpenGlobe's FramebufferTests.cs)
// ---------------------------------------------------------------------------

TEST_CASE("A new framebuffer is empty") {
    TestWindow test;
    auto framebuffer = test.context.CreateFramebuffer();

    CHECK(framebuffer->MaximumNumberOfColorAttachments() == test.device->Limits().maximumNumberOfColorAttachments);
    CHECK(framebuffer->NumberOfColorAttachments() == 0);
    CHECK(framebuffer->ColorAttachment(0) == nullptr);
    CHECK_FALSE(framebuffer->DepthAttachment());
    CHECK_FALSE(framebuffer->DepthStencilAttachment());
    CHECK(test.context.GetFramebuffer() == nullptr);
}

TEST_CASE("Framebuffer attachments check the texture format") {
    TestWindow test;
    auto color = CreateColorTexture(*test.device, 16, 16);
    auto depth = test.device->CreateTexture2D({16, 16, TextureFormat::Depth32f});
    auto depthStencil = test.device->CreateTexture2D({16, 16, TextureFormat::Depth24Stencil8});

    auto framebuffer = test.context.CreateFramebuffer();
    CHECK_THROWS_AS(framebuffer->SetColorAttachment(0, depth), std::invalid_argument);
    CHECK_THROWS_AS(framebuffer->SetDepthAttachment(color), std::invalid_argument);
    CHECK_THROWS_AS(framebuffer->SetDepthStencilAttachment(depth), std::invalid_argument);   // no stencil bits
    CHECK_THROWS_AS(framebuffer->SetColorAttachment(-1, color), std::out_of_range);
    CHECK_THROWS_AS(framebuffer->SetColorAttachment(framebuffer->MaximumNumberOfColorAttachments(), color),
                    std::out_of_range);

    // A failed set leaves the framebuffer unchanged.
    CHECK(framebuffer->NumberOfColorAttachments() == 0);

    framebuffer->SetColorAttachment(0, color);
    framebuffer->SetDepthAttachment(depth);
    CHECK(framebuffer->ColorAttachment(0) == color);
    CHECK(framebuffer->DepthAttachment() == depth);

    // A depth/stencil texture is also depth renderable.
    framebuffer->SetDepthAttachment(depthStencil);
    CHECK(framebuffer->DepthAttachment() == depthStencil);
}

TEST_CASE("The color attachment count tracks set and removed textures") {
    TestWindow test;
    auto color0 = CreateColorTexture(*test.device, 1, 1);
    auto color1 = CreateColorTexture(*test.device, 1, 1);
    auto color2 = CreateColorTexture(*test.device, 1, 1);

    auto framebuffer = test.context.CreateFramebuffer();
    framebuffer->SetColorAttachment(0, color0);
    framebuffer->SetColorAttachment(1, color1);
    framebuffer->SetColorAttachment(2, color2);
    CHECK(framebuffer->NumberOfColorAttachments() == 3);

    framebuffer->SetColorAttachment(1, nullptr);
    CHECK(framebuffer->NumberOfColorAttachments() == 2);
    CHECK_FALSE(framebuffer->ColorAttachment(1));

    framebuffer->SetColorAttachment(1, color1);
    CHECK(framebuffer->NumberOfColorAttachments() == 3);

    // Setting the same texture again, or null on an empty slot, changes nothing.
    framebuffer->SetColorAttachment(1, color1);
    framebuffer->SetColorAttachment(3, nullptr);
    CHECK(framebuffer->NumberOfColorAttachments() == 3);

    // Replacing one texture with another keeps the count.
    framebuffer->SetColorAttachment(2, color0);
    CHECK(framebuffer->NumberOfColorAttachments() == 3);
}

TEST_CASE("Depth and depth/stencil attachments can be set and removed") {
    TestWindow test;
    auto depth = test.device->CreateTexture2D({1, 1, TextureFormat::Depth24});
    auto depthStencil = test.device->CreateTexture2D({1, 1, TextureFormat::Depth32fStencil8});

    auto framebuffer = test.context.CreateFramebuffer();

    framebuffer->SetDepthAttachment(depth);
    CHECK(framebuffer->DepthAttachment() == depth);
    framebuffer->SetDepthAttachment(nullptr);
    CHECK_FALSE(framebuffer->DepthAttachment());

    framebuffer->SetDepthStencilAttachment(depthStencil);
    CHECK(framebuffer->DepthStencilAttachment() == depthStencil);
    framebuffer->SetDepthStencilAttachment(nullptr);
    CHECK_FALSE(framebuffer->DepthStencilAttachment());
}

// ---------------------------------------------------------------------------
// Rendering into textures
// ---------------------------------------------------------------------------

TEST_CASE("Clearing a framebuffer writes to its texture, not the window") {
    TestWindow test;
    auto color = CreateColorTexture(*test.device, 4, 4);
    auto framebuffer = test.context.CreateFramebuffer();
    framebuffer->SetColorAttachment(0, color);

    ClearState red;
    red.buffers = ClearBuffers::ColorBuffer;
    red.color = {1.0f, 0.0f, 0.0f, 1.0f};

    test.context.SetFramebuffer(framebuffer);
    test.context.SetViewport({0, 0, 4, 4});
    test.context.Clear(red);

    // Back to the window. This clear must not touch the texture.
    ClearState blue;
    blue.color = {0.0f, 0.0f, 1.0f, 1.0f};
    test.context.SetFramebuffer(nullptr);
    test.context.SetViewport({0, 0, test.window->Width(), test.window->Height()});
    test.context.Clear(blue);

    const auto pixels = ReadPixels(*color);
    REQUIRE(pixels.size() == 4 * 4 * 4);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            CHECK(PixelAt(pixels, 4, x, y) == Red);
        }
    }
}

TEST_CASE("Rendering into a texture puts row 0 at the bottom") {
    TestWindow test;
    auto color = CreateColorTexture(*test.device, 4, 4);
    auto framebuffer = test.context.CreateFramebuffer();
    framebuffer->SetColorAttachment(0, color);

    auto program = test.device->CreateShaderProgram(SolidVertexShader, SolidFragmentShader);
    program->Uniforms().Get<Vector4<float>>("u_color").SetValue(Vector4<float>(1.0f, 0.0f, 0.0f, 1.0f));

    DrawState drawState;
    drawState.renderState.facetCulling.enabled = false;
    drawState.renderState.depthTest.enabled = false;   // this framebuffer has no depth attachment
    drawState.shaderProgram = program;
    drawState.vertexArray = CreateRectangle(test.context, *program, -1.0f, 0.0f);   // bottom half

    ClearState black;
    black.color = {0.0f, 0.0f, 0.0f, 1.0f};

    test.context.SetFramebuffer(framebuffer);
    // The viewport must match the texture. Try removing this line: the window is
    // 16x16, so the bottom half covers 8 rows and fills the whole 4x4 texture.
    test.context.SetViewport({0, 0, 4, 4});
    test.context.Clear(black);
    test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{});

    const auto pixels = ReadPixels(*color);
    for (int x = 0; x < 4; ++x) {
        CHECK(PixelAt(pixels, 4, x, 0) == Red);     // bottom rows: the rectangle
        CHECK(PixelAt(pixels, 4, x, 1) == Red);
        CHECK(PixelAt(pixels, 4, x, 2) == Black);   // top rows: the clear color
        CHECK(PixelAt(pixels, 4, x, 3) == Black);
    }
}

TEST_CASE("The depth test needs a depth attachment") {
    TestWindow test;
    auto color = CreateColorTexture(*test.device, 4, 4);
    auto depth = test.device->CreateTexture2D({4, 4, TextureFormat::Depth32f});
    auto framebuffer = test.context.CreateFramebuffer();
    framebuffer->SetColorAttachment(0, color);

    auto program = test.device->CreateShaderProgram(SolidVertexShader, SolidFragmentShader);
    auto& colorUniform = program->Uniforms().Get<Vector4<float>>("u_color");
    auto& depthUniform = program->Uniforms().Get<float>("u_depth");

    DrawState drawState;                                // depth test enabled: RenderState's default
    drawState.renderState.facetCulling.enabled = false;
    drawState.shaderProgram = program;
    drawState.vertexArray = CreateRectangle(test.context, *program, -1.0f, 1.0f);   // fills the target

    test.context.SetFramebuffer(framebuffer);
    test.context.SetViewport({0, 0, 4, 4});

    SUBCASE("without one, Draw throws") {
        CHECK_THROWS_AS(test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{}),
                        std::invalid_argument);
    }

    SUBCASE("with one, nearer fragments win") {
        framebuffer->SetDepthAttachment(depth);
        test.context.Clear(ClearState{});   // color and depth (depth = 1)

        // Near and green first, then far and red. If the depth attachment were
        // missing, GL would skip the depth test and the red rectangle would win.
        depthUniform.SetValue(-0.5f);
        colorUniform.SetValue(Vector4<float>(0.0f, 1.0f, 0.0f, 1.0f));
        test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{});

        depthUniform.SetValue(0.5f);
        colorUniform.SetValue(Vector4<float>(1.0f, 0.0f, 0.0f, 1.0f));
        test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{});

        CHECK(PixelAt(ReadPixels(*color), 4, 1, 1) == Green);
    }

    SUBCASE("a depth/stencil attachment also satisfies the check") {
        framebuffer->SetDepthStencilAttachment(test.device->CreateTexture2D({4, 4, TextureFormat::Depth24Stencil8}));
        test.context.Clear(ClearState{});
        CHECK_NOTHROW(test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{}));
    }
}

TEST_CASE("Fragment output locations select color attachments") {
    TestWindow test;
    auto texture0 = CreateColorTexture(*test.device, 4, 4);
    auto texture1 = CreateColorTexture(*test.device, 4, 4);

    auto program = test.device->CreateShaderProgram(SolidVertexShader, TwoOutputFragmentShader);
    auto framebuffer = test.context.CreateFramebuffer();
    framebuffer->SetColorAttachment(program->FragmentOutputLocation("color0"), texture0);
    framebuffer->SetColorAttachment(program->FragmentOutputLocation("color1"), texture1);
    CHECK(framebuffer->ColorAttachment(0) == texture0);
    CHECK(framebuffer->ColorAttachment(1) == texture1);

    DrawState drawState;
    drawState.renderState.facetCulling.enabled = false;
    drawState.renderState.depthTest.enabled = false;
    drawState.shaderProgram = program;
    drawState.vertexArray = CreateRectangle(test.context, *program, -1.0f, 1.0f);

    ClearState black;
    black.color = {0.0f, 0.0f, 0.0f, 1.0f};

    test.context.SetFramebuffer(framebuffer);
    test.context.SetViewport({0, 0, 4, 4});
    test.context.Clear(black);
    test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{});

    CHECK(PixelAt(ReadPixels(*texture0), 4, 2, 2) == Red);     // location 0 -> attachment 0
    CHECK(PixelAt(ReadPixels(*texture1), 4, 2, 2) == Green);   // location 1 -> attachment 1

    // Detach attachment 0. Draw buffers become { GL_NONE, GL_COLOR_ATTACHMENT1 } and
    // the read buffer moves to attachment 1, so the framebuffer stays complete.
    framebuffer->SetColorAttachment(0, nullptr);
    test.context.Clear(black);   // clears texture1 only
    test.context.Draw(PrimitiveType::Triangles, drawState, SceneState{});

    CHECK(PixelAt(ReadPixels(*texture1), 4, 2, 2) == Green);   // drawn again after the clear
    CHECK(PixelAt(ReadPixels(*texture0), 4, 2, 2) == Red);     // untouched since it was detached
}

#ifndef NDEBUG
TEST_CASE("An incomplete framebuffer is reported in debug builds") {
    TestWindow test;
    auto framebuffer = test.context.CreateFramebuffer();   // nothing attached
    test.context.SetFramebuffer(framebuffer);

    // An empty FBO breaks several rules at once (missing attachment, and the default
    // draw and read buffers name an empty point). Which status GL reports is up to
    // the driver, so only the common part of the message is checked.
    CHECK_THROWS_WITH_AS(test.context.Clear(ClearState{}),
                         doctest::Contains("Framebuffer is incomplete"),
                         std::runtime_error);
}
#endif
```

A few notes on the test code.

- **`TestWindow`** makes the destruction order explicit. Members are
  initialized top to bottom and destroyed bottom to top, so the window (and
  its context) always goes before the device. Framebuffers and textures
  declared *after* `test` in a test case are destroyed before it, while the
  window's GL context is still alive and current. That matters because
  `glDeleteFramebuffers` must run in the FBO's own context.
- **The reads happen while the framebuffer is still set.** In GL, reading an
  attached texture with `glGetTexImage` is fine. GL finishes the earlier
  draw before the read, so the tests don't need to switch back to the window
  first.
- **`SceneState{}`** is passed because `Draw` requires one. None of these
  shaders use automatic uniforms.
- **`PixelAt`** is where the orientation test gets its meaning: index
  `(y * width + x) * 4` with `y = 0` is the first row `glGetTexImage`
  returns, which is the bottom row of the texture. Step 8's cross-API test
  relies on D3D11 producing the same layout.
- **`CHECK_THROWS_WITH_AS`** checks both the exception type and part of its
  `what()` text. `doctest::Contains` matches a substring, so the test doesn't
  depend on the exact wording. The whole test is inside `#ifndef NDEBUG`,
  because the check it tests only exists in debug builds.
- **`SUBCASE`**: doctest runs the test case once per `SUBCASE`, each time
  running the code outside the subcases again. Each subcase therefore gets a
  fresh window, framebuffer and draw state.

> **C++ note — member initializers that depend on each other:** in
> `TestWindow`, `window`'s initializer uses `device`, and `context`'s uses
> `window`. That is only safe because members are initialized in
> **declaration order**, not in the order a constructor's initializer list
> happens to name them. Reordering the three declarations would read
> `device` before it exists. Compilers warn (`-Wreorder`) when a constructor's
> initializer list is written in a different order from the declarations.
> A reference member (`Context& context`) must be initialized and can't be
> reseated, which also makes the struct non-assignable. That's fine for a
> test fixture.

### CMake for the tests

```cmake
add_executable(arda_tests
    src/main.cpp
    # ...existing test files...
    src/renderer/FramebufferTests.cpp
)
```

### Checkpoint: run the tests

Build and run `arda_tests`. If the render tests fail:

- **Everything reads as zero (transparent black):** the draw went somewhere
  else. Check that `DoClear` and `ApplyBeforeDraw` both call
  `ApplyFramebuffer()`.
- **The orientation test sees red in every row:** the viewport wasn't set to
  4×4.
- **The depth test sees red:** the depth attachment wasn't attached. Look at
  the depth block in `Clean`.
- **The detach part of the MRT test throws `INCOMPLETE_READ_BUFFER` or
  `INCOMPLETE_DRAW_BUFFER`:** check the `glReadBuffer` call and the
  `drawBuffers` loop.
- **An exception mentions `GL_FRAMEBUFFER_UNSUPPORTED`:** your driver rejects
  a format. The tests use RGBA8 and `Depth32f`/`Depth24Stencil8`, which GL
  3.3 requires drivers to support.

---

## Step 6.7: Milestone: render to a texture, then draw it on screen ("Try This" in 3.8)

**Pass 1** draws the book's triangle into a 512×512 texture. **Pass 2**
draws a fullscreen quad into the window that samples that texture. You should
see the red triangle on a dark gray square (the texture's clear color),
stretched to fill the window.

### A scope guard for the render target

Pass 1 has to set the framebuffer *and* a matching viewport, and pass 2 has
to get the window's back. OpenGlobe's `HighResolutionSnap` does this by hand
in `PreRenderFrame` and `PostRenderFrame`. In C++, a small RAII class can do
it: it sets both in its constructor and restores both in its destructor.

> **C++ note — RAII scope guards:** [Step 0](00-setup.md) introduced RAII for
> owning resources: a constructor acquires, the destructor releases. A
> **scope guard** applies the same idea to *temporary state*: the constructor
> changes something, and the destructor changes it back when the enclosing
> scope ends. The scope can end normally, through `return`, or through an
> exception. Compare C#:
>
> ```csharp
> var previous = context.Framebuffer;
> context.Framebuffer = framebuffer;
> try { /* render */ } finally { context.Framebuffer = previous; }
> ```
>
> In C++ the `finally` block is the destructor, and it can't be forgotten:
>
> ```cpp
> {
>     ScopedRenderTarget target(context, framebuffer, {0, 0, 512, 512});
>     // render; even if Draw throws, the window's target is restored
> }   // <- destructor runs here
> ```
>
> Other scope guards you'll meet are `std::lock_guard` (lock a mutex, unlock
> on exit), which `GlfwLibrary` uses, and `std::scoped_lock`.
>
> Two rules for writing one. The destructor must not throw: destructors are
> `noexcept` by default, and a throw from one calls `std::terminate`.
> `ScopedRenderTarget` restores values that were valid when it saved them,
> so its `SetViewport` call can't fail. And the class must not be copyable,
> or two objects would both restore the state.

> **C++ note — `[[nodiscard]]` on a class:** a classic scope-guard bug is
> forgetting the variable name:
>
> ```cpp
> ScopedRenderTarget(context, framebuffer, viewport);   // a temporary: constructed and destroyed on this line
> ```
>
> This compiles, and restores the state immediately, so the guard does
> nothing. Marking the class `[[nodiscard]]` makes C++20 compilers warn when
> an object of that type is created and discarded like this.

> **Why:** *the scope guard lives in the application, not the renderer.*
> The book keeps render state out of global stacks (3.3). Every draw says
> what it needs, and the context only caches it. A renderer-level
> "push/pop render target" stack would be exactly that kind of hidden global
> state. A guard in application code keeps the save and restore visible
> where the render pass is written.

### `scene/src/main.cpp`

This is the whole program. It needs no GL includes: everything goes through
the public renderer types.

```cpp
#include <arda/core/geometry/Indices.h>
#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/framebuffer/Framebuffer.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/textures/Texture2D.h>
#include <arda/renderer/textures/TextureSampler.h>

#include <cstdio>
#include <exception>
#include <memory>
#include <utility>

using namespace arda::renderer;
using namespace arda::core::geometry;
using arda::core::Vector3;

namespace {

// Sets a render target (framebuffer and viewport) for the lifetime of the object,
// then restores the previous one (HighResolutionSnap's PreRenderFrame/PostRenderFrame).
class [[nodiscard]] ScopedRenderTarget {
public:
    ScopedRenderTarget(Context& context, std::shared_ptr<Framebuffer> framebuffer, const Rectangle& viewport)
        : m_context(context),
          m_previousFramebuffer(context.GetFramebuffer()),
          m_previousViewport(context.GetViewport()) {
        m_context.SetFramebuffer(std::move(framebuffer));
        m_context.SetViewport(viewport);
    }

    ~ScopedRenderTarget() {
        m_context.SetViewport(m_previousViewport);
        m_context.SetFramebuffer(std::move(m_previousFramebuffer));
    }

    ScopedRenderTarget(const ScopedRenderTarget&)            = delete;
    ScopedRenderTarget& operator=(const ScopedRenderTarget&) = delete;

private:
    Context& m_context;
    std::shared_ptr<Framebuffer> m_previousFramebuffer;
    Rectangle m_previousViewport;
};

// The book's triangle (Listing 3.30): an isosceles right triangle in the xz plane.
Mesh CreateTriangleMesh() {
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

// Two triangles covering clip space from (-1, -1) to (1, 1), counterclockwise.
Mesh CreateFullscreenQuadMesh() {
    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector2>("position", 4).Values();
    positions.emplace_back(-1.0f, -1.0f);
    positions.emplace_back( 1.0f, -1.0f);
    positions.emplace_back( 1.0f,  1.0f);
    positions.emplace_back(-1.0f,  1.0f);

    auto indices = std::make_unique<IndicesUnsignedShort>(6);
    indices->AddTriangle(0, 1, 2);
    indices->AddTriangle(0, 2, 3);
    mesh.indices = std::move(indices);
    return mesh;
}

} // namespace

int main() {
    try {
        // Declared first so it is destroyed last.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(800, 600, "Step 6: Render to texture");
        Context& context = window->GetContext();

        // --- Pass 1: the triangle, drawn into a texture ---------------------

        auto triangleProgram = device->CreateShaderProgram(
            R"(layout(location = og_positionVertexLocation) in vec4 position;
               uniform mat4 og_modelViewPerspectiveMatrix;
               void main()
               {
                   gl_Position = og_modelViewPerspectiveMatrix * position;
               })",
            R"(out vec3 fragmentColor;
               uniform vec3 u_color;
               void main()
               {
                   fragmentColor = u_color;
               })");
        triangleProgram->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));

        DrawState triangleState;
        triangleState.renderState.facetCulling.enabled = false;
        // The depth test stays enabled (RenderState's default), so the framebuffer needs a depth attachment.
        triangleState.shaderProgram = triangleProgram;
        triangleState.vertexArray = context.CreateVertexArray(
            CreateTriangleMesh(), triangleProgram->VertexAttributes(), BufferHint::StaticDraw);

        // The render target. RGBA8 rather than RGB8: GL 3.3 requires drivers to
        // render to RGBA8, but not to RGB8. No mipmaps (see "Mipmaps go stale").
        constexpr int size = 512;
        auto colorTexture = device->CreateTexture2D(
            Texture2DDescription{size, size, TextureFormat::RedGreenBlueAlpha8, false});
        auto depthTexture = device->CreateTexture2D(
            Texture2DDescription{size, size, TextureFormat::Depth32f, false});

        auto framebuffer = context.CreateFramebuffer();
        framebuffer->SetColorAttachment(triangleProgram->FragmentOutputLocation("fragmentColor"), colorTexture);
        framebuffer->SetDepthAttachment(depthTexture);

        SceneState sceneState;
        sceneState.camera.ZoomToTarget(1.0);
        sceneState.camera.aspectRatio = 1.0;   // the render target is square, whatever the window's shape

        ClearState textureClear;
        textureClear.color = {0.2f, 0.2f, 0.2f, 1.0f};

        // --- Pass 2: a fullscreen quad that samples the texture --------------

        // The positions are already in clip space, so no matrices are needed.
        auto quadProgram = device->CreateShaderProgram(
            R"(layout(location = og_positionVertexLocation) in vec2 position;
               out vec2 fsTextureCoordinate;
               void main()
               {
                   fsTextureCoordinate = position * 0.5 + 0.5;   // [-1, 1] -> [0, 1]
                   gl_Position = vec4(position, 0.0, 1.0);
               })",
            R"(in vec2 fsTextureCoordinate;
               out vec3 fragmentColor;
               uniform sampler2D og_texture0;
               void main()
               {
                   fragmentColor = texture(og_texture0, fsTextureCoordinate).rgb;
               })");

        DrawState quadState;
        quadState.renderState.depthTest.enabled = false;
        quadState.shaderProgram = quadProgram;
        quadState.vertexArray = context.CreateVertexArray(
            CreateFullscreenQuadMesh(), quadProgram->VertexAttributes(), BufferHint::StaticDraw);

        ClearState windowClear;
        windowClear.color = {0.02f, 0.05f, 0.12f, 1.0f};

        // --- Frame handlers --------------------------------------------------

        window->SetResizeHandler([&] {
            // The window's viewport. Pass 1 changes it temporarily and restores it.
            context.SetViewport({0, 0, window->Width(), window->Height()});
        });

        bool saved = false;
        window->SetRenderFrameHandler([&] {
            TextureUnit& unit0 = context.GetTextureUnits()[0];

            // Pass 1: into the texture.
            {
                ScopedRenderTarget target(context, framebuffer, Rectangle{0, 0, size, size});

                // No texture unit the program can sample may hold a texture we are
                // rendering into (the feedback loop). Pass 2 of the previous frame
                // left colorTexture on unit 0.
                unit0.SetTexture(nullptr);

                context.Clear(textureClear);
                context.Draw(PrimitiveType::Triangles, triangleState, sceneState);
            }   // the window's framebuffer and viewport are restored here

            // Optional: look at what pass 1 produced.
            if (!saved) {
                colorTexture->Save("render-to-texture.png");
                saved = true;
            }

            // Pass 2: to the window, sampling the texture.
            unit0.SetTexture(colorTexture);
            unit0.SetSampler(device->Samplers().linearClamp);
            context.Clear(windowClear);
            context.Draw(PrimitiveType::Triangles, quadState, sceneState);
        });

        window->Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

Walk through one frame and match each line to what GL sees.

1. **`ScopedRenderTarget`'s constructor** saves `nullptr` and the window's
   viewport, then records the framebuffer and calls `glViewport(0, 0, 512,
   512)`. The framebuffer is not bound yet.
2. **`unit0.SetTexture(nullptr)`** marks unit 0 dirty. The triangle's
   program has no sampler, so this frame would work without it. But the
   habit prevents a feedback loop the day the triangle becomes textured.
   Change it to that texture then.
3. **`Clear`** runs `ApplyFramebuffer`, which binds the FBO. On the first
   frame it also runs `Clean`: the color and depth attachments,
   `glDrawBuffers`, `glReadBuffer`, and the completeness check. Then
   `glClear` fills the color texture with gray and the depth texture with 1.
4. **`Draw`** applies the render state, vertex array and program, unbinds
   unit 0, sees the framebuffer is already bound and clean, and draws. The
   triangle's `fragmentColor` is location 0, so it lands in `colorTexture`.
   Its depth goes to `depthTexture`.
5. **The guard's destructor** restores the 800×600 viewport and a null
   framebuffer. Still nothing is bound. GL has the FBO bound until the next
   clear or draw.
6. **`Save`** (first frame only) reads the texture back and writes a PNG
   next to the executable. It's the quickest way to check pass 1 on its own.
7. **Pass 2's `Clear`** runs `ApplyFramebuffer` again, sees null, and binds
   framebuffer 0. The window is cleared.
8. **Pass 2's `Draw`** binds `colorTexture` to unit 0 with the linear-clamp
   sampler. The quad samples it with `og_texture0` (which
   [Step 5](05-textures.md) sets to 0). `colorTexture` is no longer attached
   to the bound framebuffer, so there's no feedback loop.

> **Why:** *the camera's aspect ratio is 1, not the window's.* The texture
> is square, so the triangle is rendered with a square projection. Pass 2
> then stretches the square texture over the whole window, so the triangle
> looks wider in an 800×600 window. That's correct: it's what a texture
> mapped onto a non-square quad looks like. If you want it undistorted, draw
> the quad at the texture's aspect ratio instead. What would be *wrong* is
> leaving the window's aspect ratio (1.33) on the camera while rendering into
> the square texture. The triangle would be squeezed in the texture and
> stretched again on screen.

### Build and run

Build `arda_scene` and run it. You should see:

- a dark gray square filling the window, with the red triangle on it
- the same image in `render-to-texture.png`, the right way up
- when you resize the window, the image stretches with it and stays sharp at
  its center. The texture stays 512×512, so its texels are magnified with
  linear filtering.

If it goes wrong:

| Symptom | Likely cause |
|---|---|
| The window shows only the window clear color | Pass 2 draws the quad culled or clipped. Check the quad's winding (counterclockwise) and that its positions go straight to `gl_Position` |
| The quad is black | `colorTexture` is incomplete for sampling (a mipmap filter on a texture without mipmaps), or pass 1 drew nothing. Look at the saved PNG |
| The PNG is gray with no triangle | Pass 1's depth test failed everything (clear depth not 1?), or the camera isn't looking at the triangle |
| The triangle is cut off or off-center in the PNG | The viewport wasn't set to 512×512 for pass 1 |
| The triangle is squeezed | The camera's aspect ratio doesn't match the render target |
| `std::invalid_argument` about the depth test | The framebuffer has no depth attachment, and the triangle's depth test is enabled |
| `Framebuffer is incomplete: ...` | Read the message. It names the problem and a fix |

### Try this

- Remove `unit0.SetTexture(nullptr)` and make the triangle's fragment shader
  sample `og_texture0`. You've built a feedback loop. Depending on the
  driver you'll see garbage, flicker, or nothing wrong at all. That last case
  is why the habit matters.
- Change the texture to `TextureFormat::RedGreenBlue8`. On most desktop
  drivers it still works, but GL doesn't guarantee it. If yours reports
  `GL_FRAMEBUFFER_UNSUPPORTED`, you've seen why the milestone uses RGBA8.
- Replace the depth attachment with a `Depth24Stencil8` depth/stencil
  attachment. Nothing visible changes, and `VerifyDraw` accepts it.
- Save the depth texture too: `depthTexture->Save("depth.png")` writes it as
  grayscale ([Step 5](05-textures.md)). Near is dark, far is light.
- Render a second, smaller framebuffer (64×64) and display it with nearest
  filtering (`device->Samplers().nearestClamp`) to see the individual texels.

---

## Common pitfalls (summary)

| Pitfall | What happens | How this renderer handles it |
|---|---|---|
| Viewport not set to the render target's size | Image cropped or shifted | Your job. Use a scope guard like `ScopedRenderTarget` |
| Camera aspect ratio from the window | Image squeezed | Your job. Set it for the target |
| Sampling a texture that is attached to the bound framebuffer | Undefined results, no error | Your job. Clear the texture unit before rendering into the texture |
| No depth attachment with the depth test on | Silently no depth testing | `VerifyDraw` throws |
| Incomplete framebuffer | Draws silently do nothing | Debug builds throw with a readable message |
| Mipmapped render target | Lower levels show old content | Use `generateMipmaps = false` for now |
| RGB8 render target | May be `GL_FRAMEBUFFER_UNSUPPORTED` | Use RGBA8 |
| Depth-only framebuffer in GL 3.3 | `INCOMPLETE_READ_BUFFER` | `Clean` sets the read buffer |
| Using a framebuffer in a different window's context | `GL_INVALID_OPERATION` or the wrong FBO | Create framebuffers from the context that uses them |
| Destroying a framebuffer after its window | `glDeleteFramebuffers` with no (or the wrong) context current | Declare framebuffers after the window, so they're destroyed first |
| Forgetting that `SetFramebuffer` is lazy | Confusion when stepping through GL calls in a debugger | Nothing is bound until the next `Clear` or `Draw` |

---

## Complete CMake files after Step 6

The CMake sections above list only this step's additions. Here are the three
CMake files as they should look once Step 6 is done, with every step so far
folded into one list. If a build fails with an unresolved symbol or a missing
include after a step, compare your files against these first.

`core/CMakeLists.txt`:

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
    include/arda/core/Half.h
    include/arda/core/Math.h
    include/arda/core/Trig.h
    include/arda/core/geometry/ClipDepth.h
    include/arda/core/geometry/Ellipsoid.h
    include/arda/core/geometry/Indices.h
    include/arda/core/geometry/Matrix.h
    include/arda/core/geometry/Matrix4.h
    include/arda/core/geometry/Mesh.h
    include/arda/core/geometry/PrimitiveType.h
    include/arda/core/geometry/Vector2.h
    include/arda/core/geometry/Vector3.h
    include/arda/core/geometry/Vector4.h
    include/arda/core/geometry/VertexAttribute.h
    include/arda/core/geometry/WindingOrder.h
)
target_include_directories(arda_core PUBLIC include)
```

`renderer/CMakeLists.txt`:

```cmake
# Headers are listed for IDE project-tree visibility only; CMake compiles
# just the .cpp sources. Keep this list current when adding headers.
add_library(arda_renderer
    src/Context.cpp
    src/Device.cpp
    src/GlfwLibrary.cpp
    src/GraphicsWindow.cpp
    src/framebuffer/Framebuffer.cpp
    src/mesh/MeshBuffers.cpp
    src/scene/Camera.cpp
    src/scene/SceneState.cpp
    src/shaders/BuiltinConstants.cpp
    src/shaders/ShaderCache.cpp
    src/shaders/ShaderProgram.cpp
    src/shaders/UniformCollection.cpp
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.cpp
    src/textures/CreateTexture2D.cpp
    src/textures/Image.cpp
    src/textures/Texture2D.cpp
    src/textures/Texture2DDescription.cpp
    src/textures/TextureUnits.cpp
    src/textures/TextureUtility.cpp
    src/vertexarray/VertexArray.cpp
    include/arda/renderer/ClearState.h
    include/arda/renderer/Color.h
    include/arda/renderer/Context.h
    include/arda/renderer/Device.h
    include/arda/renderer/DrawState.h
    include/arda/renderer/Exceptions.h
    include/arda/renderer/GraphicsApi.h
    include/arda/renderer/GraphicsWindow.h
    include/arda/renderer/Rectangle.h
    include/arda/renderer/buffers/BufferHint.h
    include/arda/renderer/buffers/IndexBuffer.h
    include/arda/renderer/buffers/PixelBufferHint.h
    include/arda/renderer/buffers/ReadPixelBuffer.h
    include/arda/renderer/buffers/VertexBuffer.h
    include/arda/renderer/buffers/WritePixelBuffer.h
    include/arda/renderer/framebuffer/Framebuffer.h
    include/arda/renderer/mesh/MeshBuffers.h
    include/arda/renderer/renderstate/Blending.h
    include/arda/renderer/renderstate/ColorMask.h
    include/arda/renderer/renderstate/DepthRange.h
    include/arda/renderer/renderstate/DepthTest.h
    include/arda/renderer/renderstate/FacetCulling.h
    include/arda/renderer/renderstate/PrimitiveRestart.h
    include/arda/renderer/renderstate/RenderState.h
    include/arda/renderer/renderstate/ScissorTest.h
    include/arda/renderer/renderstate/StencilTest.h
    include/arda/renderer/scene/Camera.h
    include/arda/renderer/scene/SceneState.h
    include/arda/renderer/shaders/AutomaticUniforms.h
    include/arda/renderer/shaders/ShaderCache.h
    include/arda/renderer/shaders/ShaderProgram.h
    include/arda/renderer/shaders/ShaderVertexAttribute.h
    include/arda/renderer/shaders/Uniform.h
    include/arda/renderer/shaders/UniformCollection.h
    include/arda/renderer/textures/Image.h
    include/arda/renderer/textures/ImageFormat.h
    include/arda/renderer/textures/Texture2D.h
    include/arda/renderer/textures/Texture2DDescription.h
    include/arda/renderer/textures/TextureFormat.h
    include/arda/renderer/textures/TextureSampler.h
    include/arda/renderer/textures/TextureUnits.h
    include/arda/renderer/textures/TextureUtility.h
    include/arda/renderer/vertexarray/ComponentDatatype.h
    include/arda/renderer/vertexarray/VertexArray.h
    include/arda/renderer/vertexarray/VertexBufferAttribute.h
    include/arda/renderer/vertexarray/VertexLocations.h
    src/Cleanable.h
    src/GlfwLibrary.h
    src/shaders/BuiltinConstants.h
    src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.h
    src/shaders/automaticuniforms/TextureUniform.h
    src/shaders/automaticuniforms/Wgs84HeightUniform.h
)

# PUBLIC include: the public headers under include/arda/renderer.
# PRIVATE src: backend sources include their siblings as "gl/DeviceGL3x.h".
target_include_directories(arda_renderer PUBLIC include PRIVATE src)
# stb is only included from src/textures/Image.cpp (Step 5). SYSTEM hides its warnings.
target_include_directories(arda_renderer SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

# PUBLIC: renderer's headers use core's types, so consumers need core too.
# PRIVATE: glfw appears only in .cpp files, so consumers never see it.
target_link_libraries(arda_renderer PUBLIC arda_core PRIVATE glfw)

# Warnings, including a missing enumerator in a switch (Step 1).
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
        src/gl/buffers/BufferGL3x.cpp
        src/gl/framebuffer/FramebufferGL3x.cpp
        src/gl/shaders/GlslPrelude.cpp
        src/gl/shaders/ShaderObjectGL3x.cpp
        src/gl/shaders/ShaderProgramGL3x.cpp
        src/gl/shaders/UniformGL3x.cpp
        src/gl/textures/Texture2DGL3x.cpp
        src/gl/textures/TextureSamplerGL3x.cpp
        src/gl/vertexarray/VertexArrayGL3x.cpp
        src/gl/ContextGL3x.h
        src/gl/DeviceGL3x.h
        src/gl/GLHandle.h
        src/gl/GraphicsWindowGL3x.h
        src/gl/TypeConverterGL3x.h
        src/gl/buffers/BufferGL3x.h
        src/gl/buffers/IndexBufferGL3x.h
        src/gl/buffers/ReadPixelBufferGL3x.h
        src/gl/buffers/VertexBufferGL3x.h
        src/gl/buffers/WritePixelBufferGL3x.h
        src/gl/framebuffer/FramebufferGL3x.h
        src/gl/shaders/GlslPrelude.h
        src/gl/shaders/ShaderObjectGL3x.h
        src/gl/shaders/ShaderProgramGL3x.h
        src/gl/shaders/UniformGL3x.h
        src/gl/textures/Texture2DGL3x.h
        src/gl/textures/TextureSamplerGL3x.h
        src/gl/vertexarray/VertexArrayGL3x.h
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

`tests/CMakeLists.txt`:

```cmake
add_executable(arda_tests
    src/main.cpp
    src/EllipsoidTests.cpp
    src/core/Matrix4Tests.cpp
    src/renderer/AutomaticUniformTests.cpp
    src/renderer/ContextGL3xTests.cpp
    src/renderer/DeviceTests.cpp
    src/renderer/FramebufferTests.cpp
    src/renderer/RenderStateTests.cpp
    src/renderer/SceneStateTests.cpp
    src/renderer/ShaderProgramTests.cpp
    src/renderer/TextureTests.cpp
    src/renderer/VertexDataTests.cpp
)

# glad: some tests call GL directly to check the state the renderer set (Step 1).
target_link_libraries(arda_tests PRIVATE arda_core arda_renderer glad::glad doctest::doctest)

# White-box tests of the GL backend include private headers from renderer/src (Step 2).
if(ARDA_RENDERER_GL)
    target_sources(arda_tests PRIVATE src/renderer/gl/UniformGL3xTests.cpp)
    target_include_directories(arda_tests PRIVATE ${PROJECT_SOURCE_DIR}/renderer/src)
endif()

add_test(NAME arda_tests COMMAND arda_tests)
```

The root `CMakeLists.txt` also has `find_package(Stb REQUIRED)` next to the
other `find_package` calls, and `vcpkg.json` lists `"stb"` (Step 5).

---

## D3D11 check (3.7.2)

Step 8 implements this. The public interface above needs no changes for it.

- **Views instead of attachment points:** D3D11 doesn't attach textures to a
  framebuffer object. It binds *views* of them. `FramebufferD3D11::Clean`
  creates an `ID3D11RenderTargetView` for each dirty color attachment, and an
  `ID3D11DepthStencilView` for the depth or depth/stencil attachment (using
  the typeless-format table in Step 8). `ContextD3D11::ApplyFramebuffer`
  binds them with `OMSetRenderTargets`. The base class's `Attachment` slots
  and dirty flags are reused as they are.
- **No draw buffers:** slot `N` of `OMSetRenderTargets` receives `SV_TargetN`
  directly. An empty attachment point becomes a `nullptr` view in its slot,
  which does what `GL_NONE` does in `glDrawBuffers`. There is no read
  buffer. The "location N writes to attachment N" rule holds on both APIs.
- **No completeness status:** errors show up when the views are created
  (`CreateRenderTargetView` fails for a format that can't be a render target)
  or as debug layer messages. D3D11 is stricter than GL about render targets
  and depth views of different sizes, so keep all of a framebuffer's
  attachments the same size if you want portable code.
- **No framebuffer:** a `nullptr` framebuffer binds the window's back
  buffer view and the window's own depth view.
- **Feedback loop:** D3D11 won't let a resource be a render target and a
  shader input at the same time. When you bind it as a render target, the
  runtime unbinds it from shader stages, and the debug layer reports it.
  `ContextD3D11::ApplyFramebuffer` unbinds such textures itself.
- **Clear:** `DoClear` calls `ClearRenderTargetView` on every bound color
  view and `ClearDepthStencilView` on the depth view. D3D clears ignore
  scissor and masks, which is why `ClearState` has neither
  ([Step 1](01-state-management.md)).
- **Viewport:** D3D11's viewport origin is top-left. For the window, a
  bottom-left `Rectangle` converts with
  `TopLeftY = backBufferHeight - (bottom + height)`. While a framebuffer is
  set, the backend uses `TopLeftY = bottom` unchanged. Because of the
  clip-space Y flip below, that places a sub-rectangle exactly where GL
  would put it in the texture. Converting it as well would flip it twice.
  See "the viewport's origin is top-left" in [Step 8](08-direct3d11.md).
- **Orientation:** in D3D, row 0 of a render target is the *top* row. In GL
  it is the bottom. Texture coordinate v = 0 is row 0 in both APIs. So
  without a fix, a texture rendered on D3D11 appears upside down when
  sampled with the same coordinates. Step 8 fixes it by flipping clip-space
  Y while a framebuffer is bound (`Context::ClipSpaceTransform`) and
  reversing the front-face winding. The "row 0 is at the bottom" test above
  is what proves the fix.

## Checklist

- [ ] `framebuffer/Framebuffer.h` and `src/framebuffer/Framebuffer.cpp`, with the renderability checks and the attachment count
- [ ] `Context::CreateFramebuffer` (`[[nodiscard]]`), `GetFramebuffer`, `SetFramebuffer`, `DoCreateFramebuffer`, and the depth check in `VerifyDraw`
- [ ] GL: `FramebufferGL3x` (`Bind`, `Unbind`, `Clean` with `glDrawBuffers` and `glReadBuffer`, `Attach`, `StatusMessage`)
- [ ] GL: `ContextGL3x::DoCreateFramebuffer`, `ApplyFramebuffer`, and `m_boundFramebuffer`; `ApplyFramebuffer` called from both `DoClear` and `ApplyBeforeDraw`
- [ ] CMake: `Framebuffer.cpp`, `FramebufferGL3x.cpp`, `FramebufferTests.cpp`
- [ ] `FramebufferTests` pass, in a debug build (which includes the completeness test) and a release build
- [ ] **Milestone:** the triangle, rendered into a texture, appears on a fullscreen quad, and `render-to-texture.png` is the right way up
- [ ] You can explain why framebuffers come from the Context, why the framebuffer is context state, and what the feedback loop is
