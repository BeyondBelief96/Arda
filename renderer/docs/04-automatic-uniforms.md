# Step 4: Automatic uniforms and the shader cache (3.4.5–3.4.6)

**Goal:** a shader that declares `uniform mat4 og_modelViewPerspectiveMatrix;`
gets the right value on every draw, with no client code. To get there you
need three things this step builds, in this order:

1. **Matrix math** in core: `Matrix4<T>` multiplication, `LookAt`, and
   perspective and orthographic projections that work for both OpenGL and
   Direct3D depth ranges.
2. **A `Camera` and a real `SceneState`**, which turn the camera into view
   and projection matrices.
3. **Automatic uniforms:** a registry of factories on the `Device`. When a
   program is linked, each uniform whose name starts with `og_` is matched
   against the registry. Link-time ones are set once. Draw-time ones are set
   from the `SceneState` before every draw.

The shader cache (3.4.6) comes at the end. It's independent of the rest.

**Read:** 3.4.5 "Automatic Uniforms" (Listings 3.12–3.15, Table 3.1) and
3.4.6 "Caching Shaders" (Listing 3.16). Chapter 5's first pages explain why
the CPU keeps doubles; skim them if you're curious, but they aren't needed yet.

**Milestone:** the Step 3 triangle, moved into the xz plane and viewed through
a `Camera`, stays correctly shaped when you resize the window.

> **Build order:** this step comes after Step 3 (vertex data), not straight
> after Step 2. See the note at the top of the [README](../README.md).

## What you'll learn

| Topic | Where |
|---|---|
| The transform pipeline: model, view, projection, clip space, perspective division, NDC, viewport | [Background](#background-from-a-vertex-to-a-pixel) |
| Column-major storage, and why `glUniformMatrix4fv` gets `GL_FALSE` | [1.3](#13-matrix4h-the-full-file) |
| GL vs D3D clip depth, and `ClipDepth` | [1.1](#11-clipdepthh), [1.3](#13-matrix4h-the-full-file) |
| Doubles on the CPU, floats on the GPU | [2.2](#22-scenestate) |
| Link-time vs draw-time automatic uniforms | [3.1](#31-what-automatic-uniforms-are) |
| C++: class templates for math types, operator overloading, `constexpr`, explicit conversions, `std::numbers`, abstract factories, `std::function` registries, `mutable` caches, `std::weak_ptr` | Throughout, in `> **C++ note**` callouts |

## OpenGlobe reference

| File | What to take from it |
|---|---|
| [Core/Matrices/Matrix4D.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Matrices/Matrix4D.cs) | Constructor argument order, `operator*` (matrix and vector), `Transpose`, `CreatePerspectiveFieldOfView`, `CreateOrthographicOffCenter`, `LookAt`, `ToMatrix4F` |
| [Core/Matrices/Matrix42.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Core/Matrices/Matrix42.cs) | Only needed for `ModelZToClipCoordinates` (Chapter 4). Not ported in this step. |
| [Renderer/Scene/Camera.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Scene/Camera.cs) | Defaults, `Forward`, `Right`, `FieldOfViewX`, `OrthographicDepth`, `EyeHigh`/`EyeLow`, `ZoomToTarget`, `Height` |
| [Renderer/Scene/SceneState.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Scene/SceneState.cs) | Defaults and every matrix property |
| [Shaders/LinkAutomaticUniforms/*](https://github.com/virtualglobebook/OpenGlobe/tree/master/Source/Renderer/Shaders/LinkAutomaticUniforms) | `LinkAutomaticUniform`, `LinkAutomaticUniformCollection`. `TextureUniform` comes in Step 5. |
| [DrawAutomaticUniform.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/DrawAutomaticUniform.cs), [DrawAutomaticUniformFactory.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/DrawAutomaticUniformFactory.cs), [DrawAutomaticUniformFactoryCollection.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/DrawAutomaticUniformFactoryCollection.cs) | Base classes (Listings 3.12 and 3.13) |
| [ModelViewPerspectiveMatrixUniform.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/ModelViewPerspectiveMatrixUniform.cs) + [Factory](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/ModelViewPerspectiveMatrixUniformFactory.cs) | The pattern every automatic uniform follows |
| [Wgs84HeightUniform.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/Wgs84HeightUniform.cs) + [Factory](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/DrawAutomaticUniforms/Wgs84HeightUniformFactory.cs) | Listing 3.15 |
| The other `Shaders/DrawAutomaticUniforms/*Uniform.cs` files | One `Set` body each. They become one-line registrations in [3.6](#36-the-standard-draw-automatic-uniforms). |
| [Shaders/ShaderProgram.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/ShaderProgram.cs) | `InitializeAutomaticUniforms`, `SetDrawAutomaticUniforms` |
| [Device.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Device.cs) | The static constructor, lines 45–84, registers every automatic uniform |
| [Shaders/Cache/ShaderCache.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/Cache/ShaderCache.cs), [CachedShaderProgram.cs](https://github.com/virtualglobebook/OpenGlobe/blob/master/Source/Renderer/Shaders/Cache/CachedShaderProgram.cs) | The shader cache (Listing 3.16) |

## Files

```
core/include/arda/core/geometry/
  ClipDepth.h                         NEW
  Vector4.h                           CHECK   the two #includes from Step 2
  Matrix4.h                           UPDATE  the math (full file shown)
renderer/
  include/arda/renderer/
    Device.h                          UPDATE  ClipDepthRange, automatic uniform registries, InitializeCommon
    scene/
      Camera.h                        NEW
      SceneState.h                    REWRITE
    shaders/
      AutomaticUniforms.h             NEW     LinkAutomaticUniform, DrawAutomaticUniform, DrawAutomaticUniformFactory,
                                              NamedCollection, FunctionDrawAutomaticUniform(Factory)
      ShaderProgram.h                 UPDATE  InitializeAutomaticUniforms, SetDrawAutomaticUniforms (full file shown)
      ShaderCache.h                   NEW
  src/
    Device.cpp                        UPDATE  InitializeCommon
    scene/Camera.cpp                  NEW
    scene/SceneState.cpp              NEW
    shaders/ShaderProgram.cpp         NEW
    shaders/ShaderCache.cpp           NEW
    shaders/automaticuniforms/
      ModelViewPerspectiveMatrixUniform.h     NEW
      Wgs84HeightUniform.h                    NEW
      StandardDrawAutomaticUniforms.h / .cpp  NEW   registers the rest of Table 3.1
    gl/
      DeviceGL3x.cpp                  UPDATE  call InitializeCommon, pass *this to programs
      shaders/ShaderProgramGL3x.h / .cpp  UPDATE  takes const Device&, sets automatic uniforms
tests/src/core/Matrix4Tests.cpp               NEW
tests/src/renderer/SceneStateTests.cpp        NEW
tests/src/renderer/AutomaticUniformTests.cpp  NEW
scene/src/main.cpp                            UPDATE  the milestone
```

---

## Background: from a vertex to a pixel

Everything in this step exists to compute one matrix per draw. Before any
code, here is what that matrix does.

> **Math note — the transform pipeline:** a vertex position goes through
> these coordinate systems, one matrix (or fixed-function step) at a time:
>
> ```
>  model coords ──M──▶ world coords ──V──▶ eye coords ──P──▶ clip coords
>      ──(÷ w)──▶ normalized device coords (NDC) ──viewport──▶ window coords (pixels)
> ```
>
> - **Model matrix `M`** places a mesh in the world. The triangle is authored
>   in its own coordinates; `M` could move, rotate or scale it. It's the
>   identity in this step. (`SceneState::modelMatrix`)
> - **View matrix `V`** moves the world so the camera sits at the origin
>   looking down −z, with +y up and +x right. This is "eye space".
>   (`SceneState::ViewMatrix()`, built by `LookAt`)
> - **Projection matrix `P`** maps the visible volume (the view frustum) into
>   a box. It writes the vertex into 4D **clip coordinates** `(x, y, z, w)`.
>   (`PerspectiveMatrix()` or `OrthographicMatrix()`)
> - **Perspective division:** the GPU divides x, y and z by w. The result is
>   **NDC**, where the visible box is x and y in [−1, 1] and z in the clip
>   depth range ([−1, 1] in GL, [0, 1] in D3D). For a perspective projection
>   `w = −z_eye`, the distance in front of the camera, so dividing by it makes
>   far things smaller. You never write this division; the rasterizer does it
>   after the vertex shader.
> - **Viewport transform:** NDC x and y are scaled to pixels using the
>   rectangle passed to `glViewport`, and NDC z is mapped into the depth range
>   set by `glDepthRange` (default [0, 1]). This is also fixed-function.
>
> The vertex shader's job is only to produce clip coordinates, so it needs
> `P · V · M`. With column vectors, the matrix nearest the vector is applied
> first: `P · V · M · v = P · (V · (M · v))`. That product is
> `og_modelViewPerspectiveMatrix`. It's computed once per draw on the CPU,
> instead of once per vertex on the GPU.

> **Math note — why "clip" coordinates:** clipping happens in this 4D space,
> before the division. A point is inside the frustum when
> `−w ≤ x ≤ w`, `−w ≤ y ≤ w` and `−w ≤ z ≤ w` (GL) or `0 ≤ z ≤ w` (D3D).
> Clipping before dividing avoids dividing by zero for points at the eye, and
> correctly handles triangles that cross behind the camera.

---

## Part 1: Core math

Everything here is in `arda_core`. It has no renderer or GL dependency, and
you can test it without a GPU.

> **Why:** `Matrix4` lives in core, like `Matrix4D` lives in `OpenGlobe.Core`.
> Chapter 4's tessellators, Chapter 5's precision code and the ellipsoid math
> all need matrices, and none of them should link against a graphics API.
> Keeping the math in core also keeps the tests fast: `Matrix4Tests` needs no
> window or context.

### 1.1 `ClipDepth.h`

After perspective division, OpenGL expects NDC z in [−1, 1]. Direct3D expects
[0, 1]. A projection matrix has to produce one or the other, so every function
that builds a projection takes a `ClipDepth`. The device reports which one its
API uses (`Device::ClipDepthRange()`, in [3.3](#33-device-the-registries-and-clipdepthrange)).

`core/include/arda/core/geometry/ClipDepth.h`:

```cpp
#pragma once

namespace arda::core {

// The z range of normalized device coordinates, after perspective division.
enum class ClipDepth {
    NegativeOneToOne,   // OpenGL: the near plane maps to -1, the far plane to +1
    ZeroToOne,          // Direct3D: the near plane maps to 0, the far plane to 1
};

} // namespace arda::core
```

> **OpenGL note — clip depth vs `glDepthRange`:** these are two different
> ranges. `ClipDepth` is the NDC z range the *projection matrix* must produce.
> `glDepthRange` (Step 1's `RenderState::depthRange`) is where the
> fixed-function viewport transform then puts that range in the depth buffer,
> by default [0, 1] in both APIs. GL 4.5 added `glClipControl(…,
> GL_ZERO_TO_ONE)` to switch GL to D3D's convention, but GL 3.3 doesn't have
> it, so the matrix has to adapt instead.

> **C++ note — `enum class`:** a reminder from [Step 0](00-setup.md): the
> enumerators are scoped (`ClipDepth::ZeroToOne`) and don't convert to `int`.

### 1.2 Check `Vector4.h`'s includes

`Matrix4.h` includes `Vector4.h`, which calls `std::sqrt` and throws
`std::runtime_error`. [Step 2, Part 1c](02-shaders.md) added `#include <cmath>`
and `#include <stdexcept>` to it. If you skipped that, add them now, after
`#pragma once` in `core/include/arda/core/geometry/Vector4.h`. A header should
include everything it uses, so it compiles no matter what was included before
it.

### 1.3 `Matrix4.h`: the full file

Step 2 created `Matrix4.h` with storage only: the 16-argument constructor,
`Identity`, element access, `Data()` and `==`. This step adds the math. The
file is short enough to show whole; replace
`core/include/arda/core/geometry/Matrix4.h` with the version below, then read
the notes that follow it.

```cpp
#pragma once

#include <arda/core/geometry/ClipDepth.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>

#include <array>
#include <cmath>
#include <concepts>
#include <numbers>
#include <stdexcept>
#include <type_traits>

namespace arda::core {

// 4x4 matrix stored column-major, like GL, GLSL and OpenGlobe's Matrix4D/Matrix4F.
// Element (column, row) is at index column * 4 + row.
template <std::floating_point T>
class Matrix4 {
public:
    constexpr Matrix4() = default;   // all zeros

    // Arguments are in reading order (row 0 first), like OpenGlobe's constructor,
    // so a matrix in source code looks like the matrix on paper.
    constexpr Matrix4(T c0r0, T c1r0, T c2r0, T c3r0,
                      T c0r1, T c1r1, T c2r1, T c3r1,
                      T c0r2, T c1r2, T c2r2, T c3r2,
                      T c0r3, T c1r3, T c2r3, T c3r3)
        : m_values{c0r0, c0r1, c0r2, c0r3,
                   c1r0, c1r1, c1r2, c1r3,
                   c2r0, c2r1, c2r2, c2r3,
                   c3r0, c3r1, c3r2, c3r3} {}

    // Converts from a matrix with another element type. Explicit, because
    // double -> float loses precision: Matrix4<float> f(someMatrix4d);
    template <std::floating_point U>
    constexpr explicit Matrix4(const Matrix4<U>& other) {
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                (*this)(column, row) = static_cast<T>(other(column, row));
            }
        }
    }

    static constexpr Matrix4 Identity() {
        return Matrix4(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       0, 0, 0, 1);
    }

    // Element access: m(column, row).
    constexpr T operator()(int column, int row) const { return m_values[column * 4 + row]; }
    constexpr T& operator()(int column, int row) { return m_values[column * 4 + row]; }

    // 16 values, column-major. Passed directly to glUniformMatrix4fv.
    constexpr const T* Data() const { return m_values.data(); }

    bool operator==(const Matrix4&) const = default;

    constexpr Matrix4 Transpose() const {
        Matrix4 result;
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                result(row, column) = (*this)(column, row);
            }
        }
        return result;
    }

    // Matrix4D.ToMatrix4F() becomes matrix.Cast<float>().
    template <std::floating_point U>
    constexpr Matrix4<U> Cast() const {
        return Matrix4<U>(*this);
    }

    // Matrix4D.CreatePerspectiveFieldOfView. Right-handed: the camera looks down -z.
    // fovy is the full vertical field of view in radians. zNear and zFar are
    // positive distances in front of the camera.
    static Matrix4 CreatePerspectiveFieldOfView(T fovy, T aspect, T zNear, T zFar, ClipDepth clipDepth) {
        if (fovy <= 0 || fovy > std::numbers::pi_v<T>) {
            throw std::out_of_range("fovy must be in (0, pi]");
        }
        if (aspect <= 0) {
            throw std::out_of_range("aspect must be greater than zero");
        }
        if (zNear <= 0 || zFar <= 0) {
            throw std::out_of_range("zNear and zFar must be greater than zero");
        }
        if (zNear == zFar) {
            throw std::out_of_range("zNear and zFar must be different");
        }

        const T f = 1 / std::tan(fovy / 2);   // cot(fovy / 2)

        // Row 2 maps eye z to clip z. See the math note below for both derivations.
        T a;
        T b;
        if (clipDepth == ClipDepth::NegativeOneToOne) {
            a = (zFar + zNear) / (zNear - zFar);
            b = (2 * zFar * zNear) / (zNear - zFar);
        } else {
            a = zFar / (zNear - zFar);
            b = (zFar * zNear) / (zNear - zFar);
        }

        return Matrix4(f / aspect, 0,  0, 0,
                       0,          f,  0, 0,
                       0,          0,  a, b,
                       0,          0, -1, 0);
    }

    // Matrix4D.CreateOrthographicOffCenter. zNear and zFar are distances along -z.
    static constexpr Matrix4 CreateOrthographicOffCenter(T left, T right, T bottom, T top,
                                                         T zNear, T zFar, ClipDepth clipDepth) {
        if (left == right || bottom == top || zNear == zFar) {
            throw std::out_of_range("the orthographic volume must not be empty");
        }

        const T a = 1 / (right - left);
        const T b = 1 / (top - bottom);
        const T c = 1 / (zFar - zNear);

        const T tx = -(right + left) * a;
        const T ty = -(top + bottom) * b;

        T sz;
        T tz;
        if (clipDepth == ClipDepth::NegativeOneToOne) {
            sz = -2 * c;
            tz = -(zFar + zNear) * c;
        } else {
            sz = -c;
            tz = -zNear * c;
        }

        return Matrix4(2 * a, 0,     0,  tx,
                       0,     2 * b, 0,  ty,
                       0,     0,     sz, tz,
                       0,     0,     0,  1);
    }

    // Matrix4D.LookAt: the view matrix of a camera at eye, looking at target.
    // Throws std::runtime_error (from Normalize) if eye == target, or if up is
    // parallel to the view direction.
    static Matrix4 LookAt(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up) {
        const Vector3<T> f = (target - eye).Normalize();   // forward
        const Vector3<T> s = f.Cross(up).Normalize();      // side (right)
        const Vector3<T> u = s.Cross(f).Normalize();       // true up

        const Matrix4 rotation( s.X(),  s.Y(),  s.Z(), 0,
                                u.X(),  u.Y(),  u.Z(), 0,
                               -f.X(), -f.Y(), -f.Z(), 0,
                                0,      0,      0,     1);
        const Matrix4 translation(1, 0, 0, -eye.X(),
                                  0, 1, 0, -eye.Y(),
                                  0, 0, 1, -eye.Z(),
                                  0, 0, 0, 1);
        return rotation * translation;
    }

private:
    std::array<T, 16> m_values{};
};

// Matrix product. (left * right) * v == left * (right * v).
template <std::floating_point T>
constexpr Matrix4<T> operator*(const Matrix4<T>& left, const Matrix4<T>& right) {
    Matrix4<T> result;
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            T sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += left(k, row) * right(column, k);   // row of left . column of right
            }
            result(column, row) = sum;
        }
    }
    return result;
}

// Transforms a column vector.
template <std::floating_point T>
constexpr Vector4<T> operator*(const Matrix4<T>& m, const Vector4<T>& v) {
    return Vector4<T>(
        m(0, 0) * v.X() + m(1, 0) * v.Y() + m(2, 0) * v.Z() + m(3, 0) * v.W(),
        m(0, 1) * v.X() + m(1, 1) * v.Y() + m(2, 1) * v.Z() + m(3, 1) * v.W(),
        m(0, 2) * v.X() + m(1, 2) * v.Y() + m(2, 2) * v.Z() + m(3, 2) * v.W(),
        m(0, 3) * v.X() + m(1, 3) * v.Y() + m(2, 3) * v.Z() + m(3, 3) * v.W());
}

// The GL and D3D backends copy Matrix4<float> straight into uniform storage.
static_assert(sizeof(Matrix4<float>) == 16 * sizeof(float));
static_assert(std::is_trivially_copyable_v<Matrix4<float>>);

} // namespace arda::core
```

The rest of this section explains the file, piece by piece.

#### Storage

> **Math note — column-major storage and column vectors:** a 4×4 matrix
> needs 16 numbers in memory, in some order. *Row-major* stores row 0, then
> row 1, and so on. *Column-major* stores column 0 first. GL, GLSL and
> OpenGlobe use column-major, and so does `Matrix4`: element (column c, row r)
> is at index `c * 4 + r`.
>
> ```
> matrix on paper          m_values (memory order)
> | 1  0  0  tx |          [ 1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 1, 0,   tx, ty, tz, 1 ]
> | 0  1  0  ty |            └ column 0 ┘  └ column 1 ┘  └ column 2 ┘  └── column 3 ──┘
> | 0  0  1  tz |
> | 0  0  0  1  |
> ```
>
> A translation ends up in the last four values. That's a quick way to check
> the layout in a debugger.
>
> The storage order is a separate question from the math convention. Arda
> uses **column vectors**: a point is a 4×1 column, it's transformed as
> `M · v`, and transforms compose right to left. GLSL's `M * v` means the
> same thing. The constructor takes its arguments in reading order (row 0
> first), so the code looks like the matrix on paper even though memory is
> column-major.

> **OpenGL note — `glUniformMatrix4fv` and `transpose = GL_FALSE`:** the
> Step 2 upload is
> `glUniformMatrix4fv(location, 1, GL_FALSE, m.Data())`. The third argument
> asks "is this data row-major, so GL should transpose it?" `Data()` is
> already column-major, which is what GLSL's `mat4` expects, so the answer is
> `GL_FALSE`. If the matrix came out transposed, the translation would land in
> the bottom row, and `gl_Position.w` would get garbage. Symptoms are nothing
> on screen, or geometry smeared toward a vanishing point. The test in
> [1.4](#14-matrix4tests) pins the layout down so this can't happen silently.

> **C++ note — class templates for math types:** `Matrix4` is a *class
> template*: a recipe for classes, not a class. `Matrix4<double>` and
> `Matrix4<float>` are two distinct types that the compiler stamps out from
> the recipe the first time each is used (this is *instantiation*). Every
> member function body is compiled separately for each `T`, with `T` replaced
> by `double` or `float`, so `T sum = 0; sum += a * b;` is plain double or
> float arithmetic with no overhead.
>
> Compare C#: OpenGlobe has two nearly identical classes, `Matrix4D` and
> `Matrix4F`. That's not an accident. Before .NET 7's generic math, a C#
> generic couldn't write `a * b` for a type parameter, because generics are
> checked once, against their constraints, and no constraint said "has `*`".
> So `Matrix42<T>` in OpenGlobe is generic but only *stores* values, while the
> matrices with math are written out per type. C++ templates are checked
> after substitution, per instantiation, so one template covers both.
>
> Two consequences you'll run into:
>
> - **Templates live in headers.** The compiler needs the full body wherever
>   `Matrix4<double>` is first used, so there is no `Matrix4.cpp`.
> - **The constraint `template <std::floating_point T>`** (a concept; see
>   [Step 3](03-vertex-data.md)) rejects `Matrix4<int>` with a clear error.
>   Integer matrices would silently truncate `1 / std::tan(...)` to 0.
>
> Code that uses one type a lot can name it with an alias, as `SceneState.cpp`
> does: `using Matrix4D = core::Matrix4<double>;`.

> **C++ note — `operator()` for element access, and why not `operator[]`:**
> C++ lets a class define what `m(1, 2)` means by writing
> `operator()(int column, int row)`. There are two overloads: the `const` one
> returns a copy and is used on `const` matrices; the non-`const` one returns
> `T&`, a reference to the stored value, so `m(3, 0) = 5.0;` assigns into the
> matrix. The two differ only in `const`, and C++ picks one based on whether
> the object itself is `const`.
>
> `operator[]` would read more naturally, but before C++23 it could only take
> one argument. `m[1][2]` would need a proxy "column" object, and `m[1, 2]`
> doesn't compile in C++20 (the comma is the comma operator). `operator()` is
> the usual C++20 workaround and is what Eigen uses too. C# indexers
> (`this[int c, int r]`) have no such limit.

#### Multiplication

> **C++ note — operator overloading, member vs free:** in C#, operators are
> always `public static` methods: `public static Matrix4D operator *(Matrix4D
> left, Matrix4D right)`. C++ gives you two choices:
>
> ```cpp
> // 1. Member: the left operand is *this.
> Matrix4 operator*(const Matrix4& right) const;
>
> // 2. Free (non-member) function: both operands are parameters.
> template <std::floating_point T>
> Matrix4<T> operator*(const Matrix4<T>& left, const Matrix4<T>& right);
> ```
>
> Both make `a * b` work. The free form is closer to C#'s static operator,
> treats both operands the same way, and is the only option when the left
> operand isn't your class (for example `2.0 * vector`; core's `Vector3.h` has
> exactly this free `operator*` for scalar-times-vector). `Matrix4` uses free
> functions for both of its `operator*`s. `operator()` above has to be a
> member; C++ requires that for `()`, `[]`, `=` and `->`.
>
> Because the free `operator*` is a template, `T` is deduced from both
> arguments and must match. `Matrix4<double> * Matrix4<float>` fails to
> compile, which is what you want: mixing precisions should take an explicit
> `Cast`. Rules of thumb for operators: return new values by value (as here),
> take inputs by `const&`, and only overload an operator when its meaning is
> obvious. `*` for matrix product is obvious; `*` for "transpose" would not be.

> **Math note — the product formula:** entry (row r, column c) of `L · R` is
> the dot product of row r of `L` with column c of `R`:
> `Σₖ L[r][k] · R[k][c]`. In `Matrix4`'s `(column, row)` accessor that's
> `left(k, row) * right(column, k)`, which is exactly the inner loop.
> Matrix products aren't commutative: `P · V` and `V · P` are different
> matrices, so multiplication order is the thing to double-check whenever a
> scene looks wrong.

> **C++ note — `constexpr` functions:** `constexpr` on a function means "this
> *can* run at compile time, if its arguments are known at compile time". It
> still runs normally at run time otherwise. `Identity`, `Transpose`, `Cast`,
> both `operator*`s and `CreateOrthographicOffCenter` are all `constexpr`, so
> a test can check them with no running program at all:
>
> ```cpp
> constexpr Matrix4<double> scale(2, 0, 0, 0,  0, 3, 0, 0,  0, 0, 4, 0,  0, 0, 0, 1);
> static_assert(scale * Matrix4<double>::Identity() == scale);   // checked by the compiler
> ```
>
> A `constexpr` function may contain loops, local variables and even a
> `throw`, as long as the `throw` isn't *reached* during compile-time
> evaluation (reaching it is a compile error, which makes it a compile-time
> assertion). What it can't do is call non-`constexpr` functions on the path
> it takes. `std::tan` and `std::sqrt` aren't `constexpr` until C++26, so
> `CreatePerspectiveFieldOfView` and `LookAt` (via `Normalize`) are ordinary
> functions. `constexpr` functions are implicitly `inline`
> (see [Step 1](01-state-management.md)), which is also why they can live in a
> header.
>
> C# has `const` fields but no compile-time function evaluation, so this has
> no direct C# equivalent.

#### Converting between `Matrix4<double>` and `Matrix4<float>`

The CPU does its math in doubles and the GPU gets floats (the reasons are in
[2.2](#22-scenestate)). The conversion is the last step before upload:
`sceneState.ModelViewPerspectiveMatrix(clipDepth).Cast<float>()`.

> **C++ note — converting constructors and `explicit`:** a constructor that
> takes one argument of another type is a *converting constructor*. Without
> `explicit`, C++ would use it silently:
>
> ```cpp
> Matrix4<float> f = someDoubleMatrix;        // compiles only if the constructor is NOT explicit
> uniform.SetValue(someDoubleMatrix);         // same: an implicit double -> float conversion
> ```
>
> Silent precision loss is exactly the bug a virtual globe can't afford, so
> the constructor is `explicit`. Now only these compile:
>
> ```cpp
> Matrix4<float> f(someDoubleMatrix);              // direct initialization
> auto g = someDoubleMatrix.Cast<float>();         // named function, like C#'s ToMatrix4F()
> auto h = static_cast<Matrix4<float>>(someDoubleMatrix);
> ```
>
> The constructor is itself a template (`template <std::floating_point U>`),
> so one definition converts from any floating-point matrix. When `U` is the
> same as `T`, the compiler prefers the ordinary (implicit) copy constructor,
> because a non-template beats a template when both match equally. Mark every
> single-argument constructor `explicit` unless you *want* the implicit
> conversion. C#'s rule is the reverse: user-defined conversions are declared
> `implicit` or `explicit` on an operator, and a constructor never converts.
>
> One gotcha: inside *another* template, where the matrix type depends on a
> template parameter, calling `Cast` needs the `template` keyword:
> `m.template Cast<float>()`. Without it the compiler parses `<` as
> less-than. Outside templates (all of this step's uses) you don't need it.

#### Perspective projection

> **Math note — the perspective matrix:** with `f = cot(fovy / 2)` and `a`,
> `b` depending on the clip depth range:
>
> ```
> | f/aspect  0   0   0 |   | x_e |     | (f/aspect) x_e |
> | 0         f   0   0 | · | y_e |  =  | f y_e          |
> | 0         0   a   b |   | z_e |     | a z_e + b      |
> | 0         0  -1   0 |   | 1   |     | -z_e           |
> ```
>
> - **x and y:** after dividing by `w = −z_e`, `y_ndc = f · y_e / −z_e`. A
>   point on the top edge of the frustum has `y_e / −z_e = tan(fovy / 2)`, so
>   `y_ndc = cot · tan = 1`. The top edge lands on the top of the screen.
>   `x` is the same, divided by `aspect` so that a square in eye space stays
>   square in pixels on a wide viewport.
> - **z:** pick `a` and `b` so the near plane (`z_e = −n`) and the far plane
>   (`z_e = −f`) land at the ends of the clip depth range. NDC z is
>   `(a z_e + b) / −z_e`.
>   - **`NegativeOneToOne` (GL):** solve `(−a n + b)/n = −1` and
>     `(−a f + b)/f = 1`. This gives `a = (f + n)/(n − f)` and
>     `b = 2fn/(n − f)`, which is OpenGlobe's (and `gluPerspective`'s) matrix.
>   - **`ZeroToOne` (D3D):** solve `(−a n + b)/n = 0` and `(−a f + b)/f = 1`.
>     The first gives `b = a n`, then `a = f/(n − f)` and `b = fn/(n − f)`.
>     This is the same right-handed projection with depth remapped, not
>     D3DX's left-handed matrix.
>
>   Both are checked by a test that projects points on the near and far
>   planes.
> - **Depth is not linear.** NDC z is a function of `1/z_e`, so most of the
>   depth range is spent near the near plane. With the default near distance
>   of 0.01 and far distance of 64 that's fine. At planetary scale it isn't,
>   and that's Chapter 6 (depth buffer precision).

> **C++ note — `std::numbers::pi` and `pi_v<T>`:** C++20's `<numbers>` header
> provides mathematical constants: `std::numbers::pi`, `e`, `sqrt2` and
> others. Before C++20, code used `M_PI`, which isn't standard (MSVC hides it
> behind `_USE_MATH_DEFINES`), or wrote its own. Each constant is a `double`;
> the `_v` variable templates give you the constant in another type:
> `std::numbers::pi_v<float>`. `Matrix4<T>` compares `fovy` against
> `std::numbers::pi_v<T>`, so the float version compares float to float
> instead of promoting to double. It's the equivalent of C#'s `Math.PI`
> (`double` only) and `MathF.PI`.

#### Orthographic projection

An orthographic projection has no perspective division effect: `w` stays 1,
so size doesn't shrink with distance. It maps the box
`[left, right] × [bottom, top] × [−zNear, −zFar]` to the NDC box. It's used
for 2D overlays and heads-up displays in later chapters.

> **Math note — the orthographic matrix:** each axis is an independent
> scale-and-offset. For x: `x_ndc = 2(x − left)/(right − left) − 1`, which is
> `2a · x + tx` with `a = 1/(right − left)` and `tx = −(right + left) a`. y is
> the same. For z, the near plane `z_e = −n` must map to the start of the clip
> depth range and `z_e = −f` to 1:
>
> - `NegativeOneToOne`: `sz = −2/(f − n)`, `tz = −(f + n)/(f − n)` (OpenGlobe's).
> - `ZeroToOne`: `sz = −1/(f − n)`, `tz = −n/(f − n)`.
>
> Plug in `z_e = −n` and `z_e = −f` to check both.

#### `LookAt`

> **Math note — building the view matrix:** the view matrix must move the
> eye to the origin, then rotate so the view direction becomes −z and "up"
> becomes +y. `LookAt` builds an orthonormal basis:
>
> 1. `f = normalize(target − eye)`: the direction the camera looks.
> 2. `s = normalize(f × up)`: the camera's right. The `up` you pass in only
>    needs to be roughly up; the cross product discards the part of it along
>    `f`. If `up` is parallel to `f`, the cross product is zero and
>    `Normalize` throws.
> 3. `u = s × f`: the true up, perpendicular to both.
>
> The translation `T(−eye)` moves the eye to the origin. Then a rotation whose
> *rows* are `s`, `u` and `−f` takes those world directions to the x, y and z
> axes. (For an orthonormal basis, the matrix with the basis as rows is the
> inverse of the one with it as columns, which is why rows appear here.) The
> result is `rotation · translation`: translate first, then rotate.

`Matrix4` has no inverse. Nothing in Chapter 3 needs one, because every
matrix here is built directly. Add a general 4×4 inverse when a later chapter
needs one (picking, for example).

### 1.4 `Matrix4Tests`

`tests/src/core/Matrix4Tests.cpp`:

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Matrix4.h>

#include <initializer_list>
#include <numbers>
#include <utility>

using arda::core::ClipDepth;
using arda::core::Matrix4;
using arda::core::Vector4;

namespace {
// Compile-time checks: these run inside the compiler, not the test runner.
constexpr Matrix4<double> kScale(2, 0, 0, 0,
                                 0, 3, 0, 0,
                                 0, 0, 4, 0,
                                 0, 0, 0, 1);
static_assert(kScale * Matrix4<double>::Identity() == kScale);
static_assert(kScale.Transpose() == kScale);
static_assert(kScale.Cast<float>()(1, 1) == 3.0f);
} // namespace

TEST_CASE("Matrix4 stores values column-major") {
    const Matrix4<double> m(1, 2, 3, 4,
                            5, 6, 7, 8,
                            9, 10, 11, 12,
                            13, 14, 15, 16);
    CHECK(m(0, 1) == 5);        // column 0, row 1
    CHECK(m.Data()[1] == 5);    // second value in memory
    CHECK(m(3, 0) == 4);
    CHECK(m.Data()[12] == 4);   // column 3 starts at index 12
    CHECK(m.Transpose()(0, 3) == 4);
    CHECK(m.Transpose().Transpose() == m);
}

TEST_CASE("Matrix4 multiplication composes right to left") {
    const Matrix4<double> translate(1, 0, 0, 10,
                                    0, 1, 0, 0,
                                    0, 0, 1, 0,
                                    0, 0, 0, 1);
    const Matrix4<double> scale(2, 0, 0, 0,
                                0, 2, 0, 0,
                                0, 0, 2, 0,
                                0, 0, 0, 1);
    const Vector4<double> p(1, 0, 0, 1);

    // translate * scale: scale first, then translate. (1 * 2) + 10 = 12.
    CHECK((translate * scale * p).X() == doctest::Approx(12));
    // scale * translate: translate first, then scale. (1 + 10) * 2 = 22.
    CHECK((scale * translate * p).X() == doctest::Approx(22));
    // Associativity: (A * B) * v == A * (B * v).
    CHECK((translate * scale) * p == translate * (scale * p));
    CHECK(translate * Matrix4<double>::Identity() == translate);
}

TEST_CASE("Cast converts element by element") {
    const Matrix4<double> d(1.5, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 1);
    const Matrix4<float> f = d.Cast<float>();
    CHECK(f(0, 0) == 1.5f);
    CHECK(Matrix4<float>(d) == f);   // the explicit constructor does the same
}

TEST_CASE("LookAt puts the eye at the origin, looking down -z") {
    const auto view = Matrix4<double>::LookAt({0, -1, 0}, {0, 0, 0}, {0, 0, 1});

    const auto eye = view * Vector4<double>(0, -1, 0, 1);
    CHECK(eye.X() == doctest::Approx(0));
    CHECK(eye.Y() == doctest::Approx(0));
    CHECK(eye.Z() == doctest::Approx(0));

    const auto target = view * Vector4<double>(0, 0, 0, 1);
    CHECK(target.Z() == doctest::Approx(-1));   // one unit in front of the camera

    const auto up = view * Vector4<double>(0, 0, 1, 0);   // a direction: w = 0
    CHECK(up.Y() == doctest::Approx(1));

    const auto right = view * Vector4<double>(1, 0, 0, 0);
    CHECK(right.X() == doctest::Approx(1));

    CHECK_THROWS(Matrix4<double>::LookAt({0, 0, 0}, {0, 0, 0}, {0, 0, 1}));   // eye == target
}

TEST_CASE("Perspective maps the near and far planes to the clip depth range") {
    const double fovy = std::numbers::pi / 2;
    for (auto [clipDepth, expectedNear] : {std::pair{ClipDepth::NegativeOneToOne, -1.0},
                                           std::pair{ClipDepth::ZeroToOne, 0.0}}) {
        const auto p = Matrix4<double>::CreatePerspectiveFieldOfView(fovy, 1, 1, 10, clipDepth);

        const auto nearPoint = p * Vector4<double>(0, 0, -1, 1);
        const auto farPoint = p * Vector4<double>(0, 0, -10, 1);
        CHECK(nearPoint.Z() / nearPoint.W() == doctest::Approx(expectedNear));
        CHECK(farPoint.Z() / farPoint.W() == doctest::Approx(1.0));
        CHECK(farPoint.W() == doctest::Approx(10.0));   // w = -z_eye

        // With a 90 degree field of view, the top edge at distance 5 is y = 5.
        const auto top = p * Vector4<double>(0, 5, -5, 1);
        CHECK(top.Y() / top.W() == doctest::Approx(1.0));
    }

    CHECK_THROWS_AS(Matrix4<double>::CreatePerspectiveFieldOfView(0, 1, 1, 10, ClipDepth::NegativeOneToOne),
                    std::out_of_range);
    CHECK_THROWS_AS(Matrix4<double>::CreatePerspectiveFieldOfView(fovy, 1, 0, 10, ClipDepth::NegativeOneToOne),
                    std::out_of_range);
}

TEST_CASE("Orthographic maps the box to NDC") {
    for (auto [clipDepth, expectedNear] : {std::pair{ClipDepth::NegativeOneToOne, -1.0},
                                           std::pair{ClipDepth::ZeroToOne, 0.0}}) {
        const auto o = Matrix4<double>::CreateOrthographicOffCenter(0, 4, 0, 2, 1, 5, clipDepth);

        const auto nearCorner = o * Vector4<double>(0, 0, -1, 1);
        CHECK(nearCorner.X() == doctest::Approx(-1));
        CHECK(nearCorner.Y() == doctest::Approx(-1));
        CHECK(nearCorner.Z() == doctest::Approx(expectedNear));
        CHECK(nearCorner.W() == doctest::Approx(1));

        const auto farCorner = o * Vector4<double>(4, 2, -5, 1);
        CHECK(farCorner.X() == doctest::Approx(1));
        CHECK(farCorner.Y() == doctest::Approx(1));
        CHECK(farCorner.Z() == doctest::Approx(1));
    }
}
```

> **C++ note — structured bindings:** `auto [clipDepth, expectedNear] = pair;`
> unpacks a pair, tuple, array or simple struct into named variables. In the
> range-`for`, each element of the braced list is a `std::pair`, unpacked on
> every iteration. It's like C# tuple deconstruction:
> `var (clipDepth, expectedNear) = pair;`.

Add the test file to `tests/CMakeLists.txt`, and list the new header in
`core/CMakeLists.txt` (it lists headers for the IDE):

```cmake
# core/CMakeLists.txt, in add_library(arda_core ...)
    include/arda/core/geometry/ClipDepth.h
    include/arda/core/geometry/Matrix4.h

# tests/CMakeLists.txt, in add_executable(arda_tests ...)
    src/core/Matrix4Tests.cpp
```

> **Checkpoint:** build and run `arda_tests`. The `static_assert`s are checked
> while compiling `Matrix4Tests.cpp`; the rest run with the other tests. If
> "column-major" fails, the constructor's initializer list is wrong. If a
> projection test fails, compare `a` and `b` against the math note.

---

## Part 2: `Camera` and `SceneState`

### 2.1 `Camera`

`Camera.cs` is a bag of settable properties plus a few computed ones. The
settable properties become public fields, following the README's convention
for plain data (camelCase fields), and the computed ones become `const` member
functions. `SaveView` and `LoadView` (XML) aren't ported.

`renderer/include/arda/renderer/scene/Camera.h`:

```cpp
#pragma once

#include <arda/core/geometry/Vector3.h>

#include <numbers>

namespace arda::core::geometry {
class Ellipsoid;
} // namespace arda::core::geometry

namespace arda::renderer {

// Camera.cs. The settable properties are public fields, and the computed ones are functions.
// Angles are in radians and distances are in the same units as the scene (meters on a globe).
struct Camera {
    core::Vector3<double> eye{0.0, -1.0, 0.0};
    core::Vector3<double> target{0.0, 0.0, 0.0};
    core::Vector3<double> up{0.0, 0.0, 1.0};

    double fieldOfViewY = std::numbers::pi / 6.0;   // 30 degrees
    double aspectRatio = 1.0;                       // viewport width / height

    double perspectiveNearPlaneDistance = 0.01;
    double perspectiveFarPlaneDistance = 64.0;

    double orthographicNearPlaneDistance = 0.0;
    double orthographicFarPlaneDistance = 1.0;
    double orthographicLeft = 0.0;
    double orthographicRight = 1.0;
    double orthographicBottom = 0.0;
    double orthographicTop = 1.0;

    core::Vector3<double> Forward() const;   // unit vector from eye toward target
    core::Vector3<double> Right() const;     // unit vector, Forward x up
    double FieldOfViewX() const;             // derived from fieldOfViewY and aspectRatio
    double OrthographicDepth() const;        // |far - near| of the orthographic volume

    // The eye split into a float "high" part and a float "low" remainder, for
    // Chapter 5's GPU relative-to-eye technique.
    core::Vector3<float> EyeHigh() const;
    core::Vector3<float> EyeLow() const;

    // Moves the eye along the target-to-eye direction until a sphere of this
    // radius, centered on the target, just fits in the view.
    void ZoomToTarget(double radius);

    // Height of the eye above the ellipsoid (Listing 3.15 calls this Altitude).
    double Height(const core::geometry::Ellipsoid& shape) const;
};

} // namespace arda::renderer
```

`Camera.h` only mentions `Ellipsoid` by reference, so a forward declaration is
enough, and every file that includes `Camera.h` is spared `Ellipsoid.h` and
its includes. (Forward declarations: [Step 0](00-setup.md).)

> **Math note — `FieldOfViewX`:** the vertical and horizontal half-angles
> share the same distance to the image plane, and the image's width is
> `aspect` times its height, so `tan(fovx/2) = aspect · tan(fovy/2)`. Hence
> `fovx = 2 · atan(aspect · tan(fovy/2))`. Note that it's *not*
> `aspect · fovy`; angles don't scale linearly.

> **Math note — `ZoomToTarget`:** a sphere of radius `r` at distance `d`
> touches the edges of a view cone with half-angle `θ` when `sin θ = r/d`.
> Using the narrower of the two fields of view makes the sphere fit in both
> directions, so `d = r / sin(min(fovx, fovy)/2)`. The eye moves to that
> distance along its current direction from the target. With the defaults and
> `r = 1`, `d = 1/sin(15°) ≈ 3.86`.

`renderer/src/scene/Camera.cpp`:

```cpp
#include <arda/renderer/scene/Camera.h>

#include <arda/core/Geodetic3D.h>
#include <arda/core/geometry/Ellipsoid.h>

#include <algorithm>
#include <cmath>

namespace arda::renderer {

using core::Vector3;

Vector3<double> Camera::Forward() const {
    return (target - eye).Normalize();
}

Vector3<double> Camera::Right() const {
    return Forward().Cross(up).Normalize();
}

double Camera::FieldOfViewX() const {
    return 2.0 * std::atan(aspectRatio * std::tan(fieldOfViewY * 0.5));
}

double Camera::OrthographicDepth() const {
    return std::abs(orthographicFarPlaneDistance - orthographicNearPlaneDistance);
}

Vector3<float> Camera::EyeHigh() const {
    return Vector3<float>(static_cast<float>(eye.X()),
                          static_cast<float>(eye.Y()),
                          static_cast<float>(eye.Z()));
}

Vector3<float> Camera::EyeLow() const {
    const Vector3<float> high = EyeHigh();
    return Vector3<float>(static_cast<float>(eye.X() - high.X()),
                          static_cast<float>(eye.Y() - high.Y()),
                          static_cast<float>(eye.Z() - high.Z()));
}

void Camera::ZoomToTarget(double radius) {
    const Vector3<double> toEye = (eye - target).Normalize();
    const double sine = std::sin(std::min(FieldOfViewX(), fieldOfViewY) * 0.5);
    const double distance = radius / sine;
    eye = target + toEye * distance;
}

double Camera::Height(const core::geometry::Ellipsoid& shape) const {
    return shape.ToGeodetic3D(eye).Altitude();
}

} // namespace arda::renderer
```

`EyeLow` subtracts in double: `eye.X() - high.X()` promotes the float to
double first, so the remainder is exact before it's rounded to float.

### 2.2 `SceneState`

`SceneState` (Listing 3.14) is what automatic uniforms read: the camera, the
model matrix, the sun and lighting parameters, and functions that compute the
matrices from them. Step 3 declared an empty `class SceneState {};` so that
`Context::Draw`'s signature wouldn't change. Now it's filled in.

Every matrix function of `SceneState.cs` is here except two:

- `ModelZToClipCoordinates` returns a `Matrix42` (GLSL `mat4x2`), which needs
  a new matrix type and a new uniform type in both backends. Chapter 4's
  ray-casted globe is the first user; add it then.
- `HighResolutionSnapScale` is for high-resolution screenshots (later
  chapters).

The projection functions take a `ClipDepth`, because the right matrix depends
on the API. Automatic uniforms get it from
`context.GetDevice().ClipDepthRange()`.

> **Math note — why doubles on the CPU, floats on the GPU:** a float has a
> 24-bit significand, so between 4,194,304 and 8,388,608 the gap between
> neighboring floats is 0.5. Earth's equatorial radius is 6,378,137 m. A
> vertex on the surface stored as a float can only be placed to the nearest
> half meter, and a camera position in floats snaps by the same amount. When
> you zoom in to street level, vertices visibly jump around as the camera
> moves ("jitter").
>
> Doubles have a 53-bit significand: sub-micrometer resolution at that
> distance. So the camera, the model matrix and every product (`V · M`,
> `P · V · M`) stay in doubles. Only the *final* matrix is converted to float
> for the GPU, because GLSL 3.30 has no doubles and float math is what GPUs
> are fast at. Converting last matters: the view matrix contains −eye (about
> 6.4 million) in its translation, and the model matrix may contain a similar
> translation that cancels it. In doubles, that cancellation is exact. In
> floats it would lose the half-meter first.
>
> Converting last still leaves a float matrix multiplied by float positions on
> the GPU, so for full precision Chapter 5 goes further. *Relative to eye*
> rendering subtracts the eye position on the CPU in double, so the GPU only
> ever sees small numbers. `ModelViewMatrixRelativeToEye` below is that
> chapter's matrix: the model-view matrix with its translation removed.

`renderer/include/arda/renderer/scene/SceneState.h`:

```cpp
#pragma once

#include <arda/core/geometry/ClipDepth.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/scene/Camera.h>

#include <optional>

namespace arda::renderer {

// SceneState.cs (Listing 3.14): everything draw automatic uniforms read.
class SceneState {
public:
    float diffuseIntensity = 0.65f;
    float specularIntensity = 0.25f;
    float ambientIntensity = 0.10f;
    float shininess = 12.0f;

    Camera camera;
    core::Vector3<double> sunPosition{200000.0, 0.0, 0.0};
    core::Matrix4<double> modelMatrix = core::Matrix4<double>::Identity();

    core::Vector3<double> CameraLightPosition() const { return camera.eye; }

    // Projections (eye -> clip).
    core::Matrix4<double> PerspectiveMatrix(core::ClipDepth clipDepth) const;
    core::Matrix4<double> OrthographicMatrix(core::ClipDepth clipDepth) const;

    // World -> eye, and model -> eye.
    core::Matrix4<double> ViewMatrix() const;
    core::Matrix4<double> ModelViewMatrix() const;
    core::Matrix4<double> ModelViewMatrixRelativeToEye() const;   // Chapter 5

    // Model -> clip.
    core::Matrix4<double> ModelViewPerspectiveMatrix(core::ClipDepth clipDepth) const;
    core::Matrix4<double> ModelViewPerspectiveMatrixRelativeToEye(core::ClipDepth clipDepth) const;   // Chapter 5
    core::Matrix4<double> ModelViewOrthographicMatrix(core::ClipDepth clipDepth) const;

    // NDC -> window coordinates (pixels), and window coordinates -> clip, for a viewport.
    static core::Matrix4<double> ComputeViewportTransformationMatrix(
        const Rectangle& viewport, double nearDepthRange, double farDepthRange, core::ClipDepth clipDepth);
    static core::Matrix4<double> ComputeViewportOrthographicMatrix(
        const Rectangle& viewport, core::ClipDepth clipDepth);

    // Add from SceneState.cs when a later chapter needs them:
    //   ModelZToClipCoordinates (Chapter 4, needs Matrix42)
    //   HighResolutionSnapScale (high-resolution screenshots)

private:
    // Lazily computed matrices, reused while their inputs are unchanged.
    struct ViewKey {
        core::Vector3<double> eye;
        core::Vector3<double> target;
        core::Vector3<double> up;
        bool operator==(const ViewKey&) const = default;
    };

    struct PerspectiveKey {
        double fieldOfViewY;
        double aspectRatio;
        double nearPlaneDistance;
        double farPlaneDistance;
        core::ClipDepth clipDepth;
        bool operator==(const PerspectiveKey&) const = default;
    };

    template <typename Key>
    struct CachedMatrix {
        std::optional<Key> key;   // empty until the first computation
        core::Matrix4<double> value;
    };

    mutable CachedMatrix<ViewKey> m_viewMatrix;
    mutable CachedMatrix<PerspectiveKey> m_perspectiveMatrix;
};

} // namespace arda::renderer
```

`SceneState` keeps its public fields (`sceneState.camera.aspectRatio = …` is
used by every later step), and gains a private cache.

> **C++ note — `mutable` and lazy caching:** a `const` member function
> promises not to change the object, so the compiler rejects assignments to
> members inside it. But some changes are invisible from outside: caching a
> result you'll return again is one. The observable value, "the view matrix
> for the current camera", is the same whether it's recomputed or reused.
> This is called *logical constness* (vs. *bitwise constness*). `mutable`
> marks a member that `const` functions may modify, for exactly this case:
>
> ```cpp
> class Circle {
> public:
>     double Area() const {
>         if (!m_area) { m_area = std::numbers::pi * m_radius * m_radius; }   // OK: m_area is mutable
>         return *m_area;
>     }
>     void SetRadius(double r) { m_radius = r; m_area.reset(); }   // invalidate
> private:
>     double m_radius = 1.0;
>     mutable std::optional<double> m_area;
> };
> ```
>
> `Circle` can invalidate its cache in a setter because its data is private.
> `SceneState`'s camera is a public field, so there is no setter to hook.
> Instead, each cache stores the *inputs* it was computed from (the key) and
> compares them on every call. Comparing a few doubles is much cheaper than
> `LookAt` (three square roots) or `tan`.
>
> Two rules keep `mutable` honest:
>
> - **Only for state that doesn't change the observable result.** Never use
>   it to sneak real modifications past `const`.
> - **Watch threads.** The C++ standard library treats `const` member
>   functions as safe to call from several threads at once. A `mutable` cache
>   breaks that: two threads calling `ViewMatrix()` on the same `SceneState`
>   would race on `m_viewMatrix`. The renderer only uses a `SceneState` on the
>   rendering thread, so this is fine for now. Chapter 10's multithreading
>   would need a mutex or per-thread `SceneState` copies.
>
> Also note what `const` does *not* cover. In `const std::unique_ptr<T>& p`,
> the pointer is `const` but `*p` isn't. You'll see that in
> `InitializeAutomaticUniforms` below, where a `const` loop still gets a
> mutable `UniformBase&`. C# has no `const` methods at all; C# properties with
> lazy backing fields are the closest thing to this pattern.

> **Why:** is the cache worth it? `SceneState.cs` recomputes everything on
> every call, and it works. But one draw can have several automatic uniforms
> that each need the view matrix (`og_viewMatrix`, `og_modelViewMatrix`,
> `og_modelViewPerspectiveMatrix`, …), and a scene issues many draws per
> frame with the same camera. The cache makes the repeat calls a comparison
> and a copy. It is still a small saving; the main reason it's here is that it
> is the textbook use of `mutable`, and the alternative
> (making `camera` private with setters that invalidate) would break the
> public-field API the other steps use.

> **C++ note — defaulted `operator==` and `!=`:** a reminder from
> [Step 1](01-state-management.md): `= default` compares member by member.
> `ViewKey`'s default needs `Vector3::operator==`, which Step 2 added. In
> C++20 you only write `==`; the compiler rewrites `a != b` as `!(a == b)`.
> `std::optional<Key> != Key` works too: an empty optional is unequal to every
> key, so the first call always computes.

`renderer/src/scene/SceneState.cpp`:

```cpp
#include <arda/renderer/scene/SceneState.h>

namespace arda::renderer {

using core::ClipDepth;
using Matrix4D = core::Matrix4<double>;

Matrix4D SceneState::PerspectiveMatrix(ClipDepth clipDepth) const {
    const PerspectiveKey key{camera.fieldOfViewY, camera.aspectRatio,
                             camera.perspectiveNearPlaneDistance, camera.perspectiveFarPlaneDistance,
                             clipDepth};
    if (m_perspectiveMatrix.key != key) {
        // Compute first, then store the key. If this throws (bad camera
        // values), the cache is left as it was.
        m_perspectiveMatrix.value = Matrix4D::CreatePerspectiveFieldOfView(
            key.fieldOfViewY, key.aspectRatio, key.nearPlaneDistance, key.farPlaneDistance, clipDepth);
        m_perspectiveMatrix.key = key;
    }
    return m_perspectiveMatrix.value;
}

// SceneState.cs passes top and bottom swapped ("MS -> OpenGL"): the camera's
// orthographic top/bottom are in window coordinates, with y growing downward.
// This keeps that behavior, so with the defaults (bottom 0, top 1) y is flipped.
Matrix4D SceneState::OrthographicMatrix(ClipDepth clipDepth) const {
    return Matrix4D::CreateOrthographicOffCenter(
        camera.orthographicLeft, camera.orthographicRight,
        camera.orthographicTop, camera.orthographicBottom,
        camera.orthographicNearPlaneDistance, camera.orthographicFarPlaneDistance, clipDepth);
}

Matrix4D SceneState::ViewMatrix() const {
    const ViewKey key{camera.eye, camera.target, camera.up};
    if (m_viewMatrix.key != key) {
        m_viewMatrix.value = Matrix4D::LookAt(key.eye, key.target, key.up);
        m_viewMatrix.key = key;
    }
    return m_viewMatrix.value;
}

Matrix4D SceneState::ModelViewMatrix() const {
    return ViewMatrix() * modelMatrix;
}

// The model-view matrix without its translation column (Chapter 5).
// Positions are made relative to the eye on the CPU, in double, before upload.
Matrix4D SceneState::ModelViewMatrixRelativeToEye() const {
    const Matrix4D m = ModelViewMatrix();
    return Matrix4D(m(0, 0), m(1, 0), m(2, 0), 0.0,
                    m(0, 1), m(1, 1), m(2, 1), 0.0,
                    m(0, 2), m(1, 2), m(2, 2), 0.0,
                    m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

Matrix4D SceneState::ModelViewPerspectiveMatrix(ClipDepth clipDepth) const {
    return PerspectiveMatrix(clipDepth) * ModelViewMatrix();
}

Matrix4D SceneState::ModelViewPerspectiveMatrixRelativeToEye(ClipDepth clipDepth) const {
    return PerspectiveMatrix(clipDepth) * ModelViewMatrixRelativeToEye();
}

// SceneState.cs computes ModelViewMatrix * OrthographicMatrix, which applies the
// projection first, in model space. The correct order, the same as the
// perspective version, is projection * model-view. No OpenGlobe shader uses
// og_modelViewOrthographicMatrix, which is why the C# order went unnoticed.
Matrix4D SceneState::ModelViewOrthographicMatrix(ClipDepth clipDepth) const {
    return OrthographicMatrix(clipDepth) * ModelViewMatrix();
}

// Maps NDC to window coordinates: x and y to pixels in the viewport, and z to
// [nearDepthRange, farDepthRange] (the glDepthRange values).
Matrix4D SceneState::ComputeViewportTransformationMatrix(
    const Rectangle& viewport, double nearDepthRange, double farDepthRange, ClipDepth clipDepth) {
    const double halfWidth = viewport.width * 0.5;
    const double halfHeight = viewport.height * 0.5;

    // GL: NDC z in [-1, 1] -> depth = near + (z + 1) * (far - near) / 2
    // D3D: NDC z in [0, 1]  -> depth = near + z * (far - near)
    const double depthScale = clipDepth == ClipDepth::NegativeOneToOne
                                  ? (farDepthRange - nearDepthRange) * 0.5
                                  : (farDepthRange - nearDepthRange);
    const double depthOffset = clipDepth == ClipDepth::NegativeOneToOne
                                   ? nearDepthRange + depthScale
                                   : nearDepthRange;

    return Matrix4D(halfWidth, 0.0,        0.0,        viewport.left + halfWidth,
                    0.0,       halfHeight, 0.0,        viewport.bottom + halfHeight,
                    0.0,       0.0,        depthScale, depthOffset,
                    0.0,       0.0,        0.0,        1.0);
}

// An orthographic projection whose units are the viewport's pixels, for
// drawing in window coordinates (billboards, HUDs).
Matrix4D SceneState::ComputeViewportOrthographicMatrix(const Rectangle& viewport, ClipDepth clipDepth) {
    return Matrix4D::CreateOrthographicOffCenter(
        viewport.left, viewport.Right(),
        viewport.bottom, viewport.Top(),
        0.0, 1.0, clipDepth);
}

} // namespace arda::renderer
```

Notes on the port:

- **Rectangle origin.** OpenGlobe uses `System.Drawing.Rectangle`, whose
  `Top` is the y origin and `Bottom` is `Top + Height`, so its viewport code
  swaps them ("MS → OpenGL"). Arda's `Rectangle` (Step 1) already has its
  origin at the bottom-left, so the viewport functions use `bottom` and
  `Top()` directly with no swap. The camera's orthographic swap is kept,
  because it's about the camera's values, not a `Rectangle`.
- **`ModelViewOrthographicMatrix` order.** This is a deliberate fix, called
  out in the comment above. A test in [2.3](#23-scenestatetests) checks it.
- **The viewport functions take a `ClipDepth`.** `SceneState.cs` assumes GL's
  [−1, 1]. On D3D the viewport transformation maps [0, 1] instead, and the
  viewport orthographic matrix must produce D3D's depth range.
- **They're `static`.** Neither reads the scene, so a `static` member function
  says so (see [Step 1](01-state-management.md)).
  `ComputeViewportTransformationMatrix` is an instance method in C#, but it
  never used `this`.

> **Math note — the viewport transformation:** this is the fixed-function
> step from the [Background](#background-from-a-vertex-to-a-pixel), written
> as a matrix so a shader can apply it itself. Chapter 7's wide lines and Chapter 9's
> billboards work in pixels. They transform to clip space, divide by w, apply
> this matrix to get window coordinates, offset by some pixels, then go back
> with `og_viewportOrthographicMatrix`.

### 2.3 `SceneStateTests`

These are pure math tests with no GPU. `tests/src/renderer/SceneStateTests.cpp`:

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/scene/SceneState.h>

#include <cmath>
#include <numbers>

using namespace arda::renderer;
using arda::core::ClipDepth;
using arda::core::Matrix4;
using arda::core::Vector3;
using arda::core::Vector4;

namespace {
// Clip coordinates -> NDC.
Vector4<double> Ndc(const Vector4<double>& clip) {
    return Vector4<double>(clip.X() / clip.W(), clip.Y() / clip.W(), clip.Z() / clip.W(), 1.0);
}
} // namespace

TEST_CASE("Camera defaults match Camera.cs") {
    const Camera camera;
    CHECK(camera.eye == Vector3<double>(0, -1, 0));
    CHECK(camera.fieldOfViewY == doctest::Approx(std::numbers::pi / 6));
    CHECK(camera.Forward() == Vector3<double>(0, 1, 0));
    CHECK(camera.Right() == Vector3<double>(1, 0, 0));
    CHECK(camera.FieldOfViewX() == doctest::Approx(camera.fieldOfViewY));   // aspect 1
}

TEST_CASE("ZoomToTarget fits a sphere in the view") {
    Camera camera;
    camera.ZoomToTarget(1.0);
    CHECK(camera.eye.X() == doctest::Approx(0));
    CHECK(camera.eye.Y() == doctest::Approx(-1.0 / std::sin(std::numbers::pi / 12)));
    CHECK(camera.eye.Z() == doctest::Approx(0));
}

TEST_CASE("ModelViewPerspectiveMatrix puts the target at the center of the screen") {
    SceneState sceneState;
    sceneState.camera.ZoomToTarget(1.0);

    for (ClipDepth clipDepth : {ClipDepth::NegativeOneToOne, ClipDepth::ZeroToOne}) {
        const auto mvp = sceneState.ModelViewPerspectiveMatrix(clipDepth);

        const auto center = Ndc(mvp * Vector4<double>(0, 0, 0, 1));
        CHECK(center.X() == doctest::Approx(0));
        CHECK(center.Y() == doctest::Approx(0));

        // +x is to the right and +z is up on the screen.
        CHECK(Ndc(mvp * Vector4<double>(1, 0, 0, 1)).X() > 0.0);
        CHECK(Ndc(mvp * Vector4<double>(0, 0, 1, 1)).Y() > 0.0);
    }
}

TEST_CASE("The aspect ratio squeezes x, not y") {
    SceneState sceneState;
    sceneState.camera.ZoomToTarget(1.0);
    const auto square = Ndc(sceneState.ModelViewPerspectiveMatrix(ClipDepth::NegativeOneToOne) *
                            Vector4<double>(1, 0, 1, 1));

    sceneState.camera.aspectRatio = 2.0;
    const auto wide = Ndc(sceneState.ModelViewPerspectiveMatrix(ClipDepth::NegativeOneToOne) *
                          Vector4<double>(1, 0, 1, 1));

    CHECK(wide.X() == doctest::Approx(square.X() / 2));
    CHECK(wide.Y() == doctest::Approx(square.Y()));
}

TEST_CASE("Cached matrices follow the camera") {
    SceneState sceneState;
    const auto before = sceneState.ViewMatrix();
    CHECK(sceneState.ViewMatrix() == before);   // cached

    sceneState.camera.eye = Vector3<double>(0, -2, 0);
    CHECK_FALSE(sceneState.ViewMatrix() == before);   // recomputed
    CHECK(sceneState.ViewMatrix() == Matrix4<double>::LookAt({0, -2, 0}, {0, 0, 0}, {0, 0, 1}));

    const auto gl = sceneState.PerspectiveMatrix(ClipDepth::NegativeOneToOne);
    const auto d3d = sceneState.PerspectiveMatrix(ClipDepth::ZeroToOne);
    CHECK_FALSE(gl == d3d);   // the clip depth is part of the cache key
}

TEST_CASE("ModelViewOrthographicMatrix applies the projection last") {
    SceneState sceneState;   // default camera: eye (0, -1, 0), orthographic box [0, 1] x [0, 1], near 0, far 1
    const auto p = Ndc(sceneState.ModelViewOrthographicMatrix(ClipDepth::NegativeOneToOne) *
                       Vector4<double>(0.5, -0.5, 0.5, 1));
    CHECK(p.X() == doctest::Approx(0));
    CHECK(p.Y() == doctest::Approx(0));
    CHECK(p.Z() == doctest::Approx(0));   // halfway between near and far
}

TEST_CASE("Relative-to-eye matrices give the same result for eye-relative positions") {
    SceneState sceneState;
    sceneState.camera.eye = Vector3<double>(6378137.0, 1000.0, 20.0);
    sceneState.camera.target = Vector3<double>(0, 0, 0);

    const Vector3<double> p(6378000.0, 900.0, 10.0);
    const Vector3<double> relative = p - sceneState.camera.eye;

    const auto mv = sceneState.ModelViewMatrix() * Vector4<double>(p.X(), p.Y(), p.Z(), 1);
    const auto mvRte = sceneState.ModelViewMatrixRelativeToEye() *
                       Vector4<double>(relative.X(), relative.Y(), relative.Z(), 1);
    CHECK(mvRte.X() == doctest::Approx(mv.X()));
    CHECK(mvRte.Y() == doctest::Approx(mv.Y()));
    CHECK(mvRte.Z() == doctest::Approx(mv.Z()));
}

TEST_CASE("The viewport transformation maps NDC to pixels and depth") {
    const Rectangle viewport{10, 20, 800, 600};
    const auto gl = SceneState::ComputeViewportTransformationMatrix(viewport, 0.0, 1.0, ClipDepth::NegativeOneToOne);
    const auto corner = gl * Vector4<double>(-1, -1, -1, 1);
    CHECK(corner.X() == doctest::Approx(10));
    CHECK(corner.Y() == doctest::Approx(20));
    CHECK(corner.Z() == doctest::Approx(0));

    const auto d3d = SceneState::ComputeViewportTransformationMatrix(viewport, 0.0, 1.0, ClipDepth::ZeroToOne);
    const auto farCorner = d3d * Vector4<double>(1, 1, 1, 1);
    CHECK(farCorner.X() == doctest::Approx(810));
    CHECK(farCorner.Y() == doctest::Approx(620));
    CHECK(farCorner.Z() == doctest::Approx(1));

    // The viewport orthographic matrix is its inverse for x and y.
    const auto ortho = SceneState::ComputeViewportOrthographicMatrix(viewport, ClipDepth::NegativeOneToOne);
    const auto back = ortho * Vector4<double>(10, 20, 0, 1);
    CHECK(back.X() == doctest::Approx(-1));
    CHECK(back.Y() == doctest::Approx(-1));
}
```

The variable is `farCorner`, not `far`: `<windows.h>` defines `near` and `far`
as empty macros, and a test file can end up including it indirectly.

Add the renderer sources and the test (the full CMake summary is in
[CMake](#cmake)):

```cmake
# renderer/CMakeLists.txt, after add_library(arda_renderer ...)
target_sources(arda_renderer PRIVATE
    src/scene/Camera.cpp
    src/scene/SceneState.cpp
    include/arda/renderer/scene/Camera.h
    include/arda/renderer/scene/SceneState.h
)

# tests/CMakeLists.txt
    src/renderer/SceneStateTests.cpp
```

> **Checkpoint:** build and run `arda_tests`. The Step 3 triangle program
> still compiles unchanged, because `SceneState` is still default
> constructible and `Draw` still takes it by `const&`.

---

## Part 3: Automatic uniforms (3.4.5)

### 3.1 What automatic uniforms are

Without them, every app does this before every draw:

```cpp
sp->Uniforms().Get<Matrix4<float>>("u_modelViewPerspective").SetValue(
    sceneState.ModelViewPerspectiveMatrix(device->ClipDepthRange()).Cast<float>());
```

That's a string lookup and a checked cast per uniform, per draw, repeated in
every example, and easy to forget when the camera moves. With automatic
uniforms, the shader declares the name and the renderer does the rest:

```glsl
uniform mat4 og_modelViewPerspectiveMatrix;   // set by the renderer before every draw
```

There are two kinds, and the difference is *when* the value is known.

| | Link-time automatic uniform | Draw-time automatic uniform |
|---|---|---|
| Value depends on | Nothing that changes: fixed per name | The scene, the context or the draw state |
| Set | Once, right after the program links | Before every draw that uses the program |
| Example | `og_texture0` is always texture unit 0 (Step 5) | `og_modelViewPerspectiveMatrix` follows the camera |
| OpenGlobe type | `LinkAutomaticUniform` | `DrawAutomaticUniformFactory` creates a `DrawAutomaticUniform` |
| Stored on the program | Nothing; the value is set and forgotten | One `DrawAutomaticUniform` object per matching uniform |

A draw-time uniform needs a *factory* because each program gets its own
object that holds a typed reference to *that program's* uniform. The
dynamic cast and the name lookup happen once, at link time. After that,
each draw is a virtual call and a `SetValue`, and `SetValue` only marks the
uniform dirty when the value actually changed (Step 2's delayed technique). If
the camera doesn't move, a draw makes no `glUniform*` calls at all.

> **Why:** automatic uniforms (3.4.5) move the knowledge of "where does this
> value come from" out of every client and into the renderer, in one place per
> uniform. A shader becomes self-describing: reading its uniform list tells
> you what it needs, and adding `og_sunPosition` to a shader is the whole
> change. It's also efficient: the per-draw work is proportional to the
> automatic uniforms a program *actually uses*.

> **OpenGL note — inactive uniforms:** the GL compiler removes uniforms that
> don't affect the output, and `glGetActiveUniform` (Step 2's `FindUniforms`)
> doesn't report them. A shader that declares `og_wgs84Height` but never uses
> it gets no `Wgs84HeightUniform`, and the camera height isn't computed for
> it. That's what you want, but it surprises people in tests: a test uniform
> must contribute to the output to exist.

### 3.2 `AutomaticUniforms.h` (Listings 3.12 and 3.13)

This public header holds the three abstract classes, the collection they're
registered in, and a generic draw automatic uniform built on `std::function`.

`renderer/include/arda/renderer/shaders/AutomaticUniforms.h`:

```cpp
#pragma once

#include <arda/renderer/shaders/Uniform.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace arda::renderer {

class Context;
struct DrawState;
class SceneState;

// LinkAutomaticUniform.cs: set once, right after a program is linked (for example og_texture0).
class LinkAutomaticUniform {
public:
    virtual ~LinkAutomaticUniform() = default;
    virtual std::string Name() const = 0;
    virtual void Set(UniformBase& uniform) const = 0;
};

// DrawAutomaticUniform.cs: set before every draw call that uses the program.
class DrawAutomaticUniform {
public:
    virtual ~DrawAutomaticUniform() = default;
    virtual void Set(Context& context, const DrawState& drawState, const SceneState& sceneState) = 0;
};

// DrawAutomaticUniformFactory.cs: creates a DrawAutomaticUniform for each
// program that declares a uniform with this name.
class DrawAutomaticUniformFactory {
public:
    virtual ~DrawAutomaticUniformFactory() = default;
    virtual std::string Name() const = 0;

    // Throws std::bad_cast if the uniform's type doesn't match (for example, a
    // float named og_modelViewPerspectiveMatrix).
    virtual std::unique_ptr<DrawAutomaticUniform> Create(UniformBase& uniform) const = 0;
};

// Anything with a Name() that returns a string.
template <typename T>
concept Named = requires(const T& item) {
    { item.Name() } -> std::convertible_to<std::string>;
};

// KeyedCollection<string, T> in OpenGlobe: owns items and finds them by name.
template <Named T>
class NamedCollection {
public:
    // Adding an item whose name is already present replaces the old item.
    // (C#'s KeyedCollection throws instead. Replacing lets an app override a
    // built-in automatic uniform.)
    void Add(std::unique_ptr<T> item) {
        if (!item) {
            throw std::invalid_argument("NamedCollection::Add: item is null");
        }
        std::string name = item->Name();   // read the name before item is moved from
        m_items.insert_or_assign(std::move(name), std::move(item));
    }

    // Returns false if there was no item with this name.
    bool Remove(std::string_view name) {
        auto it = m_items.find(name);
        if (it == m_items.end()) {
            return false;
        }
        m_items.erase(it);
        return true;
    }

    // Returns nullptr if there is no item with this name.
    const T* Find(std::string_view name) const {
        auto it = m_items.find(name);
        return it == m_items.end() ? nullptr : it->second.get();
    }

    bool Contains(std::string_view name) const { return Find(name) != nullptr; }
    std::size_t Size() const { return m_items.size(); }

private:
    std::map<std::string, std::unique_ptr<T>, std::less<>> m_items;   // std::less<> allows lookup by string_view
};

using LinkAutomaticUniformCollection = NamedCollection<LinkAutomaticUniform>;
using DrawAutomaticUniformFactoryCollection = NamedCollection<DrawAutomaticUniformFactory>;

// A draw automatic uniform whose value comes from a function. Most of Table
// 3.1 is one expression each, so one template replaces a class pair per uniform.
template <typename T>
class FunctionDrawAutomaticUniform final : public DrawAutomaticUniform {
public:
    using Function = std::function<T(const Context&, const DrawState&, const SceneState&)>;

    FunctionDrawAutomaticUniform(UniformBase& uniform, Function function)
        : m_uniform(dynamic_cast<Uniform<T>&>(uniform)), m_function(std::move(function)) {}

    void Set(Context& context, const DrawState& drawState, const SceneState& sceneState) override {
        m_uniform.SetValue(m_function(context, drawState, sceneState));
    }

private:
    Uniform<T>& m_uniform;   // owned by the program, which also owns this object
    Function m_function;     // a copy: the factory may be replaced while the program lives
};

template <typename T>
class FunctionDrawAutomaticUniformFactory final : public DrawAutomaticUniformFactory {
public:
    using Function = typename FunctionDrawAutomaticUniform<T>::Function;

    FunctionDrawAutomaticUniformFactory(std::string name, Function function)
        : m_name(std::move(name)), m_function(std::move(function)) {
        if (!m_function) {
            throw std::invalid_argument("FunctionDrawAutomaticUniformFactory: function is empty");
        }
    }

    std::string Name() const override { return m_name; }

    std::unique_ptr<DrawAutomaticUniform> Create(UniformBase& uniform) const override {
        return std::make_unique<FunctionDrawAutomaticUniform<T>>(uniform, m_function);
    }

private:
    std::string m_name;
    Function m_function;
};

} // namespace arda::renderer
```

Reading the classes against the C#:

- **`UniformBase&` instead of `Uniform`.** C++ can't have a class and a class
  template with the same name, so Step 2 named the base `UniformBase`
  (see [Step 2](02-shaders.md)).
- **The constructors cast with `dynamic_cast<Uniform<T>&>`.** The C# constructor does
  `(Uniform<Matrix4F>)uniform`, which throws `InvalidCastException` on a type
  mismatch. `dynamic_cast` to a *reference* throws `std::bad_cast` the same
  way (to a pointer it would return `nullptr`). See
  [Step 2](02-shaders.md) for `dynamic_cast` vs `static_cast`.
- **`Uniform<T>& m_uniform` is a reference member.** The uniform belongs to
  the program, and the `DrawAutomaticUniform` also belongs to the program, so
  the referenced object outlives the reference. A reference can't be null or
  reseated, and the class can't be copy-assigned, which is fine here. See
  [Step 0](00-setup.md) for reference members.
- **`final` and `override`:** see [Step 0](00-setup.md).
- **`std::map` with `std::less<>`**, like Step 2's `UniformCollection`: the
  transparent comparator lets `Find` take a `std::string_view` without
  building a `std::string`. A `std::unordered_map` would also work, but
  heterogeneous lookup in unordered containers needs a custom transparent hash
  as well, and the collections are small and only searched at link time.
- **`Named` is a concept** ([Step 3](03-vertex-data.md)): `NamedCollection<int>`
  fails with "constraints not satisfied" instead of an error deep inside `Add`.
- **`std::string name = item->Name();` on its own line.** Writing
  `m_items.insert_or_assign(item->Name(), std::move(item))` would probably
  work, since `std::move` is only a cast, but it makes the reader reason
  about evaluation order within one call. A separate statement doesn't.

> **C++ note — the abstract factory pattern:** `DrawAutomaticUniformFactory`
> is an *abstract factory*: an interface whose job is to create objects of
> another interface (`DrawAutomaticUniform`), without the caller knowing the
> concrete classes. `ShaderProgram` only knows "something that can create a
> `DrawAutomaticUniform` from a `UniformBase&`". Each concrete factory
> (`ModelViewPerspectiveMatrixUniformFactory`, `Wgs84HeightUniformFactory`)
> knows its one product class.
>
> The factory returns `std::unique_ptr<DrawAutomaticUniform>`, while the body
> creates `std::make_unique<ModelViewPerspectiveMatrixUniform>(...)`. A
> `unique_ptr<Derived>` converts implicitly to `unique_ptr<Base>`, like a
> derived-class pointer converts to a base-class pointer. The virtual
> destructor in the base makes deleting through the base pointer correct
> ([Step 0](00-setup.md)). C# does the same with `return new
> ModelViewPerspectiveMatrixUniform(uniform);` and the GC.

> **C++ note — factory registries and `std::function`:** a *registry* maps a
> key to a factory: here, a uniform name to a `DrawAutomaticUniformFactory`.
> `ShaderProgram` asks the registry by name and never mentions a concrete
> class, so adding a new automatic uniform touches only the registration code.
>
> Writing a class *pair* per uniform is how OpenGlobe does it (about 50
> files). Most pairs differ only in one expression, so
> `FunctionDrawAutomaticUniform<T>` stores that expression as a
> `std::function`. [Step 0](00-setup.md) introduced `std::function` for
> window callbacks. In depth, it's a *type-erasing* wrapper: a
> `std::function<float(const Context&, const DrawState&, const SceneState&)>`
> can hold any callable with that signature: a lambda, a function pointer or
> an object with `operator()`. Each lambda has its own unique, unnamed type,
> and `std::function` gives them all one common type you can store in a
> member or a container. It works like a C# delegate
> (`Func<Context, DrawState, SceneState, float>`).
>
> The costs, which are why the hand-written classes remain the pattern for
> anything non-trivial:
>
> - **An indirect call** per invocation (like a virtual call), and the
>   compiler usually can't inline through it.
> - **Possibly a heap allocation** when it's constructed or copied, if the
>   callable is larger than a small internal buffer. Captureless lambdas, like
>   every one in this step, fit in the buffer.
> - **Captures must outlive the function.** A lambda capturing a local by
>   reference (`[&]`) and stored in the device's registry dangles once the
>   local dies. See the lambda capture lifetime note in
>   [Step 1](01-state-management.md).
>
> `FunctionDrawAutomaticUniformFactory` is a class template deriving from a
> non-template interface, the same pattern as `Uniform<T>` deriving from
> `UniformBase` in Step 2: the template supplies the type-specific part,
> the base class lets different `T`s live in one registry.

### 3.3 `Device`: the registries and `ClipDepthRange`

The registries live on the `Device`, which every program is created from. Add
these members to `renderer/include/arda/renderer/Device.h`. The includes go
with the others at the top; the public functions go in the existing `public:`
section, after `Limits()`; `InitializeCommon` goes in the existing
`protected:` section, after `SetLimits`; the two data members go in the
existing `private:` section, after `m_limits`. Everything else in the class
stays as Steps 0–3 left it.

```cpp
// Device.h: new includes
#include <arda/core/geometry/ClipDepth.h>
#include <arda/renderer/shaders/AutomaticUniforms.h>

class Device {
public:
    // The NDC z range this API expects. Every projection matrix is built for it.
    core::ClipDepth ClipDepthRange() const {
        return Api() == GraphicsApi::Direct3D11 ? core::ClipDepth::ZeroToOne
                                                : core::ClipDepth::NegativeOneToOne;
    }

    // The automatic uniform registries (3.4.5). Apps can add their own; register
    // them before creating the programs that use them.
    LinkAutomaticUniformCollection& LinkAutomaticUniforms() { return m_linkAutomaticUniforms; }
    const LinkAutomaticUniformCollection& LinkAutomaticUniforms() const { return m_linkAutomaticUniforms; }
    DrawAutomaticUniformFactoryCollection& DrawAutomaticUniformFactories() { return m_drawAutomaticUniformFactories; }
    const DrawAutomaticUniformFactoryCollection& DrawAutomaticUniformFactories() const { return m_drawAutomaticUniformFactories; }

protected:
    // Backends call this at the end of their constructor, once the API works.
    // It can't be called from Device's own constructor: during a base-class
    // constructor, virtual calls don't reach the derived class, and Step 5's
    // additions create samplers through virtual functions.
    void InitializeCommon();

private:
    LinkAutomaticUniformCollection m_linkAutomaticUniforms;
    DrawAutomaticUniformFactoryCollection m_drawAutomaticUniformFactories;
};
```

> **Why:** OpenGlobe's `Device` is a C# `static class`, so its registries are
> global, filled in by a static constructor. Arda's `Device` is an object you
> create, so the registries are members:
>
> - **Several devices can coexist** (a GL one and a D3D11 one in the same test
>   run, in Step 8), each with its own registry. A global registry would be
>   shared between them.
> - **Tests are isolated.** A test that registers a custom uniform on its own
>   device can't affect other tests.
> - **Registration can depend on the device.** Step 5 registers one
>   `og_textureN` per texture unit, and the count comes from the device's
>   limits.
> - **Rejected: a global registry filled by static initializers** (each
>   uniform's `.cpp` registers itself at program start). It's a well-known C++
>   trick, but the order in which static objects in different files are
>   initialized is unspecified (the "static initialization order fiasco"), and
>   a static library's unreferenced object files may be dropped by the linker,
>   silently losing their registrations.
>
> `ClipDepthRange()` is on the device because the depth convention is a
> property of the API, and every context of a device shares it.

### 3.4 `ShaderProgram`: finding and setting automatic uniforms

`ShaderProgram.cs` implements both functions in the abstract base class, and
so does arda: they only use the uniform collection and the device, which are
API-agnostic. Backends call them.

Here is the whole of `renderer/include/arda/renderer/shaders/ShaderProgram.h`:
Step 2's file, with the Step 4 lines marked.

```cpp
#pragma once

#include <arda/renderer/shaders/AutomaticUniforms.h>        // Step 4
#include <arda/renderer/shaders/ShaderVertexAttribute.h>
#include <arda/renderer/shaders/UniformCollection.h>

#include <memory>        // Step 4
#include <string>
#include <string_view>
#include <vector>        // Step 4

namespace arda::renderer {

class Context;           // Step 4
class Device;            // Step 4
struct DrawState;        // Step 4
class SceneState;        // Step 4

class ShaderProgram {
public:
    virtual ~ShaderProgram() = default;

    ShaderProgram(const ShaderProgram&)            = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    virtual std::string Log() const = 0;

    const ShaderVertexAttributeCollection& VertexAttributes() const { return m_vertexAttributes; }
    UniformCollection& Uniforms() { return m_uniforms; }
    const UniformCollection& Uniforms() const { return m_uniforms; }

    // Color attachment index of a fragment shader output (3.4.3). Throws std::out_of_range.
    virtual int FragmentOutputLocation(std::string_view name) const = 0;

protected:
    ShaderProgram() = default;

    // Step 4. Backends call this once, after filling m_uniforms. Sets link
    // automatic uniforms and creates a DrawAutomaticUniform for each draw
    // automatic uniform the program uses. Throws std::bad_cast if an automatic
    // uniform is declared with the wrong type.
    void InitializeAutomaticUniforms(const Device& device);

    // Step 4. Backends call this before uploading dirty uniforms, on every draw.
    void SetDrawAutomaticUniforms(Context& context, const DrawState& drawState, const SceneState& sceneState);

    ShaderVertexAttributeCollection m_vertexAttributes;
    UniformCollection m_uniforms;

private:
    // Step 4. Declared after m_uniforms, so it's destroyed first: its objects
    // hold references into m_uniforms.
    std::vector<std::unique_ptr<DrawAutomaticUniform>> m_drawAutomaticUniforms;
};

} // namespace arda::renderer
```

`renderer/src/shaders/ShaderProgram.cpp` (new):

```cpp
#include <arda/renderer/shaders/ShaderProgram.h>

#include <arda/renderer/Device.h>

namespace arda::renderer {

// ShaderProgram.cs, InitializeAutomaticUniforms. A name registered as both
// kinds is treated as a link automatic uniform, as in the C#.
void ShaderProgram::InitializeAutomaticUniforms(const Device& device) {
    for (const auto& uniform : m_uniforms) {
        const std::string& name = uniform->Name();
        if (const LinkAutomaticUniform* link = device.LinkAutomaticUniforms().Find(name)) {
            link->Set(*uniform);
        } else if (const DrawAutomaticUniformFactory* factory = device.DrawAutomaticUniformFactories().Find(name)) {
            m_drawAutomaticUniforms.push_back(factory->Create(*uniform));
        }
    }
}

// ShaderProgram.cs, SetDrawAutomaticUniforms.
void ShaderProgram::SetDrawAutomaticUniforms(Context& context, const DrawState& drawState,
                                             const SceneState& sceneState) {
    for (const auto& uniform : m_drawAutomaticUniforms) {
        uniform->Set(context, drawState, sceneState);
    }
}

} // namespace arda::renderer
```

- `m_uniforms` iterates `std::unique_ptr<UniformBase>` (Step 2's
  `UniformCollection::begin()`). `uniform` is a `const unique_ptr&`, but
  `*uniform` is a non-`const` `UniformBase&`, which is what `Set` and `Create`
  need. See the `mutable` note in [2.2](#22-scenestate) for why.
- Declaring a variable in an `if` condition
  (`if (const LinkAutomaticUniform* link = ...)`) scopes it to the
  `if`/`else` chain, and the branch runs only when the pointer is non-null.
- **Registration order matters.** A program looks at the registries *once*,
  when it's created. A factory added later only affects programs created
  after it.

### 3.5 The first two automatic uniforms

Every automatic uniform in OpenGlobe is two classes: the uniform and its
factory. Keep them together in one private header per uniform, in
`renderer/src/shaders/automaticuniforms/`. They're private because apps never
name them; apps only see the uniform name in GLSL.

**`og_modelViewPerspectiveMatrix`** (the class the milestone needs).
`renderer/src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h`:

```cpp
#pragma once

#include <arda/core/geometry/ClipDepth.h>
#include <arda/core/geometry/Matrix4.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/AutomaticUniforms.h>

#include <memory>
#include <string>

namespace arda::renderer {

class ModelViewPerspectiveMatrixUniform final : public DrawAutomaticUniform {
public:
    // Throws std::bad_cast if the shader declared og_modelViewPerspectiveMatrix
    // with a type other than mat4 (the "Try This" in 3.4.5).
    explicit ModelViewPerspectiveMatrixUniform(UniformBase& uniform)
        : m_uniform(dynamic_cast<Uniform<core::Matrix4<float>>&>(uniform)) {}

    void Set(Context& context, const DrawState&, const SceneState& sceneState) override {
        const core::ClipDepth clipDepth = context.GetDevice().ClipDepthRange();
        // Multiply in double, convert once at the end.
        m_uniform.SetValue(sceneState.ModelViewPerspectiveMatrix(clipDepth).Cast<float>());
    }

private:
    Uniform<core::Matrix4<float>>& m_uniform;
};

class ModelViewPerspectiveMatrixUniformFactory final : public DrawAutomaticUniformFactory {
public:
    std::string Name() const override { return "og_modelViewPerspectiveMatrix"; }

    std::unique_ptr<DrawAutomaticUniform> Create(UniformBase& uniform) const override {
        return std::make_unique<ModelViewPerspectiveMatrixUniform>(uniform);
    }
};

} // namespace arda::renderer
```

The `DrawState` parameter is unnamed because it isn't used. Leaving the name
out is how C++ says "intentionally unused" without a compiler warning.

**`og_wgs84Height`** (Listing 3.15), the book's example of a uniform that isn't
a matrix. Chapter 4 uses it to fade things in with altitude.
`renderer/src/shaders/automaticuniforms/Wgs84HeightUniform.h`:

```cpp
#pragma once

#include <arda/core/geometry/Ellipsoid.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/AutomaticUniforms.h>

#include <memory>
#include <string>

namespace arda::renderer {

class Wgs84HeightUniform final : public DrawAutomaticUniform {
public:
    explicit Wgs84HeightUniform(UniformBase& uniform)
        : m_uniform(dynamic_cast<Uniform<float>&>(uniform)) {}

    void Set(Context&, const DrawState&, const SceneState& sceneState) override {
        m_uniform.SetValue(static_cast<float>(sceneState.camera.Height(m_wgs84)));
    }

private:
    Uniform<float>& m_uniform;
    core::geometry::Ellipsoid m_wgs84 = core::geometry::Ellipsoid::WGS84();
};

class Wgs84HeightUniformFactory final : public DrawAutomaticUniformFactory {
public:
    std::string Name() const override { return "og_wgs84Height"; }

    std::unique_ptr<DrawAutomaticUniform> Create(UniformBase& uniform) const override {
        return std::make_unique<Wgs84HeightUniform>(uniform);
    }
};

} // namespace arda::renderer
```

The C# reads the static `Ellipsoid.Wgs84` each time. Here each
`Wgs84HeightUniform` keeps its own copy, built once when the program is
created. It's a few dozen bytes and avoids rebuilding the ellipsoid on every
draw.

### 3.6 The standard draw automatic uniforms

The rest of Table 3.1 (and of OpenGlobe's `Device` static constructor) is one
expression each, so they're registered through
`FunctionDrawAutomaticUniformFactory<T>`. `T` must match the type Step 2's
`ShaderProgramGL3x::CreateUniform` creates for the GLSL type: `float` for
`float`, `Vector2<float>` for `vec2`, `Vector3<float>` for `vec3`,
`Vector4<float>` for `vec4`, `Matrix4<float>` for `mat4`.

| Name | GLSL type | Value |
|---|---|---|
| `og_modelViewPerspectiveMatrix` | `mat4` | `P · V · M` (hand-written class) |
| `og_wgs84Height` | `float` | Eye height above WGS84 (hand-written class) |
| `og_modelMatrix` | `mat4` | `M` |
| `og_viewMatrix` | `mat4` | `V` |
| `og_modelViewMatrix` | `mat4` | `V · M` |
| `og_perspectiveMatrix` | `mat4` | `P` (perspective) |
| `og_orthographicMatrix` | `mat4` | Orthographic `P` from the camera's box |
| `og_modelViewOrthographicMatrix` | `mat4` | Orthographic `P · V · M` |
| `og_modelViewMatrixRelativeToEye` | `mat4` | `V · M` without translation (Chapter 5) |
| `og_modelViewPerspectiveMatrixRelativeToEye` | `mat4` | `P ·` the above (Chapter 5) |
| `og_viewportOrthographicMatrix` | `mat4` | Pixels → clip, for the current viewport |
| `og_viewportTransformationMatrix` | `mat4` | NDC → pixels and depth, for the current viewport and depth range |
| `og_windowToWorldNearPlane` | `mat4` | NDC x, y → world position on the near plane (Chapter 4 ray casting) |
| `og_viewport` | `vec4` | `(left, bottom, width, height)` |
| `og_inverseViewportDimensions` | `vec2` | `(1 / width, 1 / height)` |
| `og_cameraEye` | `vec3` | Eye position |
| `og_cameraEyeHigh`, `og_cameraEyeLow` | `vec3` | Eye split into two floats (Chapter 5) |
| `og_cameraLightPosition` | `vec3` | Light at the eye |
| `og_sunPosition` | `vec3` | `SceneState::sunPosition` |
| `og_diffuseSpecularAmbientShininess` | `vec4` | The four lighting parameters |
| `og_perspectiveNearPlaneDistance`, `og_perspectiveFarPlaneDistance` | `float` | Camera near and far distances |
| `og_pixelSizePerDistance` | `float` | Height of one pixel at distance 1, for sizing things in pixels |

Not registered: `og_modelZToClipCoordinates` (Chapter 4, needs `mat4x2`
support) and `og_highResolutionSnapScale` (later chapters).

`renderer/src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.h`:

```cpp
#pragma once

#include <arda/renderer/shaders/AutomaticUniforms.h>

namespace arda::renderer {

// Registers every draw automatic uniform the renderer provides
// (Device.cs static constructor, lines 54-79).
void AddStandardDrawAutomaticUniformFactories(DrawAutomaticUniformFactoryCollection& factories);

} // namespace arda::renderer
```

`renderer/src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.cpp`:

```cpp
#include "shaders/automaticuniforms/StandardDrawAutomaticUniforms.h"
#include "shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h"
#include "shaders/automaticuniforms/Wgs84HeightUniform.h"

#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/Vector2.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/core/geometry/Vector4.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/Rectangle.h>
#include <arda/renderer/scene/SceneState.h>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace arda::renderer {

namespace {

using Matrix4D = core::Matrix4<double>;
using Matrix4F = core::Matrix4<float>;
using Vector2F = core::Vector2<float>;
using Vector3F = core::Vector3<float>;
using Vector4F = core::Vector4<float>;

Vector3F ToVector3F(const core::Vector3<double>& v) {
    return Vector3F(static_cast<float>(v.X()), static_cast<float>(v.Y()), static_cast<float>(v.Z()));
}

core::ClipDepth ClipDepthOf(const Context& context) {
    return context.GetDevice().ClipDepthRange();
}

// One registration. T is the C++ type of the uniform (see the table in the guide).
template <typename T>
void Register(DrawAutomaticUniformFactoryCollection& factories, std::string name,
              typename FunctionDrawAutomaticUniformFactory<T>::Function function) {
    factories.Add(std::make_unique<FunctionDrawAutomaticUniformFactory<T>>(std::move(name), std::move(function)));
}

// WindowToWorldNearPlaneUniform.cs: maps (x, y, 0, 1), with x and y in [-1, 1]
// like NDC, to the world position of that point on the near plane.
Matrix4D WindowToWorldNearPlane(const Camera& camera) {
    const double theta = camera.FieldOfViewX() * 0.5;
    const double phi = camera.fieldOfViewY * 0.5;
    const double nearDistance = camera.perspectiveNearPlaneDistance;

    const core::Vector3<double> origin = camera.eye + camera.Forward() * nearDistance;   // eye projected onto the near plane
    const core::Vector3<double> xAxis = camera.Right() * (nearDistance * std::tan(theta));
    const core::Vector3<double> yAxis = camera.up * (nearDistance * std::tan(phi));

    return Matrix4D(xAxis.X(), yAxis.X(), 0.0, origin.X(),
                    xAxis.Y(), yAxis.Y(), 0.0, origin.Y(),
                    xAxis.Z(), yAxis.Z(), 0.0, origin.Z(),
                    0.0,       0.0,       0.0, 1.0);
}

} // namespace

void AddStandardDrawAutomaticUniformFactories(DrawAutomaticUniformFactoryCollection& factories) {
    // Hand-written classes: Listings 3.13 and 3.15.
    factories.Add(std::make_unique<ModelViewPerspectiveMatrixUniformFactory>());
    factories.Add(std::make_unique<Wgs84HeightUniformFactory>());

    // Matrices.
    Register<Matrix4F>(factories, "og_modelMatrix",
        [](const Context&, const DrawState&, const SceneState& s) { return s.modelMatrix.Cast<float>(); });
    Register<Matrix4F>(factories, "og_viewMatrix",
        [](const Context&, const DrawState&, const SceneState& s) { return s.ViewMatrix().Cast<float>(); });
    Register<Matrix4F>(factories, "og_modelViewMatrix",
        [](const Context&, const DrawState&, const SceneState& s) { return s.ModelViewMatrix().Cast<float>(); });
    Register<Matrix4F>(factories, "og_perspectiveMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return s.PerspectiveMatrix(ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_orthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return s.OrthographicMatrix(ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_modelViewOrthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return s.ModelViewOrthographicMatrix(ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_modelViewMatrixRelativeToEye",
        [](const Context&, const DrawState&, const SceneState& s) {
            return s.ModelViewMatrixRelativeToEye().Cast<float>();
        });
    Register<Matrix4F>(factories, "og_modelViewPerspectiveMatrixRelativeToEye",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return s.ModelViewPerspectiveMatrixRelativeToEye(ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_viewportOrthographicMatrix",
        [](const Context& c, const DrawState&, const SceneState&) {
            return SceneState::ComputeViewportOrthographicMatrix(c.GetViewport(), ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_viewportTransformationMatrix",
        [](const Context& c, const DrawState& d, const SceneState&) {
            const DepthRange& depthRange = d.renderState.depthRange;
            return SceneState::ComputeViewportTransformationMatrix(
                c.GetViewport(), depthRange.nearValue, depthRange.farValue, ClipDepthOf(c)).Cast<float>();
        });
    Register<Matrix4F>(factories, "og_windowToWorldNearPlane",
        [](const Context&, const DrawState&, const SceneState& s) {
            return WindowToWorldNearPlane(s.camera).Cast<float>();
        });

    // Viewport.
    Register<Vector4F>(factories, "og_viewport",
        [](const Context& c, const DrawState&, const SceneState&) {
            const Rectangle& v = c.GetViewport();
            return Vector4F(static_cast<float>(v.left), static_cast<float>(v.bottom),
                            static_cast<float>(v.width), static_cast<float>(v.height));
        });
    Register<Vector2F>(factories, "og_inverseViewportDimensions",
        [](const Context& c, const DrawState&, const SceneState&) {
            const Rectangle& v = c.GetViewport();
            return Vector2F(1.0f / static_cast<float>(v.width), 1.0f / static_cast<float>(v.height));
        });

    // Camera.
    Register<Vector3F>(factories, "og_cameraEye",
        [](const Context&, const DrawState&, const SceneState& s) { return ToVector3F(s.camera.eye); });
    Register<Vector3F>(factories, "og_cameraEyeHigh",
        [](const Context&, const DrawState&, const SceneState& s) { return s.camera.EyeHigh(); });
    Register<Vector3F>(factories, "og_cameraEyeLow",
        [](const Context&, const DrawState&, const SceneState& s) { return s.camera.EyeLow(); });
    Register<float>(factories, "og_perspectiveNearPlaneDistance",
        [](const Context&, const DrawState&, const SceneState& s) {
            return static_cast<float>(s.camera.perspectiveNearPlaneDistance);
        });
    Register<float>(factories, "og_perspectiveFarPlaneDistance",
        [](const Context&, const DrawState&, const SceneState& s) {
            return static_cast<float>(s.camera.perspectiveFarPlaneDistance);
        });
    Register<float>(factories, "og_pixelSizePerDistance",
        [](const Context& c, const DrawState&, const SceneState& s) {
            return static_cast<float>(std::tan(0.5 * s.camera.fieldOfViewY) * 2.0 / c.GetViewport().height);
        });

    // Lighting.
    Register<Vector3F>(factories, "og_cameraLightPosition",
        [](const Context&, const DrawState&, const SceneState& s) { return ToVector3F(s.CameraLightPosition()); });
    Register<Vector3F>(factories, "og_sunPosition",
        [](const Context&, const DrawState&, const SceneState& s) { return ToVector3F(s.sunPosition); });
    Register<Vector4F>(factories, "og_diffuseSpecularAmbientShininess",
        [](const Context&, const DrawState&, const SceneState& s) {
            return Vector4F(s.diffuseIntensity, s.specularIntensity, s.ambientIntensity, s.shininess);
        });
}

} // namespace arda::renderer
```

- **The anonymous namespace** keeps the helpers private to this file
  ([Step 0](00-setup.md)).
- **`Register<T>`'s third parameter** is
  `typename FunctionDrawAutomaticUniformFactory<T>::Function`. `T` can't be
  deduced from it (a type nested inside a template is a "non-deduced
  context"), which is fine because every call names `T` explicitly. The
  lambda then converts to that `std::function`. The `typename` keyword tells
  the compiler that `Function` names a type; inside a template it can't know
  that until `T` is known.
- **`og_viewport` uses `bottom`**, where the C# used `viewport.Top` with a
  comment that "viewport.Bottom should really be used". Arda's bottom-left
  `Rectangle` makes the intended value the natural one.
- **A zero-sized viewport** (a minimized window) gives infinities in
  `og_inverseViewportDimensions` and `og_pixelSizePerDistance`. Nothing is
  visible then anyway, and the milestone's resize handler skips a zero-height
  viewport.

### 3.7 `Device::InitializeCommon`

Add to `renderer/src/Device.cpp` (with the other includes at the top, and the
function inside `namespace arda::renderer`):

```cpp
#include "shaders/automaticuniforms/StandardDrawAutomaticUniforms.h"

// Device.cs static constructor, lines 36-84. Step 5 adds the og_textureN link
// automatic uniforms and the samplers here.
void Device::InitializeCommon() {
    AddStandardDrawAutomaticUniformFactories(m_drawAutomaticUniformFactories);
}
```

The `"shaders/..."` include works because `renderer/src` is a private include
directory of `arda_renderer`.

### 3.8 GL backend changes

Two changes: `ShaderProgramGL3x` needs the device, so it can look up the
registries after linking, and it sets draw automatic uniforms before
uploading dirty uniforms.

**`src/gl/shaders/ShaderProgramGL3x.h`**, whole file (Step 2's, plus Step 3's
`Clean` and this step's constructor parameter):

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
    // Step 4: takes the device, for the automatic uniform registries.
    ShaderProgramGL3x(const Device& device,
                      std::string_view vertexShaderSource,
                      std::string_view geometryShaderSource,
                      std::string_view fragmentShaderSource);

    std::string Log() const override;
    int FragmentOutputLocation(std::string_view name) const override;

    void NotifyDirty(ICleanable& value) override { m_dirtyUniforms.push_back(&value); }

    GLuint Handle() const { return m_program.Get(); }
    void Bind() const { glUseProgram(m_program.Get()); }

    // Called by ContextGL3x before each draw (Step 3). Step 4 sets the draw
    // automatic uniforms first.
    void Clean(Context& context, const DrawState& drawState, const SceneState& sceneState);

private:
    void FindVertexAttributes();
    void FindUniforms();
    std::unique_ptr<UniformBase> CreateUniform(std::string name, GLint location, GLenum type);

    ShaderObjectGL3x m_vertexShader;
    std::optional<ShaderObjectGL3x> m_geometryShader;
    ShaderObjectGL3x m_fragmentShader;
    ProgramName m_program;
    std::vector<ICleanable*> m_dirtyUniforms;   // points into m_uniforms, which lives as long as this program
};

} // namespace arda::renderer::gl
```

`Context`, `Device`, `DrawState` and `SceneState` are forward-declared in
`ShaderProgram.h`, in `arda::renderer`. Inside `arda::renderer::gl`, the
unqualified names find them in the enclosing namespace.

**`src/gl/shaders/ShaderProgramGL3x.cpp`**: replace the constructor and
`Clean`. The other functions don't change.

```cpp
ShaderProgramGL3x::ShaderProgramGL3x(
    const Device& device,
    std::string_view vertexShaderSource,
    std::string_view geometryShaderSource,
    std::string_view fragmentShaderSource)
    : m_vertexShader(GL_VERTEX_SHADER, vertexShaderSource),
      m_fragmentShader(GL_FRAGMENT_SHADER, fragmentShaderSource),
      m_program(glCreateProgram()) {
    if (!geometryShaderSource.empty()) {
        m_geometryShader.emplace(GL_GEOMETRY_SHADER, geometryShaderSource);
    }

    const GLuint program = m_program.Get();
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
    InitializeAutomaticUniforms(device);   // Step 4
}

void ShaderProgramGL3x::Clean(Context& context, const DrawState& drawState, const SceneState& sceneState) {
    // Step 4: compute this draw's automatic uniform values. Each SetValue only
    // marks its uniform dirty (NotifyDirty) if the value changed.
    SetDrawAutomaticUniforms(context, drawState, sceneState);

    // Upload everything that changed, whether the app or an automatic uniform set it.
    for (ICleanable* uniform : m_dirtyUniforms) {
        uniform->Clean();
    }
    m_dirtyUniforms.clear();
}
```

Two details worth knowing:

- **Link automatic uniforms call `NotifyDirty` from inside the constructor.**
  `link->Set(uniform)` → `SetValue` → `m_observer.NotifyDirty(*this)`, a
  virtual call on the program being constructed. That's safe here: the call
  happens in `ShaderProgramGL3x`'s *own* constructor body, when the object is
  already a complete `ShaderProgramGL3x` and `m_dirtyUniforms` is constructed.
  (The problem case is a virtual call from a *base* class constructor,
  which is why `InitializeCommon` isn't called from `Device`'s constructor.)
- **If `InitializeAutomaticUniforms` throws** (`std::bad_cast` for a
  mistyped automatic uniform), the constructor fails, and C++ destroys every
  member and base that was already constructed: the program name, the shader
  objects and the uniforms. RAII means there's nothing to clean up by hand.

> **OpenGL note — uniform values live in the program:** `glUniform*` writes
> into the *program object*, and the value stays until it's written again,
> even across draws, `glUseProgram` switches and (because programs are shared)
> contexts. That's what makes Step 2's dirty tracking valid: the program
> remembers its last uploaded values, so `UniformGL3x` only uploads a changed
> value. It's also why automatic uniforms must be set before *every* draw, not
> once per frame: the same program may be drawn with two different model
> matrices in one frame.

**`src/gl/DeviceGL3x.cpp`**: call `InitializeCommon` at the end of the
constructor, and pass the device to new programs. The whole constructor,
with the Step 4 line marked:

```cpp
DeviceGL3x::DeviceGL3x() {
    ApplyContextHints(WindowType::Hidden);
    m_shareWindow = glfwCreateWindow(1, 1, "arda share context", nullptr, nullptr);
    if (m_shareWindow == nullptr) {
        throw std::runtime_error("DeviceGL3x: could not create an OpenGL 3.3 context");
    }

    // Resources created through the device are created while this context is current.
    glfwMakeContextCurrent(m_shareWindow);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(m_shareWindow);
        throw std::runtime_error("DeviceGL3x: gladLoadGL failed");
    }

    // Device.cs static constructor, lines 33-35.
    DeviceLimits limits;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &limits.maximumNumberOfVertexAttributes);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &limits.numberOfTextureUnits);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &limits.maximumNumberOfColorAttachments);
    SetLimits(limits);

    InitializeCommon();   // Step 4: register the automatic uniforms
}

std::shared_ptr<ShaderProgram> DeviceGL3x::DoCreateShaderProgram(
    std::string_view vs, std::string_view gs, std::string_view fs) {
    return std::make_shared<ShaderProgramGL3x>(*this, vs, gs, fs);   // Step 4: *this
}
```

`*this` is a `DeviceGL3x&`, which converts to the `const Device&` the
constructor takes.

Add the new sources to CMake:

```cmake
# renderer/CMakeLists.txt
target_sources(arda_renderer PRIVATE
    src/shaders/ShaderProgram.cpp
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.cpp
    include/arda/renderer/shaders/AutomaticUniforms.h
    src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.h
    src/shaders/automaticuniforms/Wgs84HeightUniform.h
)
```

> **Checkpoint:** build and run `arda_tests`. Step 2's `ShaderProgramTests`
> and Step 3's `VertexDataTests` must still pass: their shaders use `u_`
> names, which match no registry entry. The Step 3 triangle still runs
> unchanged.

---

## Part 4: Milestone, the triangle through a camera

Switch the Step 3 program to the book's setup (Listings 3.30–3.32): the
triangle in the xz plane, a vertex shader that uses
`og_modelViewPerspectiveMatrix`, and a camera zoomed to fit it. Step 7 turns
this into the full `Chapter03Triangle` port.

Here is the complete `scene/src/main.cpp`:

```cpp
#include <arda/core/geometry/Mesh.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/ClearState.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/ShaderProgram.h>

#include <cstdio>
#include <exception>
#include <memory>

int main() {
    using namespace arda::renderer;
    using namespace arda::core::geometry;
    using arda::core::Vector3;

    try {
        // Declared first so it is destroyed last.
        auto device = CreateDevice(GraphicsApi::OpenGL33);
        auto window = device->CreateGraphicsWindow(800, 600, "Step 4: Triangle through a camera");
        Context& context = window->GetContext();

        // Listing 3.31. No uniform is set for the matrix: the renderer does it before each draw.
        auto sp = device->CreateShaderProgram(
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
        sp->Uniforms().Get<Vector3<float>>("u_color").SetValue(Vector3<float>(1.0f, 0.0f, 0.0f));

        // The book's triangle: a right triangle in the xz plane with legs of length 1.
        Mesh mesh;
        auto& positions = mesh.attributes.Add<VertexAttributeFloatVector3>("position", 3).Values();
        positions.emplace_back(0.0f, 0.0f, 0.0f);
        positions.emplace_back(1.0f, 0.0f, 0.0f);
        positions.emplace_back(0.0f, 0.0f, 1.0f);

        auto indices = std::make_unique<IndicesUnsignedShort>(3);
        indices->AddTriangle(0, 1, 2);
        mesh.indices = std::move(indices);

        DrawState drawState;
        drawState.renderState.facetCulling.enabled = false;
        drawState.renderState.depthTest.enabled = false;
        drawState.shaderProgram = sp;
        drawState.vertexArray = context.CreateVertexArray(mesh, sp->VertexAttributes(), BufferHint::StaticDraw);

        ClearState clearState;
        SceneState sceneState;
        sceneState.camera.ZoomToTarget(1.0);   // the triangle fits in a sphere of radius 1 around the origin

        window->SetResizeHandler([&] {
            if (window->Width() == 0 || window->Height() == 0) {
                return;   // minimized: keep the last viewport and aspect ratio
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

What you should see: a red right triangle on a white background (`ClearState`'s
default color is white, like OpenGlobe's), with the right angle at the center
of the window, one leg going right and one going up. The default camera is at
(0, −1, 0), looking at the origin with +z up, so world x is screen right and
world z is screen up. `ZoomToTarget(1)` backs it off to about 3.86 units.

> **OpenGL note — `vec4 position` from a 3-component attribute:** the mesh
> stores 3 floats per vertex, but the shader declares `vec4`. GL fills missing
> components from `(0, 0, 0, 1)`, so `w` is 1, which is exactly what a
> position needs to be affected by translation.

> **OpenGL note — the aspect ratio comes from the viewport:** the projection
> must squeeze x by `width / height` of the area it's drawn into, or a square
> looks stretched. That area is the viewport, in *framebuffer pixels*.
> `window->Width()` and `Height()` are the framebuffer size (Step 0 reads
> them with `glfwGetFramebufferSize`), which on a high-DPI display can differ
> from the window size in screen coordinates. Using framebuffer size for both
> the viewport and the aspect ratio keeps them consistent. `static_cast<double>`
> matters: `800 / 600` in integers is 1.

**Check the milestone:**

- Resize the window to be very wide, then very tall. The triangle keeps its
  shape (its legs stay equal length) and stays centered. If it stretches,
  the aspect ratio isn't being updated; if it's clipped oddly or missing, check
  the viewport.
- Minimize and restore. Nothing should throw.

Things to try:

- **Orbit the camera.** In an update handler, rotate the eye about the z axis
  every frame, using core's `RotateAboutAxis`:
  ```cpp
  window->SetUpdateFrameHandler([&] {
      sceneState.camera.eye = sceneState.camera.eye.RotateAboutAxis(0.01, Vector3<double>::UnitZ());
  });
  ```
  The triangle turns edge-on and then shows its back. With facet culling
  enabled, the back side disappears.
- **Move the model instead of the camera.** Set `sceneState.modelMatrix` to a
  translation, for example
  `arda::core::Matrix4<double>(1,0,0,-0.5, 0,1,0,0, 0,0,1,-0.5, 0,0,0,1)`, to
  center the triangle's bounding box on the screen.
- **Break it on purpose.** Declare `uniform float og_modelViewPerspectiveMatrix;`
  and multiply `position` by it. `CreateShaderProgram` throws `std::bad_cast`
  (the "Try This" in 3.4.5). Then stop using the uniform in the shader's
  `main`, but keep the declaration: now nothing throws, because the inactive
  uniform is never reported.
- **Push the near plane.** Set `camera.perspectiveNearPlaneDistance = 4.0`.
  The whole triangle disappears: it lies in the plane y = 0, which is 3.86
  units in front of the camera everywhere, so all of it is now nearer than the
  near plane. Combine this with the orbiting camera to watch the near plane
  clip part of it.

---

## Part 5: The shader cache (3.4.6)

### Why a cache

Several objects often need the same shader. In a globe, every tile of terrain,
or every polyline, would otherwise compile and link its own identical program.
Linking is slow (milliseconds, sometimes more), and each program costs driver
memory. Worse for performance, *different* program objects defeat sorting by
shader: two identical programs are two `glUseProgram` switches.

`ShaderCache` hands out one shared program per *key*, chosen by the caller
(usually a name for the shader's purpose). The first `FindOrAdd` compiles;
later ones return the same program.

> **Why:** the cache is reference counted (3.4.6), and the program is removed
> when the count reaches zero. Callers pair every `FindOrAdd` (or successful
> `Find`) with one `Release`, exactly like OpenGlobe's `ShaderCache`, which
> disposes the program on the last release. In C#, that explicit count is the
> only way to get deterministic cleanup of a GPU resource: the garbage
> collector would free it at some unknown time, or never. In arda the program
> is a `shared_ptr`, so removing the cache entry only drops the cache's
> reference, and the program is destroyed when the last user's `shared_ptr`
> goes away. Keeping the explicit count still matters: it's what lets the
> cache *forget* a key, so a later `FindOrAdd` with new sources recompiles,
> and it keeps the API the book describes. See the `weak_ptr` alternative
> below.

`renderer/include/arda/renderer/shaders/ShaderCache.h`:

```cpp
#pragma once

#include <arda/renderer/shaders/ShaderProgram.h>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace arda::renderer {

class Device;

// ShaderCache.cs (Listing 3.16). Returns the same ShaderProgram for the same key,
// so objects that use the same shader share one program. Thread-safe.
// The cache must not outlive the device.
class ShaderCache {
public:
    explicit ShaderCache(Device& device) : m_device(device) {}

    ShaderCache(const ShaderCache&)            = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;

    // Returns the cached program for key, or creates one from the sources.
    // If the key is already cached, the sources are ignored.
    std::shared_ptr<ShaderProgram> FindOrAdd(const std::string& key,
                                             std::string_view vertexShaderSource,
                                             std::string_view fragmentShaderSource);

    std::shared_ptr<ShaderProgram> FindOrAdd(const std::string& key,
                                             std::string_view vertexShaderSource,
                                             std::string_view geometryShaderSource,
                                             std::string_view fragmentShaderSource);

    // Returns nullptr if the key isn't cached.
    std::shared_ptr<ShaderProgram> Find(const std::string& key);

    // Each FindOrAdd or successful Find must be matched by one Release.
    // Throws std::out_of_range if the key isn't cached.
    void Release(const std::string& key);

private:
    struct CachedShaderProgram {
        std::shared_ptr<ShaderProgram> shaderProgram;
        int referenceCount = 1;
    };

    Device& m_device;
    std::mutex m_mutex;
    std::unordered_map<std::string, CachedShaderProgram> m_shaderPrograms;
};

} // namespace arda::renderer
```

`renderer/src/shaders/ShaderCache.cpp`:

```cpp
#include <arda/renderer/shaders/ShaderCache.h>

#include <arda/renderer/Device.h>

#include <stdexcept>

namespace arda::renderer {

std::shared_ptr<ShaderProgram> ShaderCache::FindOrAdd(const std::string& key,
                                                      std::string_view vertexShaderSource,
                                                      std::string_view fragmentShaderSource) {
    return FindOrAdd(key, vertexShaderSource, {}, fragmentShaderSource);   // {} = no geometry shader
}

std::shared_ptr<ShaderProgram> ShaderCache::FindOrAdd(const std::string& key,
                                                      std::string_view vertexShaderSource,
                                                      std::string_view geometryShaderSource,
                                                      std::string_view fragmentShaderSource) {
    std::lock_guard lock(m_mutex);   // one coarse lock, as in the book

    if (auto it = m_shaderPrograms.find(key); it != m_shaderPrograms.end()) {
        ++it->second.referenceCount;
        return it->second.shaderProgram;
    }

    // If this throws (a compile error), nothing is added and the lock is released.
    auto program = m_device.CreateShaderProgram(vertexShaderSource, geometryShaderSource, fragmentShaderSource);
    m_shaderPrograms.emplace(key, CachedShaderProgram{program, 1});
    return program;
}

std::shared_ptr<ShaderProgram> ShaderCache::Find(const std::string& key) {
    std::lock_guard lock(m_mutex);
    auto it = m_shaderPrograms.find(key);
    if (it == m_shaderPrograms.end()) {
        return nullptr;
    }
    ++it->second.referenceCount;
    return it->second.shaderProgram;
}

void ShaderCache::Release(const std::string& key) {
    std::lock_guard lock(m_mutex);
    auto it = m_shaderPrograms.find(key);
    if (it == m_shaderPrograms.end()) {
        throw std::out_of_range("ShaderCache::Release: unknown key '" + key + "'");
    }
    if (--it->second.referenceCount == 0) {
        m_shaderPrograms.erase(it);   // the program is destroyed when its last shared_ptr goes away
    }
}

} // namespace arda::renderer
```

Notes on the port:

- **`std::mutex` and `std::lock_guard`** replace C#'s `lock (_shaderPrograms)`.
  The guard locks in its constructor and unlocks in its destructor, even when
  an exception leaves the function. See [Step 0](00-setup.md) for mutexes and
  RAII. `std::lock_guard lock(m_mutex);` doesn't write
  `std::lock_guard<std::mutex>`: C++17's class template argument deduction
  (CTAD) deduces it from the constructor argument.
- **Thread safety is only the map's.** The lock protects the cache's own data.
  `CreateShaderProgram` still makes GL calls, which need a current context on
  the calling thread. Creating GL resources from worker threads is Chapter 10.
- **`if (auto it = ...; it != end)`** is an `if` with an initializer (C++17):
  `it` exists only inside the `if` and its `else`, like a `for` loop variable.
- **`std::unordered_map`** is a hash table, like C#'s `Dictionary`
  ([Step 2](02-shaders.md)). Order doesn't matter for a cache, and lookups
  are O(1) on average.
- **`Device&`** is a reference member, as OpenGlobe's cache uses the static
  `Device`. The cache doesn't own the device and must be destroyed first.
- **`= delete` copy operations.** Copying a cache would duplicate reference
  counts that callers expect to release once. A `std::mutex` can't be copied
  anyway, so the compiler would delete them implicitly; writing it out makes
  the intent clear.

> **C++ note — `std::weak_ptr`, and a cache without manual counts:** a
> `std::weak_ptr<T>` observes an object owned by `shared_ptr`s without keeping
> it alive. It doesn't add to the strong count. To use the object you call
> `lock()`, which returns a `shared_ptr` that is either the object (now kept
> alive by you) or empty if every owner is gone:
>
> ```cpp
> std::weak_ptr<ShaderProgram> observer = program;   // no ownership
> program.reset();                                   // last owner gone: program destroyed
> if (auto p = observer.lock()) { /* not reached */ }
> ```
>
> Step 3's `ContextGL3x` already uses one (`m_boundShaderProgram`), to
> remember which program is bound without keeping it alive. A cache can use
> the same idea to let `shared_ptr` do the reference counting:
>
> ```cpp
> // An alternative design, not the one arda uses.
> std::shared_ptr<ShaderProgram> FindOrAdd(const std::string& key, std::string_view vs, std::string_view fs) {
>     std::lock_guard lock(m_mutex);
>     if (auto it = m_programs.find(key); it != m_programs.end()) {
>         if (auto program = it->second.lock()) {
>             return program;   // still alive somewhere
>         }
>     }
>     auto program = m_device.CreateShaderProgram(vs, fs);
>     m_programs.insert_or_assign(key, program);   // stores a weak_ptr
>     return program;
> }
> std::unordered_map<std::string, std::weak_ptr<ShaderProgram>> m_programs;
> ```
>
> No `Release`: when the last user drops its `shared_ptr`, the program is
> destroyed, and the next `FindOrAdd` sees an expired entry and recompiles.
> Arda keeps the book's explicit counting instead, because:
>
> - **It's the design 3.4.6 describes**, so the book and the code match.
> - **Expired entries pile up** in the weak version until the same key is
>   looked up again; it needs a periodic sweep to erase them.
> - **Explicit `Release` keeps a program alive between uses.** With weak
>   pointers, an object that destroys its last `shared_ptr` at the end of one
>   frame and asks again the next frame recompiles every time.
>
> Both are reasonable. The weak version is less error-prone (a forgotten
> `Release` can't leak); the counted one is more predictable.

Add to CMake:

```cmake
# renderer/CMakeLists.txt
target_sources(arda_renderer PRIVATE
    src/shaders/ShaderCache.cpp
    include/arda/renderer/shaders/ShaderCache.h
)
```

---

## CMake

Everything this step adds, in one place.

`core/CMakeLists.txt`, in `add_library(arda_core ...)` (headers, for the IDE):

```cmake
    include/arda/core/geometry/ClipDepth.h
    include/arda/core/geometry/Matrix4.h
```

`renderer/CMakeLists.txt`, after `add_library(arda_renderer ...)` (these are
API-agnostic, so they go outside the `if(ARDA_RENDERER_GL)` block):

```cmake
target_sources(arda_renderer PRIVATE
    src/scene/Camera.cpp
    src/scene/SceneState.cpp
    src/shaders/ShaderCache.cpp
    src/shaders/ShaderProgram.cpp
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.cpp

    # Headers, for the IDE
    include/arda/renderer/scene/Camera.h
    include/arda/renderer/scene/SceneState.h
    include/arda/renderer/shaders/AutomaticUniforms.h
    include/arda/renderer/shaders/ShaderCache.h
    src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.h
    src/shaders/automaticuniforms/Wgs84HeightUniform.h
)
```

No GL sources are added; `ShaderProgramGL3x.cpp` and `DeviceGL3x.cpp` are
already listed.

`tests/CMakeLists.txt`, in `add_executable(arda_tests ...)`:

```cmake
    src/core/Matrix4Tests.cpp
    src/renderer/SceneStateTests.cpp
    src/renderer/AutomaticUniformTests.cpp
```

## Tests

`Matrix4Tests` ([1.4](#14-matrix4tests)) and `SceneStateTests`
([2.3](#23-scenestatetests)) are shown above. This one needs a GPU: it creates
programs, and one test draws into a hidden window to prove that `Draw` really
sets the automatic uniforms.

`tests/src/renderer/AutomaticUniformTests.cpp`:

```cpp
#include <doctest/doctest.h>

#include <arda/core/geometry/Matrix4.h>
#include <arda/core/geometry/PrimitiveType.h>
#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Context.h>
#include <arda/renderer/Device.h>
#include <arda/renderer/DrawState.h>
#include <arda/renderer/GraphicsWindow.h>
#include <arda/renderer/scene/SceneState.h>
#include <arda/renderer/shaders/AutomaticUniforms.h>
#include <arda/renderer/shaders/ShaderCache.h>
#include <arda/renderer/vertexarray/VertexBufferAttribute.h>
#include <arda/renderer/vertexarray/VertexLocations.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

using namespace arda::renderer;
using arda::core::Matrix4;
using arda::core::Vector3;
using arda::core::geometry::PrimitiveType;

namespace {

constexpr const char* kVertexShader = R"(
layout(location = og_positionVertexLocation) in vec4 position;
uniform mat4 og_modelViewPerspectiveMatrix;
void main()
{
    gl_Position = og_modelViewPerspectiveMatrix * position;
})";

constexpr const char* kFragmentShader = R"(
out vec4 fragmentColor;
void main()
{
    fragmentColor = vec4(1.0);
})";

// A link automatic uniform an app might register: always 42.
class AnswerUniform final : public LinkAutomaticUniform {
public:
    std::string Name() const override { return "u_answer"; }
    void Set(UniformBase& uniform) const override { dynamic_cast<Uniform<int>&>(uniform).SetValue(42); }
};

} // namespace

TEST_CASE("Draw automatic uniform factories are registered by name") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    const auto& factories = device->DrawAutomaticUniformFactories();

    CHECK(factories.Contains("og_modelViewPerspectiveMatrix"));
    CHECK(factories.Contains("og_wgs84Height"));
    CHECK(factories.Contains("og_viewMatrix"));
    CHECK(factories.Contains("og_sunPosition"));
    CHECK_FALSE(factories.Contains("u_notAutomatic"));
    CHECK(factories.Find("u_notAutomatic") == nullptr);

    CHECK(device->ClipDepthRange() == arda::core::ClipDepth::NegativeOneToOne);
}

TEST_CASE("An automatic uniform with the wrong type throws") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);

    CHECK_NOTHROW(device->CreateShaderProgram(kVertexShader, kFragmentShader));
    CHECK_THROWS_AS(device->CreateShaderProgram(
        R"(layout(location = og_positionVertexLocation) in vec4 position;
           uniform float og_modelViewPerspectiveMatrix;
           void main() { gl_Position = position * og_modelViewPerspectiveMatrix; })",
        kFragmentShader), std::bad_cast);
}

TEST_CASE("Link automatic uniforms are set when the program is created") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    device->LinkAutomaticUniforms().Add(std::make_unique<AnswerUniform>());

    auto sp = device->CreateShaderProgram(
        kVertexShader,
        R"(out vec4 fragmentColor;
           uniform int u_answer;
           void main() { fragmentColor = vec4(float(u_answer) / 42.0); })");   // used, so it stays active

    CHECK(sp->Uniforms().Get<int>("u_answer").Value() == 42);
}

TEST_CASE("Context::Draw sets draw automatic uniforms from the SceneState") {
    float time = 0.0f;   // declared before the device, so it outlives the lambda that captures it

    auto device = CreateDevice(GraphicsApi::OpenGL33);
    auto window = device->CreateGraphicsWindow(64, 64, "test", WindowType::Hidden);
    Context& context = window->GetContext();

    // An app-defined draw automatic uniform, registered before the program is created.
    device->DrawAutomaticUniformFactories().Add(std::make_unique<FunctionDrawAutomaticUniformFactory<float>>(
        "u_time", [&time](const Context&, const DrawState&, const SceneState&) { return time; }));

    auto sp = device->CreateShaderProgram(
        kVertexShader,
        R"(out vec4 fragmentColor;
           uniform float u_time;
           void main() { fragmentColor = vec4(u_time); })");

    const std::array<Vector3<float>, 3> positions = {
        Vector3<float>(0.0f, 0.0f, 0.0f),
        Vector3<float>(1.0f, 0.0f, 0.0f),
        Vector3<float>(0.0f, 0.0f, 1.0f),
    };
    auto positionBuffer = device->CreateVertexBuffer(BufferHint::StaticDraw, sizeof(positions));
    positionBuffer->CopyFromSystemMemory(positions);

    DrawState drawState;
    drawState.shaderProgram = sp;
    drawState.vertexArray = context.CreateVertexArray();
    drawState.vertexArray->SetAttribute(VertexLocations::Position,
                                        VertexBufferAttribute(positionBuffer, ComponentDatatype::Float, 3));

    SceneState sceneState;
    sceneState.camera.ZoomToTarget(1.0);
    time = 2.5f;

    context.Draw(PrimitiveType::Triangles, drawState, sceneState);

    const auto& mvp = sp->Uniforms().Get<Matrix4<float>>("og_modelViewPerspectiveMatrix");
    CHECK(mvp.Value() == sceneState.ModelViewPerspectiveMatrix(device->ClipDepthRange()).Cast<float>());
    CHECK(sp->Uniforms().Get<float>("u_time").Value() == 2.5f);

    // Change the scene; the next draw picks it up.
    sceneState.camera.aspectRatio = 2.0;
    context.Draw(PrimitiveType::Triangles, drawState, sceneState);
    CHECK(mvp.Value() == sceneState.ModelViewPerspectiveMatrix(device->ClipDepthRange()).Cast<float>());
}

TEST_CASE("ShaderCache returns the same program for the same key") {
    auto device = CreateDevice(GraphicsApi::OpenGL33);
    ShaderCache cache(*device);

    auto a = cache.FindOrAdd("key", kVertexShader, kFragmentShader);   // count 1
    auto b = cache.Find("key");                                         // count 2
    auto c = cache.FindOrAdd("key", "ignored", "ignored");              // count 3: sources ignored
    CHECK(a == b);
    CHECK(a == c);
    CHECK(cache.Find("missing") == nullptr);

    cache.Release("key");
    cache.Release("key");
    CHECK(cache.Find("key") == a);   // count back to 2
    cache.Release("key");
    cache.Release("key");            // count 0: removed
    CHECK(cache.Find("key") == nullptr);
    CHECK_THROWS_AS(cache.Release("key"), std::out_of_range);

    // The cache dropped its reference, but a is still usable.
    CHECK(a->Uniforms().Contains("og_modelViewPerspectiveMatrix"));
}
```

A few details of the tests:

- **The `u_time` lambda captures `time` by reference.** Local variables are
  destroyed in reverse order of declaration. `time` is declared before
  `device`, so it's destroyed after the device, the registry and every copy of
  the lambda. Declared after `device`, it would be destroyed first, leaving
  the registry holding a dangling reference. In app code, make sure whatever a
  registered lambda captures by reference outlives the device, or capture by
  value.
- **`FindOrAdd("key", "ignored", "ignored")`** would fail to compile as GLSL,
  but it's never compiled: the key is found first. That's the documented
  behavior: the key identifies the program, not the sources.

## Complete CMake files after Step 4

The CMake sections above list only this step's additions. Here are the three
CMake files as they should look once Step 4 is done, with every step so far
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
    include/arda/renderer/scene/Camera.h
    include/arda/renderer/scene/SceneState.h
    include/arda/renderer/shaders/AutomaticUniforms.h
    include/arda/renderer/shaders/ShaderCache.h
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
    src/shaders/automaticuniforms/ModelViewPerspectiveMatrixUniform.h
    src/shaders/automaticuniforms/StandardDrawAutomaticUniforms.h
    src/shaders/automaticuniforms/Wgs84HeightUniform.h
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
    src/core/Matrix4Tests.cpp
    src/renderer/AutomaticUniformTests.cpp
    src/renderer/ContextGL3xTests.cpp
    src/renderer/DeviceTests.cpp
    src/renderer/RenderStateTests.cpp
    src/renderer/SceneStateTests.cpp
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

## D3D11 check

Most of this step is API-agnostic. What differs on D3D11 (Step 8 implements
it):

- **Clip depth.** `Device::ClipDepthRange()` already returns `ZeroToOne` for
  D3D11, and every projection, including the viewport matrices, goes through
  it. `SceneStateTests` already test both ranges.
- **HLSL.** Declare `float4x4 og_modelViewPerspectiveMatrix;` and multiply with
  `mul(og_modelViewPerspectiveMatrix, position)`, which is `M · v` like GLSL's
  `M * v`. HLSL packs matrices column-major by default; compiling with
  `D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR` (the README's decision) makes that
  explicit, so `UniformD3D11<Matrix4<float>>` copies `Data()` into the
  constant buffer unchanged, the same bytes GL gets.
- **Link automatics.** `og_textureN` never matches in HLSL, because textures
  aren't loose globals there (the README's sampler-binding decision).
- **Render-to-texture Y flip.** Step 8 adds `Context::ClipSpaceTransform()`
  and applies it in `ModelViewPerspectiveMatrixUniform::Set`. The same flip
  should also be applied to the other uniforms that produce clip coordinates:
  `og_perspectiveMatrix`, `og_orthographicMatrix`,
  `og_modelViewOrthographicMatrix`, `og_modelViewPerspectiveMatrixRelativeToEye`
  and `og_viewportOrthographicMatrix`.
- **Window coordinates.** D3D's window y grows downward.
  `og_viewportTransformationMatrix` produces arda's bottom-left window
  coordinates, so an HLSL shader that compares its result with `SV_Position`
  must flip y.
- **`ShaderProgramD3D11`** calls `InitializeAutomaticUniforms(device)` after
  reflecting its uniforms, and `SetDrawAutomaticUniforms` at the start of its
  `Clean`, exactly like the GL program.

## Checklist

- [ ] Core: `ClipDepth.h`, the full `Matrix4.h` (and `Vector4.h`'s includes from Step 2)
- [ ] `Matrix4Tests` pass, including the `static_assert`s
- [ ] `Camera` (`Camera.h`, `Camera.cpp`) and `SceneState` (`SceneState.h`, `SceneState.cpp`)
- [ ] `SceneStateTests` pass
- [ ] `AutomaticUniforms.h`: the three abstract classes, `NamedCollection`, `FunctionDrawAutomaticUniform(Factory)`
- [ ] `Device`: `ClipDepthRange`, the two registries, `InitializeCommon`
- [ ] `ShaderProgram::InitializeAutomaticUniforms` and `SetDrawAutomaticUniforms`
- [ ] `ModelViewPerspectiveMatrixUniform`, `Wgs84HeightUniform`, `StandardDrawAutomaticUniforms`
- [ ] GL: `ShaderProgramGL3x` takes `const Device&`, calls `InitializeAutomaticUniforms`, and `Clean` sets draw automatic uniforms first; `DeviceGL3x` calls `InitializeCommon` and passes `*this`
- [ ] Step 2 and Step 3 tests still pass
- [ ] `ShaderCache`, and `AutomaticUniformTests` pass
- [ ] **Milestone:** the xz-plane triangle is visible through the default camera, keeps its shape when the window is resized, and survives minimizing
