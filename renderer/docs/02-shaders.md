# Step 2: Shaders (3.4.1–3.4.4)

**Goal:** `device->CreateShaderProgram(vs, fs)` compiles and links a GLSL
program. It throws `CouldNotCreateVideoCardResourceException` with the
compile or link log when the shaders are wrong. The program reports its
vertex attributes, fragment outputs and uniforms. Setting a uniform only
records the value, and uniforms that changed are uploaded to GL just before
the next draw (Step 3 does the drawing).

**Read:** 3.4.1 "Compiling and Linking Shaders", 3.4.2 "Vertex Attributes",
3.4.3 "Fragment Outputs", 3.4.4 "Uniforms" (Listings 3.8–3.11). Automatic
uniforms (3.4.5) and the shader cache (3.4.6) are Step 4.

**Milestone:** `arda_tests` passes the new `ShaderProgramTests` and
`UniformGL3xTests`: a program compiles, its attributes, outputs and every
kind of uniform are found by reflection, errors throw with the driver's log,
and a uniform uploads the value that was set.

## OpenGlobe reference

The C# source is in your local clone under `OpenGlobe/Source/`.

| File | What to take from it |
|---|---|
| [Renderer/Shaders/ShaderProgram.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/ShaderProgram.cs) | The abstract interface. `InitializeAutomaticUniforms` and `SetDrawAutomaticUniforms` are Step 4. |
| [Renderer/GL3x/Shaders/ShaderObjectGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Shaders/ShaderObjectGL3x.cs) | The built-in constants prelude (lines 22–43), `#version` handling (54–64), compiling and the compile log |
| [Renderer/GL3x/Shaders/ShaderProgramGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Shaders/ShaderProgramGL3x.cs) | The constructor (linking), `FindVertexAttributes`, `FindUniforms`, `CreateUniform`, `CorrectUniformName`, `Bind`, `Clean`, `NotifyDirty` |
| [Renderer/Shaders/ShaderVertexAttribute.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/ShaderVertexAttribute.cs), [ShaderVertexAttributeCollection.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/ShaderVertexAttributeCollection.cs) | `ShaderVertexAttributeType` and the attribute record |
| [Renderer/Shaders/FragmentOutputs.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/FragmentOutputs.cs), [GL3x/Shaders/FragmentOutputsGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/Shaders/FragmentOutputsGL3x.cs) | The `glGetFragDataLocation` lookup |
| [Renderer/Shaders/Uniform.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/Uniform.cs), [UniformCollection.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/UniformCollection.cs) | `UniformType`, `Uniform`, `Uniform<T>` |
| [Renderer/GL3x/Shaders/Uniform*GL3x.cs](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Renderer/GL3x/Shaders) | The dirty-uniform pattern. The 22 files are identical except for the value type and the `GL.Uniform*` call in `Clean`. Compare `UniformFloatVector3GL3x.cs` with `UniformFloatMatrix44GL3x.cs`. |
| [Renderer/GL3x/ICleanable.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ICleanable.cs), [ICleanableObserver.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/ICleanableObserver.cs) | The dirty-list interfaces |
| [Renderer/GL3x/TypeConverterGL3x.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/GL3x/TypeConverterGL3x.cs) | `To(ActiveAttribType)` (line 41) and `To(ActiveUniformType)` (line 72) |
| [Renderer/VertexArray/VertexLocations.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/VertexArray/VertexLocations.cs) | The location values. They differ from Listing 3.10: `Normal` is 2, not 1. |
| [Renderer/CouldNotCreateVideoCardResourceException.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/CouldNotCreateVideoCardResourceException.cs) | The exception type |
| [Core/Matrices/Matrix4F.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Matrices/Matrix4F.cs), [Matrix2.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Matrices/Matrix2.cs), [Matrix23.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Matrices/Matrix23.cs) | Column-major storage (`ReadOnlyColumnMajorValues`), and the "columns × rows" naming of the non-square matrices |

## Files

```
core/
  CMakeLists.txt                         UPDATE  list the two new headers
  include/arda/core/geometry/
    Vector2.h  Vector3.h  Vector4.h      UPDATE  operator==; Vector3 static_asserts; Vector4 includes
    Matrix4.h                            NEW     4x4 storage only; the math comes in Step 4
    Matrix.h                             NEW     storage for mat2, mat3 and the non-square matrices
renderer/
  CMakeLists.txt                         UPDATE
  include/arda/renderer/
    Exceptions.h                         NEW     CouldNotCreateVideoCardResourceException
    Device.h                             UPDATE  CreateShaderProgram, DoCreateShaderProgram
    shaders/
      ShaderVertexAttribute.h            NEW     ShaderVertexAttributeType, ShaderVertexAttribute, ShaderVertexAttributeCollection
      Uniform.h                          NEW     UniformType, UniformBase, Uniform<T>
      UniformCollection.h                NEW
      ShaderProgram.h                    NEW
    vertexarray/
      VertexLocations.h                  NEW
  src/
    Cleanable.h                          NEW     ICleanable, ICleanableObserver
    Device.cpp                           UPDATE  CreateShaderProgram
    shaders/
      UniformCollection.cpp              NEW
      BuiltinConstants.h / .cpp          NEW     API-agnostic table of og_ defines and constants
    gl/
      TypeConverterGL3x.h / .cpp         UPDATE  ToShaderVertexAttributeType, ToUniformType
      DeviceGL3x.h / .cpp                UPDATE  DoCreateShaderProgram
      shaders/
        GlslPrelude.h / .cpp             NEW     GlslPrelude()
        ShaderObjectGL3x.h / .cpp        NEW
        UniformGL3x.h / .cpp             NEW     UniformGL3x<T>, UploadUniform overloads
        ShaderProgramGL3x.h / .cpp       NEW
tests/
  CMakeLists.txt                         UPDATE
  src/renderer/ShaderProgramTests.cpp    NEW     public-API tests
  src/renderer/gl/UniformGL3xTests.cpp   NEW     white-box tests of the GL backend
```

This guide assumes Steps 0 and 1 are done: `CreateDevice` works,
`src/gl/GLHandle.h` has `ShaderName` and `ProgramName`, and
`src/gl/TypeConverterGL3x.h` / `.cpp` exist.

---

## How this guide is organized

Same format as [Step 0](00-setup.md): numbered parts, each one says what to
type and then explains it. Three kinds of callout:

> **C++ note — topic:** a C++ feature, explained where it first appears.
> Features that earlier guides explained get a one-line reminder and a link.

> **OpenGL note — topic:** what OpenGL is doing when a call is made.

> **Why:** a design decision, especially where arda differs from OpenGlobe.

The parts go from the bottom up: core types first, then the API-agnostic
public classes, then the GL backend, then the `Device` entry point that ties
them together, then tests. There are three build checkpoints on the way.
The [index at the end](#c-and-opengl-notes-index) lists every note in this
guide.

---

## OpenGL concepts for this step

Read this before typing anything. Every class in this step wraps one of the
ideas below.

### The programmable pipeline

When you draw, the GPU runs a fixed sequence of stages. Some are fixed
hardware (primitive assembly, clipping, rasterization, depth testing,
blending) and some run programs you write, called **shaders**:

```
vertex data ──► vertex shader ──► [geometry shader] ──► rasterizer ──► fragment shader ──► framebuffer
 (Step 3)       once per vertex     once per primitive    fixed         once per pixel       (Step 6)
                                    (optional)                          covered
```

- The **vertex shader** receives one vertex's attributes (position, normal,
  and so on) and must write `gl_Position`, the vertex's position in *clip
  coordinates*. It can also write outputs (`out vec3 worldNormal;`) that are
  interpolated across the triangle.
- The optional **geometry shader** receives a whole primitive (for example,
  the three vertices of a triangle) and emits zero or more primitives. The
  book uses it later for wide lines and billboards.
- The **fragment shader** runs once for every pixel the triangle covers. It
  receives the interpolated outputs and writes one or more colors
  (`out vec3 fragmentColor;`).

The GL 3.3 core profile has no fixed-function vertex or fragment processing.
Nothing draws without a program.

### Shader objects and program objects

OpenGL splits compiling from linking, like a C++ toolchain:

| C++ toolchain | OpenGL | arda |
|---|---|---|
| `.cpp` source | a GLSL string | `std::string_view` passed to `CreateShaderProgram` |
| compile to `.obj` | **shader object**: `glCreateShader`, `glShaderSource`, `glCompileShader` | `ShaderObjectGL3x` (private) |
| link to `.exe` | **program object**: `glCreateProgram`, `glAttachShader`, `glLinkProgram` | `ShaderProgramGL3x`, seen by users as `ShaderProgram` |
| compiler/linker output | the **info log**: `glGetShaderInfoLog`, `glGetProgramInfoLog` | `CompileLog()`, `Log()` |

Neither `glCompileShader` nor `glLinkProgram` reports failure through its
return value (both return `void`) or through `glGetError`. You must ask:
`glGetShaderiv(shader, GL_COMPILE_STATUS, &status)` and
`glGetProgramiv(program, GL_LINK_STATUS, &status)`. Forgetting to check is
the classic beginner bug: the program silently draws nothing.

Linking also checks the stages against each other. A fragment shader input
must match a vertex (or geometry) shader output with the same name and type,
and every stage needs a `main`. Those are link errors, not compile errors.

### GLSL `#version 330` and the prelude

Every GLSL source starts with `#version`. `#version 330` means GLSL 3.30, the
version that ships with OpenGL 3.3, and it defaults to the core profile. The
directive must come **before anything else except comments and whitespace**,
and it may appear only once.

OpenGlobe gives every shader a *prelude* of built-in constants (3.4.1,
Listing 3.8): `#define og_positionVertexLocation 0`, `const float og_pi =
3.14159...;` and so on. `glShaderSource` takes an array of strings and
compiles them as if they were concatenated, so the prelude is passed as the
first string and your source as the second. Because the prelude comes first,
it must contain the `#version` line. If your source has its own `#version
330`, it would then be a second `#version`, which is an error, so
`ShaderObjectGL3x` comments it out.

### Vertex attribute locations

A vertex shader input (`in vec4 position;`) reads from a numbered slot called
an **attribute location**. Step 3's vertex array connects each buffer to a
location. There are three ways a location gets chosen:

1. **In the shader**, with `layout(location = 0) in vec4 position;`. This is
   core in GLSL 3.30. OpenGlobe writes `layout(location =
   og_positionVertexLocation)`, and the prelude's `#define` supplies the
   number (3.4.2, Listing 3.10).
2. **In C++, before linking**, with `glBindAttribLocation(program, 0,
   "position")`. The older approach, needed before GLSL 3.30.
3. **By the linker**, if neither is used. Then you have to ask afterwards
   with `glGetAttribLocation`.

arda uses option 1, like the book, and asks with `glGetAttribLocation`
anyway, so the program's `VertexAttributes()` always reports the real
location whichever option the shader used.

### Fragment outputs

A fragment shader output (`out vec3 fragmentColor;`) writes to a numbered
color attachment of the framebuffer (3.4.3). With one output, the linker
assigns location 0. With several, use `layout(location = N) out ...` or
`glBindFragDataLocation` before linking. `glGetFragDataLocation` reports the
result, and Step 6 uses it to attach textures to the right outputs.

### Uniforms

A **uniform** is a shader input that stays the same for a whole draw call:
a color, a matrix, a texture unit. Each active uniform has a **location**, an
integer handle you get from `glGetUniformLocation(program, "u_color")`. You
set it with a `glUniform*` call that matches its type:

```cpp
glUseProgram(program);                        // glUniform* writes to the bound program
glUniform3f(location, 1.0f, 0.0f, 0.0f);      // vec3
glUniformMatrix4fv(location, 1, GL_FALSE, m); // mat4, 16 floats, column-major
```

Four facts about uniforms drive the design in this step:

1. **`glUniform*` writes to the program that is currently bound** with
   `glUseProgram`. GL 3.3 has no call that targets a program by name
   (`glProgramUniform*` is GL 4.1). So to set a uniform right away you would
   have to bind the program, which disturbs whatever the context had bound.
2. **Uniform values live in the program object.** They persist across draws
   and are shared by every context in the share group. The `glUseProgram`
   *binding*, on the other hand, is per-context state.
3. **Every uniform is zero after a successful link.**
4. **The linker removes uniforms (and attributes) that don't affect the
   output.** A uniform that is declared but never used is *inactive*: it has
   no location, and reflection doesn't report it. This surprises everyone
   once. See [Common problems](#common-problems).

Uniform locations are arbitrary integers chosen by the linker. They are not
the index `i` you pass to `glGetActiveUniform`. Always look them up by name.

### Reflection

After linking, a program can describe itself. arda calls this *reflection*,
and uses it to fill the program's collections:

| Query | Gives |
|---|---|
| `glGetProgramiv(p, GL_ACTIVE_ATTRIBUTES, &n)` | number of active vertex inputs |
| `glGetActiveAttrib(p, i, ...)` | name, array size and type (`GL_FLOAT_VEC4`, ...) of input `i` |
| `glGetProgramiv(p, GL_ACTIVE_UNIFORMS, &n)` | number of active uniforms |
| `glGetActiveUniform(p, i, ...)` | name, array size and type (`GL_FLOAT_MAT4`, `GL_SAMPLER_2D`, ...) of uniform `i` |
| `glGetActiveUniformsiv(p, 1, &i, GL_UNIFORM_BLOCK_INDEX, &b)` | whether uniform `i` lives in a uniform block |
| `glGetAttribLocation`, `glGetUniformLocation`, `glGetFragDataLocation` | the location for a name |

The `...MAX_LENGTH` queries give the longest name, including the null
terminator, so one buffer fits every name.

Two naming details:

- **Built-ins start with `gl_`.** A driver may list `gl_VertexID` as an
  active attribute. They have no location, so both OpenGlobe and arda skip
  them.
- **Array uniforms are reported as `name[0]`.** The GL 3.3 spec requires the
  suffix (OpenGlobe's comment blames ATI, which was simply first to follow
  the spec). `CorrectUniformName` strips it. arda, like OpenGlobe, throws for
  real arrays (`uniform float weights[4];`), so in practice only a
  one-element array ever has its name corrected.

### What a shader looks like from C++

In C++, GLSL source is a string. A **raw string literal** keeps it readable:

```cpp
constexpr const char* kFragmentShader = R"(
in vec3 worldNormal;
out vec3 fragmentColor;
uniform vec3 u_color;
void main()
{
    fragmentColor = u_color * og_oneOverPi + worldNormal;
})";
```

<a id="cpp-raw-string-literals"></a>

> **C++ note — raw string literals:** `R"( ... )"` is a string literal in
> which backslashes and quotes are ordinary characters and newlines are kept.
> Without it, the shader above would be
> `"in vec3 worldNormal;\nout vec3 fragmentColor;\n..."`. It's the same as
> C#'s verbatim `@"..."` strings, except that `"` needs no doubling. If the
> text itself contains `)"`, add a delimiter of up to 16 characters that
> appears on both ends: `R"glsl( ... )glsl"`. Note that the string above
> **starts with a newline**, because the line break after `R"(` is part of
> the literal. That matters for `#version`, as Part 10 explains.

---

## Build order at a glance

| Part | What | Checkpoint |
|---|---|---|
| 1 | Core: vector `==`, `Matrix4`, `Matrix` | build `arda_core` |
| 2 | `Exceptions.h`, `VertexLocations.h` | |
| 3 | `ShaderVertexAttribute.h` | |
| 4 | `Uniform.h` | |
| 5 | `UniformCollection.h` / `.cpp` | |
| 6 | `ShaderProgram.h` | |
| 7 | `Cleanable.h` | |
| 8 | `BuiltinConstants.h` / `.cpp` | **Checkpoint 1:** `arda_renderer` builds |
| 9 | `GlslPrelude` | |
| 10 | `ShaderObjectGL3x` | |
| 11 | `UniformGL3x` | |
| 12 | `TypeConverterGL3x` additions | |
| 13 | `ShaderProgramGL3x` | **Checkpoint 2:** `arda_renderer` builds with the GL files |
| 14 | `Device` and `DeviceGL3x` additions | **Checkpoint 3:** everything builds |
| 15 | Tests | **Milestone:** `arda_tests` passes |

---

## 1. Core prerequisites

Uniforms hold values of core types: `Vector3<float>` for a `vec3`,
`Matrix4<float>` for a `mat4`, and so on. This step needs three things from
`arda_core`.

### 1a. `operator==` on the vectors

A uniform only marks itself dirty when its new value differs from the old
one (Part 11), so every value type needs `==`. Add this line to each vector
class, in the `public:` section. The end of the operator overloads, just
before `private:`, is a good spot:

```cpp
// Vector2.h, inside class Vector2
bool operator==(const Vector2<T>& other) const = default;
```

```cpp
// Vector3.h, inside class Vector3
bool operator==(const Vector3<T>& other) const = default;
```

```cpp
// Vector4.h, inside class Vector4
bool operator==(const Vector4<T>& other) const = default;
```

A defaulted `operator==` compares the members in order (`m_x`, then `m_y`,
...). In C++20 the compiler also rewrites `a != b` as `!(a == b)`, so you get
`!=` for free. [Step 1](01-state-management.md) explains defaulted
comparisons in full.

### 1b. Two checks on `Vector3`

Step 3 copies arrays of `Vector3<float>` straight into GPU vertex buffers, so
the class must stay exactly three floats with no hidden members. Add two
`static_assert`s at the end of `Vector3.h`, after the free `operator*` and
before the namespace's closing brace:

```cpp
    // Step 3 copies arrays of Vector3<float> straight into GPU buffers.
    static_assert(sizeof(Vector3<float>) == 3 * sizeof(float));
    static_assert(std::is_trivially_copyable_v<Vector3<float>>);
}
```

`Vector3.h` already includes `<type_traits>`. Step 3 explains `static_assert`
and trivially copyable types.

### 1c. Fix the includes in `Vector4.h`

`Vector4.h` calls `std::sqrt` and throws `std::runtime_error` but includes
nothing. It only compiles today because every file that includes it happens
to include `<cmath>` and `<stdexcept>` first. `UniformGL3x.h` (Part 11) is
the first file where that isn't true. Add the includes under `#pragma once`:

```cpp
#pragma once

#include <cmath>
#include <stdexcept>
```

A header should include everything it uses. Then it compiles no matter what
was included before it.

### 1d. `core/include/arda/core/geometry/Matrix4.h` (new)

A GLSL `mat4` uniform needs a 4×4 matrix type. This step only needs storage.
Step 4 adds multiplication, projections and `LookAt`.

```cpp
#pragma once

#include <array>
#include <concepts>

namespace arda::core {

// A 4x4 matrix stored column-major, like GLSL and OpenGlobe's Matrix4D/Matrix4F.
// Step 2 needs only the storage. Step 4 adds the math.
template <std::floating_point T>
class Matrix4 {
public:
    // All zeros.
    constexpr Matrix4() = default;

    // Arguments are in reading order (row 0 first), like OpenGlobe's constructor.
    // They are stored column by column.
    constexpr Matrix4(T c0r0, T c1r0, T c2r0, T c3r0,
                      T c0r1, T c1r1, T c2r1, T c3r1,
                      T c0r2, T c1r2, T c2r2, T c3r2,
                      T c0r3, T c1r3, T c2r3, T c3r3)
        : m_values{c0r0, c0r1, c0r2, c0r3,
                   c1r0, c1r1, c1r2, c1r3,
                   c2r0, c2r1, c2r2, c2r3,
                   c3r0, c3r1, c3r2, c3r3} {}

    static constexpr Matrix4 Identity() {
        return Matrix4(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       0, 0, 0, 1);
    }

    constexpr T operator()(int column, int row) const { return m_values[column * 4 + row]; }
    constexpr T& operator()(int column, int row) { return m_values[column * 4 + row]; }

    // The 16 values, column-major. Passed directly to glUniformMatrix4fv.
    constexpr const T* Data() const { return m_values.data(); }

    bool operator==(const Matrix4&) const = default;

    // Step 4 adds: operator*, Transpose, Cast<U>, CreatePerspectiveFieldOfView,
    // CreateOrthographicOffCenter, LookAt.

private:
    std::array<T, 16> m_values{};
};

} // namespace arda::core
```

**Column-major** means the 16 numbers are stored one column after another:
index 0–3 is column 0, index 4–7 is column 1, and so on. Element (column
`c`, row `r`) is at `c * 4 + r`. GLSL stores matrices the same way, and
`mat[c][r]` in GLSL means the same element. The constructor takes its
arguments in *reading order*, row by row, because that's how you'd write a
matrix on paper. It shuffles them into columns.

`std::array<T, 16> m_values{}` with `{}` zero-initializes all 16 values, so
a default-constructed `Matrix4` is all zeros, matching GL's initial uniform
value. `std::floating_point` is a C++20 *concept* that limits `T` to
`float`, `double` and `long double`. Step 3 explains concepts, and Step 4
explains `constexpr` and operator overloading in depth.

### 1e. `core/include/arda/core/geometry/Matrix.h` (new)

GLSL has eight more matrix shapes: `mat2`, `mat3` and the non-square
`mat2x3`, `mat2x4`, `mat3x2`, `mat3x4`, `mat4x2` and `mat4x3`. OpenGlobe has
a C# class for each (`Matrix2`, `Matrix3F`, `Matrix23`, ...), and a uniform
class for each. One class template covers them all:

```cpp
#pragma once

#include <array>
#include <concepts>

namespace arda::core {

// Storage for the other matrix shapes GLSL has: mat2, mat3 and the
// non-square matStxR types. Columns x Rows, stored column-major, the same
// layout glUniformMatrix*fv expects. Like OpenGlobe's Matrix2, Matrix3F and
// Matrix23..Matrix43, it has no math yet. Matrix4 is a separate class
// because it gets the math in Step 4.
template <std::floating_point T, int Columns, int Rows>
class Matrix {
public:
    static_assert(Columns >= 2 && Columns <= 4 && Rows >= 2 && Rows <= 4);

    static constexpr int NumberOfColumns = Columns;
    static constexpr int NumberOfRows = Rows;

    // All zeros.
    constexpr Matrix() = default;

    constexpr T operator()(int column, int row) const { return m_values[column * Rows + row]; }
    constexpr T& operator()(int column, int row) { return m_values[column * Rows + row]; }

    // Columns * Rows values, column-major.
    constexpr const T* Data() const { return m_values.data(); }

    bool operator==(const Matrix&) const = default;

private:
    std::array<T, Columns * Rows> m_values{};
};

// GLSL's matCxR has C columns and R rows. So does MatrixCR here, and so
// does OpenGlobe's (Matrix23 is "2 columns and 3 rows").
template <typename T> using Matrix2  = Matrix<T, 2, 2>;
template <typename T> using Matrix3  = Matrix<T, 3, 3>;
template <typename T> using Matrix23 = Matrix<T, 2, 3>;
template <typename T> using Matrix24 = Matrix<T, 2, 4>;
template <typename T> using Matrix32 = Matrix<T, 3, 2>;
template <typename T> using Matrix34 = Matrix<T, 3, 4>;
template <typename T> using Matrix42 = Matrix<T, 4, 2>;
template <typename T> using Matrix43 = Matrix<T, 4, 3>;

} // namespace arda::core
```

`Columns` and `Rows` are *non-type template parameters*: template arguments
that are values instead of types. `Matrix<float, 2, 3>` and
`Matrix<float, 3, 2>` are different types, so `UploadUniform` can overload
on them. The `template <typename T> using Matrix23 = ...` lines are *alias
templates*: `Matrix23<float>` is just another spelling of
`Matrix<float, 2, 3>`. Class templates and `using` aliases are explained in
[Step 0](00-setup.md).

> **Why a second matrix type instead of making `Matrix4` an alias:** Step 4
> adds `Identity`, `LookAt` and the projection functions to `Matrix4`.
> They only make sense for 4×4 matrices. Keeping `Matrix4` separate lets
> Step 4 add them without template tricks, and keeps `Matrix<T, C, R>` a
> dumb container for uniform values.

### 1f. `core/CMakeLists.txt`

Add the two headers to the IDE list in `add_library(arda_core ...)`:

```cmake
    include/arda/core/geometry/Matrix.h
    include/arda/core/geometry/Matrix4.h
```

**Build now:** `cmake --build build/vs --config Debug --target arda_core`.
Header-only changes compile when something includes them, so also build the
existing tests, which include the vectors:
`cmake --build build/vs --config Debug --target arda_tests`.

---

## 2. `Exceptions.h` and `VertexLocations.h`

### `include/arda/renderer/Exceptions.h`

```cpp
#pragma once

#include <stdexcept>

namespace arda::renderer {

// Thrown when a GPU resource can't be created: a shader that fails to
// compile, a program that fails to link, and so on.
// CouldNotCreateVideoCardResourceException.cs.
class CouldNotCreateVideoCardResourceException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} // namespace arda::renderer
```

<a id="cpp-custom-exceptions"></a>

> **C++ note — custom exception classes:** C++ can throw any type, even an
> `int`, but by convention you throw classes derived from `std::exception`,
> which has a virtual `what()` returning the message. `std::runtime_error`
> (errors only detectable at run time) and `std::logic_error` (bugs, with
> subclasses `std::invalid_argument` and `std::out_of_range`) store the
> message for you. Deriving from `std::runtime_error` gives three ways to
> catch a failed shader:
>
> ```cpp
> try {
>     auto sp = device->CreateShaderProgram(vs, fs);
> } catch (const CouldNotCreateVideoCardResourceException& e) {   // just this error
>     std::fprintf(stderr, "%s\n", e.what());
> } catch (const std::runtime_error& e) {                          // any runtime error
> } catch (const std::exception& e) {                              // anything standard
> }
> ```
>
> Handlers are tried in order, so put the most derived type first. Always
> catch by `const&`: catching by value copies the exception and *slices*
> it down to the base class, losing the derived part. The C# class has four
> constructors (empty, message, message plus inner exception,
> serialization). `using std::runtime_error::runtime_error;` inherits the
> base class's constructors instead, which take a `const std::string&` or a
> `const char*` message. C++ has no inner exceptions in the C# sense
> (`std::nested_exception` exists but is rarely used), and no serialization.

`CreateShaderProgram` throws two other standard exceptions too:
`std::invalid_argument` for a bad argument (an empty vertex shader, an
unsupported `#version`), and `std::out_of_range` from lookups by name. That
follows the standard library's split: `invalid_argument` and `out_of_range`
mean "the caller made a mistake", while
`CouldNotCreateVideoCardResourceException` means "the driver said no".
[Step 0](00-setup.md) explains how exceptions work in general.

### `include/arda/renderer/vertexarray/VertexLocations.h`

```cpp
#pragma once

// The attribute location each kind of vertex data uses (3.4.2). Shaders see
// the same values through the og_*VertexLocation defines. The values match
// OpenGlobe's VertexLocations.cs, which differs from Listing 3.10: Normal is 2.
namespace arda::renderer::VertexLocations {

inline constexpr int Position = 0;
inline constexpr int PositionLow = 1;
inline constexpr int Normal = 2;
inline constexpr int TextureCoordinate = 3;
inline constexpr int Color = 4;

// Position and PositionHigh share a location so that shaders with and
// without emulated doubles (Chapter 5) can use the same vertex array.
inline constexpr int PositionHigh = Position;

} // namespace arda::renderer::VertexLocations
```

In C#, `VertexLocations` is a `static class` holding `const int`s. C++
doesn't need a class for that: a namespace groups the names the same way,
and the call site reads the same, `VertexLocations::Normal`.

`inline constexpr int` defines a compile-time constant in a header. Every
`.cpp` that includes the header sees the same single variable, and it can be
used where a constant expression is required (an array size, a `case`
label). [Step 1](01-state-management.md) explains `inline` and `constexpr`
variables.

> **Why the locations live in one table:** the vertex array (Step 3) binds
> the mesh's positions to location 0, and the shader reads `position` from
> location 0. If those two numbers came from different places they would
> eventually disagree. With `VertexLocations` for C++ and the matching
> `og_*VertexLocation` defines for GLSL (Part 8), both sides use the same
> number.

---

## 3. `include/arda/renderer/shaders/ShaderVertexAttribute.h`

After linking, the program reports each active vertex shader input: its
name, location, type and array length. Step 3 matches these against a
mesh's attributes by name.

```cpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace arda::renderer {

// The types a vertex shader input can have. ShaderVertexAttribute.cs.
enum class ShaderVertexAttributeType {
    Float,
    FloatVector2,
    FloatVector3,
    FloatVector4,
    FloatMatrix22,
    FloatMatrix33,
    FloatMatrix44,
    Int,
    IntVector2,
    IntVector3,
    IntVector4,
};

// One active vertex shader input, found by reflection after linking.
struct ShaderVertexAttribute {
    std::string name;
    int location = -1;
    ShaderVertexAttributeType type = ShaderVertexAttributeType::Float;
    int length = 1;   // array length; 1 unless the input is an array
};

// ShaderVertexAttributeCollection.cs: a list that can also be searched by name.
class ShaderVertexAttributeCollection {
public:
    void Add(ShaderVertexAttribute attribute) { m_attributes.push_back(std::move(attribute)); }

    // nullptr if there is no attribute with this name.
    const ShaderVertexAttribute* Find(std::string_view name) const {
        auto it = std::ranges::find(m_attributes, name, &ShaderVertexAttribute::name);
        return it == m_attributes.end() ? nullptr : &*it;
    }

    bool Contains(std::string_view name) const { return Find(name) != nullptr; }

    // Throws std::out_of_range if there is no attribute with this name.
    const ShaderVertexAttribute& operator[](std::string_view name) const {
        if (const ShaderVertexAttribute* attribute = Find(name)) {
            return *attribute;
        }
        throw std::out_of_range("No vertex attribute named '" + std::string(name) + "'");
    }

    std::size_t Size() const { return m_attributes.size(); }
    auto begin() const { return m_attributes.begin(); }
    auto end() const { return m_attributes.end(); }

private:
    std::vector<ShaderVertexAttribute> m_attributes;
};

} // namespace arda::renderer
```

How it maps to the C#:

| C# | C++ |
|---|---|
| `class ShaderVertexAttribute` with a constructor and four read-only properties | a plain `struct` with public fields (the README's naming convention for data records) |
| `Datatype` property | `type` field |
| `KeyedCollection<string, ShaderVertexAttribute>` | a `std::vector` searched by name |
| `attributes["position"]` (throws `KeyNotFoundException`) | `attributes["position"]` (throws `std::out_of_range`) |
| `attributes.Contains("position")` | `attributes.Contains("position")` |
| `foreach (var a in attributes)` | `for (const ShaderVertexAttribute& a : attributes)` |

Because `ShaderVertexAttribute` is an aggregate with default member
initializers, the backend can create one with braces:
`{"position", 0, ShaderVertexAttributeType::FloatVector4, 1}`
([Step 1](01-state-management.md) covers aggregates). Step 3's tests build
collections by hand this way.

A program has a handful of attributes, so a linear search is as fast as a
hash map and simpler. `UniformCollection` (Part 5) uses a hash map, mostly
to show how.

<a id="cpp-string-view"></a>

> **C++ note — `std::string_view`:** a `std::string_view` is a pointer and a
> length that *refers to* characters stored somewhere else. It never owns or
> copies them. A function that takes `std::string_view` accepts a string
> literal, a `std::string` or a `const char*` without allocating anything:
>
> ```cpp
> void Print(std::string_view text);
> Print("literal");            // no allocation
> Print(someStdString);        // no copy
> ```
>
> A `const std::string&` parameter would force callers with a literal to
> build a temporary `std::string` (an allocation, for longer strings). C#
> has no equivalent because its strings are immutable references already.
> Two rules keep `string_view` safe:
>
> 1. **It isn't null-terminated.** `view.data()` points at the first
>    character, but nothing guarantees a `'\0'` after the last one. C APIs
>    such as `glGetUniformLocation(program, const char* name)` need a
>    terminator, so convert first: `std::string(view).c_str()`.
> 2. **It dangles if the characters go away.** Don't store a `string_view`
>    that came from a temporary `std::string`:
>
>    ```cpp
>    std::string_view bad = std::string("temp");   // the string dies at the ;
>    ```
>
>    As a *parameter* it's fine, even with a temporary argument:
>    `CreateShaderProgram(std::string("...") + source, fs)` keeps the
>    temporary alive until the call returns. Store a `std::string` when you
>    need to keep text, which is why `ShaderVertexAttribute::name` is a
>    `std::string`.

<a id="cpp-ranges-find"></a>

> **C++ note — `std::ranges::find` with a projection:** `std::ranges::find(
> m_attributes, name, &ShaderVertexAttribute::name)` searches the whole
> vector (no `begin()`/`end()` pair needed) for an element whose `name`
> member equals `name`. The third argument is a *projection*: a pointer to
> the member to compare, applied to each element before comparing. It reads
> like LINQ's `attributes.FirstOrDefault(a => a.Name == name)`. The result is
> an iterator; `&*it` turns it into a pointer to the element.

> **C++ note — `begin()` and `end()` make range-for work:** any class with
> `begin()` and `end()` members can be used in `for (const auto& a :
> collection)`. Here they just forward the vector's iterators. The `auto`
> return type lets the compiler write out the iterator type
> (`std::vector<ShaderVertexAttribute>::const_iterator`) for you. Because
> the functions are `const`, callers get read-only access, which is all they
> need.

---

## 4. `include/arda/renderer/shaders/Uniform.h` (Listing 3.11)

This is the most interesting C++ in the step. Here is the C# (Uniform.cs):

```csharp
public class Uniform {
    protected Uniform(string name, UniformType type) { ... }
    public string Name { get; }
    public UniformType Datatype { get; }
}

public abstract class Uniform<T> : Uniform {
    public abstract T Value { set; get; }
}
```

And the C++:

```cpp
#pragma once

#include <string>
#include <utility>

namespace arda::renderer {

// Every uniform type OpenGlobe supports, in the same order as Uniform.cs.
enum class UniformType {
    Int,
    Float,
    FloatVector2,
    FloatVector3,
    FloatVector4,
    IntVector2,
    IntVector3,
    IntVector4,
    Bool,
    BoolVector2,
    BoolVector3,
    BoolVector4,
    FloatMatrix22,
    FloatMatrix33,
    FloatMatrix44,
    Sampler1D,
    Sampler2D,
    Sampler2DRectangle,
    Sampler2DRectangleShadow,
    Sampler3D,
    SamplerCube,
    Sampler1DShadow,
    Sampler2DShadow,
    FloatMatrix23,
    FloatMatrix24,
    FloatMatrix32,
    FloatMatrix34,
    FloatMatrix42,
    FloatMatrix43,
    Sampler1DArray,
    Sampler2DArray,
    Sampler1DArrayShadow,
    Sampler2DArrayShadow,
    SamplerCubeShadow,
    IntSampler1D,
    IntSampler2D,
    IntSampler2DRectangle,
    IntSampler3D,
    IntSamplerCube,
    IntSampler1DArray,
    IntSampler2DArray,
    UnsignedIntSampler1D,
    UnsignedIntSampler2D,
    UnsignedIntSampler2DRectangle,
    UnsignedIntSampler3D,
    UnsignedIntSamplerCube,
    UnsignedIntSampler1DArray,
    UnsignedIntSampler2DArray,
};

// The part of a uniform that doesn't depend on its value type. OpenGlobe
// calls this class Uniform. C++ can't have a class and a class template with
// the same name, so here it is UniformBase (Listing 3.11).
class UniformBase {
public:
    virtual ~UniformBase() = default;

    UniformBase(const UniformBase&)            = delete;
    UniformBase& operator=(const UniformBase&) = delete;

    const std::string& Name() const { return m_name; }
    UniformType Datatype() const { return m_type; }

protected:
    UniformBase(std::string name, UniformType type) : m_name(std::move(name)), m_type(type) {}

private:
    std::string m_name;
    UniformType m_type;
};

// A uniform whose value is a T. Each backend derives its own class from this.
template <typename T>
class Uniform : public UniformBase {
public:
    virtual const T& Value() const = 0;
    virtual void SetValue(const T& value) = 0;

protected:
    using UniformBase::UniformBase;   // inherit UniformBase(std::string, UniformType)
};

} // namespace arda::renderer
```

`UniformType` is the full enum from `Uniform.cs`, in the same order. It says
what the *shader* declared (`sampler2D` vs `samplerCube`), which is more
detail than the C++ value type carries: every sampler is a `Uniform<int>`.

The C# property `T Value { set; get; }` becomes a getter/setter pair. The
setter is a function, not an assignment, because setting a uniform does
work (Part 11).

<a id="cpp-type-erasure"></a>

> **C++ note — a non-template base class under a class template:** a
> program's uniforms have different value types: `Uniform<float>`,
> `Uniform<Vector3<float>>`, `Uniform<Matrix4<float>>`. Those are unrelated
> types in C++. There is no `Uniform<?>` wildcard, and unlike C# generics
> they share nothing at run time. To store them in one container, they need
> a common, *non-template* base class, and the container holds pointers to
> that base:
>
> ```
>                 UniformBase                 name, type; virtual destructor
>              /       |        \
>   Uniform<float>  Uniform<Vector3f>  ...    Value(), SetValue(): pure virtual
>        |              |
>  UniformGL3x<float>  UniformGL3x<Vector3f>  the backend's implementation (Part 11)
> ```
>
> Code that doesn't care about the value type (listing names, matching
> automatic uniforms by name in Step 4) works with `UniformBase&`. Code that
> knows the type asks for a `Uniform<T>&` (Part 5). This is a simple form of
> *type erasure*: the base class "erases" `T` so different instantiations
> can be handled uniformly. It is exactly OpenGlobe's `Uniform` /
> `Uniform<T>` split; C++ just can't reuse the name `Uniform` for both.
> The README's naming convention (`FooBase` + `Foo<T>`) comes from here.

<a id="cpp-virtual-templates"></a>

> **C++ note — why a member function template can't be virtual:** you might
> want to put the typed accessors on the base class:
>
> ```cpp
> class UniformBase {
>     template <typename T>
>     virtual void SetValue(const T& value) = 0;   // error: templates may not be virtual
> };
> ```
>
> A virtual call goes through a *vtable*: a fixed array of function
> pointers, one per virtual function, laid out when the class is compiled.
> A member function template is not one function but a recipe for an
> unlimited number of them (`SetValue<float>`, `SetValue<int>`, ...), created
> on demand in whatever `.cpp` file uses them. The compiler can't reserve
> vtable slots for functions that don't exist yet, so C++ forbids it. (C#
> can do it because the .NET JIT creates code at run time.) The fix is
> the one used here: make the *class* a template, so each
> `Uniform<T>` has its own vtable with `Value()` and `SetValue()` for that
> one `T`. A non-virtual template can still sit on top and *pick* the right
> class, which is what `UniformCollection::Get<T>` does (Part 5). The README
> uses the same trick for buffers: a template `CopyFromSystemMemory` that
> forwards to a virtual byte-based function.

<a id="cpp-inheriting-constructors"></a>

> **C++ note — inheriting constructors:** `using UniformBase::UniformBase;`
> gives `Uniform<T>` the base's `(std::string, UniformType)` constructor, so
> it doesn't have to write `Uniform(std::string name, UniformType type) :
> UniformBase(std::move(name), type) {}` by hand. The inherited constructor
> keeps its access level: it was `protected` in `UniformBase`, so it is
> `protected` in `Uniform<T>`, and only derived classes such as
> `UniformGL3x<T>` can call it. `Exceptions.h` used the same feature with
> `std::runtime_error`.

The copy operations are deleted and the constructor is protected, like
`Device` and `ShaderProgram`: a uniform belongs to one program, is only
created by a backend, and is never copied. [Step 0](00-setup.md) explains
virtual destructors, deleted copies and protected constructors.

> **Why `UniformBase` takes `std::string` by value and moves it:** the
> constructor stores the name, so it needs its own copy anyway. Taking it by
> value and `std::move`-ing it into `m_name` costs one copy when the caller
> passes an lvalue and zero copies when the caller passes a temporary, which
> is what the backend does. See the move semantics note in
> [Step 0](00-setup.md).

---

## 5. `UniformCollection.h` and `UniformCollection.cpp`

The collection owns a program's uniforms and finds them by name. In C#,
client code finds a uniform and casts it:

```csharp
((Uniform<Vector3F>)sp.Uniforms["u_color"]).Value = new Vector3F(1, 0, 0);
```

In arda, `Get<T>` does the lookup and the checked cast in one call:

```cpp
sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1, 0, 0));
```

### `include/arda/renderer/shaders/UniformCollection.h`

```cpp
#pragma once

#include <arda/renderer/shaders/Uniform.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arda::renderer {

// A shader program's uniforms, in the order the backend found them, plus a
// lookup by name. In OpenGlobe this is a KeyedCollection<string, Uniform>.
class UniformCollection {
public:
    // Takes ownership. Throws std::invalid_argument if the name is already used.
    void Add(std::unique_ptr<UniformBase> uniform);

    // nullptr if the program has no active uniform with this name.
    UniformBase* Find(std::string_view name);
    const UniformBase* Find(std::string_view name) const;

    bool Contains(std::string_view name) const { return Find(name) != nullptr; }

    // Throws std::out_of_range if the program has no active uniform with this name.
    UniformBase& operator[](std::string_view name);
    const UniformBase& operator[](std::string_view name) const;

    // The uniform as a Uniform<T>. Throws std::out_of_range if there is no
    // such uniform, and std::invalid_argument if its value type isn't T.
    template <typename T>
    Uniform<T>& Get(std::string_view name) {
        if (auto* typed = dynamic_cast<Uniform<T>*>(&(*this)[name])) {
            return *typed;
        }
        ThrowWrongType(name);
    }

    template <typename T>
    const Uniform<T>& Get(std::string_view name) const {
        if (const auto* typed = dynamic_cast<const Uniform<T>*>(&(*this)[name])) {
            return *typed;
        }
        ThrowWrongType(name);
    }

    std::size_t Size() const { return m_uniforms.size(); }

    // Iterates the owning pointers: for (const std::unique_ptr<UniformBase>& uniform : uniforms)
    auto begin() const { return m_uniforms.begin(); }
    auto end() const { return m_uniforms.end(); }

private:
    [[noreturn]] static void ThrowWrongType(std::string_view name);

    // Lets m_byName.find() take a std::string_view without building a std::string.
    struct StringHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view text) const noexcept {
            return std::hash<std::string_view>{}(text);
        }
    };

    std::vector<std::unique_ptr<UniformBase>> m_uniforms;   // owns the uniforms
    std::unordered_map<std::string, UniformBase*, StringHash, std::equal_to<>> m_byName;   // points into m_uniforms
};

} // namespace arda::renderer
```

There are two data members, doing two jobs:

- **`m_uniforms` owns.** A `std::vector<std::unique_ptr<UniformBase>>` keeps
  the uniforms in the order GL reported them and deletes them when the
  collection (and so the program) is destroyed.
- **`m_byName` finds.** A hash map from name to a plain, non-owning pointer
  into `m_uniforms`.

<a id="cpp-vector-unique-ptr"></a>

> **C++ note — `std::vector<std::unique_ptr<Base>>` for polymorphic
> objects:** a `std::vector<UniformBase>` can't work. A vector stores its
> elements by value, all the same size, and a `UniformGL3x<Matrix4<float>>`
> is bigger than a `UniformBase` (and `UniformBase` is abstract anyway).
> Putting a derived object into a base-typed slot would *slice* it. So the
> vector stores pointers, and `std::unique_ptr` makes each pointer the sole
> owner of its object: when the vector is destroyed, each `unique_ptr`
> deletes its uniform, through `UniformBase`'s virtual destructor, which
> runs the right derived destructor. This is the C++ spelling of C#'s
> `List<Uniform>`, where the garbage collector owns everything.
>
> Two properties matter here. First, moving the `unique_ptr` into the vector
> (`push_back(std::move(uniform))`) transfers ownership without moving the
> uniform itself, so the uniform's address never changes, and raw pointers
> to it (in `m_byName`, and in the program's dirty list in Part 13) stay
> valid while the vector grows. Second, the vector can't be copied, because
> `unique_ptr` can't, so the whole collection is move-only. That's what we
> want: copying a program's uniforms would make no sense.
> [Step 0](00-setup.md) introduces `std::unique_ptr` itself.

<a id="cpp-unordered-map"></a>

> **C++ note — `std::unordered_map` and heterogeneous lookup:**
> `std::unordered_map<Key, Value>` is a hash table, like C#'s
> `Dictionary<TKey, TValue>`. `std::map` is the other standard map: a sorted
> tree, with ordered iteration and O(log n) lookup. We don't need order
> (`m_uniforms` keeps it), so the hash map is the natural choice.
>
> The extra template arguments solve a performance trap. By default,
> `m_byName.find(key)` takes a `const std::string&`, so calling it with a
> `std::string_view` would build a temporary `std::string` on every lookup.
> C++20 allows *heterogeneous lookup* if both the hash and the equality
> functions declare `using is_transparent = void;`:
>
> - `StringHash` hashes a `std::string_view`. A `std::string` converts to one
>   implicitly, and `std::hash<std::string_view>` gives the same hash for the
>   same characters, so keys and lookups agree.
> - `std::equal_to<>` (with empty angle brackets) is the standard
>   transparent equality: it compares any two types that have an `==`.
>
> With both in place, `m_byName.find(name)` takes the `string_view`
> directly. `std::map` has the same feature with `std::less<>` and needs no
> custom hash; Step 4's `NamedCollection` uses that form. Heterogeneous
> lookup for `unordered_map` needs a recent C++20 standard library (Visual
> Studio 2022, GCC 11 or Clang 12 and later).

<a id="cpp-dynamic-cast"></a>

> **C++ note — `dynamic_cast` vs `static_cast`:** both convert a base-class
> pointer or reference to a derived type. They differ in checking:
>
> | | `static_cast<Derived*>(base)` | `dynamic_cast<Derived*>(base)` |
> |---|---|---|
> | Checked? | No. The compiler trusts you. | Yes, at run time, using the object's type information |
> | Wrong type | Undefined behavior: a bad pointer that may crash later, or corrupt memory | Returns `nullptr` (pointer form) or throws `std::bad_cast` (reference form) |
> | Cost | Free | A type lookup, roughly a few string comparisons |
> | Needs | Only that the types are related | A polymorphic base (at least one virtual function) |
>
> `Get<T>` uses the pointer form: `dynamic_cast<Uniform<T>*>(&uniform)` is
> `nullptr` when someone asks for `Get<float>("u_color")` on a `vec3`, and
> `Get` turns that into a clear `std::invalid_argument`. The C# cast
> `(Uniform<Vector3F>)x` is checked too, so this matches it; a
> `static_cast` here would turn a typo into memory corruption. Step 4's
> automatic uniforms use the reference form,
> `dynamic_cast<Uniform<core::Matrix4<float>>&>(uniform)`, which throws
> `std::bad_cast` if a shader declares `og_modelViewPerspectiveMatrix` with
> the wrong type.
>
> The backends use `static_cast` when *they* know the type, for example
> `static_cast<ShaderProgramGL3x&>(*drawState.shaderProgram)` in Step 3: a
> GL context only ever sees GL programs, so the check would cost time and
> never fail. The rule of thumb: `dynamic_cast` when the type depends on
> input (here, the shader source), `static_cast` when it is guaranteed by
> the design. [Step 0](00-setup.md) covers `static_cast` and the other casts.

> **Why a typed `Get<T>` instead of returning `UniformBase&` and letting
> the caller cast:** the cast is needed at almost every call site, it's easy
> to get wrong, and a `static_cast` in user code would be unchecked. `Get<T>`
> puts one checked cast in one place and gives a good error message. The
> untyped `operator[]` is still there for code that only needs the name or
> `Datatype()`.

<a id="cpp-const-overloads"></a>

> **C++ note — `const` and non-`const` overloads:** `Find`, `operator[]` and
> `Get<T>` each come in two versions. C++ picks one based on whether the
> collection is `const`: through a `const UniformCollection&` you get
> `const UniformBase&` and `const Uniform<T>&`, on which only `Value()` and
> the other `const` functions can be called. This makes `const` mean "you
> can look but not change", all the way down. `ShaderProgram::Uniforms()`
> has the same pair. (Iterating with `begin()`/`end()` gives
> `const std::unique_ptr<UniformBase>&`, and a `const unique_ptr` still
> points at a non-`const` object. Step 4 relies on that to wire up automatic
> uniforms.)

<a id="cpp-noreturn"></a>

> **C++ note — `[[noreturn]]`:** `ThrowWrongType` always throws. Marking it
> `[[noreturn]]` tells the compiler that control never comes back from the
> call, so it doesn't warn that `Get` "doesn't return a value on all paths".
> Putting the throw in a non-template function also keeps the message
> building out of the header, instead of being compiled again for every
> `T`. `static` means it doesn't need a `this`; [Step 1](01-state-management.md)
> covers static member functions.

### `src/shaders/UniformCollection.cpp`

```cpp
#include <arda/renderer/shaders/UniformCollection.h>

#include <stdexcept>
#include <utility>

namespace arda::renderer {

void UniformCollection::Add(std::unique_ptr<UniformBase> uniform) {
    if (!uniform) {
        throw std::invalid_argument("UniformCollection::Add: uniform is null");
    }
    const std::string& name = uniform->Name();
    if (m_byName.contains(name)) {
        throw std::invalid_argument("Duplicate uniform '" + name + "'");
    }
    m_byName.emplace(name, uniform.get());
    m_uniforms.push_back(std::move(uniform));
}

UniformBase* UniformCollection::Find(std::string_view name) {
    auto it = m_byName.find(name);
    return it == m_byName.end() ? nullptr : it->second;
}

const UniformBase* UniformCollection::Find(std::string_view name) const {
    auto it = m_byName.find(name);
    return it == m_byName.end() ? nullptr : it->second;
}

UniformBase& UniformCollection::operator[](std::string_view name) {
    if (UniformBase* uniform = Find(name)) {
        return *uniform;
    }
    throw std::out_of_range("No uniform named '" + std::string(name) + "'");
}

const UniformBase& UniformCollection::operator[](std::string_view name) const {
    if (const UniformBase* uniform = Find(name)) {
        return *uniform;
    }
    throw std::out_of_range("No uniform named '" + std::string(name) + "'");
}

void UniformCollection::ThrowWrongType(std::string_view name) {
    throw std::invalid_argument("Uniform '" + std::string(name) + "' is not of the requested type");
}

} // namespace arda::renderer
```

Notes:

- In `Add`, `name` is a reference to the string *inside* the uniform. It
  stays valid after `std::move(uniform)`, because moving a `unique_ptr`
  moves the pointer, not the object. The order still matters for a
  different reason: `emplace` copies `name` into the map's key *before* the
  move, while `uniform` is certainly still valid.
- `contains` is C++20. In older code you'll see `find(name) != end()`.
- `[[noreturn]]` goes on the declaration only, not the definition.
- `if (UniformBase* uniform = Find(name))` declares a variable inside the
  `if` condition. It's in scope only inside the `if`, and the test is
  "is the pointer non-null".

---

## 6. `include/arda/renderer/shaders/ShaderProgram.h` (Listing 3.9)

```cpp
#pragma once

#include <arda/renderer/shaders/ShaderVertexAttribute.h>
#include <arda/renderer/shaders/UniformCollection.h>

#include <string>
#include <string_view>

namespace arda::renderer {

// A linked vertex shader, optional geometry shader and fragment shader
// (3.4, Listing 3.9). Create one with Device::CreateShaderProgram.
class ShaderProgram {
public:
    virtual ~ShaderProgram() = default;

    ShaderProgram(const ShaderProgram&)            = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // The link log. Usually empty, but some drivers put warnings here.
    virtual std::string Log() const = 0;

    const ShaderVertexAttributeCollection& VertexAttributes() const { return m_vertexAttributes; }

    UniformCollection& Uniforms() { return m_uniforms; }
    const UniformCollection& Uniforms() const { return m_uniforms; }

    // The color attachment index that a fragment shader output writes to
    // (3.4.3). Throws std::out_of_range if there is no output with this name.
    virtual int FragmentOutputLocation(std::string_view name) const = 0;

    // Step 4 adds InitializeAutomaticUniforms and SetDrawAutomaticUniforms here.

protected:
    ShaderProgram() = default;

    // The backend's constructor fills these in after linking.
    ShaderVertexAttributeCollection m_vertexAttributes;
    UniformCollection m_uniforms;
};

} // namespace arda::renderer
```

Compared with `ShaderProgram.cs`:

| C# | C++ | Change |
|---|---|---|
| `abstract VertexAttributes { get; }` | `VertexAttributes()`, non-virtual, returns a base-class member | collections moved to the base class |
| `abstract Uniforms { get; }` | `Uniforms()`, non-virtual, two overloads | same |
| `abstract FragmentOutputs FragmentOutputs { get; }` with an indexer | `virtual int FragmentOutputLocation(name)` | one function instead of a class |
| `abstract Log { get; }` | `virtual std::string Log() const` | |
| `abstract UniformBlocks { get; }` | not ported | the book never uses uniform blocks |
| extends `Disposable` | virtual destructor, deleted copies | RAII in the backend |

> **Why the collections live in the base class:** the collections don't
> depend on the API. GL and D3D11 fill them differently (from
> `glGetActiveUniform` or from `ID3D11ShaderReflection`), but once filled
> they're identical. Keeping them in the base means each backend fills two
> `protected` members instead of implementing four virtual getters, and
> Step 4's `InitializeAutomaticUniforms` can loop over `m_uniforms` in
> API-agnostic code. This is the README's "base classes hold API-agnostic
> state" pattern.

> **Why `FragmentOutputs` became one function:** OpenGlobe's
> `FragmentOutputs` class exists only to provide `outputs["name"]`, and its
> GL version just calls `glGetFragDataLocation`. A virtual function does
> the same job without an extra class, an extra allocation and an extra
> header.

---

## 7. `src/Cleanable.h` (private)

These two interfaces implement the dirty list of 3.4.4. They are private
(in `src/`, not `include/`), because only backends use them.

```cpp
#pragma once

namespace arda::renderer {

// Something whose changes are applied to the graphics API later, just
// before a draw call, instead of immediately. GL3x/ICleanable.cs.
class ICleanable {
public:
    virtual ~ICleanable() = default;
    virtual void Clean() = 0;
};

// Something that keeps a list of cleanables that have changed and cleans
// them before drawing. GL3x/ICleanableObserver.cs.
class ICleanableObserver {
public:
    virtual ~ICleanableObserver() = default;
    virtual void NotifyDirty(ICleanable& value) = 0;
};

} // namespace arda::renderer
```

C++ has no `interface` keyword. An interface is a class with only pure
virtual functions and a virtual destructor, and a class "implements" it by
deriving from it. The `I` prefix is kept from the C# names.

The names are in `arda::renderer`, not `arda::renderer::gl`, because
Step 8's D3D11 uniforms and constant buffers reuse them.

---

## 8. `src/shaders/BuiltinConstants.h` and `.cpp`

OpenGlobe builds the prelude with string concatenation inside
`ShaderObjectGL3x`. arda splits it in two: an API-agnostic table of names
and values here, and a GLSL formatter in the GL backend (Part 9). Step 8's
HLSL formatter reads the same table.

### `src/shaders/BuiltinConstants.h`

```cpp
#pragma once

#include <span>
#include <string_view>

namespace arda::renderer {

// A preprocessor define every shader gets, e.g. og_positionVertexLocation.
struct BuiltinDefine {
    std::string_view name;
    int value;
};

// A float constant every shader gets, e.g. og_pi.
struct BuiltinConstant {
    std::string_view name;
    double value;
};

// The tables both backends build their shader prelude from
// (ShaderObjectGL3x.cs, lines 22-43).
std::span<const BuiltinDefine> BuiltinDefines();
std::span<const BuiltinConstant> BuiltinConstants();

} // namespace arda::renderer
```

### `src/shaders/BuiltinConstants.cpp`

```cpp
#include "shaders/BuiltinConstants.h"

#include <arda/core/Trig.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <limits>
#include <numbers>

namespace arda::renderer {

namespace {

using core::Trig;

constexpr BuiltinDefine s_defines[] = {
    {"og_positionVertexLocation",          VertexLocations::Position},
    {"og_normalVertexLocation",            VertexLocations::Normal},
    {"og_textureCoordinateVertexLocation", VertexLocations::TextureCoordinate},
    {"og_colorVertexLocation",             VertexLocations::Color},
    {"og_positionHighVertexLocation",      VertexLocations::PositionHigh},
    {"og_positionLowVertexLocation",       VertexLocations::PositionLow},
};

constexpr BuiltinConstant s_constants[] = {
    {"og_E",                std::numbers::e},
    {"og_pi",               std::numbers::pi},
    {"og_oneOverPi",        Trig::OneOverPi},
    {"og_piOverTwo",        Trig::PiOverTwo},
    {"og_piOverThree",      Trig::PiOverThree},
    {"og_piOverFour",       Trig::PiOverFour},
    {"og_piOverSix",        Trig::PiOverSix},
    {"og_threePiOver2",     Trig::ThreePiOverTwo},
    {"og_twoPi",            Trig::TwoPi},
    {"og_oneOverTwoPi",     Trig::OneOverTwoPi},
    {"og_radiansPerDegree", Trig::RadiansPerDegree},
    {"og_maximumFloat",     std::numeric_limits<float>::max()},
    {"og_minimumFloat",     std::numeric_limits<float>::lowest()},
};

} // namespace

std::span<const BuiltinDefine> BuiltinDefines() {
    return s_defines;
}

std::span<const BuiltinConstant> BuiltinConstants() {
    return s_constants;
}

} // namespace arda::renderer
```

The GLSL names are OpenGlobe's, including the odd `og_threePiOver2`, so the
book's shaders work unchanged. The C++ values come from `core::Trig`, which
already has the same constants. C#'s `float.MinValue` is the most negative
float, which is `std::numeric_limits<float>::lowest()`, *not* `min()`
(`min()` is the smallest positive normalized float, about 1.2e-38, a
well-known C++ trap).

<a id="cpp-span"></a>

> **C++ note — `std::span`:** a `std::span<const T>` is a pointer plus a
> count that *views* a contiguous run of `T`s owned by someone else: a C
> array, a `std::array` or a `std::vector`. It's to arrays what
> `std::string_view` is to strings, and close to C#'s `ReadOnlySpan<T>`. The
> functions return `s_defines` (a C array), and the conversion to a span
> happens implicitly, picking up the length from the array's type. Callers
> loop over it like any container:
>
> ```cpp
> for (const BuiltinDefine& define : BuiltinDefines()) { ... }
> ```
>
> Returning a span hides how the table is stored (the header doesn't even
> say how many entries there are) without copying it. It's safe here because
> the arrays live for the whole program. Returning a span of a *local*
> array would dangle, exactly like returning a `string_view` of a local
> string.

> **C++ note — `constexpr` tables:** `constexpr BuiltinDefine s_defines[]`
> is built by the compiler and stored in the executable's read-only data.
> There's no code that runs at startup to fill it. That works because every
> part is a *literal type*: `int`, `double`, and `std::string_view`
> (pointing at string literals, which also live for the whole program).
> A `std::string` member would not work in C++20 here, because it may
> allocate. The anonymous namespace keeps the arrays private to this file.
> [Step 0](00-setup.md) covers anonymous namespaces, and Step 4 covers
> `constexpr` in depth.

> **Why built-in constants are injected into every shader:** GLSL has no
> `#include`, and no standard library of constants. Without the prelude,
> every shader in the book would repeat `const float pi = 3.14159...;` and
> its own attribute location numbers, and the numbers in C++ and GLSL could
> drift apart. Alternatives OpenGlobe could have chosen:
>
> - *Uniforms* (`uniform float u_pi;`) cost a GL call per program and a
>   register on the GPU, and the compiler can't fold them into constant
>   expressions. They're for values that change.
> - *Asking the app to paste a header* is what the prelude automates.
>
> The prelude costs a few hundred bytes of text per compile and gives every
> shader the same vocabulary. The `og_` prefix ("OpenGlobe") keeps the names
> from colliding with yours.

### Checkpoint 1

Add the new API-agnostic files to `renderer/CMakeLists.txt`. The simplest,
unambiguous way is a separate `target_sources` block right after the
`add_library(arda_renderer ...)` call (you can also add the lines to the
`add_library` list itself; the effect is the same):

```cmake
# Step 2: shaders (API-agnostic)
target_sources(arda_renderer PRIVATE
    src/shaders/BuiltinConstants.cpp
    src/shaders/UniformCollection.cpp
    include/arda/renderer/Exceptions.h
    include/arda/renderer/shaders/ShaderProgram.h
    include/arda/renderer/shaders/ShaderVertexAttribute.h
    include/arda/renderer/shaders/Uniform.h
    include/arda/renderer/shaders/UniformCollection.h
    include/arda/renderer/vertexarray/VertexLocations.h
    src/Cleanable.h
    src/shaders/BuiltinConstants.h
)
```

Build:

```sh
cmake --build build/vs --config Debug --target arda_renderer
```

Headers are only compiled when a `.cpp` includes them, so this checks
`UniformCollection.h` (through `UniformCollection.cpp`), `Uniform.h` and
`VertexLocations.h`. `ShaderVertexAttribute.h` and `ShaderProgram.h` get
compiled in Part 13. Typical errors:

- *"`contains` is not a member of `std::unordered_map`"*: the compiler isn't
  in C++20 mode. The top-level `CMakeLists.txt` sets
  `CMAKE_CXX_STANDARD 20`; reconfigure.
- *"cannot convert `std::string_view` to `const std::string&`"* in
  `m_byName.find(name)`: `StringHash` is missing `using is_transparent =
  void;`, or the map's fourth template argument isn't `std::equal_to<>`.
- *"cannot open include file `shaders/BuiltinConstants.h`"*: the quotes
  version is resolved against `renderer/src` (`PRIVATE src` in CMake). Check
  the file is in `renderer/src/shaders/`.

---

## 9. `src/gl/shaders/GlslPrelude.h` and `.cpp`

The GL backend turns the tables from Part 8 into GLSL text.

### `src/gl/shaders/GlslPrelude.h`

```cpp
#pragma once

#include <string>

namespace arda::renderer::gl {

// "#version 330", then a #define for each built-in define and a const float
// for each built-in constant (ShaderObjectGL3x.cs, lines 22-43). Built once.
const std::string& GlslPrelude();

} // namespace arda::renderer::gl
```

### `src/gl/shaders/GlslPrelude.cpp`

```cpp
#include "gl/shaders/GlslPrelude.h"
#include "shaders/BuiltinConstants.h"

#include <format>

namespace arda::renderer::gl {

namespace {

std::string BuildGlslPrelude() {
    std::string prelude = "#version 330\n";
    for (const BuiltinDefine& define : BuiltinDefines()) {
        prelude += std::format("#define {} {}\n", define.name, define.value);
    }
    for (const BuiltinConstant& constant : BuiltinConstants()) {
        // 17 significant digits print any double exactly. The GLSL compiler
        // rounds it to the nearest float.
        prelude += std::format("const float {} = {:.17g};\n", constant.name, constant.value);
    }
    return prelude;
}

} // namespace

const std::string& GlslPrelude() {
    static const std::string prelude = BuildGlslPrelude();
    return prelude;
}

} // namespace arda::renderer::gl
```

The result, which every GL shader gets as string 0:

```glsl
#version 330
#define og_positionVertexLocation 0
#define og_normalVertexLocation 2
#define og_textureCoordinateVertexLocation 3
#define og_colorVertexLocation 4
#define og_positionHighVertexLocation 0
#define og_positionLowVertexLocation 1
const float og_E = 2.7182818284590451;
const float og_pi = 3.1415926535897931;
const float og_oneOverPi = 0.31830988618379069;
const float og_piOverTwo = 1.5707963267948966;
const float og_piOverThree = 1.0471975511965976;
const float og_piOverFour = 0.78539816339744828;
const float og_piOverSix = 0.52359877559829882;
const float og_threePiOver2 = 4.7123889803846897;
const float og_twoPi = 6.2831853071795862;
const float og_oneOverTwoPi = 0.15915494309189535;
const float og_radiansPerDegree = 0.017453292519943295;
const float og_maximumFloat = 3.4028234663852886e+38;
const float og_minimumFloat = -3.4028234663852886e+38;
```

OpenGlobe's prelude also appends `BuiltinFunctions.glsl`. The README defers
that to Chapters 5 and 7, which need its emulated-double and wide-line
functions.

<a id="cpp-format"></a>

> **C++ note — `std::format`:** C++20's `std::format` works like C#'s
> `string.Format` with `{}` placeholders: `std::format("#define {} {}\n",
> name, value)`. Format specs go after a colon, and `{:.17g}` means "general
> notation, 17 significant digits", the precision that round-trips any
> `double`. Unlike `printf`, the argument types are checked at compile time,
> and unlike C++ streams, it is **locale-independent** unless you ask for
> the locale with `{:L}`. That matters here. OpenGlobe passes
> `NumberFormatInfo.InvariantInfo` to every `ToString()`, because on a
> German Windows `Math.PI.ToString()` gives `3,14159...` and the shader fails
> to compile. `std::format` never does that. `std::format` needs Visual
> Studio 2019 16.10+, GCC 13+ or Clang 17+. On macOS, formatting floating
> point also needs a deployment target of macOS 13.3 or later.

<a id="cpp-function-local-static"></a>

> **C++ note — function-local `static` variables:** `static const
> std::string prelude = BuildGlslPrelude();` inside a function is created the
> first time the function runs, and lives until the program exits. Later
> calls skip the initialization and return the same string. Since C++11 the
> first initialization is also thread-safe: if two threads call
> `GlslPrelude()` at once, one builds the string and the other waits. It's
> the C++ equivalent of a C# `static readonly` field initialized by a static
> constructor, but lazy. Returning `const std::string&` avoids copying the
> string on every call, and is safe because the string outlives every
> caller.

---

## 10. `src/gl/shaders/ShaderObjectGL3x.h` and `.cpp`

One compiled stage (3.4.1). It's a private detail of the GL program, so it
has no public base class.

### `src/gl/shaders/ShaderObjectGL3x.h`

```cpp
#pragma once

#include "gl/GLHandle.h"

#include <string>
#include <string_view>

namespace arda::renderer::gl {

// One compiled shader stage (3.4.1). Only ShaderProgramGL3x uses it.
class ShaderObjectGL3x {
public:
    // shaderType is GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER.
    // Throws CouldNotCreateVideoCardResourceException with the compile log if
    // the source doesn't compile.
    ShaderObjectGL3x(GLenum shaderType, std::string_view source);

    GLuint Handle() const { return m_shader.Get(); }
    std::string CompileLog() const;

private:
    ShaderName m_shader;
};

} // namespace arda::renderer::gl
```

`ShaderName` is `GLHandle<ShaderDeleter>` from Step 0: it calls
`glDeleteShader` when the `ShaderObjectGL3x` is destroyed, and it's
move-only. The compiler therefore gives `ShaderObjectGL3x` a move
constructor and no copy constructor, without us writing either (the rule of
zero, see [Step 0](00-setup.md)). Part 13 relies on the move.

### `src/gl/shaders/ShaderObjectGL3x.cpp`

```cpp
#include "gl/shaders/ShaderObjectGL3x.h"
#include "gl/shaders/GlslPrelude.h"

#include <arda/renderer/Exceptions.h>

#include <stdexcept>

namespace arda::renderer::gl {

namespace {

// The prelude already starts with "#version 330", and a shader may contain
// only one #version directive. If the source has its own, comment it out
// rather than delete it, so the line numbers in compile errors don't change.
std::string CommentOutVersionDirective(std::string_view source) {
    std::string body(source);

    // Skip blank lines, so a raw string literal that starts with a newline works.
    const std::size_t first = body.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return body;
    }

    const std::string_view text = std::string_view(body).substr(first);
    if (text.starts_with("#version")) {
        if (!text.starts_with("#version 330")) {
            throw std::invalid_argument("Only GLSL version 330 is supported.");
        }
        body.insert(first, "//");
    }
    return body;
}

} // namespace

ShaderObjectGL3x::ShaderObjectGL3x(GLenum shaderType, std::string_view source)
    : m_shader(glCreateShader(shaderType)) {
    if (m_shader.Get() == 0) {
        throw CouldNotCreateVideoCardResourceException("glCreateShader failed");
    }

    const std::string& prelude = GlslPrelude();
    const std::string body = CommentOutVersionDirective(source);

    // Two strings: the prelude is string 0 and the app's source is string 1.
    const GLchar* const strings[] = {prelude.data(), body.data()};
    const GLint lengths[] = {static_cast<GLint>(prelude.size()), static_cast<GLint>(body.size())};
    glShaderSource(m_shader.Get(), 2, strings, lengths);
    glCompileShader(m_shader.Get());

    GLint status = GL_FALSE;
    glGetShaderiv(m_shader.Get(), GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        throw CouldNotCreateVideoCardResourceException(
            "Could not compile shader object. Compile log:\n\n" + CompileLog());
    }
}

std::string ShaderObjectGL3x::CompileLog() const {
    GLint length = 0;   // includes the null terminator
    glGetShaderiv(m_shader.Get(), GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetShaderInfoLog(m_shader.Get(), length, nullptr, log.data());
    log.pop_back();   // drop the null terminator
    return log;
}

} // namespace arda::renderer::gl
```

Step by step:

1. **`glCreateShader(shaderType)`** creates an empty shader object and
   returns its name, or 0 on failure (for example, no current context). The
   name goes straight into `m_shader`, so it's deleted even if a later line
   throws.
2. **`CommentOutVersionDirective`** turns `#version 330` into
   `//#version 330`, and rejects any other version.
3. **`glShaderSource(shader, 2, strings, lengths)`** hands GL two strings.
   GL copies them, so `body` can be destroyed after the call. Passing
   `lengths` means the strings don't need null terminators; passing
   `nullptr` instead would make GL read each one up to its `'\0'`.
4. **`glCompileShader`** compiles. It returns nothing.
5. **`glGetShaderiv(..., GL_COMPILE_STATUS, ...)`** asks whether it worked.
   If not, the info log says why.

<a id="gl-info-logs"></a>

> **OpenGL note — info logs and line numbers:** `GL_INFO_LOG_LENGTH`
> includes the null terminator, so a length of 0 or 1 means "no log". The
> log's format isn't standardized. On AMD a compile error looks like this:
>
> ```
> Could not compile shader object. Compile log:
>
> ERROR: 0:24: 'undeclared' : undeclared identifier
> ERROR: 0:24: '' : compilation terminated
> ERROR: 2 compilation errors.  No code generated.
> ```
>
> `0:24` is "string 0, line 24". AMD counts lines from the start of the
> prelude, which is 20 lines long, so the error is on line 4 of your source.
> NVIDIA reports `1(4)` instead: string 1 (your source), line 4. Either way,
> commenting out `#version` instead of deleting it keeps your own line
> numbers where you expect them. A successful compile can have a log too,
> with warnings; the program's `Log()` makes the link log available for the
> same reason.

<a id="cpp-string-buffer"></a>

> **C++ note — a `std::string` as an output buffer:** `std::string
> log(length, '\0')` creates a string of `length` null characters, and
> `log.data()` gives a writable `char*` (since C++17) for
> `glGetShaderInfoLog` to fill. That avoids a separate `std::vector<char>`
> and a copy. GL writes a terminator at the end, which `pop_back()` removes
> so the string's size is the real text length.

> **Why comment out `#version` instead of rejecting or deleting it:** the
> book's shaders don't have `#version`, but shaders pasted from elsewhere
> usually do, so accepting `#version 330` is friendly. Deleting the line
> would shift every line number in the error messages by one. Anything other
> than 330 is rejected, because the prelude has already committed to 330.

> **Why this skips leading whitespace, unlike the C#:** OpenGlobe only
> checks `source.StartsWith("#version")`, and its comment admits this
> doesn't follow the spec, which allows whitespace and comments before
> `#version`. In C++ you'll write shaders as raw string literals, and
> `R"(` followed by a line break puts a `\n` first. Without the skip,
> `R"(\n#version 330 ...` would slip through, GL would see two `#version`
> lines, and the compile error would be confusing. Comments before
> `#version` are still not handled, same as OpenGlobe.

> **C++ note — a constructor that throws:** if the constructor throws after
> `m_shader` was initialized, C++ destroys the members that were already
> constructed, so `m_shader`'s destructor calls `glDeleteShader`. The
> `ShaderObjectGL3x` destructor itself doesn't run, because the object never
> finished being constructed. This is why every GL name is wrapped in a
> `GLHandle` *member* immediately, rather than stored in a raw `GLuint` and
> deleted in a destructor. [Step 0](00-setup.md) covers RAII and member
> order.

---

## 11. `src/gl/shaders/UniformGL3x.h` and `.cpp`

This is 3.4.4's "delayed technique". OpenGlobe has 22 `Uniform*GL3x.cs`
classes, and they are identical except for the value type and one
`GL.Uniform*` line in `Clean()`. Here is one of them, trimmed:

```csharp
internal class UniformFloatVector3GL3x : Uniform<Vector3F>, ICleanable
{
    internal UniformFloatVector3GL3x(string name, int location, ICleanableObserver observer)
        : base(name, UniformType.FloatVector3)
    {
        _location = location;
        _dirty = true;
        _observer = observer;
        _observer.NotifyDirty(this);
    }

    public override Vector3F Value
    {
        set
        {
            if (!_dirty && (_value != value))
            {
                _dirty = true;
                _observer.NotifyDirty(this);
            }
            _value = value;
        }
        get { return _value; }
    }

    public void Clean()
    {
        GL.Uniform3(_location, _value.X, _value.Y, _value.Z);
        _dirty = false;
    }
    ...
}
```

In C++, the shared part becomes one class template, and the one line that
differs becomes an overloaded function, `UploadUniform`.

### `src/gl/shaders/UniformGL3x.h`

```cpp
#pragma once

#include "Cleanable.h"

#include <arda/core/geometry/Matrix.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/shaders/Uniform.h>

#include <glad/glad.h>

#include <string>
#include <utility>

namespace arda::renderer::gl {

// One overload per uniform value type. Each is the Clean() body of one of
// OpenGlobe's Uniform*GL3x.cs classes. They upload to the program that is
// currently bound with glUseProgram.
void UploadUniform(GLint location, int value);
void UploadUniform(GLint location, float value);
void UploadUniform(GLint location, bool value);

void UploadUniform(GLint location, const core::Vector2<float>& value);
void UploadUniform(GLint location, const core::Vector3<float>& value);
void UploadUniform(GLint location, const core::Vector4<float>& value);

void UploadUniform(GLint location, const core::Vector2<int>& value);
void UploadUniform(GLint location, const core::Vector3<int>& value);
void UploadUniform(GLint location, const core::Vector4<int>& value);

void UploadUniform(GLint location, const core::Vector2<bool>& value);
void UploadUniform(GLint location, const core::Vector3<bool>& value);
void UploadUniform(GLint location, const core::Vector4<bool>& value);

void UploadUniform(GLint location, const core::Matrix2<float>& value);
void UploadUniform(GLint location, const core::Matrix3<float>& value);
void UploadUniform(GLint location, const core::Matrix4<float>& value);
void UploadUniform(GLint location, const core::Matrix23<float>& value);
void UploadUniform(GLint location, const core::Matrix24<float>& value);
void UploadUniform(GLint location, const core::Matrix32<float>& value);
void UploadUniform(GLint location, const core::Matrix34<float>& value);
void UploadUniform(GLint location, const core::Matrix42<float>& value);
void UploadUniform(GLint location, const core::Matrix43<float>& value);

// A uniform in a GL program. SetValue only stores the value and tells the
// program it changed. The program calls Clean() just before a draw, which
// uploads it with glUniform* (3.4.4, the "delayed technique").
// This one template replaces OpenGlobe's 22 Uniform*GL3x.cs classes.
template <typename T>
class UniformGL3x final : public Uniform<T>, public ICleanable {
public:
    UniformGL3x(std::string name, UniformType type, GLint location, ICleanableObserver& observer)
        : Uniform<T>(std::move(name), type), m_location(location), m_observer(observer) {
        // GL sets every uniform to zero when the program links, and m_value
        // starts at zero too. OpenGlobe uploads it anyway on the first draw.
        m_observer.NotifyDirty(*this);
    }

    const T& Value() const override { return m_value; }

    void SetValue(const T& value) override {
        // Join the dirty list only once, and only if the value really changed.
        if (!m_dirty && m_value != value) {
            m_dirty = true;
            m_observer.NotifyDirty(*this);
        }
        m_value = value;
    }

    // The program must be bound (glUseProgram) when this is called.
    void Clean() override {
        UploadUniform(m_location, m_value);
        m_dirty = false;
    }

private:
    GLint m_location;
    T m_value{};
    bool m_dirty = true;
    ICleanableObserver& m_observer;
};

} // namespace arda::renderer::gl
```

### `src/gl/shaders/UniformGL3x.cpp`

```cpp
#include "gl/shaders/UniformGL3x.h"

namespace arda::renderer::gl {

namespace {
GLint BoolToInt(bool value) {
    return value ? 1 : 0;
}
} // namespace

// Scalars: UniformIntGL3x.cs (also used for samplers), UniformFloatGL3x.cs, UniformBoolGL3x.cs.
void UploadUniform(GLint location, int value) {
    glUniform1i(location, value);
}

void UploadUniform(GLint location, float value) {
    glUniform1f(location, value);
}

void UploadUniform(GLint location, bool value) {
    glUniform1i(location, BoolToInt(value));
}

// UniformFloatVector{2,3,4}GL3x.cs
void UploadUniform(GLint location, const core::Vector2<float>& v) {
    glUniform2f(location, v.X(), v.Y());
}

void UploadUniform(GLint location, const core::Vector3<float>& v) {
    glUniform3f(location, v.X(), v.Y(), v.Z());
}

void UploadUniform(GLint location, const core::Vector4<float>& v) {
    glUniform4f(location, v.X(), v.Y(), v.Z(), v.W());
}

// UniformIntVector{2,3,4}GL3x.cs
void UploadUniform(GLint location, const core::Vector2<int>& v) {
    glUniform2i(location, v.X(), v.Y());
}

void UploadUniform(GLint location, const core::Vector3<int>& v) {
    glUniform3i(location, v.X(), v.Y(), v.Z());
}

void UploadUniform(GLint location, const core::Vector4<int>& v) {
    glUniform4i(location, v.X(), v.Y(), v.Z(), v.W());
}

// UniformBoolVector{2,3,4}GL3x.cs: GL has no glUniform*b, so bools go up as ints.
void UploadUniform(GLint location, const core::Vector2<bool>& v) {
    glUniform2i(location, BoolToInt(v.X()), BoolToInt(v.Y()));
}

void UploadUniform(GLint location, const core::Vector3<bool>& v) {
    glUniform3i(location, BoolToInt(v.X()), BoolToInt(v.Y()), BoolToInt(v.Z()));
}

void UploadUniform(GLint location, const core::Vector4<bool>& v) {
    glUniform4i(location, BoolToInt(v.X()), BoolToInt(v.Y()), BoolToInt(v.Z()), BoolToInt(v.W()));
}

// UniformFloatMatrix*GL3x.cs. Every matrix is stored column-major, which is
// what GL expects, so transpose is GL_FALSE.
void UploadUniform(GLint location, const core::Matrix2<float>& m) {
    glUniformMatrix2fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix3<float>& m) {
    glUniformMatrix3fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix4<float>& m) {
    glUniformMatrix4fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix23<float>& m) {
    glUniformMatrix2x3fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix24<float>& m) {
    glUniformMatrix2x4fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix32<float>& m) {
    glUniformMatrix3x2fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix34<float>& m) {
    glUniformMatrix3x4fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix42<float>& m) {
    glUniformMatrix4x2fv(location, 1, GL_FALSE, m.Data());
}

void UploadUniform(GLint location, const core::Matrix43<float>& m) {
    glUniformMatrix4x3fv(location, 1, GL_FALSE, m.Data());
}

} // namespace arda::renderer::gl
```

<a id="gl-uniform-calls"></a>

> **OpenGL note — the `glUniform*` family:** the suffix encodes the
> component count and type: `glUniform3f` is three floats, `glUniform2i` two
> ints. The matrix calls take `(location, count, transpose, pointer)`:
> `count` is the number of matrices (1, since arrays aren't supported), and
> `transpose = GL_FALSE` says the data is already column-major. Passing
> `GL_TRUE` would accept row-major data, which is how you'd upload a
> row-major matrix library's data. The `glUniformMatrix2x3fv` name is
> *columns × rows*, matching `mat2x3` and `Matrix23`. The call must match
> the uniform's declared type: `glUniform1f` on an `int` uniform is a
> `GL_INVALID_OPERATION` error and changes nothing. That's one reason the
> uniform classes are created from the reflected type instead of from what
> the app thinks the type is. A location of `-1` is silently ignored, and
> **every `glUniform*` call writes to the program bound with
> `glUseProgram`**. Calling `Clean()` without binding writes to whatever
> program happens to be bound, or raises `GL_INVALID_OPERATION` if none is.

<a id="cpp-dirty-flag"></a>

> **Why the dirty flag and the dirty list (3.4.4):** there are three ways to
> get a uniform's value to GL.
>
> 1. *Immediately, in `SetValue`.* In GL 3.3 that means `glUseProgram`
>    first, which changes the context's bound program behind the renderer's
>    back. Worse, the app may set uniforms when no context is current, or
>    while a *different* context is current. The book rejects this.
> 2. *Delayed: upload every uniform before every draw.* Always correct, but a
>    program with 20 uniforms makes 20 GL calls per draw even when nothing
>    changed.
> 3. *Delayed, dirty only* (OpenGlobe, and arda). `SetValue` stores the value
>    and, if it changed, adds the uniform to its program's dirty list. Just
>    before a draw, with the program bound, the program calls `Clean()` on
>    each uniform on the list and clears it (Step 3). Uniforms that didn't
>    change cost nothing.
>
> The `m_dirty` flag makes sure a uniform joins the list at most once
> between draws, however many times it's set. The value comparison makes
> setting the same matrix every frame free: Step 4's automatic uniforms do
> exactly that. And the constructor starts each uniform dirty, so the first
> draw uploads everything once. That's redundant for GL, which already
> zeroed the uniforms at link time, but it's what OpenGlobe does and it
> keeps GL's state and `m_value` provably in step.
>
> The pattern is an *observer*: each uniform knows an `ICleanableObserver`
> (its program) and notifies it; the program doesn't have to poll. Step 3's
> vertex arrays and Step 6's framebuffers use the same idea with their own
> dirty flags.

> **Why one template instead of 22 classes:** the C# can't use one generic
> class because C# generics can't call `GL.Uniform3(x, y, z)` on an arbitrary
> `T`; the call must be resolved when the class is compiled. A C++ template
> is compiled separately for each `T`, so `UploadUniform(m_location,
> m_value)` inside the template picks the right overload for each
> instantiation at compile time. Adding a uniform type means one new
> `UploadUniform` overload and one `case` in `CreateUniform` (Part 13).

<a id="cpp-overloads-vs-specialization"></a>

> **C++ note — overloads vs template specialization vs `if constexpr`:**
> there are three ways to make a template do something different per type.
>
> ```cpp
> // 1. Overloading (used here): ordinary functions, one per type.
> void UploadUniform(GLint location, float value);
> void UploadUniform(GLint location, const core::Vector3<float>& value);
>
> // 2. Explicit specialization of a member function.
> template <> void UniformGL3x<float>::Clean() { glUniform1f(m_location, m_value); }
>
> // 3. if constexpr: one function, branches chosen at compile time.
> void Clean() override {
>     if constexpr (std::is_same_v<T, float>) {
>         glUniform1f(m_location, m_value);
>     } else if constexpr (std::is_same_v<T, core::Vector3<float>>) {
>         glUniform3f(m_location, m_value.X(), m_value.Y(), m_value.Z());
>     } // ... 20 more branches
> }
> ```
>
> `if constexpr` (C++17) discards the branches whose condition is false, so
> code that wouldn't compile for this `T` (like `m_value.X()` on a `float`)
> is never compiled. It's the right tool when the branches share code.
> Here they share nothing, and the overloads have two advantages: they live
> in a `.cpp`, so the GL calls aren't compiled into every file that includes
> the header, and a missing type is a clear error ("no matching overload
> for `UploadUniform(GLint, const Vector3<double>&)`") at the point where
> `UniformGL3x<Vector3<double>>` is used. Specialization (option 2) works
> too, but must be written outside the class, and forgetting one only fails
> at link time.

<a id="cpp-two-phase-lookup"></a>

> **C++ note — why the overloads are declared before the template:** inside
> a template, `UploadUniform(m_location, m_value)` depends on `T`, so the
> compiler resolves it when the template is *instantiated*. But it only
> looks in two places: the functions declared *before the template's
> definition* (ordinary lookup), and the namespaces of the argument types
> (argument-dependent lookup). The value types live in `arda::core` and
> `UploadUniform` lives in `arda::renderer::gl`, so argument-dependent
> lookup can't find it. The overloads must therefore be declared above the
> class template, which is why they're at the top of this header. Move them
> below and you get "UploadUniform: identifier not found" (MSVC in
> conformance mode, GCC, Clang).

<a id="cpp-multiple-interfaces"></a>

> **C++ note — deriving from a class and an interface:** `class UniformGL3x
> final : public Uniform<T>, public ICleanable` is the C++ spelling of C#'s
> `class UniformFloatVector3GL3x : Uniform<Vector3F>, ICleanable`. C++ allows
> any number of base classes, and "interface" is just a convention (Part 7).
> A `UniformGL3x<T>` object can be used as a `UniformBase&` (by the
> collection), a `Uniform<T>&` (by the app) and an `ICleanable&` (by the dirty
> list), and each is the same object. `m_observer.NotifyDirty(*this)`
> converts `*this` to `ICleanable&` automatically. It's safe inside the
> constructor, because base classes are fully constructed before the
> constructor body runs.

Other details in the template:

- **`T m_value{}`** *value-initializes* the value: numbers become 0, and a
  class whose default constructor is `= default` has its members zeroed. So
  `Vector3<float>` starts at (0, 0, 0) even though its members have no
  initializers, and a sampler's `int` starts at texture unit 0. With `T
  m_value;` (no braces) a `float` or `Vector3<float>` would contain garbage.
- **`m_value != value`** works because of the defaulted `operator==` from
  Part 1: C++20 rewrites `!=` as `!(==)`.
- **`ICleanableObserver& m_observer`** is a reference member: the uniform
  can't exist without its program, and the program outlives its uniforms
  because it owns them. See the reference members note in
  [Step 0](00-setup.md).
- **`final`** says nothing derives from `UniformGL3x`. It also lets the
  compiler call the virtual functions directly, without going through the
  vtable, when it knows it has a `UniformGL3x<T>`.
- **`Vector2<bool>` etc.** replace OpenGlobe's separate `Vector2B`,
  `Vector3B`, `Vector4B` types. A C++ template can be instantiated with
  `bool`; member functions like `Magnitude()` that make no sense for `bool`
  are only compiled if someone calls them.

---

## 12. `TypeConverterGL3x` additions

Reflection reports types as GL enums (`GL_FLOAT_VEC3`). These two functions
turn them into arda's enums. They are the reverse direction of Step 1's
`ToGL` functions, which is why they're named after their *result* type.

### `src/gl/TypeConverterGL3x.h`

Add two includes next to the existing ones, and two declarations inside the
namespace:

```cpp
#include <arda/renderer/shaders/ShaderVertexAttribute.h>
#include <arda/renderer/shaders/Uniform.h>
```

```cpp
// GL type enums reported by reflection -> arda's enums.
ShaderVertexAttributeType ToShaderVertexAttributeType(GLenum type);
UniformType ToUniformType(GLenum type);
```

### `src/gl/TypeConverterGL3x.cpp`

Add `#include <format>` to the includes, and these two functions inside the
namespace:

```cpp
// TypeConverterGL3x.cs, To(ActiveAttribType)
ShaderVertexAttributeType ToShaderVertexAttributeType(GLenum type) {
    switch (type) {
    case GL_FLOAT:      return ShaderVertexAttributeType::Float;
    case GL_FLOAT_VEC2: return ShaderVertexAttributeType::FloatVector2;
    case GL_FLOAT_VEC3: return ShaderVertexAttributeType::FloatVector3;
    case GL_FLOAT_VEC4: return ShaderVertexAttributeType::FloatVector4;
    case GL_FLOAT_MAT2: return ShaderVertexAttributeType::FloatMatrix22;
    case GL_FLOAT_MAT3: return ShaderVertexAttributeType::FloatMatrix33;
    case GL_FLOAT_MAT4: return ShaderVertexAttributeType::FloatMatrix44;
    case GL_INT:        return ShaderVertexAttributeType::Int;
    case GL_INT_VEC2:   return ShaderVertexAttributeType::IntVector2;
    case GL_INT_VEC3:   return ShaderVertexAttributeType::IntVector3;
    case GL_INT_VEC4:   return ShaderVertexAttributeType::IntVector4;
    }
    throw std::invalid_argument(std::format("Unsupported vertex attribute type 0x{:X}", type));
}

// TypeConverterGL3x.cs, To(ActiveUniformType)
UniformType ToUniformType(GLenum type) {
    switch (type) {
    case GL_INT:                               return UniformType::Int;
    case GL_FLOAT:                             return UniformType::Float;
    case GL_FLOAT_VEC2:                        return UniformType::FloatVector2;
    case GL_FLOAT_VEC3:                        return UniformType::FloatVector3;
    case GL_FLOAT_VEC4:                        return UniformType::FloatVector4;
    case GL_INT_VEC2:                          return UniformType::IntVector2;
    case GL_INT_VEC3:                          return UniformType::IntVector3;
    case GL_INT_VEC4:                          return UniformType::IntVector4;
    case GL_BOOL:                              return UniformType::Bool;
    case GL_BOOL_VEC2:                         return UniformType::BoolVector2;
    case GL_BOOL_VEC3:                         return UniformType::BoolVector3;
    case GL_BOOL_VEC4:                         return UniformType::BoolVector4;
    case GL_FLOAT_MAT2:                        return UniformType::FloatMatrix22;
    case GL_FLOAT_MAT3:                        return UniformType::FloatMatrix33;
    case GL_FLOAT_MAT4:                        return UniformType::FloatMatrix44;
    case GL_SAMPLER_1D:                        return UniformType::Sampler1D;
    case GL_SAMPLER_2D:                        return UniformType::Sampler2D;
    case GL_SAMPLER_2D_RECT:                   return UniformType::Sampler2DRectangle;
    case GL_SAMPLER_2D_RECT_SHADOW:            return UniformType::Sampler2DRectangleShadow;
    case GL_SAMPLER_3D:                        return UniformType::Sampler3D;
    case GL_SAMPLER_CUBE:                      return UniformType::SamplerCube;
    case GL_SAMPLER_1D_SHADOW:                 return UniformType::Sampler1DShadow;
    case GL_SAMPLER_2D_SHADOW:                 return UniformType::Sampler2DShadow;
    case GL_FLOAT_MAT2x3:                      return UniformType::FloatMatrix23;
    case GL_FLOAT_MAT2x4:                      return UniformType::FloatMatrix24;
    case GL_FLOAT_MAT3x2:                      return UniformType::FloatMatrix32;
    case GL_FLOAT_MAT3x4:                      return UniformType::FloatMatrix34;
    case GL_FLOAT_MAT4x2:                      return UniformType::FloatMatrix42;
    case GL_FLOAT_MAT4x3:                      return UniformType::FloatMatrix43;
    case GL_SAMPLER_1D_ARRAY:                  return UniformType::Sampler1DArray;
    case GL_SAMPLER_2D_ARRAY:                  return UniformType::Sampler2DArray;
    case GL_SAMPLER_1D_ARRAY_SHADOW:           return UniformType::Sampler1DArrayShadow;
    case GL_SAMPLER_2D_ARRAY_SHADOW:           return UniformType::Sampler2DArrayShadow;
    case GL_SAMPLER_CUBE_SHADOW:               return UniformType::SamplerCubeShadow;
    case GL_INT_SAMPLER_1D:                    return UniformType::IntSampler1D;
    case GL_INT_SAMPLER_2D:                    return UniformType::IntSampler2D;
    case GL_INT_SAMPLER_2D_RECT:               return UniformType::IntSampler2DRectangle;
    case GL_INT_SAMPLER_3D:                    return UniformType::IntSampler3D;
    case GL_INT_SAMPLER_CUBE:                  return UniformType::IntSamplerCube;
    case GL_INT_SAMPLER_1D_ARRAY:              return UniformType::IntSampler1DArray;
    case GL_INT_SAMPLER_2D_ARRAY:              return UniformType::IntSampler2DArray;
    case GL_UNSIGNED_INT_SAMPLER_1D:           return UniformType::UnsignedIntSampler1D;
    case GL_UNSIGNED_INT_SAMPLER_2D:           return UniformType::UnsignedIntSampler2D;
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:      return UniformType::UnsignedIntSampler2DRectangle;
    case GL_UNSIGNED_INT_SAMPLER_3D:           return UniformType::UnsignedIntSampler3D;
    case GL_UNSIGNED_INT_SAMPLER_CUBE:         return UniformType::UnsignedIntSamplerCube;
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:     return UniformType::UnsignedIntSampler1DArray;
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:     return UniformType::UnsignedIntSampler2DArray;
    }
    // For example uint, uvec*, samplerBuffer or sampler2DMS: GL 3.3 has them, OpenGlobe doesn't.
    throw std::invalid_argument(std::format("Unsupported uniform type 0x{:X}", type));
}
```

These switch on a `GLenum`, which is a plain `unsigned int`, not an enum.
The compiler can't warn about missing cases the way it does for a `switch`
on an `enum class` ([Step 1](01-state-management.md)), so the throw after
the switch is the only safety net. The hex value in the message
(`0x8DC8`, say) is what you'd look up in `glad.h` to see which type it was.

The C# converter's list has one hole: it has no case for
`UnsignedIntSampler2DArray`, although `CreateUniform` accepts it, so a
`usampler2DArray` would throw in OpenGlobe. It's included here.

---

## 13. `src/gl/shaders/ShaderProgramGL3x.h` and `.cpp`

The GL program: compile the stages, link, check, reflect.

### `src/gl/shaders/ShaderProgramGL3x.h`

```cpp
#pragma once

#include "Cleanable.h"
#include "gl/GLHandle.h"
#include "gl/shaders/ShaderObjectGL3x.h"

#include <arda/renderer/shaders/ShaderProgram.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arda::renderer::gl {

class ShaderProgramGL3x final : public ShaderProgram, public ICleanableObserver {
public:
    // An empty geometryShaderSource means there is no geometry shader.
    // Throws CouldNotCreateVideoCardResourceException if a stage doesn't
    // compile or the program doesn't link.
    ShaderProgramGL3x(std::string_view vertexShaderSource,
                      std::string_view geometryShaderSource,
                      std::string_view fragmentShaderSource);

    std::string Log() const override;
    int FragmentOutputLocation(std::string_view name) const override;

    // ICleanableObserver: a uniform's value changed since the last draw.
    void NotifyDirty(ICleanable& value) override { m_dirtyUniforms.push_back(&value); }

    GLuint Handle() const { return m_program.Get(); }
    void Bind() const { glUseProgram(m_program.Get()); }

    // Step 3 adds: void Clean(Context&, const DrawState&, const SceneState&);

private:
    void FindVertexAttributes();
    void FindUniforms();
    std::unique_ptr<UniformBase> CreateUniform(std::string name, GLint location, GLenum type);

    // Declared in the order they are created. Members are destroyed in the
    // reverse order, so the program is deleted before its shader objects.
    ShaderObjectGL3x m_vertexShader;
    std::optional<ShaderObjectGL3x> m_geometryShader;
    ShaderObjectGL3x m_fragmentShader;
    ProgramName m_program;

    // Uniforms waiting to be uploaded. They are owned by m_uniforms (in the
    // base class), which lives as long as this program does.
    std::vector<ICleanable*> m_dirtyUniforms;
};

} // namespace arda::renderer::gl
```

The class is both the public `ShaderProgram` and the uniforms'
`ICleanableObserver`, exactly like the C#. `Bind()` and `Handle()` are for
the GL context in Step 3, which will bind the program and then call
`Clean(...)` to upload the dirty uniforms.

<a id="cpp-optional"></a>

> **C++ note — `std::optional`:** `std::optional<ShaderObjectGL3x>` either
> holds a `ShaderObjectGL3x` or holds nothing. It's like C#'s `Nullable<T>`
> (`int?`), but for any type, and the value is stored *inside* the optional,
> not on the heap. The C# code uses a reference that may be `null`
> (`_geometryShader`); the C++ equivalent would be a
> `std::unique_ptr<ShaderObjectGL3x>` and a heap allocation, but `optional`
> says "maybe a value" more directly. Use it like a pointer:
>
> ```cpp
> if (m_geometryShader) {                        // has a value?
>     glAttachShader(program, m_geometryShader->Handle());   // -> reaches the value
> }
> ```
>
> `std::nullopt` is the empty value, and a function returning
> `std::optional<T>` can `return T(...)` directly, as
> `CompileOptionalShader` does below. Reading an empty optional with `*` or
> `->` is undefined behavior; `.value()` throws `std::bad_optional_access`
> instead.

### `src/gl/shaders/ShaderProgramGL3x.cpp`

```cpp
#include "gl/shaders/ShaderProgramGL3x.h"
#include "gl/shaders/UniformGL3x.h"
#include "gl/TypeConverterGL3x.h"

#include <arda/core/geometry/Matrix.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/Exceptions.h>

#include <stdexcept>
#include <utility>

namespace arda::renderer::gl {

namespace {

// Returns an empty optional when there is no source, so the geometry shader
// can be created in the member initializer list like the other two stages.
std::optional<ShaderObjectGL3x> CompileOptionalShader(GLenum shaderType, std::string_view source) {
    if (source.empty()) {
        return std::nullopt;
    }
    return ShaderObjectGL3x(shaderType, source);
}

// ShaderProgramGL3x.cs, CorrectUniformName. GL reports an array uniform as
// "name[0]". OpenGlobe's comment blames ATI, but the GL 3.3 spec requires it.
std::string CorrectUniformName(std::string name) {
    if (name.ends_with("[0]")) {
        name.resize(name.size() - 3);
    }
    return name;
}

// Reads a name that glGetActiveAttrib or glGetActiveUniform wrote into buffer.
std::string NameFromBuffer(const std::string& buffer, GLsizei length) {
    return std::string(buffer.data(), static_cast<std::size_t>(length));
}

template <typename T>
std::unique_ptr<UniformBase> MakeUniform(std::string name, UniformType type, GLint location,
                                         ICleanableObserver& observer) {
    return std::make_unique<UniformGL3x<T>>(std::move(name), type, location, observer);
}

} // namespace

ShaderProgramGL3x::ShaderProgramGL3x(
    std::string_view vertexShaderSource,
    std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource)
    : m_vertexShader(GL_VERTEX_SHADER, vertexShaderSource),
      m_geometryShader(CompileOptionalShader(GL_GEOMETRY_SHADER, geometryShaderSource)),
      m_fragmentShader(GL_FRAGMENT_SHADER, fragmentShaderSource),
      m_program(glCreateProgram()) {
    const GLuint program = m_program.Get();
    if (program == 0) {
        throw CouldNotCreateVideoCardResourceException("glCreateProgram failed");
    }

    glAttachShader(program, m_vertexShader.Handle());
    if (m_geometryShader) {
        glAttachShader(program, m_geometryShader->Handle());
    }
    glAttachShader(program, m_fragmentShader.Handle());
    glLinkProgram(program);

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        throw CouldNotCreateVideoCardResourceException(
            "Could not link shader program. Link log:\n\n" + Log());
    }

    FindVertexAttributes();
    FindUniforms();
    // Step 4 adds: InitializeAutomaticUniforms(device);
}

std::string ShaderProgramGL3x::Log() const {
    GLint length = 0;   // includes the null terminator
    glGetProgramiv(m_program.Get(), GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetProgramInfoLog(m_program.Get(), length, nullptr, log.data());
    log.pop_back();   // drop the null terminator
    return log;
}

// FragmentOutputsGL3x.cs
int ShaderProgramGL3x::FragmentOutputLocation(std::string_view name) const {
    const std::string nameString(name);   // GL needs a null-terminated string
    const GLint location = glGetFragDataLocation(m_program.Get(), nameString.c_str());
    if (location == -1) {
        throw std::out_of_range("No fragment output named '" + nameString + "'");
    }
    return location;
}

void ShaderProgramGL3x::FindVertexAttributes() {
    const GLuint program = m_program.Get();

    GLint count = 0;
    GLint maxLength = 0;   // longest name, including the null terminator
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);

    std::string buffer(static_cast<std::size_t>(maxLength), '\0');
    for (GLint i = 0; i < count; ++i) {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(program, static_cast<GLuint>(i), maxLength, &length, &size, &type, buffer.data());
        std::string name = NameFromBuffer(buffer, length);

        // Built-in inputs such as gl_VertexID have no location.
        if (name.starts_with("gl_")) {
            continue;
        }

        const GLint location = glGetAttribLocation(program, name.c_str());
        m_vertexAttributes.Add({std::move(name), location, ToShaderVertexAttributeType(type), size});
    }
}

void ShaderProgramGL3x::FindUniforms() {
    const GLuint program = m_program.Get();

    GLint count = 0;
    GLint maxLength = 0;   // longest name, including the null terminator
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    std::string buffer(static_cast<std::size_t>(maxLength), '\0');
    for (GLint i = 0; i < count; ++i) {
        const auto index = static_cast<GLuint>(i);
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(program, index, maxLength, &length, &size, &type, buffer.data());
        std::string name = CorrectUniformName(NameFromBuffer(buffer, length));

        // Built-in uniforms such as gl_DepthRange have no location.
        if (name.starts_with("gl_")) {
            continue;
        }

        // Uniforms inside a named uniform block are set through a uniform
        // buffer, not glUniform*. OpenGlobe collects those separately; the
        // book doesn't use them, so they are skipped here.
        GLint blockIndex = -1;
        glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
        if (blockIndex != -1) {
            continue;
        }

        if (size != 1) {
            throw std::runtime_error("Uniform arrays are not supported: '" + name + "'");
        }

        const GLint location = glGetUniformLocation(program, name.c_str());
        m_uniforms.Add(CreateUniform(std::move(name), location, type));
    }
}

// ShaderProgramGL3x.cs, CreateUniform. First turn the GL type into arda's
// UniformType, then pick the C++ value type for it.
std::unique_ptr<UniformBase> ShaderProgramGL3x::CreateUniform(std::string name, GLint location, GLenum type) {
    using namespace core;

    const UniformType uniformType = ToUniformType(type);   // throws for types OpenGlobe doesn't support
    ICleanableObserver& observer = *this;

    switch (uniformType) {
    case UniformType::Int:           return MakeUniform<int>(std::move(name), uniformType, location, observer);
    case UniformType::Float:         return MakeUniform<float>(std::move(name), uniformType, location, observer);
    case UniformType::FloatVector2:  return MakeUniform<Vector2<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatVector3:  return MakeUniform<Vector3<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatVector4:  return MakeUniform<Vector4<float>>(std::move(name), uniformType, location, observer);
    case UniformType::IntVector2:    return MakeUniform<Vector2<int>>(std::move(name), uniformType, location, observer);
    case UniformType::IntVector3:    return MakeUniform<Vector3<int>>(std::move(name), uniformType, location, observer);
    case UniformType::IntVector4:    return MakeUniform<Vector4<int>>(std::move(name), uniformType, location, observer);
    case UniformType::Bool:          return MakeUniform<bool>(std::move(name), uniformType, location, observer);
    case UniformType::BoolVector2:   return MakeUniform<Vector2<bool>>(std::move(name), uniformType, location, observer);
    case UniformType::BoolVector3:   return MakeUniform<Vector3<bool>>(std::move(name), uniformType, location, observer);
    case UniformType::BoolVector4:   return MakeUniform<Vector4<bool>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix22: return MakeUniform<Matrix2<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix33: return MakeUniform<Matrix3<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix44: return MakeUniform<Matrix4<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix23: return MakeUniform<Matrix23<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix24: return MakeUniform<Matrix24<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix32: return MakeUniform<Matrix32<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix34: return MakeUniform<Matrix34<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix42: return MakeUniform<Matrix42<float>>(std::move(name), uniformType, location, observer);
    case UniformType::FloatMatrix43: return MakeUniform<Matrix43<float>>(std::move(name), uniformType, location, observer);

    // A sampler uniform holds the index of the texture unit to read from, so it is an int.
    case UniformType::Sampler1D:
    case UniformType::Sampler2D:
    case UniformType::Sampler2DRectangle:
    case UniformType::Sampler2DRectangleShadow:
    case UniformType::Sampler3D:
    case UniformType::SamplerCube:
    case UniformType::Sampler1DShadow:
    case UniformType::Sampler2DShadow:
    case UniformType::Sampler1DArray:
    case UniformType::Sampler2DArray:
    case UniformType::Sampler1DArrayShadow:
    case UniformType::Sampler2DArrayShadow:
    case UniformType::SamplerCubeShadow:
    case UniformType::IntSampler1D:
    case UniformType::IntSampler2D:
    case UniformType::IntSampler2DRectangle:
    case UniformType::IntSampler3D:
    case UniformType::IntSamplerCube:
    case UniformType::IntSampler1DArray:
    case UniformType::IntSampler2DArray:
    case UniformType::UnsignedIntSampler1D:
    case UniformType::UnsignedIntSampler2D:
    case UniformType::UnsignedIntSampler2DRectangle:
    case UniformType::UnsignedIntSampler3D:
    case UniformType::UnsignedIntSamplerCube:
    case UniformType::UnsignedIntSampler1DArray:
    case UniformType::UnsignedIntSampler2DArray:
        return MakeUniform<int>(std::move(name), uniformType, location, observer);
    }

    // Only reached if uniformType holds a value that isn't one of the enumerators.
    throw std::invalid_argument("CreateUniform: invalid UniformType");
}

} // namespace arda::renderer::gl
```

### The constructor, step by step

1. **The member initializer list compiles the stages**, in declaration
   order: vertex, geometry (if any), fragment. If one fails,
   `ShaderObjectGL3x` throws, the stages already compiled are destroyed
   (deleting their GL shaders), and the exception leaves
   `CreateShaderProgram`. The order matches the C#.
2. **`glCreateProgram()`** creates an empty program, stored in
   `m_program` (a `ProgramName`, so `glDeleteProgram` runs if anything
   below throws).
3. **`glAttachShader`** adds each compiled stage.
4. **`glLinkProgram`** links them. As with compiling, the result is only
   available through `glGetProgramiv(..., GL_LINK_STATUS, ...)`.
5. **Reflection** fills the base class's `m_vertexAttributes` and
   `m_uniforms`. Each uniform's constructor calls `NotifyDirty(*this)`, so
   after construction `m_dirtyUniforms` lists every uniform, ready for the
   first draw.

> **OpenGL note — what the program keeps after linking:** a linked program
> holds its own compiled, linked code. The shader objects could be detached
> and deleted right after `glLinkProgram`, and a lot of GL code does that to
> free memory. OpenGlobe keeps them for the program's lifetime, and so does
> arda; it costs a little memory and nothing else. If you delete a shader
> object that is still attached, GL only *flags* it for deletion and deletes
> it when the program is deleted. That's why the member order (program
> destroyed first, then the shaders) doesn't matter for correctness.

> **Why the geometry shader is created in the initializer list:** the
> earlier version of this guide called `m_geometryShader.emplace(...)` in
> the constructor body, which compiled the fragment shader before the
> geometry shader. Compiling through `CompileOptionalShader` in the
> initializer list keeps OpenGlobe's order, so the *first* error reported is
> the same one the C# would report, and all three stages are created the
> same way.

### Reflection details

- **`GL_ACTIVE_ATTRIBUTE_MAX_LENGTH` / `GL_ACTIVE_UNIFORM_MAX_LENGTH`** give
  a buffer size that fits every name. `glGetActive*` writes the name and its
  length (without the terminator) into `length`, and `NameFromBuffer` copies
  exactly that many characters.
- **`size`** is the array length: 1 for a non-array. Attributes keep it in
  `ShaderVertexAttribute::length`. Uniforms throw if it isn't 1, like
  OpenGlobe's `NotSupportedException`.
- **`glGetActiveUniformsiv(..., GL_UNIFORM_BLOCK_INDEX, ...)`** is -1 for
  ordinary uniforms. Members of a `uniform Block { ... };` have no
  `glUniform*` location and are skipped. `FindUniformBlocks` isn't ported
  (see the README's "Not needed for Chapter 3").
- **Locations come from `glGetUniformLocation(name)`**, never from the loop
  index `i`. They're unrelated, even though for small programs they are
  sometimes equal by coincidence.

> **Why reflect instead of trusting the app:** the app could declare its
> uniforms in C++ ("`u_color` is a `vec3`"), but then C++ and GLSL could
> disagree, and GL would silently ignore mismatched `glUniform*` calls.
> Reflection makes the shader the single source of truth: the uniform
> classes are created from what the *linker* says, with the right upload
> call, and `Get<T>` catches a C++ side that disagrees.

`CreateUniform` does in two steps what the C# does in one big switch on
the GL type: `ToUniformType` (Part 12) maps the GL enum to `UniformType`,
then a `switch` on `UniformType` picks the C++ value type. Switching on
arda's `enum class` means the compiler warns if a new `UniformType` is added
without a case here. `MakeUniform<T>` is a small function template that
saves repeating `std::make_unique<UniformGL3x<T>>(...)` 22 times; `T` is
given explicitly, because it can't be deduced from the arguments.

> **C++ note — `using namespace` inside a function:** `using namespace
> core;` makes `Vector2`, `Matrix4` and friends usable without `core::` for
> the rest of this function only. That's a reasonable use: the scope is
> small and the names are obvious. Never write `using namespace` at file
> scope in a *header*, because it leaks into every file that includes it.
> In a `.cpp`, prefer `using core::Vector3;` declarations (as the tests do)
> when only a few names are needed.

> **C++ note — `case` labels that fall through:** the sampler cases have no
> `return` of their own, so control "falls through" to the next label until
> it reaches `return MakeUniform<int>(...)`. That's how several values share
> one branch, like C#'s stacked `case` labels. Falling through from a case
> *with* statements is almost always a bug, and compilers warn about it
> (`[[fallthrough]];` marks an intentional one).

### Checkpoint 2

Add the GL files to `renderer/CMakeLists.txt`, inside `if(ARDA_RENDERER_GL)`
after the existing `target_sources` call (or add them to it):

```cmake
    # Step 2: shaders (GL backend)
    target_sources(arda_renderer PRIVATE
        src/gl/shaders/GlslPrelude.cpp
        src/gl/shaders/ShaderObjectGL3x.cpp
        src/gl/shaders/ShaderProgramGL3x.cpp
        src/gl/shaders/UniformGL3x.cpp
        src/gl/shaders/GlslPrelude.h
        src/gl/shaders/ShaderObjectGL3x.h
        src/gl/shaders/ShaderProgramGL3x.h
        src/gl/shaders/UniformGL3x.h
    )
```

`src/gl/TypeConverterGL3x.cpp` is already listed from Step 1. Build:

```sh
cmake --build build/vs --config Debug --target arda_renderer
```

Everything but the `Device` entry point now compiles. Typical errors:

- *"`UploadUniform`: no matching overloaded function"* for some `T`: the
  `case` in `CreateUniform` and the `UploadUniform` overloads disagree, or
  a vector is missing `operator==` (Part 1), which shows up as an error on
  `m_value != value` instead.
- *"`sqrt` is not a member of `std`"* inside `Vector4.h`: Part 1c.
- *"cannot instantiate abstract class"* on `UniformGL3x<T>`: a signature
  doesn't match `Uniform<T>` exactly. `Value()` must be `const T& Value()
  const`, and `SetValue` must take `const T&`. The `override` keyword turns
  this into a clearer error at the function itself.
- *"`GL_SAMPLER_2D_RECT` undeclared"*: glad was generated for a GL version
  below 3.1. vcpkg's glad has everything up to 3.3.

---

## 14. `Device` and `DeviceGL3x` additions

Now the entry point. `Device` gets the public, non-virtual
`CreateShaderProgram` overloads and a protected pure virtual
`DoCreateShaderProgram`, following the NVI pattern from
[Step 0](00-setup.md).

### `include/arda/renderer/Device.h`

Add two includes next to the existing ones:

```cpp
#include <arda/renderer/shaders/ShaderProgram.h>

#include <string_view>
```

Add the public functions after `CreateGraphicsWindow`:

```cpp
    // Compiles and links a program (3.4.1). The sources are in the backend's
    // shading language: GLSL for OpenGL33, HLSL for Direct3D11 (see Api()).
    // Throws CouldNotCreateVideoCardResourceException with the compile or
    // link log if the shaders are invalid.
    std::shared_ptr<ShaderProgram> CreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view fragmentShaderSource);

    std::shared_ptr<ShaderProgram> CreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view geometryShaderSource,
        std::string_view fragmentShaderSource);
```

And the backend hook in the `protected:` section, after
`DoCreateGraphicsWindow`:

```cpp
    // An empty geometryShaderSource means there is no geometry shader.
    virtual std::shared_ptr<ShaderProgram> DoCreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view geometryShaderSource,
        std::string_view fragmentShaderSource) = 0;
```

The result is a `std::shared_ptr`, because a program is a *resource* (see
the README's ownership rules): a `DrawState` holds one, and several draw
states, plus Step 4's `ShaderCache`, can share it. Step 3 explains
`shared_ptr`.

> **Why the sources are per backend, not one language for both:** OpenGL
> compiles GLSL and D3D11 compiles HLSL, and the two differ in more than
> syntax (vertex inputs are matched by location in GLSL but by semantic in
> HLSL; samplers are one object in GLSL but two in HLSL). Translating one
> into the other means a cross-compiler such as SPIRV-Cross or glslang,
> which is a large dependency and a debugging layer between you and the
> errors. The book targets one API, so OpenGlobe never faced this.
> arda takes the simplest route that keeps both backends honest: the app
> picks the right source with `device->Api()`, and the README's
> portability table lists the conventions that make the two versions line
> up. Step 7 shows the pattern.

### `src/Device.cpp`

Add these two definitions inside `namespace arda::renderer`, after
`Device::CreateGraphicsWindow`. `Device.cpp` already includes
`<stdexcept>`.

```cpp
std::shared_ptr<ShaderProgram> Device::CreateShaderProgram(
    std::string_view vertexShaderSource, std::string_view fragmentShaderSource) {
    return CreateShaderProgram(vertexShaderSource, {}, fragmentShaderSource);
}

std::shared_ptr<ShaderProgram> Device::CreateShaderProgram(
    std::string_view vertexShaderSource,
    std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource) {
    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        throw std::invalid_argument("CreateShaderProgram: vertex and fragment shader sources are required");
    }
    return DoCreateShaderProgram(vertexShaderSource, geometryShaderSource, fragmentShaderSource);
}
```

`{}` passed for a `std::string_view` parameter is an empty view. The
two-argument overload forwards to the three-argument one, so the validation
is written once, and each backend implements one function.

### `src/gl/DeviceGL3x.h`

In the `protected:` section, after `DoCreateGraphicsWindow`:

```cpp
    std::shared_ptr<ShaderProgram> DoCreateShaderProgram(
        std::string_view vertexShaderSource,
        std::string_view geometryShaderSource,
        std::string_view fragmentShaderSource) override;
```

### `src/gl/DeviceGL3x.cpp`

Add the include with the other `gl/` includes:

```cpp
#include "gl/shaders/ShaderProgramGL3x.h"
```

And the function at the end of the namespace:

```cpp
std::shared_ptr<ShaderProgram> DeviceGL3x::DoCreateShaderProgram(
    std::string_view vertexShaderSource,
    std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource) {
    return std::make_shared<ShaderProgramGL3x>(vertexShaderSource, geometryShaderSource, fragmentShaderSource);
}
```

`std::make_shared<ShaderProgramGL3x>` returns a
`std::shared_ptr<ShaderProgramGL3x>`, which converts to
`std::shared_ptr<ShaderProgram>` automatically, the same way a derived
pointer converts to a base pointer. Step 4 changes this line to pass
`*this` as well, so the program can look up the device's automatic
uniforms.

> **OpenGL note — which context is current?** GL calls need a current
> context, and `ShaderProgramGL3x` makes many of them. After `CreateDevice`,
> the device's hidden share context is current. After
> `CreateGraphicsWindow`, the new window's context is. Either is fine:
> shader and program objects belong to the *share group*, so a program
> created in one context can be used in every window
> ([Step 0](00-setup.md) explains share groups). The same is true of uniform
> *values*, which are stored in the program. What is not shared is the
> `glUseProgram` binding, which each context has its own copy of. That's
> why Step 3's context binds the program itself before every draw.

### Checkpoint 3

```sh
cmake --build build/vs --config Debug
```

Everything, including `arda_scene` and the existing tests, should build
and run as before.

**Optional: see reflection work in `arda_scene`.** Temporarily add this
inside `main`'s `try` block, after the window is created, and add
`#include <arda/renderer/shaders/ShaderProgram.h>` (and `<cstdio>`, if it
isn't there):

```cpp
auto program = device->CreateShaderProgram(R"(
layout(location = og_positionVertexLocation) in vec4 position;
uniform mat4 u_modelViewPerspective;
void main() { gl_Position = u_modelViewPerspective * position; })",
R"(
out vec3 fragmentColor;
uniform vec3 u_color;
void main() { fragmentColor = u_color; })");

for (const auto& attribute : program->VertexAttributes()) {
    std::printf("attribute %s at location %d\n", attribute.name.c_str(), attribute.location);
}
for (const auto& uniform : program->Uniforms()) {
    std::printf("uniform %s\n", uniform->Name().c_str());
}
std::printf("fragmentColor -> color attachment %d\n", program->FragmentOutputLocation("fragmentColor"));
```

Then break the fragment shader (misspell `u_color` in `main`) and run
again: `main`'s `catch` prints the driver's compile log. Remove the snippet
afterwards; Step 3 draws with a real program.

---

## 15. Tests

The public API is tested in `ShaderProgramTests.cpp`. The GL-specific
behavior that has no public API yet (the dirty list, the actual upload, the
prelude text) is tested in `gl/UniformGL3xTests.cpp`, which includes the
backend's private headers.

The device's hidden context is current after `CreateDevice`, so no window is
needed.

### `tests/CMakeLists.txt`

Add `ShaderProgramTests.cpp` to the `add_executable(arda_tests ...)` list,
and a block for the white-box tests after the existing
`target_link_libraries(arda_tests ...)` line:

```cmake
add_executable(arda_tests
    # ... the existing files ...
    src/renderer/ShaderProgramTests.cpp
)
```

```cmake
# White-box tests of the GL backend. They include private headers from
# renderer/src and make GL calls directly, so they need renderer/src on the
# include path and glad's headers.
if(ARDA_RENDERER_GL)
    target_sources(arda_tests PRIVATE src/renderer/gl/UniformGL3xTests.cpp)
    target_include_directories(arda_tests PRIVATE ${PROJECT_SOURCE_DIR}/renderer/src)
    target_link_libraries(arda_tests PRIVATE glad::glad)
endif()
```

`ARDA_RENDERER_GL` is visible here because `option()` stores it in the CMake
cache, and `renderer/` is added before `tests/`. `${PROJECT_SOURCE_DIR}` is
the repository root, where the top-level `project(Arda ...)` is. Linking
`glad::glad` again is harmless: there is still only one copy of glad's
function pointers in the final executable, the ones `DeviceGL3x` loaded.

> **Why white-box tests are a separate file:** everything else in
> `arda_tests` uses only public headers, which is the contract the scene
> sees. Keeping the private-header tests in their own file, behind the
> `ARDA_RENDERER_GL` option, makes that boundary visible. When the D3D11
> backend exists, its equivalents go in `gl/`'s sibling, `d3d11/`.

### `tests/src/renderer/ShaderProgramTests.cpp`

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/Exceptions.h>
#include <arda/renderer/shaders/ShaderProgram.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace arda::renderer;
using arda::core::Matrix4;
using arda::core::Vector3;

namespace {

// No #version line: the prelude adds it. og_positionVertexLocation and
// og_oneOverPi come from the prelude too.
constexpr const char* kVertexShader = R"(
layout(location = og_positionVertexLocation) in vec4 position;
layout(location = og_normalVertexLocation) in vec3 normal;
out vec3 worldNormal;
uniform mat4 u_modelViewPerspective;
void main()
{
    gl_Position = u_modelViewPerspective * position;
    worldNormal = normal;
})";

constexpr const char* kFragmentShader = R"(
in vec3 worldNormal;
out vec3 fragmentColor;
uniform vec3 u_color;
void main()
{
    fragmentColor = u_color * og_oneOverPi + worldNormal;
})";

// Passes each triangle through unchanged.
constexpr const char* kGeometryShader = R"(
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 worldNormal[];
out vec3 geometryNormal;
void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        geometryNormal = worldNormal[i];
        EmitVertex();
    }
    EndPrimitive();
})";

constexpr const char* kFragmentShaderAfterGeometry = R"(
in vec3 geometryNormal;
out vec3 fragmentColor;
void main()
{
    fragmentColor = geometryNormal;
})";

constexpr const char* kPassThroughVertexShader = R"(
in vec4 position;
void main()
{
    gl_Position = position;
})";

// Declares one uniform of every type in UniformType (except the samplers
// for 1D, 3D and array textures, which work the same way). Every uniform is
// used, because the linker removes unused uniforms.
constexpr const char* kEveryUniformTypeFragmentShader = R"(
out vec4 fragmentColor;

uniform int u_int;
uniform float u_float;
uniform vec2 u_vec2;
uniform vec3 u_vec3;
uniform vec4 u_vec4;
uniform ivec2 u_ivec2;
uniform ivec3 u_ivec3;
uniform ivec4 u_ivec4;
uniform bool u_bool;
uniform bvec2 u_bvec2;
uniform bvec3 u_bvec3;
uniform bvec4 u_bvec4;
uniform mat2 u_mat2;
uniform mat3 u_mat3;
uniform mat4 u_mat4;
uniform mat2x3 u_mat2x3;
uniform mat2x4 u_mat2x4;
uniform mat3x2 u_mat3x2;
uniform mat3x4 u_mat3x4;
uniform mat4x2 u_mat4x2;
uniform mat4x3 u_mat4x3;
uniform sampler2D u_sampler2D;
uniform sampler2DShadow u_sampler2DShadow;
uniform sampler2DRect u_sampler2DRect;
uniform samplerCube u_samplerCube;
uniform isampler2D u_isampler2D;
uniform usampler2D u_usampler2D;

void main()
{
    float sum = float(u_int) + u_float + u_vec2.x + u_vec3.x + u_vec4.x;
    sum += float(u_ivec2.x + u_ivec3.x + u_ivec4.x);
    sum += (u_bool ? 1.0 : 0.0) + (u_bvec2.x ? 1.0 : 0.0) + (u_bvec3.x ? 1.0 : 0.0) + (u_bvec4.x ? 1.0 : 0.0);
    sum += u_mat2[0][0] + u_mat3[0][0] + u_mat4[0][0];
    sum += u_mat2x3[0][0] + u_mat2x4[0][0] + u_mat3x2[0][0] + u_mat3x4[0][0] + u_mat4x2[0][0] + u_mat4x3[0][0];
    sum += texture(u_sampler2D, vec2(0.0)).r;
    sum += texture(u_sampler2DShadow, vec3(0.0));
    sum += texture(u_sampler2DRect, vec2(0.0)).r;
    sum += texture(u_samplerCube, vec3(1.0)).r;
    sum += float(texture(u_isampler2D, vec2(0.0)).r);
    sum += float(texture(u_usampler2D, vec2(0.0)).r);
    fragmentColor = vec4(sum);
})";

} // namespace

TEST_CASE("ShaderProgram finds vertex attributes, uniforms and fragment outputs") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto sp = device->CreateShaderProgram(kVertexShader, kFragmentShader);

    const ShaderVertexAttributeCollection& attributes = sp->VertexAttributes();
    CHECK(attributes.Size() == 2);
    REQUIRE(attributes.Contains("position"));
    CHECK(attributes["position"].location == VertexLocations::Position);
    CHECK(attributes["position"].type == ShaderVertexAttributeType::FloatVector4);
    CHECK(attributes["normal"].location == VertexLocations::Normal);
    CHECK(attributes["normal"].type == ShaderVertexAttributeType::FloatVector3);
    CHECK_FALSE(attributes.Contains("missing"));
    CHECK_THROWS_AS(attributes["missing"], std::out_of_range);

    CHECK(sp->Uniforms().Size() == 2);
    CHECK(sp->Uniforms()["u_color"].Datatype() == UniformType::FloatVector3);
    CHECK(sp->Uniforms()["u_modelViewPerspective"].Datatype() == UniformType::FloatMatrix44);

    CHECK(sp->FragmentOutputLocation("fragmentColor") == 0);
    CHECK_THROWS_AS(sp->FragmentOutputLocation("missing"), std::out_of_range);
}

TEST_CASE("Uniforms are looked up by name and value type") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto sp = device->CreateShaderProgram(kVertexShader, kFragmentShader);
    UniformCollection& uniforms = sp->Uniforms();

    // Every uniform starts at zero, like GL's own initial values.
    auto& color = uniforms.Get<Vector3<float>>("u_color");
    CHECK(color.Value() == Vector3<float>(0, 0, 0));

    color.SetValue(Vector3<float>(1, 0, 0));
    CHECK(color.Value() == Vector3<float>(1, 0, 0));

    auto& mvp = uniforms.Get<Matrix4<float>>("u_modelViewPerspective");
    mvp.SetValue(Matrix4<float>::Identity());
    CHECK(mvp.Value() == Matrix4<float>::Identity());

    // The name exists, but the value type is wrong.
    CHECK_THROWS_AS(uniforms.Get<float>("u_color"), std::invalid_argument);
    // The name doesn't exist.
    CHECK_THROWS_AS(uniforms.Get<float>("u_missing"), std::out_of_range);
    CHECK_THROWS_AS(uniforms["u_missing"], std::out_of_range);
    CHECK(uniforms.Find("u_missing") == nullptr);
    CHECK(uniforms.Find("u_color") == &color);

    // Reading through a const collection gives const uniforms.
    const UniformCollection& constUniforms = uniforms;
    CHECK(constUniforms.Get<Vector3<float>>("u_color").Value() == Vector3<float>(1, 0, 0));

    // Iteration visits every uniform once.
    int count = 0;
    for (const auto& uniform : uniforms) {
        CHECK(uniforms.Contains(uniform->Name()));
        ++count;
    }
    CHECK(count == 2);
}

TEST_CASE("Every supported GLSL uniform type is reflected") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto sp = device->CreateShaderProgram(kPassThroughVertexShader, kEveryUniformTypeFragmentShader);

    const std::vector<std::pair<std::string, UniformType>> expected = {
        {"u_int", UniformType::Int},
        {"u_float", UniformType::Float},
        {"u_vec2", UniformType::FloatVector2},
        {"u_vec3", UniformType::FloatVector3},
        {"u_vec4", UniformType::FloatVector4},
        {"u_ivec2", UniformType::IntVector2},
        {"u_ivec3", UniformType::IntVector3},
        {"u_ivec4", UniformType::IntVector4},
        {"u_bool", UniformType::Bool},
        {"u_bvec2", UniformType::BoolVector2},
        {"u_bvec3", UniformType::BoolVector3},
        {"u_bvec4", UniformType::BoolVector4},
        {"u_mat2", UniformType::FloatMatrix22},
        {"u_mat3", UniformType::FloatMatrix33},
        {"u_mat4", UniformType::FloatMatrix44},
        {"u_mat2x3", UniformType::FloatMatrix23},
        {"u_mat2x4", UniformType::FloatMatrix24},
        {"u_mat3x2", UniformType::FloatMatrix32},
        {"u_mat3x4", UniformType::FloatMatrix34},
        {"u_mat4x2", UniformType::FloatMatrix42},
        {"u_mat4x3", UniformType::FloatMatrix43},
        {"u_sampler2D", UniformType::Sampler2D},
        {"u_sampler2DShadow", UniformType::Sampler2DShadow},
        {"u_sampler2DRect", UniformType::Sampler2DRectangle},
        {"u_samplerCube", UniformType::SamplerCube},
        {"u_isampler2D", UniformType::IntSampler2D},
        {"u_usampler2D", UniformType::UnsignedIntSampler2D},
    };

    CHECK(sp->Uniforms().Size() == expected.size());
    for (const auto& [name, type] : expected) {
        CAPTURE(name);
        REQUIRE(sp->Uniforms().Contains(name));
        CHECK(sp->Uniforms()[name].Datatype() == type);
    }

    // Samplers hold a texture unit index, so they are Uniform<int>.
    CHECK_NOTHROW(sp->Uniforms().Get<int>("u_sampler2D").SetValue(3));
    CHECK_NOTHROW(sp->Uniforms().Get<bool>("u_bool").SetValue(true));
}

TEST_CASE("A geometry shader is optional") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto sp = device->CreateShaderProgram(kVertexShader, kGeometryShader, kFragmentShaderAfterGeometry);

    CHECK(sp->VertexAttributes().Contains("position"));
    CHECK(sp->Uniforms().Contains("u_modelViewPerspective"));
    CHECK(sp->FragmentOutputLocation("fragmentColor") == 0);
}

TEST_CASE("Compile and link errors throw with the log") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK_THROWS_WITH_AS(device->CreateShaderProgram("this is not glsl", kFragmentShader),
                         doctest::Contains("Could not compile shader object"),
                         CouldNotCreateVideoCardResourceException);

    // This fragment shader compiles, because a shader on its own doesn't need
    // main(). Linking fails, because every stage of a program does.
    constexpr const char* noMain = R"(
out vec3 fragmentColor;
void notMain()
{
    fragmentColor = vec3(1.0);
})";
    CHECK_THROWS_WITH_AS(device->CreateShaderProgram(kPassThroughVertexShader, noMain),
                         doctest::Contains("Could not link shader program"),
                         CouldNotCreateVideoCardResourceException);

    // Catching by the standard base class works too.
    CHECK_THROWS_AS(device->CreateShaderProgram("this is not glsl", kFragmentShader), std::runtime_error);

    CHECK_THROWS_AS(device->CreateShaderProgram("", kFragmentShader), std::invalid_argument);
}

TEST_CASE("A #version 330 line in the source is accepted; other versions are not") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK_NOTHROW(device->CreateShaderProgram(
        std::string("#version 330\n") + kVertexShader,
        std::string("#version 330 core\n") + kFragmentShader));

    // Leading blank lines, as in a raw string literal that starts on the next line.
    CHECK_NOTHROW(device->CreateShaderProgram(
        std::string("\n  \n#version 330\n") + kVertexShader, kFragmentShader));

    CHECK_THROWS_AS(device->CreateShaderProgram(
        std::string("#version 410\n") + kVertexShader, kFragmentShader), std::invalid_argument);
}
```

What the tests cover:

- **Reflection**: attribute locations come from the prelude's defines
  (`normal` is at 2, not 1), both uniforms are found with the right types,
  and the single fragment output is at location 0.
- **Lookup**: `Get<T>` with the right type, the wrong type, and a missing
  name each behave as documented. `Find` returns the same object as `Get`.
- **Every uniform type**: one shader declares 27 uniforms of different
  types, and each one is reflected with the right `UniformType`. This
  exercises every branch of `ToUniformType` and `CreateUniform` that a
  sampler2D-and-friends shader can reach, and instantiates every
  `UploadUniform` overload.
- **Errors**: a compile error and a link error both throw
  `CouldNotCreateVideoCardResourceException` with a message containing the
  log. The link error uses a fragment shader without `main`, which is a link
  error on every driver. (A fragment input with no matching vertex output
  looks like a reliable link error, but some drivers accept it.)
- **`#version`**: accepted with and without leading blank lines, and with
  `core`; anything but 330 is rejected.

A few doctest features appear here for the first time:
`CHECK_THROWS_WITH_AS(expr, doctest::Contains("text"), Type)` checks both
the exception type and that `what()` contains `text`. `CAPTURE(name)` adds
the current `name` to the report if a check in the loop fails, so you know
*which* uniform broke. `REQUIRE` stops the test case on failure, where
`CHECK` would carry on; it's used before lines that would crash if the
check failed.

<a id="cpp-structured-bindings"></a>

> **C++ note — structured bindings:** `for (const auto& [name, type] :
> expected)` unpacks each `std::pair` into two names, instead of writing
> `entry.first` and `entry.second`. It works on pairs, tuples, arrays and
> structs with public members, like C#'s tuple deconstruction
> `foreach (var (name, type) in expected)`.

### `tests/src/renderer/gl/UniformGL3xTests.cpp`

```cpp
// White-box tests of the GL backend. They include private headers from
// renderer/src, so tests/CMakeLists.txt adds that include directory.
#include <doctest/doctest.h>

#include "gl/shaders/GlslPrelude.h"
#include "gl/shaders/ShaderProgramGL3x.h"
#include "gl/shaders/UniformGL3x.h"

#include <arda/core/geometry/Matrix.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Device.h>

#include <vector>

using namespace arda::renderer;
using namespace arda::renderer::gl;
using arda::core::Matrix23;
using arda::core::Vector3;

namespace {

// Records what a uniform reports, instead of cleaning it before a draw.
class RecordingObserver final : public ICleanableObserver {
public:
    void NotifyDirty(ICleanable& value) override { notified.push_back(&value); }
    std::vector<ICleanable*> notified;
};

constexpr const char* kVertexShader = R"(
in vec4 position;
void main()
{
    gl_Position = position;
})";

constexpr const char* kFragmentShader = R"(
out vec4 fragmentColor;
uniform vec3 u_color;
uniform mat2x3 u_matrix;
void main()
{
    fragmentColor = vec4(u_color + u_matrix[1], 1.0);
})";

} // namespace

TEST_CASE("The GLSL prelude has #version first, then the defines and constants") {
    const std::string& prelude = GlslPrelude();
    CHECK(prelude.starts_with("#version 330\n"));
    CHECK(prelude.find("#define og_positionVertexLocation 0\n") != std::string::npos);
    CHECK(prelude.find("#define og_normalVertexLocation 2\n") != std::string::npos);
    CHECK(prelude.find("const float og_pi = 3.14159265358979") != std::string::npos);
    CHECK(&GlslPrelude() == &prelude);   // built once
}

TEST_CASE("UniformGL3x joins the dirty list once, and only when its value changes") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);   // makes a GL context current
    ShaderProgramGL3x program(kVertexShader, {}, kFragmentShader);
    const GLint location = glGetUniformLocation(program.Handle(), "u_color");
    REQUIRE(location != -1);

    RecordingObserver observer;
    UniformGL3x<Vector3<float>> color("u_color", UniformType::FloatVector3, location, observer);
    CHECK(observer.notified.size() == 1);   // new uniforms start dirty

    color.SetValue(Vector3<float>(1, 2, 3));
    CHECK(observer.notified.size() == 1);   // already on the list

    program.Bind();   // glUniform* writes to the bound program
    color.Clean();

    GLfloat uploaded[3] = {};
    glGetUniformfv(program.Handle(), location, uploaded);
    CHECK(uploaded[0] == 1.0f);
    CHECK(uploaded[1] == 2.0f);
    CHECK(uploaded[2] == 3.0f);

    color.SetValue(Vector3<float>(1, 2, 3));
    CHECK(observer.notified.size() == 1);   // same value: nothing to upload

    color.SetValue(Vector3<float>(4, 5, 6));
    CHECK(observer.notified.size() == 2);   // changed: dirty again
    CHECK(observer.notified.back() == &color);

    glUseProgram(0);
}

TEST_CASE("Matrices upload column-major, matching GLSL's matrix[column][row]") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    ShaderProgramGL3x program(kVertexShader, {}, kFragmentShader);
    const GLint location = glGetUniformLocation(program.Handle(), "u_matrix");
    REQUIRE(location != -1);

    RecordingObserver observer;
    UniformGL3x<Matrix23<float>> matrix("u_matrix", UniformType::FloatMatrix23, location, observer);

    Matrix23<float> value;
    value(1, 2) = 5.0f;   // column 1, row 2: u_matrix[1][2] in GLSL
    matrix.SetValue(value);

    program.Bind();
    matrix.Clean();

    GLfloat uploaded[6] = {};
    glGetUniformfv(program.Handle(), location, uploaded);   // returned column-major
    CHECK(uploaded[1 * 3 + 2] == 5.0f);

    glUseProgram(0);
}
```

These tests build a `ShaderProgramGL3x` directly (not through the device) so
they can call `Bind()` and `Handle()`, and create their own `UniformGL3x`
with a `RecordingObserver`, a *test double* that just records what it's
told. That isolates the dirty-flag logic from the program's own list, which
is private until Step 3's `Clean`. `glGetUniformfv` reads a uniform's value
back from the program, which proves the upload really reached GL. Each test
unbinds the program at the end so nothing leaks into the next test.

The device is declared first in each test so it's destroyed last, after the
program: GL objects must be deleted while a context of their share group
still exists.

### Milestone: run the tests

```sh
./run.sh -t arda_tests
```

All test cases should pass, including the existing Step 0 and Step 1
tests. The same caveat as Step 0 applies: these tests need a GPU driver and
a desktop session.

---

## Common problems

| Symptom | Cause and fix |
|---|---|
| `No uniform named 'u_color'` although the shader declares it | The uniform isn't *used*, so the linker removed it (it's inactive). Use it, or look it up with `Find`/`Contains` when it's optional. Unused vertex inputs disappear from `VertexAttributes()` the same way. |
| `Uniform 'u_color' is not of the requested type` | The C++ type in `Get<T>` doesn't match the GLSL type. `vec3` is `Vector3<float>`, `ivec3` is `Vector3<int>`, `bool` is `bool`, every sampler is `int`, `mat4` is `Matrix4<float>`. See `CreateUniform`. |
| Compile error `#version must occur first` / `#version directive must occur before anything else` | The source has a `#version` that `CommentOutVersionDirective` didn't see, usually because a comment comes before it. Remove the line; the prelude supplies it. |
| `Only GLSL version 330 is supported.` | The shader asks for another version. GLSL 3.30 is what a GL 3.3 context guarantees. Port the shader, or drop the line and see what fails. |
| Compile error on `og_...` names | The name is misspelled, or the prelude isn't being passed. The names are OpenGlobe's, including `og_threePiOver2`. |
| Compile error line numbers look wrong | Your driver counts from the start of the prelude (AMD, Mesa) instead of per string (NVIDIA). Subtract the prelude's 20 lines. See the [info log note](#gl-info-logs). |
| `Uniform arrays are not supported` | A `uniform float values[4];`. Not supported, as in OpenGlobe. Use separate uniforms, a texture, or (later) a uniform buffer. |
| `Unsupported uniform type 0x....` | A `uint`, `uvec*`, `samplerBuffer` or multisample sampler. Look the value up in `glad.h`. Adding one means a `UniformType`, a `ToUniformType` case, a `CreateUniform` case and maybe an `UploadUniform` overload. |
| `CHECK(sp->FragmentOutputLocation("fragmentColor") == 0)` fails with several outputs | With more than one output, the linker assigns locations as it likes. Give each output `layout(location = N)`. |
| Access violation at address 0 in `glCreateShader` | No GL context is current, or glad wasn't loaded. Create the device first. |
| Access violation on `std::string(view).c_str()`-style code you wrote yourself | A `string_view` that pointed into a destroyed temporary. See the [`string_view` note](#cpp-string-view). |
| Uniform values don't change on screen (Step 3 onwards) | The program wasn't bound before `Clean()`, or `SetValue` was called on a *different* program's uniform with the same name. Each program has its own uniforms. |

---

## Complete CMake files after Step 2

The CMake sections above list only this step's additions. Here are the three
CMake files as they should look once Step 2 is done, with every step so far
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
    include/arda/core/Math.h
    include/arda/core/Trig.h
    include/arda/core/geometry/Ellipsoid.h
    include/arda/core/geometry/Matrix.h
    include/arda/core/geometry/Matrix4.h
    include/arda/core/geometry/Vector2.h
    include/arda/core/geometry/Vector3.h
    include/arda/core/geometry/Vector4.h
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
    src/shaders/BuiltinConstants.cpp
    src/shaders/UniformCollection.cpp
    include/arda/renderer/ClearState.h
    include/arda/renderer/Color.h
    include/arda/renderer/Context.h
    include/arda/renderer/Device.h
    include/arda/renderer/DrawState.h
    include/arda/renderer/Exceptions.h
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
    include/arda/renderer/shaders/ShaderProgram.h
    include/arda/renderer/shaders/ShaderVertexAttribute.h
    include/arda/renderer/shaders/Uniform.h
    include/arda/renderer/shaders/UniformCollection.h
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
        src/gl/shaders/GlslPrelude.cpp
        src/gl/shaders/ShaderObjectGL3x.cpp
        src/gl/shaders/ShaderProgramGL3x.cpp
        src/gl/shaders/UniformGL3x.cpp
        src/gl/ContextGL3x.h
        src/gl/DeviceGL3x.h
        src/gl/GLHandle.h
        src/gl/GraphicsWindowGL3x.h
        src/gl/TypeConverterGL3x.h
        src/gl/shaders/GlslPrelude.h
        src/gl/shaders/ShaderObjectGL3x.h
        src/gl/shaders/ShaderProgramGL3x.h
        src/gl/shaders/UniformGL3x.h
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

## D3D11 check

How Step 8 implements the same interfaces:

- **Compiling:** `ShaderProgramD3D11` compiles each stage with `D3DCompile`
  (`vs_5_0`, `gs_5_0`, `ps_5_0`, entry point `main`), using
  `D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR` so matrices upload the same bytes as
  in GL. Errors come back in an `ID3DBlob` and are thrown as
  `CouldNotCreateVideoCardResourceException`, same as here.
- **Prelude:** `HlslPrelude()` formats the same `BuiltinDefines()` and
  `BuiltinConstants()` tables as `#define` and `static const float og_pi =
  ...;` lines. There is no `#version` in HLSL; the target (`vs_5_0`) plays
  that role.
- **Filling the collections from reflection** (`D3DReflect` gives an
  `ID3D11ShaderReflection`):

  | Collection | Source |
  |---|---|
  | Vertex attributes | Input parameters. `SemanticName` is the attribute name and `SemanticIndex` is the location (`float4 position : position0`). |
  | Uniforms | The variables of each stage's `$Globals` constant buffer. `UniformD3D11<T>` writes at each variable's `StartOffset`. |
  | `FragmentOutputLocation` | Output parameters named `SV_Target`. The semantic index is the location. |

- **Uniform interface:** `UniformBase`, `Uniform<T>`, `UniformCollection`
  and `Get<T>` are unchanged. Only the implementing class changes:
  `UniformD3D11<T>` copies its value into a CPU-side copy of `$Globals`
  instead of calling `glUniform*`, and the constant buffer is uploaded once
  before the draw if anything changed.
- **Dirty list:** `ICleanable` and `ICleanableObserver` in `src/` are reused
  as they are.
- **`Matrix<T, C, R>`:** HLSL has `float2x3` and friends too, and the
  public types work unchanged, but the bytes are *not* always the same as
  GL's. In a constant buffer, each column of a column-major matrix starts on
  a new 16-byte register. A `mat4` (4 floats per column) needs no padding,
  so it uploads the same 64 bytes as `glUniformMatrix4fv`. A `mat2` or
  `mat3x2`, whose columns are shorter, gets padding after each column, so
  `UniformD3D11<T>` copies it column by column (Step 8). Also, HLSL's
  `float2x3` is 2 *rows* by 3 columns, the opposite of GLSL's naming, so
  Step 8's `ToUniformType` swaps them.

Nothing in the public headers mentions GL, so no public change is needed.

---

## Checklist

- [ ] Core: `operator==` on `Vector2`/`3`/`4`, `Vector3` static_asserts, `Vector4` includes
- [ ] Core: `Matrix4.h` (storage), `Matrix.h`, listed in `core/CMakeLists.txt`
- [ ] `Exceptions.h`, `VertexLocations.h`
- [ ] `ShaderVertexAttribute.h`, `Uniform.h`, `UniformCollection.h` / `.cpp`, `ShaderProgram.h`
- [ ] `src/Cleanable.h`, `src/shaders/BuiltinConstants.h` / `.cpp`
- [ ] **Checkpoint 1:** `arda_renderer` builds
- [ ] `GlslPrelude`, `ShaderObjectGL3x`, `UniformGL3x<T>` and the `UploadUniform` overloads
- [ ] `TypeConverterGL3x`: `ToShaderVertexAttributeType`, `ToUniformType`
- [ ] `ShaderProgramGL3x`
- [ ] **Checkpoint 2:** `arda_renderer` builds with the GL files
- [ ] `Device::CreateShaderProgram` (both overloads), `DoCreateShaderProgram`, `DeviceGL3x::DoCreateShaderProgram`
- [ ] **Checkpoint 3:** everything builds
- [ ] `tests/CMakeLists.txt`, `ShaderProgramTests.cpp`, `gl/UniformGL3xTests.cpp`
- [ ] **Milestone:** `arda_tests` passes

---

## C++ and OpenGL notes index

Later guides link to these instead of explaining them again.

**C++ notes**

| Topic | Link |
|---|---|
| Raw string literals | [`#cpp-raw-string-literals`](#cpp-raw-string-literals) |
| Custom exception classes | [`#cpp-custom-exceptions`](#cpp-custom-exceptions) |
| `std::string_view` | [`#cpp-string-view`](#cpp-string-view) |
| `std::ranges::find` with a projection | [`#cpp-ranges-find`](#cpp-ranges-find) |
| Non-template base under a class template (type erasure) | [`#cpp-type-erasure`](#cpp-type-erasure) |
| Why member function templates can't be virtual | [`#cpp-virtual-templates`](#cpp-virtual-templates) |
| Inheriting constructors | [`#cpp-inheriting-constructors`](#cpp-inheriting-constructors) |
| `std::vector<std::unique_ptr<Base>>` | [`#cpp-vector-unique-ptr`](#cpp-vector-unique-ptr) |
| `std::unordered_map` and heterogeneous lookup | [`#cpp-unordered-map`](#cpp-unordered-map) |
| `dynamic_cast` vs `static_cast` | [`#cpp-dynamic-cast`](#cpp-dynamic-cast) |
| `const` and non-`const` overloads | [`#cpp-const-overloads`](#cpp-const-overloads) |
| `[[noreturn]]` | [`#cpp-noreturn`](#cpp-noreturn) |
| `std::span` | [`#cpp-span`](#cpp-span) |
| `std::format` | [`#cpp-format`](#cpp-format) |
| Function-local `static` variables | [`#cpp-function-local-static`](#cpp-function-local-static) |
| `std::string` as an output buffer | [`#cpp-string-buffer`](#cpp-string-buffer) |
| The dirty flag and dirty list | [`#cpp-dirty-flag`](#cpp-dirty-flag) |
| Overloads vs specialization vs `if constexpr` | [`#cpp-overloads-vs-specialization`](#cpp-overloads-vs-specialization) |
| Why overloads are declared before the template | [`#cpp-two-phase-lookup`](#cpp-two-phase-lookup) |
| Deriving from a class and an interface | [`#cpp-multiple-interfaces`](#cpp-multiple-interfaces) |
| `std::optional` | [`#cpp-optional`](#cpp-optional) |
| Structured bindings | [`#cpp-structured-bindings`](#cpp-structured-bindings) |

**OpenGL notes**

| Topic | Link |
|---|---|
| The programmable pipeline | [`#the-programmable-pipeline`](#the-programmable-pipeline) |
| Shader objects and program objects | [`#shader-objects-and-program-objects`](#shader-objects-and-program-objects) |
| `#version 330` and the prelude | [`#glsl-version-330-and-the-prelude`](#glsl-version-330-and-the-prelude) |
| Vertex attribute locations | [`#vertex-attribute-locations`](#vertex-attribute-locations) |
| Fragment outputs | [`#fragment-outputs`](#fragment-outputs) |
| Uniforms | [`#uniforms`](#uniforms) |
| Reflection | [`#reflection`](#reflection) |
| Info logs and line numbers | [`#gl-info-logs`](#gl-info-logs) |
| The `glUniform*` family | [`#gl-uniform-calls`](#gl-uniform-calls) |
