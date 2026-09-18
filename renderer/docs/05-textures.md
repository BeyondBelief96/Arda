# Step 5: Textures (3.6)

**Goal:** 2D textures, pixel buffers that move image data into and out of
textures, samplers, the context's texture units, and the `og_textureN`
automatic uniforms. The milestone is a textured triangle whose image comes
from a file (or from a generated checkerboard).

**Read:** 3.6.1 "Creating Textures", 3.6.2 "Samplers", 3.6.3 "Rendering with
Textures", 3.6.4 "GL Renderer Implementation" (Listings 3.25–3.28). Section
3.6.5 (textures in Direct3D) goes with this step's D3D11 check.

## OpenGlobe reference

All paths are under `Source/Renderer/`. A local clone is the quickest way to
read them side by side with this guide.

| File | What to take from it |
|---|---|
| [Textures/TextureFormat.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureFormat.cs), [ImageFormat.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/ImageFormat.cs), [ImageDatatype.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/ImageDatatype.cs) | The three format enums, in full |
| [Textures/Texture2DDescription.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/Texture2DDescription.cs) | `ColorRenderable`, `DepthRenderable`, `DepthStencilRenderable`, `ApproximateSizeInBytes` and its per-format size table |
| [Textures/Texture2D.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/Texture2D.cs) | `CopyFromBuffer` and `CopyToBuffer` overloads, `Save` (`SaveColor`, `SaveDepth`, `SaveRed`, `SaveFloat`) |
| [Textures/TextureUtility.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureUtility.cs) | `RequiredSizeInBytes`, `NumberOfChannels`, `SizeInBytes(ImageDatatype)`, `IsPowerOfTwo` |
| [Textures/TextureSampler.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureSampler.cs), [TextureSamplers.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureSamplers.cs), [TextureMinificationFilter.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureMinificationFilter.cs), [TextureMagnificationFilter.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureMagnificationFilter.cs), [TextureWrap.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureWrap.cs) | Samplers and the four built-in ones |
| [Textures/TextureUnit.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureUnit.cs), [TextureUnits.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Textures/TextureUnits.cs) | The public texture unit interface |
| [Buffers/WritePixelBuffer.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/WritePixelBuffer.cs), [ReadPixelBuffer.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/ReadPixelBuffer.cs), [PixelBufferHint.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Buffers/PixelBufferHint.cs) | Pixel buffers |
| [GL3x/Buffers/PixelBufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/PixelBufferGL3x.cs), [WritePixelBufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/WritePixelBufferGL3x.cs), [ReadPixelBufferGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Buffers/ReadPixelBufferGL3x.cs) | GL pixel buffers, the hint tables (`_bufferHints`) and the bitmap row flip |
| [GL3x/Textures/Texture2DGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Textures/Texture2DGL3x.cs) | Constructor, `BindToLastTextureUnit`, `CopyFromBuffer`, `CopyToBuffer`, `GenerateMipmaps`, `ApplySampler` |
| [GL3x/Textures/TextureSamplerGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Textures/TextureSamplerGL3x.cs) | The `glSamplerParameter` calls and the anisotropy extension check |
| [GL3x/Textures/TextureUnitGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Textures/TextureUnitGL3x.cs), [TextureUnitsGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Textures/TextureUnitsGL3x.cs) | Dirty tracking, `Clean`, `Validate`, and `CleanLastTextureUnit` |
| [GL3x/TypeConverterGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/TypeConverterGL3x.cs) | `To(TextureFormat)` (line 507), `To(ImageFormat)` (616), `To(ImageDatatype)` (663), `TextureToPixelFormat` (720), `TextureToPixelType` (816), filters and wrap modes (977–1030) |
| [Device.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Device.cs) | `CreateWritePixelBuffer` (392), `CreateTexture2D` overloads, `CreateTexture2DFromBitmap` (417), `CreateTexture2DSampler` (475), `TextureSamplers`, and the `TextureUniform` registration (lines 45–48) |
| [Shaders/LinkAutomaticUniforms/TextureUniform.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/LinkAutomaticUniforms/TextureUniform.cs) | `og_textureN` |

## Files

```
vcpkg.json                              UPDATE  add "stb"
CMakeLists.txt                          UPDATE  find_package(Stb)
renderer/
  CMakeLists.txt                        UPDATE  stb include path, new sources
  include/arda/renderer/
    Exceptions.h                        UPDATE  InsufficientVideoCardException
    Device.h                            UPDATE  pixel buffer, texture and sampler creation; Samplers(); ReleaseCommon()
    Context.h                           UPDATE  GetTextureUnits()
    buffers/
      PixelBufferHint.h                 NEW
      WritePixelBuffer.h                NEW
      ReadPixelBuffer.h                 NEW
    textures/
      Image.h                           NEW     replaces System.Drawing.Bitmap
      ImageFormat.h                     NEW     ImageFormat, ImageDatatype
      TextureFormat.h                   NEW
      Texture2DDescription.h            NEW
      Texture2D.h                       NEW
      TextureSampler.h                  NEW     filter and wrap enums, TextureSamplerDescription, TextureSampler, TextureSamplers
      TextureUnits.h                    NEW     TextureUnit, TextureUnits
      TextureUtility.h                  NEW
  src/
    Context.cpp                         UPDATE  creates the TextureUnits
    Device.cpp                          UPDATE  creation functions; InitializeCommon registers og_textureN and creates the samplers
    textures/
      Image.cpp                         NEW     stb_image / stb_image_write (the one implementation TU)
      Texture2D.cpp                     NEW     validation, Save
      Texture2DDescription.cpp          NEW
      TextureUnits.cpp                  NEW
      TextureUtility.cpp                NEW
      CreateTexture2D.cpp               NEW     Device::CreateTexture2D(const Image&, ...)
    shaders/automaticuniforms/
      TextureUniform.h                  NEW
    gl/
      buffers/WritePixelBufferGL3x.h    NEW
      buffers/ReadPixelBufferGL3x.h     NEW
      textures/Texture2DGL3x.h / .cpp   NEW
      textures/TextureSamplerGL3x.h / .cpp   NEW
      ContextGL3x.h / .cpp              UPDATE  CleanTextureUnits, BindTextureUnit
      DeviceGL3x.h / .cpp               UPDATE  DoCreate* functions; the destructor calls ReleaseCommon
      TypeConverterGL3x.h / .cpp        UPDATE  texture enums
scene/src/main.cpp                      UPDATE  the textured triangle
tests/
  CMakeLists.txt                        UPDATE
  src/renderer/TextureTests.cpp         NEW
```

---

## The big picture

A texture is an image that lives in GPU memory, plus the rules for reading
it. This step builds every stage a pixel passes through, from a file on disk
to a fragment shader, and back again:

```
 image file (PNG, JPEG, ...)
     │  Image::Load (stb_image), rows flipped to bottom-first
     ▼
 Image                        system memory: width, height, channels, bytes
     │  WritePixelBuffer::CopyFromImage            glBufferSubData into a GL_PIXEL_UNPACK_BUFFER
     ▼
 WritePixelBuffer             GPU-visible memory owned by GL
     │  Texture2D::CopyFromBuffer                  glTexSubImage2D reads from the bound buffer
     ▼
 Texture2D                    GPU texture object: format, size, mipmaps
     │  context.GetTextureUnits()[N].SetTexture(...) and .SetSampler(...)
     ▼
 texture unit N               texture + sampler, bound lazily in Context::Draw
     │  sampler2D og_textureN = N                  set once by a link automatic uniform
     ▼
 fragment shader              texture(og_textureN, uv)

 and back:  Texture2D::CopyToBuffer → ReadPixelBuffer (GL_PIXEL_PACK_BUFFER, glGetTexImage)
            → CopyToSystemMemory / CopyToImage / Texture2D::Save
```

The book splits this into creating textures (3.6.1), samplers (3.6.2) and
rendering with them (3.6.3). This guide builds it in the order you can test
it:

| Part | You build | Checkpoint |
|---|---|---|
| 1 | stb_image in vcpkg and CMake, `Image` | Save and load a PNG in a test |
| 2 | Formats, `TextureUtility`, `Texture2DDescription` | CPU-only tests pass |
| 3 | Pixel buffers (public and GL) | Bytes round-trip through a GL buffer |
| 4 | `Texture2D`, `Texture2DGL3x`, the type converter | Pixels round-trip through a texture |
| 5 | Samplers and `Device::Samplers()` | The built-in samplers exist |
| 6 | Texture units on the `Context`, `CleanTextureUnits` | A draw checks its texture units |
| 7 | The `og_textureN` automatic uniforms | A sampler uniform gets its unit index |
| 8 | The textured triangle | **Milestone** |

C++ features explained in depth in this step:

| Feature | Where |
|---|---|
| Header-only C libraries, the one-definition rule, `STB_IMAGE_IMPLEMENTATION` | 1.2 |
| `extern "C"` and linkage | 1.2 |
| `std::filesystem::path` | 1.3 |
| `std::unique_ptr` with a custom deleter for C-allocated memory | 1.4 |
| `std::vector<std::byte>` vs `std::vector<std::uint8_t>` | 1.4 |
| Captureless lambdas as C callbacks | 1.4 |
| `std::ranges` algorithms (`swap_ranges`, `minmax_element`) | 1.4, 4.2 |
| Integer overflow and `std::size_t` arithmetic | 2.3 |
| `friend` and private constructors | 6.1 |
| `std::bitset` and `std::array` (and why the dirty units don't use them) | 6.1 |

---

## Part 1: Loading images with stb_image

OpenGlobe loads images with `System.Drawing.Bitmap`, which is part of .NET.
C++ has no standard image library, so this step brings in
[stb_image](https://github.com/nothings/stb): two public-domain, single-file
C libraries, `stb_image.h` (decode PNG, JPEG, BMP, TGA, GIF, HDR, PSD) and
`stb_image_write.h` (encode PNG, BMP, TGA, JPEG).

> **Why:** the alternatives were rejected for these reasons.
> - **libpng + libjpeg:** two libraries, each with a long, callback-heavy C
>   API, for what is one function call in stb.
> - **WIC (Windows Imaging Component):** Windows only, and COM-based. The
>   renderer also builds on macOS.
> - **FreeImage, DevIL:** large, with more restrictive licenses, and far more
>   than the book needs.
>
> stb covers every format the book's examples load, compiles in a second, and
> has no dependencies. Its costs are that it is C, not C++, and that it is a
> "header-only" library, which needs one special source file. Both are
> explained below.

`Image` replaces `Bitmap`. It is a plain struct: size, channel count and
bytes. It lives in the renderer, not in core, because it exists to feed
textures. Its one important rule is **row order**: `pixels[0]` is the
**bottom-left** pixel.

### 1.1 Add stb to vcpkg

`vcpkg.json`:

```json
{
    "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
    "name": "arda",
    "version": "0.1.0",
    "description": "3D Engine Design for Virtual Globes",
    "builtin-baseline": "04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4",
    "dependencies": ["glfw3", "glad", "doctest", "stb"]
}
```

The next CMake configure installs it. The vcpkg `stb` port installs only
headers, plus a small `FindStb.cmake` module that sets `Stb_INCLUDE_DIR`.

### 1.2 Find it in CMake

In the root `CMakeLists.txt`, next to the other `find_package` calls:

```cmake
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(Stb REQUIRED)   # header-only; sets Stb_INCLUDE_DIR
```

There is no `CONFIG` here. The port ships a find module (`FindStb.cmake`)
rather than a package config file, so CMake must use module mode. There is
also no `Stb::Stb` target to link, only an include directory.

In `renderer/CMakeLists.txt`, after the existing `target_include_directories`:

```cmake
# stb is a C library we only include from src/textures/Image.cpp.
# SYSTEM: treat its headers like system headers, so their warnings
#         (sprintf, unused functions) don't show up in our build.
# PRIVATE: no public renderer header includes stb.
target_include_directories(arda_renderer SYSTEM PRIVATE ${Stb_INCLUDE_DIR})
```

> **C++ note — header-only libraries and the one-definition rule:**
> C++ compiles each `.cpp` file separately into an object file, and the
> linker then joins them. The **one-definition rule (ODR)** says a normal
> (non-`inline`) function must be *defined* in exactly one object file,
> though it may be *declared* in many. Two definitions give a linker error
> such as MSVC's `LNK2005: already defined`. Zero definitions give
> `LNK2019: unresolved external symbol`.
>
> A header-only library like stb puts both the declarations and the
> definitions in one header, and hides the definitions behind a macro:
>
> ```cpp
> // stb_image.h, simplified
> unsigned char* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp);  // declaration
>
> #ifdef STB_IMAGE_IMPLEMENTATION
> unsigned char* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp) {
>     // ...thousands of lines of decoder...
> }
> #endif
> ```
>
> Every file that includes `stb_image.h` gets the declarations. **Exactly
> one** `.cpp` file writes `#define STB_IMAGE_IMPLEMENTATION` *before* the
> `#include`, and that file compiles the definitions. In arda that file is
> `src/textures/Image.cpp`. Never put the `#define` in a header: every `.cpp`
> that included that header would define the functions again, and the link
> would fail with hundreds of duplicate symbols.
>
> C# has nothing like this, because an assembly already contains compiled
> code and there is no textual `#include`. The C++ pattern exists so a
> library can be one file you drop into a project with no build system.

> **C++ note — `extern "C"` and linkage:** C++ allows overloading, so the
> compiler encodes each function's parameter types into its symbol name. This
> is called **name mangling**. MSVC might turn `stbi_load` into something like
> `?stbi_load@@YAPEAEPEBDPEAH11H@Z`. A C compiler has no overloading and
> emits plain `stbi_load`. If a C++ file declared a function that was
> compiled as C, the linker would look for the mangled name and fail.
>
> `extern "C"` tells the C++ compiler to give a function **C linkage**: an
> unmangled name, and no overloading. C libraries wrap their headers so they
> work from both languages:
>
> ```cpp
> #ifdef __cplusplus      // defined only by C++ compilers
> extern "C" {
> #endif
>
> unsigned char* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp);
>
> #ifdef __cplusplus
> }
> #endif
> ```
>
> stb, glad and GLFW all do this. It matters most for glad and GLFW, whose
> vcpkg builds really are compiled as C. For stb, the implementation is
> compiled inside our own C++ file, so both sides would match anyway, but the
> wrapper keeps the symbol names simple and identical everywhere.

### 1.3 `include/arda/renderer/textures/Image.h`

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace arda::renderer {

// An image in system memory with 8 bits per channel. Replaces System.Drawing.Bitmap.
//
// Rows are stored bottom to top with no padding between them: pixels[0] is
// the first channel of the bottom-left pixel. That is the order
// glTexImage2D expects, so an Image can be uploaded without flipping
// (PixelBufferGL3x.CopyFromBitmap flips bitmaps for the same reason).
struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;                    // 1 (gray), 2 (gray, alpha), 3 (RGB) or 4 (RGBA)
    std::vector<std::uint8_t> pixels;    // width * height * channels bytes

    // width * height * channels, computed in std::size_t so it can't overflow int.
    std::size_t SizeInBytes() const {
        return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
               static_cast<std::size_t>(channels);
    }

    // Loads a PNG, JPEG, BMP, TGA, GIF, HDR or PSD file.
    // desiredChannels = 0 keeps the file's channel count; 1 to 4 converts to
    // that many channels (for example, 4 adds an opaque alpha to an RGB file).
    // Throws std::invalid_argument for a bad desiredChannels and
    // std::runtime_error if the file can't be read or decoded.
    static Image Load(const std::filesystem::path& path, int desiredChannels = 0);

    // Writes a PNG. Throws std::invalid_argument if the image is empty or its
    // pixels don't match its size, and std::runtime_error if writing fails.
    void SavePng(const std::filesystem::path& path) const;
};

} // namespace arda::renderer
```

`Image` is an aggregate (no constructors), so tests can build one field by
field, and `std::vector` gives it value semantics: copying an `Image` copies
its pixels, and moving it moves them. Aggregates are covered in
[Step 1](01-state-management.md).

> **C++ note — `std::filesystem::path`:** `path` (C++17, `<filesystem>`) is
> a file path stored in the operating system's native encoding: `wchar_t`
> (UTF-16) on Windows and `char` (usually UTF-8) on macOS and Linux. It
> converts implicitly from string literals and `std::string`, so
> `Image::Load("assets/earth.png")` works. It also has path operations built
> in:
>
> ```cpp
> std::filesystem::path dir = std::filesystem::temp_directory_path();
> std::filesystem::path file = dir / "arda" / "test.png";   // operator/ joins with the right separator
> file.extension();   // ".png"
> file.filename();    // "test.png"
> file.string();      // narrow std::string, for error messages
> ```
>
> It is roughly C#'s `System.IO.Path` and `FileInfo` in one type. The
> encoding is the reason `Image::Load` doesn't call `stbi_load(const char*)`.
> On Windows, a `char*` filename goes through the ANSI code page, so a path
> such as `C:\Users\Zoë\earth.png` can fail to open. `std::ifstream` accepts
> a `path` and uses the wide Windows API, so `Image::Load` opens the file with
> it and hands the bytes to `stbi_load_from_memory`. Only the error messages
> use `path.string()`. That conversion can lose characters on Windows, which
> is acceptable in a message.

### 1.4 `src/textures/Image.cpp`

This is the file that compiles stb's implementation. Read it top to bottom.
The C++ notes after it explain each piece that talks to C.

```cpp
#include <arda/renderer/textures/Image.h>

// The one translation unit that compiles stb's implementation (see the
// one-definition rule note). The defines must come before the includes.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>

namespace arda::renderer {

namespace {

// stb_image allocates the decoded pixels with its own allocator (malloc by
// default), so they must be released with stbi_image_free, never delete.
struct StbiImageDeleter {
    void operator()(stbi_uc* data) const { stbi_image_free(data); }
};
using StbiImagePtr = std::unique_ptr<stbi_uc, StbiImageDeleter>;

// Reads a whole file. std::ifstream takes the path in its native encoding,
// so Unicode paths work on Windows.
std::vector<std::byte> ReadFile(const std::filesystem::path& path) {
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("Could not read '" + path.string() + "': " + error.message());
    }
    // stbi_load_from_memory takes the length as an int.
    if (size > static_cast<std::uintmax_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("'" + path.string() + "' is too large to decode");
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("Could not read '" + path.string() + "'");
    }
    return bytes;
}

// Reverses the order of the rows: top-to-bottom becomes bottom-to-top, and back.
void FlipRows(std::span<std::uint8_t> pixels, std::size_t rowSizeInBytes) {
    if (rowSizeInBytes == 0) {
        return;
    }
    const std::size_t rows = pixels.size() / rowSizeInBytes;
    if (rows < 2) {
        return;   // also keeps "rows - 1" below from wrapping around when rows == 0
    }
    for (std::size_t top = 0, bottom = rows - 1; top < bottom; ++top, --bottom) {
        std::ranges::swap_ranges(pixels.subspan(top * rowSizeInBytes, rowSizeInBytes),
                                 pixels.subspan(bottom * rowSizeInBytes, rowSizeInBytes));
    }
}

} // namespace

Image Image::Load(const std::filesystem::path& path, int desiredChannels) {
    if (desiredChannels < 0 || desiredChannels > 4) {
        throw std::invalid_argument("Image::Load: desiredChannels must be 0, 1, 2, 3 or 4");
    }

    const std::vector<std::byte> file = ReadFile(path);

    int width = 0;
    int height = 0;
    int fileChannels = 0;
    const StbiImagePtr data(stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(file.data()), static_cast<int>(file.size()),
        &width, &height, &fileChannels, desiredChannels));
    if (!data) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Could not decode '" + path.string() + "': " +
                                 (reason != nullptr ? reason : "unknown error"));
    }

    Image image;
    image.width = width;
    image.height = height;
    image.channels = desiredChannels != 0 ? desiredChannels : fileChannels;
    image.pixels.assign(data.get(), data.get() + image.SizeInBytes());

    // Image files store the top row first. GL (and Image) want the bottom row first.
    FlipRows(image.pixels, static_cast<std::size_t>(width) * static_cast<std::size_t>(image.channels));
    return image;
}   // data's destructor calls stbi_image_free here, or during stack unwinding if assign throws

void Image::SavePng(const std::filesystem::path& path) const {
    if (width <= 0 || height <= 0 || channels < 1 || channels > 4 || pixels.size() != SizeInBytes()) {
        throw std::invalid_argument("Image::SavePng: the image is empty or its pixels don't match its size");
    }
    const std::size_t rowSizeInBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    if (rowSizeInBytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Image::SavePng: the image is too wide");
    }

    // PNG stores the top row first, so write a flipped copy.
    std::vector<std::uint8_t> topRowFirst = pixels;
    FlipRows(topRowFirst, rowSizeInBytes);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open '" + path.string() + "' for writing");
    }

    // stb_image_write calls this for each chunk of the encoded file. It has
    // no captures, so it converts to the plain C function pointer stb expects.
    // The stream comes back through the void* context argument.
    auto writeToStream = [](void* context, void* data, int size) {
        static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), size);
    };

    const int ok = stbi_write_png_to_func(writeToStream, &file, width, height, channels,
                                          topRowFirst.data(), static_cast<int>(rowSizeInBytes));
    if (ok == 0 || !file) {
        throw std::runtime_error("Could not write '" + path.string() + "'");
    }
}

} // namespace arda::renderer
```

> **OpenGL note — row order:** `glTexImage2D` and `glTexSubImage2D` treat
> the **first row in memory as the bottom of the texture**, the row with
> texture coordinate t = 0. Almost every image file format stores the **top**
> row first, because that is how screens scan out and how people read. Upload
> a PNG's bytes directly and the image appears upside down.
>
> There are three common fixes:
> - Flip the rows on the CPU once, when loading. OpenGlobe does this in
>   `PixelBufferGL3x.CopyFromBitmap`, and arda does it in `Image::Load`, so
>   an `Image` is always bottom-row-first.
> - Flip the t coordinate in every shader (`1.0 - uv.y`). This is easy to
>   forget, and it disagrees with render-to-texture results (Step 6), which
>   are already bottom-row-first.
> - Use stb's `stbi_set_flip_vertically_on_load`. It works, but it is a global
>   (or thread-local) switch that affects every other `stbi_load` caller in the
>   process. `FlipRows` keeps the rule inside `Image`.
>
> `SavePng` flips back, because PNG wants the top row first. Textures read
> back from GL (`Texture2D::Save`) are bottom-row-first too, so they go
> through the same `Image` path and come out the right way up.

> **C++ note — `std::unique_ptr` with a custom deleter for C memory:**
> `stbi_load_from_memory` returns memory allocated with `malloc`. Freeing it
> with `delete` is undefined behaviour (mismatched allocators), and forgetting
> to free it is a leak, especially when an exception is thrown between the
> load and the free. `std::unique_ptr<T, Deleter>` calls `Deleter{}(pointer)`
> in its destructor instead of `delete`:
>
> ```cpp
> struct StbiImageDeleter {
>     void operator()(stbi_uc* data) const { stbi_image_free(data); }
> };
> std::unique_ptr<stbi_uc, StbiImageDeleter> data(stbi_load_from_memory(...));
> // data.get() is the raw pointer; stbi_image_free runs when data goes out of scope
> ```
>
> A function pointer also works as the deleter:
> `std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> data(ptr, &stbi_image_free);`.
> The struct is better. It has no data members, so the `unique_ptr` stays the
> size of one pointer, and you can't forget to pass the function. With a
> function-pointer deleter, the `unique_ptr` stores the pointer too (16 bytes
> on 64-bit), and the constructor needs it every time.
>
> This is the same RAII idea as `GLHandle` and its deleters in
> [Step 0](00-setup.md). The C# equivalent is a `SafeHandle` or a `using`
> block around an `IDisposable`. The difference is that C++ runs the cleanup
> automatically at the end of the scope, on every path out of it.

> **C++ note — `std::vector<std::byte>` vs `std::vector<std::uint8_t>`:**
> Both are one byte per element, but they mean different things.
> - **`std::uint8_t`** is a number. `Image::pixels` uses it because channel
>   values are numbers you compute with (`255 - value`, averages, and so on).
> - **`std::byte`** (C++17, `<cstddef>`) is raw memory. It is an `enum class`
>   with no arithmetic, only the bitwise operators, and you convert it
>   explicitly with `std::to_integer<int>(b)`. `ReadFile` returns
>   `std::vector<std::byte>` because the file's contents are opaque until stb
>   decodes them. The buffer interfaces from
>   [Step 3](03-vertex-data.md) (`std::span<const std::byte>`) use it for the
>   same reason, and the D3D11 backend stores pixel buffers as
>   `std::vector<std::byte>` in [Step 8](08-direct3d11.md).
>
> C#'s `byte` is always a number, so it plays both roles. In C++, the type
> documents whether you're allowed to do arithmetic on the data.
>
> To hand a `std::byte*` to a C API that wants `unsigned char*`, use
> `reinterpret_cast<const stbi_uc*>(file.data())`. `reinterpret_cast` between
> unrelated pointer types is normally a red flag, because accessing an object
> through the wrong type is undefined behaviour ("strict aliasing"). `char`,
> `unsigned char` and `std::byte` are the exception: they may examine the
> bytes of any object. The same rule makes `reinterpret_cast<char*>` safe in
> `file.read`.

> **C++ note — captureless lambdas as C callbacks:** C has no closures. A C
> library that calls you back takes a **function pointer** plus a `void*`
> "context" that it passes through untouched:
>
> ```c
> typedef void stbi_write_func(void* context, void* data, int size);
> int stbi_write_png_to_func(stbi_write_func* func, void* context, int w, int h, int comp,
>                            const void* data, int stride_in_bytes);
> ```
>
> A C++ lambda with **no captures** converts implicitly to a plain function
> pointer with the same signature, so it can be passed directly. Any state it
> needs goes in through `context`. `SavePng` passes `&file`, and the lambda
> casts it back with `static_cast<std::ofstream*>(context)`. A lambda that
> captured `file` (`[&file](...)`) would have state, so it could not convert
> to a function pointer, and the call would not compile. The `void*` is the
> C-style version of the target object a C# delegate carries around.
>
> `stbi_write_png_to_func` is used instead of `stbi_write_png(const char*
> filename, ...)` for the same Unicode-path reason as in `Load`.

> **C++ note — `std::ranges` algorithms:** `<algorithm>` has range versions
> of the classic algorithms in `std::ranges`, which take whole ranges instead
> of iterator pairs. `std::ranges::swap_ranges(a, b)` swaps the elements of
> two equal-length ranges one by one. `FlipRows` applies it to two
> `std::span` "windows" into the same vector (`subspan(offset, count)` is a
> view, not a copy), so each pair of rows is swapped in place with no
> temporary row buffer. It is `Array.Reverse` for rows, without the
> allocation. `Texture2D::Save` uses another one, `minmax_element`, in 4.2.

**Checkpoint:** add `src/textures/Image.cpp` (and `Image.h`, if you list
headers) to `arda_renderer` in `renderer/CMakeLists.txt`, reconfigure, and
build. The first configure after changing `vcpkg.json` installs stb. Then
create `tests/src/renderer/TextureTests.cpp` from the
[Tests](#tests-testssrcrenderertexturetestscpp) section with only what Part 1
needs: `<doctest/doctest.h>`, `Image.h`, the standard headers, the helper
functions and the two `Image` test cases. Add the other includes and test
cases as you reach the parts that need them. Add the file to
`tests/CMakeLists.txt`, and run `arda_tests`. If the link fails
with "unresolved external symbol stbi_load_from_memory", the
`#define STB_IMAGE_IMPLEMENTATION` is missing or comes after the include. If
it fails with "already defined", the define is in a second file.

---

## Part 2: Formats, `TextureUtility` and `Texture2DDescription`

A texture upload always involves **two** descriptions of the pixels, and
keeping them apart is the key to this whole step:

- **`TextureFormat`** says how the *texture* stores its texels on the GPU:
  `RedGreenBlueAlpha8`, `Red32f`, `Depth24`, and so on. It is fixed when the
  texture is created.
- **`ImageFormat` + `ImageDatatype`** say how the bytes in *your* buffer are
  laid out: which channels, in which order (`RedGreenBlue`, `BlueGreenRed`,
  ...), and what type each channel is (`UnsignedByte`, `Float`, ...). They
  are given with every copy.

GL converts from one to the other during every upload and download.

> **OpenGL note — internal format vs pixel format and type:**
> `glTexImage2D(target, level, internalformat, width, height, border, format, type, data)`
> takes both descriptions.
> - **`internalformat`** is the texture's storage, such as `GL_RGBA8` (four
>   8-bit normalized channels) or `GL_R32F` (one 32-bit float). Always use a
>   **sized** format like these. An unsized one such as `GL_RGBA` lets the
>   driver pick the precision.
> - **`format` and `type`** describe the memory that `data` points to, such as
>   `GL_BGRA` + `GL_UNSIGNED_BYTE`, or `GL_RED` + `GL_FLOAT`.
>
> GL converts between them. Uploading `GL_RGB`/`GL_UNSIGNED_BYTE` data into a
> `GL_RGBA8` texture fills alpha with 1.0, and reading a `GL_RGBA8` texture
> back as `GL_RGBA`/`GL_FLOAT` gives values in [0, 1].
>
> GL still checks that the combination is legal, **even when `data` is null**
> and you are only allocating:
> - Integer textures (`GL_R32UI`, `GL_RGBA8I`, ...) need an `_INTEGER`
>   format, such as `GL_RED_INTEGER`.
> - Depth textures need `GL_DEPTH_COMPONENT`.
> - Depth/stencil textures need `GL_DEPTH_STENCIL` with a packed type:
>   `GL_UNSIGNED_INT_24_8` or `GL_FLOAT_32_UNSIGNED_INT_24_8_REV`.
>
> Any other combination fails with `GL_INVALID_OPERATION` and leaves the
> texture with no storage. That is why the type converter in 4.3 has
> `ToPixelFormat(TextureFormat)` and `ToPixelType(TextureFormat)`: they pick
> a legal allocation-time format and type for each internal format
> (`TextureToPixelFormat` and `TextureToPixelType` in OpenGlobe).

### 2.1 `include/arda/renderer/textures/ImageFormat.h`

Both enums are complete ports of the C# ones, in the same order.

```cpp
#pragma once

namespace arda::renderer {

// How the pixels in a pixel buffer are laid out: which channels, in which
// order (ImageFormat.cs). Maps to the format argument of glTexSubImage2D.
enum class ImageFormat {
    StencilIndex,
    DepthComponent,
    Red,
    Green,
    Blue,
    RedGreenBlue,
    RedGreenBlueAlpha,
    BlueGreenRed,
    BlueGreenRedAlpha,
    RedGreen,
    RedGreenInteger,
    DepthStencil,
    RedInteger,
    GreenInteger,
    BlueInteger,
    RedGreenBlueInteger,
    RedGreenBlueAlphaInteger,
    BlueGreenRedInteger,
    BlueGreenRedAlphaInteger,
};

// The type of each channel or, for packed types, of the whole pixel
// (ImageDatatype.cs). Maps to the type argument of glTexSubImage2D.
//
// Packed types store every channel of a pixel in one value. For example,
// UnsignedShort565 is a 16-bit value with 5 bits of red, 6 of green and 5 of
// blue. "Reversed" types store the channels in the opposite bit order.
enum class ImageDatatype {
    Byte,
    UnsignedByte,
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Float,
    HalfFloat,
    UnsignedByte332,
    UnsignedShort4444,
    UnsignedShort5551,
    UnsignedInt8888,
    UnsignedInt1010102,
    UnsignedByte233Reversed,
    UnsignedShort565,
    UnsignedShort565Reversed,
    UnsignedShort4444Reversed,
    UnsignedShort1555Reversed,
    UnsignedInt8888Reversed,
    UnsignedInt2101010Reversed,
    UnsignedInt248,                 // depth/stencil: 24-bit depth, 8-bit stencil
    UnsignedInt10F11F11FReversed,
    UnsignedInt5999Reversed,
    Float32UnsignedInt248Reversed,  // depth/stencil: 32-bit float depth, 24 unused bits, 8-bit stencil
};

} // namespace arda::renderer
```

### 2.2 `include/arda/renderer/textures/TextureFormat.h`

```cpp
#pragma once

namespace arda::renderer {

// How a texture stores its texels on the GPU (TextureFormat.cs).
//   8, 16    normalized unsigned integers: stored as integers, read as floats in [0, 1]
//   16f, 32f floating point
//   i, ui    signed and unsigned integers: read as ints in the shader (isampler2D, usampler2D)
//   S...     sRGB: stored gamma-encoded, converted to linear when sampled
enum class TextureFormat {
    RedGreenBlue8,
    RedGreenBlue16,
    RedGreenBlueAlpha8,
    RedGreenBlue10A2,
    RedGreenBlueAlpha16,
    Depth16,
    Depth24,
    Red8,
    Red16,
    RedGreen8,
    RedGreen16,
    Red16f,
    Red32f,
    RedGreen16f,
    RedGreen32f,
    Red8i,
    Red8ui,
    Red16i,
    Red16ui,
    Red32i,
    Red32ui,
    RedGreen8i,
    RedGreen8ui,
    RedGreen16i,
    RedGreen16ui,
    RedGreen32i,
    RedGreen32ui,
    RedGreenBlueAlpha32f,
    RedGreenBlue32f,
    RedGreenBlueAlpha16f,
    RedGreenBlue16f,
    Depth24Stencil8,
    Red11fGreen11fBlue10f,
    RedGreenBlue9E5,
    SRedGreenBlue8,
    SRedGreenBlue8Alpha8,
    Depth32f,
    Depth32fStencil8,
    RedGreenBlueAlpha32ui,
    RedGreenBlue32ui,
    RedGreenBlueAlpha16ui,
    RedGreenBlue16ui,
    RedGreenBlueAlpha8ui,
    RedGreenBlue8ui,
    RedGreenBlueAlpha32i,
    RedGreenBlue32i,
    RedGreenBlueAlpha16i,
    RedGreenBlue16i,
    RedGreenBlueAlpha8i,
    RedGreenBlue8i,
};

} // namespace arda::renderer
```

Each backend's type converter must handle every value. The GL one maps all
50 in 4.3. The D3D11 one ([Step 8](08-direct3d11.md)) maps the formats DXGI
has and throws for the rest. `enum class` and `switch` without `default:`
are covered in [Step 0](00-setup.md) and [Step 1](01-state-management.md).

### 2.3 `include/arda/renderer/textures/TextureUtility.h` and `src/textures/TextureUtility.cpp`

`TextureUtility.cs` is an `internal static class`. The C++ version is a
namespace of free functions. It is public, because the D3D11 backend and the
pixel buffers use it too.

`RequiredSizeInBytes` answers the question every upload asks: how many bytes
must a buffer hold for `width × height` pixels in this format, with each row
padded to `rowAlignment`?

> **OpenGL note — `GL_UNPACK_ALIGNMENT`, `GL_PACK_ALIGNMENT` and row
> padding:** GL doesn't take a row pitch (stride) argument. It assumes each
> row in your memory **starts at a multiple of the unpack alignment**, which
> is 1, 2, 4 or 8, and **4 by default**. For RGBA8 data, every row is
> `width × 4` bytes, already a multiple of 4, so the default never causes
> trouble. For RGB8 data with an odd width, it does:
>
> ```
> width 3, RGB8: 9 bytes of pixels per row
>   GL_UNPACK_ALIGNMENT = 4  →  GL expects rows 12 bytes apart
>   stb_image gives rows 9 bytes apart  →  each row starts 3 bytes early
> ```
>
> The result is the classic "diagonally sheared" texture, or a
> `GL_INVALID_OPERATION` because GL wants to read past the end of the buffer.
> The fix is to tell GL the truth: `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)`
> for tightly packed data. `GL_PACK_ALIGNMENT` is the same setting for data
> GL writes to you (`glGetTexImage`, `glReadPixels`).
>
> In arda every copy takes an explicit `rowAlignment`, and the GL backend
> sets the matching `glPixelStorei` right before each copy, so the GL state
> left behind by an earlier call never matters. OpenGlobe defaulted to 4
> because .NET bitmaps pad their rows to 4 bytes (`BitmapData.Stride`).
> stb_image never pads, so `Image` uploads use 1.

```cpp
// include/arda/renderer/textures/TextureUtility.h
#pragma once

#include <arda/renderer/textures/ImageFormat.h>

#include <cstddef>

// TextureUtility.cs. A C# static class becomes a namespace of free functions.
namespace arda::renderer::TextureUtility {

bool IsPowerOfTwo(unsigned value);

// 1, 2, 4 and 8 are the only row alignments GL accepts.
bool IsValidRowAlignment(int rowAlignment);

int NumberOfChannels(ImageFormat format);

// Bytes per channel, or per pixel for packed datatypes.
int SizeInBytes(ImageDatatype datatype);

// True if one value of this datatype holds a whole pixel (UnsignedShort565, UnsignedInt248, ...).
bool IsPacked(ImageDatatype datatype);

std::size_t BytesPerPixel(ImageFormat format, ImageDatatype datatype);

// Size of width x height pixels, with each row padded to a multiple of rowAlignment bytes.
// Throws std::invalid_argument for a negative size or an invalid rowAlignment.
std::size_t RequiredSizeInBytes(int width, int height, ImageFormat format, ImageDatatype datatype, int rowAlignment);

} // namespace arda::renderer::TextureUtility
```

```cpp
// src/textures/TextureUtility.cpp
#include <arda/renderer/textures/TextureUtility.h>

#include <stdexcept>

namespace arda::renderer::TextureUtility {

bool IsPowerOfTwo(unsigned value) {
    // A power of two has exactly one bit set. value - 1 clears that bit and
    // sets every bit below it, so the AND is zero only for powers of two.
    return value != 0 && (value & (value - 1)) == 0;
}

bool IsValidRowAlignment(int rowAlignment) {
    return rowAlignment == 1 || rowAlignment == 2 || rowAlignment == 4 || rowAlignment == 8;
}

int NumberOfChannels(ImageFormat format) {
    switch (format) {
    case ImageFormat::StencilIndex:             return 1;
    case ImageFormat::DepthComponent:           return 1;
    case ImageFormat::Red:                      return 1;
    case ImageFormat::Green:                    return 1;
    case ImageFormat::Blue:                     return 1;
    case ImageFormat::RedGreenBlue:             return 3;
    case ImageFormat::RedGreenBlueAlpha:        return 4;
    case ImageFormat::BlueGreenRed:             return 3;
    case ImageFormat::BlueGreenRedAlpha:        return 4;
    case ImageFormat::RedGreen:                 return 2;
    case ImageFormat::RedGreenInteger:          return 2;
    case ImageFormat::DepthStencil:             return 2;
    case ImageFormat::RedInteger:               return 1;
    case ImageFormat::GreenInteger:             return 1;
    case ImageFormat::BlueInteger:              return 1;
    case ImageFormat::RedGreenBlueInteger:      return 3;
    case ImageFormat::RedGreenBlueAlphaInteger: return 4;
    case ImageFormat::BlueGreenRedInteger:      return 3;
    case ImageFormat::BlueGreenRedAlphaInteger: return 4;
    }
    throw std::invalid_argument("Invalid ImageFormat");
}

int SizeInBytes(ImageDatatype datatype) {
    switch (datatype) {
    case ImageDatatype::Byte:                          return 1;
    case ImageDatatype::UnsignedByte:                  return 1;
    case ImageDatatype::Short:                         return 2;
    case ImageDatatype::UnsignedShort:                 return 2;
    case ImageDatatype::Int:                           return 4;
    case ImageDatatype::UnsignedInt:                   return 4;
    case ImageDatatype::Float:                         return 4;
    case ImageDatatype::HalfFloat:                     return 2;
    case ImageDatatype::UnsignedByte332:               return 1;
    case ImageDatatype::UnsignedShort4444:             return 2;
    case ImageDatatype::UnsignedShort5551:             return 2;
    case ImageDatatype::UnsignedInt8888:               return 4;
    case ImageDatatype::UnsignedInt1010102:            return 4;
    case ImageDatatype::UnsignedByte233Reversed:       return 1;
    case ImageDatatype::UnsignedShort565:              return 2;
    case ImageDatatype::UnsignedShort565Reversed:      return 2;
    case ImageDatatype::UnsignedShort4444Reversed:     return 2;
    case ImageDatatype::UnsignedShort1555Reversed:     return 2;
    case ImageDatatype::UnsignedInt8888Reversed:       return 4;
    case ImageDatatype::UnsignedInt2101010Reversed:    return 4;
    case ImageDatatype::UnsignedInt248:                return 4;
    case ImageDatatype::UnsignedInt10F11F11FReversed:  return 4;
    case ImageDatatype::UnsignedInt5999Reversed:       return 4;
    case ImageDatatype::Float32UnsignedInt248Reversed: return 4;   // per channel: depth (4) + stencil word (4)
    }
    throw std::invalid_argument("Invalid ImageDatatype");
}

bool IsPacked(ImageDatatype datatype) {
    switch (datatype) {
    case ImageDatatype::UnsignedByte332:
    case ImageDatatype::UnsignedShort4444:
    case ImageDatatype::UnsignedShort5551:
    case ImageDatatype::UnsignedInt8888:
    case ImageDatatype::UnsignedInt1010102:
    case ImageDatatype::UnsignedByte233Reversed:
    case ImageDatatype::UnsignedShort565:
    case ImageDatatype::UnsignedShort565Reversed:
    case ImageDatatype::UnsignedShort4444Reversed:
    case ImageDatatype::UnsignedShort1555Reversed:
    case ImageDatatype::UnsignedInt8888Reversed:
    case ImageDatatype::UnsignedInt2101010Reversed:
    case ImageDatatype::UnsignedInt248:
    case ImageDatatype::UnsignedInt10F11F11FReversed:
    case ImageDatatype::UnsignedInt5999Reversed:
        return true;

    // Float32UnsignedInt248Reversed is 8 bytes per pixel, which is exactly
    // 2 channels x 4 bytes, so it works with the unpacked formula.
    case ImageDatatype::Float32UnsignedInt248Reversed:
    case ImageDatatype::Byte:
    case ImageDatatype::UnsignedByte:
    case ImageDatatype::Short:
    case ImageDatatype::UnsignedShort:
    case ImageDatatype::Int:
    case ImageDatatype::UnsignedInt:
    case ImageDatatype::Float:
    case ImageDatatype::HalfFloat:
        return false;
    }
    throw std::invalid_argument("Invalid ImageDatatype");
}

std::size_t BytesPerPixel(ImageFormat format, ImageDatatype datatype) {
    const auto datatypeSize = static_cast<std::size_t>(SizeInBytes(datatype));
    if (IsPacked(datatype)) {
        return datatypeSize;
    }
    return static_cast<std::size_t>(NumberOfChannels(format)) * datatypeSize;
}

std::size_t RequiredSizeInBytes(int width, int height, ImageFormat format, ImageDatatype datatype, int rowAlignment) {
    // Check the alignment before using it: "% 0" is undefined behaviour.
    if (!IsValidRowAlignment(rowAlignment)) {
        throw std::invalid_argument("RequiredSizeInBytes: rowAlignment must be 1, 2, 4 or 8");
    }
    if (width < 0 || height < 0) {
        throw std::invalid_argument("RequiredSizeInBytes: width and height must be greater than or equal to zero");
    }

    // Convert to std::size_t before multiplying, so the product can't overflow int.
    const auto alignment = static_cast<std::size_t>(rowAlignment);
    std::size_t rowSize = static_cast<std::size_t>(width) * BytesPerPixel(format, datatype);
    const std::size_t remainder = rowSize % alignment;
    rowSize += (alignment - remainder) % alignment;   // round up to the next multiple of alignment
    return rowSize * static_cast<std::size_t>(height);
}

} // namespace arda::renderer::TextureUtility
```

Two differences from the C#, both bug fixes:

1. **Packed datatypes.** `TextureUtility.cs` always multiplies channels by
   datatype size. For `RedGreenBlueAlpha` + `UnsignedInt8888`, that gives
   4 × 4 = 16 bytes per pixel, but the real size is 4, because one 32-bit
   value holds all four channels. The C++ version asks `IsPacked` first.
   OpenGlobe never hit this because its examples only use unpacked types.
2. **Validation order.** The C# computes `RequiredSizeInBytes` before
   `VerifyRowAlignment`. With `rowAlignment == 0`, that is a
   `DivideByZeroException` in C#, but **undefined behaviour** in C++. Here the
   alignment is checked first.

The `(alignment - remainder) % alignment` expression rounds up. With a
9-byte row and alignment 4, the remainder is 1, so it adds `(4 - 1) % 4 = 3`
to make 12. With a 12-byte row, the remainder is 0, and it adds
`(4 - 0) % 4 = 0`.

> **C++ note — integer overflow and `std::size_t` arithmetic:** `int` is 32
> bits on every platform arda targets, so its maximum is 2,147,483,647. Image
> sizes get there faster than you'd expect: a 16384 × 16384 RGBA32f texture
> is 4 GiB. In C#, `int` overflow silently wraps (unless you use `checked`).
> In C++, **signed overflow is undefined behaviour**. The optimizer may assume
> it never happens, so the result can be anything. The rules this step
> follows:
>
> - **Widen before you multiply.** In
>   `static_cast<std::size_t>(width) * BytesPerPixel(...)`, the first operand
>   is already 64-bit, so the multiplication is done in 64 bits. Writing
>   `static_cast<std::size_t>(width * 4)` would overflow *inside* the cast, in
>   `int`, before widening.
> - **Check the sign before converting to unsigned.** `std::size_t` is
>   unsigned. `static_cast<std::size_t>(-1)` is 18,446,744,073,709,551,615,
>   so a negative width would turn into an enormous size. That's why
>   `RequiredSizeInBytes` rejects negative sizes first.
> - **Unsigned values wrap around at 0.** Unsigned overflow is defined
>   behaviour, but that doesn't make it correct. In `FlipRows`, `rows - 1`
>   with `rows == 0` is the largest `std::size_t`, and the loop would run off
>   the end of the vector. The `rows < 2` early return prevents it.
> - **Rearrange comparisons so they can't overflow.**
>   `xOffset + width > textureWidth` overflows when both values are large.
>   `width > textureWidth - xOffset` can't, once you know `xOffset >= 0`
>   (`Texture2D::CopyFromBuffer` does this in 4.2). The same applies to buffer
>   ranges: `offset + length > size` can wrap for huge `std::size_t` values,
>   and `length > size || offset > size - length` can't. (Step 3's
>   `detail::CheckBufferRange` uses the first form. That's fine for any size a
>   real buffer can have, but the second is the bulletproof version.)
> - **Narrow back to `int` only after checking.** C APIs such as stb and GL
>   take `int`/`GLsizei` sizes. `ReadFile` checks against
>   `std::numeric_limits<int>::max()` before `static_cast<int>`.
> - **Never divide by an unchecked value.** Division or `%` by zero is
>   undefined behaviour for integers.
> - **Float to integer conversion is undefined** if the value is NaN or out
>   of range. `Texture2D::Save` clamps before converting (4.2).

### 2.4 `include/arda/renderer/textures/Texture2DDescription.h` and `src/textures/Texture2DDescription.cpp` (Listing 3.25)

The description holds the settings that can't change after a texture is
created (3.6.1). The C# struct is immutable, with read-only properties. Here
it is a plain aggregate, like the render state structs in
[Step 1](01-state-management.md), and the `Texture2D` stores its own copy, so
changing your copy later has no effect on the texture.

```cpp
// include/arda/renderer/textures/Texture2DDescription.h
#pragma once

#include <arda/renderer/textures/TextureFormat.h>

#include <cstddef>

namespace arda::renderer {

// The settings that can't change after a texture is created (Listing 3.25).
//   Texture2DDescription{512, 512, TextureFormat::RedGreenBlueAlpha8}
//   Texture2DDescription{256, 256, TextureFormat::RedGreenBlue8, true}   // with mipmaps
struct Texture2DDescription {
    int width = 0;
    int height = 0;
    TextureFormat textureFormat = TextureFormat::RedGreenBlueAlpha8;
    bool generateMipmaps = false;

    // Where the texture can be attached in a framebuffer (Step 6).
    bool ColorRenderable() const { return !DepthRenderable() && !DepthStencilRenderable(); }
    bool DepthRenderable() const;          // Depth16, Depth24, Depth32f, Depth24Stencil8, Depth32fStencil8
    bool DepthStencilRenderable() const;   // Depth24Stencil8, Depth32fStencil8

    // width * height * bytes per texel. Approximate, because the driver may
    // pad rows or store RGB as RGBA, and mipmaps are not counted.
    std::size_t ApproximateSizeInBytes() const;

    bool operator==(const Texture2DDescription&) const = default;
};

} // namespace arda::renderer
```

`ColorRenderable` is OpenGlobe's definition: anything that isn't depth. That
includes integer and sRGB formats, which GL 3.3 can render to. It also
includes `RedGreenBlue9E5`, which it can't. The framebuffer's completeness
check in Step 6 catches that case.

```cpp
// src/textures/Texture2DDescription.cpp
#include <arda/renderer/textures/Texture2DDescription.h>

#include <stdexcept>

namespace arda::renderer {

namespace {

// Texture2DDescription.cs, SizeInBytes(TextureFormat)
std::size_t BytesPerTexel(TextureFormat format) {
    switch (format) {
    case TextureFormat::RedGreenBlue8:          return 3;
    case TextureFormat::RedGreenBlue16:         return 6;
    case TextureFormat::RedGreenBlueAlpha8:     return 4;
    case TextureFormat::RedGreenBlue10A2:       return 4;
    case TextureFormat::RedGreenBlueAlpha16:    return 8;
    case TextureFormat::Depth16:                return 2;
    case TextureFormat::Depth24:                return 3;
    case TextureFormat::Red8:                   return 1;
    case TextureFormat::Red16:                  return 2;
    case TextureFormat::RedGreen8:              return 2;
    case TextureFormat::RedGreen16:             return 4;
    case TextureFormat::Red16f:                 return 2;
    case TextureFormat::Red32f:                 return 4;
    case TextureFormat::RedGreen16f:            return 4;
    case TextureFormat::RedGreen32f:            return 8;
    case TextureFormat::Red8i:                  return 1;
    case TextureFormat::Red8ui:                 return 1;
    case TextureFormat::Red16i:                 return 2;
    case TextureFormat::Red16ui:                return 2;
    case TextureFormat::Red32i:                 return 4;
    case TextureFormat::Red32ui:                return 4;
    case TextureFormat::RedGreen8i:             return 2;
    case TextureFormat::RedGreen8ui:            return 2;
    case TextureFormat::RedGreen16i:            return 4;
    case TextureFormat::RedGreen16ui:           return 4;
    case TextureFormat::RedGreen32i:            return 8;
    case TextureFormat::RedGreen32ui:           return 8;
    case TextureFormat::RedGreenBlueAlpha32f:   return 16;
    case TextureFormat::RedGreenBlue32f:        return 12;
    case TextureFormat::RedGreenBlueAlpha16f:   return 8;
    case TextureFormat::RedGreenBlue16f:        return 6;
    case TextureFormat::Depth24Stencil8:        return 4;
    case TextureFormat::Red11fGreen11fBlue10f:  return 4;
    case TextureFormat::RedGreenBlue9E5:        return 4;
    case TextureFormat::SRedGreenBlue8:         return 3;
    case TextureFormat::SRedGreenBlue8Alpha8:   return 4;
    case TextureFormat::Depth32f:               return 4;
    case TextureFormat::Depth32fStencil8:       return 5;
    case TextureFormat::RedGreenBlueAlpha32ui:  return 16;
    case TextureFormat::RedGreenBlue32ui:       return 12;
    case TextureFormat::RedGreenBlueAlpha16ui:  return 8;
    case TextureFormat::RedGreenBlue16ui:       return 6;
    case TextureFormat::RedGreenBlueAlpha8ui:   return 4;
    case TextureFormat::RedGreenBlue8ui:        return 3;
    case TextureFormat::RedGreenBlueAlpha32i:   return 16;
    case TextureFormat::RedGreenBlue32i:        return 12;
    case TextureFormat::RedGreenBlueAlpha16i:   return 8;
    case TextureFormat::RedGreenBlue16i:        return 6;
    case TextureFormat::RedGreenBlueAlpha8i:    return 4;
    case TextureFormat::RedGreenBlue8i:         return 3;
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

} // namespace

bool Texture2DDescription::DepthRenderable() const {
    return textureFormat == TextureFormat::Depth16 ||
           textureFormat == TextureFormat::Depth24 ||
           textureFormat == TextureFormat::Depth32f ||
           textureFormat == TextureFormat::Depth24Stencil8 ||
           textureFormat == TextureFormat::Depth32fStencil8;
}

bool Texture2DDescription::DepthStencilRenderable() const {
    return textureFormat == TextureFormat::Depth24Stencil8 ||
           textureFormat == TextureFormat::Depth32fStencil8;
}

std::size_t Texture2DDescription::ApproximateSizeInBytes() const {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * BytesPerTexel(textureFormat);
}

} // namespace arda::renderer
```

The C# struct also overrides `GetHashCode` and `ToString`. The defaulted
`operator==` covers what arda needs: comparing descriptions in tests and,
later, in caches. Defaulted comparisons are explained in
[Step 1](01-state-management.md).

**Checkpoint:** add `TextureUtility.cpp` and `Texture2DDescription.cpp` to
`arda_renderer`, then add the "TextureUtility ..." and
"Texture2DDescription ..." test cases from
[Tests](#tests-testssrcrenderertexturetestscpp). They need no GPU. Build and
run `arda_tests`.

---

## Part 3: Pixel buffers

A **pixel buffer** is a GPU buffer, the same kind of object as a vertex
buffer, used as a staging area for image data. OpenGlobe never uploads a
texture straight from system memory. Every upload goes system memory →
`WritePixelBuffer` → `Texture2D`, and every download goes `Texture2D` →
`ReadPixelBuffer` → system memory (3.6.1).

> **OpenGL note — pixel buffer objects (PBOs):** a PBO is an ordinary
> buffer object bound to one of two special targets.
> - **`GL_PIXEL_UNPACK_BUFFER`:** while a buffer is bound here, the `data`
>   pointer passed to `glTexImage2D` and `glTexSubImage2D` is no longer a
>   pointer. GL treats it as a **byte offset into the buffer**. That's why
>   `Texture2DGL3x` passes `nullptr`: it means "offset 0".
> - **`GL_PIXEL_PACK_BUFFER`:** the same for commands that write pixels out,
>   `glGetTexImage` and `glReadPixels`. The data goes into the buffer instead
>   of your memory.
>
> Why bother? Without a PBO, `glTexSubImage2D` must finish reading your
> memory before it returns, because you might free it right after. With a
> PBO, the data already belongs to GL, so the driver can start a DMA transfer
> and return immediately. Readback benefits even more: `glGetTexImage` into a
> PBO queues the copy, and the CPU only waits when you actually read the
> buffer (`glGetBufferSubData` here), so you can do other work in between.
> Chapter 10's multithreaded resource loading builds on this: a worker
> thread with its own context can fill a pixel buffer and copy it into a
> texture without stalling the rendering thread.

> **OpenGL note — the pitfall of leaving a PBO bound:** because a bound
> unpack buffer silently changes what the `data` argument means, forgetting
> to unbind one breaks code far away. A later
> `glTexImage2D(..., pointerToPixels)` from any code, even a library, would
> read from `bufferStart + (size_t)pointerToPixels`, which usually produces a
> `GL_INVALID_OPERATION` ("buffer too small"), or garbage. Even allocating
> with `nullptr` would read from offset 0 of the bound buffer instead of
> leaving the texture uninitialized. A bound pack buffer breaks
> `glReadPixels` into client memory in the same way.
>
> The rule in arda: **every function that binds a pixel buffer unbinds it
> before returning**, and `Texture2DGL3x`'s constructor unbinds the unpack
> target before allocating, as OpenGlobe does.

> **Why:** OpenGlobe routes all texture data through pixel buffers, even
> though a single synchronous upload isn't faster this way.
> `glBufferSubData` into the PBO is itself a copy. The reasons are about the
> design, not speed:
> 1. **One upload path.** `CopyFromBuffer` is the only way data enters a
>    texture, so validation, row alignment and mipmap generation are written
>    once.
> 2. **Asynchronous transfers later.** Chapter 10's multithreaded texture
>    loading needs exactly this separation between filling and copying.
> 3. **It maps onto D3D11.** D3D11 has no PBOs, but a `WritePixelBuffer` can
>    simply be a `std::vector<std::byte>` that `UpdateSubresource` reads
>    (see the D3D11 check at the end).

> **Why two classes:** `WritePixelBuffer` and `ReadPixelBuffer` have the same
> interface, but they are separate types, not one class with a direction flag.
> `Texture2D::CopyFromBuffer` takes a `const WritePixelBuffer&`, so passing a
> read buffer is a **compile** error instead of a runtime check. The GL
> versions also need different targets and usage hints (`*_DRAW` for data the
> application writes, `*_READ` for data GL writes and the application reads).

### 3.1 `include/arda/renderer/buffers/PixelBufferHint.h`

```cpp
#pragma once

namespace arda::renderer {

// How often the application will change the buffer's contents (PixelBufferHint.cs).
// The GL backend turns this into a *_DRAW hint for write buffers and a *_READ
// hint for read buffers.
enum class PixelBufferHint {
    Stream,    // written once, used a few times (the usual case for uploads)
    Static,    // written once, used many times
    Dynamic,   // written many times, used many times
};

} // namespace arda::renderer
```

### 3.2 `include/arda/renderer/buffers/WritePixelBuffer.h`

The typed templates are the same as `VertexBuffer`'s in
[Step 3](03-vertex-data.md): a non-virtual template turns any contiguous range
of trivially copyable values into `std::span<const std::byte>` and calls a
virtual byte-level function. The `BufferSource` concept and
`detail::CheckBufferRange` are reused from `VertexBuffer.h`. Concepts,
`std::span` and `std::as_bytes` are explained in Step 3.

```cpp
#pragma once

#include <arda/renderer/buffers/PixelBufferHint.h>
#include <arda/renderer/buffers/VertexBuffer.h>   // BufferSource, detail::CheckBufferRange
#include <arda/renderer/textures/Image.h>

#include <cstddef>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

namespace arda::renderer {

// Moves data from system memory to a texture (WritePixelBuffer.cs).
// Created with Device::CreateWritePixelBuffer; read by Texture2D::CopyFromBuffer.
class WritePixelBuffer {
public:
    virtual ~WritePixelBuffer() = default;

    WritePixelBuffer(const WritePixelBuffer&)            = delete;
    WritePixelBuffer& operator=(const WritePixelBuffer&) = delete;

    // e.g. buffer->CopyFromSystemMemory(std::vector<float>{...});
    template <BufferSource R>
    void CopyFromSystemMemory(const R& values, std::size_t destinationOffsetInBytes = 0) {
        const auto bytes = std::as_bytes(std::span(std::ranges::data(values), std::ranges::size(values)));
        detail::CheckBufferRange(destinationOffsetInBytes, bytes.size(), SizeInBytes());
        CopyFromSystemMemoryBytes(bytes, destinationOffsetInBytes);
    }

    // Replaces CopyFromBitmap. The Image is already bottom-row-first, so no flip is needed.
    void CopyFromImage(const Image& image) { CopyFromSystemMemory(image.pixels); }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::vector<T> CopyToSystemMemory(std::size_t offsetInBytes, std::size_t lengthInBytes) const {
        detail::CheckBufferRange(offsetInBytes, lengthInBytes, SizeInBytes());
        std::vector<T> values(lengthInBytes / sizeof(T));
        CopyToSystemMemoryBytes(std::as_writable_bytes(std::span(values)), offsetInBytes);
        return values;
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::vector<T> CopyToSystemMemory() const {
        return CopyToSystemMemory<T>(0, SizeInBytes());
    }

    virtual std::size_t SizeInBytes() const = 0;
    virtual PixelBufferHint UsageHint() const = 0;

protected:
    WritePixelBuffer() = default;

    virtual void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) = 0;
    virtual void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const = 0;
};

} // namespace arda::renderer
```

`CopyFromImage` works because `std::vector<std::uint8_t>` satisfies
`BufferSource`: it is contiguous and sized, and `std::uint8_t` is trivially
copyable. OpenGlobe also has `CopyToBitmap` on write buffers and
`CopyFromBitmap` on read buffers. They aren't needed, so they are left out.

### 3.3 `include/arda/renderer/buffers/ReadPixelBuffer.h`

The same interface, plus `CopyToImage`, which replaces `CopyToBitmap`. A read
buffer holds whatever `glGetTexImage` wrote, so its rows are padded to the
`rowAlignment` passed to `CopyToBuffer`. `CopyToImage` takes the same
alignment and removes the padding.

```cpp
#pragma once

#include <arda/renderer/buffers/PixelBufferHint.h>
#include <arda/renderer/buffers/VertexBuffer.h>   // BufferSource, detail::CheckBufferRange
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/TextureUtility.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace arda::renderer {

// Moves data from a texture to system memory (ReadPixelBuffer.cs).
// Returned by Texture2D::CopyToBuffer.
class ReadPixelBuffer {
public:
    virtual ~ReadPixelBuffer() = default;

    ReadPixelBuffer(const ReadPixelBuffer&)            = delete;
    ReadPixelBuffer& operator=(const ReadPixelBuffer&) = delete;

    template <BufferSource R>
    void CopyFromSystemMemory(const R& values, std::size_t destinationOffsetInBytes = 0) {
        const auto bytes = std::as_bytes(std::span(std::ranges::data(values), std::ranges::size(values)));
        detail::CheckBufferRange(destinationOffsetInBytes, bytes.size(), SizeInBytes());
        CopyFromSystemMemoryBytes(bytes, destinationOffsetInBytes);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::vector<T> CopyToSystemMemory(std::size_t offsetInBytes, std::size_t lengthInBytes) const {
        detail::CheckBufferRange(offsetInBytes, lengthInBytes, SizeInBytes());
        std::vector<T> values(lengthInBytes / sizeof(T));
        CopyToSystemMemoryBytes(std::as_writable_bytes(std::span(values)), offsetInBytes);
        return values;
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::vector<T> CopyToSystemMemory() const {
        return CopyToSystemMemory<T>(0, SizeInBytes());
    }

    // Replaces CopyToBitmap. The buffer must hold 8-bit pixels with `channels`
    // channels, rows padded to rowAlignment (the value passed to CopyToBuffer).
    // The Image comes back bottom-row-first, like every Image.
    Image CopyToImage(int width, int height, int channels, int rowAlignment = 1) const {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("CopyToImage: width and height must be greater than zero");
        }
        if (channels < 1 || channels > 4) {
            throw std::invalid_argument("CopyToImage: channels must be 1, 2, 3 or 4");
        }
        if (!TextureUtility::IsValidRowAlignment(rowAlignment)) {
            throw std::invalid_argument("CopyToImage: rowAlignment must be 1, 2, 4 or 8");
        }

        Image image;
        image.width = width;
        image.height = height;
        image.channels = channels;

        const std::size_t tightRowSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
        const auto alignment = static_cast<std::size_t>(rowAlignment);
        const std::size_t paddedRowSize = (tightRowSize + alignment - 1) / alignment * alignment;
        const std::size_t requiredSize = paddedRowSize * static_cast<std::size_t>(height);
        if (requiredSize > SizeInBytes()) {
            throw std::invalid_argument("CopyToImage: the pixel buffer is smaller than width x height x channels");
        }

        const std::vector<std::uint8_t> bytes = CopyToSystemMemory<std::uint8_t>(0, requiredSize);
        image.pixels.resize(image.SizeInBytes());
        for (std::size_t row = 0; row < static_cast<std::size_t>(height); ++row) {
            const auto source = bytes.begin() + static_cast<std::ptrdiff_t>(row * paddedRowSize);
            std::copy(source, source + static_cast<std::ptrdiff_t>(tightRowSize),
                      image.pixels.begin() + static_cast<std::ptrdiff_t>(row * tightRowSize));
        }
        return image;
    }

    virtual std::size_t SizeInBytes() const = 0;
    virtual PixelBufferHint UsageHint() const = 0;

protected:
    ReadPixelBuffer() = default;

    virtual void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) = 0;
    virtual void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const = 0;
};

} // namespace arda::renderer
```

`(tightRowSize + alignment - 1) / alignment * alignment` is another way to
round up to a multiple, with the same result as the remainder form in
`RequiredSizeInBytes`. Integer division truncates, so adding
`alignment - 1` first turns the truncation into rounding up. The iterator
offsets are cast to `std::ptrdiff_t` because iterator arithmetic takes a
*signed* difference type. Without the cast, `-Wsign-conversion` or MSVC's
`/W4` would warn.

### 3.4 GL: `src/gl/buffers/WritePixelBufferGL3x.h` and `ReadPixelBufferGL3x.h`

`PixelBufferGL3x.cs` becomes the `BufferGL3x` helper you wrote in
[Step 3](03-vertex-data.md). It already does `glBufferData` in its
constructor and `glBufferSubData`/`glGetBufferSubData` for copies, for any
target. The two classes only add the target, the hint table and the
unbinding. Everything is inline, so there are no `.cpp` files.

```cpp
// src/gl/buffers/WritePixelBufferGL3x.h
#pragma once

#include "gl/buffers/BufferGL3x.h"

#include <arda/renderer/buffers/WritePixelBuffer.h>

#include <stdexcept>

namespace arda::renderer::gl {

class WritePixelBufferGL3x final : public WritePixelBuffer {
public:
    WritePixelBufferGL3x(PixelBufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(GL_PIXEL_UNPACK_BUFFER, ToBufferHint(usageHint), sizeInBytes), m_usageHint(usageHint) {
        Unbind();   // BufferGL3x's constructor left the buffer bound
    }

    // Texture2DGL3x binds the buffer around glTexSubImage2D, then unbinds it.
    void Bind() const { m_buffer.Bind(); }
    static void Unbind() { glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); }

    std::size_t SizeInBytes() const override { return m_buffer.SizeInBytes(); }
    PixelBufferHint UsageHint() const override { return m_usageHint; }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        m_buffer.CopyFromSystemMemory(bytes, destinationOffsetInBytes);
        Unbind();   // a bound unpack buffer would change the meaning of the next glTexImage2D
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        m_buffer.CopyToSystemMemory(bytes, offsetInBytes);
        Unbind();
    }

private:
    // WritePixelBufferGL3x.cs _bufferHints: the application writes, GL reads.
    static BufferHint ToBufferHint(PixelBufferHint hint) {
        switch (hint) {
        case PixelBufferHint::Stream:  return BufferHint::StreamDraw;
        case PixelBufferHint::Static:  return BufferHint::StaticDraw;
        case PixelBufferHint::Dynamic: return BufferHint::DynamicDraw;
        }
        throw std::invalid_argument("Invalid PixelBufferHint");
    }

    BufferGL3x m_buffer;
    PixelBufferHint m_usageHint;
};

} // namespace arda::renderer::gl
```

```cpp
// src/gl/buffers/ReadPixelBufferGL3x.h
#pragma once

#include "gl/buffers/BufferGL3x.h"

#include <arda/renderer/buffers/ReadPixelBuffer.h>

#include <stdexcept>

namespace arda::renderer::gl {

class ReadPixelBufferGL3x final : public ReadPixelBuffer {
public:
    ReadPixelBufferGL3x(PixelBufferHint usageHint, std::size_t sizeInBytes)
        : m_buffer(GL_PIXEL_PACK_BUFFER, ToBufferHint(usageHint), sizeInBytes), m_usageHint(usageHint) {
        Unbind();   // BufferGL3x's constructor left the buffer bound
    }

    // Texture2DGL3x binds the buffer around glGetTexImage, then unbinds it.
    void Bind() const { m_buffer.Bind(); }
    static void Unbind() { glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); }

    std::size_t SizeInBytes() const override { return m_buffer.SizeInBytes(); }
    PixelBufferHint UsageHint() const override { return m_usageHint; }

protected:
    void CopyFromSystemMemoryBytes(std::span<const std::byte> bytes, std::size_t destinationOffsetInBytes) override {
        m_buffer.CopyFromSystemMemory(bytes, destinationOffsetInBytes);
        Unbind();   // a bound pack buffer would change the meaning of the next glReadPixels
    }

    void CopyToSystemMemoryBytes(std::span<std::byte> bytes, std::size_t offsetInBytes) const override {
        m_buffer.CopyToSystemMemory(bytes, offsetInBytes);
        Unbind();
    }

private:
    // ReadPixelBufferGL3x.cs _bufferHints: GL writes, the application reads.
    static BufferHint ToBufferHint(PixelBufferHint hint) {
        switch (hint) {
        case PixelBufferHint::Stream:  return BufferHint::StreamRead;
        case PixelBufferHint::Static:  return BufferHint::StaticRead;
        case PixelBufferHint::Dynamic: return BufferHint::DynamicRead;
        }
        throw std::invalid_argument("Invalid PixelBufferHint");
    }

    BufferGL3x m_buffer;
    PixelBufferHint m_usageHint;
};

} // namespace arda::renderer::gl
```

The C# `_bufferHints` arrays are indexed by `(int)usageHint`, which silently
depends on the enum's order. The `switch` expresses the same table without
that dependency.

### 3.5 `Device` and `DeviceGL3x`: `CreateWritePixelBuffer`

OpenGlobe has `Device.CreateWritePixelBuffer` but no
`CreateReadPixelBuffer`: read buffers are only ever created by
`Texture2D.CopyToBuffer`. arda does the same.

Add to `include/arda/renderer/Device.h`, with the other includes:

```cpp
#include <arda/renderer/buffers/PixelBufferHint.h>
#include <arda/renderer/buffers/WritePixelBuffer.h>
```

and to `class Device`, next to `CreateVertexBuffer` and its `Do` function:

```cpp
public:
    // Device.cs CreateWritePixelBuffer. Throws std::invalid_argument if sizeInBytes is 0.
    std::shared_ptr<WritePixelBuffer> CreateWritePixelBuffer(PixelBufferHint usageHint, std::size_t sizeInBytes);

protected:
    virtual std::shared_ptr<WritePixelBuffer> DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                                       std::size_t sizeInBytes) = 0;
```

In `src/Device.cpp`:

```cpp
std::shared_ptr<WritePixelBuffer> Device::CreateWritePixelBuffer(PixelBufferHint usageHint, std::size_t sizeInBytes) {
    // PixelBufferGL3x.cs checks this in its constructor; checking here covers every backend.
    if (sizeInBytes == 0) {
        throw std::invalid_argument("CreateWritePixelBuffer: sizeInBytes must be greater than zero");
    }
    return DoCreateWritePixelBuffer(usageHint, sizeInBytes);
}
```

This is the non-virtual interface pattern from [Step 0](00-setup.md): the
public function validates, then the `Do` function does the backend work.

In `src/gl/DeviceGL3x.h`, add to the `protected:` overrides:

```cpp
    std::shared_ptr<WritePixelBuffer> DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                               std::size_t sizeInBytes) override;
```

In `src/gl/DeviceGL3x.cpp`, include `"gl/buffers/WritePixelBufferGL3x.h"` and
add:

```cpp
std::shared_ptr<WritePixelBuffer> DeviceGL3x::DoCreateWritePixelBuffer(PixelBufferHint usageHint,
                                                                       std::size_t sizeInBytes) {
    return std::make_shared<WritePixelBufferGL3x>(usageHint, sizeInBytes);
}
```

A pixel buffer is a buffer object, and buffer objects are shared between
contexts (3.2), so it doesn't matter which context is current.

**Checkpoint:** build, add the "WritePixelBuffer copies ..." test case, and
run `arda_tests`. The bytes make a round trip through a real GL buffer.

---

## Part 4: `Texture2D`

With formats and pixel buffers in place, the texture itself is mostly
validation (API-agnostic) plus a handful of GL calls (backend).

> **OpenGL note — texture objects:** a texture object is GPU storage for one
> or more images (mip levels), plus some parameters. The life cycle has four
> steps:
> 1. **`glGenTextures`** reserves a name (a `GLuint`). `CreateTextureName()`
>    from Step 0 wraps it in a `TextureName` RAII handle.
> 2. **The first `glBindTexture(GL_TEXTURE_2D, name)`** creates the object and
>    fixes its type forever: a 2D texture can never be rebound as a cube map.
> 3. **`glTexImage2D(..., data = nullptr)`** allocates storage for one level
>    with an internal format and size. The contents are undefined until you
>    write them. Calling it again reallocates, which is why OpenGlobe calls it
>    once, in the constructor, and uses `glTexSubImage2D` for every copy
>    afterwards.
> 4. **`glDeleteTextures`**, called by `TextureName`'s destructor.
>
> Texture objects are shared between contexts, like buffers and programs
> (3.2). *Bindings* are not: each context has its own.

> **OpenGL note — texture units and `glActiveTexture`:** a GL context has a
> fixed number of **texture units** (`GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`,
> at least 48 in GL 3.3, and often 160 or 192 on desktop GPUs). Each unit has
> one binding point per texture target (`GL_TEXTURE_2D`,
> `GL_TEXTURE_RECTANGLE`, ...) and one sampler binding. Shaders read textures
> through units, not through texture names.
>
> `glBindTexture` has no "unit" argument. It binds to the **active** unit,
> which is a hidden selector you set with `glActiveTexture(GL_TEXTURE0 + i)`:
>
> ```cpp
> glActiveTexture(GL_TEXTURE0 + 3);           // select unit 3
> glBindTexture(GL_TEXTURE_2D, texture);      // unit 3's 2D binding = texture
> ```
>
> Pitfall: `glActiveTexture` takes the enum `GL_TEXTURE0 + i`, but
> `glBindSampler` (Part 5) and `sampler2D` uniforms (Part 7) take the plain
> index `i`. Mixing them up is a classic bug that fails silently.
>
> The GL 3.3 API is **bind-to-edit**: to upload into a texture, you must bind
> it to some unit first. (GL 4.5's direct state access, `glTextureSubImage2D`,
> removes this requirement, but that isn't available in 3.3.) So every upload
> changes a unit binding as a side effect, whether you want it to or not.

> **Why the last texture unit is reserved for uploads:** `ContextGL3x` knows
> what each unit should hold (the `TextureUnits` from Part 6), and it only
> re-binds units that changed. If a texture upload could bind to *any* unit,
> the context's view of that unit would silently go stale, and the next draw
> would sample the wrong texture. OpenGlobe's solution
> (`Texture2DGL3x.BindToLastTextureUnit`) is to do every bind-to-edit on the
> **last** unit, `GL_TEXTURE0 + numberOfTextureUnits - 1`. Only that unit can
> go stale, and `CleanTextureUnits` re-binds it before every draw
> (`TextureUnitGL3x.CleanLastTextureUnit`).
>
> Rejected alternatives:
> - Query the old binding with `glGetIntegerv(GL_TEXTURE_BINDING_2D)` and
>   restore it after the upload. Queries force the CPU to wait for the
>   driver, and they double the state traffic.
> - Mark whatever unit happened to be active as dirty. The texture would need
>   to know the context's active unit, which couples the two classes.
> - Direct state access, which needs GL 4.5.
>
> The last unit can still be used by applications. It costs one re-bind per
> draw, which is negligible.

### 4.1 `include/arda/renderer/textures/Texture2D.h` (Listing 3.27)

`Texture2D` follows the non-virtual interface pattern from
[Step 0](00-setup.md): the public `CopyFromBuffer` and `CopyToBuffer`
validate, then call the protected virtual `Do` functions. The C# `abstract`
`CopyFromBuffer` validated inside `Texture2DGL3x`, so every backend would
have repeated the checks. Here they are written once.

The C# has three `CopyFromBuffer` overloads and two `CopyToBuffer` overloads,
where the shorter ones pass a row alignment of 4. A default argument
(`int rowAlignment = 4`) does the same with one fewer overload.

```cpp
#pragma once

#include <arda/renderer/buffers/ReadPixelBuffer.h>
#include <arda/renderer/buffers/WritePixelBuffer.h>
#include <arda/renderer/textures/ImageFormat.h>
#include <arda/renderer/textures/Texture2DDescription.h>

#include <filesystem>
#include <memory>

namespace arda::renderer {

// A 2D texture (Texture2D.cs, Listing 3.27). Created with Device::CreateTexture2D.
// Data goes in through a WritePixelBuffer and comes out through a ReadPixelBuffer.
class Texture2D {
public:
    virtual ~Texture2D() = default;

    Texture2D(const Texture2D&)            = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    const Texture2DDescription& Description() const { return m_description; }

    // Copies the whole texture from a pixel buffer.
    // rowAlignment is the row padding of the data in the buffer: 1 for
    // tightly packed rows (stb_image output), 4 for OpenGlobe's default.
    void CopyFromBuffer(const WritePixelBuffer& pixelBuffer, ImageFormat format, ImageDatatype datatype,
                        int rowAlignment = 4) {
        CopyFromBuffer(pixelBuffer, 0, 0, m_description.width, m_description.height, format, datatype, rowAlignment);
    }

    // Copies into the rectangle (xOffset, yOffset, width, height) of level 0.
    // (0, 0) is the bottom-left texel. The data starts at the buffer's first byte.
    // Throws std::invalid_argument or std::out_of_range, then calls DoCopyFromBuffer.
    // Regenerates the mipmaps if the description asks for them.
    void CopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset, int width, int height,
                        ImageFormat format, ImageDatatype datatype, int rowAlignment);

    // Copies level 0 of the whole texture into a new pixel buffer, with rows
    // padded to rowAlignment.
    std::shared_ptr<ReadPixelBuffer> CopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                  int rowAlignment = 4) const;

    // Debugging aid (Texture2D.cs Save): writes a PNG.
    //   RedGreenBlue8, RedGreenBlueAlpha8 and their sRGB versions: the colors.
    //   Depth16, Depth24, Depth32f and Red32f: grayscale, stretched from the
    //   smallest value (black) to the largest (white).
    // Throws std::invalid_argument for other formats.
    void Save(const std::filesystem::path& path) const;

protected:
    // Throws std::invalid_argument unless width and height are > 0 and, if
    // generateMipmaps is set, both are powers of two.
    explicit Texture2D(const Texture2DDescription& description);

    virtual void DoCopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset, int width, int height,
                                  ImageFormat format, ImageDatatype datatype, int rowAlignment) = 0;
    virtual std::shared_ptr<ReadPixelBuffer> DoCopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                            int rowAlignment) const = 0;

private:
    Texture2DDescription m_description;
};

} // namespace arda::renderer
```

`Description()` is a non-virtual function on the base class instead of the
C# abstract property. Every backend stores the same thing, so it lives in the
base, which is pattern 2 in the README.

The public overload that forwards to the full version is defined inline in
the class, so it is `inline` automatically. Calling one overload from another
is the C++ equivalent of C#'s `this.CopyFromBuffer(...)` chaining.

### 4.2 `src/textures/Texture2D.cpp`

The validation comes from `Texture2DGL3x.cs` (constructor, `CopyFromBuffer`,
`CopyToBuffer`, `VerifyRowAlignment`). The exception types follow the C#:
`ArgumentOutOfRangeException` becomes `std::out_of_range`, and
`ArgumentException` becomes `std::invalid_argument`. `Save` is
`Texture2D.cs`'s `Save`, `SaveColor` and `SaveFloat`.

```cpp
#include <arda/renderer/textures/Texture2D.h>
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/TextureUtility.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace arda::renderer {

namespace {

void VerifyRowAlignment(int rowAlignment) {
    if (!TextureUtility::IsValidRowAlignment(rowAlignment)) {
        throw std::invalid_argument("rowAlignment must be 1, 2, 4 or 8");
    }
}

// Texture2D.cs SaveColor. OpenGlobe reads BGR with 4-byte rows to match a
// .NET Bitmap; an Image wants tightly packed RGB or RGBA.
void SaveColor(const Texture2D& texture, const std::filesystem::path& path, ImageFormat format, int channels) {
    const Texture2DDescription& description = texture.Description();
    const auto pixelBuffer = texture.CopyToBuffer(format, ImageDatatype::UnsignedByte, 1);
    pixelBuffer->CopyToImage(description.width, description.height, channels, 1).SavePng(path);
}

// Texture2D.cs SaveFloat: maps [smallest, largest] to [0, 255] gray.
void SaveFloat(const Texture2D& texture, const std::filesystem::path& path, ImageFormat format) {
    const Texture2DDescription& description = texture.Description();
    const std::vector<float> values =
        texture.CopyToBuffer(format, ImageDatatype::Float, 1)->CopyToSystemMemory<float>();

    const auto [smallest, largest] = std::ranges::minmax_element(values);
    const float minimum = *smallest;
    const float delta = *largest - minimum;
    const float oneOverDelta = delta > 0.0f ? 1.0f / delta : 1.0f;   // all values equal: avoid dividing by zero

    Image image;
    image.width = description.width;
    image.height = description.height;
    image.channels = 1;   // OpenGlobe writes the gray value into R, G and B; one channel is enough
    image.pixels.reserve(values.size());
    for (const float value : values) {
        float intensity = (value - minimum) * oneOverDelta;
        // Clamp before converting: float to integer is undefined behaviour for NaN
        // or out-of-range values. "intensity >= 0" is false for NaN, so NaN becomes 0.
        intensity = intensity >= 0.0f ? std::min(intensity, 1.0f) : 0.0f;
        image.pixels.push_back(static_cast<std::uint8_t>(intensity * 255.0f));
    }
    image.SavePng(path);   // GL's rows are bottom-first, like every Image
}

} // namespace

Texture2D::Texture2D(const Texture2DDescription& description) : m_description(description) {
    if (description.width <= 0 || description.height <= 0) {
        throw std::invalid_argument("Texture2D: description.width and description.height must be greater than zero");
    }
    if (description.generateMipmaps &&
        (!TextureUtility::IsPowerOfTwo(static_cast<unsigned>(description.width)) ||
         !TextureUtility::IsPowerOfTwo(static_cast<unsigned>(description.height)))) {
        throw std::invalid_argument("Texture2D: when generateMipmaps is true, width and height must be powers of two");
    }
}

void Texture2D::CopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset, int width, int height,
                               ImageFormat format, ImageDatatype datatype, int rowAlignment) {
    // First, because RequiredSizeInBytes below divides by it.
    VerifyRowAlignment(rowAlignment);

    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("CopyFromBuffer: width and height must be greater than zero");
    }
    if (xOffset < 0) {
        throw std::out_of_range("CopyFromBuffer: xOffset must be greater than or equal to zero");
    }
    if (yOffset < 0) {
        throw std::out_of_range("CopyFromBuffer: yOffset must be greater than or equal to zero");
    }
    // "xOffset + width > Description().width" could overflow int; this form can't,
    // because both sides of the subtraction are known to be >= 0.
    if (width > m_description.width - xOffset) {
        throw std::out_of_range("CopyFromBuffer: xOffset + width must be less than or equal to Description().width");
    }
    if (height > m_description.height - yOffset) {
        throw std::out_of_range("CopyFromBuffer: yOffset + height must be less than or equal to Description().height");
    }
    if (pixelBuffer.SizeInBytes() < TextureUtility::RequiredSizeInBytes(width, height, format, datatype, rowAlignment)) {
        throw std::invalid_argument(
            "CopyFromBuffer: the pixel buffer is not big enough for the width, height, format, datatype and row alignment");
    }

    DoCopyFromBuffer(pixelBuffer, xOffset, yOffset, width, height, format, datatype, rowAlignment);
}

std::shared_ptr<ReadPixelBuffer> Texture2D::CopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                         int rowAlignment) const {
    if (format == ImageFormat::StencilIndex) {
        throw std::invalid_argument("CopyToBuffer: StencilIndex is not supported. Try DepthStencil instead.");
    }
    VerifyRowAlignment(rowAlignment);
    return DoCopyToBuffer(format, datatype, rowAlignment);
}

void Texture2D::Save(const std::filesystem::path& path) const {
    // A default: is right here: only some formats are supported, on purpose.
    switch (m_description.textureFormat) {
    case TextureFormat::RedGreenBlue8:
    case TextureFormat::SRedGreenBlue8:
        SaveColor(*this, path, ImageFormat::RedGreenBlue, 3);
        return;
    case TextureFormat::RedGreenBlueAlpha8:
    case TextureFormat::SRedGreenBlue8Alpha8:
        SaveColor(*this, path, ImageFormat::RedGreenBlueAlpha, 4);
        return;
    case TextureFormat::Depth16:
    case TextureFormat::Depth24:
    case TextureFormat::Depth32f:
        SaveFloat(*this, path, ImageFormat::DepthComponent);   // Texture2D.cs SaveDepth
        return;
    case TextureFormat::Red32f:
        SaveFloat(*this, path, ImageFormat::Red);              // Texture2D.cs SaveRed
        return;
    default:
        break;
    }
    throw std::invalid_argument("Texture2D::Save is not implemented for this TextureFormat");
}

} // namespace arda::renderer
```

`SaveColor` and `SaveFloat` are free functions in an anonymous namespace,
not private members. They only use the public interface, so they don't need
to be in the header at all. Anonymous namespaces are covered in
[Step 0](00-setup.md).

The C# `Save` handles only `RedGreenBlue8` among the color formats. arda adds
RGBA8 and the sRGB variants, because Step 6 renders into RGBA8 textures and
`Save` is the easiest way to look at them.

> **C++ note — structured bindings and `std::ranges::minmax_element`:**
> `std::ranges::minmax_element(values)` finds both the smallest and the
> largest element in one pass. It returns a small struct with two iterators,
> `min` and `max`. A **structured binding** unpacks a struct (or `std::pair`,
> or `std::tuple`) into named variables:
>
> ```cpp
> const auto [smallest, largest] = std::ranges::minmax_element(values);   // two iterators
> float range = *largest - *smallest;
> ```
>
> It is like C# tuple deconstruction (`var (min, max) = ...`). The names are
> yours to choose. They bind to the struct's members in declaration order.
> The iterators are only valid while `values` exists, and `values` is never
> empty here, because a texture has at least one texel.

### 4.3 GL: `TypeConverterGL3x` additions

These are the eight conversion functions the texture code needs, ported from
`TypeConverterGL3x.cs`. They follow the pattern from
[Step 1](01-state-management.md): a `switch` with every enumerator and no
`default:`, so the compiler warns about any case you forget, and a `throw`
after the `switch` for values outside the enum.

Add to `src/gl/TypeConverterGL3x.h`, with the other includes:

```cpp
#include <arda/renderer/textures/ImageFormat.h>
#include <arda/renderer/textures/TextureFormat.h>
#include <arda/renderer/textures/TextureSampler.h>   // created in Part 5; see the note below
```

and with the other declarations:

```cpp
GLenum ToGL(ImageFormat format);                        // pixel format of data in a pixel buffer
GLenum ToGL(ImageDatatype datatype);                    // pixel type of data in a pixel buffer
GLenum ToInternalFormat(TextureFormat format);          // how the texture stores texels
GLenum ToPixelFormat(TextureFormat format);             // a legal format for allocating that storage
GLenum ToPixelType(TextureFormat format);               // a legal type for allocating that storage
GLenum ToGL(TextureMinificationFilter filter);
GLenum ToGL(TextureMagnificationFilter filter);
GLenum ToGL(TextureWrap wrap);
```

`TextureSampler.h` is written in Part 5. Either write that header now (it
only holds enums, a struct and a small class) and come back, or add the last
three declarations and their definitions when you get to Part 5.

Add to `src/gl/TypeConverterGL3x.cpp`:

```cpp
// TypeConverterGL3x.cs To(ImageFormat)
GLenum ToGL(ImageFormat format) {
    switch (format) {
    case ImageFormat::StencilIndex:             return GL_STENCIL_INDEX;
    case ImageFormat::DepthComponent:           return GL_DEPTH_COMPONENT;
    case ImageFormat::Red:                      return GL_RED;
    case ImageFormat::Green:                    return GL_GREEN;
    case ImageFormat::Blue:                     return GL_BLUE;
    case ImageFormat::RedGreenBlue:             return GL_RGB;
    case ImageFormat::RedGreenBlueAlpha:        return GL_RGBA;
    case ImageFormat::BlueGreenRed:             return GL_BGR;
    case ImageFormat::BlueGreenRedAlpha:        return GL_BGRA;
    case ImageFormat::RedGreen:                 return GL_RG;
    case ImageFormat::RedGreenInteger:          return GL_RG_INTEGER;
    case ImageFormat::DepthStencil:             return GL_DEPTH_STENCIL;
    case ImageFormat::RedInteger:               return GL_RED_INTEGER;
    case ImageFormat::GreenInteger:             return GL_GREEN_INTEGER;
    case ImageFormat::BlueInteger:              return GL_BLUE_INTEGER;
    case ImageFormat::RedGreenBlueInteger:      return GL_RGB_INTEGER;
    case ImageFormat::RedGreenBlueAlphaInteger: return GL_RGBA_INTEGER;
    case ImageFormat::BlueGreenRedInteger:      return GL_BGR_INTEGER;
    case ImageFormat::BlueGreenRedAlphaInteger: return GL_BGRA_INTEGER;
    }
    throw std::invalid_argument("Invalid ImageFormat");
}

// TypeConverterGL3x.cs To(ImageDatatype)
GLenum ToGL(ImageDatatype datatype) {
    switch (datatype) {
    case ImageDatatype::Byte:                          return GL_BYTE;
    case ImageDatatype::UnsignedByte:                  return GL_UNSIGNED_BYTE;
    case ImageDatatype::Short:                         return GL_SHORT;
    case ImageDatatype::UnsignedShort:                 return GL_UNSIGNED_SHORT;
    case ImageDatatype::Int:                           return GL_INT;
    case ImageDatatype::UnsignedInt:                   return GL_UNSIGNED_INT;
    case ImageDatatype::Float:                         return GL_FLOAT;
    case ImageDatatype::HalfFloat:                     return GL_HALF_FLOAT;
    case ImageDatatype::UnsignedByte332:               return GL_UNSIGNED_BYTE_3_3_2;
    case ImageDatatype::UnsignedShort4444:             return GL_UNSIGNED_SHORT_4_4_4_4;
    case ImageDatatype::UnsignedShort5551:             return GL_UNSIGNED_SHORT_5_5_5_1;
    case ImageDatatype::UnsignedInt8888:               return GL_UNSIGNED_INT_8_8_8_8;
    case ImageDatatype::UnsignedInt1010102:            return GL_UNSIGNED_INT_10_10_10_2;
    case ImageDatatype::UnsignedByte233Reversed:       return GL_UNSIGNED_BYTE_2_3_3_REV;
    case ImageDatatype::UnsignedShort565:              return GL_UNSIGNED_SHORT_5_6_5;
    case ImageDatatype::UnsignedShort565Reversed:      return GL_UNSIGNED_SHORT_5_6_5_REV;
    case ImageDatatype::UnsignedShort4444Reversed:     return GL_UNSIGNED_SHORT_4_4_4_4_REV;
    case ImageDatatype::UnsignedShort1555Reversed:     return GL_UNSIGNED_SHORT_1_5_5_5_REV;
    case ImageDatatype::UnsignedInt8888Reversed:       return GL_UNSIGNED_INT_8_8_8_8_REV;
    case ImageDatatype::UnsignedInt2101010Reversed:    return GL_UNSIGNED_INT_2_10_10_10_REV;
    case ImageDatatype::UnsignedInt248:                return GL_UNSIGNED_INT_24_8;
    case ImageDatatype::UnsignedInt10F11F11FReversed:  return GL_UNSIGNED_INT_10F_11F_11F_REV;
    case ImageDatatype::UnsignedInt5999Reversed:       return GL_UNSIGNED_INT_5_9_9_9_REV;
    case ImageDatatype::Float32UnsignedInt248Reversed: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    }
    throw std::invalid_argument("Invalid ImageDatatype");
}

// TypeConverterGL3x.cs To(TextureFormat)
GLenum ToInternalFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::RedGreenBlue8:          return GL_RGB8;
    case TextureFormat::RedGreenBlue16:         return GL_RGB16;
    case TextureFormat::RedGreenBlueAlpha8:     return GL_RGBA8;
    case TextureFormat::RedGreenBlue10A2:       return GL_RGB10_A2;
    case TextureFormat::RedGreenBlueAlpha16:    return GL_RGBA16;
    case TextureFormat::Depth16:                return GL_DEPTH_COMPONENT16;
    case TextureFormat::Depth24:                return GL_DEPTH_COMPONENT24;
    case TextureFormat::Red8:                   return GL_R8;
    case TextureFormat::Red16:                  return GL_R16;
    case TextureFormat::RedGreen8:              return GL_RG8;
    case TextureFormat::RedGreen16:             return GL_RG16;
    case TextureFormat::Red16f:                 return GL_R16F;
    case TextureFormat::Red32f:                 return GL_R32F;
    case TextureFormat::RedGreen16f:            return GL_RG16F;
    case TextureFormat::RedGreen32f:            return GL_RG32F;
    case TextureFormat::Red8i:                  return GL_R8I;
    case TextureFormat::Red8ui:                 return GL_R8UI;
    case TextureFormat::Red16i:                 return GL_R16I;
    case TextureFormat::Red16ui:                return GL_R16UI;
    case TextureFormat::Red32i:                 return GL_R32I;
    case TextureFormat::Red32ui:                return GL_R32UI;
    case TextureFormat::RedGreen8i:             return GL_RG8I;
    case TextureFormat::RedGreen8ui:            return GL_RG8UI;
    case TextureFormat::RedGreen16i:            return GL_RG16I;
    case TextureFormat::RedGreen16ui:           return GL_RG16UI;
    case TextureFormat::RedGreen32i:            return GL_RG32I;
    case TextureFormat::RedGreen32ui:           return GL_RG32UI;
    case TextureFormat::RedGreenBlueAlpha32f:   return GL_RGBA32F;
    case TextureFormat::RedGreenBlue32f:        return GL_RGB32F;
    case TextureFormat::RedGreenBlueAlpha16f:   return GL_RGBA16F;
    case TextureFormat::RedGreenBlue16f:        return GL_RGB16F;
    case TextureFormat::Depth24Stencil8:        return GL_DEPTH24_STENCIL8;
    case TextureFormat::Red11fGreen11fBlue10f:  return GL_R11F_G11F_B10F;
    case TextureFormat::RedGreenBlue9E5:        return GL_RGB9_E5;
    case TextureFormat::SRedGreenBlue8:         return GL_SRGB8;
    case TextureFormat::SRedGreenBlue8Alpha8:   return GL_SRGB8_ALPHA8;
    case TextureFormat::Depth32f:               return GL_DEPTH_COMPONENT32F;
    case TextureFormat::Depth32fStencil8:       return GL_DEPTH32F_STENCIL8;
    case TextureFormat::RedGreenBlueAlpha32ui:  return GL_RGBA32UI;
    case TextureFormat::RedGreenBlue32ui:       return GL_RGB32UI;
    case TextureFormat::RedGreenBlueAlpha16ui:  return GL_RGBA16UI;
    case TextureFormat::RedGreenBlue16ui:       return GL_RGB16UI;
    case TextureFormat::RedGreenBlueAlpha8ui:   return GL_RGBA8UI;
    case TextureFormat::RedGreenBlue8ui:        return GL_RGB8UI;
    case TextureFormat::RedGreenBlueAlpha32i:   return GL_RGBA32I;
    case TextureFormat::RedGreenBlue32i:        return GL_RGB32I;
    case TextureFormat::RedGreenBlueAlpha16i:   return GL_RGBA16I;
    case TextureFormat::RedGreenBlue16i:        return GL_RGB16I;
    case TextureFormat::RedGreenBlueAlpha8i:    return GL_RGBA8I;
    case TextureFormat::RedGreenBlue8i:         return GL_RGB8I;
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

// TypeConverterGL3x.cs TextureToPixelFormat, with the sRGB cases fixed.
GLenum ToPixelFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::RedGreenBlue8:
    case TextureFormat::RedGreenBlue16:
    case TextureFormat::RedGreenBlue32f:
    case TextureFormat::RedGreenBlue16f:
    case TextureFormat::Red11fGreen11fBlue10f:
    case TextureFormat::RedGreenBlue9E5:
    case TextureFormat::SRedGreenBlue8:            // OpenGlobe: RgbInteger, which GL rejects for sRGB
        return GL_RGB;

    case TextureFormat::RedGreenBlueAlpha8:
    case TextureFormat::RedGreenBlue10A2:
    case TextureFormat::RedGreenBlueAlpha16:
    case TextureFormat::RedGreenBlueAlpha32f:
    case TextureFormat::RedGreenBlueAlpha16f:
    case TextureFormat::SRedGreenBlue8Alpha8:      // OpenGlobe: RgbaInteger, which GL rejects for sRGB
        return GL_RGBA;

    case TextureFormat::Depth16:
    case TextureFormat::Depth24:
    case TextureFormat::Depth32f:
        return GL_DEPTH_COMPONENT;

    case TextureFormat::Depth24Stencil8:
    case TextureFormat::Depth32fStencil8:
        return GL_DEPTH_STENCIL;

    case TextureFormat::Red8:
    case TextureFormat::Red16:
    case TextureFormat::Red16f:
    case TextureFormat::Red32f:
        return GL_RED;

    case TextureFormat::RedGreen8:
    case TextureFormat::RedGreen16:
    case TextureFormat::RedGreen16f:
    case TextureFormat::RedGreen32f:
        return GL_RG;

    case TextureFormat::Red8i:
    case TextureFormat::Red8ui:
    case TextureFormat::Red16i:
    case TextureFormat::Red16ui:
    case TextureFormat::Red32i:
    case TextureFormat::Red32ui:
        return GL_RED_INTEGER;

    case TextureFormat::RedGreen8i:
    case TextureFormat::RedGreen8ui:
    case TextureFormat::RedGreen16i:
    case TextureFormat::RedGreen16ui:
    case TextureFormat::RedGreen32i:
    case TextureFormat::RedGreen32ui:
        return GL_RG_INTEGER;

    case TextureFormat::RedGreenBlue32ui:
    case TextureFormat::RedGreenBlue16ui:
    case TextureFormat::RedGreenBlue8ui:
    case TextureFormat::RedGreenBlue32i:
    case TextureFormat::RedGreenBlue16i:
    case TextureFormat::RedGreenBlue8i:
        return GL_RGB_INTEGER;

    case TextureFormat::RedGreenBlueAlpha32ui:
    case TextureFormat::RedGreenBlueAlpha16ui:
    case TextureFormat::RedGreenBlueAlpha8ui:
    case TextureFormat::RedGreenBlueAlpha32i:
    case TextureFormat::RedGreenBlueAlpha16i:
    case TextureFormat::RedGreenBlueAlpha8i:
        return GL_RGBA_INTEGER;
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

// TypeConverterGL3x.cs TextureToPixelType, with the depth/stencil and sRGB cases fixed.
GLenum ToPixelType(TextureFormat format) {
    switch (format) {
    case TextureFormat::RedGreenBlue8:
    case TextureFormat::RedGreenBlueAlpha8:
    case TextureFormat::Red8:
    case TextureFormat::RedGreen8:
    case TextureFormat::Red8ui:
    case TextureFormat::RedGreen8ui:
    case TextureFormat::RedGreenBlueAlpha8ui:
    case TextureFormat::RedGreenBlue8ui:
    case TextureFormat::SRedGreenBlue8:            // OpenGlobe: Byte
    case TextureFormat::SRedGreenBlue8Alpha8:      // OpenGlobe: Byte
        return GL_UNSIGNED_BYTE;

    case TextureFormat::Red8i:
    case TextureFormat::RedGreen8i:
    case TextureFormat::RedGreenBlueAlpha8i:       // OpenGlobe: UnsignedByte
    case TextureFormat::RedGreenBlue8i:            // OpenGlobe: UnsignedByte
        return GL_BYTE;

    case TextureFormat::RedGreenBlue16:
    case TextureFormat::RedGreenBlueAlpha16:
    case TextureFormat::Red16:
    case TextureFormat::RedGreen16:
    case TextureFormat::Red16ui:
    case TextureFormat::RedGreen16ui:
    case TextureFormat::RedGreenBlueAlpha16ui:
    case TextureFormat::RedGreenBlue16ui:
    case TextureFormat::Depth16:                   // OpenGlobe: HalfFloat
        return GL_UNSIGNED_SHORT;

    case TextureFormat::Red16i:
    case TextureFormat::RedGreen16i:
    case TextureFormat::RedGreenBlueAlpha16i:      // OpenGlobe: UnsignedShort
    case TextureFormat::RedGreenBlue16i:           // OpenGlobe: UnsignedShort
        return GL_SHORT;

    case TextureFormat::Red32ui:
    case TextureFormat::RedGreen32ui:
    case TextureFormat::RedGreenBlueAlpha32ui:
    case TextureFormat::RedGreenBlue32ui:
    case TextureFormat::Depth24:                   // OpenGlobe: Float
        return GL_UNSIGNED_INT;

    case TextureFormat::Red32i:
    case TextureFormat::RedGreen32i:
    case TextureFormat::RedGreenBlueAlpha32i:      // OpenGlobe: UnsignedInt
    case TextureFormat::RedGreenBlue32i:           // OpenGlobe: UnsignedInt
        return GL_INT;

    case TextureFormat::Red16f:
    case TextureFormat::RedGreen16f:
    case TextureFormat::RedGreenBlueAlpha16f:
    case TextureFormat::RedGreenBlue16f:
        return GL_HALF_FLOAT;

    case TextureFormat::Red32f:
    case TextureFormat::RedGreen32f:
    case TextureFormat::RedGreenBlueAlpha32f:
    case TextureFormat::RedGreenBlue32f:
    case TextureFormat::Red11fGreen11fBlue10f:
    case TextureFormat::RedGreenBlue9E5:
    case TextureFormat::Depth32f:
        return GL_FLOAT;

    case TextureFormat::RedGreenBlue10A2:
        return GL_UNSIGNED_INT_10_10_10_2;

    case TextureFormat::Depth24Stencil8:
        return GL_UNSIGNED_INT_24_8;

    case TextureFormat::Depth32fStencil8:          // OpenGlobe: Float, which GL rejects with GL_DEPTH_STENCIL
        return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    }
    throw std::invalid_argument("Invalid TextureFormat");
}

// TypeConverterGL3x.cs To(TextureMinificationFilter)
GLenum ToGL(TextureMinificationFilter filter) {
    switch (filter) {
    case TextureMinificationFilter::Nearest:              return GL_NEAREST;
    case TextureMinificationFilter::Linear:               return GL_LINEAR;
    case TextureMinificationFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
    case TextureMinificationFilter::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
    case TextureMinificationFilter::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
    case TextureMinificationFilter::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    throw std::invalid_argument("Invalid TextureMinificationFilter");
}

// TypeConverterGL3x.cs To(TextureMagnificationFilter)
GLenum ToGL(TextureMagnificationFilter filter) {
    switch (filter) {
    case TextureMagnificationFilter::Nearest: return GL_NEAREST;
    case TextureMagnificationFilter::Linear:  return GL_LINEAR;
    }
    throw std::invalid_argument("Invalid TextureMagnificationFilter");
}

// TypeConverterGL3x.cs To(TextureWrap)
GLenum ToGL(TextureWrap wrap) {
    switch (wrap) {
    case TextureWrap::Clamp:          return GL_CLAMP_TO_EDGE;
    case TextureWrap::Repeat:         return GL_REPEAT;
    case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
    }
    throw std::invalid_argument("Invalid TextureWrap");
}
```

**Fixes to the C# tables.** `ToPixelFormat` and `ToPixelType` only matter
when allocating (`glTexImage2D` with no data), but GL still rejects illegal
combinations. Three C# entries are wrong, and would leave the texture with no
storage:
- `SRedGreenBlue8` and `SRedGreenBlue8Alpha8` use an integer pixel format.
  sRGB formats are normalized, so they need `GL_RGB`/`GL_RGBA` with
  `GL_UNSIGNED_BYTE`.
- `Depth32fStencil8` uses `GL_FLOAT`. `GL_DEPTH_STENCIL` requires a packed
  type, here `GL_FLOAT_32_UNSIGNED_INT_24_8_REV`.

The remaining changes (the signed integer formats with signed types, and
`GL_UNSIGNED_SHORT`/`GL_UNSIGNED_INT` for Depth16/Depth24) are legal either
way. They are changed so each type matches its format, which is less
surprising to read.

`GL_CLAMP_TO_EDGE` is the modern clamp. The legacy `GL_CLAMP` also blended
in a border color and doesn't exist in a core profile. OpenGlobe's `Clamp`
has always meant clamp-to-edge.

### 4.4 GL: `src/gl/textures/Texture2DGL3x.h` and `.cpp`

```cpp
// src/gl/textures/Texture2DGL3x.h
#pragma once

#include "gl/GLHandle.h"

#include <arda/renderer/textures/Texture2D.h>

#include <memory>

namespace arda::renderer::gl {

class Texture2DGL3x final : public Texture2D {
public:
    // target is GL_TEXTURE_2D. (OpenGlobe also supports GL_TEXTURE_RECTANGLE; that comes in Chapter 11.)
    // numberOfTextureUnits comes from DeviceLimits: the last unit is used for uploads.
    Texture2DGL3x(const Texture2DDescription& description, GLenum target, int numberOfTextureUnits);

    // Binds to the active texture unit. ContextGL3x calls this after glActiveTexture.
    void Bind() const { glBindTexture(m_target, m_name.Get()); }

    GLuint Handle() const { return m_name.Get(); }   // used by FramebufferGL3x (Step 6)
    GLenum Target() const { return m_target; }

protected:
    void DoCopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset, int width, int height,
                          ImageFormat format, ImageDatatype datatype, int rowAlignment) override;
    std::shared_ptr<ReadPixelBuffer> DoCopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                    int rowAlignment) const override;

private:
    void BindToLastTextureUnit() const;
    void GenerateMipmaps() const;

    TextureName m_name;
    GLenum m_target;
    GLenum m_lastTextureUnit;   // GL_TEXTURE0 + numberOfTextureUnits - 1
};

} // namespace arda::renderer::gl
```

`Bind` is `const` even though it changes GL state. `const` in C++ is about
the object's own state, and binding doesn't change the texture. The same
reasoning makes `CopyToBuffer` `const`: reading a texture back doesn't change
it.

> **OpenGL note — mipmaps and texture completeness:** a mipmap chain is the
> texture's image repeated at half size, quarter size, and so on down to
> 1×1. A 256×256 texture has 9 levels. When a texture is drawn smaller than
> its size on screen (**minification**), sampling level 0 skips texels, which
> makes the image shimmer and sparkle. With mipmaps, the GPU picks a level
> whose texels are about pixel-sized. `glGenerateMipmap(target)` builds every
> level below 0 by downsampling level 0. It has to run again after every
> change to level 0, which is why `DoCopyFromBuffer` calls `GenerateMipmaps`
> after each copy (as OpenGlobe does).
>
> A texture is **complete** only if every level its minification filter may
> use exists. Sampling an incomplete texture returns black (0, 0, 0, 1),
> with no error. The trap: **GL's default minification filter is
> `GL_NEAREST_MIPMAP_LINEAR`**, which uses mipmaps. A freshly created,
> non-mipmapped texture sampled with default settings is therefore
> incomplete, and it renders black. This is the most common "my texture is
> black" bug.
>
> `Texture2DGL3x` avoids it in two ways:
> - Like OpenGlobe (`ApplySampler(Device.TextureSamplers.LinearClamp)`), it
>   sets the texture's own filter to `GL_LINEAR` and its wrap mode to
>   clamp-to-edge.
> - For non-mipmapped textures, it sets `GL_TEXTURE_MAX_LEVEL` to 0, which
>   says "this texture has only level 0". Even a sampler that asks for mipmap
>   filtering (Part 5) then sees a complete texture, and samples level 0.
>   `GL_TEXTURE_MAX_LEVEL` is a texture parameter, not a sampler parameter,
>   so it has to be set here. OpenGlobe doesn't set it; it is an addition.
>
> Why power-of-two sizes for mipmaps? GL 3.3 can build mipmaps for any size,
> but OpenGlobe requires powers of two (`Texture2DGL3x.cs` constructor), and
> arda keeps that rule so the book's code and ours accept the same textures.
> Relaxing it later only means deleting the check in `Texture2D`'s
> constructor.

```cpp
// src/gl/textures/Texture2DGL3x.cpp
#include "gl/textures/Texture2DGL3x.h"
#include "gl/buffers/ReadPixelBufferGL3x.h"
#include "gl/buffers/WritePixelBufferGL3x.h"
#include "gl/TypeConverterGL3x.h"

#include <arda/renderer/textures/TextureUtility.h>

namespace arda::renderer::gl {

Texture2DGL3x::Texture2DGL3x(const Texture2DDescription& description, GLenum target, int numberOfTextureUnits)
    : Texture2D(description),   // validates the description before any GL call
      m_name(CreateTextureName()),
      m_target(target),
      m_lastTextureUnit(GL_TEXTURE0 + static_cast<GLenum>(numberOfTextureUnits - 1)) {
    // glTexImage2D is only allocating here, so no pixel buffer may be bound:
    // with one bound, the nullptr below would mean "offset 0 into that buffer".
    WritePixelBufferGL3x::Unbind();

    // Bind to edit, on the unit ContextGL3x re-binds before every draw.
    BindToLastTextureUnit();

    glTexImage2D(m_target, 0,
                 static_cast<GLint>(ToInternalFormat(description.textureFormat)),
                 description.width, description.height,
                 0,                                        // border: must be 0
                 ToPixelFormat(description.textureFormat),
                 ToPixelType(description.textureFormat),
                 nullptr);                                 // allocate only; contents are undefined

    // Default sampling state (Texture2DGL3x.cs ApplySampler(LinearClamp)).
    // A sampler object bound to the same unit overrides all four.
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Only level 0 exists, so the texture is complete whatever the sampler's filter is.
    if (!description.generateMipmaps) {
        glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, 0);
    }
}

void Texture2DGL3x::BindToLastTextureUnit() const {
    glActiveTexture(m_lastTextureUnit);
    Bind();
}

void Texture2DGL3x::DoCopyFromBuffer(const WritePixelBuffer& pixelBuffer, int xOffset, int yOffset,
                                     int width, int height, ImageFormat format, ImageDatatype datatype,
                                     int rowAlignment) {
    // A WritePixelBuffer from this device is always a WritePixelBufferGL3x (README, pattern 4).
    const auto& bufferGL = static_cast<const WritePixelBufferGL3x&>(pixelBuffer);

    bufferGL.Bind();                                   // GL_PIXEL_UNPACK_BUFFER
    BindToLastTextureUnit();
    glPixelStorei(GL_UNPACK_ALIGNMENT, rowAlignment);  // how the rows in the buffer are padded
    glTexSubImage2D(m_target, 0,
                    xOffset, yOffset, width, height,
                    ToGL(format), ToGL(datatype),
                    nullptr);                          // offset 0 into the bound unpack buffer
    WritePixelBufferGL3x::Unbind();

    GenerateMipmaps();
}

std::shared_ptr<ReadPixelBuffer> Texture2DGL3x::DoCopyToBuffer(ImageFormat format, ImageDatatype datatype,
                                                               int rowAlignment) const {
    const Texture2DDescription& description = Description();
    auto pixelBuffer = std::make_shared<ReadPixelBufferGL3x>(
        PixelBufferHint::Stream,
        TextureUtility::RequiredSizeInBytes(description.width, description.height, format, datatype, rowAlignment));

    pixelBuffer->Bind();                               // GL_PIXEL_PACK_BUFFER
    BindToLastTextureUnit();
    glPixelStorei(GL_PACK_ALIGNMENT, rowAlignment);
    glGetTexImage(m_target, 0, ToGL(format), ToGL(datatype),
                  nullptr);                            // offset 0 into the bound pack buffer
    ReadPixelBufferGL3x::Unbind();                     // OpenGlobe leaves it bound
    return pixelBuffer;
}

void Texture2DGL3x::GenerateMipmaps() const {
    if (Description().generateMipmaps) {
        // The texture is still bound to the active (last) unit from DoCopyFromBuffer.
        glGenerateMipmap(m_target);
    }
}

} // namespace arda::renderer::gl
```

The `static_cast` to `WritePixelBufferGL3x&` is a downcast without a runtime
check. It is safe because a GL device only ever creates GL pixel buffers.
Mixing objects from two devices isn't supported, as in OpenGlobe. `static_cast`
vs `dynamic_cast` is covered in [Step 0](00-setup.md) and
[Step 2](02-shaders.md).

`glGetTexImage` always reads a whole level. GL 3.3 has no "read a
sub-rectangle of a texture" call (`glGetTextureSubImage` is GL 4.5), which is
why `CopyToBuffer` has no offset arguments.

Two small differences from `Texture2DGL3x.cs`:
- `GenerateMipmaps` uses `m_target`. The C# hard-codes
  `GenerateMipmapTarget.Texture2D`, which would be wrong for a rectangle
  texture if one could have mipmaps.
- `DoCopyToBuffer` unbinds the pack buffer, following the rule from Part 3.
  The C# leaves it bound, so a later `glReadPixels` into client memory would
  write into the buffer instead.

### 4.5 `Device` and `DeviceGL3x`: `CreateTexture2D`

Add to `include/arda/renderer/Device.h`, with the other includes:

```cpp
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/Texture2D.h>
#include <arda/renderer/textures/TextureFormat.h>
```

and to `class Device`:

```cpp
public:
    // Allocates a texture with undefined contents (Device.cs CreateTexture2D(Texture2DDescription)).
    // Texture2D's constructor validates the description.
    std::shared_ptr<Texture2D> CreateTexture2D(const Texture2DDescription& description);

    // Creates a texture and copies an image into it (Device.cs CreateTexture2DFromBitmap).
    // Non-virtual: built from CreateWritePixelBuffer and CreateTexture2D, so every backend gets it.
    std::shared_ptr<Texture2D> CreateTexture2D(const Image& image, TextureFormat format, bool generateMipmaps);

protected:
    virtual std::shared_ptr<Texture2D> DoCreateTexture2D(const Texture2DDescription& description) = 0;
```

In `src/Device.cpp`:

```cpp
std::shared_ptr<Texture2D> Device::CreateTexture2D(const Texture2DDescription& description) {
    return DoCreateTexture2D(description);
}
```

There is nothing to check here. The `Texture2D` base-class constructor
validates the description, and it runs before `Texture2DGL3x`'s member
initializers, so an invalid description throws before `glGenTextures` is
called.

In `src/gl/DeviceGL3x.h`, add to the `protected:` overrides:

```cpp
    std::shared_ptr<Texture2D> DoCreateTexture2D(const Texture2DDescription& description) override;
```

In `src/gl/DeviceGL3x.cpp`, include `"gl/textures/Texture2DGL3x.h"` and add:

```cpp
std::shared_ptr<Texture2D> DeviceGL3x::DoCreateTexture2D(const Texture2DDescription& description) {
    return std::make_shared<Texture2DGL3x>(description, GL_TEXTURE_2D, Limits().numberOfTextureUnits);
}
```

Textures are shared between contexts, but the upload binds the texture to
the last unit of whichever context is current. Every context re-binds its own
last unit before drawing, so this is safe in any context.

### 4.6 `src/textures/CreateTexture2D.cpp`: `Device::CreateTexture2D(const Image&, ...)`

This is `Device.CreateTexture2DFromBitmap`. The C# asks
`TextureUtility.ImagingPixelFormatToImageFormat` what the bitmap's bytes look
like. Here the channel count decides.

```cpp
#include <arda/renderer/Device.h>

#include <stdexcept>

namespace arda::renderer {

namespace {

// What an Image's bytes look like to a pixel buffer.
// (TextureUtility.ImagingPixelFormatToImageFormat and ...ToDatatype in OpenGlobe.)
ImageFormat ImageFormatFor(int channels) {
    switch (channels) {
    case 1: return ImageFormat::Red;
    case 2: return ImageFormat::RedGreen;
    case 3: return ImageFormat::RedGreenBlue;
    case 4: return ImageFormat::RedGreenBlueAlpha;
    default: break;
    }
    throw std::invalid_argument("CreateTexture2D: an Image must have 1, 2, 3 or 4 channels");
}

} // namespace

// Device.cs CreateTexture2DFromBitmap
std::shared_ptr<Texture2D> Device::CreateTexture2D(const Image& image, TextureFormat format, bool generateMipmaps) {
    const ImageFormat imageFormat = ImageFormatFor(image.channels);
    if (image.width <= 0 || image.height <= 0) {
        throw std::invalid_argument("CreateTexture2D: the image is empty");
    }
    if (image.pixels.size() != image.SizeInBytes()) {
        throw std::invalid_argument("CreateTexture2D: image.pixels.size() must be width * height * channels");
    }

    auto pixelBuffer = CreateWritePixelBuffer(PixelBufferHint::Stream, image.pixels.size());
    pixelBuffer->CopyFromImage(image);

    auto texture = CreateTexture2D(Texture2DDescription{image.width, image.height, format, generateMipmaps});

    // stb_image rows have no padding, so the row alignment is 1.
    // (.NET bitmaps pad rows to 4 bytes, which is why OpenGlobe uses the default of 4.)
    texture->CopyFromBuffer(*pixelBuffer, imageFormat, ImageDatatype::UnsignedByte, 1);
    return texture;
}   // the pixel buffer is released here, like OpenGlobe's using block

} // namespace arda::renderer
```

The `Image`'s channel count and the texture's format are independent. GL
converts during the copy, so a 3-channel image can go into a
`RedGreenBlueAlpha8` texture (alpha becomes 1). A 1-channel image in an RGB
texture fills only red, though. To get gray, load it with
`Image::Load(path, 3)`. A 2-channel (gray + alpha) image lands in red and
green, so load it with 4 channels.

**Checkpoint:** add `Texture2D.cpp`, `CreateTexture2D.cpp` and
`gl/textures/Texture2DGL3x.cpp` to CMake, then add the Part 4 test cases
(round trip, row alignment, sub-rectangle, validation, mipmaps, `Save`). If
`TypeConverterGL3x.h` fails to compile because `TextureSampler.h` doesn't
exist yet, write 5.1 first. Run `arda_tests`. The row alignment test is the
one to study: it shows the padding GL adds when reading an RGB texture with
`rowAlignment` 4.

---

## Part 5: Samplers

A texture holds data. A **sampler** holds the rules for reading it: how to
filter when the texture is magnified or minified, and what to do with
texture coordinates outside [0, 1] (3.6.2).

> **OpenGL note — filtering:** a fragment's texture coordinate almost never
> lands exactly on a texel center, so the GPU has to decide what to return.
> - **Magnification** (a texel covers several pixels): `GL_NEAREST` returns
>   the closest texel, which looks blocky. `GL_LINEAR` blends the four
>   nearest texels (bilinear filtering), which looks smooth.
> - **Minification** (several texels fall in one pixel): the same two
>   choices, plus four mipmap variants named `GL_<within>_MIPMAP_<between>`.
>   The first word is how to sample inside a level, and the second is whether
>   to use the nearest level or blend the two nearest.
>   `GL_LINEAR_MIPMAP_LINEAR` blends two bilinear samples, which is
>   **trilinear filtering**.
> - **Anisotropic filtering:** mipmaps assume a pixel covers a square
>   footprint on the texture. On a surface seen at a grazing angle, such as
>   the globe near the horizon, the footprint is a long thin sliver. Trilinear
>   filtering then picks a level blurry enough for the long side, and the
>   whole surface smears. Anisotropic filtering takes several samples along
>   the long axis instead. It is an extension in GL 3.3,
>   `GL_EXT_texture_filter_anisotropic`, supported by every desktop driver
>   (and core since GL 4.6). The sampler parameter is the maximum number of
>   samples: 1 is off, and 16 is the usual maximum.

> **OpenGL note — wrap modes:** `GL_TEXTURE_WRAP_S` and `GL_TEXTURE_WRAP_T`
> decide what a coordinate outside [0, 1] reads. S is the horizontal
> coordinate and T the vertical one: GL's names for u and v.
> - `GL_CLAMP_TO_EDGE` repeats the edge texels.
> - `GL_REPEAT` tiles the texture (it uses the fractional part).
> - `GL_MIRRORED_REPEAT` tiles it, flipping every other copy.
>
> Pitfall: linear filtering reads neighboring texels, and with `GL_REPEAT`
> the neighbor of the right edge is the **left** edge. A non-tiling image
> drawn with a repeat sampler can show a thin seam of color from the opposite
> side along its borders. Clamp is the safe default for images that aren't
> meant to tile, which is why OpenGlobe's textures default to
> `LinearClamp`.

> **OpenGL note — sampler objects vs texture parameters:** before GL 3.3,
> filter and wrap settings were **texture parameters** (`glTexParameteri`),
> stored inside the texture object. Each texture could only be read one way
> at a time: to sample the same texture with nearest filtering in one draw
> and linear in the next, you had to change the texture itself between them.
> GL 3.3 added **sampler objects** (`glGenSamplers`, `glSamplerParameteri`).
> A sampler is bound to a texture unit with `glBindSampler(unitIndex,
> sampler)`, and while it is bound it **overrides** the parameters of
> whatever texture is on that unit. The texture keeps a few settings that
> describe the data rather than how to read it, such as
> `GL_TEXTURE_MAX_LEVEL`.

> **Why samplers are separate objects (3.6.2–3.6.3):** keeping sampling
> state out of the texture has three benefits.
> - **It separates what from how.** One texture can be read by several
>   samplers, and one sampler by many textures. A globe's color texture might
>   be sampled with anisotropic filtering when drawn, and with nearest
>   filtering by a picking pass.
> - **It is cheap to share.** Most textures use one of four combinations, so
>   `Device::Samplers()` creates those once. Binding an existing sampler is
>   one call, where changing texture parameters is up to five.
> - **It matches Direct3D.** D3D10 and D3D11 were designed with
>   `SamplerState` objects separate from textures from the start. With the
>   same split in the abstraction, the D3D11 backend has nothing to emulate.
>
> Like OpenGlobe, arda makes a sampler **immutable**: all of its settings are
> given at creation. This makes samplers safe to share between texture units
> and threads, and it matches `ID3D11SamplerState`, which can't be changed
> after creation either.

### 5.1 `include/arda/renderer/textures/TextureSampler.h` (Listing 3.28)

The C# `TextureSampler` constructor takes five arguments. A description
struct, like `D3D11_SAMPLER_DESC`, keeps `CreateTexture2DSampler` to one
parameter, lets you name only the fields you change (with designated
initializers, from [Step 1](01-state-management.md)), and makes samplers easy
to compare.

```cpp
#pragma once

#include <memory>

namespace arda::renderer {

// TextureMinificationFilter.cs. The mipmap filters are named <within a level>Mipmap<between levels>.
enum class TextureMinificationFilter {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear,
};

// TextureMagnificationFilter.cs
enum class TextureMagnificationFilter {
    Nearest,
    Linear,
};

// TextureWrap.cs. Clamp means clamp-to-edge.
enum class TextureWrap {
    Clamp,
    Repeat,
    MirroredRepeat,
};

// Everything about a sampler, fixed at creation (the TextureSampler.cs constructor arguments).
//   device->CreateTexture2DSampler({.minificationFilter = TextureMinificationFilter::LinearMipmapLinear,
//                                   .maximumAnisotropy = 16.0f});
struct TextureSamplerDescription {
    TextureMinificationFilter minificationFilter = TextureMinificationFilter::Linear;
    TextureMagnificationFilter magnificationFilter = TextureMagnificationFilter::Linear;
    TextureWrap wrapS = TextureWrap::Clamp;
    TextureWrap wrapT = TextureWrap::Clamp;
    float maximumAnisotropy = 1.0f;   // 1 = off. Values above the GPU's maximum are clamped.

    bool operator==(const TextureSamplerDescription&) const = default;
};

// How a shader reads a texture (TextureSampler.cs). Immutable once created.
// Created with Device::CreateTexture2DSampler.
class TextureSampler {
public:
    virtual ~TextureSampler() = default;

    TextureSampler(const TextureSampler&)            = delete;
    TextureSampler& operator=(const TextureSampler&) = delete;

    const TextureSamplerDescription& Description() const { return m_description; }

protected:
    explicit TextureSampler(const TextureSamplerDescription& description) : m_description(description) {}

private:
    TextureSamplerDescription m_description;
};

// The common samplers, created once per device (TextureSamplers.cs).
// Device::Samplers() returns them.
struct TextureSamplers {
    std::shared_ptr<TextureSampler> nearestClamp;
    std::shared_ptr<TextureSampler> linearClamp;
    std::shared_ptr<TextureSampler> nearestRepeat;
    std::shared_ptr<TextureSampler> linearRepeat;
};

} // namespace arda::renderer
```

The C# `TextureSampler` has one property per setting (`MinificationFilter`,
`WrapS`, ...). `Description()` returns them all at once:
`sampler->Description().wrapS`.

### 5.2 `include/arda/renderer/Exceptions.h` addition

`TextureSamplerGL3x.cs` throws OpenGlobe's `InsufficientVideoCardException`
when anisotropic filtering is requested but unsupported. Add it next to
`CouldNotCreateVideoCardResourceException` from [Step 2](02-shaders.md):

```cpp
// Thrown when the GPU or driver lacks a feature the application asked for,
// e.g. anisotropic filtering (InsufficientVideoCardException.cs).
class InsufficientVideoCardException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
```

### 5.3 GL: `src/gl/textures/TextureSamplerGL3x.h` and `.cpp`

```cpp
// src/gl/textures/TextureSamplerGL3x.h
#pragma once

#include "gl/GLHandle.h"

#include <arda/renderer/textures/TextureSampler.h>

namespace arda::renderer::gl {

class TextureSamplerGL3x final : public TextureSampler {
public:
    explicit TextureSamplerGL3x(const TextureSamplerDescription& description);

    // glBindSampler takes the unit's index i, not GL_TEXTURE0 + i.
    void Bind(int textureUnit) const { glBindSampler(static_cast<GLuint>(textureUnit), m_name.Get()); }
    static void Unbind(int textureUnit) { glBindSampler(static_cast<GLuint>(textureUnit), 0); }

    GLuint Handle() const { return m_name.Get(); }

private:
    SamplerName m_name;
};

} // namespace arda::renderer::gl
```

OpenGlobe checks `Device.Extensions.AnisotropicFiltering`. arda has no
extension table yet, so the check lives in this file: GL 3.3 lists extensions
one at a time with `glGetStringi(GL_EXTENSIONS, i)`. (The old
`glGetString(GL_EXTENSIONS)`, one long string, was removed from core
profiles and returns an error there.)

```cpp
// src/gl/textures/TextureSamplerGL3x.cpp
#include "gl/textures/TextureSamplerGL3x.h"
#include "gl/TypeConverterGL3x.h"

#include <arda/renderer/Exceptions.h>

#include <algorithm>
#include <string_view>

namespace arda::renderer::gl {

namespace {

// GL_EXT_texture_filter_anisotropic (GL_TEXTURE_MAX_ANISOTROPY in GL 4.6).
// Defined here because a GL 3.3 core glad header may not include extension
// enums. The names differ from glad's macros, so they can't clash.
constexpr GLenum kTextureMaxAnisotropy = 0x84FE;      // GL_TEXTURE_MAX_ANISOTROPY_EXT
constexpr GLenum kMaxTextureMaxAnisotropy = 0x84FF;   // GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT

bool HasExtension(std::string_view name) {
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i) {
        const auto* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        if (extension != nullptr && name == extension) {
            return true;
        }
    }
    return false;
}

// Device.Extensions.AnisotropicFiltering in OpenGlobe. Computed once, the
// first time a sampler is created (in InitializeCommon, with the device's
// context current). Every context in a process runs on the same driver.
bool AnisotropicFilteringSupported() {
    static const bool supported = HasExtension("GL_EXT_texture_filter_anisotropic") ||
                                  HasExtension("GL_ARB_texture_filter_anisotropic");
    return supported;
}

} // namespace

TextureSamplerGL3x::TextureSamplerGL3x(const TextureSamplerDescription& description)
    : TextureSampler(description), m_name(CreateSamplerName()) {
    const GLuint sampler = m_name.Get();
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(ToGL(description.minificationFilter)));
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(ToGL(description.magnificationFilter)));
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, static_cast<GLint>(ToGL(description.wrapS)));
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, static_cast<GLint>(ToGL(description.wrapT)));

    if (AnisotropicFilteringSupported()) {
        GLfloat maximumSupported = 1.0f;
        glGetFloatv(kMaxTextureMaxAnisotropy, &maximumSupported);
        glSamplerParameterf(sampler, kTextureMaxAnisotropy, std::min(description.maximumAnisotropy, maximumSupported));
    } else if (description.maximumAnisotropy != 1.0f) {
        throw InsufficientVideoCardException(
            "Anisotropic filtering is not supported. The extension GL_EXT_texture_filter_anisotropic was not found.");
    }
}

} // namespace arda::renderer::gl
```

`static const bool supported = ...;` inside a function is a
**function-local static**. It is initialized the first time control reaches
it, exactly once, and C++11 guarantees that's thread-safe. `GlslPrelude()`
in Step 2 uses the same idiom. `name == extension` compares a
`std::string_view` with a `const char*` by converting the pointer to a view,
which compares the characters, not the pointers.

Clamping to the GPU's maximum with `std::min` makes the behaviour explicit
instead of leaving it to the driver. D3D11 needs the same clamp, because a
`MaxAnisotropy` above 16 makes `CreateSamplerState` fail there.

### 5.4 `Device` and `DeviceGL3x`: samplers

Add to `include/arda/renderer/Device.h`, with the other includes:

```cpp
#include <arda/renderer/textures/TextureSampler.h>
```

and to `class Device`:

```cpp
public:
    // Device.cs CreateTexture2DSampler. Throws std::invalid_argument if
    // maximumAnisotropy < 1, and InsufficientVideoCardException if it is above 1
    // but the GPU can't filter anisotropically.
    std::shared_ptr<TextureSampler> CreateTexture2DSampler(const TextureSamplerDescription& description);

    // The four common samplers (Device.TextureSamplers), created by InitializeCommon.
    const TextureSamplers& Samplers() const { return m_samplers; }

protected:
    virtual std::shared_ptr<TextureSampler> DoCreateTexture2DSampler(const TextureSamplerDescription& description) = 0;

    // The opposite of InitializeCommon: releases the API objects the base class
    // owns (the samplers). Backends call it at the start of their destructor,
    // while the API still works.
    void ReleaseCommon();

private:
    TextureSamplers m_samplers;
```

In `src/Device.cpp`:

```cpp
std::shared_ptr<TextureSampler> Device::CreateTexture2DSampler(const TextureSamplerDescription& description) {
    // Written as !(x >= 1) rather than x < 1, so NaN is rejected too: every comparison with NaN is false.
    if (!(description.maximumAnisotropy >= 1.0f)) {
        throw std::invalid_argument("CreateTexture2DSampler: maximumAnisotropy must be at least 1");
    }
    return DoCreateTexture2DSampler(description);
}

void Device::ReleaseCommon() {
    m_samplers = {};   // assigns a value-initialized TextureSamplers: all four shared_ptrs become null
}
```

Then add the samplers at the end of `Device::InitializeCommon` (Part 7 shows
the whole function):

```cpp
    // TextureSamplers.cs
    using Min = TextureMinificationFilter;
    using Mag = TextureMagnificationFilter;
    m_samplers.nearestClamp = CreateTexture2DSampler({.minificationFilter = Min::Nearest, .magnificationFilter = Mag::Nearest,
                                                      .wrapS = TextureWrap::Clamp, .wrapT = TextureWrap::Clamp});
    m_samplers.linearClamp = CreateTexture2DSampler({.minificationFilter = Min::Linear, .magnificationFilter = Mag::Linear,
                                                     .wrapS = TextureWrap::Clamp, .wrapT = TextureWrap::Clamp});
    m_samplers.nearestRepeat = CreateTexture2DSampler({.minificationFilter = Min::Nearest, .magnificationFilter = Mag::Nearest,
                                                       .wrapS = TextureWrap::Repeat, .wrapT = TextureWrap::Repeat});
    m_samplers.linearRepeat = CreateTexture2DSampler({.minificationFilter = Min::Linear, .magnificationFilter = Mag::Linear,
                                                      .wrapS = TextureWrap::Repeat, .wrapT = TextureWrap::Repeat});
```

`InitializeCommon` runs from `DeviceGL3x`'s constructor body. By then the
object is a complete `DeviceGL3x`, so the virtual `DoCreateTexture2DSampler`
dispatches to the GL version. The same call from `Device`'s own constructor
would not dispatch to the backend, which is why `InitializeCommon` exists
([Step 4](04-automatic-uniforms.md)).

In `src/gl/DeviceGL3x.h`, add to the `protected:` overrides:

```cpp
    std::shared_ptr<TextureSampler> DoCreateTexture2DSampler(const TextureSamplerDescription& description) override;
```

In `src/gl/DeviceGL3x.cpp`, include `"gl/textures/TextureSamplerGL3x.h"`,
add the function, and **change the destructor**:

```cpp
std::shared_ptr<TextureSampler> DeviceGL3x::DoCreateTexture2DSampler(const TextureSamplerDescription& description) {
    return std::make_shared<TextureSamplerGL3x>(description);
}

DeviceGL3x::~DeviceGL3x() {
    // Device::m_samplers holds GL sampler objects. Base-class members are
    // destroyed after this destructor body, when the share context (and, after
    // m_glfw, GLFW itself) is gone. Delete them now, with a context current.
    glfwMakeContextCurrent(m_shareWindow);
    ReleaseCommon();
    glfwDestroyWindow(m_shareWindow);
}
```

> **Why `ReleaseCommon`:** C++ destroys an object from the most-derived
> class down. `~DeviceGL3x()`'s body runs first, then `DeviceGL3x`'s members
> (`m_glfw`, which may call `glfwTerminate`), and **then** `Device`'s members,
> including `m_samplers`. Without `ReleaseCommon`, the four `TextureSamplerGL3x`
> destructors would call `glDeleteSamplers` after the share window's context
> was destroyed, with no context current. At best that silently does nothing.
> At worst the driver crashes, because GLFW has already unloaded it. C# never
> hits this, because the finalizer order doesn't depend on class layout and
> OpenGlobe's samplers are static. The fix mirrors `InitializeCommon`: the
> base class offers a function that releases what it owns, and the backend
> calls it while its API is still alive. The D3D11 backend may call it too,
> but it doesn't have to: a `ComPtr` sampler keeps its `ID3D11Device` alive
> by itself.

**Checkpoint:** add `gl/textures/TextureSamplerGL3x.cpp` to CMake and the
sampler test case, then run `arda_tests`. Every test that creates a device
now also creates and destroys four samplers. A crash at exit, or a GL error
there, means the destructor change is missing.

---

## Part 6: Texture units

A texture unit pairs one texture with one sampler (Figure 3.18). The context
owns an array of units. The application fills units, and draw calls read
them.

```cpp
context.GetTextureUnits()[0].SetTexture(dayTexture);
context.GetTextureUnits()[0].SetSampler(device->Samplers().linearClamp);
context.GetTextureUnits()[1].SetTexture(nightTexture);
context.GetTextureUnits()[1].SetSampler(device->Samplers().linearClamp);
context.Draw(PrimitiveType::Triangles, drawState, sceneState);   // units 0 and 1 are bound here
```

> **Why texture units live on the `Context` and are applied at draw time
> (3.6.3):** there are three reasons.
> - **Bindings are per context.** Texture *objects* are shared between GL
>   contexts, but which texture is bound to unit 3 is state of one context
>   (3.2). The same is true in D3D11, where textures are bound to the
>   device context, not the device.
> - **Setting a unit is cheap and makes no API calls.** `SetTexture` stores a
>   `shared_ptr` and marks the unit dirty. `ContextGL3x` binds only the dirty
>   units, and only when a draw call needs them to be right. Setting a unit
>   twice before a draw costs no GL calls, and units nobody touched cost
>   nothing. This is the same lazy, cached approach the book uses for render
>   state (3.3) and vertex arrays (3.5).
> - **The state is API-agnostic.** OpenGlobe's `TextureUnitGL3x` does both the
>   bookkeeping and the GL calls, so the D3D11 backend would have to duplicate
>   the bookkeeping. Here `TextureUnits` holds the texture, sampler and dirty
>   list in the base library (README pattern 2), and each backend only turns
>   the dirty list into API calls: `glBindTexture` in GL,
>   `PSSetShaderResources` in D3D11.

### 6.1 `include/arda/renderer/textures/TextureUnits.h`

```cpp
#pragma once

#include <arda/renderer/textures/Texture2D.h>
#include <arda/renderer/textures/TextureSampler.h>

#include <memory>
#include <vector>

namespace arda::renderer {

class TextureUnits;

// One texture unit: a texture and the sampler that reads it (TextureUnit.cs, Figure 3.18).
// Owned by TextureUnits; reached with context.GetTextureUnits()[index].
class TextureUnit {
public:
    TextureUnit(const TextureUnit&)            = delete;
    TextureUnit& operator=(const TextureUnit&) = delete;

    const std::shared_ptr<Texture2D>& GetTexture() const { return m_texture; }
    const std::shared_ptr<TextureSampler>& GetSampler() const { return m_sampler; }

    // Pass nullptr to clear. The change reaches the graphics API at the next draw.
    void SetTexture(std::shared_ptr<Texture2D> texture);
    void SetSampler(std::shared_ptr<TextureSampler> sampler);

    int Index() const { return m_index; }

private:
    friend class TextureUnits;   // the only class that can create units and read m_dirty

    TextureUnit(TextureUnits& owner, int index) : m_owner(owner), m_index(index) {}
    void MarkDirty();

    TextureUnits& m_owner;
    int m_index;
    std::shared_ptr<Texture2D> m_texture;
    std::shared_ptr<TextureSampler> m_sampler;
    bool m_dirty = false;   // true while m_index is in m_owner.m_dirtyUnits
};

// A context's texture units (TextureUnits.cs). The count is
// DeviceLimits::numberOfTextureUnits.
class TextureUnits {
public:
    explicit TextureUnits(int count);

    TextureUnits(const TextureUnits&)            = delete;
    TextureUnits& operator=(const TextureUnits&) = delete;

    // Throws std::out_of_range for an index outside [0, Count()).
    TextureUnit& operator[](int index) { return *m_units.at(static_cast<std::size_t>(index)); }
    const TextureUnit& operator[](int index) const { return *m_units.at(static_cast<std::size_t>(index)); }

    int Count() const { return static_cast<int>(m_units.size()); }

    // For backends, before a draw: returns the indices of the units that changed
    // since the last call, in the order they first changed, and clears their
    // dirty flags. Throws std::logic_error, and changes nothing, if a changed unit
    // has a texture but no sampler (TextureUnitGL3x.Validate).
    std::vector<int> TakeDirtyUnits();

private:
    friend class TextureUnit;   // MarkDirty appends to m_dirtyUnits

    // unique_ptr so each unit keeps its address: TextureUnit& references handed
    // out by operator[] stay valid, and TextureUnit needs no copy or move.
    std::vector<std::unique_ptr<TextureUnit>> m_units;
    std::vector<int> m_dirtyUnits;
};

} // namespace arda::renderer
```

The two classes point at each other: a `TextureUnit` tells its owner it has
changed, and the owner reads and clears the unit's private flag. The forward
declaration `class TextureUnits;` at the top lets `TextureUnit` hold a
`TextureUnits&` before `TextureUnits` is defined, because a reference doesn't
need the full type (forward declarations are covered in
[Step 0](00-setup.md)).

`operator[]` returns a reference, which is what makes
`context.GetTextureUnits()[0].SetTexture(...)` change the real unit. C#'s
indexer returns a reference to the object because `TextureUnit` is a class.
In C++ you have to ask for a reference explicitly. If `operator[]` returned a
`TextureUnit` by value, it would modify a copy (and here it wouldn't compile,
because the copy constructor is deleted). The indices are checked, through
`std::vector::at`. A negative index becomes a huge `std::size_t` and fails
the same check.

> **C++ note — `friend` and private constructors:** `friend class
> TextureUnits;` inside `TextureUnit` gives `TextureUnits` access to
> `TextureUnit`'s private members, including its constructor. C# has no
> exact equivalent. `internal` is the closest, but it opens a member to the
> whole assembly, where `friend` opens it to exactly one class. Here it
> enforces the ownership rule: only a `TextureUnits` can create a
> `TextureUnit`, so every unit has a valid owner and index.
>
> One consequence: `std::make_unique<TextureUnit>(*this, i)` doesn't
> compile, because the constructor call happens inside the standard library,
> which is not a friend. The constructor of `TextureUnits` calls `new`
> itself and wraps the result straight away:
>
> ```cpp
> m_units.push_back(std::unique_ptr<TextureUnit>(new TextureUnit(*this, i)));
> ```
>
> This is safe: the `unique_ptr` owns the object before `push_back` can throw,
> so nothing leaks. Friendship isn't inherited or mutual, which is why
> `TextureUnits` has its own `friend class TextureUnit;` so that `MarkDirty`
> can append to `m_dirtyUnits`.

> **C++ note — `std::bitset` and `std::array`, and why the dirty units don't
> use them:** these are the usual tools for fixed-size sets.
> - `std::array<T, N>` is a fixed-size array whose size is part of its type,
>   like `T[N]` but with `.size()`, iterators, bounds-checked `.at()` and value
>   semantics. It never allocates.
> - `std::bitset<N>` packs N booleans into bits, with `set(i)`, `reset(i)`,
>   `test(i)`, `any()` and `count()`. A dirty set for 32 units takes 4 bytes.
>
> ```cpp
> std::bitset<32> dirty;
> dirty.set(3);                      // unit 3 changed
> if (dirty.any()) {
>     for (std::size_t i = 0; i < dirty.size(); ++i) {
>         if (dirty.test(i)) { /* bind unit i */ }
>     }
>     dirty.reset();                 // all clean
> }
> ```
>
> Both need **N at compile time**. The number of texture units is only known
> at run time: `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` is 48 on one GPU and 192
> on another, and D3D11 uses 16. You could pick a fixed maximum, such as
> `std::array<TextureUnit, 32>`, but then either some GPUs' units are
> unreachable or the array is mostly unused. Also, a bitset has to be
> scanned from 0 to N on every draw, even when only unit 0 changed.
>
> So `TextureUnits` uses what OpenGlobe uses (`_dirtyTextureUnits`): a
> `std::vector<int>` of changed indices, with a `bool` on each unit so that
> an index is added only once. The cost per draw is proportional to the
> number of units that changed, usually zero or one. When the size *is* fixed
> and small, as with the `DirtyFlags` inside `TextureUnitGL3x.cs`,
> `std::bitset` or a flag enum (see [Step 1](01-state-management.md)) is the
> better choice.

### 6.2 `src/textures/TextureUnits.cpp`

```cpp
#include <arda/renderer/textures/TextureUnits.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace arda::renderer {

void TextureUnit::SetTexture(std::shared_ptr<Texture2D> texture) {
    if (m_texture != texture) {
        m_texture = std::move(texture);
        MarkDirty();
    }
}

void TextureUnit::SetSampler(std::shared_ptr<TextureSampler> sampler) {
    if (m_sampler != sampler) {
        m_sampler = std::move(sampler);
        MarkDirty();
    }
}

// TextureUnitGL3x.cs: "if (_dirtyFlags == DirtyFlags.None) _observer.NotifyDirty(this);"
void TextureUnit::MarkDirty() {
    if (!m_dirty) {
        m_dirty = true;
        m_owner.m_dirtyUnits.push_back(m_index);
    }
}

TextureUnits::TextureUnits(int count) {
    if (count <= 0) {
        throw std::invalid_argument("TextureUnits: count must be greater than zero");
    }
    m_units.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        // std::make_unique can't reach TextureUnit's private constructor; this class can, as a friend.
        m_units.push_back(std::unique_ptr<TextureUnit>(new TextureUnit(*this, i)));
    }
}

std::vector<int> TextureUnits::TakeDirtyUnits() {
    // Validate first, so that a throw leaves every unit dirty and the next
    // draw tries again, instead of silently skipping the units that were valid.
    for (const int index : m_dirtyUnits) {
        const TextureUnit& unit = *m_units[static_cast<std::size_t>(index)];
        if (unit.m_texture && !unit.m_sampler) {
            throw std::logic_error("Texture unit " + std::to_string(index) +
                                   " has a texture but no sampler. Set a sampler on every texture unit that has a texture.");
        }
    }

    std::vector<int> dirty = std::exchange(m_dirtyUnits, {});
    for (const int index : dirty) {
        m_units[static_cast<std::size_t>(index)]->m_dirty = false;
    }
    return dirty;
}

} // namespace arda::renderer
```

`SetTexture` takes the `shared_ptr` by value and moves it into the member:
one copy when the caller passes an lvalue, and none when it passes a
temporary. The comparison `m_texture != texture` compares the pointers, so
setting the same texture again doesn't mark the unit dirty.
`std::exchange(m_dirtyUnits, {})` returns the old vector and leaves an empty
one behind in a single step. `std::exchange` was introduced with move
semantics in [Step 0](00-setup.md), and `shared_ptr` in
[Step 3](03-vertex-data.md).

`TextureUnitGL3x.Validate` also checks rectangle textures against their
samplers. arda has no rectangle textures yet (Chapter 11), so that check
waits until they exist.

The C# throws `InvalidOperationException` ("the object is in the wrong state
for this call"). The closest standard exception is `std::logic_error`, which
signals a bug in the calling code rather than a runtime condition.

### 6.3 `Context.h` and `Context.cpp`

Add to `include/arda/renderer/Context.h`, with the other includes:

```cpp
#include <arda/renderer/textures/TextureUnits.h>
```

To `class Context`, add the accessors in the `public:` section and the member
after `m_device`:

```cpp
public:
    // Figure 3.18. Changes are applied at the next Draw.
    TextureUnits& GetTextureUnits() { return m_textureUnits; }
    const TextureUnits& GetTextureUnits() const { return m_textureUnits; }

private:
    Device& m_device;
    TextureUnits m_textureUnits;   // one per DeviceLimits::numberOfTextureUnits
    // ...the other members (m_viewport, ...) stay as they are
```

In `src/Context.cpp`, make sure `<arda/renderer/Device.h>` is included (Step
3 already includes it), and change the constructor:

```cpp
Context::Context(Device& device)
    : m_device(device),
      m_textureUnits(device.Limits().numberOfTextureUnits) {}
```

`TextureUnits` has no default constructor, so it must be initialized in the
member initializer list. The constructor body would be too late. The count
comes from the device, which sets its limits before any window, and so any
context, can exist. `Context.h` only declares `class Device;`, which is why
this line lives in the `.cpp` file, where `Device.h` is included.

`TextureUnits` is held by value, not through a pointer. That is safe
because `Context` is neither copyable nor movable, so the `TextureUnits`
never moves, and the references between the units and their owner stay
valid.

### 6.4 GL: `ContextGL3x` additions (replaces `TextureUnitsGL3x`)

In `src/gl/ContextGL3x.h`, add to the `private:` functions:

```cpp
    void CleanTextureUnits();          // TextureUnitsGL3x.Clean
    void BindTextureUnit(int index);   // TextureUnitGL3x.Clean
```

In `src/gl/ContextGL3x.cpp`, add the includes:

```cpp
#include "gl/textures/Texture2DGL3x.h"
#include "gl/textures/TextureSamplerGL3x.h"
```

replace the `// Step 5 adds: CleanTextureUnits();` line in `ApplyBeforeDraw`,
and add the two functions:

```cpp
void ContextGL3x::ApplyBeforeDraw(const DrawState& drawState, const SceneState& sceneState) {
    ApplyRenderState(drawState.renderState);
    ApplyVertexArray(*drawState.vertexArray);
    ApplyShaderProgram(drawState, sceneState);
    CleanTextureUnits();
    // Step 6 adds: ApplyFramebuffer();
}

void ContextGL3x::CleanTextureUnits() {
    TextureUnits& units = GetTextureUnits();
    for (const int index : units.TakeDirtyUnits()) {
        BindTextureUnit(index);
    }

    // Texture2DGL3x binds textures to the last unit whenever it creates one or
    // copies into one, so GL's binding there may no longer match units[last].
    // Restore it before every draw (TextureUnitGL3x.CleanLastTextureUnit).
    BindTextureUnit(units.Count() - 1);
}

void ContextGL3x::BindTextureUnit(int index) {
    const TextureUnit& unit = GetTextureUnits()[index];
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(index));

    if (const auto& texture = unit.GetTexture()) {
        static_cast<const Texture2DGL3x&>(*texture).Bind();
    } else {
        // Clear every target a Texture2DGL3x can use.
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    }

    if (const auto& sampler = unit.GetSampler()) {
        static_cast<const TextureSamplerGL3x&>(*sampler).Bind(index);   // the index, not GL_TEXTURE0 + index
    } else {
        TextureSamplerGL3x::Unbind(index);
    }
}
```

The order in `ApplyBeforeDraw` is OpenGlobe's (`ContextGL3x.cs`, line 524).
Texture units are cleaned after the program is applied. The debug-build
`glValidateProgram` in `ApplyShaderProgram` only checks things such as two
sampler types sharing a unit, which don't depend on the bindings.

Differences from `TextureUnitGL3x.cs`:
- OpenGlobe tracks separate dirty flags for the texture and the sampler, and
  re-binds only the one that changed. arda re-binds both for a dirty unit.
  That is at most one extra `glBindSampler`, and it keeps the unit's state in
  one flag.
- `CleanLastTextureUnit` in OpenGlobe re-binds the last unit only when it
  holds a texture. When it doesn't, the last texture uploaded stays bound
  there, so a shader that reads that unit would see a random texture instead
  of nothing. arda always restores it, for three or four GL calls per draw.

`if (const auto& texture = unit.GetTexture())` declares a variable inside the
`if` condition. The `shared_ptr` converts to `bool` (true when non-null), and
`texture` is only in scope inside the `if` and its `else`.

**Checkpoint:** add `textures/TextureUnits.cpp` to CMake and the texture unit
test cases, then run `arda_tests`. The "Drawing checks texture units" test
draws with a hidden window: a texture without a sampler must throw, and with
a sampler the draw must succeed.

---

## Part 7: `og_textureN` automatic uniforms

> **OpenGL note — `sampler2D` uniforms:** in GLSL, a `sampler2D` uniform is
> an **integer that holds a texture unit index**. It is set with
> `glUniform1i(location, unitIndex)`, and it doesn't hold a texture name or
> `GL_TEXTURE0 + i`. Uniforms start at 0, so every sampler you forget to set
> reads unit 0. That is a silent bug when a shader has two samplers.
>
> GLSL 4.20 lets a shader choose its unit with
> `layout(binding = 1) uniform sampler2D night;`, but GLSL 3.30 doesn't have
> that, so the application must set every sampler after linking.
> OpenGlobe automates it with a **link automatic uniform**
> ([Step 4](04-automatic-uniforms.md)): a uniform named `og_texture3` is set
> to 3 once, when the program links, and never changes. The shader says which
> unit it reads just by the name it picks:
>
> ```glsl
> uniform sampler2D og_texture0;   // day
> uniform sampler2D og_texture1;   // night
> ```
>
> The sampler's value is uploaded with the program's other dirty uniforms at
> the first draw ([Step 2](02-shaders.md)).

### 7.1 `src/shaders/automaticuniforms/TextureUniform.h`

This is a private header, next to the Step 4 automatic uniforms.

```cpp
#pragma once

#include <arda/renderer/shaders/AutomaticUniforms.h>
#include <arda/renderer/shaders/Uniform.h>

#include <string>

namespace arda::renderer {

// og_textureN (TextureUniform.cs): a sampler uniform set once, when a program
// links, to texture unit N. A shader that declares
// "uniform sampler2D og_texture0;" reads context.GetTextureUnits()[0].
class TextureUniform final : public LinkAutomaticUniform {
public:
    explicit TextureUniform(int textureUnit)
        : m_textureUnit(textureUnit), m_name("og_texture" + std::to_string(textureUnit)) {}

    std::string Name() const override { return m_name; }

    // Sampler uniforms are Uniform<int> (Step 2). Like the other automatic
    // uniforms, this throws std::bad_cast if a shader declares og_textureN with
    // a type that isn't a sampler.
    void Set(UniformBase& uniform) const override {
        dynamic_cast<Uniform<int>&>(uniform).SetValue(m_textureUnit);
    }

private:
    int m_textureUnit;
    std::string m_name;   // built once; Name() is called for every uniform of every program
};

} // namespace arda::renderer
```

`dynamic_cast` to a reference throws `std::bad_cast` on failure instead of
returning null. That is the right behaviour for a shader that misuses a
reserved name (see [Step 2](02-shaders.md)).

### 7.2 `src/Device.cpp`: the whole `InitializeCommon`

With Part 5's samplers and this part's texture uniforms, `InitializeCommon`
becomes:

```cpp
#include "shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h"
#include "shaders/automaticuniforms/TextureUniform.h"
#include "shaders/automaticuniforms/Wgs84HeightUniform.h"

void Device::InitializeCommon() {
    // Device.cs static constructor, lines 45-84. Register only what is used so far.

    // og_texture0 ... og_texture(N-1), one per texture unit (Device.cs lines 45-48).
    for (int i = 0; i < Limits().numberOfTextureUnits; ++i) {
        m_linkAutomaticUniforms.Add(std::make_unique<TextureUniform>(i));
    }

    m_drawAutomaticUniformFactories.Add(std::make_unique<ModelViewPerspectiveMatrixUniformFactory>());
    m_drawAutomaticUniformFactories.Add(std::make_unique<Wgs84HeightUniformFactory>());

    // TextureSamplers.cs
    using Min = TextureMinificationFilter;
    using Mag = TextureMagnificationFilter;
    m_samplers.nearestClamp = CreateTexture2DSampler({.minificationFilter = Min::Nearest, .magnificationFilter = Mag::Nearest,
                                                      .wrapS = TextureWrap::Clamp, .wrapT = TextureWrap::Clamp});
    m_samplers.linearClamp = CreateTexture2DSampler({.minificationFilter = Min::Linear, .magnificationFilter = Mag::Linear,
                                                     .wrapS = TextureWrap::Clamp, .wrapT = TextureWrap::Clamp});
    m_samplers.nearestRepeat = CreateTexture2DSampler({.minificationFilter = Min::Nearest, .magnificationFilter = Mag::Nearest,
                                                       .wrapS = TextureWrap::Repeat, .wrapT = TextureWrap::Repeat});
    m_samplers.linearRepeat = CreateTexture2DSampler({.minificationFilter = Min::Linear, .magnificationFilter = Mag::Linear,
                                                      .wrapS = TextureWrap::Repeat, .wrapT = TextureWrap::Repeat});
}
```

OpenGlobe creates a temporary window to count its context's texture units
(`window.Context.TextureUnits.Count`). arda already has the count in
`DeviceLimits`, from the same `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` query.
With 192 units that is 192 entries in a `std::map`, which is looked up once
per uniform when a program links. That's negligible.

Registering `og_textureN` for every backend is harmless. In HLSL, textures
aren't uniforms, so the names never match anything, and D3D11 binds unit N to
registers `tN` and `sN` instead (README portability table).

**Checkpoint:** add the "og_textureN ..." test case and run `arda_tests`.

---

## Part 8: Milestone, a textured triangle ("Try This" in 3.8)

This is the Step 4 triangle (the book's triangle in the xz plane, seen
through the default camera) with a texture coordinate per vertex, and a
fragment shader that samples `og_texture0`. With no arguments it shows a
generated checkerboard. With a path, it shows that image:

```
./run.sh                             # checkerboard
./run.sh -- path/to/some/image.png   # a file (run.sh starts the program from the repository root)
```

`scene/src/main.cpp`:

```cpp
#include <arda/core/geometry/Mesh.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/Texture2D.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <memory>

using namespace arda::renderer;
using namespace arda::core::geometry;

namespace {

constexpr const char* kVertexShader = R"(
layout(location = og_positionVertexLocation) in vec4 position;
layout(location = og_textureCoordinateVertexLocation) in vec2 textureCoordinate;

out vec2 fsTextureCoordinate;

uniform mat4 og_modelViewPerspectiveMatrix;

void main()
{
    fsTextureCoordinate = textureCoordinate;
    gl_Position = og_modelViewPerspectiveMatrix * position;
})";

constexpr const char* kFragmentShader = R"(
in vec2 fsTextureCoordinate;

out vec3 fragmentColor;

uniform sampler2D og_texture0;   // texture unit 0; set to 0 automatically when the program links

void main()
{
    fragmentColor = texture(og_texture0, fsTextureCoordinate).rgb;
})";

// A size x size RGB checkerboard with `squares` squares per side. The
// bottom-left square is red, so you can see which way up the texture is.
Image CreateCheckerboard(int size, int squares) {
    Image image;
    image.width = size;
    image.height = size;
    image.channels = 3;
    image.pixels.resize(image.SizeInBytes());

    const int squareSize = size / squares;
    std::size_t i = 0;
    for (int y = 0; y < size; ++y) {   // y = 0 is the bottom row, as in every Image
        for (int x = 0; x < size; ++x) {
            const int column = x / squareSize;
            const int row = y / squareSize;

            std::uint8_t red = 40;     // dark square
            std::uint8_t green = 40;
            std::uint8_t blue = 40;
            if (row == 0 && column == 0) {
                red = 220;             // the bottom-left marker
                green = 30;
                blue = 30;
            } else if ((row + column) % 2 == 0) {
                red = green = blue = 230;   // light square
            }

            image.pixels[i++] = red;
            image.pixels[i++] = green;
            image.pixels[i++] = blue;
        }
    }
    return image;
}

// The book's triangle (3.8) plus a texture coordinate per vertex.
Mesh CreateTexturedTriangle() {
    Mesh mesh;

    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(0.0f, 0.0f, 0.0f);
    positions.emplace_back(1.0f, 0.0f, 0.0f);
    positions.emplace_back(0.0f, 0.0f, 1.0f);

    // The attribute names must match the vertex shader's inputs.
    auto& textureCoordinates = mesh.attributes.Add<VertexAttributeFloatVector2>("textureCoordinate", 3).Values();
    textureCoordinates.emplace_back(0.0f, 0.0f);   // the texture's bottom-left corner
    textureCoordinates.emplace_back(1.0f, 0.0f);   // bottom-right
    textureCoordinates.emplace_back(0.0f, 1.0f);   // top-left

    auto indices = std::make_unique<IndicesUnsignedShort>(3);
    indices->AddTriangle(0, 1, 2);
    mesh.indices = std::move(indices);

    return mesh;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        // Declaration order matters: the device must outlive the window, and
        // the window must outlive everything drawn in it.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(800, 600, "Step 5: Textured triangle");
        Context& context = window->GetContext();

        auto shaderProgram = device->CreateShaderProgram(kVertexShader, kFragmentShader);

        const Mesh mesh = CreateTexturedTriangle();
        DrawState drawState;
        drawState.renderState.facetCulling.enabled = false;
        drawState.renderState.depthTest.enabled = false;
        drawState.shaderProgram = shaderProgram;
        drawState.vertexArray = context.CreateVertexArray(mesh, shaderProgram->VertexAttributes(), BufferHint::StaticDraw);

        // A file named on the command line, or a generated checkerboard.
        // Loading with 3 channels turns gray and gray + alpha files into RGB as well.
        const Image image = argc > 1 ? Image::Load(argv[1], 3) : CreateCheckerboard(256, 8);
        auto texture = device->CreateTexture2D(image, TextureFormat::RedGreenBlue8, false);

        context.GetTextureUnits()[0].SetTexture(texture);
        context.GetTextureUnits()[0].SetSampler(device->Samplers().linearClamp);

        ClearState clearState;
        SceneState sceneState;
        sceneState.camera.ZoomToTarget(1.0);

        window->SetResizeHandler([&] {
            if (window->Width() == 0 || window->Height() == 0) {
                return;   // minimized
            }
            context.SetViewport({0, 0, window->Width(), window->Height()});
            sceneState.camera.aspectRatio = window->Width() / static_cast<double>(window->Height());
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

**What you should see:** the triangle from Step 4, filled with the
checkerboard. Its right angle is at the bottom left, and the red square sits
in that corner, at texture coordinate (0, 0). If the red square is at the top
instead, a row flip is missing or doubled (see Troubleshooting below). The
triangle shows only half of the image, the half below the diagonal from
(1, 0) to (0, 1).

`argv[1]` is a `char*`, and it converts to `std::filesystem::path`
implicitly. The `texture` variable could go out of scope right after
`SetTexture`: the texture unit's `shared_ptr` keeps the texture alive until
the context (inside the window) is destroyed. That happens while the
window's GL context still exists, in `~GraphicsWindowGL3x`.

Things to try:

- **Mipmaps.** Use a power-of-two image (the checkerboard is 256×256), pass
  `true` for `generateMipmaps`, and a mipmapped sampler:
  `device->CreateTexture2DSampler({.minificationFilter = TextureMinificationFilter::LinearMipmapLinear})`.
  Zoom out with `sceneState.camera.ZoomToTarget(20.0)`. The checkerboard
  turns smoothly gray instead of shimmering. Compare with `linearClamp`.
- **Anisotropic filtering.** Add `.maximumAnisotropy = 16.0f` to the
  mipmapped sampler, and look at the triangle edge-on.
- **Wrap modes.** Change the texture coordinates to (0, 0), (4, 0), (0, 4)
  and switch between `linearClamp`, `linearRepeat` and a
  `TextureWrap::MirroredRepeat` sampler.
- **Nearest filtering.** Use a small image (`CreateCheckerboard(8, 8)`) with
  `nearestClamp`, then `linearClamp`.
- **See the alignment bug.** Load an RGB image with an odd width, and
  temporarily change the `1` in `CreateTexture2D.cpp` to `4`. The image
  shears diagonally (or GL reports `GL_INVALID_OPERATION` and the texture
  stays black). Change it back.
- **Save.** Call `texture->Save("saved.png")` and open the file. It should
  match the input, the right way up.
- **Two textures.** Declare `uniform sampler2D og_texture1;`, put a second
  texture and sampler on unit 1, and blend the two in the fragment shader.
  This is how the book's night-lights example (Chapter 4) combines day and
  night images.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Texture is black | Incomplete texture: a mipmap filter on a texture without mipmaps (`GL_TEXTURE_MAX_LEVEL` 0 prevents this), or an integer texture with a linear filter. Or the sampler uniform isn't `og_textureN` and is still 0. Or the upload failed; check for `GL_INVALID_OPERATION`. |
| Texture is upside down | A row flip is missing (the image was not loaded through `Image::Load`) or done twice (for example, `1.0 - uv.y` in the shader as well). |
| Texture is sheared diagonally | `rowAlignment` doesn't match the data: RGB data with an odd width uploaded with alignment 4. |
| `std::logic_error` "has a texture but no sampler" | `SetTexture` without `SetSampler` on that unit. |
| Every sampler shows the same texture | The shader's samplers aren't named `og_texture0`, `og_texture1`, ..., so all of them are 0. |
| Link error: `stbi_*` unresolved | `STB_IMAGE_IMPLEMENTATION` is missing or comes after the `#include` in `Image.cpp`. |
| Link error: `stbi_*` already defined | `STB_IMAGE_IMPLEMENTATION` is defined in a second file or in a header. |
| `GL_INVALID_OPERATION` from `glTexImage2D` | A pixel buffer was left bound, or `ToPixelFormat`/`ToPixelType` gave an illegal combination for the internal format. |
| Crash or GL error when the program exits | `~DeviceGL3x` doesn't call `ReleaseCommon`, so the samplers are deleted after the context. |

---

## CMake

`renderer/CMakeLists.txt` now has these additions. The stb include line is
from Part 1. Headers are listed only so IDEs show them, following the
comment at the top of the file.

```cmake
add_library(arda_renderer
    # ...existing sources and headers...
    src/textures/CreateTexture2D.cpp
    src/textures/Image.cpp
    src/textures/Texture2D.cpp
    src/textures/Texture2DDescription.cpp
    src/textures/TextureUnits.cpp
    src/textures/TextureUtility.cpp
    include/arda/renderer/buffers/PixelBufferHint.h
    include/arda/renderer/buffers/ReadPixelBuffer.h
    include/arda/renderer/buffers/WritePixelBuffer.h
    include/arda/renderer/textures/Image.h
    include/arda/renderer/textures/ImageFormat.h
    include/arda/renderer/textures/Texture2D.h
    include/arda/renderer/textures/Texture2DDescription.h
    include/arda/renderer/textures/TextureFormat.h
    include/arda/renderer/textures/TextureSampler.h
    include/arda/renderer/textures/TextureUnits.h
    include/arda/renderer/textures/TextureUtility.h
    src/shaders/automaticuniforms/TextureUniform.h
)

target_include_directories(arda_renderer PUBLIC include PRIVATE src)
target_include_directories(arda_renderer SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

# ...

if(ARDA_RENDERER_GL)
    target_sources(arda_renderer PRIVATE
        # ...existing GL sources and headers...
        src/gl/textures/Texture2DGL3x.cpp
        src/gl/textures/TextureSamplerGL3x.cpp
        src/gl/buffers/ReadPixelBufferGL3x.h
        src/gl/buffers/WritePixelBufferGL3x.h
        src/gl/textures/Texture2DGL3x.h
        src/gl/textures/TextureSamplerGL3x.h
    )
    # ...
endif()
```

Root `CMakeLists.txt`: `find_package(Stb REQUIRED)` (Part 1).

`tests/CMakeLists.txt`: add `src/renderer/TextureTests.cpp` to `arda_tests`:

```cmake
add_executable(arda_tests
    src/main.cpp
    src/EllipsoidTests.cpp
    src/renderer/DeviceTests.cpp
    # ...the test files from Steps 1-4...
    src/renderer/TextureTests.cpp
)
```

The tests use only public headers (`Image`, `Texture2D`, ...), so they don't
need stb's include directory.

---

## Tests: `tests/src/renderer/TextureTests.cpp`

The first four test cases need no GPU. The rest create a device, whose
hidden context is current, and the draw test creates a hidden window. The
sections follow the parts of this guide, so you can add them as you go.

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/textures/Image.h>
#include <arda/renderer/textures/Texture2D.h>
#include <arda/renderer/textures/TextureUnits.h>
#include <arda/renderer/textures/TextureUtility.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace arda::renderer;
using namespace arda::core::geometry;

namespace {

// An image whose bytes are all different, so misplaced rows or channels show up.
Image MakeImage(int width, int height, int channels) {
    Image image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.pixels.resize(image.SizeInBytes());
    for (std::size_t i = 0; i < image.pixels.size(); ++i) {
        image.pixels[i] = static_cast<std::uint8_t>(i * 7 + 1);
    }
    return image;
}

std::filesystem::path TempFile(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

// ---- Part 1: Image --------------------------------------------------------

TEST_CASE("Image::SavePng and Image::Load round-trip pixels") {
    const Image original = MakeImage(3, 2, 4);
    const std::filesystem::path path = TempFile("arda_image_roundtrip.png");
    original.SavePng(path);

    const Image loaded = Image::Load(path);
    CHECK(loaded.width == 3);
    CHECK(loaded.height == 2);
    CHECK(loaded.channels == 4);
    CHECK(loaded.pixels == original.pixels);

    // Asking for 3 channels drops alpha.
    const Image rgb = Image::Load(path, 3);
    CHECK(rgb.channels == 3);
    REQUIRE(rgb.pixels.size() == 3 * 2 * 3);
    CHECK(rgb.pixels[0] == original.pixels[0]);
    CHECK(rgb.pixels[3] == original.pixels[4]);   // the second pixel's red

    std::filesystem::remove(path);
}

TEST_CASE("Image::Load reports missing files and bad arguments") {
    CHECK_THROWS_AS(Image::Load("this/file/does/not/exist.png"), std::runtime_error);
    CHECK_THROWS_AS(Image::Load("whatever.png", 5), std::invalid_argument);
    CHECK_THROWS_AS(Image{}.SavePng(TempFile("arda_empty.png")), std::invalid_argument);
}

// ---- Part 2: formats ------------------------------------------------------

TEST_CASE("TextureUtility computes row-padded sizes") {
    using namespace arda::renderer::TextureUtility;

    CHECK(RequiredSizeInBytes(3, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 1) == 18);
    CHECK(RequiredSizeInBytes(3, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 4) == 24);   // 9 -> 12 per row
    CHECK(RequiredSizeInBytes(3, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 8) == 32);   // 9 -> 16 per row
    CHECK(RequiredSizeInBytes(2, 2, ImageFormat::RedGreenBlueAlpha, ImageDatatype::Float, 4) == 64);

    // Packed datatypes hold a whole pixel in one value.
    CHECK(RequiredSizeInBytes(2, 2, ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedInt8888, 1) == 16);
    CHECK(RequiredSizeInBytes(2, 2, ImageFormat::DepthStencil, ImageDatatype::UnsignedInt248, 1) == 16);
    CHECK(RequiredSizeInBytes(2, 2, ImageFormat::DepthStencil, ImageDatatype::Float32UnsignedInt248Reversed, 1) == 32);

    CHECK_THROWS_AS(RequiredSizeInBytes(3, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 3),
                    std::invalid_argument);
    CHECK_THROWS_AS(RequiredSizeInBytes(3, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 0),
                    std::invalid_argument);
    CHECK_THROWS_AS(RequiredSizeInBytes(-1, 2, ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 1),
                    std::invalid_argument);

    // 65536 x 65536 RGBA32f is 64 GiB: far past int, fine in a 64-bit std::size_t.
    if constexpr (sizeof(std::size_t) >= 8) {
        CHECK(RequiredSizeInBytes(65536, 65536, ImageFormat::RedGreenBlueAlpha, ImageDatatype::Float, 4) ==
              std::size_t{65536} * 65536 * 16);
    }
}

TEST_CASE("TextureUtility helpers") {
    using namespace arda::renderer::TextureUtility;

    CHECK_FALSE(IsPowerOfTwo(0));
    CHECK(IsPowerOfTwo(1));
    CHECK(IsPowerOfTwo(2));
    CHECK_FALSE(IsPowerOfTwo(3));
    CHECK(IsPowerOfTwo(1024));
    CHECK(IsPowerOfTwo(0x80000000u));

    CHECK(NumberOfChannels(ImageFormat::BlueGreenRedAlpha) == 4);
    CHECK(NumberOfChannels(ImageFormat::DepthComponent) == 1);
    CHECK(SizeInBytes(ImageDatatype::HalfFloat) == 2);
    CHECK(IsPacked(ImageDatatype::UnsignedShort565));
    CHECK_FALSE(IsPacked(ImageDatatype::Float));
}

TEST_CASE("Texture2DDescription classifies formats") {
    CHECK(Texture2DDescription{4, 4, TextureFormat::Depth32f}.DepthRenderable());
    CHECK_FALSE(Texture2DDescription{4, 4, TextureFormat::Depth32f}.ColorRenderable());
    CHECK_FALSE(Texture2DDescription{4, 4, TextureFormat::Depth32f}.DepthStencilRenderable());
    CHECK(Texture2DDescription{4, 4, TextureFormat::Depth24Stencil8}.DepthStencilRenderable());
    CHECK(Texture2DDescription{4, 4, TextureFormat::Depth24Stencil8}.DepthRenderable());
    CHECK(Texture2DDescription{4, 4, TextureFormat::RedGreenBlueAlpha8}.ColorRenderable());

    CHECK(Texture2DDescription{4, 2, TextureFormat::RedGreenBlueAlpha32f}.ApproximateSizeInBytes() == 4 * 2 * 16);
    CHECK(Texture2DDescription{4, 4, TextureFormat::RedGreenBlue8} ==
          Texture2DDescription{4, 4, TextureFormat::RedGreenBlue8, false});
    CHECK(Texture2DDescription{4, 4, TextureFormat::RedGreenBlue8} !=
          Texture2DDescription{4, 4, TextureFormat::RedGreenBlue8, true});
}

// ---- Part 3: pixel buffers ------------------------------------------------

TEST_CASE("WritePixelBuffer copies to and from system memory") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const std::vector<std::uint8_t> bytes = {1, 2, 3, 4, 5, 6, 7, 8};

    auto buffer = device->CreateWritePixelBuffer(PixelBufferHint::Stream, bytes.size());
    buffer->CopyFromSystemMemory(bytes);

    CHECK(buffer->SizeInBytes() == 8);
    CHECK(buffer->UsageHint() == PixelBufferHint::Stream);
    CHECK(buffer->CopyToSystemMemory<std::uint8_t>() == bytes);
    CHECK(buffer->CopyToSystemMemory<std::uint8_t>(2, 3) == std::vector<std::uint8_t>{3, 4, 5});
    CHECK_THROWS_AS(buffer->CopyFromSystemMemory(bytes, 1), std::out_of_range);
    CHECK_THROWS_AS(device->CreateWritePixelBuffer(PixelBufferHint::Stream, 0), std::invalid_argument);
}

// ---- Part 4: textures -----------------------------------------------------

TEST_CASE("Texture2D round-trips pixels through pixel buffers") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    Image image;
    image.width = 2;
    image.height = 1;
    image.channels = 4;
    image.pixels = {255, 0, 0, 255,   0, 255, 0, 255};

    auto texture = device->CreateTexture2D(image, TextureFormat::RedGreenBlueAlpha8, false);
    CHECK(texture->Description().width == 2);
    CHECK(texture->Description().height == 1);
    CHECK(texture->Description().ColorRenderable());

    auto readBack = texture->CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1);
    CHECK(readBack->CopyToSystemMemory<std::uint8_t>() == image.pixels);
    CHECK(readBack->CopyToImage(2, 1, 4).pixels == image.pixels);
}

TEST_CASE("Row alignment pads the rows of RGB data") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const Image image = MakeImage(3, 2, 3);   // 9-byte rows
    auto texture = device->CreateTexture2D(image, TextureFormat::RedGreenBlue8, false);

    // Alignment 1: tightly packed, exactly like the Image.
    auto tight = texture->CopyToBuffer(ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 1);
    CHECK(tight->SizeInBytes() == 18);
    CHECK(tight->CopyToSystemMemory<std::uint8_t>() == image.pixels);

    // Alignment 4: each row starts at a multiple of 4, so row 1 starts at byte 12, not 9.
    auto padded = texture->CopyToBuffer(ImageFormat::RedGreenBlue, ImageDatatype::UnsignedByte, 4);
    CHECK(padded->SizeInBytes() == 24);
    const std::vector<std::uint8_t> bytes = padded->CopyToSystemMemory<std::uint8_t>();
    CHECK(bytes[12] == image.pixels[9]);
    CHECK(padded->CopyToImage(3, 2, 3, 4).pixels == image.pixels);

    // GL converts formats: an RGB texture read as RGBA has alpha 1.0 (255).
    auto rgba = texture->CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1);
    const std::vector<std::uint8_t> rgbaBytes = rgba->CopyToSystemMemory<std::uint8_t>();
    CHECK(rgbaBytes[0] == image.pixels[0]);
    CHECK(rgbaBytes[3] == 255);
}

TEST_CASE("CopyFromBuffer can update a sub-rectangle") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto texture = device->CreateTexture2D({4, 4, TextureFormat::Red8});

    const std::vector<std::uint8_t> zeros(16, 0);
    auto zeroBuffer = device->CreateWritePixelBuffer(PixelBufferHint::Stream, zeros.size());
    zeroBuffer->CopyFromSystemMemory(zeros);
    texture->CopyFromBuffer(*zeroBuffer, ImageFormat::Red, ImageDatatype::UnsignedByte, 1);

    const std::vector<std::uint8_t> patch = {1, 2, 3, 4};
    auto patchBuffer = device->CreateWritePixelBuffer(PixelBufferHint::Stream, patch.size());
    patchBuffer->CopyFromSystemMemory(patch);
    texture->CopyFromBuffer(*patchBuffer, 1, 2, 2, 2, ImageFormat::Red, ImageDatatype::UnsignedByte, 1);

    // Texel (x, y) is texels[y * 4 + x], with y = 0 the bottom row. The patch covers x = 1..2, y = 2..3.
    const std::vector<std::uint8_t> texels =
        texture->CopyToBuffer(ImageFormat::Red, ImageDatatype::UnsignedByte, 1)->CopyToSystemMemory<std::uint8_t>();
    CHECK(texels == std::vector<std::uint8_t>{
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 1, 2, 0,
        0, 3, 4, 0,
    });
}

TEST_CASE("Texture2D validates its description and its copies") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK_THROWS_AS(device->CreateTexture2D({0, 4, TextureFormat::RedGreenBlueAlpha8}), std::invalid_argument);
    CHECK_THROWS_AS(device->CreateTexture2D({3, 4, TextureFormat::RedGreenBlueAlpha8, true}), std::invalid_argument);
    CHECK_NOTHROW(device->CreateTexture2D({4, 8, TextureFormat::RedGreenBlueAlpha8, true}));

    auto texture = device->CreateTexture2D({4, 4, TextureFormat::RedGreenBlueAlpha8});
    auto small = device->CreateWritePixelBuffer(PixelBufferHint::Stream, 4 * 4 * 4 - 1);
    CHECK_THROWS_AS(texture->CopyFromBuffer(*small, ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1),
                    std::invalid_argument);

    auto buffer = device->CreateWritePixelBuffer(PixelBufferHint::Stream, 4 * 4 * 4);
    CHECK_THROWS_AS(texture->CopyFromBuffer(*buffer, ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 3),
                    std::invalid_argument);
    CHECK_THROWS_AS(texture->CopyFromBuffer(*buffer, -1, 0, 2, 2, ImageFormat::RedGreenBlueAlpha,
                                            ImageDatatype::UnsignedByte, 1),
                    std::out_of_range);
    CHECK_THROWS_AS(texture->CopyFromBuffer(*buffer, 3, 0, 2, 2, ImageFormat::RedGreenBlueAlpha,
                                            ImageDatatype::UnsignedByte, 1),
                    std::out_of_range);
    CHECK_THROWS_AS(texture->CopyToBuffer(ImageFormat::StencilIndex, ImageDatatype::UnsignedByte, 1),
                    std::invalid_argument);

    Image badImage = MakeImage(2, 2, 3);
    badImage.pixels.pop_back();
    CHECK_THROWS_AS(device->CreateTexture2D(badImage, TextureFormat::RedGreenBlue8, false), std::invalid_argument);
}

TEST_CASE("Mipmapped textures accept uploads") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const Image image = MakeImage(8, 4, 4);

    auto texture = device->CreateTexture2D(image, TextureFormat::RedGreenBlueAlpha8, true);
    CHECK(texture->Description().generateMipmaps);

    // CopyToBuffer reads level 0, which is unchanged by glGenerateMipmap.
    auto readBack = texture->CopyToBuffer(ImageFormat::RedGreenBlueAlpha, ImageDatatype::UnsignedByte, 1);
    CHECK(readBack->CopyToSystemMemory<std::uint8_t>() == image.pixels);
}

TEST_CASE("Texture2D::Save writes PNGs") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const std::filesystem::path path = TempFile("arda_texture_save.png");

    // Color: the PNG loads back as the same image.
    const Image image = MakeImage(4, 2, 3);
    auto color = device->CreateTexture2D(image, TextureFormat::RedGreenBlue8, false);
    color->Save(path);
    CHECK(Image::Load(path).pixels == image.pixels);

    // Depth: stretched to gray, smallest value black, largest white.
    auto depth = device->CreateTexture2D({2, 1, TextureFormat::Depth32f});
    const std::vector<float> depths = {0.25f, 0.75f};
    auto depthBuffer = device->CreateWritePixelBuffer(PixelBufferHint::Stream, depths.size() * sizeof(float));
    depthBuffer->CopyFromSystemMemory(depths);
    depth->CopyFromBuffer(*depthBuffer, ImageFormat::DepthComponent, ImageDatatype::Float, 4);
    depth->Save(path);
    const Image gray = Image::Load(path);
    CHECK(gray.channels == 1);
    CHECK(gray.pixels == std::vector<std::uint8_t>{0, 255});

    auto integer = device->CreateTexture2D({4, 4, TextureFormat::Red8ui});
    CHECK_THROWS_AS(integer->Save(path), std::invalid_argument);

    std::filesystem::remove(path);
}

// ---- Part 5: samplers -----------------------------------------------------

TEST_CASE("The built-in samplers exist, and samplers validate anisotropy") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const TextureSamplers& samplers = device->Samplers();

    REQUIRE(samplers.nearestClamp);
    REQUIRE(samplers.linearClamp);
    REQUIRE(samplers.nearestRepeat);
    REQUIRE(samplers.linearRepeat);
    CHECK(samplers.nearestClamp->Description().minificationFilter == TextureMinificationFilter::Nearest);
    CHECK(samplers.linearClamp->Description().magnificationFilter == TextureMagnificationFilter::Linear);
    CHECK(samplers.linearRepeat->Description().wrapS == TextureWrap::Repeat);

    CHECK_THROWS_AS(device->CreateTexture2DSampler({.maximumAnisotropy = 0.5f}), std::invalid_argument);

    auto mipmapped = device->CreateTexture2DSampler({.minificationFilter = TextureMinificationFilter::LinearMipmapLinear,
                                                     .wrapS = TextureWrap::MirroredRepeat});
    CHECK(mipmapped->Description().wrapS == TextureWrap::MirroredRepeat);
    CHECK(mipmapped->Description().wrapT == TextureWrap::Clamp);   // the default
}

// ---- Part 6: texture units ------------------------------------------------

TEST_CASE("TextureUnits records which units changed") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto texture = device->CreateTexture2D({4, 4, TextureFormat::RedGreenBlueAlpha8});
    auto sampler = device->Samplers().linearClamp;

    TextureUnits units(4);
    CHECK(units.Count() == 4);
    CHECK(units.TakeDirtyUnits().empty());

    units[2].SetSampler(sampler);
    units[2].SetTexture(texture);
    units[0].SetSampler(sampler);
    CHECK(units.TakeDirtyUnits() == std::vector<int>{2, 0});   // each unit once, in the order it first changed
    CHECK(units.TakeDirtyUnits().empty());

    units[2].SetTexture(texture);   // the same texture again is not a change
    CHECK(units.TakeDirtyUnits().empty());

    units[1].SetTexture(texture);   // a texture with no sampler
    CHECK_THROWS_AS(units.TakeDirtyUnits(), std::logic_error);
    units[1].SetSampler(sampler);
    CHECK(units.TakeDirtyUnits() == std::vector<int>{1});   // still dirty after the failed call

    CHECK(units[3].Index() == 3);
    CHECK_THROWS_AS((void)units[4], std::out_of_range);
    CHECK_THROWS_AS(TextureUnits(0), std::invalid_argument);
}

TEST_CASE("Drawing checks the texture units") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(16, 16, "test", WindowType::Hidden);
    Context& context = window->GetContext();
    CHECK(context.GetTextureUnits().Count() == device->Limits().numberOfTextureUnits);

    auto program = device->CreateShaderProgram(
        R"(layout(location = og_positionVertexLocation) in vec4 position;
           void main() { gl_Position = position; })",
        R"(out vec4 color;
           uniform sampler2D og_texture0;
           void main() { color = texture(og_texture0, vec2(0.5)); })");

    Mesh mesh;
    auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
    positions.emplace_back(-1.0f, -1.0f, 0.0f);
    positions.emplace_back(1.0f, -1.0f, 0.0f);
    positions.emplace_back(0.0f, 1.0f, 0.0f);

    DrawState drawState;
    drawState.shaderProgram = program;
    drawState.vertexArray = context.CreateVertexArray(mesh, program->VertexAttributes(), BufferHint::StaticDraw);
    SceneState sceneState;

    context.GetTextureUnits()[0].SetTexture(device->CreateTexture2D({4, 4, TextureFormat::RedGreenBlueAlpha8}));
    CHECK_THROWS_AS(context.Draw(PrimitiveType::Triangles, drawState, sceneState), std::logic_error);

    context.GetTextureUnits()[0].SetSampler(device->Samplers().nearestClamp);
    CHECK_NOTHROW(context.Draw(PrimitiveType::Triangles, drawState, sceneState));
}

// ---- Part 7: og_textureN --------------------------------------------------

TEST_CASE("og_textureN is set to its texture unit when a program links") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    CHECK(device->LinkAutomaticUniforms().Find("og_texture0") != nullptr);
    CHECK(device->LinkAutomaticUniforms().Find("og_texture3") != nullptr);

    // The sampler must be used, or the GLSL compiler removes it and there is no uniform to find.
    auto program = device->CreateShaderProgram(
        "in vec4 position; void main() { gl_Position = position; }",
        R"(out vec4 color;
           uniform sampler2D og_texture3;
           void main() { color = texture(og_texture3, vec2(0.5)); })");
    CHECK(program->Uniforms().Get<int>("og_texture3").Value() == 3);
}
```

A few details in these tests:
- `(void)units[4]` casts the result to `void`, which says "evaluate this for
  its side effect (the throw) and ignore the value". Without it, some
  compilers warn about an unused result inside `CHECK_THROWS_AS`.
- `if constexpr (sizeof(std::size_t) >= 8)` removes the 64 GiB check at
  compile time on a 32-bit build, where the expected value itself would
  overflow.
- The PNG round-trip test can't catch a missing row flip on its own: if both
  `Load` and `SavePng` skipped their flips, the round trip would still pass.
  That is why the milestone marks the bottom-left square in red, and why
  `Texture2D::Save` is worth trying on the render-to-texture output in
  Step 6.
- The depth test uploads with `rowAlignment` 4. Float rows are always a
  multiple of 4 bytes, so 4 and 1 mean the same thing here.

These tests need a GPU and a desktop session, like the earlier renderer
tests.

---

## Complete CMake files after Step 5

The CMake sections above list only this step's additions. Here are the three
CMake files as they should look once Step 5 is done, with every step so far
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

## D3D11 check (3.6.5)

Step 8 implements all of this. The points that shape the interfaces above:

- **Pixel buffers:** D3D11 has no PBOs, so `WritePixelBufferD3D11` is a
  `std::vector<std::byte>` in system memory. `Texture2DD3D11::DoCopyFromBuffer`
  sends it with `UpdateSubresource`, using a `D3D11_BOX` for the rectangle and
  a row pitch of `TextureUtility::RequiredSizeInBytes(width, 1, format,
  datatype, rowAlignment)`. D3D takes the row pitch explicitly, so the
  `rowAlignment` mechanism translates directly.
- **Reading textures:** `DoCopyToBuffer` copies into a `D3D11_USAGE_STAGING`
  texture, maps it, and copies the rows (`mapped.RowPitch` apart) into a
  `ReadPixelBufferD3D11`, padding each row to `rowAlignment`.
- **Texture creation:** `Texture2DD3D11` sets its bind flags from the
  description:
  - Always `D3D11_BIND_SHADER_RESOURCE`.
  - Color-renderable formats add `D3D11_BIND_RENDER_TARGET`.
  - Depth formats add `D3D11_BIND_DEPTH_STENCIL` and use a typeless format,
    such as `R32_TYPELESS`, with a `D32_FLOAT` depth view and an
    `R32_FLOAT` shader resource view.
  - `generateMipmaps` adds `D3D11_BIND_RENDER_TARGET` and
    `D3D11_RESOURCE_MISC_GENERATE_MIPS`, sets `MipLevels = 0`, and calls
    `GenerateMips` after each copy.
- **Formats:** `TypeConverterD3D11` maps each `TextureFormat` DXGI supports.
  DXGI has almost no 3-channel formats, so `RedGreenBlue8`, `16`, `16f` and
  `32f` are stored with 4 channels, and the upload pads each pixel with
  alpha = 1. The 3-channel *integer* formats throw
  `InsufficientVideoCardException`.
- **Row order:** GL and D3D both treat the first uploaded row as texture
  coordinate v = 0, so the same `Image` and the same texture coordinates look
  the same in both. Only rendering *into* a texture differs (Step 8).
- **Samplers:** `TextureSamplerD3D11` calls `CreateSamplerState`.
  `TextureSamplerDescription` maps directly to `D3D11_SAMPLER_DESC`. For
  non-mipmap minification filters, set `MaxLOD = 0` to match GL, where a
  non-mipmapped texture only has level 0.
- **Texture units:** unit N is bound with `PSSetShaderResources(N, ...)` and
  `PSSetSamplers(N, ...)`, plus the `VS` and `GS` versions, from the same
  `TakeDirtyUnits()` list. There is no last-unit workaround, because D3D11
  updates textures without binding them. In HLSL:
  `Texture2D og_texture0 : register(t0); SamplerState og_sampler0 : register(s0);`
- **No `ReleaseCommon` requirement:** a `ComPtr` sampler keeps its
  `ID3D11Device` alive, so destruction order doesn't matter there.

> **Why RGB8 becomes RGBA8 on D3D11:** DXGI has no 24-bit, 3-channel
> normalized format. GPUs fetch texels in power-of-two sizes, so every
> 8-bit-per-channel format DXGI offers has 1, 2 or 4 channels.
> (`R32G32B32_FLOAT` exists, but it has limited filtering and render target
> support.) GL accepts `GL_RGB8`, and many drivers store it as RGBA8
> internally. `Texture2DD3D11` does the same explicitly: it creates an
> `R8G8B8A8_UNORM` texture and pads the data with alpha = 255 during
> `DoCopyFromBuffer`. The alternative, removing `RedGreenBlue8` from
> `TextureFormat`, was rejected because the book's examples and Step 6's
> render-to-texture use it, and the padding is a single loop in one backend.

## Checklist

- [ ] `stb` in `vcpkg.json`, `find_package(Stb)`, the `SYSTEM PRIVATE` include path
- [ ] `Image` (`Load`, `SavePng`, `SizeInBytes`) with `STB_IMAGE_IMPLEMENTATION` in `Image.cpp` only
- [ ] `ImageFormat`, `ImageDatatype`, `TextureFormat` (complete), `TextureUtility`, `Texture2DDescription`
- [ ] `PixelBufferHint`, `WritePixelBuffer`, `ReadPixelBuffer` (with `CopyToImage`)
- [ ] `Texture2D` (validation, `Save`), `TextureSampler`, `TextureSamplers`, `TextureUnits`
- [ ] `Exceptions.h`: `InsufficientVideoCardException`
- [ ] `Device`: `CreateWritePixelBuffer`, `CreateTexture2D` (both overloads), `CreateTexture2DSampler`, `Samplers`, `ReleaseCommon`; `InitializeCommon` registers `og_textureN` and creates the samplers
- [ ] `Context::GetTextureUnits`
- [ ] GL: `WritePixelBufferGL3x`, `ReadPixelBufferGL3x`, `Texture2DGL3x`, `TextureSamplerGL3x`
- [ ] GL: `ContextGL3x::CleanTextureUnits` and `BindTextureUnit`, called from `ApplyBeforeDraw`
- [ ] GL: `DeviceGL3x` `DoCreate*` functions, and the destructor calls `ReleaseCommon`
- [ ] GL: the eight new `TypeConverterGL3x` functions
- [ ] `TextureTests` pass
- [ ] **Milestone:** the textured triangle, with the red square in the bottom-left corner, and a file loaded from the command line
