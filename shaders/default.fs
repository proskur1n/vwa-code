#version 330 core

uniform vec3 u_color;
uniform struct {
    vec3 ambient;
    vec3 diffuse;

    sampler2D depth_buffer;

    vec3 position;
    float width;

    float near_plane;
    float far_plane;
    float frustum_width;

    // 0 -> Hard Shadows
    // 1 -> PCF
    // 2 -> PCSS
    int algorithm;
    int sample_count;

    // 0 -> Regular grid
    // 1 -> Poisson Disk
    // 2 -> Rotated Poisson Disk
    int filter_pattern;
    float filter_radius;
} u_light;

in vec3 v_world_pos;
in vec3 v_normal;
in vec4 v_shadow_coords;

out vec4 out_color;

float hard_shadow(vec3 uvz, vec2 offset) {
    // TODO bias
    // float bias = -0.01 / v_shadow_coords.w;
    // float bias = -0.005;
    float bias = 0.0;
    return float(texture(u_light.depth_buffer, uvz.xy + offset).r > uvz.z - bias);
}

// Defined at the bottom of the file
vec2 get_sample(int index);

float PCF(vec3 uvz, float radius) {
    float sum = 0.0;
    for (int i = 0; i < u_light.sample_count; ++i) {
        sum += hard_shadow(uvz, get_sample(i) * radius);
    }
    return sum / float(u_light.sample_count);
}

// Perspective projection stores depth values as 1/z. This function linearizes
// the depth value returned by texture(...) and transforms it to the interval
// [near_plane, far_plane].
float depth_to_z(float depth) {
    float n = u_light.near_plane;
    float f = u_light.far_plane;
    return n * f / (depth * (n - f) + f);
}

float find_average_occluder(out int counter, vec3 uvz) {
    float search_radius = (1.0 - u_light.near_plane / v_shadow_coords.w) * 0.5;
    search_radius *= u_light.width / u_light.frustum_width;

    counter = 0;
    float sum = 0.0f;
    
    for (int i = 0; i < u_light.sample_count; ++i) {
        vec2 s = uvz.xy + get_sample(i) * search_radius;
        float depth = texture(u_light.depth_buffer, s).r;
        if (depth < uvz.z) {
            ++counter;
            sum += depth_to_z(depth);
        }
    }

    return sum / float(counter);
}

float PCSS(vec3 uvz) {
    int occluder_count;
    float d_occluder = find_average_occluder(occluder_count, uvz);
    float d_receiver = v_shadow_coords.w;

    if (occluder_count == 0) {
        // No occluders were found, fragment is fully lit
        return 1.0;
    }

    float penumbra_width = u_light.width * (d_receiver / d_occluder - 1.0);
    float filter_radius = penumbra_width * u_light.near_plane / d_receiver;
    filter_radius /= u_light.frustum_width * 2.0;

    return PCF(uvz, filter_radius);
}

float shadow() {
    vec3 uvz = (v_shadow_coords.xyz / abs(v_shadow_coords.w)) * 0.5 + 0.5;
    if (uvz.z < 0.0 || 1.0 < uvz.z) {
        // Discard the point, since it lies outside the light frustum.
        // uvz.x and uvz.y dimensions are handled with GL_TEXTURE_BORDER_COLOR
        // and GL_CLAMP_TO_BORDER.
        return 1.0;
    }

    switch (u_light.algorithm) {
    case 0:
        return hard_shadow(uvz, vec2(0.0, 0.0));
    case 1:
        return PCF(uvz, u_light.filter_radius);
    default:
        return PCSS(uvz);
    }
}

void main() {
    // TODO cleanup

    // vec3 to_light = u_light.position - v_world_pos;
    vec3 to_light = u_light.position;

    float diffuse = dot(normalize(v_normal), normalize(to_light));
    diffuse = max(0, diffuse) / dot(to_light, to_light) * 200.0; // TODO amplifier

    float diffuse2 = max(0, dot(normalize(vec3(-3.2, 4.4, -1.5)), normalize(v_normal)));

    // TODO gamma correction
    vec3 ambient_light = pow(u_light.ambient, vec3(2.2));
    vec3 diffuse_light = pow(u_light.diffuse, vec3(2.2));
    vec3 obj_color = pow(u_color, vec3(2.2));

    float d = diffuse * 0.7 * shadow() + max(0.6, shadow()) * diffuse2 * 0.15;

    vec3 color = obj_color * (d * diffuse_light + ambient_light);
    out_color = vec4(pow(color, vec3(1.0/2.2)), 1.0);
}

const int MAX_SAMPLES = 64;

const vec2 REGULAR_SAMPLES[MAX_SAMPLES] = vec2[](
    vec2(0.0, 0.0),vec2(-0.303046,-0.505076),vec2(0.707107,0.303046),
    vec2(0.101015,0.505076),vec2(-0.505076,-0.303046),vec2(0.101015,-0.303046),
    vec2(-0.101015,-0.707107),vec2(-0.303046,0.505076),vec2(0.303046,-0.101015),
    vec2(0.303046,0.505076),vec2(0.101015,-0.505076),vec2(-0.505076,0.707107),
    vec2(0.505076,0.303046),vec2(-0.707107,0.707107),vec2(0.707107,-0.101015),
    vec2(0.101015,0.303046),vec2(0.303046,0.101015),vec2(0.101015,0.707107),
    vec2(-0.101015,-0.505076),vec2(-0.101015,0.505076),vec2(-0.505076,-0.707107),
    vec2(0.505076,-0.505076),vec2(0.707107,0.505076),vec2(-0.505076,-0.101015),
    vec2(-0.707107,-0.707107),vec2(-0.707107,0.303046),vec2(-0.505076,0.505076),
    vec2(-0.707107,-0.303046),vec2(0.303046,0.303046),vec2(0.101015,0.101015),
    vec2(0.303046,0.707107),vec2(0.505076,0.505076),vec2(-0.101015,0.303046),
    vec2(-0.303046,0.303046),vec2(-0.707107,-0.505076),vec2(-0.505076,0.101015),
    vec2(0.303046,-0.505076),vec2(-0.101015,-0.303046),vec2(0.707107,0.101015),
    vec2(0.707107,0.707107),vec2(0.707107,-0.303046),vec2(-0.303046,-0.707107),
    vec2(-0.303046,-0.303046),vec2(-0.707107,0.505076),vec2(0.303046,-0.707107),
    vec2(0.505076,0.101015),vec2(0.505076,-0.101015),vec2(-0.303046,0.101015),
    vec2(0.101015,-0.101015),vec2(0.707107,-0.707107),vec2(0.707107,-0.505076),
    vec2(-0.707107,-0.101015),vec2(-0.303046,-0.101015),vec2(-0.303046,0.707107),
    vec2(-0.707107,0.101015),vec2(0.101015,-0.707107),vec2(-0.101015,0.101015),
    vec2(0.505076,-0.303046),vec2(-0.505076,0.303046),vec2(0.303046,-0.303046),
    vec2(0.505076,0.707107),vec2(-0.505076,-0.505076),vec2(-0.101015,0.707107),
    vec2(0.505076,-0.707107)
);

const vec2 POISSON_SAMPLES[MAX_SAMPLES] = vec2[](
    vec2(-0.351859,0.106769),vec2(-0.178395,-0.049275),vec2(-0.512065,0.290097),
    vec2(-0.158281,0.146668),vec2(-0.534996,-0.079190),vec2(-0.282050,0.398939),
    vec2(-0.575266,0.119424),vec2(-0.308026,-0.242392),vec2(-0.601290,-0.367771),
    vec2(-0.842546,0.053855),vec2(-0.760456,0.379012),vec2(-0.212398,-0.402350),
    vec2(-0.034624,-0.343944),vec2(-0.418209,-0.386024),vec2(-0.331176,-0.562278),
    vec2(0.009401,-0.130803),vec2(-0.021162,0.307088),vec2(0.129079,0.164361),
    vec2(-0.767727,-0.114591),vec2(0.164757,0.412959),vec2(0.031751,0.583892),
    vec2(-0.142373,0.535536),vec2(0.347247,0.147926),vec2(0.190450,-0.023296),
    vec2(-0.343150,-0.817173),vec2(-0.074857,-0.561971),vec2(-0.066542,-0.789211),
    vec2(-0.660645,-0.602821),vec2(0.143080,-0.313792),vec2(0.162385,-0.623499),
    vec2(0.459006,-0.302363),vec2(0.332014,-0.536220),vec2(0.385269,-0.124572),
    vec2(0.362823,0.551628),vec2(0.416019,0.322252),vec2(0.187659,0.764267),
    vec2(-0.854011,-0.296882),vec2(-0.971921,-0.088353),vec2(-0.555691,-0.760734),
    vec2(-0.129067,-0.983567),vec2(-0.189672,0.820918),vec2(-0.336024,0.682886),
    vec2(-0.430858,0.514630),vec2(-0.761832,0.587257),vec2(-0.910169,0.250733),
    vec2(0.623072,-0.222174),vec2(0.471969,-0.654405),vec2(0.523986,-0.476633),
    vec2(0.392550,0.782360),vec2(0.628093,0.451577),vec2(0.566884,0.647194),
    vec2(0.732889,-0.409076),vec2(0.729535,-0.622573),vec2(0.125663,-0.817391),
    vec2(0.355508,-0.854747),vec2(0.605970,0.063792),vec2(-0.797107,-0.478937),
    vec2(-0.593447,0.698320),vec2(0.593583,0.251305),vec2(-0.006533,0.809872),
    vec2(-0.442785,0.846463),vec2(0.896469,0.357716),vec2(0.766316,0.641855),
    vec2(0.903308,-0.327009)
);

float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 get_sample(int index) {
    switch(u_light.filter_pattern) {
    case 0:
        return REGULAR_SAMPLES[index];
    case 1:
        return POISSON_SAMPLES[index];
    default:
        float angle = rand(v_world_pos.xy);
        float c = cos(angle);
        float s = sin(angle);
        vec2 p = POISSON_SAMPLES[index];

        return vec2(
            c * p.x + s * p.y,
            s * p.x - c * p.y
        );
    }
}