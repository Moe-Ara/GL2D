#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSceneTex;
uniform sampler2D uNormalTex;
uniform sampler2D uCookieTex[8];
uniform vec3 uAmbient;
uniform int uLightCount;
uniform vec4 uViewBounds; // minX, minY, maxX, maxY

struct Light {
    int type;          // 0=point,1=directional,2=spot
    vec2 pos;
    vec2 dir;
    float radius;
    vec3 color;
    float intensity;
    float falloff;
    float innerCutoff;
    float outerCutoff;
    float emissiveBoost;
    int cookieSlot;
    float cookieStrength;
};
uniform Light uLights[8];

void main() {
    vec4 sceneSample = texture(uSceneTex, vUV);
    vec3 albedo = sceneSample.rgb;
    float alpha = sceneSample.a;
    vec3 normalSample = texture(uNormalTex, vUV).xyz;
    vec3 N = normalize(normalSample * 2.0 - 1.0);
    if (length(N) < 0.001) {
        N = vec3(0.0, 0.0, 1.0);
    }

    // Reconstruct world position from UV and view bounds (orthographic).
    vec2 worldPos;
    worldPos.x = mix(uViewBounds.x, uViewBounds.z, vUV.x);
    worldPos.y = mix(uViewBounds.y, uViewBounds.w, vUV.y);

    vec3 lit = albedo * uAmbient;

    for (int i = 0; i < uLightCount; ++i) {
        Light l = uLights[i];
        float att = 1.0;

        if (l.type == 0) { // Point
            vec2 toL = l.pos - worldPos;
            float dist = length(toL);
            if (dist >= l.radius) {
                continue;
            }
            att = clamp(1.0 - pow(dist / l.radius, l.falloff), 0.0, 1.0);
            vec3 L = normalize(vec3(toL, 0.5));
            float ndotl = max(dot(N, L), 0.0);
            // For flat normals, keep some shading but avoid blowing out.
            if (N.z > 0.9) ndotl = 0.6;
            att *= ndotl;
        } else if (l.type == 2) { // Spot
            vec2 toFrag = worldPos - l.pos;
            float dist = length(toFrag);
            if (dist >= l.radius) {
                continue;
            }
            vec2 dirNorm = normalize(l.dir);
            vec2 fragDir = dist > 0.0001 ? toFrag / dist : vec2(0.0);
            float cosTheta = dot(dirNorm, fragDir);
            float angleAtt = smoothstep(l.outerCutoff, l.innerCutoff, cosTheta);
            float radial = clamp(1.0 - pow(dist / l.radius, l.falloff), 0.0, 1.0);
            att = angleAtt * radial;
            if (att <= 0.0) {
                continue;
            }
            vec3 L = normalize(vec3(-toFrag, 0.5));
            float ndotl = max(dot(N, L), 0.0);
            if (N.z > 0.9) ndotl = 0.6;
            att *= ndotl;
            if (l.cookieSlot >= 0 && l.cookieStrength > 0.0) {
                vec2 perp = vec2(-dirNorm.y, dirNorm.x);
                vec2 local = vec2(dot(perp, fragDir), dot(dirNorm, fragDir));
                vec2 uv = local * 0.5 + 0.5;
                float cookie = texture(uCookieTex[l.cookieSlot], uv).r;
                att *= mix(1.0, cookie, clamp(l.cookieStrength, 0.0, 1.0));
            }
        } else {
            // Directional: no attenuation (camera-wide).
            vec3 L = normalize(vec3(-l.dir, 0.5));
            float ndotl = max(dot(N, L), 0.0);
            if (N.z > 0.9) ndotl = 0.6;
            att = ndotl;
        }

        vec3 contrib = albedo * l.color * (l.intensity * att);
        contrib += albedo * l.emissiveBoost * att;
        lit += contrib;
    }

    FragColor = vec4(lit, alpha);
}
