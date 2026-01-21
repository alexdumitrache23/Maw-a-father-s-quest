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
    // Slower, multi-directional texture scrolling
    vec2 flow1 = TexCoords + vec2(time * 0.03, time * 0.01);
    vec2 flow2 = TexCoords + vec2(-time * 0.01, time * 0.04);

    // Sample texture twice and blend to hide the static pattern
    vec4 texColor1 = texture(waterTexture, flow1);
    vec4 texColor2 = texture(waterTexture, flow2);
    vec3 objectColor = mix(texColor1, texColor2, 0.5).rgb;
    
    // Tint it slightly brownish-green for "Sewer" look
    objectColor = objectColor * vec3(0.8, 0.9, 0.7);

    // Ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (Blinn-Phong)
    // Increased shininess to 128 (was 32) -> makes it look wet/glossy
    float specularStrength = 0.8; 
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 128.0);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine
    vec3 result = (ambient + diffuse + specular) * objectColor; 
    
    // Alpha 0.85 for slightly murky water
    FragColor = vec4(result, 0.85); 
}