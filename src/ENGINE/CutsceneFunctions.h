#include "CutsceneCommands.h"

namespace CutsceneFunctions
{

	// Load graphics
	int LoadSprite(CutsceneParameters parameters, CutsceneCommands& c);
	int ClearSprite(CutsceneParameters parameters, CutsceneCommands& c);
	int SetSpriteProperty(CutsceneParameters parameters, CutsceneCommands& c);
	int LoadBackground(CutsceneParameters parameters, CutsceneCommands& c);
	int AnimationCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int ParticleCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Text
	int LoadText(CutsceneParameters parameters, CutsceneCommands& c);
	int TextColor(CutsceneParameters parameters, CutsceneCommands& c);
	int LoadTextFromSaveFile(CutsceneParameters parameters, CutsceneCommands& c);

	// Sounds
	int MusicCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int SoundCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int MusicEffectCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Stuff
	int SetVelocity(CutsceneParameters parameters, CutsceneCommands& c);
	int Wait(CutsceneParameters parameters, CutsceneCommands& c);
	int Textbox(CutsceneParameters parameters, CutsceneCommands& c);
	int Namebox(CutsceneParameters parameters, CutsceneCommands& c);
	int Fade(CutsceneParameters parameters, CutsceneCommands& c);
	int SetStringAlias(CutsceneParameters parameters, CutsceneCommands& c);
	int SetNumAlias(CutsceneParameters parameters, CutsceneCommands& c);
	int IncludeCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int AssetPathCommand(CutsceneParameters parameters, CutsceneCommands& c);

	// Variables
	int SetNumberVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int SetStringVariable(CutsceneParameters parameters, CutsceneCommands& c);

	int CreateArrayVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int ConcatenateStringVariables(CutsceneParameters parameters, CutsceneCommands& c);

	// Numeric Operations
	void CacheNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int AddNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int SubtractNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int MultiplyNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int DivideNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int ModNumberVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int RandomNumberVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int MoveVariables(CutsceneParameters parameters, CutsceneCommands& c);
	int SubstringVariables(CutsceneParameters parameters, CutsceneCommands& c);

	// Control Flow
	int GoToLabel(CutsceneParameters parameters, CutsceneCommands& c);
	int IfCondition(CutsceneParameters parameters, CutsceneCommands& c);
	int JumpBack(CutsceneParameters parameters, CutsceneCommands& c);
	int JumpForward(CutsceneParameters parameters, CutsceneCommands& c);
	int GoSubroutine(CutsceneParameters parameters, CutsceneCommands& c);
	int ReturnFromSubroutine(CutsceneParameters parameters, CutsceneCommands& c);
	int DisplayChoice(CutsceneParameters parameters, CutsceneCommands& c);

	int WaitForClick(CutsceneParameters parameters, CutsceneCommands& c);
	int WaitForButton(CutsceneParameters parameters, CutsceneCommands& c);
	int SetSpriteButton(CutsceneParameters parameters, CutsceneCommands& c);

	int EndGame(CutsceneParameters parameters, CutsceneCommands& c);
	int ResetGame(CutsceneParameters parameters, CutsceneCommands& c);
	int SaveGame(CutsceneParameters parameters, CutsceneCommands& c);
	int LoadGame(CutsceneParameters parameters, CutsceneCommands& c);

	int SetResolution(CutsceneParameters parameters, CutsceneCommands& c);
	int DefineUserFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int DefineChoice(CutsceneParameters parameters, CutsceneCommands& c);

	int DoNothing(CutsceneParameters parameters, CutsceneCommands& c);

	int SetGlobalNumber(CutsceneParameters parameters, CutsceneCommands& c);
	int OpenBacklog(CutsceneParameters parameters, CutsceneCommands& c);

	int TimerFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int CameraFunction(CutsceneParameters parameters, CutsceneCommands& c);

	int WindowFunction(CutsceneParameters parameters, CutsceneCommands& c);
	int ControlBindings(CutsceneParameters parameters, CutsceneCommands& c);
	int BindKeyToLabel(CutsceneParameters parameters, CutsceneCommands& c);

	int FlipSprite(CutsceneParameters parameters, CutsceneCommands& c);

	int RightClickSettings(CutsceneParameters parameters, CutsceneCommands& c);
	int Quake(CutsceneParameters parameters, CutsceneCommands& c);

	int LuaCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int SetClickToContinue(CutsceneParameters parameters, CutsceneCommands& c);

	int ScreenshotCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int ErrorLog(CutsceneParameters parameters, CutsceneCommands& c);

	int FontCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int GetData(CutsceneParameters parameters, CutsceneCommands& c);
	int NameCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int NameDefineCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int IntToString(CutsceneParameters parameters, CutsceneCommands& c);

	int IncrementVariable(CutsceneParameters parameters, CutsceneCommands& c);
	int DecrementVariable(CutsceneParameters parameters, CutsceneCommands& c);

	int Output(CutsceneParameters parameters, CutsceneCommands& c);

	int TagCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int FileExist(CutsceneParameters parameters, CutsceneCommands& c);
	int TextSpeed(CutsceneParameters parameters, CutsceneCommands& c);
	int AutoMode(CutsceneParameters parameters, CutsceneCommands& c);
	int AutoReturn(CutsceneParameters parameters, CutsceneCommands& c);
	int AutoSave(CutsceneParameters parameters, CutsceneCommands& c);
	int AutoSkip(CutsceneParameters parameters, CutsceneCommands& c);
	int AutoChoice(CutsceneParameters parameters, CutsceneCommands& c);
	int AlignCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int InputCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int PrintCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int EffectCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int LineBreakCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int RepeatCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int TravelCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int ToggleSkipping(CutsceneParameters parameters, CutsceneCommands& c);
	int IsSkipping(CutsceneParameters parameters, CutsceneCommands& c);

	int CreateShader(CutsceneParameters parameters, CutsceneCommands& c);
	int SetShaderFilter(CutsceneParameters parameters, CutsceneCommands& c);
	int ShellCommand(CutsceneParameters parameters, CutsceneCommands& c);

	int SteamCommand(CutsceneParameters parameters, CutsceneCommands& c);
	int DrawRectCommand(CutsceneParameters parameters, CutsceneCommands& c);

}