#include "Shader.h"
#include "Renderer.h"

ShaderProgram::ShaderProgram(const ShaderName n, const char* vertexFilePath, const char* fragmentFilePath)
{
    name = n;
    programID = 0;
    uniformVariables[ShaderVariable::model] = 0;
    uniformVariables[ShaderVariable::projection] = 0;

    CreateFromFiles(vertexFilePath, fragmentFilePath);
}

ShaderProgram::~ShaderProgram()
{
    ClearShader();
}

const std::string& ShaderProgram::GetNameString()
{
    if (nameString == "")
        return Globals::NONE_STRING;

    return nameString;
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
        printf("Error linking program: '%s'\n", eLog);
        return;
    }

    glValidateProgram(programID);
    glGetProgramiv(programID, GL_VALIDATE_STATUS, &result);

    if (!result)
    {
        glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
        printf("Error validating program: '%s'\n", eLog);
        return;
    }

    uniformVariables[ShaderVariable::model] = glGetUniformLocation(programID, "model");
    uniformVariables[ShaderVariable::projection] = glGetUniformLocation(programID, "projection");
    uniformVariables[ShaderVariable::view] = glGetUniformLocation(programID, "view");
    uniformVariables[ShaderVariable::texFrame] = glGetUniformLocation(programID, "texFrame");
    uniformVariables[ShaderVariable::texOffset] = glGetUniformLocation(programID, "texOffset");

    //TODO: What is a good way for us to define variables for specific shaders?
    uniformVariables[ShaderVariable::fadeColor] = glGetUniformLocation(programID, "spriteColor");
    uniformVariables[ShaderVariable::currentTime] = glGetUniformLocation(programID, "time");
    uniformVariables[ShaderVariable::frequency] = glGetUniformLocation(programID, "freq");

    uniformVariables[ShaderVariable::ambientColor] = glGetUniformLocation(programID, "directionalLight.color");
    uniformVariables[ShaderVariable::ambientIntensity] = glGetUniformLocation(programID, "directionalLight.ambientIntensity");
    uniformVariables[ShaderVariable::diffuseIntensity] = glGetUniformLocation(programID, "directionalLight.diffuseIntensity");
    uniformVariables[ShaderVariable::lightDirection] = glGetUniformLocation(programID, "directionalLight.direction");

    uniformVariables[ShaderVariable::specularIntensity] = glGetUniformLocation(programID, "material.specularIntensity");
    uniformVariables[ShaderVariable::specularShine] = glGetUniformLocation(programID, "material.shine");
    uniformVariables[ShaderVariable::eyePosition] = glGetUniformLocation(programID, "eyePosition");
}

GLuint ShaderProgram::GetUniformVariable(ShaderVariable variable)
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
    /*
    static unsigned int lastProgramID = -1;

    if (programID != lastProgramID)
    {
        
        lastProgramID = programID;
    }   */
}

void ShaderProgram::ClearShader()
{
    if (programID != 0)
    {
        glDeleteProgram(programID);
        programID = 0;
    }

    uniformVariables[ShaderVariable::model] = 0;
    uniformVariables[ShaderVariable::projection] = 0;
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
        std::cout << "Error compiling the " << GetNameString() << " shader: " << eLog << std::endl;
        return;
    }

    glAttachShader(theProgram, theShader);

    return;
}