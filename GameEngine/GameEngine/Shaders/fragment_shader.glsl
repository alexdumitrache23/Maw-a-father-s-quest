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
    //TO DO: Add illumination from Lab 9
    if (useTexture) {
        fragColor = texture(texture1, textureCoord);
    } else {
        fragColor = vec4(overrideColor, 1.0);
    }
}