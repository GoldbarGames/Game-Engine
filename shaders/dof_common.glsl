#ifndef KINJO_DOF_COMMON_GLSL
#define KINJO_DOF_COMMON_GLSL
// Shared by the depth-of-field passes (src/ENGINE/render/DepthOfField.h).

// Linear view depth from a depth-buffer value.
float DofViewDepth(float d, float nearPlane, float farPlane)
{
	float z = d * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Signed blur size (circle of confusion) in pixels for a thin lens focused at
// `focus`: negative in front of the focus, positive behind; levels off at
// `aperture` towards infinity, capped at maxBlur either way.
float DofCoc(float depth, float focus, float aperture, float maxBlur)
{
	return clamp(aperture * (1.0 - focus / max(depth, 1.0e-3)), -maxBlur, maxBlur);
}
#endif
