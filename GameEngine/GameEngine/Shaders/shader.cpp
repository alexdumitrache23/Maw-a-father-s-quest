#include "shader.h"
#include <iostream>
#include <vector>

using namespace std;

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vertexShaderFile;
	std::ifstream fragmentShaderFile;

	try
	{
        vertexShaderFile.open(vertexPath);
        fragmentShaderFile.open(fragmentPath);
        if (vertexShaderFile.is_open() && fragmentShaderFile.is_open()) {
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vertexShaderFile.rdbuf();
            fShaderStream << fragmentShaderFile.rdbuf();
            vertexShaderFile.close();
            fragmentShaderFile.close();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } else {
            std::cout << "Warning: shader files not found: " << vertexPath << " or " << fragmentPath << ". Using fallback shaders." << std::endl;
        }
	}
	catch (std::ifstream::failure e)
	{
        std::cout << "Error reading shader files, using fallback shaders." << std::endl;
	}
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

    // If files were empty or not loaded, provide minimal fallback shaders
    std::string fallbackVertex = "#version 330 core\nlayout(location = 0) in vec3 pos; uniform mat4 MVP; void main(){ gl_Position = MVP * vec4(pos,1.0); }";
    std::string fallbackFragment = "#version 330 core\nout vec4 fragColor; void main(){ fragColor = vec4(1.0,0.0,1.0,1.0); }";
    if (vertexCode.empty()) vShaderCode = fallbackVertex.c_str();
    if (fragmentCode.empty()) fShaderCode = fallbackFragment.c_str();

	//compile shaders
	unsigned int vertex, fragment;
	int success;

	// vertex Shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);

	// compile errors 
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		std::cout << "Error compiling vertex shader! " << std::endl;
	}

	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);

	// compile errors
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		std::cout << "Error compiling fragment shader! " << std::endl;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Check Vertex Shader
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(vertex, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Check Fragment Shader
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(fragment, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// shader Program
	id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
	glLinkProgram(id);

	// linking errors
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (!success)
	{
		std::cout << "Error linking shader!" << std::endl;
	}
 
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::use()
{
	glUseProgram(id);
}

int Shader::getId()
{
	return id;
}

Shader::~Shader()
{
}
