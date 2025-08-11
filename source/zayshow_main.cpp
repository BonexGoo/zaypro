#include <boss.hpp>
#include <platform/boss_platform.hpp>
#include <service/boss_zay.hpp>

#include <resource.hpp>

String gFirstPath;
sint32 gFirstPosX = 0;
sint32 gFirstPosY = 0;
sint32 gPythonAppID = -1;
sint32 gPythonPort = 0;

void SetWindowRectSafety(sint32 x, sint32 y, sint32 w, sint32 h);

bool PlatformInit()
{
    // Rem폴더 설정
    #if BOSS_WINDOWS
        String DataPath = Platform::File::RootForData();
        Platform::File::ResetAssetsRemRoot(DataPath);
    #else
        return false;
    #endif

    // 윈도우 불러오기
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

    // 윈도우 설정
    Platform::InitForMDI(true);
    Platform::SetViewCreator(ZayView::Creator);
    Platform::SetWindowName("ZayShow");
    SetWindowRectSafety(gFirstPosX, gFirstPosY, 800, 800);
    Platform::SetWindowView("zayshowView");
    return true;
}

void PlatformQuit()
{
    // 윈도우 저장하기
    const rect128 WindowRect = Platform::GetWindowRect(true);
    Context ZayShowInfo;
    ZayShowInfo.At("path").Set(gFirstPath);
    ZayShowInfo.At("posx").Set(String::FromInteger(WindowRect.l));
    ZayShowInfo.At("posy").Set(String::FromInteger(WindowRect.t));
    ZayShowInfo.SaveJson().ToAsset("zayshowinfo.json");

    // 아틀라스 저장하기
    Context ZayShowAtlas;
    R::SaveAtlas(ZayShowAtlas);
    ZayShowAtlas.SaveJson().ToAsset("zayshowatlas.json");
}

void PlatformFree()
{
}

void SetWindowRectSafety(sint32 x, sint32 y, sint32 w, sint32 h)
{
    // 가장 가까운 화면을 찾고
    rect128 WindowRect{x, y, x + w, y + h};
    sint32 NearScreenID = 0;
    float BestDist = 0;
    for(sint32 i = 0, iend = Platform::Utility::GetScreenCount(); i < iend; ++i)
    {
        rect128 CurRect;
        Platform::Utility::GetScreenRect(CurRect, i);
        const float CurDist = Math::Distance((CurRect.l + CurRect.r) * 0.5, (CurRect.t + CurRect.b) * 0.5,
            (WindowRect.l + WindowRect.r) * 0.5, (WindowRect.t + WindowRect.b) * 0.5);
        if(BestDist > CurDist || i == 0)
        {
            NearScreenID = i;
            BestDist = CurDist;
        }
    }

    // 화면을 나간 경우를 처리
    rect128 ScreenRect;
    Platform::Utility::GetScreenRect(ScreenRect, NearScreenID);
    const sint32 InnerGap = 0;
    if(ScreenRect.r - InnerGap < WindowRect.r)
    {
        const sint32 AddX = ScreenRect.r - InnerGap - WindowRect.r;
        WindowRect.l += AddX;
        WindowRect.r += AddX;
    }
    if(ScreenRect.b - InnerGap < WindowRect.b)
    {
        const sint32 AddY = ScreenRect.b - InnerGap - WindowRect.b;
        WindowRect.t += AddY;
        WindowRect.b += AddY;
    }
    if(WindowRect.l < ScreenRect.l + InnerGap)
    {
        const sint32 AddX = ScreenRect.l + InnerGap - WindowRect.l;
        WindowRect.l += AddX;
        WindowRect.r += AddX;
    }
    if(WindowRect.t < ScreenRect.t + InnerGap)
    {
        const sint32 AddY = ScreenRect.t + InnerGap - WindowRect.t;
        WindowRect.t += AddY;
        WindowRect.b += AddY;
    }
    Platform::SetWindowRect(WindowRect.l, WindowRect.t,
        WindowRect.r - WindowRect.l, WindowRect.b - WindowRect.t);
}
