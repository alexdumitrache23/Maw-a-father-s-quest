#version 400

in vec2 textureCoord;
in vec3 fragPos;
in vec3 norm;

out vec4 fragColor;

uniform sampler2D texture1;
uniform float time;

void main()
{
    float wave = sin(time + textureCoord.x * 10.0) * 0.05;
    vec2 uv = textureCoord + vec2(0.0, wave);
    vec4 col = texture(texture1, uv);
    fragColor = col;
}
