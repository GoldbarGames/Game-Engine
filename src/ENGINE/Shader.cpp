#include "Shader.h"
#include "Renderer.h"
#include "render/RenderDevice.h"
#include "EnginePaths.h"
#include "UniformBlocks.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <cstring>

unsigned int ShaderProgram::lastProgramID = -1;

ShaderProgram::ShaderProgram(const int n, const char* vertexFilePath, const char* fragmentFilePath, bool fromString)
{
    name = n;
    programID = 0;
    uniformVariables[ShaderVariable::model] = 0;
    uniformVariables[ShaderVariable::projection] = 0;

    pointLightCount = 0;

    if (fromString)
        CreateFromString(vertexFilePath, fragmentFilePath);
    else
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
    std::string log;
    programID = Device().CreateProgram(vertexCode, fragmentCode, log).id;
    if (!log.empty())
        std::cout << "Shader " << GetNameString() << ": " << log;
    if (programID == 0)
        return;   // compile/link failed (reported above)

    // Point every engine uniform block this program declares at its shared
    // binding (see UniformBlocks.h); blocks it doesn't declare are skipped.
    for (const UniformBlock::Entry& block : UniformBlock::kAll)
        Device().SetUniformBlockBinding(ProgramHandle(programID), block.name, block.binding);

    // Per-draw values: "draw.<name>" in engine shaders, plain names in older
    // game shaders (DrawUniformLocation). view/projection are in the Camera
    // block for engine shaders, so these two resolve to -1 there (no-op sets).
    uniformVariables[ShaderVariable::model] = DrawUniformLocation(programID, "model");
    uniformVariables[ShaderVariable::projection] = DrawUniformLocation(programID, "projection");
    uniformVariables[ShaderVariable::view] = DrawUniformLocation(programID, "view");

    // The size of the frame (width and height) in the texture
    uniformVariables[ShaderVariable::texFrame] = DrawUniformLocation(programID, "texFrame");

    // The offset of the frame within the texture
    uniformVariables[ShaderVariable::texOffset] = DrawUniformLocation(programID, "texOffset");

    //TODO: What is a good way for us to define variables for specific shaders?
    uniformVariables[ShaderVariable::fadeColor] = DrawUniformLocation(programID, "spriteColor");
    uniformVariables[ShaderVariable::currentTime] = DrawUniformLocation(programID, "time");
    uniformVariables[ShaderVariable::frequency] = DrawUniformLocation(programID, "freq");

    //uniformVariables[ShaderVariable::textureWidth] = Device().UniformLocation(ProgramHandle(programID), "textureWidth");
    //uniformVariables[ShaderVariable::textureHeight] = Device().UniformLocation(ProgramHandle(programID), "textureHeight");

    /*
    uniformVariables[ShaderVariable::ambientColor] = Device().UniformLocation(ProgramHandle(programID), "directionalLight.color");
    uniformVariables[ShaderVariable::ambientIntensity] = Device().UniformLocation(ProgramHandle(programID), "directionalLight.ambientIntensity");
    uniformVariables[ShaderVariable::diffuseIntensity] = Device().UniformLocation(ProgramHandle(programID), "directionalLight.diffuseIntensity");
    uniformVariables[ShaderVariable::lightDirection] = Device().UniformLocation(ProgramHandle(programID), "directionalLight.direction");
    */

    uniformDirectionalLight.uniformColor = Device().UniformLocation(ProgramHandle(programID), "directionalLight.base.color");
    uniformDirectionalLight.uniformAmbientIntensity = Device().UniformLocation(ProgramHandle(programID), "directionalLight.base.ambientIntensity");
    uniformDirectionalLight.uniformDiffuseIntensity = Device().UniformLocation(ProgramHandle(programID), "directionalLight.base.diffuseIntensity");
    uniformDirectionalLight.uniformDirection = Device().UniformLocation(ProgramHandle(programID), "directionalLight.direction");

    uniformVariables[ShaderVariable::specularIntensity] = Device().UniformLocation(ProgramHandle(programID), "material.specularIntensity");
    uniformVariables[ShaderVariable::specularShine] = Device().UniformLocation(ProgramHandle(programID), "material.shine");
    uniformVariables[ShaderVariable::eyePosition] = Device().UniformLocation(ProgramHandle(programID), "eyePosition");

    uniformVariables[ShaderVariable::pointLightCount] = Device().UniformLocation(ProgramHandle(programID), "pointLightCount");
    uniformVariables[ShaderVariable::spotLightCount] = Device().UniformLocation(ProgramHandle(programID), "spotLightCount");

    uniformVariables[ShaderVariable::distanceToLight2D] = DrawUniformLocation(programID, "lightRatio");

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
        uniformPointLight[i].uniformColor = Device().UniformLocation(ProgramHandle(programID), locBuff);
        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
        uniformPointLight[i].uniformAmbientIntensity = Device().UniformLocation(ProgramHandle(programID), locBuff);
        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
        uniformPointLight[i].uniformDiffuseIntensity = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
        uniformPointLight[i].uniformPosition = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
        uniformPointLight[i].uniformConstant = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
        uniformPointLight[i].uniformLinear = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
        uniformPointLight[i].uniformExponent = Device().UniformLocation(ProgramHandle(programID), locBuff);
    }


    for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
        uniformSpotLight[i].uniformColor = Device().UniformLocation(ProgramHandle(programID), locBuff);
        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
        uniformSpotLight[i].uniformAmbientIntensity = Device().UniformLocation(ProgramHandle(programID), locBuff);
        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
        uniformSpotLight[i].uniformDiffuseIntensity = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
        uniformSpotLight[i].uniformPosition = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
        uniformSpotLight[i].uniformConstant = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
        uniformSpotLight[i].uniformLinear = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
        uniformSpotLight[i].uniformExponent = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
        uniformSpotLight[i].uniformDirection = Device().UniformLocation(ProgramHandle(programID), locBuff);

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
        uniformSpotLight[i].uniformEdge = Device().UniformLocation(ProgramHandle(programID), locBuff);
    }
}

unsigned int ShaderProgram::GetUniformVariable(ShaderVariable variable) const
{
    return uniformVariables[variable];
}

int ShaderProgram::glslVersion = 330;

std::string ShaderProgram::ApplyVersion(const std::string& src)
{
#ifdef __EMSCRIPTEN__
    // Web keeps its existing GLSL ES handling (FULL_ES3); don't rewrite.
    return src;
#else
    // Rewrite the first "#version ..." line to the context's version, e.g.
    // "#version 460 core", so 330-authored files can use 4.x features when the
    // GPU grants a 4.x context (and still work on the 3.3 fallback). Also inject
    // a capability define so shaders can #ifdef GL 4.x-only paths (e.g.
    // samplerCubeArray). Defines must follow the #version line.
    std::string ver = "#version " + std::to_string(glslVersion) + " core";
    if (glslVersion >= 400)
        ver += "\n#define KINJO_GL4";
    size_t pos = src.find("#version");
    if (pos == std::string::npos)
        return ver + "\n" + src;                 // no directive: prepend one
    size_t eol = src.find('\n', pos);
    if (eol == std::string::npos) eol = src.size();
    std::string out = src;
    out.replace(pos, eol - pos, ver);            // replace up to (not incl.) newline
    return out;
#endif
}

std::string ShaderProgram::ResolvePath(const std::string& path)
{
    // Development switch: KINJO_PREFER_ENGINE_SHADERS=1 makes the engine's copy
    // win wherever one exists, so any game can be run against the current
    // engine shaders without touching its data/shaders folder (e.g. to check
    // whether its old copies can be retired).
    static const bool preferEngine = []()
    {
        const char* v = std::getenv("KINJO_PREFER_ENGINE_SHADERS");
        const bool on = (v != nullptr && v[0] == '1');
        if (on)
            std::cout << "KINJO_PREFER_ENGINE_SHADERS: engine shaders override game copies" << std::endl;
        return on;
    }();

    std::error_code ec;
    static const std::string gameShaderFolder = "data/shaders/";
    std::string engineCandidate;
    if (path.compare(0, gameShaderFolder.size(), gameShaderFolder) == 0 && !EngineShaderDir().empty())
        engineCandidate = EngineShaderDir() + path.substr(gameShaderFolder.size());

    if (preferEngine && !engineCandidate.empty() && std::filesystem::exists(engineCandidate, ec))
        return engineCandidate;
    if (std::filesystem::exists(path, ec))
        return path;
    if (!engineCandidate.empty() && std::filesystem::exists(engineCandidate, ec))
        return engineCandidate;
    return path;
}

namespace
{
    // Expand `#include "name"` lines. The name resolves like any shader file
    // (data/shaders/<name>, then the engine's copy), so a game can override an
    // include too. Line numbers in compile errors after an include are offset
    // by the included file's length.
    std::string ExpandIncludes(ShaderProgram& program, const std::string& source, int depth = 0)
    {
        if (depth > 8)
        {
            std::cout << "ERROR: shader #include nesting too deep (cycle?)" << std::endl;
            return source;
        }

        std::string out;
        out.reserve(source.size());
        size_t start = 0;
        while (start < source.size())
        {
            size_t end = source.find('\n', start);
            if (end == std::string::npos)
                end = source.size();
            const std::string line = source.substr(start, end - start);

            const size_t hash = line.find_first_not_of(" \t");
            const size_t open = line.find('"');
            const size_t close = (open == std::string::npos) ? std::string::npos : line.find('"', open + 1);
            if (hash != std::string::npos && line.compare(hash, 8, "#include") == 0 && close != std::string::npos)
            {
                const std::string name = line.substr(open + 1, close - open - 1);
                const std::string path = ShaderProgram::ResolvePath("data/shaders/" + name);
                out += ExpandIncludes(program, program.ReadFile(path.c_str()), depth + 1);
            }
            else
            {
                out += line;
                out += '\n';
            }
            start = end + 1;
        }
        return out;
    }
}

void ShaderProgram::CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath)
{
    const std::string vertexPath = ResolvePath(vertexFilePath);
    const std::string fragmentPath = ResolvePath(fragmentFilePath);
    std::string vertexString = ApplyVersion(ExpandIncludes(*this, ReadFile(vertexPath.c_str())));
    std::string fragmentString = ApplyVersion(ExpandIncludes(*this, ReadFile(fragmentPath.c_str())));

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
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        content.append(line + "\n");
    }

    fileStream.close();

    return content;
}

void ShaderProgram::UseShader() const
{
    Device().UseProgram(ProgramHandle(programID));

    /*
    if (programID != lastProgramID)
    {
        lastProgramID = programID;
    } 
    */
}

int ShaderProgram::DrawUniformLocation(unsigned int program, const char* name)
{
    char drawName[96] = "draw.";
    const size_t prefix = 5;
    const size_t len = strlen(name);
    if (len + prefix < sizeof(drawName))
    {
        memcpy(drawName + prefix, name, len + 1);
        const int location = Device().UniformLocation(ProgramHandle(program), drawName);
        if (location != -1)
            return location;
    }
    return Device().UniformLocation(ProgramHandle(program), name);
}

void ShaderProgram::SetInt(const char* name, int value) const{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetFloat(const char* name, float value) const
{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetVec2(const char* name, const glm::vec2& value) const
{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetVec3(const char* name, const glm::vec3& value) const
{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetVec4(const char* name, const glm::vec4& value) const
{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetMat4(const char* name, const glm::mat4& value) const
{
    Device().SetUniform(DrawUniformLocation(programID, name), value);
}

void ShaderProgram::SetFloat(ShaderVariable variable, float value) const
{
    Device().SetUniform((int)GetUniformVariable(variable), value);
}

void ShaderProgram::SetVec4(ShaderVariable variable, const glm::vec4& value) const
{
    Device().SetUniform((int)GetUniformVariable(variable), value);
}

void ShaderProgram::ClearShader()
{
    if (programID != 0)
    {
        ProgramHandle program(programID);
        Device().DestroyProgram(program);
        programID = 0;
    }

    uniformVariables[ShaderVariable::model] = 0;
    uniformVariables[ShaderVariable::projection] = 0;
}
