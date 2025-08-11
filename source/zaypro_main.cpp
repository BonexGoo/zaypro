#include <boss.hpp>
#include <platform/boss_platform.hpp>
#include <service/boss_zay.hpp>

#include <resource.hpp>

sint32 gBgPercent = 100;
sint32 gZoomPercent = 100;
bool gCtrlPressing = false;

bool PlatformInit()
{
    // Rem폴더 설정
    #if !BOSS_WASM
        String DataPath = Platform::File::RootForData();
        Platform::File::ResetAssetsRemRoot(DataPath);
    #endif

    // 관리자권한으로 실행하면 레지스트리에 zayshow를 등록
    Platform::Utility::BindExtProgram(".zay", "Zay.Show",
        Platform::Utility::GetProgramPath(true) + "zayshow.exe");

    // 윈도우 불러오기
    String WindowInfoString = String::FromAsset("windowinfo.json");
    Context WindowInfo(ST_Json, SO_OnlyReference, WindowInfoString, WindowInfoString.Length());
    const sint32 ScreenID = WindowInfo("screen").GetInt(0);
    gBgPercent = WindowInfo("bgpercent").GetInt(100);
    gZoomPercent = WindowInfo("zoompercent").GetInt(100);
    rect128 ScreenRect = {};
    Platform::Utility::GetScreenRect(ScreenRect, ScreenID);
    const sint32 ScreenWidth = ScreenRect.r - ScreenRect.l;
    const sint32 ScreenHeight = ScreenRect.b - ScreenRect.t;
    const sint32 WindowWidth = WindowInfo("w").GetInt(1200);
    const sint32 WindowHeight = WindowInfo("h").GetInt(800);
    const sint32 WindowX = Math::Clamp(WindowInfo("x").GetInt((ScreenWidth - WindowWidth) / 2), 0, ScreenWidth - WindowWidth);
    const sint32 WindowY = Math::Clamp(WindowInfo("y").GetInt((ScreenHeight - WindowHeight) / 2), 30, ScreenHeight - WindowHeight);
    Platform::SetWindowRect(ScreenRect.l + WindowX, ScreenRect.t + WindowY, WindowWidth, WindowHeight);

    // 아틀라스 불러오기
    const String AtlasInfoString = String::FromAsset("atlasinfo.json");
    Context AtlasInfo(ST_Json, SO_OnlyReference, AtlasInfoString, AtlasInfoString.Length());
    R::SetAtlasDir("image");
    R::AddAtlas("ui_atlaskey.png", "zaypro_atlas.png", AtlasInfo);
    if(R::IsAtlasUpdated())
        R::RebuildAll();
    Platform::AddProcedure(PE_100MSEC,
        [](payload data)->void
        {
            if(R::IsAtlasUpdated())
            {
                R::RebuildAll();
                Platform::UpdateAllViews();
            }
        });

    // 윈도우 설정
    Platform::InitForMDI(true);
    Platform::SetViewCreator(ZayView::Creator);
    Platform::SetWindowName("ZAYPRO");
    Platform::SetWindowView("zayproView");
    return true;
}

void PlatformQuit()
{
    // 윈도우 저장하기
    const rect128 WindowRect = Platform::GetWindowRect(true);
    const sint32 ScreenID = Platform::Utility::GetScreenID({(WindowRect.l + WindowRect.r) / 2, (WindowRect.t + WindowRect.b) / 2});
    rect128 ScreenRect = {};
    Platform::Utility::GetScreenRect(ScreenRect, ScreenID);
    Context WindowInfo;
    WindowInfo.At("screen").Set(String::FromInteger(ScreenID));
    WindowInfo.At("bgpercent").Set(String::FromInteger(gBgPercent));
    WindowInfo.At("zoompercent").Set(String::FromInteger(gZoomPercent));
    WindowInfo.At("x").Set(String::FromInteger(WindowRect.l - ScreenRect.l));
    WindowInfo.At("y").Set(String::FromInteger(WindowRect.t - ScreenRect.t));
    WindowInfo.At("w").Set(String::FromInteger(WindowRect.r - WindowRect.l));
    WindowInfo.At("h").Set(String::FromInteger(WindowRect.b - WindowRect.t));
    WindowInfo.SaveJson().ToAsset("windowinfo.json");

    // 아틀라스 저장하기
    Context AtlasInfo;
    R::SaveAtlas(AtlasInfo);
    AtlasInfo.SaveJson().ToAsset("atlasinfo.json");
}

void PlatformFree()
{
}
