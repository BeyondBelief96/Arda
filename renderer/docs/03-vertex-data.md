# Step 3: Vertex data (3.5), then the first triangle

**Goal:** vertex buffers, index buffers, vertex arrays and `Context::Draw`,
then meshes. There are two milestones:

- **3A:** a triangle drawn from raw buffers.
- **3B:** the same triangle built from a `Mesh`.

**Read:** 3.5.1–3.5.4 and 3.5.6 (Listings 3.17–3.24). 3.5.5 (vertex data in
Direct3D) goes with this step's D3D11 check.

## OpenGlobe reference

| File | What to take from it |
|---|---|
| [Buffers/BufferHint.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/BufferHint.cs), [VertexBuffer.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/VertexBuffer.cs), [IndexBuffer.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/IndexBuffer.cs) | Interfaces and copy overloads |
| [GL3x/Buffers/BufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/BufferGL3x.cs) | Shared GL buffer code and range validation |
| [GL3x/Buffers/VertexBufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/VertexBufferGL3x.cs), [IndexBufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/IndexBufferGL3x.cs) | Thin wrappers; the index datatype is inferred from `T` |
| [VertexArray/ComponentDatatype.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/ComponentDatatype.cs), [VertexArraySizes.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/VertexArraySizes.cs) | Component types and their sizes |
| [VertexArray/VertexBufferAttribute.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/VertexBufferAttribute.cs) | Validation and default stride |
| [VertexArray/VertexArray.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/VertexArray.cs), [VertexBufferAttributes.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/VertexBufferAttributes.cs) | Interfaces |
| [GL3x/VertexArray/VertexArrayGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/VertexArray/VertexArrayGL3x.cs), [VertexBufferAttributesGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/VertexArray/VertexBufferAttributesGL3x.cs) | Applying changes at draw time: `Clean`, `Attach`, `Detach`, `MaximumArrayIndex` |
| [GL3x/ContextGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ContextGL3x.cs) | `Draw` (182–222), `VerifyDraw` (486–522), `ApplyBeforeDraw` (524–532), `ApplyVertexArray` (549–554), `ApplyShaderProgram` (556–579) |
| [Core/Geometry/Mesh.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Geometry/Mesh.cs) | `PrimitiveType`, `Mesh` |
| [Core/Geometry/VertexAttributes/*](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Core/Geometry/VertexAttributes) | `VertexAttribute`, `VertexAttribute<T>`, the 13 concrete attribute classes, `VertexAttributeCollection`, `VertexAttributeType` |
| [Core/Geometry/Indices/*](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Core/Geometry/Indices) | `IndicesBase`, `IndicesUnsignedShort`, `IndicesUnsignedInt`, `TriangleIndices*` |
| [Core/Half.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Half.cs), [Core/Vectors/EmulatedVector3D.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Vectors/EmulatedVector3D.cs) | Half-float conversion; splitting a double into high and low floats |
| [Mesh/MeshBuffers.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Mesh/MeshBuffers.cs) | Container of buffers built from a mesh |
| [Device.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Device.cs) | `CreateMeshBuffers` (131–375), `CreateVertexBuffer<T>` (377–385) |
| [Context.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Context.cs) | `CreateVertexArray(Mesh, ...)` and `CreateVertexArray(MeshBuffers)` (lines 33–50) |

## Files

```
core/
  include/arda/core/
    Half.h                          NEW   Half, FloatToHalfBits, HalfBitsToFloat
    geometry/
      PrimitiveType.h               NEW
      VertexAttribute.h             NEW   VertexAttributeType, VertexAttributeBase, VertexAttribute<T, Type>,
                                          aliases, VertexAttributeRGB, VertexAttributeRGBA, VertexAttributeCollection
      Indices.h                     NEW   IndicesType, IndicesBase, Indices<T, Type>, aliases
      Mesh.h                        NEW
  CMakeLists.txt                    UPDATE  list the new headers
renderer/
  include/arda/renderer/
    buffers/
      BufferHint.h                  NEW
      VertexBuffer.h                NEW   BufferSource, detail::CheckBufferRange, VertexBuffer
      IndexBuffer.h                 NEW   IndexBufferDatatype, SizeInBytes(IndexBufferDatatype), IndexBufferSource, IndexBuffer
    vertexarray/
      ComponentDatatype.h           NEW   ComponentDatatype, SizeInBytes(ComponentDatatype)
      VertexBufferAttribute.h       NEW
      VertexArray.h                 NEW
    mesh/
      MeshBuffers.h                 NEW
    scene/
      SceneState.h                  NEW   empty for now; Step 4 fills it in
    DrawState.h                     UPDATE  shaderProgram, vertexArray
    Context.h                       UPDATE  CreateVertexArray (3 overloads), Draw (2 overloads), VerifyDraw
    Device.h                        UPDATE  CreateVertexBuffer, CreateIndexBuffer, CreateMeshBuffers
  src/
    Context.cpp                     UPDATE
    Device.cpp                      UPDATE
    vertexarray/VertexArray.cpp     NEW
    mesh/MeshBuffers.cpp            NEW   Device::CreateMeshBuffers
    gl/
      buffers/BufferGL3x.h / .cpp        NEW
      buffers/VertexBufferGL3x.h         NEW   header only
      buffers/IndexBufferGL3x.h          NEW   header only
      vertexarray/VertexArrayGL3x.h / .cpp NEW
      shaders/ShaderProgramGL3x.h / .cpp UPDATE  Clean
      ContextGL3x.h / .cpp          UPDATE  DoCreateVertexArray, DoDraw, DoDrawRange, Apply*
      DeviceGL3x.h / .cpp           UPDATE  DoCreateVertexBuffer, DoCreateIndexBuffer
      TypeConverterGL3x.h / .cpp    UPDATE  BufferHint, ComponentDatatype, PrimitiveType, IndexBufferDatatype
  CMakeLists.txt                    UPDATE
scene/src/main.cpp                  UPDATE  Milestones 3A and 3B
tests/
  CMakeLists.txt                    UPDATE
  src/renderer/VertexDataTests.cpp  NEW
```

---

## How this step is organized

This is the biggest step in the series. It ends with pixels on the screen,
so it's worth building in small pieces and checking each one:

| Part | You build | Checkpoint |
|---|---|---|
| **A1** Buffers | `VertexBuffer`, `IndexBuffer`, their GL versions, `Device::Create*Buffer` | Tests copy data to the GPU and read it back |
| **A2** Vertex arrays | `VertexBufferAttribute`, `VertexArray`, `VertexArrayGL3x`, `Context::CreateVertexArray` | Tests build a vertex array and check its bookkeeping |
| **A3** Drawing | `DrawState`, `SceneState`, `Context::Draw`, `ContextGL3x::DoDraw` | **Milestone 3A: the first triangle** |
| **B** Meshes | `Half`, `VertexAttribute`, `Indices`, `Mesh`, `MeshBuffers`, `Device::CreateMeshBuffers` | **Milestone 3B: the triangle from a `Mesh`** |

Each part adds its CMake lines and tests as it goes. The [CMake](#cmake) and
[Tests](#tests) sections at the end summarize them.

## Background: what OpenGL needs before it can draw

Before looking at any classes, here is what the GPU needs in order to draw a
triangle. Each item becomes one of this step's classes.

1. **Vertex data in GPU memory.** Positions, normals, colors and so on are
   copied from system memory into *buffer objects*. That's `VertexBuffer`.
2. **Optionally, indices.** A list of vertex numbers that says which
   vertices make up each triangle, so shared vertices are stored once. That's
   `IndexBuffer`.
3. **A description of the vertex layout.** For each vertex shader input
   (`in vec3 position;`), GL needs to know which buffer it comes from, where it
   starts, how many components it has, what type they are and how far apart
   consecutive vertices are. That's `VertexBufferAttribute`. A set of them,
   plus the index buffer, is a *vertex array object*: `VertexArray`.
4. **A shader program** to run (Step 2) and **render state** to configure the
   fixed-function stages (Step 1).
5. **A draw call** that says what kind of primitive to assemble (triangles,
   lines, ...) and how many vertices or indices to read. That's
   `Context::Draw`.

In OpenGlobe (and here), items 3–5 all go into a `DrawState` (Listing 3.5),
and one call draws it:

```cpp
context.Draw(PrimitiveType::Triangles, drawState, sceneState);
```

The book's reasoning (3.5) is that GL's global bind-then-draw style is easy
to get wrong. Wrapping each concept in an object and passing everything to
`Draw` explicitly means there is no hidden "currently bound" state for
client code to trip over.

---

## Part A1: buffers (3.5.1, 3.5.2)

### A1.1 `buffers/BufferHint.h`

`BufferHint` tells the driver how the buffer will be used, so it can decide
where to put the memory. It's a direct copy of `BufferHint.cs`, and it maps
one-to-one onto the `usage` argument of `glBufferData`.

```cpp
#pragma once

namespace arda::renderer {

// How a buffer's data will be written and read (3.5.1). Maps to glBufferData's usage argument.
enum class BufferHint {
    StreamDraw,  StreamRead,  StreamCopy,
    StaticDraw,  StaticRead,  StaticCopy,
    DynamicDraw, DynamicRead, DynamicCopy,
};

} // namespace arda::renderer
```

> **OpenGL note — usage hints:** each hint is two words.
>
> - The first word says **how often the data changes**:
>   - `Stream`: written once and used a few times, like per-frame data.
>   - `Static`: written once and used many times, like terrain or a globe mesh.
>   - `Dynamic`: rewritten often and used many times between writes.
> - The second word says **who writes and who reads**:
>   - `Draw`: the application writes and GL reads, for drawing.
>   - `Read`: GL writes and the application reads the data back.
>   - `Copy`: GL writes and GL reads.
>
> Vertex and index buffers are almost always `StaticDraw` or `DynamicDraw`.
> Hints only affect **speed**. A wrong hint never gives wrong results, and
> drivers are free to ignore them. It is still worth getting them right,
> because a driver may put a `StaticDraw` buffer in video memory that is
> slow for the CPU to update.

### A1.2 `buffers/VertexBuffer.h` (Listing 3.18)

A vertex buffer is a block of GPU memory with a fixed size. It has no
element type: GL only ever sees bytes. The vertex array (Part A2) is what
later says "these bytes are three floats per vertex".

OpenGlobe's `VertexBuffer` has generic methods:
`CopyFromSystemMemory<T>(T[] bufferInSystemMemory, ...) where T : struct`,
and each backend implements them. C++ can't do that directly, so the class
is split into two layers:

- **Public, non-virtual templates** accept any suitable container
  (`std::vector`, `std::array`, a C array, a `std::span`), check the byte
  range, and turn the values into a `std::span<const std::byte>`.
- **Protected pure virtual functions** take only bytes. Each backend
  implements these.

```cpp
#pragma once

#include <arda/renderer/buffers/BufferHint.h>

#include <cstddef>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace arda::renderer {

namespace detail {

// Throws if [offsetInBytes, offsetInBytes + lengthInBytes) doesn't fit in a buffer of
// bufferSizeInBytes bytes. Written so the addition can't overflow.
inline void CheckBufferRange(std::size_t offsetInBytes, std::size_t lengthInBytes, std::size_t bufferSizeInBytes) {
    if (offsetInBytes > bufferSizeInBytes || lengthInBytes > bufferSizeInBytes - offsetInBytes) {
        throw std::out_of_range("offsetInBytes + lengthInBytes must be less than or equal to SizeInBytes()");
    }
}

// Reading back lengthInBytes bytes as T only makes sense if they hold a whole number of Ts.
template <typename T>
void CheckWholeElements(std::size_t lengthInBytes) {
    if (lengthInBytes % sizeof(T) != 0) {
        throw std::invalid_argument("lengthInBytes must be a multiple of sizeof(T)");
    }
}

} // namespace detail

// A contiguous range (std::vector, std::array, C array, std::span) of trivially copyable values.
// These are exactly the ranges that can be copied to the GPU as raw bytes.
template <typename R>
concept BufferSource = std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
                       std::is_trivially_copyable_v<std::ranges::range_value_t<R>>;

// Vertex data in GPU memory (3.5.1, Listing 3.18). Created by Device::CreateVertexBuffer.
class VertexBuffer {
public:
    virtual ~VertexBuffer() = default;

    VertexBuffer(const VertexBuffer&)            = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // Copies every element of values into the buffer, starting destinationOffsetInBytes bytes in.
    //   buffer->CopyFromSystemMemory(std::vector<Vector3<float>>{...});
    // To copy only part of a container, pass a std::span of that part.
    template <BufferSource R>
    void CopyFromSystemMemory(const R& values, std::size_t destinationOffsetInBytes = 0) {
        const auto bytes = std::as_bytes(std::span(std::ranges::data(values), std::ranges::size(values)));
        detail::CheckBufferRange(destinationOffsetInBytes, bytes.size(), SizeInBytes());
        CopyFromSystemMemoryBytes(bytes, destinationOffsetInBytes);
    }

    // Reads lengthInBytes bytes, starting offsetInBytes bytes in, back as Ts.
    template <typename T>
        requires std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>
    std::vector<T> CopyToSystemMemory(std::size_t offsetInBytes, std::size_t lengthInBytes) const {
        detail::CheckBufferRange(offsetInBytes, lengthInBytes, SizeInBytes());
        detail::CheckWholeElements<T>(lengthInBytes);
        std::vector<T> values(lengthInBytes / sizeof(T));
        CopyToSystemMemoryBytes(std::as_writable_bytes(std::span(values)), offsetInBytes);
        return values;
    }

    // Reads the whole buffer back as Ts.
    template <typename T>
    std::vector<T> CopyToSystemMemory() const {
        return CopyToSystemMemory<T>(0, SizeInBytes());
    }

    virtual std::size_t SizeInBytes() const = 0;
    virtual BufferHint UsageHint() const = 0;

protected:
    VertexBuffer() = default;

    // The range has already been checked by the public templates.
    virtual void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) = 0;
    virtual void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const = 0;
};

} // namespace arda::renderer
```

Some notes on how the C# became C++:

- **One template covers three C# overloads.** C# has
  `CopyFromSystemMemory<T>(T[])`, `(T[], int offset)` and
  `(T[], int offset, int lengthInBytes)`. Here the offset has a default
  argument. The length is given by the range itself: to copy part of a
  vector, pass `std::span(values).subspan(first, count)`.
- **Range checks happen once.** `BufferGL3x.cs` checks the range in every
  backend. Here the public template checks it, so backends can trust their
  arguments. This is the same idea as the non-virtual interface pattern in
  the README.
- **`CopyToSystemMemory` needs the type spelled out.** Write
  `buffer->CopyToSystemMemory<float>()`, because nothing in the arguments
  says what `T` is. C# works the same way.
- **The copy operations are deleted**, as in every resource class since
  Step 0. See [Step 0](00-setup.md) for the rule of 0/3/5.

> **Why:** typed templates forward to byte-level virtual functions because
> **a function template can't be virtual** (see [Step 2](02-shaders.md)).
> The vtable needs one entry per function, and a template stands for an
> unbounded number of functions. Moving the typing into non-virtual
> templates and the GPU work into byte-level virtuals gives callers the
> convenience of typed calls, and each backend implements only two small
> functions. GL and D3D only ever deal in bytes anyway.
>
> Rejected alternatives:
>
> - **A class template `VertexBuffer<T>`.** Every buffer would carry an
>   element type, but `VertexArray` needs to hold buffers of *different* `T`s
>   side by side. It would need a non-template base again, and one buffer
>   couldn't hold interleaved data of mixed types (see
>   [A3.11](#a311-things-to-try)).
> - **Virtual functions taking `const void*` and a size.** This works, but
>   loses the size/pointer pairing and all type checking. `std::span<const std::byte>`
>   is the same two words, with both kept together.

> **C++ note — `std::byte`, `std::span<const std::byte>` and `std::as_bytes`:**
> `std::byte` (C++17, `<cstddef>`) is a type for raw memory. Unlike `char` or
> `std::uint8_t`, it isn't a character or a number: you can't add to it, only
> apply bitwise operators. Using it for "bytes that go to the GPU" makes the
> intent clear, and the compiler rejects accidental arithmetic.
>
> A `std::span<T>` is a non-owning view: a pointer and a count (the basics
> are in [Step 2](02-shaders.md)). `std::as_bytes(span<const T>)` turns it
> into a `span<const std::byte>` over the same memory, with
> `size() == count * sizeof(T)`. Nothing is copied. `std::as_writable_bytes`
> does the same for a non-const span, which `CopyToSystemMemory` uses so GL
> can write straight into the vector.
>
> ```cpp
> std::vector<float> values = {1.0f, 2.0f, 3.0f};
> std::span<const std::byte> bytes = std::as_bytes(std::span(values));
> // bytes.data() points at values.data(), and bytes.size() == 12.
> ```
>
> Viewing any object as bytes is always allowed in C++. The reverse
> (treating bytes as a `T`) is only safe for trivially copyable types, which
> is what the `BufferSource` concept below checks. In C#, this is what
> `MemoryMarshal.AsBytes(ReadOnlySpan<T>)` does.

> **C++ note — concepts and `requires`:** a *concept* (C++20) is a named,
> compile-time true/false test on a type. It plays the role of a C#
> generic constraint (`where T : struct`), but can express far more.
> `BufferSource` combines three standard tests:
>
> - `std::ranges::contiguous_range<R>`: the elements sit next to each other
>   in memory, so they can be handed to GL as one block. `std::vector`,
>   `std::array`, C arrays, `std::span` and `std::string` qualify.
>   `std::list`, `std::deque` and `std::vector<bool>` (which packs bits)
>   don't.
> - `std::ranges::sized_range<R>`: the size is known without walking the
>   range.
> - `std::is_trivially_copyable_v<range_value_t<R>>`: the element can be
>   copied with `memcpy`. It has no virtual functions and no user-written
>   copy, move or destructor. `float`, `Vector3<float>` and
>   `std::uint16_t` are fine. `std::string` isn't: it holds a pointer to
>   heap memory, and uploading a `std::vector<std::string>` would send
>   pointer values to the GPU.
>
> There are two ways to apply a concept:
>
> ```cpp
> template <BufferSource R>                   // shorthand: R must satisfy BufferSource
> void CopyFromSystemMemory(const R& values);
>
> template <typename T>
>     requires std::is_trivially_copyable_v<T>   // a requires-clause: any compile-time bool
> std::vector<T> CopyToSystemMemory(...);
> ```
>
> Passing a `std::list<float>` now fails with "constraints not satisfied" at
> the call, instead of a page of errors from deep inside `std::span`.

`detail::CheckBufferRange` is `inline` because it's defined in a header (see
[Step 1](01-state-management.md)). Code in a `detail` namespace isn't part
of the API, but it has to be in the header because templates call it.
Step 5's pixel buffers reuse both `BufferSource` and `detail::CheckBufferRange`.

### A1.3 `buffers/IndexBuffer.h`

An index buffer is a vertex buffer for indices. There are two differences:

- The elements must be `std::uint16_t` or `std::uint32_t`, because those are
  the only index types GL and D3D share. (GL also allows bytes. OpenGlobe
  leaves them out on purpose; see the comment in `IndexBuffer.cs`.)
- The buffer remembers **the index type** and **how many indices it holds**,
  because the draw call needs both.

`IndexBufferGL3x.cs` checks `typeof(T)` at runtime and throws for other
types. In C++, the check moves to compile time: the `IndexBufferSource`
concept rejects a `std::vector<int>` before the program even builds.

```cpp
#pragma once

#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/buffers/VertexBuffer.h>   // BufferSource, detail::CheckBufferRange

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace arda::renderer {

// OpenGL also supports byte indices, but Direct3D doesn't (IndexBuffer.cs).
enum class IndexBufferDatatype {
    UnsignedShort,
    UnsignedInt,
};

// VertexArraySizes.cs, SizeOf(IndexBufferDatatype)
constexpr std::size_t SizeInBytes(IndexBufferDatatype datatype) {
    switch (datatype) {
    case IndexBufferDatatype::UnsignedShort: return sizeof(std::uint16_t);
    case IndexBufferDatatype::UnsignedInt:   return sizeof(std::uint32_t);
    }
    throw std::invalid_argument("Invalid IndexBufferDatatype");
}

// A BufferSource whose elements are 16- or 32-bit unsigned indices.
template <typename R>
concept IndexBufferSource = BufferSource<R> &&
                            (std::same_as<std::ranges::range_value_t<R>, std::uint16_t> ||
                             std::same_as<std::ranges::range_value_t<R>, std::uint32_t>);

// Indices in GPU memory (3.5.2). Created by Device::CreateIndexBuffer.
class IndexBuffer {
public:
    virtual ~IndexBuffer() = default;

    IndexBuffer(const IndexBuffer&)            = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    // Copies indices into the buffer and records their type. Only std::uint16_t
    // and std::uint32_t elements compile.
    template <IndexBufferSource R>
    void CopyFromSystemMemory(const R& indices, std::size_t destinationOffsetInBytes = 0) {
        using T = std::ranges::range_value_t<R>;
        const auto bytes = std::as_bytes(std::span(std::ranges::data(indices), std::ranges::size(indices)));
        detail::CheckBufferRange(destinationOffsetInBytes, bytes.size(), SizeInBytes());

        m_datatype = std::same_as<T, std::uint16_t> ? IndexBufferDatatype::UnsignedShort
                                                    : IndexBufferDatatype::UnsignedInt;
        m_count = SizeInBytes() / sizeof(T);   // as in IndexBufferGL3x.cs: the capacity, not indices.size()
        CopyFromSystemMemoryBytes(bytes, destinationOffsetInBytes);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>
    std::vector<T> CopyToSystemMemory(std::size_t offsetInBytes, std::size_t lengthInBytes) const {
        detail::CheckBufferRange(offsetInBytes, lengthInBytes, SizeInBytes());
        detail::CheckWholeElements<T>(lengthInBytes);
        std::vector<T> values(lengthInBytes / sizeof(T));
        CopyToSystemMemoryBytes(std::as_writable_bytes(std::span(values)), offsetInBytes);
        return values;
    }

    template <typename T>
    std::vector<T> CopyToSystemMemory() const {
        return CopyToSystemMemory<T>(0, SizeInBytes());
    }

    virtual std::size_t SizeInBytes() const = 0;
    virtual BufferHint UsageHint() const = 0;

    // Set by the last CopyFromSystemMemory. UnsignedShort until then.
    IndexBufferDatatype Datatype() const { return m_datatype; }

    // Number of indices the buffer can hold: SizeInBytes() / SizeInBytes(Datatype()).
    std::size_t Count() const { return m_count; }

protected:
    IndexBuffer() = default;

    virtual void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) = 0;
    virtual void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const = 0;

private:
    IndexBufferDatatype m_datatype = IndexBufferDatatype::UnsignedShort;
    std::size_t m_count = 0;
};

} // namespace arda::renderer
```

`m_datatype` and `m_count` live in the base class, not in each backend like
`IndexBufferGL3x.cs` has them. They don't depend on the API, and
`ContextGL3x` and `ContextD3D11` both read them when drawing.

`std::same_as<T, std::uint16_t>` is a concept used as an ordinary `bool`
expression, which is allowed anywhere a constant is.

> **C++ note — fixed-width integer types:** C++ only guarantees minimum sizes
> for its built-in integers. `unsigned short` is *at least* 16 bits, and
> `unsigned int` at least 16 too (it's 32 on every desktop compiler, but that
> isn't promised). GPU data has to match bit for bit: `GL_UNSIGNED_SHORT`
> means exactly 16 bits per index. `<cstdint>` provides types with exact
> sizes: `std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::int16_t`
> and so on. C#'s `ushort` and `uint` are always 16 and 32 bits, so the C#
> code didn't need this distinction.
>
> ```cpp
> std::vector<std::uint16_t> indices = {0, 1, 2};   // 6 bytes, always
> static_assert(sizeof(std::uint32_t) == 4);        // true on every platform that has the type
> ```
>
> **Choosing an index type:** 16-bit indices can only address vertices 0 to
> 65535 (65534 if primitive restart is on, because the maximum value is
> reserved; see [A3.7](#a37-gl-contextgl3x-drawing)). A globe tessellation
> with more vertices needs `std::uint32_t`. Otherwise use `std::uint16_t`,
> which halves the index memory. Watch for silent wrap-around:
> `static_cast<std::uint16_t>(70000)` is `4464`, not an error.

> **OpenGL note — index types:** `glDrawElements` takes a `type` argument
> of `GL_UNSIGNED_BYTE`, `GL_UNSIGNED_SHORT` or `GL_UNSIGNED_INT`, and reads
> every index in the buffer as that type. Nothing in the buffer records the
> type, so if you upload `std::uint32_t`s and draw with `GL_UNSIGNED_SHORT`,
> GL reads each 32-bit index as two 16-bit ones (`1, 0, 2, 0, ...`) and
> draws garbage. Recording the type in `CopyFromSystemMemory` makes that
> mistake impossible.

### A1.4 `Device.h` and `Device.cpp` additions

Buffers are created by the device (3.2) because GL shares them between
contexts. Add these to the class from [Step 0](00-setup.md) and
[Step 2](02-shaders.md):

```cpp
// Device.h: new includes, next to the existing ones
#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/buffers/VertexBuffer.h>

#include <cstddef>

class Device {
public:
    // ...existing members, after CreateShaderProgram...

    // Allocates sizeInBytes bytes of GPU memory. The contents start undefined;
    // fill them with CopyFromSystemMemory. Throws std::invalid_argument if sizeInBytes is 0.
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes);
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes);

protected:
    // ...existing Do* functions...

    virtual std::shared_ptr<VertexBuffer> DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) = 0;
    virtual std::shared_ptr<IndexBuffer> DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) = 0;
};
```

```cpp
// src/Device.cpp: add inside namespace arda::renderer, after CreateShaderProgram.
// <stdexcept> is already included.

std::shared_ptr<VertexBuffer> Device::CreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    if (sizeInBytes == 0) {
        throw std::invalid_argument("CreateVertexBuffer: sizeInBytes must be greater than zero");
    }
    return DoCreateVertexBuffer(usageHint, sizeInBytes);
}

std::shared_ptr<IndexBuffer> Device::CreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    if (sizeInBytes == 0) {
        throw std::invalid_argument("CreateIndexBuffer: sizeInBytes must be greater than zero");
    }
    return DoCreateIndexBuffer(usageHint, sizeInBytes);
}
```

The zero-size check comes from the `BufferGL3x` constructor in C#. It sits in
the public function so D3D11 gets it too. (`CreateBuffer` fails on a zero-sized
buffer, and `glBufferData` accepts one, so without the shared check the two
backends would behave differently.)

> **C++ note — `std::shared_ptr`: shared ownership:** in C#, a `VertexBuffer`
> stays alive as long as anything can reach it, and the garbage collector
> frees it later. GPU memory isn't freed until someone calls `Dispose`,
> which is why OpenGlobe has `DisposeBuffers` flags. C++ has no garbage
> collector, so ownership must be stated in the types:
>
> - `std::unique_ptr<T>`: exactly one owner (see [Step 0](00-setup.md)).
>   Used for windows.
> - `std::shared_ptr<T>`: any number of owners. Used for resources, because a
>   buffer really can be owned by several things at once: the code that
>   created it, one or more vertex arrays, and the `MeshBuffers` it came
>   from.
>
> A `shared_ptr` points at the object and at a *control block* holding a
> reference count. Copying a `shared_ptr` increments the count, destroying
> one decrements it, and when it reaches zero the object is deleted there
> and then. For a `VertexBufferGL3x`, deletion runs the destructor, which
> destroys its `BufferName`, which calls `glDeleteBuffers`. GPU memory is
> released at a predictable moment, with no finalizer thread (C# needed one:
> see `FinalizerThreadContextGL3x` in the README's "not needed" table).
>
> ```cpp
> std::shared_ptr<VertexBuffer> a = device->CreateVertexBuffer(BufferHint::StaticDraw, 12);  // count 1
> {
>     std::shared_ptr<VertexBuffer> b = a;   // count 2: a and b own the same buffer
> }                                          // b destroyed: count 1
> a.reset();                                 // count 0: glDeleteBuffers runs here
> ```
>
> The backends create objects with `std::make_shared<VertexBufferGL3x>(...)`.
> It allocates the object and its control block in a single allocation, and
> the returned `shared_ptr<VertexBufferGL3x>` converts implicitly to
> `shared_ptr<VertexBuffer>`.
>
> Two costs to keep in mind:
>
> - **Copies aren't free.** The count is updated atomically, so it's
>   thread-safe but slower than copying a raw pointer. Pass
>   `const std::shared_ptr<T>&` (or `T&`) to functions that only use the
>   object, and take `shared_ptr<T>` by value and `std::move` it only where
>   ownership is stored (see [Step 1](01-state-management.md) on `const&`
>   versus by-value).
> - **Cycles leak.** If A holds a `shared_ptr` to B and B holds one to A,
>   neither count ever reaches zero. A garbage collector handles that;
>   reference counting doesn't. The renderer's ownership graph has no
>   cycles: draw states own vertex arrays, vertex arrays own buffers, and
>   buffers own nothing. Where something needs to *refer back* without
>   owning, it uses `std::weak_ptr` (see [A3.7](#a37-gl-contextgl3x-drawing)).

### A1.5 GL: `TypeConverterGL3x`, `ToGL(BufferHint)`

The GL buffer code needs the GL value for each hint. Add to the type
converter from [Step 1](01-state-management.md):

```cpp
// TypeConverterGL3x.h: new include and declaration
#include <arda/renderer/buffers/BufferHint.h>

GLenum ToGL(BufferHint hint);
```

```cpp
// TypeConverterGL3x.cpp
GLenum ToGL(BufferHint hint) {
    switch (hint) {
    case BufferHint::StreamDraw:  return GL_STREAM_DRAW;
    case BufferHint::StreamRead:  return GL_STREAM_READ;
    case BufferHint::StreamCopy:  return GL_STREAM_COPY;
    case BufferHint::StaticDraw:  return GL_STATIC_DRAW;
    case BufferHint::StaticRead:  return GL_STATIC_READ;
    case BufferHint::StaticCopy:  return GL_STATIC_COPY;
    case BufferHint::DynamicDraw: return GL_DYNAMIC_DRAW;
    case BufferHint::DynamicRead: return GL_DYNAMIC_READ;
    case BufferHint::DynamicCopy: return GL_DYNAMIC_COPY;
    }
    throw std::invalid_argument("Invalid BufferHint");
}
```

This follows Step 1's rule: a `switch` with no `default`, so the compiler
warns if an enum value is added and not handled here.

OpenGlobe also converts the other way (`BufferUsageHint` to `BufferHint`)
so `UsageHint` can be read back from the stored GL value. `BufferGL3x` below
stores the arda hint instead, so that function isn't needed.

### A1.6 GL: `src/gl/buffers/BufferGL3x.h` and `.cpp`

`BufferGL3x` is the GL code that vertex, index and (in Step 5) pixel
buffers all share. It's a private class, like the C# one (`internal sealed`).
Each public buffer type *contains* a `BufferGL3x` rather than deriving from
it, which is also what OpenGlobe does.

> **OpenGL note — buffer objects and targets:** a *buffer object* is a block
> of memory owned by the driver (usually in video memory), named by a
> `GLuint`. `glGenBuffers` reserves a name; `glDeleteBuffers` frees the
> object. `BufferName` from [Step 0](00-setup.md) wraps that pair.
>
> GL uses **bind-to-edit**. Most buffer functions don't take the buffer's
> name. They act on "whatever buffer is bound to *target* X":
>
> ```cpp
> glBindBuffer(GL_ARRAY_BUFFER, name);                              // select it
> glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);     // acts on the selected buffer
> ```
>
> The *target* says what the binding is for:
>
> | Target | Used for |
> |---|---|
> | `GL_ARRAY_BUFFER` | Vertex data. `glVertexAttribPointer` records whichever buffer is bound here when it's called. |
> | `GL_ELEMENT_ARRAY_BUFFER` | Indices. This binding is **part of the bound vertex array object**, not global state. |
> | `GL_PIXEL_UNPACK_BUFFER`, `GL_PIXEL_PACK_BUFFER` | Texture uploads and downloads (Step 5) |
> | `GL_UNIFORM_BUFFER`, `GL_COPY_READ_BUFFER`, ... | Not used in Chapter 3 |
>
> A buffer object doesn't have a fixed type. The target only describes how
> GL uses it *through that binding*. The same buffer could be bound as
> `GL_ARRAY_BUFFER` for one call and `GL_ELEMENT_ARRAY_BUFFER` for the next.
> GL 4.5 added "direct state access" (`glNamedBufferData(name, ...)`), which
> avoids binding altogether, but it isn't available in 3.3.

> **OpenGL note — `glBufferData`, `glBufferSubData` and `glGetBufferSubData`:**
>
> - `glBufferData(target, size, data, usage)` **allocates** the buffer's
>   storage, freeing any storage it had before. If `data` isn't null, it also
>   copies `size` bytes in. The usage hint is fixed at this point.
> - `glBufferSubData(target, offset, size, data)` **overwrites** part of the
>   existing storage. It never reallocates, and a range past the end is a
>   `GL_INVALID_VALUE` error.
> - `glGetBufferSubData(target, offset, size, data)` **reads** bytes back into
>   system memory. The CPU has to wait until the GPU has finished any
>   commands that use the buffer, so it's fine for tests but shouldn't be
>   called every frame.
>
> `BufferGL3x` allocates once with `glBufferData(..., nullptr, ...)` in the
> constructor, and every copy uses `glBufferSubData`. A buffer's size is
> therefore fixed for its lifetime, which is what the `SizeInBytes()` checks
> rely on. The comment in `BufferGL3x.cs` notes that allocating first and
> writing later has no real overhead.

```cpp
// src/gl/buffers/BufferGL3x.h
#pragma once

#include "gl/GLHandle.h"

#include <arda/renderer/buffers/BufferHint.h>

#include <cstddef>
#include <span>

namespace arda::renderer::gl {

// One GL buffer object and its fixed size and target (BufferGL3x.cs).
// Shared by VertexBufferGL3x, IndexBufferGL3x and, in Step 5, the pixel buffers.
class BufferGL3x {
public:
    // Allocates sizeInBytes bytes with glBufferData. Leaves the buffer bound to target.
    BufferGL3x(GLenum target, BufferHint usageHint, std::size_t sizeInBytes);

    // The caller has already checked that the range fits.
    void CopyFromSystemMemory(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes);
    void CopyToSystemMemory(std::span<std::byte> bytes, std::size_t offsetInBytes) const;

    std::size_t SizeInBytes() const { return m_sizeInBytes; }
    BufferHint UsageHint() const { return m_usageHint; }
    GLuint Handle() const { return m_name.Get(); }

    void Bind() const { glBindBuffer(m_target, m_name.Get()); }

private:
    BufferName m_name;
    GLenum m_target;
    BufferHint m_usageHint;
    std::size_t m_sizeInBytes;
};

} // namespace arda::renderer::gl
```

```cpp
// src/gl/buffers/BufferGL3x.cpp
#include "gl/buffers/BufferGL3x.h"
#include "gl/TypeConverterGL3x.h"

namespace arda::renderer::gl {

BufferGL3x::BufferGL3x(GLenum target, BufferHint usageHint, std::size_t sizeInBytes)
    : m_name(CreateBufferName()), m_target(target), m_usageHint(usageHint), m_sizeInBytes(sizeInBytes) {
    // Unbind the vertex array first. Otherwise binding GL_ELEMENT_ARRAY_BUFFER
    // would replace the index buffer of whichever vertex array is bound (3.5.3).
    glBindVertexArray(0);
    Bind();
    glBufferData(m_target, static_cast<GLsizeiptr>(sizeInBytes), nullptr, ToGL(usageHint));
}

void BufferGL3x::CopyFromSystemMemory(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) {
    glBindVertexArray(0);
    Bind();
    glBufferSubData(m_target,
        static_cast<GLintptr>(destinationOffsetInBytes),
        static_cast<GLsizeiptr>(bytes.size()),
        bytes.data());
}

void BufferGL3x::CopyToSystemMemory(std::span<std::byte> bytes, std::size_t offsetInBytes) const {
    glBindVertexArray(0);
    Bind();
    glGetBufferSubData(m_target,
        static_cast<GLintptr>(offsetInBytes),
        static_cast<GLsizeiptr>(bytes.size()),
        bytes.data());
}

} // namespace arda::renderer::gl
```

The `static_cast`s convert `std::size_t` (unsigned) to GL's signed
pointer-sized types `GLintptr` and `GLsizeiptr` (see [Step 0](00-setup.md)
on `static_cast`).

> **Why:** every function calls `glBindVertexArray(0)` before binding the
> buffer. This is the most important line in the file.
>
> The `GL_ELEMENT_ARRAY_BUFFER` binding is stored **in the currently bound
> vertex array object** (see [A2.3](#a23-vertexarrayvertexarrayh-listing-319)).
> Suppose vertex array X is still bound from the last draw, and you create
> an index buffer for a different mesh. The constructor's
> `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newBuffer)` would silently make
> `newBuffer` X's index buffer, and the next draw with X would read the
> wrong indices. There's no GL error: X just renders garbage, and only
> after an unrelated buffer was created. Unbinding the VAO first means the
> binding goes to VAO 0, which is never drawn with.
>
> `GL_ARRAY_BUFFER` is *not* VAO state, so binding a vertex buffer can't
> corrupt a VAO. OpenGlobe unbinds for every target anyway, which is
> simpler and costs almost nothing.
>
> There's a side effect: after any buffer call, no VAO is bound. That's fine
> here because `ContextGL3x` binds the draw's VAO before every draw (A3.7).
> If you ever cache "the currently bound VAO" to skip that bind, this is
> the code that would make the cache wrong.

> **Pitfall — macOS:** in a core profile, binding `GL_ELEMENT_ARRAY_BUFFER`
> with VAO 0 bound isn't listed as an error in the spec, and Windows and
> Linux drivers accept it. Some drivers, mostly on macOS, have been reported
> to raise `GL_INVALID_OPERATION` for `glBufferData` on the element target
> with no VAO bound. If you hit that, create
> and fill every buffer through `GL_ARRAY_BUFFER` (a buffer has no fixed
> type, as noted above) and use `GL_ELEMENT_ARRAY_BUFFER` only in
> `VertexArrayGL3x::Clean`.

### A1.7 GL: `src/gl/buffers/VertexBufferGL3x.h` and `IndexBufferGL3x.h`

These are the same thin wrappers as the C# classes. They're short enough to
be entirely inline, so there are no `.cpp` files.

```cpp
// src/gl/buffers/VertexBufferGL3x.h
#pragma once

#include "gl/buffers/BufferGL3x.h"

#include <arda/renderer/buffers/VertexBuffer.h>

namespace arda::renderer::gl {

class VertexBufferGL3x final : public VertexBuffer {
public:
    VertexBufferGL3x(BufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(GL_ARRAY_BUFFER, usageHint, sizeInBytes) {}

    void Bind() const { m_buffer.Bind(); }
    static void Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

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
    BufferGL3x m_buffer;
};

} // namespace arda::renderer::gl
```

```cpp
// src/gl/buffers/IndexBufferGL3x.h
#pragma once

#include "gl/buffers/BufferGL3x.h"

#include <arda/renderer/buffers/IndexBuffer.h>

namespace arda::renderer::gl {

class IndexBufferGL3x final : public IndexBuffer {
public:
    IndexBufferGL3x(BufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(GL_ELEMENT_ARRAY_BUFFER, usageHint, sizeInBytes) {}

    // Only call while the vertex array that should use this index buffer is bound.
    void Bind() const { m_buffer.Bind(); }
    static void Unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

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
    BufferGL3x m_buffer;
};

} // namespace arda::renderer::gl
```

The overrides are `protected`, like the functions they override. Calling
code goes through the public templates in the base class. Both classes are
`final` and every override is marked `override` (see [Step 0](00-setup.md)),
so a typo in a signature is a compile error rather than a new, unrelated
function.

### A1.8 GL: `DeviceGL3x` additions

```cpp
// DeviceGL3x.h: add to the protected section
std::shared_ptr<VertexBuffer> DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;
std::shared_ptr<IndexBuffer> DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) override;
```

```cpp
// DeviceGL3x.cpp: new includes
#include "gl/buffers/IndexBufferGL3x.h"
#include "gl/buffers/VertexBufferGL3x.h"

// DeviceGL3x.cpp: new functions
std::shared_ptr<VertexBuffer> DeviceGL3x::DoCreateVertexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<VertexBufferGL3x>(usageHint, sizeInBytes);
}

std::shared_ptr<IndexBuffer> DeviceGL3x::DoCreateIndexBuffer(BufferHint usageHint, std::size_t sizeInBytes) {
    return std::make_shared<IndexBufferGL3x>(usageHint, sizeInBytes);
}
```

Buffers are shared between contexts (3.2), so it doesn't matter which
context is current when one is created, as long as *some* context of this
device is current. After `CreateDevice` the hidden share context is current,
and after `CreateGraphicsWindow` the window's context is.

### A1.9 Checkpoint 1: buffers round-trip through the GPU

**CMake.** Add the new files. Headers are listed only so IDEs show them
(see the comment at the top of each `CMakeLists.txt`).

```cmake
# renderer/CMakeLists.txt, in add_library(arda_renderer ...)
    include/arda/renderer/buffers/BufferHint.h
    include/arda/renderer/buffers/IndexBuffer.h
    include/arda/renderer/buffers/VertexBuffer.h

# renderer/CMakeLists.txt, in the ARDA_RENDERER_GL target_sources(...)
    src/gl/buffers/BufferGL3x.cpp
    src/gl/buffers/BufferGL3x.h
    src/gl/buffers/IndexBufferGL3x.h
    src/gl/buffers/VertexBufferGL3x.h
```

```cmake
# tests/CMakeLists.txt, in add_executable(arda_tests ...)
    src/renderer/VertexDataTests.cpp
```

**Tests.** Create `tests/src/renderer/VertexDataTests.cpp`. Later parts add
more test cases and includes to this file.

```cpp
#include <doctest/doctest.h>

#include <arda/renderer/Device.h>
#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/buffers/VertexBuffer.h>

#include <array>
#include <cstdint>
#include <list>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace arda::renderer;

// Compile-time checks of which containers can be uploaded.
static_assert(BufferSource<std::vector<float>>);
static_assert(BufferSource<std::array<std::uint16_t, 3>>);
static_assert(BufferSource<float[4]>);
static_assert(!BufferSource<std::list<float>>);          // not contiguous
static_assert(!BufferSource<std::vector<bool>>);         // packed bits, not contiguous bools
static_assert(!BufferSource<std::vector<std::string>>);  // not trivially copyable
static_assert(IndexBufferSource<std::vector<std::uint32_t>>);
static_assert(!IndexBufferSource<std::vector<int>>);     // indices must be uint16_t or uint32_t

TEST_CASE("VertexBuffer copies to and from system memory") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const std::vector<float> values = {1, 2, 3, 4};

    auto buffer = device->CreateVertexBuffer(BufferHint::StaticDraw, values.size() * sizeof(float));
    buffer->CopyFromSystemMemory(values);

    CHECK(buffer->SizeInBytes() == 16);
    CHECK(buffer->UsageHint() == BufferHint::StaticDraw);
    CHECK(buffer->CopyToSystemMemory<float>() == values);
    CHECK(buffer->CopyToSystemMemory<float>(4, 8) == std::vector<float>{2, 3});

    CHECK_THROWS_AS(buffer->CopyFromSystemMemory(values, 4), std::out_of_range);   // 4 + 16 > 16
    CHECK_THROWS_AS(buffer->CopyToSystemMemory<float>(0, 6), std::invalid_argument); // 1.5 floats
}

TEST_CASE("VertexBuffer accepts any contiguous range and copies at an offset") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto buffer = device->CreateVertexBuffer(BufferHint::DynamicDraw, 4 * sizeof(float));

    const float cArray[] = {1, 2, 3, 4};
    buffer->CopyFromSystemMemory(cArray);

    const std::array<float, 2> replacement = {20, 30};
    buffer->CopyFromSystemMemory(replacement, 1 * sizeof(float));   // overwrite elements 1 and 2
    CHECK(buffer->CopyToSystemMemory<float>() == std::vector<float>{1, 20, 30, 4});

    const std::vector<float> source = {7, 8, 9};
    buffer->CopyFromSystemMemory(std::span(source).subspan(1, 2));   // copy 8 and 9 only
    CHECK(buffer->CopyToSystemMemory<float>() == std::vector<float>{8, 9, 30, 4});
}

TEST_CASE("IndexBuffer infers its datatype and count") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    auto shortIndices = device->CreateIndexBuffer(BufferHint::StaticDraw, 6 * sizeof(std::uint16_t));
    shortIndices->CopyFromSystemMemory(std::vector<std::uint16_t>{0, 1, 2, 2, 1, 3});
    CHECK(shortIndices->Datatype() == IndexBufferDatatype::UnsignedShort);
    CHECK(shortIndices->Count() == 6);
    CHECK(shortIndices->CopyToSystemMemory<std::uint16_t>() == std::vector<std::uint16_t>{0, 1, 2, 2, 1, 3});

    auto intIndices = device->CreateIndexBuffer(BufferHint::StaticDraw, 3 * sizeof(std::uint32_t));
    intIndices->CopyFromSystemMemory(std::vector<std::uint32_t>{0, 1, 2});
    CHECK(intIndices->Datatype() == IndexBufferDatatype::UnsignedInt);
    CHECK(intIndices->Count() == 3);

    CHECK(SizeInBytes(IndexBufferDatatype::UnsignedShort) == 2);
    CHECK(SizeInBytes(IndexBufferDatatype::UnsignedInt) == 4);
}

TEST_CASE("Creating an empty buffer throws") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    CHECK_THROWS_AS(device->CreateVertexBuffer(BufferHint::StaticDraw, 0), std::invalid_argument);
    CHECK_THROWS_AS(device->CreateIndexBuffer(BufferHint::StaticDraw, 0), std::invalid_argument);
}
```

**Build and run `arda_tests` now.** The buffer tests should pass. If
`CopyToSystemMemory` returns zeros, check that `CopyFromSystemMemoryBytes`
is marked `override` and actually calls `m_buffer.CopyFromSystemMemory`.

Try uncommenting a line like
`buffer->CopyFromSystemMemory(std::list<float>{1, 2});` to see the concept's
error message, then remove it again.

---

## Part A2: vertex arrays (3.5.3, 3.5.4)

A buffer is just bytes. A vertex array says how to read them: "location 0
is three floats per vertex from buffer P, location 3 is two floats per
vertex from buffer T, and the indices are in buffer I".

### A2.1 `vertexarray/ComponentDatatype.h`

The type of one component of a vertex attribute, as stored in the buffer
(`ComponentDatatype.cs`). `SizeInBytes` ports `VertexArraySizes.SizeOf`.

```cpp
#pragma once

#include <stdexcept>

namespace arda::renderer {

// The type of each component of a vertex attribute in a vertex buffer.
enum class ComponentDatatype {
    Byte,
    UnsignedByte,
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Float,
    HalfFloat,
};

// VertexArraySizes.cs, SizeOf(ComponentDatatype)
constexpr int SizeInBytes(ComponentDatatype type) {
    switch (type) {
    case ComponentDatatype::Byte:
    case ComponentDatatype::UnsignedByte:  return 1;
    case ComponentDatatype::Short:
    case ComponentDatatype::UnsignedShort:
    case ComponentDatatype::HalfFloat:     return 2;
    case ComponentDatatype::Int:
    case ComponentDatatype::UnsignedInt:
    case ComponentDatatype::Float:         return 4;
    }
    throw std::invalid_argument("Invalid ComponentDatatype");
}

} // namespace arda::renderer
```

A `constexpr` function can contain a `throw`, as long as it isn't reached
during compile-time evaluation. `static_assert(SizeInBytes(ComponentDatatype::Float) == 4)`
works; `SizeInBytes(static_cast<ComponentDatatype>(99))` in a constant
expression is a compile error.

### A2.2 `vertexarray/VertexBufferAttribute.h` (Listing 3.20)

One `VertexBufferAttribute` describes one shader input: which buffer, how
many components, what type, whether to normalize, where the first one
starts and how far apart they are.

In C# this is a class with read-only properties. Here it's a struct with
public fields (the README's naming convention for plain data) and a
constructor that validates. The C# class checks `numberOfComponents` and
`normalize` in the `VertexBufferAttributesGL3x` indexer rather than in the
constructor. Moving them into the constructor means a bad attribute can't
be created at all, and the D3D11 backend gets the same checks for free.

```cpp
#pragma once

#include <arda/renderer/buffers/VertexBuffer.h>
#include <arda/renderer/vertexarray/ComponentDatatype.h>

#include <memory>
#include <stdexcept>
#include <utility>

namespace arda::renderer {

// How to read one vertex attribute from a vertex buffer (3.5.3, Listing 3.20).
struct VertexBufferAttribute {
    VertexBufferAttribute(std::shared_ptr<VertexBuffer> buffer,
                          ComponentDatatype datatype,
                          int components,
                          bool normalizeValues = false,
                          int offset = 0,
                          int stride = 0)   // 0 = tightly packed: components * SizeInBytes(datatype)
        : vertexBuffer(std::move(buffer)),
          componentDatatype(datatype),
          numberOfComponents(components),
          normalize(normalizeValues),
          offsetInBytes(offset),
          strideInBytes(stride == 0 ? components * SizeInBytes(datatype) : stride) {
        if (!vertexBuffer) {
            throw std::invalid_argument("vertexBuffer is null");
        }
        if (numberOfComponents < 1 || numberOfComponents > 4) {
            throw std::invalid_argument("numberOfComponents must be between one and four");
        }
        if (offsetInBytes < 0 || strideInBytes < 0) {
            throw std::invalid_argument("offsetInBytes and strideInBytes must be >= 0");
        }
        if (normalize && (componentDatatype == ComponentDatatype::Float ||
                          componentDatatype == ComponentDatatype::HalfFloat)) {
            throw std::invalid_argument("normalize requires an integer ComponentDatatype");
        }
    }

    std::shared_ptr<VertexBuffer> vertexBuffer;
    ComponentDatatype componentDatatype;
    int numberOfComponents;
    bool normalize;
    int offsetInBytes;
    int strideInBytes;   // never 0 after construction

    bool operator==(const VertexBufferAttribute&) const = default;
};

} // namespace arda::renderer
```

The member initializer list is in declaration order. That matters because
members are always initialized in the order they're declared, whatever order
the list is written in (see [Step 0](00-setup.md)). The defaulted `==`
([Step 1](01-state-management.md)) compares the `shared_ptr`s by address, so
two attributes are equal when they read the same buffer the same way. That's
what C#'s reference comparison achieved, and `VertexArray::SetAttribute` uses
it to skip no-op changes.

> **OpenGL note — `glVertexAttribPointer`:** these fields are exactly the
> arguments of the GL call that `VertexArrayGL3x` makes later:
>
> ```cpp
> glVertexAttribPointer(location,           // which shader input: layout(location = N)
>                       numberOfComponents, // 1-4
>                       ToGL(componentDatatype),
>                       normalize,          // GL_TRUE or GL_FALSE
>                       strideInBytes,      // bytes from the start of one vertex to the next
>                       offset);            // byte offset of the first value in the buffer
> ```
>
> - **Type and conversion.** The shader always sees floats (`float`, `vec2`,
>   ...). GL converts each component from `componentDatatype` as it reads.
> - **`normalize`** only matters for integer types. With it off, the byte
>   `255` becomes `255.0`. With it on, unsigned types map to [0, 1] and
>   signed ones to [-1, 1], so `255` becomes `1.0`. Colors stored as four
>   `std::uint8_t`s use `UnsignedByte`, 4 components, normalized. Floats are
>   already floats, so normalizing them is meaningless, and the constructor
>   rejects it.
> - **Stride** is the distance between consecutive vertices. For a buffer
>   holding only positions it's `3 * 4 = 12`. For *interleaved* data
>   (position, color, position, color, ...) it's the size of the whole
>   vertex. GL also treats `0` as "tightly packed", but OpenGlobe always
>   computes the real value, because `MaximumArrayIndex` divides by it.
> - **Missing components** are filled from `(0, 0, 0, 1)`. Feeding
>   3-component positions to an `in vec4 position` gives `w = 1`, which is
>   exactly right for a position. The Step 2 test shader and the book's
>   Listing 3.31 rely on this.
> - **Pitfall — integer inputs.** `glVertexAttribPointer` always converts to
>   float. A shader input declared `in ivec4` or `in int` needs
>   `glVertexAttribIPointer` instead, or it reads garbage. OpenGlobe
>   doesn't support integer inputs and neither does this renderer; declare
>   inputs as floats.

> **C++ note — `sizeof`, alignment, padding and `offsetof`:** GL reads vertex
> data as raw bytes, so the exact memory layout of your C++ types *is* the
> vertex format. Three rules decide it:
>
> 1. **`sizeof(T)`** is the number of bytes one `T` occupies, including any
>    padding. `sizeof(float)` is 4 and `sizeof(Vector3<float>)` is 12.
> 2. **Alignment:** each type must start at an address that's a multiple of
>    its `alignof`. `float` has alignment 4, so a `float` can't start at
>    byte 1.
> 3. **Padding:** to satisfy rule 2, the compiler inserts unused bytes
>    between members and at the end of a struct.
>
> ```cpp
> struct Padded {
>     std::uint8_t flag;          // offset 0, then 3 bytes of padding
>     Vector3<float> position;    // offset 4 (alignof(float) == 4)
> };
> static_assert(sizeof(Padded) == 16);                  // not 13
> static_assert(offsetof(Padded, position) == 4);       // not 1
> ```
>
> `offsetof(Type, member)` (from `<cstddef>`) gives a member's byte offset,
> which is exactly what `offsetInBytes` wants. It's only guaranteed to work
> for *standard-layout* types: no virtual functions and all data members
> with the same access. Plain vertex structs meet that easily.
>
> A layout mistake produces no error: GL reads whatever bytes are there.
> That's why [Step 2](02-shaders.md) added
> `static_assert(sizeof(Vector3<float>) == 3 * sizeof(float))` to
> `Vector3.h`: adding a member or a virtual function to `Vector3` would
> silently break every vertex buffer. [A3.11](#a311-things-to-try) builds an
> interleaved vertex format with `offsetof` and `static_assert`.

> **C++ note — `static_assert`:** `static_assert(condition, "message")` checks
> a condition at compile time and stops the build with the message if it's
> false. The message is optional. C# has nothing equivalent:
> `Debug.Assert` runs at run time and only in debug builds. Use
> `static_assert` for anything the compiler already knows, such as sizes,
> offsets, type traits and `constexpr` results. It costs nothing at run
> time, and a broken assumption stops the build instead of producing a
> garbled mesh.

### A2.3 `vertexarray/VertexArray.h` (Listing 3.19)

In OpenGlobe, `VertexArray` is nearly empty, and `VertexArrayGL3x` plus
`VertexBufferAttributesGL3x` store the attributes, the index buffer and the
dirty flags. This port moves all of that into the **base class**, because
none of it depends on the API (README, "Base classes hold API-agnostic
state"). Backends only turn dirty entries into API calls. Step 8's
`VertexArrayD3D11` reads the same `m_slots`, `m_attributesDirty` and
`Attribute()`.

The C# indexer `va.Attributes[location] = attribute` (with `null` to
remove) becomes `SetAttribute(location, attribute)` (with `std::nullopt` to
remove). `std::optional` is covered in [Step 2](02-shaders.md).

```cpp
#pragma once

#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/vertexarray/VertexBufferAttribute.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace arda::renderer {

// Vertex attributes and an optional index buffer: everything a draw call
// reads vertices from (3.5.3, Listing 3.19). Created by Context::CreateVertexArray.
class VertexArray {
public:
    virtual ~VertexArray() = default;

    VertexArray(const VertexArray&)            = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    // location is the shader attribute location (ShaderVertexAttribute::location).
    // Pass std::nullopt to remove an attribute. Throws std::out_of_range for a bad location.
    void SetAttribute(int location, std::optional<VertexBufferAttribute> attribute);
    const std::optional<VertexBufferAttribute>& Attribute(int location) const;

    int MaximumNumberOfAttributes() const { return static_cast<int>(m_slots.size()); }
    int NumberOfAttributes() const { return m_count; }

    // nullptr removes the index buffer, so draws use glDrawArrays.
    void SetIndexBuffer(std::shared_ptr<IndexBuffer> indexBuffer);
    const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_indexBuffer; }

    // Largest vertex index that can be read from every attribute's buffer
    // (VertexBufferAttributesGL3x.cs, MaximumArrayIndex). 0 if there are no attributes.
    int MaximumArrayIndex() const;

protected:
    explicit VertexArray(int maximumNumberOfAttributes)
        : m_slots(static_cast<std::size_t>(maximumNumberOfAttributes)) {}

    // VertexBufferAttributeGL3x in C#: an attribute and whether it changed since the last Clean.
    struct AttributeSlot {
        std::optional<VertexBufferAttribute> attribute;
        bool dirty = false;
    };

    std::vector<AttributeSlot> m_slots;   // one per location
    bool m_attributesDirty = false;       // true if any slot is dirty
    std::shared_ptr<IndexBuffer> m_indexBuffer;
    bool m_indexBufferDirty = false;

private:
    int m_count = 0;
};

} // namespace arda::renderer
```

The number of slots comes from `DeviceLimits::maximumNumberOfVertexAttributes`,
which is `GL_MAX_VERTEX_ATTRIBS` (at least 16).

> **Why:** buffers are `shared_ptr`s, and a vertex array **keeps its
> buffers alive**. Each `VertexBufferAttribute` holds a
> `std::shared_ptr<VertexBuffer>`, and `m_indexBuffer` holds the index
> buffer. So this is safe:
>
> ```cpp
> {
>     auto positions = device->CreateVertexBuffer(BufferHint::StaticDraw, 36);
>     vertexArray->SetAttribute(0, VertexBufferAttribute(positions, ComponentDatatype::Float, 3));
> }   // `positions` goes out of scope, but the vertex array still owns the buffer
> ```
>
> In C#, this was `VertexArray.DisposeBuffers`: a flag saying whether
> disposing the array should also dispose its buffers, plus a `HashSet` to
> avoid disposing a shared buffer twice (see `VertexArrayGL3x.Dispose`).
> Shared ownership makes the flag unnecessary. The last owner, whoever it
> is, frees the buffer exactly once.
>
> The GL vertex array object doesn't hold a reference to its buffers: GL
> would keep using the old buffer's *name* after `glDeleteBuffers`. Holding
> `shared_ptr`s in the C++ object is what guarantees that can't happen.

### A2.4 `src/vertexarray/VertexArray.cpp`

This ports the indexer, `Count` and `MaximumArrayIndex` from
`VertexBufferAttributesGL3x.cs`, and the `IndexBuffer` setter from
`VertexArrayGL3x.cs`.

```cpp
#include <arda/renderer/vertexarray/VertexArray.h>

#include <algorithm>
#include <utility>

namespace arda::renderer {

void VertexArray::SetAttribute(int location, std::optional<VertexBufferAttribute> attribute) {
    // at() throws std::out_of_range for a location that's negative (it wraps to a huge
    // std::size_t) or past the end.
    AttributeSlot& slot = m_slots.at(static_cast<std::size_t>(location));
    if (slot.attribute == attribute) {
        return;   // no change, so nothing to re-apply
    }

    if (slot.attribute && !attribute) {
        --m_count;
    } else if (!slot.attribute && attribute) {
        ++m_count;
    }

    slot.attribute = std::move(attribute);
    slot.dirty = true;
    m_attributesDirty = true;
}

const std::optional<VertexBufferAttribute>& VertexArray::Attribute(int location) const {
    return m_slots.at(static_cast<std::size_t>(location)).attribute;
}

void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer> indexBuffer) {
    m_indexBuffer = std::move(indexBuffer);
    m_indexBufferDirty = true;
}

int VertexArray::MaximumArrayIndex() const {
    int maximumArrayIndex = 0;
    for (const AttributeSlot& slot : m_slots) {
        if (slot.attribute) {
            const VertexBufferAttribute& a = *slot.attribute;
            const int numberOfVertices = static_cast<int>(a.vertexBuffer->SizeInBytes()) / a.strideInBytes;
            maximumArrayIndex = std::max(numberOfVertices - 1, maximumArrayIndex);
        }
    }
    return maximumArrayIndex;
}

} // namespace arda::renderer
```

`SetAttribute` takes the optional **by value** and moves it into the slot.
That's the "sink" pattern from [Step 1](01-state-management.md): callers
passing a temporary pay for one move and no copies.

`MaximumArrayIndex` computes the value on every call. OpenGlobe caches it in
`Clean` and throws if it's read while the attributes are dirty. Computing it
here means it's always correct, and D3D11 can use it without its own
`Clean`. The loop visits 16 or so slots per draw, which doesn't matter
yet. Cache it if it ever shows up in a profiler.

`MaximumArrayIndex` uses the *largest* attribute buffer, as OpenGlobe does.
If one attribute's buffer is shorter than another's, a draw can read past
its end. Keep attribute buffers the same length.

### A2.5 `Context.h` and `Context.cpp`: `CreateVertexArray`

Vertex arrays are created by the **context**, not the device.

> **Why:** a GL vertex array object is a *container object*: it only holds
> references to other objects (buffers). GL shares buffers, textures,
> shaders and samplers between contexts that were created as sharing, but
> **never container objects**: VAOs and framebuffers. A VAO created in the
> hidden share context doesn't exist in the window's context. Binding it
> there is `GL_INVALID_OPERATION`, or worse, binds an unrelated VAO that
> happens to have the same name.
>
> Putting `CreateVertexArray` on `Context` makes that rule part of the
> API: you can only get a vertex array from the context that will draw
> with it. Step 6 does the same for framebuffers. D3D11 has no such
> restriction, but following the stricter API costs nothing.

Add to `Context.h` (the full file is shown in [A3.4](#a34-contexth-and-contextcpp-draw)):

```cpp
#include <arda/renderer/vertexarray/VertexArray.h>

#include <memory>

class Context {
public:
    // ...existing members...

    // Creates an empty vertex array that belongs to this context (3.2, 3.5.3).
    std::shared_ptr<VertexArray> CreateVertexArray() { return DoCreateVertexArray(); }

protected:
    // ...existing Do* functions...

    virtual std::shared_ptr<VertexArray> DoCreateVertexArray() = 0;
};
```

### A2.6 GL: `TypeConverterGL3x`, `ToGL(ComponentDatatype)`

```cpp
// TypeConverterGL3x.h: new include and declaration
#include <arda/renderer/vertexarray/ComponentDatatype.h>

GLenum ToGL(ComponentDatatype type);
```

```cpp
// TypeConverterGL3x.cpp
GLenum ToGL(ComponentDatatype type) {
    switch (type) {
    case ComponentDatatype::Byte:          return GL_BYTE;
    case ComponentDatatype::UnsignedByte:  return GL_UNSIGNED_BYTE;
    case ComponentDatatype::Short:         return GL_SHORT;
    case ComponentDatatype::UnsignedShort: return GL_UNSIGNED_SHORT;
    case ComponentDatatype::Int:           return GL_INT;
    case ComponentDatatype::UnsignedInt:   return GL_UNSIGNED_INT;
    case ComponentDatatype::Float:         return GL_FLOAT;
    case ComponentDatatype::HalfFloat:     return GL_HALF_FLOAT;
    }
    throw std::invalid_argument("Invalid ComponentDatatype");
}
```

### A2.7 GL: `src/gl/vertexarray/VertexArrayGL3x.h` and `.cpp`

> **OpenGL note — vertex array objects (VAOs):** a VAO is a GL object that
> stores the vertex input setup, so one `glBindVertexArray` call restores
> all of it before a draw. Specifically, a VAO stores:
>
> - For **each attribute location** 0 to `GL_MAX_VERTEX_ATTRIBS - 1`:
>   whether it's enabled (`glEnableVertexAttribArray`), and the size, type,
>   normalized flag, stride and offset from `glVertexAttribPointer`,
>   **together with the buffer that was bound to `GL_ARRAY_BUFFER` when
>   `glVertexAttribPointer` was called**.
> - The **`GL_ELEMENT_ARRAY_BUFFER` binding**, meaning the index buffer.
>
> A VAO does **not** store:
>
> - The current `GL_ARRAY_BUFFER` binding. That's global. It only matters
>   at the moment `glVertexAttribPointer` copies it into the VAO. Binding a
>   different array buffer afterwards changes nothing.
> - The shader program, uniforms, textures or render state.
>
> The asymmetry between the two buffer targets causes the classic bug from
> [A1.6](#a16-gl-srcglbuffersbuffergl3xh-and-cpp): binding an *index*
> buffer while some VAO is bound changes that VAO.
>
> **A core profile requires a bound VAO.** In the compatibility profile,
> VAO 0 is a real, default VAO. The 3.3 core profile removed it:
> `glVertexAttribPointer`, `glEnableVertexAttribArray` and every draw call
> give `GL_INVALID_OPERATION` when VAO 0 is bound, and nothing is drawn. Many
> tutorials written for the compatibility profile never create a VAO. Their
> code draws nothing in a core context, with no message unless you check
> `glGetError`.

> **OpenGL note — `glEnableVertexAttribArray`:** each location is disabled
> by default. For a disabled location, the shader doesn't read the buffer:
> it gets a constant (by default `(0, 0, 0, 1)`) for every vertex. Forgetting
> to enable a location is a silent bug. Every position becomes the origin,
> so every triangle has zero area and nothing appears. `Attach` below always
> enables, and `Detach` disables.

`VertexArrayGL3x` owns a VAO name and applies dirty slots to it. `Clean`
combines `VertexArrayGL3x.Clean` and `VertexBufferAttributesGL3x.Clean`
from C#.

```cpp
// src/gl/vertexarray/VertexArrayGL3x.h
#pragma once

#include "gl/GLHandle.h"

#include <arda/renderer/vertexarray/VertexArray.h>

namespace arda::renderer::gl {

class VertexArrayGL3x final : public VertexArray {
public:
    // Creates the GL vertex array object in the current context.
    explicit VertexArrayGL3x(int maximumNumberOfAttributes)
        : VertexArray(maximumNumberOfAttributes), m_name(CreateVertexArrayName()) {}

    void Bind() const { glBindVertexArray(m_name.Get()); }

    // Applies changed attributes and the index buffer to the GL object. Call right after Bind().
    void Clean();

private:
    void Attach(int location, const VertexBufferAttribute& attribute);
    static void Detach(int location);

    VertexArrayName m_name;
};

} // namespace arda::renderer::gl
```

```cpp
// src/gl/vertexarray/VertexArrayGL3x.cpp
#include "gl/vertexarray/VertexArrayGL3x.h"
#include "gl/buffers/IndexBufferGL3x.h"
#include "gl/buffers/VertexBufferGL3x.h"
#include "gl/TypeConverterGL3x.h"

#include <cstddef>
#include <cstdint>

namespace arda::renderer::gl {

void VertexArrayGL3x::Clean() {
    if (m_attributesDirty) {
        for (int location = 0; location < MaximumNumberOfAttributes(); ++location) {
            AttributeSlot& slot = m_slots[static_cast<std::size_t>(location)];
            if (!slot.dirty) {
                continue;
            }
            if (slot.attribute) {
                Attach(location, *slot.attribute);
            } else {
                Detach(location);
            }
            slot.dirty = false;
        }
        m_attributesDirty = false;
    }

    if (m_indexBufferDirty) {
        // This VAO is bound, so this sets *its* index buffer, which is what we want here.
        if (m_indexBuffer) {
            static_cast<IndexBufferGL3x&>(*m_indexBuffer).Bind();
        } else {
            IndexBufferGL3x::Unbind();
        }
        m_indexBufferDirty = false;
    }
}

void VertexArrayGL3x::Attach(int location, const VertexBufferAttribute& attribute) {
    const auto index = static_cast<GLuint>(location);
    glEnableVertexAttribArray(index);

    // glVertexAttribPointer captures whichever buffer is bound to GL_ARRAY_BUFFER right now.
    static_cast<VertexBufferGL3x&>(*attribute.vertexBuffer).Bind();
    glVertexAttribPointer(index,
        attribute.numberOfComponents,
        ToGL(attribute.componentDatatype),
        attribute.normalize ? GL_TRUE : GL_FALSE,
        attribute.strideInBytes,
        reinterpret_cast<const void*>(static_cast<std::intptr_t>(attribute.offsetInBytes)));
}

void VertexArrayGL3x::Detach(int location) {
    glDisableVertexAttribArray(static_cast<GLuint>(location));
}

} // namespace arda::renderer::gl
```

`static_cast<VertexBufferGL3x&>(*attribute.vertexBuffer)` is the backend
downcast from the README (pattern 4). It's safe because a GL context only
ever receives buffers made by a GL device. [Step 2](02-shaders.md) compares
`static_cast` and `dynamic_cast`.

> **C++ note — `reinterpret_cast` of an offset to `const void*`:** the last
> parameter of `glVertexAttribPointer` (and of `glDrawElements`) is a
> `const void*`, a leftover from GL 1.1. Back then it pointed at vertex data
> in *system* memory. When a buffer is bound, GL reinterprets that "pointer"
> as a **byte offset into the buffer**. So an integer has to be passed
> through a pointer type.
>
> `static_cast` refuses to convert an integer to a pointer, because the
> conversion isn't meaningful in general. `reinterpret_cast` is the cast
> for "reinterpret these bits as a different type". It does no checking, and
> the result is implementation-defined, which on every platform GL runs on
> means "the pointer whose address is this number". Converting through
> `std::intptr_t` (an integer type as wide as a pointer, from `<cstdint>`)
> makes the width explicit, so a 32-bit `int` isn't widened in a surprising
> way:
>
> ```cpp
> const int offsetInBytes = 12;
> const void* offset = reinterpret_cast<const void*>(static_cast<std::intptr_t>(offsetInBytes));
> ```
>
> A C-style cast `(const void*)offsetInBytes` would compile too, but it
> quietly picks whichever cast works, including ones you didn't intend.
> Named casts say what's being done, and are easy to search for. GL 4.3's
> `glVertexAttribFormat` finally takes the offset as a real integer, but it
> isn't available in 3.3. C# hid all this: OpenTK has an overload of
> `VertexAttribPointer` that takes an `int` offset.

**Destroying a `VertexArrayGL3x`:** `VertexArrayName`'s destructor calls
`glDeleteVertexArrays` on *whatever context is current*. If a different
context is current at that moment, the call deletes nothing, or deletes that
context's VAO with the same number. With one window this never happens. In
`main`, declare the vertex arrays (and the `DrawState`s holding them) after
the window, so they're destroyed first while the window's context is
current.

### A2.8 GL: `ContextGL3x::DoCreateVertexArray`

```cpp
// ContextGL3x.h: add to the protected section
std::shared_ptr<VertexArray> DoCreateVertexArray() override;
```

```cpp
// ContextGL3x.cpp: new includes
#include "gl/vertexarray/VertexArrayGL3x.h"

#include <arda/renderer/Device.h>

#include <memory>

// ContextGL3x.cpp: new function
std::shared_ptr<VertexArray> ContextGL3x::DoCreateVertexArray() {
    MakeCurrent();   // the GL vertex array object must belong to this context
    return std::make_shared<VertexArrayGL3x>(GetDevice().Limits().maximumNumberOfVertexAttributes);
}
```

`ContextGL3x.cs` doesn't call `MakeCurrent` here. It's added because a
program with two windows would otherwise create the VAO in whichever
window was made current last. `glfwMakeContextCurrent` isn't free, but
creating vertex arrays is rare.

### A2.9 Checkpoint 2: vertex arrays

**CMake:**

```cmake
# renderer/CMakeLists.txt, in add_library(arda_renderer ...)
    src/vertexarray/VertexArray.cpp
    include/arda/renderer/vertexarray/ComponentDatatype.h
    include/arda/renderer/vertexarray/VertexArray.h
    include/arda/renderer/vertexarray/VertexBufferAttribute.h

# renderer/CMakeLists.txt, in the ARDA_RENDERER_GL target_sources(...)
    src/gl/vertexarray/VertexArrayGL3x.cpp
    src/gl/vertexarray/VertexArrayGL3x.h
```

**Tests.** Add these includes to the top of `VertexDataTests.cpp`:

```cpp
#include <arda/renderer/Context.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/vertexarray/ComponentDatatype.h>
#include <arda/renderer/vertexarray/VertexArray.h>
#include <arda/renderer/vertexarray/VertexBufferAttribute.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <memory>
#include <optional>
```

Then add these test cases. Vertex arrays need a context, so these tests open
a hidden window, as in [Step 0](00-setup.md)'s tests.

```cpp
static_assert(SizeInBytes(ComponentDatatype::UnsignedByte) == 1);
static_assert(SizeInBytes(ComponentDatatype::HalfFloat) == 2);
static_assert(SizeInBytes(ComponentDatatype::Float) == 4);

TEST_CASE("VertexBufferAttribute validates its arguments and computes a packed stride") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto buffer = device->CreateVertexBuffer(BufferHint::StaticDraw, 36);

    const VertexBufferAttribute attribute(buffer, ComponentDatatype::Float, 3);
    CHECK(attribute.strideInBytes == 12);
    CHECK(attribute.offsetInBytes == 0);
    CHECK_FALSE(attribute.normalize);
    CHECK(attribute == VertexBufferAttribute(buffer, ComponentDatatype::Float, 3));

    CHECK(VertexBufferAttribute(buffer, ComponentDatatype::Float, 3, false, 12, 24).strideInBytes == 24);
    CHECK_NOTHROW(VertexBufferAttribute(buffer, ComponentDatatype::UnsignedByte, 4, true));

    CHECK_THROWS_AS(VertexBufferAttribute(nullptr, ComponentDatatype::Float, 3), std::invalid_argument);
    CHECK_THROWS_AS(VertexBufferAttribute(buffer, ComponentDatatype::Float, 0), std::invalid_argument);
    CHECK_THROWS_AS(VertexBufferAttribute(buffer, ComponentDatatype::Float, 5), std::invalid_argument);
    CHECK_THROWS_AS(VertexBufferAttribute(buffer, ComponentDatatype::Float, 3, true), std::invalid_argument);
    CHECK_THROWS_AS(VertexBufferAttribute(buffer, ComponentDatatype::Float, 3, false, -4), std::invalid_argument);
}

TEST_CASE("VertexArray tracks attributes and the largest vertex index") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 64, "test", WindowType::Hidden);
    Context& context = window->GetContext();

    auto positions = device->CreateVertexBuffer(BufferHint::StaticDraw, 3 * 3 * sizeof(float));   // 3 vec3s
    auto textureCoordinates = device->CreateVertexBuffer(BufferHint::StaticDraw, 4 * 2 * sizeof(float));   // 4 vec2s

    auto vertexArray = context.CreateVertexArray();
    CHECK(vertexArray->MaximumNumberOfAttributes() == device->Limits().maximumNumberOfVertexAttributes);
    CHECK(vertexArray->NumberOfAttributes() == 0);
    CHECK(vertexArray->MaximumArrayIndex() == 0);
    CHECK_FALSE(vertexArray->GetIndexBuffer());

    vertexArray->SetAttribute(VertexLocations::Position,
        VertexBufferAttribute(positions, ComponentDatatype::Float, 3));
    CHECK(vertexArray->NumberOfAttributes() == 1);
    CHECK(vertexArray->MaximumArrayIndex() == 2);

    vertexArray->SetAttribute(VertexLocations::TextureCoordinate,
        VertexBufferAttribute(textureCoordinates, ComponentDatatype::Float, 2));
    CHECK(vertexArray->NumberOfAttributes() == 2);
    CHECK(vertexArray->MaximumArrayIndex() == 3);

    // Setting an equal attribute again changes nothing.
    vertexArray->SetAttribute(VertexLocations::Position,
        VertexBufferAttribute(positions, ComponentDatatype::Float, 3));
    CHECK(vertexArray->NumberOfAttributes() == 2);

    vertexArray->SetAttribute(VertexLocations::TextureCoordinate, std::nullopt);
    CHECK(vertexArray->NumberOfAttributes() == 1);
    CHECK_FALSE(vertexArray->Attribute(VertexLocations::TextureCoordinate).has_value());
    CHECK(vertexArray->MaximumArrayIndex() == 2);

    CHECK_THROWS_AS(vertexArray->SetAttribute(-1, std::nullopt), std::out_of_range);
    CHECK_THROWS_AS(vertexArray->SetAttribute(vertexArray->MaximumNumberOfAttributes(), std::nullopt),
                    std::out_of_range);
}

TEST_CASE("A vertex array keeps its buffers alive") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 64, "test", WindowType::Hidden);
    auto vertexArray = window->GetContext().CreateVertexArray();

    std::weak_ptr<VertexBuffer> observer;
    {
        auto buffer = device->CreateVertexBuffer(BufferHint::StaticDraw, 12);
        observer = buffer;
        vertexArray->SetAttribute(0, VertexBufferAttribute(buffer, ComponentDatatype::Float, 3));
        CHECK(observer.use_count() == 2);   // `buffer`, and the attribute inside the vertex array
    }
    CHECK_FALSE(observer.expired());        // the vertex array is now the only owner

    vertexArray->SetAttribute(0, std::nullopt);
    CHECK(observer.expired());              // last owner gone: glDeleteBuffers has run
}
```

The last test shows shared ownership in action. `std::weak_ptr` observes a
buffer without owning it; it's explained in [A3.7](#a37-gl-contextgl3x-drawing).

**Build and run `arda_tests`.** `Clean` doesn't run yet (nothing draws), so
these tests check the base-class bookkeeping and that VAO creation works.

---

## Part A3: drawing (3.5.4) and the first triangle

### A3.1 `core/include/arda/core/geometry/PrimitiveType.h`

`PrimitiveType` is declared in OpenGlobe's `Core/Geometry/Mesh.cs`, because
meshes store it (Part B). It lives in core here too, so core never depends
on the renderer.

```cpp
#pragma once

namespace arda::core::geometry {

// How vertices are assembled into primitives (Mesh.cs).
enum class PrimitiveType {
    Points,
    Lines,
    LineLoop,        // not supported by Direct3D 11
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan,     // not supported by Direct3D 11
    LinesAdjacency,
    LineStripAdjacency,
    TrianglesAdjacency,
    TriangleStripAdjacency,
};

} // namespace arda::core::geometry
```

### A3.2 `scene/SceneState.h` (empty for now)

`Draw` takes a `SceneState` (3.4.5): the camera, sun position and other
per-scene values that automatic uniforms read. Declaring it now, empty,
means `Draw`'s signature doesn't change in Step 4, which rewrites this
file.

```cpp
#pragma once

namespace arda::renderer {

// Scene-level state used by automatic uniforms. Step 4 adds the camera and matrices.
class SceneState {};

} // namespace arda::renderer
```

### A3.3 `DrawState.h` (Listing 3.5)

This replaces the file from [Step 1](01-state-management.md), adding the
two fields that step left out.

```cpp
#pragma once

#include <arda/renderer/renderstate/RenderState.h>

#include <memory>

namespace arda::renderer {

class ShaderProgram;
class VertexArray;

// Everything a draw call needs besides the primitive type and scene state (Listing 3.5).
struct DrawState {
    RenderState renderState;
    std::shared_ptr<ShaderProgram> shaderProgram;
    std::shared_ptr<VertexArray> vertexArray;
};

} // namespace arda::renderer
```

Forward declarations (see [Step 0](00-setup.md)) are enough here, because a
`shared_ptr<T>` member doesn't need `T`'s definition. Code that *uses* the
program or vertex array includes their headers.

A `DrawState` holds `shared_ptr`s, so copying one is cheap and the copy
shares the same program and vertex array. That makes it easy to have
several draw states that differ only in render state.

### A3.4 `Context.h` and `Context.cpp`: `Draw`

Here is the whole of `Context.h` as it stands after Part A of this step.
It combines [Step 0](00-setup.md), [Step 1](01-state-management.md) and
the additions from A2.5 and this section.

```cpp
#pragma once

#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/scene/SceneState.h>
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

    // Step 1
    void Clear(const ClearState& clearState) { DoClear(clearState); }
    const Rectangle& GetViewport() const { return m_viewport; }
    void SetViewport(const Rectangle& viewport);

    // Step 3. Vertex arrays can't be shared between contexts, so the context creates them.
    std::shared_ptr<VertexArray> CreateVertexArray() { return DoCreateVertexArray(); }
    // Part B adds the MeshBuffers and Mesh overloads here.

    // Draws every vertex, or every index if the vertex array has an index buffer.
    void Draw(core::geometry::PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState);

    // Draws count vertices (or indices), starting at vertex (or index) offset.
    void Draw(core::geometry::PrimitiveType primitiveType, int offset, int count,
              const DrawState& drawState, const SceneState& sceneState);

protected:
    explicit Context(Device& device);

    virtual void DoClear(const ClearState& clearState) = 0;
    virtual void DoSetViewport(const Rectangle& viewport) = 0;

    virtual std::shared_ptr<VertexArray> DoCreateVertexArray() = 0;

    // Called after VerifyDraw, so drawState.shaderProgram and drawState.vertexArray are set.
    virtual void DoDraw(core::geometry::PrimitiveType primitiveType,
                        const DrawState& drawState, const SceneState& sceneState) = 0;
    virtual void DoDrawRange(core::geometry::PrimitiveType primitiveType, int offset, int count,
                             const DrawState& drawState, const SceneState& sceneState) = 0;

    // Checks shared by every backend (ContextGL3x.cs, VerifyDraw).
    void VerifyDraw(const DrawState& drawState) const;

private:
    Device& m_device;
    Rectangle m_viewport;
};

} // namespace arda::renderer
```

The two `Draw` overloads are non-virtual, and each backend implements
`DoDraw` and `DoDrawRange`. This is the non-virtual interface pattern from
the README. Two virtual overloads named `Draw` would cause a trap: a
derived class that overrides one of them hides the other (see
[Step 0](00-setup.md)).

`src/Context.cpp`, in full:

```cpp
#include <arda/renderer/Context.h>

#include <stdexcept>

namespace arda::renderer {

Context::Context(Device& device) : m_device(device) {}

// Step 1
void Context::SetViewport(const Rectangle& viewport) {
    if (viewport.width < 0 || viewport.height < 0) {
        throw std::invalid_argument("SetViewport: width and height must be greater than or equal to zero");
    }
    if (viewport != m_viewport) {
        m_viewport = viewport;
        DoSetViewport(viewport);
    }
}

// Step 3
void Context::Draw(core::geometry::PrimitiveType primitiveType,
                   const DrawState& drawState, const SceneState& sceneState) {
    VerifyDraw(drawState);
    DoDraw(primitiveType, drawState, sceneState);
}

void Context::Draw(core::geometry::PrimitiveType primitiveType, int offset, int count,
                   const DrawState& drawState, const SceneState& sceneState) {
    if (offset < 0 || count < 0) {
        throw std::invalid_argument("Draw: offset and count must be >= 0");
    }
    VerifyDraw(drawState);
    DoDrawRange(primitiveType, offset, count, drawState, sceneState);
}

// ContextGL3x.cs VerifyDraw. Step 6 adds the depth attachment check.
void Context::VerifyDraw(const DrawState& drawState) const {
    if (!drawState.shaderProgram) {
        throw std::invalid_argument("drawState.shaderProgram is null");
    }
    if (!drawState.vertexArray) {
        throw std::invalid_argument("drawState.vertexArray is null");
    }
}

} // namespace arda::renderer
```

> **Why:** `VerifyDraw` checks what it checks because each of those
> mistakes makes the backend crash or draw nothing, with no explanation:
>
> - **A null program or vertex array** would be dereferenced by
>   `DoDraw`, which is undefined behavior (usually a crash deep inside the
>   backend). Throwing `std::invalid_argument` with the field's name
>   points straight at the bug.
> - **C#'s other null checks disappear.** C# also checks `drawState`,
>   `drawState.RenderState` and `sceneState` for null. In C++ they're
>   references and a value member, which can't be null.
> - **A negative offset or count** would wrap to a huge unsigned value
>   inside GL. C# doesn't check this; it costs nothing to add.
> - **Step 6** adds "depth test enabled but the framebuffer has no depth
>   buffer", a mistake GL never reports.
>
> The checks run in the base class, before the backend, so D3D11 gets the
> same messages. Things only the backend can check, such as GL program
> validation, stay in the backend (`ApplyShaderProgram` below).
>
> **Not checked:** that every shader input has an attribute in the vertex
> array. On GL, a missing attribute silently reads `(0, 0, 0, 1)`. On
> D3D11, input-layout creation fails. If you want the check, add it here.
> It needs `ShaderProgram.h`:
>
> ```cpp
> for (const ShaderVertexAttribute& input : drawState.shaderProgram->VertexAttributes()) {
>     if (!drawState.vertexArray->Attribute(input.location)) {
>         throw std::invalid_argument("The vertex array has no attribute for shader input '" + input.name + "'");
>     }
> }
> ```
>
> OpenGlobe doesn't check this, and it's left out of the base design so the
> Step 3 code matches the book.

### A3.5 GL: `TypeConverterGL3x`, `PrimitiveType` and `IndexBufferDatatype`

```cpp
// TypeConverterGL3x.h: new includes and declarations
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/buffers/IndexBuffer.h>

GLenum ToGL(core::geometry::PrimitiveType type);
GLenum ToGL(IndexBufferDatatype type);
```

```cpp
// TypeConverterGL3x.cpp
GLenum ToGL(core::geometry::PrimitiveType type) {
    using core::geometry::PrimitiveType;
    switch (type) {
    case PrimitiveType::Points:                 return GL_POINTS;
    case PrimitiveType::Lines:                  return GL_LINES;
    case PrimitiveType::LineLoop:               return GL_LINE_LOOP;
    case PrimitiveType::LineStrip:              return GL_LINE_STRIP;
    case PrimitiveType::Triangles:              return GL_TRIANGLES;
    case PrimitiveType::TriangleStrip:          return GL_TRIANGLE_STRIP;
    case PrimitiveType::TriangleFan:            return GL_TRIANGLE_FAN;
    case PrimitiveType::LinesAdjacency:         return GL_LINES_ADJACENCY;
    case PrimitiveType::LineStripAdjacency:     return GL_LINE_STRIP_ADJACENCY;
    case PrimitiveType::TrianglesAdjacency:     return GL_TRIANGLES_ADJACENCY;
    case PrimitiveType::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
    }
    throw std::invalid_argument("Invalid PrimitiveType");
}

GLenum ToGL(IndexBufferDatatype type) {
    switch (type) {
    case IndexBufferDatatype::UnsignedShort: return GL_UNSIGNED_SHORT;
    case IndexBufferDatatype::UnsignedInt:   return GL_UNSIGNED_INT;
    }
    throw std::invalid_argument("Invalid IndexBufferDatatype");
}
```

The adjacency primitives are only useful with a geometry shader. Without
one, GL draws them as ordinary lines or triangles and ignores the extra
vertices.

### A3.6 GL: `ShaderProgramGL3x::Clean`

[Step 2](02-shaders.md) collects changed uniforms in `m_dirtyUniforms`.
`Clean` uploads them just before a draw. It takes the context, draw state and
scene state because Step 4 sets automatic uniforms from them first.

```cpp
// ShaderProgramGL3x.h: before `namespace arda::renderer::gl {`
namespace arda::renderer {
class Context;
struct DrawState;
class SceneState;
} // namespace arda::renderer

// ShaderProgramGL3x.h: add to the public section of ShaderProgramGL3x
// Uploads every uniform that changed since the last draw. Call after Bind().
void Clean(Context& context, const DrawState& drawState, const SceneState& sceneState);
```

```cpp
// ShaderProgramGL3x.cpp
void ShaderProgramGL3x::Clean([[maybe_unused]] Context& context,
                              [[maybe_unused]] const DrawState& drawState,
                              [[maybe_unused]] const SceneState& sceneState) {
    // Step 4 adds: SetDrawAutomaticUniforms(context, drawState, sceneState);
    for (ICleanable* uniform : m_dirtyUniforms) {
        uniform->Clean();
    }
    m_dirtyUniforms.clear();
}
```

`[[maybe_unused]]` silences "unused parameter" warnings until Step 4 uses
the parameters. The forward declarations must match how the types are
defined (`struct DrawState`, `class SceneState`). A mismatch is legal, but
MSVC warns about it (C4099).

`glUniform*` calls affect the program that is *currently in use*, so
`Clean` must run after `Bind()` (`glUseProgram`). `ApplyShaderProgram`
below does both in that order.

### A3.7 GL: `ContextGL3x` drawing

This is the port of `ContextGL3x.cs`'s `Draw` methods and the `Apply*`
methods they call.

> **OpenGL note — draw calls:**
>
> - **`glDrawArrays(mode, first, count)`** reads vertices `first` to
>   `first + count - 1` straight from the attribute buffers, in order.
>   Every three vertices make a triangle (for `GL_TRIANGLES`). Shared
>   corners must be repeated.
> - **`glDrawElements(mode, count, type, offset)`** reads `count` indices of
>   `type` from the bound VAO's element buffer, starting `offset` **bytes**
>   in. It then fetches the vertex each index names. Shared corners are
>   stored once and referenced many times, and the GPU's post-transform
>   cache can reuse the shaded result.
> - **`glDrawRangeElements(mode, start, end, count, type, offset)`** is
>   `glDrawElements` plus a promise that every index is between `start` and
>   `end`. The driver can use that to prepare only that range of vertices.
>   If the promise is broken, the spec says the results depend on the
>   implementation, which means anything from correct output to garbage. OpenGlobe passes
>   `0` and `MaximumArrayIndex()`, which is always true for valid indices.
>
> **Offsets are in bytes for indices but in vertices for `glDrawArrays`.**
> `DoDrawRange` below converts "start at index 6" into
> `6 * sizeof(index)` bytes for `glDrawRangeElements`, but passes `first`
> straight through to `glDrawArrays`.

> **OpenGL note — primitive restart:** with `GL_PRIMITIVE_RESTART` enabled,
> a special index value ends the current strip, loop or fan, and the next
> index starts a new one. Many triangle strips (such as the rows of a
> terrain grid) can then be drawn in one call:
>
> ```
> indices: 0 1 2 3 0xFFFF 4 5 6 7     (two separate strips)
> ```
>
> `glPrimitiveRestartIndex(i)` sets that special value. D3D11 always uses
> the maximum value for the index type (`0xFFFF` or `0xFFFFFFFF`), and
> GL 4.3 added a matching fixed mode. To behave the same in 3.3, the README
> fixes the restart index to the maximum, so `RenderState::primitiveRestart`
> has only `enabled`. The GL backend sets the index to match each draw's
> index type. [Step 1](01-state-management.md)'s `ApplyPrimitiveRestart`
> handles the enable flag, and `ApplyPrimitiveRestartIndex` below handles the
> value.

Add these declarations to `ContextGL3x.h` (the class is from
[Step 1](01-state-management.md)):

```cpp
// ContextGL3x.h: new includes
#include <arda/renderer/buffers/IndexBuffer.h>

#include <memory>
#include <optional>

// ContextGL3x.h: add to the protected section, after DoCreateVertexArray
void DoDraw(core::geometry::PrimitiveType primitiveType,
            const DrawState& drawState, const SceneState& sceneState) override;
void DoDrawRange(core::geometry::PrimitiveType primitiveType, int offset, int count,
                 const DrawState& drawState, const SceneState& sceneState) override;

// ContextGL3x.h: add to the private functions, after the Step 1 Apply* functions
void ApplyBeforeDraw(const DrawState& drawState, const SceneState& sceneState);
void ApplyVertexArray(VertexArray& vertexArray);
void ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState);
void ApplyPrimitiveRestartIndex(IndexBufferDatatype datatype);

// ContextGL3x.h: add to the private data members, after m_clearStencil

// The program in use (glUseProgram). A weak_ptr, not a raw pointer: if the program
// is destroyed and a new one is created at the same address, the new one must still be bound.
std::weak_ptr<ShaderProgram> m_boundShaderProgram;

// The index type glPrimitiveRestartIndex was last set for. Empty until the first indexed draw.
std::optional<IndexBufferDatatype> m_primitiveRestartDatatype;
```

`ShaderProgram` is already forward-declared through `Context.h` →
`DrawState.h`, which is enough to declare a `weak_ptr` to it.

And the implementation in `ContextGL3x.cpp`:

```cpp
// ContextGL3x.cpp: new includes, with the one from A2.8
#include "gl/shaders/ShaderProgramGL3x.h"
#include "gl/vertexarray/VertexArrayGL3x.h"

#include <arda/renderer/Device.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

// ContextGL3x.cpp: inside namespace arda::renderer::gl, after DoCreateVertexArray
using core::geometry::PrimitiveType;

void ContextGL3x::DoDraw(PrimitiveType primitiveType, const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(drawState, sceneState);

    const VertexArray& vertexArray = *drawState.vertexArray;
    if (const auto& indexBuffer = vertexArray.GetIndexBuffer()) {
        ApplyPrimitiveRestartIndex(indexBuffer->Datatype());
        glDrawRangeElements(ToGL(primitiveType),
            0, static_cast<GLuint>(vertexArray.MaximumArrayIndex()),
            static_cast<GLsizei>(indexBuffer->Count()),
            ToGL(indexBuffer->Datatype()),
            nullptr);   // start at byte 0 of the index buffer
    } else {
        glDrawArrays(ToGL(primitiveType), 0, vertexArray.MaximumArrayIndex() + 1);
    }
}

void ContextGL3x::DoDrawRange(PrimitiveType primitiveType, int offset, int count,
                              const DrawState& drawState, const SceneState& sceneState) {
    ApplyBeforeDraw(drawState, sceneState);

    const VertexArray& vertexArray = *drawState.vertexArray;
    if (const auto& indexBuffer = vertexArray.GetIndexBuffer()) {
        ApplyPrimitiveRestartIndex(indexBuffer->Datatype());
        // offset counts indices; GL wants bytes.
        const auto byteOffset = static_cast<std::intptr_t>(offset) *
                                static_cast<std::intptr_t>(SizeInBytes(indexBuffer->Datatype()));
        glDrawRangeElements(ToGL(primitiveType),
            0, static_cast<GLuint>(vertexArray.MaximumArrayIndex()),
            count,
            ToGL(indexBuffer->Datatype()),
            reinterpret_cast<const void*>(byteOffset));
    } else {
        glDrawArrays(ToGL(primitiveType), offset, count);
    }
}

// ContextGL3x.cs ApplyBeforeDraw. The order matters: the vertex array is bound
// before the program is validated, because validation checks the current state.
void ContextGL3x::ApplyBeforeDraw(const DrawState& drawState, const SceneState& sceneState) {
    ApplyRenderState(drawState.renderState);
    ApplyVertexArray(*drawState.vertexArray);
    ApplyShaderProgram(drawState, sceneState);
    // Step 5 adds: CleanTextureUnits();
    // Step 6 adds: ApplyFramebuffer();
}

void ContextGL3x::ApplyVertexArray(VertexArray& vertexArray) {
    auto& vertexArrayGL = static_cast<VertexArrayGL3x&>(vertexArray);
    vertexArrayGL.Bind();    // always: buffer creation and updates unbind it (see BufferGL3x)
    vertexArrayGL.Clean();   // then apply any attribute or index buffer changes to it
}

void ContextGL3x::ApplyShaderProgram(const DrawState& drawState, const SceneState& sceneState) {
    auto& program = static_cast<ShaderProgramGL3x&>(*drawState.shaderProgram);

    if (m_boundShaderProgram.lock() != drawState.shaderProgram) {
        program.Bind();
        m_boundShaderProgram = drawState.shaderProgram;
    }
    program.Clean(*this, drawState, sceneState);

#ifndef NDEBUG
    // Asks GL whether the program can run with the current state. Slow, so debug builds only.
    glValidateProgram(program.Handle());
    GLint status = GL_FALSE;
    glGetProgramiv(program.Handle(), GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
        throw std::invalid_argument("Shader program validation failed: " + program.Log());
    }
#endif
}

// The restart index is always the maximum value for the index type (see PrimitiveRestart.h).
void ContextGL3x::ApplyPrimitiveRestartIndex(IndexBufferDatatype datatype) {
    if (m_primitiveRestartDatatype != datatype) {
        glPrimitiveRestartIndex(datatype == IndexBufferDatatype::UnsignedShort ? 0xFFFFu : 0xFFFFFFFFu);
        m_primitiveRestartDatatype = datatype;
    }
}
```

What the code does, and how it differs from the C#:

- **The two draw functions mirror `ContextGL3x.cs` exactly**, except that
  verification has moved to the base class and `DoDrawRange` checks
  `offset` and `count` first.
- **`if (const auto& indexBuffer = ...)`** declares a variable inside the
  `if` condition. The body runs only when the `shared_ptr` is non-null.
  C# did the same with `as IndexBufferGL3x` and a null check.
- **The vertex array is bound on every draw**, not cached. Buffer uploads
  unbind it (A1.6), so a cache would need to be invalidated from
  `BufferGL3x`. The bind is cheap enough not to bother.
- **The program *is* cached**, because `glUseProgram` can be expensive. The
  cache is a `weak_ptr`.
- **The restart index is set only when the index type changes.**
  `std::optional`'s `!=` compares an empty optional as unequal to any value,
  so the first indexed draw always sets it.

> **C++ note — `std::weak_ptr`:** a `weak_ptr` refers to an object owned by
> `shared_ptr`s **without owning it**. It doesn't keep the object alive;
> it can only tell you whether the object still exists. To use the
> object, call `lock()`. That returns a `shared_ptr` which is either an
> owner (the object is alive) or empty (it has been destroyed).
>
> ```cpp
> std::weak_ptr<ShaderProgram> cached = program;   // doesn't change program's reference count
> program.reset();                                 // last owner gone: the program is destroyed
> bool gone = cached.expired();                    // true
> std::shared_ptr<ShaderProgram> p = cached.lock(); // empty
> ```
>
> **Why not a raw `ShaderProgram*`?** When a program is destroyed, a new
> program can be allocated at the *same address*. A raw-pointer cache would
> then say "already bound" and skip `glUseProgram` for a program GL has
> never seen. The expired `weak_ptr` locks to `nullptr`, which never equals
> a live program, so the new program is always bound. Holding a
> `shared_ptr` in the cache instead would work too, but it would keep the
> last-used program alive after the application has let it go.
>
> `weak_ptr` is also how a `shared_ptr` cycle is broken: the "back" link in
> a parent/child pair is made weak. Step 6 caches the bound framebuffer
> the same way.

> **Pitfall — GL errors are silent.** A GL call with bad arguments sets an
> error flag and does nothing, and no exception is thrown. If a draw shows
> nothing, call `glGetError()` after it in a debug build, or use a frame
> debugger such as [RenderDoc](https://renderdoc.org/), which shows every GL
> call, the bound VAO, the buffer contents and the output of each draw.
> RenderDoc supports GL 3.2+ core profiles, and it's the fastest way to find
> out why a triangle is missing.

### A3.8 Checkpoint 3: drawing

**Tests.** Add these includes to `VertexDataTests.cpp`:

```cpp
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
```

and this test case. There's no way to read pixels back until Step 6, so it
checks that validation works, and that valid draws run without GL program
validation errors (in debug builds).

```cpp
TEST_CASE("Draw validates the draw state and draws with and without indices") {
    using arda::core::geometry::PrimitiveType;

    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 64, "test", WindowType::Hidden);
    Context& context = window->GetContext();

    auto shaderProgram = device->CreateShaderProgram(
        R"(layout(location = og_positionVertexLocation) in vec3 position;
           void main() { gl_Position = vec4(position, 1.0); })",
        R"(out vec3 fragmentColor;
           void main() { fragmentColor = vec3(1.0, 0.0, 0.0); })");

    const std::vector<float> positions = {-0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f};
    auto positionBuffer = device->CreateVertexBuffer(BufferHint::StaticDraw, positions.size() * sizeof(float));
    positionBuffer->CopyFromSystemMemory(positions);

    auto vertexArray = context.CreateVertexArray();
    vertexArray->SetAttribute(VertexLocations::Position,
        VertexBufferAttribute(positionBuffer, ComponentDatatype::Float, 3));

    DrawState drawState;
    SceneState sceneState;

    CHECK_THROWS_AS(context.Draw(PrimitiveType::Triangles, drawState, sceneState), std::invalid_argument);
    drawState.shaderProgram = shaderProgram;
    CHECK_THROWS_AS(context.Draw(PrimitiveType::Triangles, drawState, sceneState), std::invalid_argument);
    drawState.vertexArray = vertexArray;

    CHECK_NOTHROW(context.Draw(PrimitiveType::Triangles, drawState, sceneState));        // glDrawArrays
    CHECK_NOTHROW(context.Draw(PrimitiveType::Triangles, 0, 3, drawState, sceneState));
    CHECK_THROWS_AS(context.Draw(PrimitiveType::Triangles, -1, 3, drawState, sceneState), std::invalid_argument);

    auto indexBuffer = device->CreateIndexBuffer(BufferHint::StaticDraw, 3 * sizeof(std::uint16_t));
    indexBuffer->CopyFromSystemMemory(std::array<std::uint16_t, 3>{0, 1, 2});
    vertexArray->SetIndexBuffer(indexBuffer);

    CHECK_NOTHROW(context.Draw(PrimitiveType::Triangles, drawState, sceneState));        // glDrawRangeElements
    CHECK_NOTHROW(context.Draw(PrimitiveType::LineLoop, 0, 3, drawState, sceneState));
}
```

**Build and run `arda_tests`.** Then on to the triangle.

### A3.9 Milestone 3A: a triangle from raw buffers

Replace `scene/src/main.cpp` with this. It uses only public renderer types,
so it would compile unchanged against the D3D11 backend (with HLSL
shaders; see Step 7).

```cpp
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/vertexarray/VertexArray.h>
#include <arda/renderer/vertexarray/VertexBufferAttribute.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>

namespace {

// The prelude from Step 2 adds #version 330 and defines og_positionVertexLocation (0).
constexpr const char* kVertexShader = R"(
layout(location = og_positionVertexLocation) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
})";

constexpr const char* kFragmentShader = R"(
out vec3 fragmentColor;
uniform vec3 u_color;

void main()
{
    fragmentColor = u_color;
})";

} // namespace

int main() {
    using namespace arda::renderer;
    using arda::core::Vector3;
    using arda::core::geometry::PrimitiveType;

    try {
        // Declaration order is destruction order in reverse: the device outlives the
        // window, and the window (with its context) outlives every vertex array.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(800, 600, "Step 3A: Triangle");
        Context& context = window->GetContext();

        // 1. The shader program (Step 2).
        auto shaderProgram = device->CreateShaderProgram(kVertexShader, kFragmentShader);
        shaderProgram->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));

        // 2. Vertex data. With no camera yet, positions are already in clip space:
        //    x and y from -1 to 1 cover the window.
        const std::array<Vector3<float>, 3> positions = {
            Vector3<float>(-0.5f, -0.5f, 0.0f),
            Vector3<float>( 0.5f, -0.5f, 0.0f),
            Vector3<float>( 0.0f,  0.5f, 0.0f),
        };
        auto positionBuffer = device->CreateVertexBuffer(BufferHint::StaticDraw, sizeof(positions));
        positionBuffer->CopyFromSystemMemory(positions);

        // 3. Indices. Counterclockwise, so the triangle faces the viewer.
        const std::array<std::uint16_t, 3> indices = {0, 1, 2};
        auto indexBuffer = device->CreateIndexBuffer(BufferHint::StaticDraw, sizeof(indices));
        indexBuffer->CopyFromSystemMemory(indices);

        // 4. The vertex array: "location 0 reads three floats per vertex from positionBuffer".
        auto vertexArray = context.CreateVertexArray();
        vertexArray->SetAttribute(VertexLocations::Position,
            VertexBufferAttribute(positionBuffer, ComponentDatatype::Float, 3));
        vertexArray->SetIndexBuffer(indexBuffer);

        // 5. The draw state. Culling and depth testing are on by default (3.3.2); turn them
        //    off, since there is only one triangle and no depth to test.
        DrawState drawState;
        drawState.renderState.facetCulling.enabled = false;
        drawState.renderState.depthTest.enabled = false;
        drawState.shaderProgram = shaderProgram;
        drawState.vertexArray = vertexArray;

        ClearState clearState;
        clearState.color = {0.02f, 0.05f, 0.12f, 1.0f};
        SceneState sceneState;

        window->SetResizeHandler([&] {
            context.SetViewport({0, 0, window->Width(), window->Height()});
        });
        window->SetRenderFrameHandler([&] {
            context.Clear(clearState);
            context.Draw(PrimitiveType::Triangles, drawState, sceneState);
        });
        window->Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

**Build and run `arda_scene`.** You should see a red triangle on a dark
blue background, centered, filling half the window's width. This is the
moment every earlier step was building towards. Resize the window: the
triangle stretches with it, because clip space always spans the whole
viewport. Step 4's camera fixes the aspect ratio.

Here is what happens on each frame, from `Draw` down to the GPU:

1. `Context::Draw` → `VerifyDraw` (the program and vertex array are set) →
   `ContextGL3x::DoDraw`.
2. `ApplyRenderState`: disables culling and the depth test, the first time
   only (Step 1's cache).
3. `ApplyVertexArray`: `glBindVertexArray`. On the first frame, `Clean`
   runs `glEnableVertexAttribArray(0)`, binds `positionBuffer`, calls
   `glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, 0)`, and binds
   `indexBuffer` to the VAO.
4. `ApplyShaderProgram`: `glUseProgram` the first time. `Clean` uploads
   `u_color` with `glUniform3f` the first time.
5. `glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, 0)`.

From the second frame on, steps 2–4 make only the `glBindVertexArray` call,
plus program validation in debug builds. Everything else is cached.

> **C++ note — `std::array`:** `std::array<T, N>` (from `<array>`) is a
> fixed-size array whose size is part of its type. Its elements are stored
> *inside* the object, with no heap allocation, exactly like a C array
> `T[N]`. Unlike a C array, it can be copied and returned, knows its
> `size()`, has iterators, and doesn't decay to a pointer when passed to a
> function.
>
> C# arrays are always heap objects with a runtime length. `std::array` is
> closer to a C# `fixed` buffer or an inline array struct. Use it when the
> count is known at compile time (3 vertices, 4 color channels, a 16-value
> matrix), and use `std::vector` when it isn't.
>
> `sizeof` shows the difference:
>
> ```cpp
> std::array<Vector3<float>, 3> positions = {...};
> sizeof(positions);    // 36: the elements themselves
>
> std::vector<Vector3<float>> list = {...};
> sizeof(list);         // 24 (typically): three pointers, NOT the data
> list.size() * sizeof(Vector3<float>);     // 36: the data size
> std::span(list).size_bytes();             // 36: the same, via span
> ```
>
> **Pitfall:** `CreateVertexBuffer(hint, sizeof(list))` on a `std::vector`
> allocates 24 bytes, whatever the vector holds. `CopyFromSystemMemory`
> then throws `std::out_of_range`, because the data doesn't fit. Use
> `sizeof` only on `std::array`s and plain structs.

### A3.10 If you see no triangle

Work down this list. The first four cover most cases.

1. **An exception message on stderr.** Shader compile errors, validation
   failures and bad arguments all throw with a description.
2. **The clear color, but no triangle.** Check that the attribute location
   matches the shader: `VertexLocations::Position` is 0, and
   `og_positionVertexLocation` is 0. If the shader's input has a different
   location, GL reads `(0, 0, 0, 1)` for every vertex, and the triangle
   collapses to a point.
3. **The triangle is culled.** If you re-enable facet culling, the vertices
   must be counterclockwise *as seen on screen*. `(−0.5, −0.5)`,
   `(0.5, −0.5)`, `(0, 0.5)` is counterclockwise.
4. **Wrong sizes.** `sizeof` on a vector (see the pitfall above), or a
   stride that doesn't match the data. For positions only, the stride must
   be 12.
5. **Nothing at all, not even the clear color.** The viewport may be 0×0.
   Check that the resize handler runs (`Run` calls it once before the first
   frame).
6. **Still nothing.** Open the program in RenderDoc, capture a frame, and
   look at the draw call's vertex input and output.

### A3.11 Things to try

- **Change the primitive type** to `PrimitiveType::LineLoop` or
  `PrimitiveType::Points`. (Points are one pixel wide. Step 1's
  `programPointSize` lets a shader set `gl_PointSize`.)
- **Draw a range:** `context.Draw(PrimitiveType::Triangles, 0, 3, drawState, sceneState)`.
- **Remove the index buffer** (`vertexArray->SetIndexBuffer(nullptr)`): the
  same triangle is drawn with `glDrawArrays` instead.
- **Try the index-type bug:** upload `std::uint32_t` indices and step
  through `DoDraw` to see that `GL_UNSIGNED_INT` is used. The type can't be
  mismatched.
- **Interleave a color with each position.** This exercise uses the layout
  rules from A2.2. One buffer holds whole vertices, and two attributes read
  from it with the same stride and different offsets:

  ```cpp
  #include <cstddef>        // offsetof
  #include <type_traits>    // std::is_trivially_copyable_v

  // At namespace scope in main.cpp:
  struct ColoredVertex {
      Vector3<float> position;              // offset 0, 12 bytes
      std::array<std::uint8_t, 4> color;    // offset 12, 4 bytes: RGBA, 0-255
  };
  static_assert(sizeof(ColoredVertex) == 16, "no padding expected");
  static_assert(offsetof(ColoredVertex, color) == 12);
  static_assert(std::is_trivially_copyable_v<ColoredVertex>);

  // In main, replacing steps 1, 2 and 4:
  auto shaderProgram = device->CreateShaderProgram(
      R"(layout(location = og_positionVertexLocation) in vec3 position;
         layout(location = og_colorVertexLocation) in vec4 color;
         out vec4 vertexColor;
         void main() { vertexColor = color; gl_Position = vec4(position, 1.0); })",
      R"(in vec4 vertexColor;
         out vec4 fragmentColor;
         void main() { fragmentColor = vertexColor; })");

  const std::array<ColoredVertex, 3> vertices = {{
      {Vector3<float>(-0.5f, -0.5f, 0.0f), {255, 0, 0, 255}},
      {Vector3<float>( 0.5f, -0.5f, 0.0f), {0, 255, 0, 255}},
      {Vector3<float>( 0.0f,  0.5f, 0.0f), {0, 0, 255, 255}},
  }};
  auto vertexBuffer = device->CreateVertexBuffer(BufferHint::StaticDraw, sizeof(vertices));
  vertexBuffer->CopyFromSystemMemory(vertices);

  constexpr int stride = sizeof(ColoredVertex);
  auto vertexArray = context.CreateVertexArray();
  vertexArray->SetAttribute(VertexLocations::Position,
      VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 3, false,
                            static_cast<int>(offsetof(ColoredVertex, position)), stride));
  vertexArray->SetAttribute(VertexLocations::Color,
      VertexBufferAttribute(vertexBuffer, ComponentDatatype::UnsignedByte, 4, true,
                            static_cast<int>(offsetof(ColoredVertex, color)), stride));
  vertexArray->SetIndexBuffer(indexBuffer);
  ```

  The triangle now blends red, green and blue across its surface: the
  rasterizer interpolates `vertexColor`. Try `normalize = false` on the
  color: the shader then receives 255.0 instead of 1.0, and every channel
  saturates. Then add a `std::uint8_t` *before* `position` and watch the
  `static_assert`s fail (the struct becomes 20 bytes, with 3 bytes of
  padding after the new member).

  The doubled braces in `{{ ... }}` are needed because `std::array` is a
  struct wrapping a C array. The outer braces are for the `std::array`
  and the inner ones for the array inside it.

---

## Part B: meshes (3.5.6)

Building buffers and vertex arrays by hand, as in 3A, is fine for one
triangle. For real geometry (Chapter 4 tessellates the whole ellipsoid),
the book separates **creating geometry** from **uploading it**:

- A `Mesh` holds vertex attributes and indices in system memory, by name.
  Geometry algorithms produce meshes, and know nothing about the GPU.
- `Device::CreateMeshBuffers` uploads a mesh into buffers, matching mesh
  attributes to shader inputs **by name**.
- `Context::CreateVertexArray(mesh, ...)` does both steps and returns a
  ready-to-draw vertex array.

> **Why:** `Mesh` lives in **core**, not the renderer. It's plain data: arrays
> of vectors and integers. Putting it in core means:
>
> - geometry code (`Ellipsoid` tessellation in Chapter 4, terrain later)
>   depends only on `arda_core`, and can be tested and reused without any
>   graphics API;
> - the dependency arrow points one way only: the renderer knows about
>   meshes, and meshes know nothing about the renderer.
>
> This is exactly OpenGlobe's split: `Mesh.cs` is in `OpenGlobe.Core`, and
> `MeshBuffers.cs` is in `OpenGlobe.Renderer`. The price is that core can't
> use renderer types such as `Color`, so `VertexAttributeRGB::AddColor`
> takes three bytes instead.

### B1. `core/include/arda/core/Half.h`

Half-float vertex attributes (`VertexAttributeHalfFloat*`) store 16-bit
floats, halving the memory for data that doesn't need full precision, such
as texture coordinates and normals. C++20 has no portable 16-bit float
type (`std::float16_t` is C++23 and not yet in MSVC), so this ports the
parts of OpenGlobe's `Half.cs` that meshes need: storage and conversion.

GL converts half floats back to 32-bit floats when the vertex shader reads
them (`ComponentDatatype::HalfFloat`), so shaders still declare `vec2` and
so on.

```cpp
#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

namespace arda::core {

// Converts a float to IEEE 754 half precision (1 sign bit, 5 exponent bits, 10 mantissa bits),
// rounding to the nearest value, ties to even. Values too large become infinity. Half.cs, DoubleToHalf.
constexpr std::uint16_t FloatToHalfBits(float value) {
    const std::uint32_t f = std::bit_cast<std::uint32_t>(value);
    const std::uint32_t sign = (f >> 16) & 0x8000u;
    const int exponent = static_cast<int>((f >> 23) & 0xFFu);
    const std::uint32_t mantissa = f & 0x7FFFFFu;

    if (exponent == 0xFF) {   // infinity or NaN; keep NaN a NaN
        return static_cast<std::uint16_t>(sign | 0x7C00u | (mantissa != 0 ? 0x0200u : 0u));
    }

    const int halfExponent = exponent - 127 + 15;   // re-bias from float's 127 to half's 15
    if (halfExponent >= 0x1F) {                     // too large: infinity
        return static_cast<std::uint16_t>(sign | 0x7C00u);
    }

    // Shifts `full` right by `shift` bits, rounding to nearest, ties to even.
    const auto shiftAndRound = [](std::uint32_t full, int shift) {
        const std::uint32_t result = full >> shift;
        const std::uint32_t remainder = full & ((1u << shift) - 1u);
        const std::uint32_t halfway = 1u << (shift - 1);
        const bool roundUp = remainder > halfway || (remainder == halfway && (result & 1u) != 0);
        return roundUp ? result + 1u : result;
    };

    if (halfExponent <= 0) {                        // a subnormal half, or zero
        if (halfExponent < -10) {
            return static_cast<std::uint16_t>(sign);   // too small: signed zero
        }
        const std::uint32_t full = mantissa | 0x800000u;   // restore the implicit leading 1
        return static_cast<std::uint16_t>(sign | shiftAndRound(full, 14 - halfExponent));
    }

    // A normal half. If rounding carries out of the mantissa, it correctly bumps the exponent
    // (and turns values just above 65504 into infinity).
    const std::uint32_t exponentBits = static_cast<std::uint32_t>(halfExponent) << 10;
    return static_cast<std::uint16_t>(sign | (exponentBits + shiftAndRound(mantissa, 13)));
}

// Converts half-precision bits back to a float. Exact: every half is representable as a float.
constexpr float HalfBitsToFloat(std::uint16_t half) {
    const std::uint32_t sign = static_cast<std::uint32_t>(half & 0x8000u) << 16;
    const std::uint32_t exponent = (half >> 10) & 0x1Fu;
    const std::uint32_t mantissa = half & 0x3FFu;

    if (exponent == 0) {   // zero or subnormal: mantissa * 2^-24
        const float magnitude = static_cast<float>(mantissa) / 16777216.0f;
        return sign != 0 ? -magnitude : magnitude;
    }
    if (exponent == 0x1F) {   // infinity or NaN
        return std::bit_cast<float>(sign | 0x7F800000u | (mantissa << 13));
    }
    return std::bit_cast<float>(sign | ((exponent + 112u) << 23) | (mantissa << 13));   // 112 = 127 - 15
}

// A 16-bit float for vertex data (Half.cs). Storage only: convert with ToFloat to do arithmetic.
struct Half {
    std::uint16_t bits = 0;

    constexpr Half() = default;
    constexpr explicit Half(float value) : bits(FloatToHalfBits(value)) {}

    static constexpr Half FromBits(std::uint16_t value) {
        Half half;
        half.bits = value;
        return half;
    }

    constexpr float ToFloat() const { return HalfBitsToFloat(bits); }

    bool operator==(const Half&) const = default;   // compares bits: +0 != -0, NaN == the same NaN
};

static_assert(sizeof(Half) == 2);
static_assert(std::is_trivially_copyable_v<Half>);

} // namespace arda::core
```

The constructor from `float` is `explicit` (see [Step 0](00-setup.md)),
because converting to half loses precision and shouldn't happen by
accident. C#'s `Half` has an explicit conversion operator for the same
reason. C# also converts from `double`. Only `float` is provided here,
because vertex data starts as floats.

> **C++ note — `std::bit_cast`:** `std::bit_cast<To>(from)` (C++20, `<bit>`)
> returns an object of type `To` with exactly the same bits as `from`. Both
> types must be the same size and trivially copyable. It's how you look at
> a float's sign, exponent and mantissa bits, and it works in `constexpr`
> functions. Older code does this with `reinterpret_cast<std::uint32_t&>(value)`
> or a union. Both are undefined behavior in C++, because they break the
> strict aliasing rule. C#'s `BitConverter.SingleToInt32Bits` is the
> equivalent.

### B2. `core/include/arda/core/geometry/VertexAttribute.h` (Listing 3.22)

A mesh attribute is a name and a list of values, one per vertex. OpenGlobe
has an abstract `VertexAttribute`, a generic `VertexAttribute<T>`, and one
subclass per combination (`VertexAttributeFloatVector3`, ...). Most
subclasses exist only to set the type enum. One template with the enum as a
template argument replaces them, and aliases keep the C# names.

`VertexAttributeRGB` and `VertexAttributeRGBA` are real subclasses, because
they add behavior (`AddColor`, and a capacity counted in colors), and
`CreateMeshBuffers` must be able to tell them apart from a plain byte
attribute. That's why `VertexAttribute<T, Type>` isn't `final`.

```cpp
#pragma once

#include <arda/core/Half.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace arda::core::geometry {

// VertexAttributeType.cs
enum class VertexAttributeType {
    UnsignedByte,
    HalfFloat,
    HalfFloatVector2,
    HalfFloatVector3,
    HalfFloatVector4,
    Float,
    FloatVector2,
    FloatVector3,
    FloatVector4,
    EmulatedDoubleVector3,
};

// Half-float vectors, for VertexAttributeHalfFloatVector2/3/4. Vector2<T> only accepts
// arithmetic types, so these are plain arrays: tightly packed, 2 bytes per component.
using HalfVector2 = std::array<Half, 2>;
using HalfVector3 = std::array<Half, 3>;
using HalfVector4 = std::array<Half, 4>;

// A named per-vertex value in system memory (VertexAttribute.cs).
class VertexAttributeBase {
public:
    virtual ~VertexAttributeBase() = default;

    VertexAttributeBase(const VertexAttributeBase&)            = delete;
    VertexAttributeBase& operator=(const VertexAttributeBase&) = delete;

    const std::string& Name() const { return m_name; }
    VertexAttributeType Datatype() const { return m_type; }

protected:
    VertexAttributeBase(std::string name, VertexAttributeType type)
        : m_name(std::move(name)), m_type(type) {}

private:
    std::string m_name;
    VertexAttributeType m_type;
};

// VertexAttribute<T> in C#, plus the type enum as a template argument.
// Use the aliases below rather than naming this template directly: CreateMeshBuffers
// relies on each VertexAttributeType value having the value type given below.
template <typename T, VertexAttributeType Type>
class VertexAttribute : public VertexAttributeBase {
public:
    static_assert(std::is_trivially_copyable_v<T>,
                  "vertex attribute values are copied byte for byte into vertex buffers");

    using ValueType = T;

    // capacity only reserves memory; the attribute starts empty.
    explicit VertexAttribute(std::string name, std::size_t capacity = 0)
        : VertexAttributeBase(std::move(name), Type) {
        m_values.reserve(capacity);
    }

    std::vector<T>& Values() { return m_values; }
    const std::vector<T>& Values() const { return m_values; }

private:
    std::vector<T> m_values;
};

// One alias per C# class. VertexAttributeByte.cs is VertexAttributeUnsignedByte here.
using VertexAttributeUnsignedByte     = VertexAttribute<std::uint8_t, VertexAttributeType::UnsignedByte>;
using VertexAttributeHalfFloat        = VertexAttribute<Half, VertexAttributeType::HalfFloat>;
using VertexAttributeHalfFloatVector2 = VertexAttribute<HalfVector2, VertexAttributeType::HalfFloatVector2>;
using VertexAttributeHalfFloatVector3 = VertexAttribute<HalfVector3, VertexAttributeType::HalfFloatVector3>;
using VertexAttributeHalfFloatVector4 = VertexAttribute<HalfVector4, VertexAttributeType::HalfFloatVector4>;
using VertexAttributeFloat            = VertexAttribute<float, VertexAttributeType::Float>;
using VertexAttributeFloatVector2     = VertexAttribute<Vector2<float>, VertexAttributeType::FloatVector2>;
using VertexAttributeFloatVector3     = VertexAttribute<Vector3<float>, VertexAttributeType::FloatVector3>;
using VertexAttributeFloatVector4     = VertexAttribute<Vector4<float>, VertexAttributeType::FloatVector4>;
using VertexAttributeDoubleVector3    = VertexAttribute<Vector3<double>, VertexAttributeType::EmulatedDoubleVector3>;

// Colors as 3 bytes per vertex, read by shaders as a normalized vec3 (VertexAttributeRGB.cs).
class VertexAttributeRGB final : public VertexAttributeUnsignedByte {
public:
    // capacity is in colors, not bytes.
    explicit VertexAttributeRGB(std::string name, std::size_t capacity = 0)
        : VertexAttributeUnsignedByte(std::move(name), capacity * 3) {}

    void AddColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
        Values().insert(Values().end(), {red, green, blue});
    }
};

// Colors as 4 bytes per vertex, read by shaders as a normalized vec4 (VertexAttributeRGBA.cs).
class VertexAttributeRGBA final : public VertexAttributeUnsignedByte {
public:
    // capacity is in colors, not bytes.
    explicit VertexAttributeRGBA(std::string name, std::size_t capacity = 0)
        : VertexAttributeUnsignedByte(std::move(name), capacity * 4) {}

    void AddColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
        Values().insert(Values().end(), {red, green, blue, alpha});
    }
};

// A mesh's attributes, looked up by name (VertexAttributeCollection.cs).
class VertexAttributeCollection {
public:
    // Throws std::invalid_argument if the attribute is null or its name is already used.
    void Add(std::unique_ptr<VertexAttributeBase> attribute) {
        if (!attribute) {
            throw std::invalid_argument("VertexAttributeCollection::Add: attribute is null");
        }
        const std::string name = attribute->Name();
        if (!m_attributes.emplace(name, std::move(attribute)).second) {
            throw std::invalid_argument("Duplicate vertex attribute '" + name + "'");
        }
    }

    // Creates, adds and returns a typed attribute:
    //   auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    template <typename A>
        requires std::is_base_of_v<VertexAttributeBase, A>
    A& Add(std::string name, std::size_t capacity = 0) {
        auto attribute = std::make_unique<A>(std::move(name), capacity);
        A& result = *attribute;   // the object doesn't move when the unique_ptr does
        Add(std::move(attribute));
        return result;
    }

    bool Contains(std::string_view name) const { return m_attributes.find(name) != m_attributes.end(); }

    // Throws std::out_of_range if there is no attribute with that name.
    const VertexAttributeBase& operator[](std::string_view name) const {
        auto it = m_attributes.find(name);
        if (it == m_attributes.end()) {
            throw std::out_of_range("No vertex attribute named '" + std::string(name) + "'");
        }
        return *it->second;
    }

    // Returns true if an attribute was removed.
    bool Remove(std::string_view name) {
        auto it = m_attributes.find(name);
        if (it == m_attributes.end()) {
            return false;
        }
        m_attributes.erase(it);
        return true;
    }

    void Clear() { m_attributes.clear(); }

    std::size_t Size() const { return m_attributes.size(); }

    // Iterates (name, std::unique_ptr<VertexAttributeBase>) pairs, in name order:
    //   for (const auto& [name, attribute] : mesh.attributes) { ... }
    auto begin() const { return m_attributes.begin(); }
    auto end() const { return m_attributes.end(); }

private:
    // std::less<> allows find() with a std::string_view (see Step 2).
    std::map<std::string, std::unique_ptr<VertexAttributeBase>, std::less<>> m_attributes;
};

} // namespace arda::core::geometry
```

Details worth noticing:

- **`Vector3<float>` resolves** even though this file is in
  `arda::core::geometry` and `Vector3` is in `arda::core`. Unqualified
  lookup searches the enclosing namespaces too.
- **`Add<A>` returns a reference** to the attribute it created. That's
  safe: the `unique_ptr` moves into the map, but the object it points to
  stays where it is on the heap.
- **The `requires` clause on `Add<A>`** turns
  `mesh.attributes.Add<int>("x")` into a clear error.
- **Copies are deleted on the base class.** That stops accidental slicing
  (copying only the base part of a derived object) and makes `Mesh`
  move-only, which is what you want for a potentially huge block of data.
- **Iteration is in name order**, because `std::map` is sorted. C#'s
  `Dictionary` has no defined order, so nothing may depend on it.
- **`Vector4.h` must include what it uses.** It uses `std::sqrt` and
  `std::runtime_error`, so it needs `<cmath>` and `<stdexcept>`.
  [Step 2](02-shaders.md) added them. If you skipped that, add them now, so
  the order of includes doesn't matter.

> **C++ note — an enum value as a template argument:** templates can take
> *values* as well as types (a "non-type template parameter"):
> `template <typename T, VertexAttributeType Type>`. Each combination is a
> separate class, and inside it, `Type` is a compile-time constant. C#
> generics only take types, which is why OpenGlobe needs a subclass per
> combination to pass the enum to the base constructor.

> **C++ note — polymorphic storage versus `std::variant`:** a mesh holds
> attributes of *different* types in one collection. C++ offers two
> standard ways to do that.
>
> 1. **A base class and derived classes, stored as `std::unique_ptr<Base>`**
>    (used here, and in OpenGlobe). Code that needs the concrete type checks
>    `Datatype()` and casts. The set of types is *open*: new attribute
>    classes can be added without changing existing code. The costs are one
>    heap allocation per attribute, casts, and a `Mesh` that can't be copied.
> 2. **`std::variant`**, a type-safe union that holds exactly one of a fixed
>    list of types:
>
>    ```cpp
>    using AttributeValues = std::variant<std::vector<std::uint8_t>, std::vector<float>,
>                                         std::vector<Vector3<float>> /* , ... */>;
>    std::map<std::string, AttributeValues, std::less<>> attributes;
>
>    std::visit([&](const auto& values) {   // called with the right type
>        buffer->CopyFromSystemMemory(values);
>    }, attributes["position"]);
>    ```
>
>    The set of types is *closed*, and `std::visit` makes the compiler check
>    that every alternative is handled, with no casts. A variant is a value,
>    so the mesh becomes copyable. The costs: every variant is as big as its
>    largest alternative, adding a type means editing the one list, and
>    RGB/RGBA (which share a value type with plain bytes) would need an extra
>    tag.
>
> The polymorphic version is kept because it maps one-to-one onto Listing
> 3.22, RGB and RGBA carry behavior, and attributes are few and large, so
> the extra allocation doesn't matter. In C#, only the first option exists.

### B3. `core/include/arda/core/geometry/Indices.h` (Listing 3.23)

Same pattern as the attributes, without the name. C#'s
`AddTriangle(new TriangleIndicesUnsignedShort(0, 1, 2))` takes a small
struct; here `AddTriangle(0, 1, 2)` takes the three indices directly, so
`TriangleIndices*` isn't needed.

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace arda::core::geometry {

// IndicesType.cs
enum class IndicesType {
    UnsignedShort,
    UnsignedInt,
};

// A mesh's index list (IndicesBase.cs).
class IndicesBase {
public:
    virtual ~IndicesBase() = default;

    IndicesBase(const IndicesBase&)            = delete;
    IndicesBase& operator=(const IndicesBase&) = delete;

    IndicesType Datatype() const { return m_type; }

protected:
    explicit IndicesBase(IndicesType type) : m_type(type) {}

private:
    IndicesType m_type;
};

template <typename T, IndicesType Type>
class Indices final : public IndicesBase {
public:
    static_assert(std::is_unsigned_v<T>, "indices are unsigned integers");

    // capacity only reserves memory, in indices.
    explicit Indices(std::size_t capacity = 0) : IndicesBase(Type) { m_values.reserve(capacity); }

    std::vector<T>& Values() { return m_values; }
    const std::vector<T>& Values() const { return m_values; }

    // Replaces TriangleIndicesUnsignedShort/UnsignedInt.
    void AddTriangle(T i0, T i1, T i2) { m_values.insert(m_values.end(), {i0, i1, i2}); }

private:
    std::vector<T> m_values;
};

using IndicesUnsignedShort = Indices<std::uint16_t, IndicesType::UnsignedShort>;
using IndicesUnsignedInt   = Indices<std::uint32_t, IndicesType::UnsignedInt>;

} // namespace arda::core::geometry
```

### B4. `core/include/arda/core/geometry/Mesh.h` (Listing 3.21)

```cpp
#pragma once

#include <arda/core/geometry/Indices.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/core/geometry/VertexAttribute.h>
#include <arda/core/geometry/WindingOrder.h>

#include <memory>

namespace arda::core::geometry {

// Geometry in system memory (3.5.6, Listing 3.21). Algorithms that build geometry
// (Chapter 4) return one of these, and the renderer turns it into buffers.
// Move-only: attributes and indices are owned through unique_ptrs.
struct Mesh {
    VertexAttributeCollection attributes;
    std::unique_ptr<IndicesBase> indices;   // optional: null means draw with glDrawArrays
    PrimitiveType primitiveType = PrimitiveType::Triangles;
    WindingOrder frontFaceWindingOrder = WindingOrder::Counterclockwise;
};

} // namespace arda::core::geometry
```

`WindingOrder.h` is from [Step 1](01-state-management.md). C#'s `Mesh`
leaves `PrimitiveType` and `FrontFaceWindingOrder` at the enum's first
value (`Points` and `Clockwise`). Defaulting to the far more common
triangles and counterclockwise is safer.

A `Mesh` can be returned by value (`Mesh CreateTriangleMesh()`), because it
has an implicit move constructor. Step 7 does exactly that.

### B5. `mesh/MeshBuffers.h`

The GPU-side result of uploading a mesh: one optional attribute per shader
location, and an optional index buffer. It's the input to
`Context::CreateVertexArray(const MeshBuffers&)`.

```cpp
#pragma once

#include <arda/renderer/buffers/IndexBuffer.h>
#include <arda/renderer/vertexarray/VertexBufferAttribute.h>

#include <memory>
#include <optional>
#include <vector>

namespace arda::renderer {

// Buffers created from a Mesh, ready to be put in a VertexArray (MeshBuffers.cs).
struct MeshBuffers {
    std::vector<std::optional<VertexBufferAttribute>> attributes;   // indexed by shader location
    std::shared_ptr<IndexBuffer> indexBuffer;                       // null if the mesh has no indices
};

} // namespace arda::renderer
```

C#'s `MeshBuffers` comment reads "Does not own vertex and index buffers.
They must be disposed." With `shared_ptr`, a `MeshBuffers` *does* share
ownership. It can be dropped right after creating a vertex array, or kept
to create vertex arrays for several contexts (one per window) from the same
buffers, which is the one reason to call `CreateMeshBuffers` directly.

### B6. `Device::CreateMeshBuffers`

```cpp
// Device.h: new includes
#include <arda/core/geometry/Mesh.h>
#include <arda/renderer/mesh/MeshBuffers.h>
#include <arda/renderer/shaders/ShaderVertexAttribute.h>

class Device {
public:
    // ...after CreateIndexBuffer...

    // Uploads every mesh attribute that shaderAttributes uses, matched by name, and the
    // mesh's indices (Device.cs CreateMeshBuffers). Throws std::invalid_argument if the
    // shader uses an attribute the mesh doesn't have.
    // Non-virtual: it only uses CreateVertexBuffer and CreateIndexBuffer, so every backend gets it.
    MeshBuffers CreateMeshBuffers(const core::geometry::Mesh& mesh,
                                  const ShaderVertexAttributeCollection& shaderAttributes,
                                  BufferHint usageHint);
};
```

The implementation goes in `src/mesh/MeshBuffers.cpp`, next to the rest of
the mesh code, although it's a `Device` member. It ports all of
`Device.CreateMeshBuffers` (131–375) and `CreateVertexBuffer<T>`
(377–385):

1. **Indices:** upload them with the matching index type.
2. **Emulated doubles (Chapter 5, ported now for completeness):** a
   `VertexAttributeDoubleVector3` named `position` can feed *two* shader
   inputs, `positionHigh` and `positionLow`. Each double is split into a
   float and the float error (`EmulatedVector3D.cs`), both are interleaved
   into one buffer, and two attributes read it with a stride of 24 bytes.
3. **Everything else:** for each shader input, find the mesh attribute with
   the same name and upload it with the matching component type and count.
   A double attribute used directly (not as High/Low) is converted to
   floats.

```cpp
#include <arda/renderer/Device.h>

#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/Vector3.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace arda::renderer {

// Fine in a .cpp file. Never put a using-directive in a header.
using namespace core::geometry;
using core::Vector3;

namespace {

// Named Upload*, not Create*: inside Device::CreateMeshBuffers, the member functions
// Device::CreateVertexBuffer and CreateIndexBuffer would hide free functions with those names.
template <typename T>
std::shared_ptr<VertexBuffer> UploadVertexBuffer(Device& device, const std::vector<T>& values, BufferHint usageHint) {
    auto buffer = device.CreateVertexBuffer(usageHint, values.size() * sizeof(T));
    buffer->CopyFromSystemMemory(values);
    return buffer;
}

template <typename T>
std::shared_ptr<IndexBuffer> UploadIndexBuffer(Device& device, const std::vector<T>& values, BufferHint usageHint) {
    auto buffer = device.CreateIndexBuffer(usageHint, values.size() * sizeof(T));
    buffer->CopyFromSystemMemory(values);
    return buffer;
}

// Uploads a mesh attribute whose concrete type is A, and describes how to read it.
template <typename A>
VertexBufferAttribute UploadAttribute(Device& device, const VertexAttributeBase& attribute,
                                      ComponentDatatype componentDatatype, int numberOfComponents,
                                      BufferHint usageHint, bool normalize = false) {
    // Safe: the caller has checked attribute.Datatype(), and each type value has one class (or its subclasses).
    const auto& values = static_cast<const A&>(attribute).Values();
    return VertexBufferAttribute(UploadVertexBuffer(device, values, usageHint),
                                 componentDatatype, numberOfComponents, normalize);
}

Vector3<float> ToVector3F(const Vector3<double>& v) {
    return Vector3<float>(static_cast<float>(v.X()), static_cast<float>(v.Y()), static_cast<float>(v.Z()));
}

Vector3<double> ToVector3D(const Vector3<float>& v) {
    return Vector3<double>(v.X(), v.Y(), v.Z());
}

// EmulatedVector3D.cs: high is the nearest float, and low is the float nearest to what's left over.
// high + low recovers the double to about 14 significant digits instead of 7 (Chapter 5).
struct EmulatedVector3 {
    explicit EmulatedVector3(const Vector3<double>& v)
        : high(ToVector3F(v)), low(ToVector3F(v - ToVector3D(high))) {}   // high is initialized first

    Vector3<float> high;
    Vector3<float> low;
};

// The attribute slot for a shader location, with a clear error for a bad location.
std::optional<VertexBufferAttribute>& SlotAt(MeshBuffers& meshBuffers, int location) {
    if (location < 0 || location >= static_cast<int>(meshBuffers.attributes.size())) {
        throw std::out_of_range("Shader vertex attribute location " + std::to_string(location) +
                                " is outside the device's vertex attribute range");
    }
    return meshBuffers.attributes[static_cast<std::size_t>(location)];
}

} // namespace

MeshBuffers Device::CreateMeshBuffers(const Mesh& mesh,
                                      const ShaderVertexAttributeCollection& shaderAttributes,
                                      BufferHint usageHint) {
    MeshBuffers meshBuffers;
    meshBuffers.attributes.resize(static_cast<std::size_t>(Limits().maximumNumberOfVertexAttributes));

    // 1. Indices.
    if (mesh.indices) {
        switch (mesh.indices->Datatype()) {
        case IndicesType::UnsignedShort:
            meshBuffers.indexBuffer = UploadIndexBuffer(
                *this, static_cast<const IndicesUnsignedShort&>(*mesh.indices).Values(), usageHint);
            break;
        case IndicesType::UnsignedInt:
            meshBuffers.indexBuffer = UploadIndexBuffer(
                *this, static_cast<const IndicesUnsignedInt&>(*mesh.indices).Values(), usageHint);
            break;
        }
    }

    // 2. Emulated double-precision vectors. One mesh attribute ("position") yields two shader
    //    attributes ("positionHigh" and "positionLow"), so they are handled before the others.
    std::unordered_set<std::string> ignoreAttributes;

    for (const auto& [name, attribute] : mesh.attributes) {
        if (attribute->Datatype() != VertexAttributeType::EmulatedDoubleVector3) {
            continue;
        }

        const ShaderVertexAttribute* high = shaderAttributes.Find(name + "High");
        const ShaderVertexAttribute* low = shaderAttributes.Find(name + "Low");
        if (high == nullptr && low == nullptr) {
            continue;   // the shader uses neither; it may use the attribute directly (step 3)
        }
        if (high == nullptr || low == nullptr) {
            throw std::invalid_argument("An emulated double vec3 mesh attribute requires both " + name +
                                        "High and " + name + "Low vertex attributes, but the shader only "
                                        "contains one matching attribute.");
        }

        // Copy both parts into one vertex buffer: high0, low0, high1, low1, ...
        const auto& values = static_cast<const VertexAttributeDoubleVector3&>(*attribute).Values();
        std::vector<Vector3<float>> vertices;
        vertices.reserve(2 * values.size());
        for (const Vector3<double>& value : values) {
            const EmulatedVector3 emulated(value);
            vertices.push_back(emulated.high);
            vertices.push_back(emulated.low);
        }
        auto vertexBuffer = UploadVertexBuffer(*this, vertices, usageHint);

        constexpr int vectorSize = static_cast<int>(sizeof(Vector3<float>));
        constexpr int stride = 2 * vectorSize;
        SlotAt(meshBuffers, high->location) =
            VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 3, false, 0, stride);
        SlotAt(meshBuffers, low->location) =
            VertexBufferAttribute(vertexBuffer, ComponentDatatype::Float, 3, false, vectorSize, stride);

        ignoreAttributes.insert(high->name);
        ignoreAttributes.insert(low->name);
    }

    // 3. Match every other attribute the shader uses to a mesh attribute with the same name.
    for (const ShaderVertexAttribute& shaderAttribute : shaderAttributes) {
        if (ignoreAttributes.contains(shaderAttribute.name)) {
            continue;
        }
        if (!mesh.attributes.Contains(shaderAttribute.name)) {
            throw std::invalid_argument("Shader requires vertex attribute \"" + shaderAttribute.name +
                                        "\", which is not present in mesh.");
        }

        const VertexAttributeBase& attribute = mesh.attributes[shaderAttribute.name];
        std::optional<VertexBufferAttribute>& slot = SlotAt(meshBuffers, shaderAttribute.location);

        switch (attribute.Datatype()) {
        case VertexAttributeType::UnsignedByte:
            // RGB and RGBA pack several bytes per vertex; C# checks `attribute is VertexAttributeRGBA`.
            if (dynamic_cast<const VertexAttributeRGBA*>(&attribute) != nullptr) {
                slot = UploadAttribute<VertexAttributeUnsignedByte>(
                    *this, attribute, ComponentDatatype::UnsignedByte, 4, usageHint, true);
            } else if (dynamic_cast<const VertexAttributeRGB*>(&attribute) != nullptr) {
                slot = UploadAttribute<VertexAttributeUnsignedByte>(
                    *this, attribute, ComponentDatatype::UnsignedByte, 3, usageHint, true);
            } else {
                slot = UploadAttribute<VertexAttributeUnsignedByte>(
                    *this, attribute, ComponentDatatype::UnsignedByte, 1, usageHint);
            }
            break;

        case VertexAttributeType::HalfFloat:
            slot = UploadAttribute<VertexAttributeHalfFloat>(*this, attribute, ComponentDatatype::HalfFloat, 1, usageHint);
            break;
        case VertexAttributeType::HalfFloatVector2:
            slot = UploadAttribute<VertexAttributeHalfFloatVector2>(*this, attribute, ComponentDatatype::HalfFloat, 2, usageHint);
            break;
        case VertexAttributeType::HalfFloatVector3:
            slot = UploadAttribute<VertexAttributeHalfFloatVector3>(*this, attribute, ComponentDatatype::HalfFloat, 3, usageHint);
            break;
        case VertexAttributeType::HalfFloatVector4:
            slot = UploadAttribute<VertexAttributeHalfFloatVector4>(*this, attribute, ComponentDatatype::HalfFloat, 4, usageHint);
            break;

        case VertexAttributeType::Float:
            slot = UploadAttribute<VertexAttributeFloat>(*this, attribute, ComponentDatatype::Float, 1, usageHint);
            break;
        case VertexAttributeType::FloatVector2:
            slot = UploadAttribute<VertexAttributeFloatVector2>(*this, attribute, ComponentDatatype::Float, 2, usageHint);
            break;
        case VertexAttributeType::FloatVector3:
            slot = UploadAttribute<VertexAttributeFloatVector3>(*this, attribute, ComponentDatatype::Float, 3, usageHint);
            break;
        case VertexAttributeType::FloatVector4:
            slot = UploadAttribute<VertexAttributeFloatVector4>(*this, attribute, ComponentDatatype::Float, 4, usageHint);
            break;

        case VertexAttributeType::EmulatedDoubleVector3: {
            // Used directly (not as High/Low): convert to single precision.
            const auto& values = static_cast<const VertexAttributeDoubleVector3&>(attribute).Values();
            std::vector<Vector3<float>> floats;
            floats.reserve(values.size());
            for (const Vector3<double>& value : values) {
                floats.push_back(ToVector3F(value));
            }
            slot = VertexBufferAttribute(UploadVertexBuffer(*this, floats, usageHint), ComponentDatatype::Float, 3);
            break;
        }
        }
    }

    return meshBuffers;
}

} // namespace arda::renderer
```

Notes on the port:

- **No null checks for `mesh` and `shaderAttributes`.** They're references,
  so they can't be null.
- **The C# code copies each `IList<T>` into a `T[]`** before uploading,
  because `CopyFromSystemMemory` takes arrays. A `std::vector` is already
  contiguous, so it's uploaded directly.
- **`static_cast` for the typed attribute and `dynamic_cast` for RGB/RGBA.**
  `Datatype()` already says the concrete class for every other type, so the
  cheap `static_cast` is enough there. RGB, RGBA and plain bytes all report
  `UnsignedByte`, so only a runtime type check can tell them apart, and
  `dynamic_cast` is that check. (See [Step 2](02-shaders.md) for the
  difference between the two casts.)
- **The `switch` covers every `VertexAttributeType` and has no `default`**,
  so the compiler warns if a new type is added and not handled. C# ends
  with `Debug.Fail`.
- **Empty attributes throw.** A mesh attribute with no values produces a
  zero-sized buffer, and `CreateVertexBuffer` rejects it, just as C#'s
  `BufferGL3x` constructor does.
- **Case braces:** the `EmulatedDoubleVector3` case declares local variables,
  so its body is wrapped in `{ }`. Without them, the variables would stay in
  scope for any `case` label added after it. Jumping to that label would
  skip their initialization, which is a compile error.

> **C++ note — a member name hides a free function with the same name:**
> an earlier version of this code called its helpers `CreateVertexBuffer`
> and `CreateIndexBuffer`, and it didn't compile. Inside a member function,
> unqualified name lookup searches the class *first*. It finds
> `Device::CreateIndexBuffer(BufferHint, std::size_t)`, **stops**, and never
> considers the free function in the anonymous namespace, even though the
> arguments `(*this, values, usageHint)` only fit the free function.
> Argument-dependent lookup doesn't help, because it's switched off when
> ordinary lookup finds a class member. There are two fixes: call it as
> `::arda::renderer::CreateIndexBuffer(...)` (the anonymous namespace's
> names are visible there), or give the helper a different name, as done
> here. C# has the same rule for methods, but helpers there are usually
> static members of the same class, so the question doesn't come up.
>
> The helpers are in an anonymous namespace so they're private to this
> file (see [Step 0](00-setup.md)).

### B7. `Context::CreateVertexArray` for meshes (`Context.cs` lines 33–50)

```cpp
// Context.h: new includes
#include <arda/core/geometry/Mesh.h>
#include <arda/renderer/buffers/BufferHint.h>
#include <arda/renderer/mesh/MeshBuffers.h>
#include <arda/renderer/shaders/ShaderVertexAttribute.h>

// Context.h: in the public section, right after CreateVertexArray()
// Creates a vertex array that uses meshBuffers' attributes and index buffer.
std::shared_ptr<VertexArray> CreateVertexArray(const MeshBuffers& meshBuffers);

// Uploads the mesh (Device::CreateMeshBuffers) and creates a vertex array for it.
std::shared_ptr<VertexArray> CreateVertexArray(const core::geometry::Mesh& mesh,
                                               const ShaderVertexAttributeCollection& shaderAttributes,
                                               BufferHint usageHint);
```

```cpp
// src/Context.cpp: new include
#include <arda/renderer/Device.h>

#include <cstddef>

// src/Context.cpp: new functions, after SetViewport
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
```

`Context.h` only declares `Device`, so the call to `CreateMeshBuffers` needs
the full definition. `Context.cpp` includes `Device.h`, and `Context.h`
doesn't. `Device.h` never needs `Context.h`, so there is no include cycle.

The C# version sets `va.DisposeBuffers = true`. Here, the vertex array's
`shared_ptr`s already make it an owner of the mesh's buffers: once the
`MeshBuffers` temporary is gone, the vertex array is the only owner.

The `MeshBuffers` overload is also why `CreateVertexArray()` is non-virtual:
all three overloads share one name, and the backend overrides only
`DoCreateVertexArray`.

### B8. Checkpoint 4: mesh tests

**CMake:**

```cmake
# core/CMakeLists.txt, in add_library(arda_core ...)
    include/arda/core/Half.h
    include/arda/core/geometry/Indices.h
    include/arda/core/geometry/Mesh.h
    include/arda/core/geometry/PrimitiveType.h
    include/arda/core/geometry/VertexAttribute.h

# renderer/CMakeLists.txt, in add_library(arda_renderer ...)
    src/mesh/MeshBuffers.cpp
    include/arda/renderer/DrawState.h
    include/arda/renderer/mesh/MeshBuffers.h
    include/arda/renderer/scene/SceneState.h
```

(Add `PrimitiveType.h` to the core list now if you didn't in Part A3.
`DrawState.h` may already be listed from Step 1.)

**Tests.** Add these includes to `VertexDataTests.cpp`:

```cpp
#include <arda/core/Half.h>
#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/mesh/MeshBuffers.h>
#include <arda/renderer/shaders/ShaderVertexAttribute.h>

#include <cmath>
#include <limits>
```

and these test cases:

```cpp
TEST_CASE("Half converts to and from float with IEEE rounding") {
    using arda::core::Half;

    CHECK(Half(0.0f).bits == 0x0000);
    CHECK(Half(-0.0f).bits == 0x8000);
    CHECK(Half(1.0f).bits == 0x3C00);
    CHECK(Half(-2.0f).bits == 0xC000);
    CHECK(Half(0.1f).bits == 0x2E66);                        // rounded; 0.1 isn't exact in any binary format
    CHECK(Half(65504.0f).bits == 0x7BFF);                    // largest half
    CHECK(Half(65520.0f).bits == 0x7C00);                    // rounds up to infinity
    CHECK(Half(1.0e6f).bits == 0x7C00);                      // too large: infinity
    CHECK(Half(std::ldexp(1.0f, -24)).bits == 0x0001);       // smallest subnormal
    CHECK(Half(std::ldexp(1.0f, -26)).bits == 0x0000);       // too small: zero
    CHECK(Half(std::numeric_limits<float>::infinity()).bits == 0x7C00);
    CHECK(std::isnan(Half(std::numeric_limits<float>::quiet_NaN()).ToFloat()));

    CHECK(Half(0.5f).ToFloat() == 0.5f);
    CHECK(Half(-1234.0f).ToFloat() == -1234.0f);
    CHECK(Half::FromBits(0x0001).ToFloat() == std::ldexp(1.0f, -24));
    static_assert(Half(1.0f).ToFloat() == 1.0f);             // it all works at compile time too
}

TEST_CASE("Mesh attributes and indices") {
    using arda::core::Vector3;

    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(0.0f, 0.0f, 0.0f);
    CHECK(mesh.attributes.Size() == 1);
    CHECK(mesh.attributes.Contains("position"));
    CHECK(mesh.attributes["position"].Datatype() == VertexAttributeType::FloatVector3);
    CHECK(mesh.attributes["position"].Name() == "position");

    CHECK_THROWS_AS(mesh.attributes.Add<VertexAttributeFloat>("position"), std::invalid_argument);
    CHECK_THROWS_AS(mesh.attributes["missing"], std::out_of_range);

    auto& colors = mesh.attributes.Add<VertexAttributeRGBA>("color", 1);
    colors.AddColor(255, 128, 0, 255);
    CHECK(colors.Values() == std::vector<std::uint8_t>{255, 128, 0, 255});
    CHECK(colors.Datatype() == VertexAttributeType::UnsignedByte);

    CHECK(mesh.attributes.Remove("color"));
    CHECK_FALSE(mesh.attributes.Remove("color"));

    auto indices = std::make_unique<IndicesUnsignedShort>(6);
    indices->AddTriangle(0, 1, 2);
    indices->AddTriangle(2, 1, 3);
    CHECK(indices->Values() == std::vector<std::uint16_t>{0, 1, 2, 2, 1, 3});
    CHECK(indices->Datatype() == IndicesType::UnsignedShort);
    mesh.indices = std::move(indices);

    CHECK(mesh.primitiveType == PrimitiveType::Triangles);
    CHECK(mesh.frontFaceWindingOrder == WindingOrder::Counterclockwise);
}

TEST_CASE("CreateMeshBuffers matches mesh attributes to shader attributes by name") {
    using arda::core::Vector3;

    auto device = CreateDevice(GraphicsApi::OpenGL33);

    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position").Values();
    positions = {Vector3<float>(0, 0, 0), Vector3<float>(1, 0, 0), Vector3<float>(0, 1, 0)};
    auto& colors = mesh.attributes.Add<VertexAttributeRGBA>("color", 3);
    colors.AddColor(255, 0, 0, 255);
    colors.AddColor(0, 255, 0, 255);
    colors.AddColor(0, 0, 255, 255);
    auto indices = std::make_unique<IndicesUnsignedInt>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);

    ShaderVertexAttributeCollection shaderAttributes;
    shaderAttributes.Add({"position", VertexLocations::Position, ShaderVertexAttributeType::FloatVector3, 1});
    shaderAttributes.Add({"color", VertexLocations::Color, ShaderVertexAttributeType::FloatVector4, 1});

    MeshBuffers buffers = device->CreateMeshBuffers(mesh, shaderAttributes, BufferHint::StaticDraw);
    CHECK(buffers.attributes.size() == static_cast<std::size_t>(device->Limits().maximumNumberOfVertexAttributes));

    const auto& position = buffers.attributes[VertexLocations::Position];
    REQUIRE(position.has_value());
    CHECK(position->componentDatatype == ComponentDatatype::Float);
    CHECK(position->numberOfComponents == 3);
    CHECK(position->vertexBuffer->SizeInBytes() == 36);
    CHECK(position->vertexBuffer->CopyToSystemMemory<Vector3<float>>() == positions);

    const auto& color = buffers.attributes[VertexLocations::Color];
    REQUIRE(color.has_value());
    CHECK(color->componentDatatype == ComponentDatatype::UnsignedByte);
    CHECK(color->numberOfComponents == 4);
    CHECK(color->normalize);

    CHECK_FALSE(buffers.attributes[VertexLocations::Normal].has_value());

    REQUIRE(buffers.indexBuffer);
    CHECK(buffers.indexBuffer->Datatype() == IndexBufferDatatype::UnsignedInt);
    CHECK(buffers.indexBuffer->Count() == 3);

    shaderAttributes.Add({"normal", VertexLocations::Normal, ShaderVertexAttributeType::FloatVector3, 1});
    CHECK_THROWS_AS(device->CreateMeshBuffers(mesh, shaderAttributes, BufferHint::StaticDraw), std::invalid_argument);
}

TEST_CASE("CreateMeshBuffers splits emulated doubles into high and low parts") {
    using arda::core::Vector3;

    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const Vector3<double> value(6378137.123, -1.5, 0.25);   // an equatorial radius, in meters

    Mesh mesh;
    mesh.attributes.Add<VertexAttributeDoubleVector3>("position").Values().push_back(value);

    SUBCASE("used as positionHigh and positionLow") {
        ShaderVertexAttributeCollection shaderAttributes;
        shaderAttributes.Add({"positionHigh", VertexLocations::PositionHigh, ShaderVertexAttributeType::FloatVector3, 1});
        shaderAttributes.Add({"positionLow", VertexLocations::PositionLow, ShaderVertexAttributeType::FloatVector3, 1});

        MeshBuffers buffers = device->CreateMeshBuffers(mesh, shaderAttributes, BufferHint::StaticDraw);
        const auto& high = buffers.attributes[VertexLocations::PositionHigh];
        const auto& low = buffers.attributes[VertexLocations::PositionLow];
        REQUIRE(high.has_value());
        REQUIRE(low.has_value());
        CHECK(high->vertexBuffer == low->vertexBuffer);   // one interleaved buffer
        CHECK(high->offsetInBytes == 0);
        CHECK(low->offsetInBytes == 12);
        CHECK(high->strideInBytes == 24);
        CHECK(low->strideInBytes == 24);

        const auto parts = high->vertexBuffer->CopyToSystemMemory<Vector3<float>>();
        REQUIRE(parts.size() == 2);
        const double x = static_cast<double>(parts[0].X()) + static_cast<double>(parts[1].X());
        CHECK(x == doctest::Approx(value.X()).epsilon(1e-12));                     // high + low: ~14 digits
        CHECK(static_cast<double>(parts[0].X()) != doctest::Approx(value.X()).epsilon(1e-12));   // high alone: ~7
    }

    SUBCASE("used directly as position") {
        ShaderVertexAttributeCollection shaderAttributes;
        shaderAttributes.Add({"position", VertexLocations::Position, ShaderVertexAttributeType::FloatVector3, 1});

        MeshBuffers buffers = device->CreateMeshBuffers(mesh, shaderAttributes, BufferHint::StaticDraw);
        const auto& position = buffers.attributes[VertexLocations::Position];
        REQUIRE(position.has_value());
        CHECK(position->vertexBuffer->SizeInBytes() == 12);   // converted to floats
    }
}

TEST_CASE("Context::CreateVertexArray builds a vertex array from a mesh") {
    using arda::core::Vector3;

    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 64, "test", WindowType::Hidden);

    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions = {Vector3<float>(0, 0, 0), Vector3<float>(1, 0, 0), Vector3<float>(0, 1, 0)};
    auto indices = std::make_unique<IndicesUnsignedShort>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);

    ShaderVertexAttributeCollection shaderAttributes;
    shaderAttributes.Add({"position", VertexLocations::Position, ShaderVertexAttributeType::FloatVector3, 1});

    auto vertexArray = window->GetContext().CreateVertexArray(mesh, shaderAttributes, BufferHint::StaticDraw);
    CHECK(vertexArray->NumberOfAttributes() == 1);
    CHECK(vertexArray->Attribute(VertexLocations::Position).has_value());
    CHECK(vertexArray->MaximumArrayIndex() == 2);
    REQUIRE(vertexArray->GetIndexBuffer());
    CHECK(vertexArray->GetIndexBuffer()->Count() == 3);
}
```

The test file already has `using namespace arda::renderer;`. Add
`using namespace arda::core::geometry;` below it for these tests. The two
namespaces share no names, so both directives can be active together.

**Build and run `arda_tests`.** All the mesh code is now tested, without a
window except for the last case.

### B9. Milestone 3B: the same triangle from a `Mesh`

Replace `scene/src/main.cpp` with this version. Only the middle has
changed: the buffers, index buffer and vertex array from 3A become one
`Mesh` and one `CreateVertexArray` call. The positions are still in the xy
plane, in clip space, because there's no camera yet. Step 7 uses the
book's xz-plane triangle with a camera.

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

namespace {

constexpr const char* kVertexShader = R"(
layout(location = og_positionVertexLocation) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
})";

constexpr const char* kFragmentShader = R"(
out vec3 fragmentColor;
uniform vec3 u_color;

void main()
{
    fragmentColor = u_color;
})";

// The geometry, with no renderer types at all. In Chapter 4, functions like this
// live in core (for example, an ellipsoid tessellator).
arda::core::geometry::Mesh CreateTriangleMesh() {
    using namespace arda::core::geometry;
    using arda::core::Vector3;

    Mesh mesh;
    mesh.primitiveType = PrimitiveType::Triangles;
    mesh.frontFaceWindingOrder = WindingOrder::Counterclockwise;

    // The name must match the vertex shader's input name.
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(-0.5f, -0.5f, 0.0f);
    positions.emplace_back( 0.5f, -0.5f, 0.0f);
    positions.emplace_back( 0.0f,  0.5f, 0.0f);

    auto indices = std::make_unique<IndicesUnsignedShort>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);

    return mesh;
}

} // namespace

int main() {
    using namespace arda::renderer;
    using arda::core::Vector3;
    using arda::core::geometry::Mesh;

    try {
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(800, 600, "Step 3B: Triangle from a Mesh");
        Context& context = window->GetContext();

        auto shaderProgram = device->CreateShaderProgram(kVertexShader, kFragmentShader);
        shaderProgram->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));

        const Mesh mesh = CreateTriangleMesh();

        DrawState drawState;
        drawState.renderState.facetCulling.enabled = false;
        drawState.renderState.depthTest.enabled = false;
        drawState.shaderProgram = shaderProgram;
        // Uploads the mesh, matching "position" to the shader's input by name.
        drawState.vertexArray = context.CreateVertexArray(mesh, shaderProgram->VertexAttributes(), BufferHint::StaticDraw);

        ClearState clearState;
        clearState.color = {0.02f, 0.05f, 0.12f, 1.0f};
        SceneState sceneState;

        window->SetResizeHandler([&] {
            context.SetViewport({0, 0, window->Width(), window->Height()});
        });
        window->SetRenderFrameHandler([&] {
            context.Clear(clearState);
            context.Draw(mesh.primitiveType, drawState, sceneState);
        });
        window->Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
```

**Build and run.** The window should look exactly like Milestone 3A. Then
try breaking the name match: rename the mesh attribute to `"positions"`.
`CreateVertexArray` throws "Shader requires vertex attribute "position",
which is not present in mesh.", which is much better than the silent
`(0, 0, 0, 1)` you'd get from a wrong location in 3A.

`CreateTriangleMesh` returns a `Mesh` by value. The local `mesh` is moved
(or the move is elided entirely) into the caller's `const Mesh mesh`.

The draw uses `mesh.primitiveType`, so the mesh decides how it's drawn.

---

## CMake

The full set of additions for this step, gathered from the checkpoints.

`core/CMakeLists.txt`, in `add_library(arda_core ...)` (headers only, for IDEs):

```cmake
    include/arda/core/Half.h
    include/arda/core/geometry/Indices.h
    include/arda/core/geometry/Mesh.h
    include/arda/core/geometry/PrimitiveType.h
    include/arda/core/geometry/VertexAttribute.h
```

`renderer/CMakeLists.txt`, in `add_library(arda_renderer ...)`:

```cmake
    src/mesh/MeshBuffers.cpp
    src/vertexarray/VertexArray.cpp
    include/arda/renderer/DrawState.h
    include/arda/renderer/buffers/BufferHint.h
    include/arda/renderer/buffers/IndexBuffer.h
    include/arda/renderer/buffers/VertexBuffer.h
    include/arda/renderer/mesh/MeshBuffers.h
    include/arda/renderer/scene/SceneState.h
    include/arda/renderer/vertexarray/ComponentDatatype.h
    include/arda/renderer/vertexarray/VertexArray.h
    include/arda/renderer/vertexarray/VertexBufferAttribute.h
```

`renderer/CMakeLists.txt`, in the `ARDA_RENDERER_GL` block's `target_sources(...)`:

```cmake
    src/gl/buffers/BufferGL3x.cpp
    src/gl/vertexarray/VertexArrayGL3x.cpp
    src/gl/buffers/BufferGL3x.h
    src/gl/buffers/IndexBufferGL3x.h
    src/gl/buffers/VertexBufferGL3x.h
    src/gl/vertexarray/VertexArrayGL3x.h
```

`tests/CMakeLists.txt`, in `add_executable(arda_tests ...)`:

```cmake
    src/renderer/VertexDataTests.cpp
```

## Tests

`tests/src/renderer/VertexDataTests.cpp` was built up over the checkpoints.
When it's complete, it contains:

| Checkpoint | Test case | Needs |
|---|---|---|
| A1.9 | compile-time `BufferSource` / `IndexBufferSource` checks | nothing |
| A1.9 | VertexBuffer copies to and from system memory | device |
| A1.9 | VertexBuffer accepts any contiguous range and copies at an offset | device |
| A1.9 | IndexBuffer infers its datatype and count | device |
| A1.9 | Creating an empty buffer throws | device |
| A2.9 | compile-time `SizeInBytes(ComponentDatatype)` checks | nothing |
| A2.9 | VertexBufferAttribute validates its arguments and computes a packed stride | device |
| A2.9 | VertexArray tracks attributes and the largest vertex index | hidden window |
| A2.9 | A vertex array keeps its buffers alive | hidden window |
| A3.8 | Draw validates the draw state and draws with and without indices | hidden window |
| B8 | Half converts to and from float with IEEE rounding | nothing |
| B8 | Mesh attributes and indices | nothing |
| B8 | CreateMeshBuffers matches mesh attributes to shader attributes by name | device |
| B8 | CreateMeshBuffers splits emulated doubles into high and low parts | device |
| B8 | Context::CreateVertexArray builds a vertex array from a mesh | hidden window |

As with the earlier renderer tests, the ones that need a device or window
need a GPU and a desktop session.

---

## Complete CMake files after Step 3

The CMake sections above list only this step's additions. Here are the three
CMake files as they should look once Step 3 is done, with every step so far
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
    src/mesh/MeshBuffers.cpp
    src/shaders/BuiltinConstants.cpp
    src/shaders/UniformCollection.cpp
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
    include/arda/renderer/buffers/VertexBuffer.h
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
    include/arda/renderer/scene/SceneState.h
    include/arda/renderer/shaders/ShaderProgram.h
    include/arda/renderer/shaders/ShaderVertexAttribute.h
    include/arda/renderer/shaders/Uniform.h
    include/arda/renderer/shaders/UniformCollection.h
    include/arda/renderer/vertexarray/ComponentDatatype.h
    include/arda/renderer/vertexarray/VertexArray.h
    include/arda/renderer/vertexarray/VertexBufferAttribute.h
    include/arda/renderer/vertexarray/VertexLocations.h
    src/Cleanable.h
    src/GlfwLibrary.h
    src/shaders/BuiltinConstants.h
)

# PUBLIC include: the public headers under include/arda/renderer.
# PRIVATE src: backend sources include their siblings as "gl/DeviceGL3x.h".
target_include_directories(arda_renderer PUBLIC include PRIVATE src)

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
        src/gl/shaders/GlslPrelude.cpp
        src/gl/shaders/ShaderObjectGL3x.cpp
        src/gl/shaders/ShaderProgramGL3x.cpp
        src/gl/shaders/UniformGL3x.cpp
        src/gl/vertexarray/VertexArrayGL3x.cpp
        src/gl/ContextGL3x.h
        src/gl/DeviceGL3x.h
        src/gl/GLHandle.h
        src/gl/GraphicsWindowGL3x.h
        src/gl/TypeConverterGL3x.h
        src/gl/buffers/BufferGL3x.h
        src/gl/buffers/IndexBufferGL3x.h
        src/gl/buffers/VertexBufferGL3x.h
        src/gl/shaders/GlslPrelude.h
        src/gl/shaders/ShaderObjectGL3x.h
        src/gl/shaders/ShaderProgramGL3x.h
        src/gl/shaders/UniformGL3x.h
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
    src/renderer/ContextGL3xTests.cpp
    src/renderer/DeviceTests.cpp
    src/renderer/RenderStateTests.cpp
    src/renderer/ShaderProgramTests.cpp
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

---

## D3D11 check (3.5.5)

Step 8 implements all of this. It needs no changes to any public header
from this step.

- **Buffers:** `CreateBuffer` with `D3D11_BIND_VERTEX_BUFFER` or
  `D3D11_BIND_INDEX_BUFFER`, and `D3D11_USAGE_DEFAULT`.
  - `CopyFromSystemMemoryBytes` calls `UpdateSubresource` with a
    `D3D11_BOX` covering the byte range.
  - `CopyToSystemMemoryBytes` copies into a `D3D11_USAGE_STAGING` buffer
    with `CopySubresourceRegion`, then calls `Map(D3D11_MAP_READ)`.
  - `BufferHint` has no direct equivalent. `DEFAULT` usage suits every
    hint that `UpdateSubresource` can serve.
- **Vertex array:** `VertexArrayD3D11` needs no API object. The base class
  already stores the attributes, the index buffer and the dirty flags.
- **Input layout:** it depends on both the vertex array and the vertex
  shader, so `ContextD3D11` creates it at draw time and caches it per
  (vertex array, program) pair. Each attribute gets its own input slot,
  with `AlignedByteOffset = 0`, and its `offsetInBytes` and
  `strideInBytes` go to `IASetVertexBuffers`. `ComponentDatatype`,
  `numberOfComponents` and `normalize` together select a `DXGI_FORMAT`
  (`Float` × 3 → `R32G32B32_FLOAT`, normalized `UnsignedByte` × 4 →
  `R8G8B8A8_UNORM`).
- **Drawing:**
  - `IASetPrimitiveTopology`, which throws for `LineLoop` and `TriangleFan`.
  - `IASetIndexBuffer` with `DXGI_FORMAT_R16_UINT` or `R32_UINT`, from
    `IndexBuffer::Datatype()`.
  - `DrawIndexed(count, offset, 0)` with an index buffer, otherwise
    `Draw(count, offset)`. Both take counts and offsets in elements, not
    bytes.
  - `MaximumArrayIndex` is in the base class, so both backends use it.
  - Primitive restart is always on for strips in D3D11, with the fixed
    maximum index. That's why the GL backend fixes the index the same way.
- **Contexts:** D3D11 has no per-context restriction on vertex data, but
  `Context::CreateVertexArray` works the same way.
- **Meshes:** `CreateMeshBuffers` and the `CreateVertexArray` overloads
  are API-agnostic, so no D3D11 work is needed. Shaders name their inputs
  with semantics (`float4 position : position0`), and Step 8's reflection
  turns those into the same `ShaderVertexAttribute` names that mesh
  attributes are matched against.

## Checklist

- [ ] Core: `Half.h`, `PrimitiveType.h`, `VertexAttribute.h`, `Indices.h`, `Mesh.h` (and `Vector4.h`'s includes from Step 2)
- [ ] `BufferHint.h`, `VertexBuffer.h`, `IndexBuffer.h`
- [ ] `Device::CreateVertexBuffer`, `CreateIndexBuffer`; GL `BufferGL3x`, `VertexBufferGL3x`, `IndexBufferGL3x`, `DeviceGL3x::DoCreate*Buffer`
- [ ] **Checkpoint 1:** buffer tests pass
- [ ] `ComponentDatatype.h`, `VertexBufferAttribute.h`, `VertexArray.h` and `VertexArray.cpp`
- [ ] `Context::CreateVertexArray()`; GL `VertexArrayGL3x` (`Clean`, `Attach`, `Detach`), `ContextGL3x::DoCreateVertexArray`
- [ ] **Checkpoint 2:** vertex array tests pass
- [ ] `SceneState.h` (empty), and `DrawState` with `shaderProgram` and `vertexArray`
- [ ] `Context::Draw` (2 overloads), `VerifyDraw`
- [ ] GL: `ContextGL3x::DoDraw`, `DoDrawRange`, `ApplyBeforeDraw`, `ApplyVertexArray`, `ApplyShaderProgram`, `ApplyPrimitiveRestartIndex`
- [ ] GL: `ShaderProgramGL3x::Clean`, and `ToGL` for `BufferHint`, `ComponentDatatype`, `PrimitiveType` and `IndexBufferDatatype`
- [ ] **Checkpoint 3 and Milestone 3A:** draw tests pass, and the red triangle appears from raw buffers
- [ ] `MeshBuffers.h`, `Device::CreateMeshBuffers` (every attribute type, indices, emulated doubles)
- [ ] `Context::CreateVertexArray(MeshBuffers)` and `CreateVertexArray(Mesh, ...)`
- [ ] **Checkpoint 4 and Milestone 3B:** mesh tests pass, and the same triangle appears from a `Mesh`
