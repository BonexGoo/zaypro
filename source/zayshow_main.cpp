#include <boss.hpp>
#include <platform/boss_platform.hpp>
#include <service/boss_zay.hpp>

#include <resource.hpp>

String gFirstPath;
sint32 gFirstPosX = 0;
sint32 gFirstPosY = 0;
sint32 gPythonAppID = -1;
sint32 gPythonPort = 0;

bool PlatformInit()
{
    #if BOSS_WINDOWS
        Platform::InitForGL(true);
        String DataPath = Platform::File::RootForData();
        Platform::File::ResetAssetsRemRoot(DataPath);
    #else
        return false;
    #endif

    String ZayShowInfoJson = String::FromAsset("zayshowinfo.json");
    Context ZayShowInfo(ST_Json, SO_OnlyReference, ZayShowInfoJson, ZayShowInfoJson.Length());
    gFirstPath = ZayShowInfo("path").GetText();
    gFirstPosX = ZayShowInfo("posx").GetInt();
    gFirstPosY = ZayShowInfo("posy").GetInt();

    sint32 ArgCount = 0;
    chars ZayPath = Platform::Utility::GetArgument(1, &ArgCount);
    if(ZayPath != nullptr && 1 < ArgCount)
    {
        gFirstPath = String::FromWChars(WString::FromCp949(false, ZayPath));
        if(2 < ArgCount) gFirstPosX = Parser::GetInt(Platform::Utility::GetArgument(2));
        if(3 < ArgCount) gFirstPosY = Parser::GetInt(Platform::Utility::GetArgument(3));
        if(4 < ArgCount) gPythonAppID = Parser::GetInt(Platform::Utility::GetArgument(4));
        if(5 < ArgCount) gPythonPort = Parser::GetInt(Platform::Utility::GetArgument(5));
    }

    Platform::SetViewCreator(ZayView::Creator);
    Platform::SetWindowName("ZayShow");
    Platform::SetWindowRect(gFirstPosX, gFirstPosY, 800, 800);
    Platform::SetWindowView("zayshowView");
    return true;
}

void PlatformQuit()
{
    const rect128 WindowRect = Platform::GetWindowRect(true);
    Context ZayShowInfo;
    ZayShowInfo.At("path").Set(gFirstPath);
    ZayShowInfo.At("posx").Set(String::FromInteger(WindowRect.l));
    ZayShowInfo.At("posy").Set(String::FromInteger(WindowRect.t));
    ZayShowInfo.SaveJson().ToAsset("zayshowinfo.json");

    Context ZayShowAtlas;
    R::SaveAtlas(ZayShowAtlas);
    ZayShowAtlas.SaveJson().ToAsset("zayshowatlas.json");
}

void PlatformFree()
{
}
