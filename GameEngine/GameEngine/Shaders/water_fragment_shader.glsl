#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D waterTexture; 
uniform float time; 

void main()
{
    // Slow, multi-direction flow to hide repeating patterns
    vec2 flow1 = TexCoords + vec2(time * 0.02, time * 0.01);
    vec2 flow2 = TexCoords + vec2(-time * 0.01, time * 0.02);

    vec3 texA = texture(waterTexture, flow1).rgb;
    vec3 texB = texture(waterTexture, flow2).rgb;
    vec3 baseColor = mix(texA, texB, 0.5);

    // Sewer tint
    baseColor *= vec3(0.75, 0.85, 0.70);

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);

    // Smooth lighting (keep diffuse modest; it's murky water)
    float diff = max(dot(N, L), 0.0);
    vec3 ambient = 0.35 * lightColor;
    vec3 diffuse = diff * 0.40 * lightColor;

    // Soft specular (lower strength + lower shininess to avoid sparkling)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 48.0);
    vec3 specular = spec * 0.35 * lightColor;

    // Fresnel for a subtle edge highlight
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 fresnelColor = mix(vec3(0.0), vec3(0.25, 0.35, 0.40), fresnel);

    vec3 result = (ambient + diffuse) * baseColor + specular + fresnelColor;

    // Slightly transparent so it reads as water
    FragColor = vec4(result, 0.80);
}