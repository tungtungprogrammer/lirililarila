#pragma once
// GLSL for the 2007-style look. We draw straight into the window, so colours are written as plain sRGB.

namespace shaders {

static const char* kSceneVertex = R"GLSL(#version 330
uniform vec4 vp0;
uniform vec4 vp1;
uniform vec4 vp2;
uniform vec4 vp3;
in vec3 aPos;
in vec3 aNormal;
in vec3 aColor;
in vec3 aAux;
out vec3 vNormal;
out vec3 vColor;
out vec3 vAux;
void main() {
    vNormal = aNormal;
    vColor = aColor;
    vAux = aAux;
    gl_Position = vp0 * aPos.x + vp1 * aPos.y + vp2 * aPos.z + vp3;
}
)GLSL";

static const char* kSceneFragment = R"GLSL(#version 330
uniform float uFlat;
in vec3 vNormal;
in vec3 vColor;
in vec3 vAux;
out vec4 result;
void main() {
    vec3 base = vColor;
    vec3 col;
    if (uFlat > 0.5) {
        col = base;
    } else {
        vec3 n = normalize(vNormal);
        // Flat 2007 lighting: a high sun, a soft fill from the opposite side, and a bright ambient floor.
        vec3 sun  = normalize(vec3(-0.35, 0.85, 0.40));
        vec3 fill = normalize(vec3(0.60, 0.15, -0.50));
        float d = max(dot(n, sun), 0.0);
        float f = max(dot(n, fill), 0.0);
        float down = max(-n.y, 0.0);                 // undersides are the darkest
        float light = 0.46 + 0.62 * d + 0.18 * f - 0.18 * down;

        // Huge studded parts (the baseplate) get their studs painted on.
        if (vAux.z > 0.5 && n.y > 0.9) {
            vec2 cell = fract(vAux.xy) - 0.5;
            float r = length(cell);
            if (r < 0.30) {
                light *= 1.0 + 0.16 * (1.0 - r / 0.30);
                if (r > 0.255) light *= 0.80;        // dark rim on the stud
            } else if (r < 0.36) {
                light *= 0.86;                       // contact shadow around the stud
            }
        }
        col = base * light;
    }
    col = clamp(col, 0.0, 1.0);
    result = vec4(col, 1.0);
}
)GLSL";

static const char* kSkyVertex = R"GLSL(#version 330
in vec4 aPos;
out vec2 vNdc;
void main() {
    vNdc = aPos.xy;
    gl_Position = vec4(aPos.xy, 0.999, 1.0);
}
)GLSL";

static const char* kSkyFragment = R"GLSL(#version 330
uniform vec3 uFwd;
uniform vec3 uRight;
uniform vec3 uUp;
uniform vec2 uTan;
in vec2 vNdc;
out vec4 result;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) { s += a * noise(p); p *= 2.03; a *= 0.5; }
    return s;
}

void main() {
    vec3 dir = normalize(uFwd + uRight * (vNdc.x * uTan.x) + uUp * (vNdc.y * uTan.y));
    float h = clamp(dir.y, -1.0, 1.0);

    vec3 zenith  = vec3(0.20, 0.45, 0.85);
    vec3 horizon = vec3(0.72, 0.86, 0.98);
    vec3 sky = mix(horizon, zenith, pow(clamp(h, 0.0, 1.0), 0.55));
    if (h < 0.0) sky = mix(horizon, vec3(0.62, 0.74, 0.86), clamp(-h * 4.0, 0.0, 1.0));

    // sun
    vec3 sunDir = normalize(vec3(-0.35, 0.85, 0.40));
    float s = max(dot(dir, sunDir), 0.0);
    sky += vec3(1.0, 0.95, 0.80) * (pow(s, 600.0) * 1.2 + pow(s, 12.0) * 0.10);

    // clouds on a flat layer
    if (h > 0.01) {
        vec2 uv = dir.xz / (h + 0.18) * 0.9;
        float c = fbm(uv * 1.4 + vec2(3.0, 7.0));
        c = smoothstep(0.50, 0.78, c);
        float fade = smoothstep(0.01, 0.20, h);
        sky = mix(sky, vec3(1.0), c * 0.92 * fade);
    }
    result = vec4(clamp(sky, 0.0, 1.0), 1.0);
}
)GLSL";

}  // namespace shaders
