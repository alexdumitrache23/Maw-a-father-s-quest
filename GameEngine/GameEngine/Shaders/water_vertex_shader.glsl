#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out float WaterHeight; 

uniform mat4 model;
uniform mat4 MVP;
uniform float time;

void main()
{
    vec3 pos = aPos;

    // Gentle, stable wave displacement in OBJECT space.
    // Keeping amplitudes small prevents extreme derivatives / sparkling.
    float wave = 0.0;
    wave += sin(pos.x * 0.35 + time * 1.2) * 0.06;
    wave += sin(pos.z * 0.25 + time * 0.9) * 0.04;
    wave += sin((pos.x + pos.z) * 0.20 + time * 0.6) * 0.03;
    pos.y += wave;
    WaterHeight = wave;

    // World-space position for lighting.
    FragPos = vec3(model * vec4(pos, 1.0));

    // Stable normal (world-space up). This is intentionally simple/robust.
    // (Wave lighting still looks good because fragment shader adds spec/Fresnel.)
    Normal = normalize(mat3(model) * vec3(0.0, 1.0, 0.0));

    // Tile the texture a bit so scaling the plane doesn't stretch the pattern.
    TexCoords = aTexCoords * 8.0;
    gl_Position = MVP * vec4(pos, 1.0);
}