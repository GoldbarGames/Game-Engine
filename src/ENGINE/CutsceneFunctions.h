#include "CutsceneCommands.h"

namespace CutsceneFunctions
{

	// Load graphics
	int KINJO_API LoadSprite(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ClearSprite(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetSpriteProperty(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API LoadBackground(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AnimationCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ParticleCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Text
	int KINJO_API LoadText(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API TextColor(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API LoadTextFromSaveFile(CutsceneParameters parameters, CutsceneCommands& c);

	// Sounds
	int KINJO_API MusicCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SoundCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API MusicEffectCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Stuff
	int KINJO_API SetVelocity(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API Wait(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API Textbox(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API Namebox(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API Fade(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetStringAlias(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetNumAlias(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API IncludeCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AssetPathCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Variables
	int KINJO_API SetNumberVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetStringVariable(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API CreateArrayVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ConcatenateStringVariables(CutsceneParameters parameters, CutsceneCommands& c);

	// Numeric Operations
	void CacheNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AddNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SubtractNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API MultiplyNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DivideNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ModNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API RandomNumberVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API MoveVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SubstringVariables(CutsceneParameters parameters, CutsceneCommands& c);

	// Control Flow
	int KINJO_API GoToLabel(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API IfCondition(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API JumpBack(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API JumpForward(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API GoSubroutine(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ReturnFromSubroutine(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DisplayChoice(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API WaitForClick(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API WaitForButton(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetSpriteButton(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API EndGame(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ResetGame(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SaveGame(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API LoadGame(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API SetResolution(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DefineUserFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DefineChoice(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API DoNothing(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API SetGlobalNumber(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API OpenBacklog(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API TimerFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API CameraFunction(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API WindowFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ControlBindings(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API BindKeyToLabel(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API FlipSprite(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API RightClickSettings(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API Quake(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API LuaCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetClickToContinue(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API ScreenshotCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ErrorLog(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API FontCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API GetData(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API NameCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API NameDefineCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API IntToString(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API IncrementVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DecrementVariable(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API Output(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API TagCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API FileExist(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API TextSpeed(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AutoMode(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AutoReturn(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AutoSave(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AutoSkip(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AutoChoice(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API AlignCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API InputCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API PrintCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API EffectCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API LineBreakCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API RepeatCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API TravelCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API ToggleSkipping(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API IsSkipping(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API CreateShader(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API SetShaderFilter(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API ShellCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int KINJO_API SteamCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int KINJO_API DrawRectCommand(CutsceneParameters parameters, CutsceneCommands& c);

}