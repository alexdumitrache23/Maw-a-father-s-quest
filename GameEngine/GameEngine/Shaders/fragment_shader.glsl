#version 400

in vec2 textureCoord; 
in vec3 norm;
in vec3 fragPos;

out vec4 fragColor;

uniform sampler2D texture1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool useTexture;
uniform vec3 overrideColor;

void main()
{
    // 1. Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 2. Diffuse 
    vec3 normDir = normalize(norm);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normDir, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 3. Specular (Blinn-Phong)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normDir, halfwayDir), 0.0), 32.0); // 32 is shininess
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular);
    
    vec4 texColor;
    if (useTexture) {
        texColor = texture(texture1, textureCoord);
    } else {
        texColor = vec4(overrideColor, 1.0);
    }

    fragColor = vec4(result, 1.0) * texColor;
}