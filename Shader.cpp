#include "Shader.h"

ShaderProgram::ShaderProgram()
{
    programID = 0;
    uniformModel = 0;
    uniformProjection = 0;
}

ShaderProgram::~ShaderProgram()
{
    ClearShader();
}

void ShaderProgram::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
    CompileShader(vertexCode, fragmentCode);
}

void ShaderProgram::CompileShader(const char* vertexCode, const char* fragmentCode)
{
    programID = glCreateProgram();

    if (!programID)
    {
        printf("Error creating shader program!\n");
        return;
    }

    AddShader(programID, vertexCode, GL_VERTEX_SHADER);
    AddShader(programID, fragmentCode, GL_FRAGMENT_SHADER);

    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &result);

    if (!result)
    {
        glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
        printf("Error linking program: '%s'%\n", eLog);
        return;
    }

    glValidateProgram(programID);
    glGetProgramiv(programID, GL_VALIDATE_STATUS, &result);

    if (!result)
    {
        glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
        printf("Error validating program: '%s'%\n", eLog);
        return;
    }

    uniformModel = glGetUniformLocation(programID, "model");
    uniformProjection = glGetUniformLocation(programID, "projection");
    uniformView = glGetUniformLocation(programID, "view");
    uniformViewTexture = glGetUniformLocation(programID, "texFrame");
    uniformOffsetTexture = glGetUniformLocation(programID, "texOffset");
}

GLuint ShaderProgram::GetProjectionLocation()
{
    return uniformProjection;
}

GLuint ShaderProgram::GetModelLocation()
{
    return uniformModel;
}

GLuint ShaderProgram::GetViewLocation()
{
    return uniformView;
}

GLuint ShaderProgram::GetViewTextureLocation()
{
    return uniformViewTexture;
}

GLuint ShaderProgram::GetOffsetTextureLocation()
{
    return uniformOffsetTexture;
}

void ShaderProgram::CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath)
{
    std::string vertexString = ReadFile(vertexFilePath);
    std::string fragmentString = ReadFile(fragmentFilePath);

    const char* vertexCode = vertexString.c_str();
    const char* fragmentCode = fragmentString.c_str();

    CompileShader(vertexCode, fragmentCode);
}

std::string ShaderProgram::ReadFile(const char* filePath)
{
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if (!fileStream.is_open())
    {
        printf("Failed to read in %s! File doesn't exist.", filePath);
        return "";
    }

    std::string line = "";
    while (!fileStream.eof())
    {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();

    return content;
}

void ShaderProgram::UseShader()
{
    glUseProgram(programID);
}

void ShaderProgram::ClearShader()
{
    if (programID != 0)
    {
        glDeleteProgram(programID);
        programID = 0;
    }

    uniformModel = 0;
    uniformProjection = 0;
}

void ShaderProgram::AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
    GLuint theShader = glCreateShader(shaderType);

    const GLchar* theCode[1];
    theCode[0] = shaderCode;

    GLint codeLength[1];
    codeLength[0] = strlen(shaderCode);

    glShaderSource(theShader, 1, theCode, codeLength);
    glCompileShader(theShader);

    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);

    if (!result)
    {
        glGetShaderInfoLog(theShader, sizeof(eLog), NULL, eLog);
        printf("Error compiling the  %d shader: '%s'%\n", shaderType, eLog);
        return;
    }

    glAttachShader(theProgram, theShader);

    return;
}