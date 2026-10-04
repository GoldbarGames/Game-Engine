#ifndef KINJO_LIGHTS_GLSL
#define KINJO_LIGHTS_GLSL
// The scene's point and spot lights, gathered per pixel - clustered forward
// lighting, Phase 1.5 item 8 (src/ENGINE/render/ClusteredLights.h).
//
// The view frustum is cut into screen tiles x depth slices; the engine lists
// which lights reach each cluster. A pixel finds its cluster and shades only
// those lights, so a scene can hold hundreds of lights while each pixel pays
// for the few near it. Lights arrive in the same order as before (points,
// then spots), so the sums come out exactly as they did.
//
// Without clusters (`clusteredLights 0`, or nothing built this frame) the
// Scene block's arrays serve instead: its first 8 point and 4 spot lights.
//
//     int first, count;
//     LightRange(worldPos, first, count);
//     for (int k = first; k < first + count; k++) { SceneLight l = GetLight(k); ... }
//
// Include after scene.glsl and camera.glsl.

layout(std140) uniform Clusters
{
	ivec4 clusterGrid;     // tiles x, tiles y, depth slices, 1 = clustered lights on
	vec4  clusterParams;   // slice = log(depth) * x + y; z, w = 1 / target width, height
	ivec4 clusterLayout;   // texture width, first table texel, first index texel, 1 = debug view
};

// RGBA32UI, read with texelFetch: light records (4 texels each, floats as
// bits), then a (first, count) entry per cluster, then the light lists (4
// indices per texel). Unit 14.
uniform highp usampler2D clusterData;

struct SceneLight
{
	vec3  pos;
	float range;
	vec3  color;
	float intensity;
	vec3  dir;        // spot only
	float cosOuter;   // spot only
	float cosInner;   // spot only
	int   spot;       // 0 = point, 1 = spot
	int   shadow;     // index into the Scene block's point-shadow arrays, or -1
};

uvec4 ClusterTexel(int index)
{
	int w = clusterLayout.x;
	return texelFetch(clusterData, ivec2(index % w, index / w), 0);
}

// The lights that can reach worldPos, seen at full-resolution pixel `pixel`
// (window coordinates): list entries [first, first + count).
void LightRangeAt(vec2 pixel, vec3 worldPos, out int first, out int count)
{
	if (clusterGrid.w == 0)
	{
		first = 0;
		count = pointCount + spotCount;
		return;
	}
	float depth = -(view * vec4(worldPos, 1.0)).z;
	int slice = clamp(int(floor(log(max(depth, 1.0e-4)) * clusterParams.x + clusterParams.y)), 0, clusterGrid.z - 1);
	ivec2 tile = clamp(ivec2(pixel * clusterParams.zw * vec2(clusterGrid.xy)), ivec2(0), clusterGrid.xy - 1);
	uvec4 entry = ClusterTexel(clusterLayout.y + (slice * clusterGrid.y + tile.y) * clusterGrid.x + tile.x);
	first = int(entry.x);
	count = int(entry.y);
}

// The lights that can reach a fragment at worldPos (this draw's own pixel).
void LightRange(vec3 worldPos, out int first, out int count)
{
	LightRangeAt(gl_FragCoord.xy, worldPos, first, count);
}

SceneLight GetLight(int k)
{
	SceneLight l;
	if (clusterGrid.w == 0)
	{
		// The Scene block's packed arrays: points first, then spots.
		l.dir = vec3(0.0);
		l.cosOuter = 0.0;
		l.cosInner = 0.0;
		l.shadow = -1;
		if (k < pointCount)
		{
			l.pos = pointPos[k];
			l.range = pointRange[k];
			l.color = pointColor[k];
			l.intensity = pointIntensity[k];
			l.spot = 0;
			for (int s = 0; s < MAX_PT_SHADOW_SLOTS; s++)
			{
				if (s >= pointShadowCount)
					break;
				if (pointShadowLightIdx[s] == k)
				{
					l.shadow = s;
					break;
				}
			}
		}
		else
		{
			int i = k - pointCount;
			l.pos = spotPos[i];
			l.range = spotRange[i];
			l.color = spotColor[i];
			l.intensity = spotIntensity[i];
			l.dir = spotDir[i];
			l.cosOuter = spotCosOuter[i];
			l.cosInner = spotCosInner[i];
			l.spot = 1;
		}
		return l;
	}

	int index = int(ClusterTexel(clusterLayout.z + k / 4)[k % 4]);
	uvec4 a = ClusterTexel(index * 4);
	uvec4 b = ClusterTexel(index * 4 + 1);
	uvec4 c = ClusterTexel(index * 4 + 2);
	uvec4 d = ClusterTexel(index * 4 + 3);
	l.pos = uintBitsToFloat(a.xyz);
	l.range = uintBitsToFloat(a.w);
	l.color = uintBitsToFloat(b.xyz);
	l.intensity = uintBitsToFloat(b.w);
	l.dir = uintBitsToFloat(c.xyz);
	l.cosOuter = uintBitsToFloat(c.w);
	l.cosInner = uintBitsToFloat(d.x);
	l.spot = int(d.y);
	l.shadow = int(d.z) - 1;
	return l;
}

// KINJO_CLUSTER_DEBUG: how many lights reach a fragment, as a heat colour
// (black 0, blue 1-2, green ~4, yellow ~8, red 16+).
bool ClusterDebugView()
{
	return clusterGrid.w != 0 && clusterLayout.w != 0;
}

vec3 ClusterHeat(int count)
{
	float t = clamp(float(count) / 16.0, 0.0, 1.0);
	if (count == 0)
		return vec3(0.0);
	return clamp(vec3(4.0 * t - 2.0, 2.0 - abs(4.0 * t - 2.0), 2.0 - 4.0 * t), 0.0, 1.0);
}
#endif
