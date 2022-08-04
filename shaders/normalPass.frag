#version 330 core

uniform vec3 uLightPosition;
uniform float uLightNearPlane;
uniform float uLightFarPlane;
uniform float uLightWidthUV; // lightWidth / frustumWidth
uniform sampler2D uDepthBuffer;
// Same texture as uDepthBuffer, but different variable type.
uniform sampler2DShadow uShadowSampler;
uniform int uShadowQuality; // Possible values: 0, 1, 2, 3
uniform float uFilterRadius;
uniform bool uEnablePCSS;

in vec3 vNormal;
in vec3 vWorldPosition;
in vec3 vColor;
in vec4 vShadowCoordinates;

float debug = 0.0;

vec2 POISSON8[] = vec2[8](
	vec2(-0.2602728,0.3234085), vec2(-0.3268174,0.0442592), vec2(0.1996002,0.1386711),
	vec2(0.2615348,-0.1569698), vec2(-0.2869459,-0.3421305), vec2(0.1351001,-0.4352284),
	vec2(-0.0635913,-0.1520724), vec2(0.1454225,0.4629610)
);

vec2 POISSON16[] = vec2[16](
	vec2(0.3040781,-0.1861200), vec2(0.1485699,-0.0405212), vec2(0.4016555,0.1252352),
	vec2(-0.1526961,-0.1404687), vec2(0.3480717,0.3260515), vec2(0.0584860,-0.3266001),
	vec2(0.0891062,0.2332856), vec2(-0.3487481,-0.0159209), vec2(-0.1847383,0.1410431),
	vec2(0.4678784,-0.0888323), vec2(0.1134236,0.4119219), vec2(0.2856628,-0.3658066),
	vec2(-0.1765543,0.3937907), vec2(-0.0238326,0.0518298), vec2(-0.2949835,-0.3029899),
	vec2(-0.4593541,0.1720255)
);

vec2 POISSON32[] = vec2[32](
	vec2(0.2981409,0.0490049), vec2(0.1629048,-0.1408463), vec2(0.1691782,-0.3703386),
	vec2(0.3708196,0.2632940), vec2(-0.0602839,-0.2213077), vec2(0.3062163,-0.1364151),
	vec2(0.0094440,-0.0299901), vec2(-0.0753952,-0.3944479), vec2(-0.2073224,-0.3717136),
	vec2(0.1254510,0.0428502), vec2(0.2816537,-0.3045711), vec2(-0.2343018,-0.2459390),
	vec2(0.0625516,-0.2719784), vec2(-0.3949863,-0.2474681), vec2(0.0501389,-0.4359268),
	vec2(-0.1602987,-0.0242505), vec2(0.3998221,0.1279425), vec2(0.1698757,0.2820195),
	vec2(0.4191946,-0.0148812), vec2(0.4103152,-0.2532885), vec2(-0.0010199,0.3389769),
	vec2(-0.2646317,-0.1102498), vec2(0.2064117,0.4451604), vec2(-0.0788299,0.1059370),
	vec2(-0.3209068,0.1344933), vec2(0.0868388,0.1710649), vec2(-0.3878541,-0.0204674),
	vec2(-0.4418672,0.1825800), vec2(-0.3623412,0.3157248), vec2(-0.1956292,0.2076620),
	vec2(0.0205688,0.4664732), vec2(-0.1860556,0.4323920)
);

vec2 POISSON64[] = vec2[64](
	vec2(-0.0189662,-0.0510488), vec2(-0.1820639,-0.0553801), vec2(0.0910325,0.0252679),
	vec2(0.1096571,-0.0798338), vec2(-0.1469904,0.1132023), vec2(0.2343081,-0.1905298),
	vec2(-0.0029982,0.0958551), vec2(0.3510874,-0.1930093), vec2(0.0468733,-0.1524058),
	vec2(-0.1218595,-0.2167346), vec2(0.2739988,-0.0158153), vec2(0.1341032,-0.2588954),
	vec2(0.2062096,-0.0821571), vec2(-0.1026306,-0.0041678), vec2(-0.3240024,-0.0798507),
	vec2(0.3697911,0.0458827), vec2(-0.2538350,-0.2965067), vec2(-0.2396912,0.0628588),
	vec2(-0.3017254,-0.1893546), vec2(0.2113072,-0.3186852), vec2(0.0559174,0.2359820),
	vec2(-0.3721051,0.0980429), vec2(-0.1430048,0.2194094), vec2(-0.0514073,0.3617615),
	vec2(-0.2960384,0.1891084), vec2(-0.0552694,0.1748697), vec2(-0.0987295,-0.1174246),
	vec2(0.3565632,0.1850419), vec2(0.1723162,-0.4579452), vec2(0.3403926,-0.3167597),
	vec2(-0.1414267,0.4724176), vec2(-0.4680430,-0.1488462), vec2(0.2291788,0.1936403),
	vec2(-0.1400955,-0.4132020), vec2(0.1192180,-0.3781818), vec2(-0.3150060,0.3645030),
	vec2(0.1893810,0.0889743), vec2(0.0909581,0.1423441), vec2(-0.0500480,-0.4849751),
	vec2(-0.2104492,0.2853596), vec2(0.3527338,0.3100588), vec2(0.0354831,0.4304752),
	vec2(0.4190884,-0.0489801), vec2(0.1890273,0.3002760), vec2(0.4564034,0.0862838),
	vec2(0.1851432,0.4389251), vec2(-0.0038145,-0.2962559), vec2(0.0485585,0.3323395),
	vec2(0.2843748,0.0984157), vec2(0.4504704,-0.1657754), vec2(-0.3932974,-0.2612363),
	vec2(-0.2073296,0.3838763), vec2(-0.4316504,0.2052262), vec2(-0.2043341,-0.1549807),
	vec2(-0.3898448,0.3030459), vec2(-0.4078800,-0.0078618), vec2(-0.2387565,-0.4155289),
	vec2(-0.0335876,0.2676137), vec2(0.0709581,-0.4616181), vec2(-0.3274855,-0.3756900),
	vec2(-0.0448154,0.4841810), vec2(-0.4669865,0.1102869), vec2(-0.0956072,-0.3239126),
	vec2(0.2771143,0.3817498)
);

float PCF(vec3 uvz, float filterRadius) {
	float sum = 0.0;
	int sampleCount = int(pow(2, 3 + uShadowQuality));
	for (int i = 0; i < sampleCount; ++i) {
		// Hopefully, your glsl compiler will swap switch and for-statements :/
		vec2 offset;
		switch (uShadowQuality) {
		case 0: offset = POISSON8[i];
		case 1: offset = POISSON16[i];
		case 2: offset = POISSON32[i];
		default: offset = POISSON64[i];
		}
		// Since we only render back-facing triangles into the shadow
		// buffer, we do not need to bias the depth here. The current
		// technique has its own problems, such as "Peter-Panning" on
		// very thin or intersecting geometry, but it works perfectly
		// for this small demo scene.
		sum += texture(uShadowSampler, uvz + vec3(offset * filterRadius, 0));
	}
	return sum / sampleCount;
}

// Perspective projection stores depth values as 1/z. This function linearizes
// the depth value returned by texture(...) and transforms it to the interval
// [near_plane, far_plane].
// See: https://stackoverflow.com/questions/51108596/linearize-depth
float depthToZ(float depth) {
	float n = uLightNearPlane;
	float f = uLightFarPlane;
	return n * f / (depth * (n - f) + f);
}

vec2 findAverageOccluder(vec3 uvz) {
	// Tip: If your shadows don't look right or sharp enough, try increasing
	// the near plane of the light camera. It makes a huge difference.
	float zReceiver = vShadowCoordinates.w;
	float searchWidth = uLightWidthUV * (zReceiver - uLightNearPlane) / zReceiver;

	float sum = 0.0;
	float count = 0.0;

	for (int i = 0; i < 16; ++i) {
		vec2 s = uvz.xy + POISSON16[i] * searchWidth;
		float depth = texture(uDepthBuffer, s).x;
		if (depth < uvz.z) {
			sum += depth;
			++count;
		}
	}

	// I have a feeling that this is actually slightly wrong, since the
	// average of the inverse values is not equal to the inverse of the
	// average value. But Nvidia has implemented it that way, and after
	// a few tests the two approaches give very similar results anyway.
	return vec2(depthToZ(sum / count), count);
}

float PCSS(vec3 uvz) {
	vec2 occluderInfo = findAverageOccluder(uvz);
	if (occluderInfo.y == 0.0) {
		// No occluders were found, fragment is fully lit.
		return 1.0;
	}

	float occluder = occluderInfo.x;
	float receiver = vShadowCoordinates.w;
	float penumbraWidth = uLightWidthUV * (receiver - occluder) / occluder;
	float filterRadius = uLightNearPlane * penumbraWidth / receiver;

	return PCF(uvz, filterRadius);
}

float calculateShadow() {
	vec3 uvz = (vShadowCoordinates.xyz / vShadowCoordinates.w) * 0.5 + 0.5;
	if (uvz.z < 0.0 || 1.0 < uvz.z) {
		// Discard this point, since it lies outside the light frustum.
		// uvz.x and uvz.y dimensions are handled with GL_TEXTURE_BORDER_COLOR
		// and GL_CLAMP_TO_BORDER.
		return 1.0;
	}
	if (uEnablePCSS) {
		return PCSS(uvz);
	} else {
		return PCF(uvz, uFilterRadius);
	}
}

void main() {
	vec3 normal = normalize(vNormal);
	vec3 toLight = normalize(uLightPosition - vWorldPosition);

	float light = max(0, dot(normal, toLight)) * calculateShadow() + 0.15;
	vec3 color = vColor * light;

	// Write color with gamma correction.
	gl_FragColor = vec4(pow(color, vec3(0.4545)), 1.0);

	if (debug != 0.0) {
		gl_FragColor = vec4(1, 0, 0, 1);
	}
}
