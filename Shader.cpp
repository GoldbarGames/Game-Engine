#include "Shader.h"
#include "Renderer.h"

ShaderProgram::ShaderProgram(const char* n, const char* vertexFilePath, const char* fragmentFilePath)
{
    name = n;
    programID = 0;
    uniformVariables["model"] = 0;
    uniformVariables["projection"] = 0;

    CreateFromFiles(vertexFilePath, fragmentFilePath);
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

    uniformVariables["model"] = glGetUniformLocation(programID, "model");
    uniformVariables["projection"] = glGetUniformLocation(programID, "projection");
    uniformVariables["view"] = glGetUniformLocation(programID, "view");
    uniformVariables["texFrame"] = glGetUniformLocation(programID, "texFrame");
    uniformVariables["texOffset"] = glGetUniformLocation(programID, "texOffset");

    //TODO: What is a good way for us to define variables for specific shaders?
    uniformVariables["fadeColor"] = glGetUniformLocation(programID, "fadeColor");
    uniformVariables["currentTime"] = glGetUniformLocation(programID, "currentTime");
}

GLuint ShaderProgram::GetUniformVariable(const std::string& variable)
{
    return uniformVariables[variable];
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

    uniformVariables["model"] = 0;
    uniformVariables["projection"] = 0;
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