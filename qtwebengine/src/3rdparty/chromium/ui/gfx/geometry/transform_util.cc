// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/transform_util.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"

namespace gfx {

namespace {

SkScalar Length3(SkScalar v[3]) {
  double vd[3] = {v[0], v[1], v[2]};
  return SkDoubleToScalar(
      std::sqrt(vd[0] * vd[0] + vd[1] * vd[1] + vd[2] * vd[2]));
}

template <int n>
SkScalar Dot(const SkScalar* a, const SkScalar* b) {
  double total = 0.0;
  for (int i = 0; i < n; ++i)
    total += a[i] * b[i];
  return SkDoubleToScalar(total);
}

template <int n>
void Combine(SkScalar* out,
             const SkScalar* a,
             const SkScalar* b,
             double scale_a,
             double scale_b) {
  for (int i = 0; i < n; ++i)
    out[i] = SkDoubleToScalar(a[i] * scale_a + b[i] * scale_b);
}

void Cross3(SkScalar out[3], SkScalar a[3], SkScalar b[3]) {
  SkScalar x = a[1] * b[2] - a[2] * b[1];
  SkScalar y = a[2] * b[0] - a[0] * b[2];
  SkScalar z = a[0] * b[1] - a[1] * b[0];
  out[0] = x;
  out[1] = y;
  out[2] = z;
}

SkScalar Round(SkScalar n) {
  return SkDoubleToScalar(std::floor(double{n} + 0.5));
}

// Returns false if the matrix cannot be normalized.
bool Normalize(Transform& m) {
  if (m.rc(3, 3) == 0.0)
    // Cannot normalize.
    return false;

  SkScalar scale = SK_Scalar1 / m.rc(3, 3);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      m.set_rc(i, j, m.rc(i, j) * scale);

  return true;
}

Transform BuildPerspectiveMatrix(const DecomposedTransform& decomp) {
  Transform matrix;

  for (int i = 0; i < 4; i++)
    matrix.set_rc(3, i, decomp.perspective[i]);
  return matrix;
}

Transform BuildTranslationMatrix(const DecomposedTransform& decomp) {
  Transform matrix;
  matrix.Translate3d(SkDoubleToScalar(decomp.translate[0]),
                     SkDoubleToScalar(decomp.translate[1]),
                     SkDoubleToScalar(decomp.translate[2]));
  return matrix;
}

Transform BuildSnappedTranslationMatrix(DecomposedTransform decomp) {
  decomp.translate[0] = Round(decomp.translate[0]);
  decomp.translate[1] = Round(decomp.translate[1]);
  decomp.translate[2] = Round(decomp.translate[2]);
  return BuildTranslationMatrix(decomp);
}

Transform BuildRotationMatrix(const DecomposedTransform& decomp) {
  return Transform(decomp.quaternion);
}

Transform BuildSnappedRotationMatrix(const DecomposedTransform& decomp) {
  // Create snapped rotation.
  Transform rotation_matrix = BuildRotationMatrix(decomp);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      SkScalar value = rotation_matrix.rc(i, j);
      // Snap values to -1, 0 or 1.
      if (value < -0.5f) {
        value = -1.0f;
      } else if (value > 0.5f) {
        value = 1.0f;
      } else {
        value = 0.0f;
      }
      rotation_matrix.set_rc(i, j, value);
    }
  }
  return rotation_matrix;
}

Transform BuildSkewMatrix(const DecomposedTransform& decomp) {
  Transform matrix;

  Transform temp;
  if (decomp.skew[2]) {
    temp.set_rc(1, 2, decomp.skew[2]);
    matrix.PreConcat(temp);
  }

  if (decomp.skew[1]) {
    temp.set_rc(1, 2, 0);
    temp.set_rc(0, 2, decomp.skew[1]);
    matrix.PreConcat(temp);
  }

  if (decomp.skew[0]) {
    temp.set_rc(0, 2, 0);
    temp.set_rc(0, 1, decomp.skew[0]);
    matrix.PreConcat(temp);
  }
  return matrix;
}

Transform BuildScaleMatrix(const DecomposedTransform& decomp) {
  Transform matrix;
  matrix.Scale3d(SkDoubleToScalar(decomp.scale[0]),
                 SkDoubleToScalar(decomp.scale[1]),
                 SkDoubleToScalar(decomp.scale[2]));
  return matrix;
}

Transform BuildSnappedScaleMatrix(DecomposedTransform decomp) {
  decomp.scale[0] = Round(decomp.scale[0]);
  decomp.scale[1] = Round(decomp.scale[1]);
  decomp.scale[2] = Round(decomp.scale[2]);
  return BuildScaleMatrix(decomp);
}

Transform ComposeTransform(const Transform& perspective,
                           const Transform& translation,
                           const Transform& rotation,
                           const Transform& skew,
                           const Transform& scale) {
  Transform matrix;

  matrix.PreConcat(perspective);
  matrix.PreConcat(translation);
  matrix.PreConcat(rotation);
  matrix.PreConcat(skew);
  matrix.PreConcat(scale);

  return matrix;
}

bool CheckViewportPointMapsWithinOnePixel(const Point& point,
                                          const Transform& transform) {
  auto point_original = Point3F(PointF(point));

  // Can't use TransformRect here since it would give us the axis-aligned
  // bounding rect of the 4 points in the initial rectable which is not what we
  // want.
  auto point_transformed = transform.MapPoint(point_original);

  if ((point_transformed - point_original).Length() > 1.f) {
    // The changed distance should not be more than 1 pixel.
    return false;
  }
  return true;
}

bool CheckTransformsMapsIntViewportWithinOnePixel(const Rect& viewport,
                                                  const Transform& original,
                                                  const Transform& snapped) {
  Transform original_inv(Transform::kSkipInitialization);
  bool invertible = true;
  invertible &= original.GetInverse(&original_inv);
  DCHECK(invertible) << "Non-invertible transform, cannot snap.";

  Transform combined = snapped * original_inv;

  return CheckViewportPointMapsWithinOnePixel(viewport.origin(), combined) &&
         CheckViewportPointMapsWithinOnePixel(viewport.top_right(), combined) &&
         CheckViewportPointMapsWithinOnePixel(viewport.bottom_left(),
                                              combined) &&
         CheckViewportPointMapsWithinOnePixel(viewport.bottom_right(),
                                              combined);
}

bool Is2dTransform(const Transform& transform) {
  return !transform.HasPerspective() && transform.IsFlat();
}

bool Decompose2DTransform(DecomposedTransform* decomp,
                          const Transform& transform) {
  if (!Is2dTransform(transform)) {
    return false;
  }

  double m11 = transform.rc(0, 0);
  double m21 = transform.rc(0, 1);
  double m12 = transform.rc(1, 0);
  double m22 = transform.rc(1, 1);

  double determinant = m11 * m22 - m12 * m21;
  // Test for matrix being singular.
  if (determinant == 0) {
    return false;
  }

  // Translation transform.
  // [m11 m21 0 m41]    [1 0 0 Tx] [m11 m21 0 0]
  // [m12 m22 0 m42]  = [0 1 0 Ty] [m12 m22 0 0]
  // [ 0   0  1  0 ]    [0 0 1 0 ] [ 0   0  1 0]
  // [ 0   0  0  1 ]    [0 0 0 1 ] [ 0   0  0 1]
  decomp->translate[0] = transform.rc(0, 3);
  decomp->translate[1] = transform.rc(1, 3);

  // For the remainder of the decomposition process, we can focus on the upper
  // 2x2 submatrix
  // [m11 m21] = [cos(R) -sin(R)] [1 K] [Sx 0 ]
  // [m12 m22]   [sin(R)  cos(R)] [0 1] [0  Sy]
  //           = [Sx*cos(R) Sy*(K*cos(R) - sin(R))]
  //             [Sx*sin(R) Sy*(K*sin(R) + cos(R))]

  // Determine sign of the x and y scale.
  if (determinant < 0) {
    // If the determinant is negative, we need to flip either the x or y scale.
    // Flipping both is equivalent to rotating by 180 degrees.
    if (m11 < m22) {
      decomp->scale[0] *= -1;
    } else {
      decomp->scale[1] *= -1;
    }
  }

  // X Scale.
  // m11^2 + m12^2 = Sx^2*(cos^2(R) + sin^2(R)) = Sx^2.
  // Sx = +/-sqrt(m11^2 + m22^2)
  decomp->scale[0] *= sqrt(m11 * m11 + m12 * m12);
  m11 /= decomp->scale[0];
  m12 /= decomp->scale[0];

  // Post normalization, the submatrix is now of the form:
  // [m11 m21] = [cos(R)  Sy*(K*cos(R) - sin(R))]
  // [m12 m22]   [sin(R)  Sy*(K*sin(R) + cos(R))]

  // XY Shear.
  // m11 * m21 + m12 * m22 = Sy*K*cos^2(R) - Sy*sin(R)*cos(R) +
  //                         Sy*K*sin^2(R) + Sy*cos(R)*sin(R)
  //                       = Sy*K
  double scaledShear = m11 * m21 + m12 * m22;
  m21 -= m11 * scaledShear;
  m22 -= m12 * scaledShear;

  // Post normalization, the submatrix is now of the form:
  // [m11 m21] = [cos(R)  -Sy*sin(R)]
  // [m12 m22]   [sin(R)   Sy*cos(R)]

  // Y Scale.
  // Similar process to determining x-scale.
  decomp->scale[1] *= sqrt(m21 * m21 + m22 * m22);
  m21 /= decomp->scale[1];
  m22 /= decomp->scale[1];
  decomp->skew[0] = scaledShear / decomp->scale[1];

  // Rotation transform.
  // [1-2(yy+zz)  2(xy-zw)    2(xz+yw) ]   [cos(R) -sin(R)  0]
  // [2(xy+zw)   1-2(xx+zz)   2(yz-xw) ] = [sin(R)  cos(R)  0]
  // [2(xz-yw)    2*(yz+xw)  1-2(xx+yy)]   [  0       0     1]
  // Comparing terms, we can conclude that x = y = 0.
  // [1-2zz   -2zw  0]   [cos(R) -sin(R)  0]
  // [ 2zw   1-2zz  0] = [sin(R)  cos(R)  0]
  // [  0     0     1]   [  0       0     1]
  // cos(R) = 1 - 2*z^2
  // From the double angle formula: cos(2a) = 1 - 2 sin(a)^2
  // cos(R) = 1 - 2*sin(R/2)^2 = 1 - 2*z^2 ==> z = sin(R/2)
  // sin(R) = 2*z*w
  // But sin(2a) = 2 sin(a) cos(a)
  // sin(R) = 2 sin(R/2) cos(R/2) = 2*z*w ==> w = cos(R/2)
  double angle = atan2(m12, m11);
  decomp->quaternion.set_x(0);
  decomp->quaternion.set_y(0);
  decomp->quaternion.set_z(sin(0.5 * angle));
  decomp->quaternion.set_w(cos(0.5 * angle));

  return true;
}

}  // namespace

Transform GetScaleTransform(const Point& anchor, float scale) {
  Transform transform;
  transform.Translate(anchor.x() * (1 - scale), anchor.y() * (1 - scale));
  transform.Scale(scale, scale);
  return transform;
}

DecomposedTransform::DecomposedTransform() {
  translate[0] = translate[1] = translate[2] = 0.0;
  scale[0] = scale[1] = scale[2] = 1.0;
  skew[0] = skew[1] = skew[2] = 0.0;
  perspective[0] = perspective[1] = perspective[2] = 0.0;
  perspective[3] = 1.0;
}

DecomposedTransform BlendDecomposedTransforms(const DecomposedTransform& to,
                                              const DecomposedTransform& from,
                                              double progress) {
  DecomposedTransform out;
  double scalea = progress;
  double scaleb = 1.0 - progress;
  Combine<3>(out.translate, to.translate, from.translate, scalea, scaleb);
  Combine<3>(out.scale, to.scale, from.scale, scalea, scaleb);
  Combine<3>(out.skew, to.skew, from.skew, scalea, scaleb);
  Combine<4>(out.perspective, to.perspective, from.perspective, scalea, scaleb);
  out.quaternion = from.quaternion.Slerp(to.quaternion, progress);
  return out;
}

// Taken from http://www.w3.org/TR/css3-transforms/.
// TODO(crbug/937296): This implementation is virtually identical to the
// implementation in blink::TransformationMatrix with the main difference being
// the representation of the underlying matrix. These implementations should be
// consolidated.
bool DecomposeTransform(DecomposedTransform* decomp,
                        const Transform& transform) {
  if (!decomp)
    return false;

  if (Decompose2DTransform(decomp, transform))
    return true;

  // We'll operate on a copy of the transform.
  Transform matrix = transform;

  // If we cannot normalize the matrix, then bail early as we cannot decompose.
  if (!Normalize(matrix))
    return false;

  Transform perspective_matrix = matrix;

  for (int i = 0; i < 3; ++i)
    perspective_matrix.set_rc(3, i, 0.0);

  perspective_matrix.set_rc(3, 3, 1.0);

  // If the perspective matrix is not invertible, we are also unable to
  // decompose, so we'll bail early.
  if (!perspective_matrix.IsInvertible())
    return false;

  if (matrix.HasPerspective()) {
    // rhs is the right hand side of the equation.
    SkScalar rhs[4] = {matrix.rc(3, 0), matrix.rc(3, 1), matrix.rc(3, 2),
                       matrix.rc(3, 3)};

    // Solve the equation by inverting perspectiveMatrix and multiplying
    // rhs by the inverse.
    Transform inverse_perspective_matrix(Transform::kSkipInitialization);
    if (!perspective_matrix.GetInverse(&inverse_perspective_matrix))
      return false;

    Transform transposed_inverse_perspective_matrix =
        inverse_perspective_matrix;

    transposed_inverse_perspective_matrix.Transpose();
    transposed_inverse_perspective_matrix.TransformVector4(rhs);

    for (int i = 0; i < 4; ++i)
      decomp->perspective[i] = rhs[i];

  } else {
    // No perspective.
    for (int i = 0; i < 3; ++i)
      decomp->perspective[i] = 0.0;
    decomp->perspective[3] = 1.0;
  }

  for (int i = 0; i < 3; i++)
    decomp->translate[i] = matrix.rc(i, 3);

  // Copy of matrix is stored in column major order to facilitate column-level
  // operations.
  SkScalar column[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; ++j)
      column[i][j] = matrix.rc(j, i);

  // Compute X scale factor and normalize first column.
  decomp->scale[0] = Length3(column[0]);
  if (decomp->scale[0] != 0.0) {
    column[0][0] /= decomp->scale[0];
    column[0][1] /= decomp->scale[0];
    column[0][2] /= decomp->scale[0];
  }

  // Compute XY shear factor and make 2nd column orthogonal to 1st.
  decomp->skew[0] = Dot<3>(column[0], column[1]);
  Combine<3>(column[1], column[1], column[0], 1.0, -decomp->skew[0]);

  // Now, compute Y scale and normalize 2nd column.
  decomp->scale[1] = Length3(column[1]);
  if (decomp->scale[1] != 0.0) {
    column[1][0] /= decomp->scale[1];
    column[1][1] /= decomp->scale[1];
    column[1][2] /= decomp->scale[1];
  }

  decomp->skew[0] /= decomp->scale[1];

  // Compute XZ and YZ shears, orthogonalize the 3rd column.
  decomp->skew[1] = Dot<3>(column[0], column[2]);
  Combine<3>(column[2], column[2], column[0], 1.0, -decomp->skew[1]);
  decomp->skew[2] = Dot<3>(column[1], column[2]);
  Combine<3>(column[2], column[2], column[1], 1.0, -decomp->skew[2]);

  // Next, get Z scale and normalize the 3rd column.
  decomp->scale[2] = Length3(column[2]);
  if (decomp->scale[2] != 0.0) {
    column[2][0] /= decomp->scale[2];
    column[2][1] /= decomp->scale[2];
    column[2][2] /= decomp->scale[2];
  }

  decomp->skew[1] /= decomp->scale[2];
  decomp->skew[2] /= decomp->scale[2];

  // At this point, the matrix is orthonormal.
  // Check for a coordinate system flip.  If the determinant
  // is -1, then negate the matrix and the scaling factors.
  // TODO(kevers): This is inconsistent from the 2D specification, in which
  // only 1 axis is flipped when the determinant is negative. Verify if it is
  // correct to flip all of the scales and matrix elements, as this introduces
  // rotation for the simple case of a single axis scale inversion.
  SkScalar pdum3[3];
  Cross3(pdum3, column[1], column[2]);
  if (Dot<3>(column[0], pdum3) < 0) {
    for (int i = 0; i < 3; i++) {
      decomp->scale[i] *= -1.0;
      for (int j = 0; j < 3; ++j)
        column[i][j] *= -1.0;
    }
  }

  // See https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion.
  // Note: deviating from spec (http://www.w3.org/TR/css3-transforms/)
  // which has a degenerate case of zero off-diagonal elements in the
  // orthonormal matrix, which leads to errors in determining the sign
  // of the quaternions.
  double q_xx = column[0][0];
  double q_xy = column[1][0];
  double q_xz = column[2][0];
  double q_yx = column[0][1];
  double q_yy = column[1][1];
  double q_yz = column[2][1];
  double q_zx = column[0][2];
  double q_zy = column[1][2];
  double q_zz = column[2][2];

  double r, s, t, x, y, z, w;
  t = q_xx + q_yy + q_zz;
  if (t > 0) {
    r = std::sqrt(1.0 + t);
    s = 0.5 / r;
    w = 0.5 * r;
    x = (q_zy - q_yz) * s;
    y = (q_xz - q_zx) * s;
    z = (q_yx - q_xy) * s;
  } else if (q_xx > q_yy && q_xx > q_zz) {
    r = std::sqrt(1.0 + q_xx - q_yy - q_zz);
    s = 0.5 / r;
    x = 0.5 * r;
    y = (q_xy + q_yx) * s;
    z = (q_xz + q_zx) * s;
    w = (q_zy - q_yz) * s;
  } else if (q_yy > q_zz) {
    r = std::sqrt(1.0 - q_xx + q_yy - q_zz);
    s = 0.5 / r;
    x = (q_xy + q_yx) * s;
    y = 0.5 * r;
    z = (q_yz + q_zy) * s;
    w = (q_xz - q_zx) * s;
  } else {
    r = std::sqrt(1.0 - q_xx - q_yy + q_zz);
    s = 0.5 / r;
    x = (q_xz + q_zx) * s;
    y = (q_yz + q_zy) * s;
    z = 0.5 * r;
    w = (q_yx - q_xy) * s;
  }

  decomp->quaternion.set_x(SkDoubleToScalar(x));
  decomp->quaternion.set_y(SkDoubleToScalar(y));
  decomp->quaternion.set_z(SkDoubleToScalar(z));
  decomp->quaternion.set_w(SkDoubleToScalar(w));

  return true;
}

// Taken from http://www.w3.org/TR/css3-transforms/.
Transform ComposeTransform(const DecomposedTransform& decomp) {
  Transform perspective = BuildPerspectiveMatrix(decomp);
  Transform translation = BuildTranslationMatrix(decomp);
  Transform rotation = BuildRotationMatrix(decomp);
  Transform skew = BuildSkewMatrix(decomp);
  Transform scale = BuildScaleMatrix(decomp);

  return ComposeTransform(perspective, translation, rotation, skew, scale);
}

bool SnapTransform(Transform* out,
                   const Transform& transform,
                   const Rect& viewport) {
  DecomposedTransform decomp;
  DecomposeTransform(&decomp, transform);

  Transform rotation_matrix = BuildSnappedRotationMatrix(decomp);
  Transform translation = BuildSnappedTranslationMatrix(decomp);
  Transform scale = BuildSnappedScaleMatrix(decomp);

  // Rebuild matrices for other unchanged components.
  Transform perspective = BuildPerspectiveMatrix(decomp);

  // Completely ignore the skew.
  Transform skew;

  // Get full transform.
  Transform snapped =
      ComposeTransform(perspective, translation, rotation_matrix, skew, scale);

  // Verify that viewport is not moved unnaturally.
  bool snappable = CheckTransformsMapsIntViewportWithinOnePixel(
      viewport, transform, snapped);
  if (snappable) {
    *out = snapped;
  }
  return snappable;
}

Transform TransformAboutPivot(const PointF& pivot, const Transform& transform) {
  Transform result;
  result.Translate(pivot.x(), pivot.y());
  result.PreConcat(transform);
  result.Translate(-pivot.x(), -pivot.y());
  return result;
}

Transform TransformBetweenRects(const RectF& src, const RectF& dst) {
  DCHECK(!src.IsEmpty());
  Transform result;
  result.Translate(dst.origin() - src.origin());
  result.Scale(dst.width() / src.width(), dst.height() / src.height());
  return result;
}

std::string DecomposedTransform::ToString() const {
  return base::StringPrintf(
      "translate: %+0.4f %+0.4f %+0.4f\n"
      "scale: %+0.4f %+0.4f %+0.4f\n"
      "skew: %+0.4f %+0.4f %+0.4f\n"
      "perspective: %+0.4f %+0.4f %+0.4f %+0.4f\n"
      "quaternion: %+0.4f %+0.4f %+0.4f %+0.4f\n",
      translate[0], translate[1], translate[2], scale[0], scale[1], scale[2],
      skew[0], skew[1], skew[2], perspective[0], perspective[1], perspective[2],
      perspective[3], quaternion.x(), quaternion.y(), quaternion.z(),
      quaternion.w());
}

AxisTransform2d OrthoProjectionTransform(float left,
                                         float right,
                                         float bottom,
                                         float top) {
  // Use the standard formula to map the clipping frustum to the square from
  // [-1, -1] to [1, 1].
  float delta_x = right - left;
  float delta_y = top - bottom;
  if (!delta_x || !delta_y)
    return AxisTransform2d();

  return AxisTransform2d::FromScaleAndTranslation(
      Vector2dF(2.0f / delta_x, 2.0f / delta_y),
      Vector2dF(-(right + left) / delta_x, -(top + bottom) / delta_y));
}

AxisTransform2d WindowTransform(int x, int y, int width, int height) {
  // Map from ([-1, -1] to [1, 1]) -> ([x, y] to [x + width, y + height]).
  return AxisTransform2d::FromScaleAndTranslation(
      Vector2dF(width * 0.5f, height * 0.5f),
      Vector2dF(x + width * 0.5f, y + height * 0.5f));
}

static inline bool NearlyZero(double value) {
  return std::abs(value) < std::numeric_limits<double>::epsilon();
}

static inline float ScaleOnAxis(double a, double b, double c) {
  if (NearlyZero(b) && NearlyZero(c))
    return std::abs(a);
  if (NearlyZero(a) && NearlyZero(c))
    return std::abs(b);
  if (NearlyZero(a) && NearlyZero(b))
    return std::abs(c);

  // Do the sqrt as a double to not lose precision.
  return static_cast<float>(std::sqrt(a * a + b * b + c * c));
}

absl::optional<Vector2dF> TryComputeTransform2dScaleComponents(
    const Transform& transform) {
  if (transform.rc(3, 0) != 0.0f || transform.rc(3, 1) != 0.0f) {
    return absl::nullopt;
  }

  float w = transform.rc(3, 3);
  if (!std::isnormal(w)) {
    return absl::nullopt;
  }
  float w_scale = 1.0f / w;

  // In theory, this shouldn't be using the matrix.getDouble(2, 0) and
  // .getDouble(1, 0) values; creating a large transfer from input x or
  // y (in the layer) to output z has no visible difference when the
  // transform being considered is a transform to device space, since
  // the resulting z values are ignored.  However, ignoring them here
  // might be risky because it would mean that we would have more
  // variation in the results under animation of rotateX() or rotateY(),
  // and we'd be relying more heavily on code to compute correct scales
  // during animation.  Currently some such code only considers the
  // endpoints, which would become problematic for cases like animation
  // from rotateY(-60deg) to rotateY(60deg).
  float x_scale =
      ScaleOnAxis(transform.rc(0, 0), transform.rc(1, 0), transform.rc(2, 0));
  float y_scale =
      ScaleOnAxis(transform.rc(0, 1), transform.rc(1, 1), transform.rc(2, 1));
  return Vector2dF(x_scale * w_scale, y_scale * w_scale);
}

Vector2dF ComputeTransform2dScaleComponents(const Transform& transform,
                                            float fallback_value) {
  absl::optional<Vector2dF> scale =
      TryComputeTransform2dScaleComponents(transform);
  if (scale) {
    return *scale;
  }
  return Vector2dF(fallback_value, fallback_value);
}

float ComputeApproximateMaxScale(const Transform& transform) {
  gfx::RectF unit = transform.MapRect(RectF(0.f, 0.f, 1.f, 1.f));
  return std::max(unit.width(), unit.height());
}

}  // namespace gfx
