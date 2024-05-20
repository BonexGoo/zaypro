#include <boss.hpp>
#include "zayshow.hpp"

#include <resource.hpp>
#include <service/boss_zaywidget.hpp>

ZAY_DECLARE_VIEW_CLASS("zayshowView", zayshowData)

extern String gFirstPath;
extern sint32 gPythonAppID;
extern sint32 gPythonPort;

extern void SetWindowRectSafety(sint32 x, sint32 y, sint32 w, sint32 h);

ZAY_VIEW_API OnCommand(CommandType type, id_share in, id_cloned_share* out)
{
    if(type == CT_Activate && !boolo(in).ConstValue())
        m->clearCapture();
    else if(type == CT_Tick)
    {
        const uint64 NowMsec = Platform::Utility::CurrentTimeMsec();
        if(m->mWidget && m->mWidget->TickOnce())
            m->invalidate();
        if(NowMsec <= m->mWidgetUpdater)
            m->invalidate(2);

        // 파이썬 연결
        if(gPythonPort != 0)
        {
            id_socket NewSocket = Platform::Socket::OpenForWS(false);
            if(!Platform::Socket::Connect(NewSocket, "127.0.0.1", gPythonPort))
                Platform::Socket::Close(NewSocket);
            else
            {
                m->mPython = NewSocket;
                m->PythonSend(String::Format("init,%d", gPythonAppID));
            }
            gPythonPort = 0;
        }
    }
}

ZAY_VIEW_API OnNotify(NotifyType type, chars topic, id_share in, id_cloned_share* out)
{
    if(type == NT_Normal)
    {
        if(!String::Compare(topic, "ZS_Update"))
        {
            const uint64 NewUpdateMsec = Platform::Utility::CurrentTimeMsec() + sint32o(in).ConstValue();
            if(m->mWidgetUpdater < NewUpdateMsec)
                m->mWidgetUpdater = NewUpdateMsec;
        }
        else if(!String::Compare(topic, "ZS_Log"))
        {
            const String Message(in);
            m->mWidget->SendLog(Message);
        }
        else if(!String::Compare(topic, "ZS_FullScreen"))
        {
            if(m->IsFullScreen())
                m->SetNormalWindow();
            else m->SetFullScreen();
        }
        else if(!String::Compare(topic, "ZS_ClearCapture"))
        {
            m->clearCapture();
        }
    }
    else if(type == NT_KeyPress)
    {
        const sint32 KeyCode = sint32o(in).ConstValue();
        // 파이썬 키전달
        if(m->mPython != nullptr)
        {
            const String KeyText = m->FindPythonKey(KeyCode);
            if(0 < KeyText.Length())
                m->PythonSend(String::Format("call,KeyPress_%s", (chars) KeyText));
        }
    }
    else if(type == NT_KeyRelease)
    {
        const sint32 KeyCode = sint32o(in).ConstValue();
        // 파이썬 키전달
        if(m->mPython != nullptr)
        {
            const String KeyText = m->FindPythonKey(KeyCode);
            if(0 < KeyText.Length())
                m->PythonSend(String::Format("call,KeyRelease_%s", (chars) KeyText));
        }
    }
    else if(type == NT_SocketReceive)
    {
        if(!String::Compare(topic, "message"))
            m->TryPythonRecvOnce();
        else if(!String::Compare(topic, "disconnected"))
        {
            // 파이썬 접속확인
            if(m->mPython && !Platform::Socket::IsConnected(m->mPython))
            {
                Platform::Socket::Close(m->mPython);
                m->mPython = nullptr;
                Platform::Utility::ExitProgram();
            }
        }
    }
    else if(type == NT_ZayWidget)
    {
        if(!String::Compare(topic, "SetCursor"))
        {
            auto Cursor = CursorRole(sint32o(in).Value());
            m->SetCursor(Cursor);
        }
    }
    else if(type == NT_ZayAtlas)
    {
        if(!String::Compare(topic, "AtlasUpdated"))
        {
            const String AtlasJson = R::PrintUpdatedAtlas();
            if(m->mWidget)
                m->mWidget->UpdateAtlas(AtlasJson);
        }
    }
}

ZAY_VIEW_API OnGesture(GestureType type, sint32 x, sint32 y)
{
    static point64 OldCursorPos;
    static rect128 OldWindowRect;
    static uint64 ReleaseMsec = 0;

    if(type == GT_Moving || type == GT_MovingIdle)
        m->SetCursor(CR_Arrow);
    else if(type == GT_Pressed)
    {
        Platform::Utility::GetCursorPos(OldCursorPos);
        OldWindowRect = Platform::GetWindowRect();
        m->clearCapture();
    }
    else if(type == GT_InDragging || type == GT_OutDragging)
    {
        if(!m->IsFullScreen())
        {
            point64 CurCursorPos;
            Platform::Utility::GetCursorPos(CurCursorPos);
            Platform::SetWindowRect(
                OldWindowRect.l + CurCursorPos.x - OldCursorPos.x,
                OldWindowRect.t + CurCursorPos.y - OldCursorPos.y,
                OldWindowRect.r - OldWindowRect.l, OldWindowRect.b - OldWindowRect.t);
            m->mIsWindowMoving = true;
        }
        ReleaseMsec = 0;
        m->invalidate();
    }
    else if(type == GT_InReleased || type == GT_OutReleased || type == GT_CancelReleased)
    {
        const uint64 CurReleaseMsec = Platform::Utility::CurrentTimeMsec();
        const bool HasDoubleClick = (CurReleaseMsec < ReleaseMsec + 300);
        if(HasDoubleClick)
        {
            if(m->IsFullScreen())
                m->SetNormalWindow();
            else m->SetFullScreen();
        }
        else ReleaseMsec = CurReleaseMsec;
        m->mIsWindowMoving = false;
    }
}

ZAY_VIEW_API OnRender(ZayPanel& panel)
{
    ZAY_XYWH(panel, 0, 0, panel.w(), panel.h() - 1)
    {
        if(m->mWidget)
            m->mWidget->Render(panel);
        else
        {
            ZAY_RGB(panel, 255, 255, 255)
                panel.fill();
        }

        // 아웃라인
        if(!m->IsFullScreen())
            m->RenderWindowOutline(panel);
    }
}

zayshowData::zayshowData()
{
    // 빌드시간
    String DateText = __DATE__;
    DateText.Replace("Jan", "01"); DateText.Replace("Feb", "02"); DateText.Replace("Mar", "03");
    DateText.Replace("Apr", "04"); DateText.Replace("May", "05"); DateText.Replace("Jun", "06");
    DateText.Replace("Jul", "07"); DateText.Replace("Aug", "08"); DateText.Replace("Sep", "09");
    DateText.Replace("Oct", "10"); DateText.Replace("Nov", "11"); DateText.Replace("Dec", "12");
    const String Day = String::Format("%02d", Parser::GetInt(DateText.Middle(2, DateText.Length() - 6).Trim()));
    DateText = DateText.Right(4) + "/" + DateText.Left(2) + "/" + Day;
    ZayWidgetDOM::SetValue("zayshow.build", "'" + DateText + "'");

    // 실행정보
    if(0 < gFirstPath.Length())
    {
        const String ZayShowJson = String::FromFile(gFirstPath + "/../zayshow.json");
        Context ZayShow(ST_Json, SO_OnlyReference, ZayShowJson, ZayShowJson.Length());
        ZayWidgetDOM::SetJson(ZayShow, "zayshow.");
        SetWidget(gFirstPath);
    }
}

zayshowData::~zayshowData()
{
    delete mWidget;
    for(sint32 i = 0; i < 4; ++i)
        Platform::Sound::Close(mSounds[i]);
    Platform::Socket::Close(mPython);
}

void zayshowData::SetCursor(CursorRole role)
{
    if(mNowCursor != role)
    {
        mNowCursor = role;
        Platform::Utility::SetCursor(role);
        if(mNowCursor != CR_SizeVer && mNowCursor != CR_SizeHor && mNowCursor != CR_SizeBDiag && mNowCursor != CR_SizeFDiag && mNowCursor != CR_SizeAll)
        {
            mNcLeftBorder = false;
            mNcTopBorder = false;
            mNcRightBorder = false;
            mNcBottomBorder = false;
        }
    }
}

sint32 zayshowData::MoveNcLeft(const rect128& rect, sint32 addx)
{
    const sint32 NewLeft = rect.l + addx;
    return rect.r - Math::Max(mMinWindowWidth, rect.r - NewLeft);
}

sint32 zayshowData::MoveNcTop(const rect128& rect, sint32 addy)
{
    const sint32 NewTop = rect.t + addy;
    return rect.b - Math::Max(mMinWindowHeight, rect.b - NewTop);
}

sint32 zayshowData::MoveNcRight(const rect128& rect, sint32 addx)
{
    const sint32 NewRight = rect.r + addx;
    return rect.l + Math::Max(mMinWindowWidth, NewRight - rect.l);
}

sint32 zayshowData::MoveNcBottom(const rect128& rect, sint32 addy)
{
    const sint32 NewBottom = rect.b + addy;
    return rect.t + Math::Max(mMinWindowHeight, NewBottom - rect.t);
}

void zayshowData::RenderWindowOutline(ZayPanel& panel)
{
    ZAY_INNER(panel, 1)
    ZAY_RGBA(panel, 0, 0, 0, 32)
        panel.rect(1);

    if(ZayWidgetDOM::GetValue("zayshow.flexible").ToInteger() == 0)
        return;

    // 리사이징바
    ZAY_RGBA(panel, 0, 0, 0, 64 + 128 * Math::Abs(((frame() * 2) % 100) - 50) / 50)
    {
        if(mNcLeftBorder)
        {
            for(sint32 i = 0; i < 5; ++i)
            ZAY_RGBA(panel, 128, 128, 128, 128 - i * 25)
            ZAY_XYWH(panel, i, 0, 1, panel.h())
                panel.fill();
            invalidate(2);
        }
        if(mNcTopBorder)
        {
            for(sint32 i = 0; i < 5; ++i)
            ZAY_RGBA(panel, 128, 128, 128, 128 - i * 25)
            ZAY_XYWH(panel, 0, i, panel.w(), 1)
                panel.fill();
            invalidate(2);
        }
        if(mNcRightBorder)
        {
            for(sint32 i = 0; i < 5; ++i)
            ZAY_RGBA(panel, 128, 128, 128, 128 - i * 25)
            ZAY_XYWH(panel, panel.w() - 1 - i, 0, 1, panel.h())
                panel.fill();
            invalidate(2);
        }
        if(mNcBottomBorder)
        {
            for(sint32 i = 0; i < 5; ++i)
            ZAY_RGBA(panel, 128, 128, 128, 128 - i * 25)
            ZAY_XYWH(panel, 0, panel.h() - 1 - i, panel.w(), 1)
                panel.fill();
            invalidate(2);
        }
    }

    // 윈도우 리사이징 모듈
    #define RESIZING_MODULE(C, L, T, R, B, BORDER) do {\
        static point64 OldMousePos; \
        static rect128 OldWindowRect; \
        if(t == GT_Moving) \
        { \
            SetCursor(C); \
            mNcLeftBorder = false; \
            mNcTopBorder = false; \
            mNcRightBorder = false; \
            mNcBottomBorder = false; \
            BORDER = true; \
        } \
        else if(t == GT_MovingLosed) \
        { \
            SetCursor(CR_Arrow); \
        } \
        else if(t == GT_Pressed) \
        { \
            Platform::Utility::GetCursorPos(OldMousePos); \
            OldWindowRect = Platform::GetWindowRect(); \
        } \
        else if(t == GT_InDragging || t == GT_OutDragging || t == GT_InReleased || t == GT_OutReleased || t == GT_CancelReleased) \
        { \
            point64 NewMousePos; \
            Platform::Utility::GetCursorPos(NewMousePos); \
            const rect128 NewWindowRect = {L, T, R, B}; \
            Platform::SetWindowRect(NewWindowRect.l, NewWindowRect.t, \
                NewWindowRect.r - NewWindowRect.l, NewWindowRect.b - NewWindowRect.t); \
        }} while(false);

    // 윈도우 리사이징 꼭지점
    const sint32 SizeBorder = 10;
    ZAY_LTRB_UI(panel, 0, 0, SizeBorder, SizeBorder, "NcLeftTop",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeFDiag,
                MoveNcLeft(OldWindowRect, NewMousePos.x - OldMousePos.x),
                MoveNcTop(OldWindowRect, NewMousePos.y - OldMousePos.y),
                OldWindowRect.r,
                OldWindowRect.b,
                mNcLeftBorder = mNcTopBorder);
        });
    ZAY_LTRB_UI(panel, panel.w() - SizeBorder, 0, panel.w(), SizeBorder, "NcRightTop",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeBDiag,
                OldWindowRect.l,
                MoveNcTop(OldWindowRect, NewMousePos.y - OldMousePos.y),
                MoveNcRight(OldWindowRect, NewMousePos.x - OldMousePos.x),
                OldWindowRect.b,
                mNcRightBorder = mNcTopBorder);
        });
    ZAY_LTRB_UI(panel, 0, panel.h() - SizeBorder, SizeBorder, panel.h(), "NcLeftBottom",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeBDiag,
                MoveNcLeft(OldWindowRect, NewMousePos.x - OldMousePos.x),
                OldWindowRect.t,
                OldWindowRect.r,
                MoveNcBottom(OldWindowRect, NewMousePos.y - OldMousePos.y),
                mNcLeftBorder = mNcBottomBorder);
        });
    ZAY_LTRB_UI(panel, panel.w() - SizeBorder, panel.h() - SizeBorder, panel.w(), panel.h(), "NcRightBottom",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeFDiag,
                OldWindowRect.l,
                OldWindowRect.t,
                MoveNcRight(OldWindowRect, NewMousePos.x - OldMousePos.x),
                MoveNcBottom(OldWindowRect, NewMousePos.y - OldMousePos.y),
                mNcRightBorder = mNcBottomBorder);
        });

    // 윈도우 리사이징 모서리
    ZAY_LTRB_UI(panel, 0, SizeBorder, SizeBorder, panel.h() - SizeBorder, "NcLeft",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeHor,
                MoveNcLeft(OldWindowRect, NewMousePos.x - OldMousePos.x),
                OldWindowRect.t,
                OldWindowRect.r,
                OldWindowRect.b,
                mNcLeftBorder);
        });
    ZAY_LTRB_UI(panel, SizeBorder, 0, panel.w() - SizeBorder, SizeBorder, "NcTop",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeVer,
                OldWindowRect.l,
                MoveNcTop(OldWindowRect, NewMousePos.y - OldMousePos.y),
                OldWindowRect.r,
                OldWindowRect.b,
                mNcTopBorder);
        });
    ZAY_LTRB_UI(panel, panel.w() - SizeBorder, SizeBorder, panel.w(), panel.h() - SizeBorder, "NcRight",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeHor,
                OldWindowRect.l,
                OldWindowRect.t,
                MoveNcRight(OldWindowRect, NewMousePos.x - OldMousePos.x),
                OldWindowRect.b,
                mNcRightBorder);
        });
    ZAY_LTRB_UI(panel, SizeBorder, panel.h() - SizeBorder, panel.w() - SizeBorder, panel.h(), "NcBottom",
        ZAY_GESTURE_T(t, this)
        {
            RESIZING_MODULE(CR_SizeVer,
                OldWindowRect.l,
                OldWindowRect.t,
                OldWindowRect.r,
                MoveNcBottom(OldWindowRect, NewMousePos.y - OldMousePos.y),
                mNcBottomBorder);
        });
}

bool zayshowData::IsFullScreen()
{
    return mIsFullScreen;
}

void zayshowData::SetFullScreen()
{
    if(!mIsFullScreen)
    {
        mIsFullScreen = true;
        mSavedNormalRect = Platform::GetWindowRect();

        point64 CursorPos;
        Platform::Utility::GetCursorPos(CursorPos);
        sint32 ScreenID = Platform::Utility::GetScreenID(CursorPos);
        rect128 FullScreenRect;
        Platform::Utility::GetScreenRect(FullScreenRect, ScreenID, false);
        Platform::SetWindowRect(FullScreenRect.l, FullScreenRect.t,
            FullScreenRect.r - FullScreenRect.l, FullScreenRect.b - FullScreenRect.t + 1);
    }
}

void zayshowData::SetNormalWindow()
{
    if(mIsFullScreen)
    {
        mIsFullScreen = false;
        Platform::SetWindowRect(mSavedNormalRect.l, mSavedNormalRect.t,
            mSavedNormalRect.r - mSavedNormalRect.l, mSavedNormalRect.b - mSavedNormalRect.t);
    }
}

void zayshowData::SetWidget(chars path)
{
    const String CurTitle = ZayWidgetDOM::GetValue("zayshow.title").ToText();
    const sint32 CurWidth = ZayWidgetDOM::GetValue("zayshow.sizex").ToInteger();
    const sint32 CurHeight = ZayWidgetDOM::GetValue("zayshow.sizey").ToInteger();
    const sint32 CurScale = ZayWidgetDOM::GetValue("zayshow.scale").ToInteger();

    // 폰트로딩
    const String FontPath = boss_normalpath(String::Format("%s/../../font", path), nullptr);
    Platform::File::Search(FontPath,
        [](chars itemname, payload data)->void
        {
            auto& FontPath = *((const String*) data);
            if(buffer NewFont = Asset::ToBuffer(String::Format("%s/%s", (chars) FontPath, itemname)))
            {
                auto CurFontName = Platform::Utility::CreateSystemFont((bytes) NewFont, Buffer::CountOf(NewFont));
                Buffer::Free(NewFont);
                ZayWidgetDOM::SetValue(String::Format("fonts.%s", itemname), "'" + CurFontName + "'");
            }
        }, (payload) &FontPath, false);

    // 아틀라스로딩
    const sint32 CurAtlasCount = ZayWidgetDOM::GetValue("zayshow.atlas.count").ToInteger();
    if(0 < CurAtlasCount)
    {
        const String ZayShowAtlasJson = String::FromAsset("zayshowatlas.json");
        Context ZayShowAtlas(ST_Json, SO_OnlyReference, ZayShowAtlasJson, ZayShowAtlasJson.Length());
        R::SetAtlasDir("image");
        R::SaveAtlas(ZayShowAtlas); // 최신정보의 병합
        for(sint32 i = 0; i < CurAtlasCount; ++i)
        {
            const String AtlasFileName = ZayWidgetDOM::GetValue(String::Format("zayshow.atlas.%d", i)).ToText();
            chars AtlasPathName = boss_normalpath(String::Format("%s/../../%s", path, (chars) AtlasFileName), nullptr);
            R::AddAtlas("ui_atlaskey2.png", AtlasPathName, ZayShowAtlas, 2);
        }
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
    }

    // 위젯생성
    mWidget = new ZayWidget();
    InitWidget(*mWidget, "main");
    mWidget->Reload(path);
    mWidget->UpdateAtlas(R::PrintUpdatedAtlas(true));
    Platform::SetWindowName(CurTitle);
    const rect128 CurWindowRect = Platform::GetWindowRect();
    SetWindowRectSafety(CurWindowRect.l, CurWindowRect.t,
        CurWidth * CurScale / 100, CurHeight * CurScale / 100 + 1);
}

void zayshowData::InitWidget(ZayWidget& widget, chars name)
{
    widget.Init(name, nullptr, [](chars name)->const Image* {return &((const Image&) R(name));})

        // 특정시간동안 지속적인 화면업데이트
        .AddGlue("update", ZAY_DECLARE_GLUE(params, this)
        {
            if(params.ParamCount() == 1)
            {
                const sint32o Msec(sint32(params.Param(0).ToFloat() * 1000));
                Platform::BroadcastNotify("ZS_Update", Msec, NT_Normal, nullptr, true);
            }
        })

        // 로그출력
        .AddGlue("log", ZAY_DECLARE_GLUE(params, this)
        {
            if(params.ParamCount() == 1)
            {
                const String Message = params.Param(0).ToText();
                Platform::BroadcastNotify("ZS_Log", Message);
            }
        })

        // 풀스크린
        .AddGlue("full", ZAY_DECLARE_GLUE(params, this)
        {
            Platform::BroadcastNotify("ZS_FullScreen", nullptr);
        })

        // 에디터박스 포커싱제거
        .AddGlue("clear_capture", ZAY_DECLARE_GLUE(params, this)
        {
            Platform::BroadcastNotify("ZS_ClearCapture", nullptr);
        })

        // 파이썬실행
        .AddGlue("python", ZAY_DECLARE_GLUE(params, this)
        {
            if(params.ParamCount() == 1)
            {
                const String Func = params.Param(0).ToText();
                if(mPython != nullptr)
                    PythonSend(String::Format("call,%s", (chars) Func));
            }
        })

        // DOM변경
        .AddGlue("update_dom", ZAY_DECLARE_GLUE(params, this)
        {
            if(1 < params.ParamCount())
            {
                const String Key = params.Param(0).ToText();
                const String Value = params.Param(1).ToText();
                UpdateDom(Key, Value);
                invalidate();
            }
        })

        // 사운드재생
        .AddGlue("play_sound", ZAY_DECLARE_GLUE(params, this)
        {
            if(0 < params.ParamCount())
            {
                const String FileName = params.Param(0).ToText();
                PlaySound(FileName);
                invalidate();
            }
        })

        // 사운드중지
        .AddGlue("stop_sound", ZAY_DECLARE_GLUE(params, this)
        {
            StopSound();
            invalidate();
        })

        // 게이트호출
        .AddGlue("call_gate", ZAY_DECLARE_GLUE(params, this)
        {
            if(params.ParamCount() == 1)
            {
                const String GateName = params.Param(0).ToText();
                CallGate(GateName);
                invalidate();
            }
        });
}

void zayshowData::UpdateDom(chars key, chars value)
{
    if(key[0] == 'd' && key[1] == '.')
        ZayWidgetDOM::SetValue(&key[2], value);
}

void zayshowData::PlaySound(chars filename)
{
    Platform::Sound::Close(mSounds[mSoundFocus]);
    mSounds[mSoundFocus] = Platform::Sound::OpenForFile(Platform::File::RootForAssets() + filename);
    Platform::Sound::SetVolume(mSounds[mSoundFocus], 1);
    Platform::Sound::Play(mSounds[mSoundFocus]);
    mSoundFocus = (mSoundFocus + 1) % 4;
}

void zayshowData::StopSound()
{
    for(sint32 i = 0; i < 4; ++i)
    {
        Platform::Sound::Close(mSounds[i]);
        mSounds[i] = nullptr;
    }
    mSoundFocus = 0;
}

void zayshowData::CallGate(chars gatename)
{
    if(mWidget)
        mWidget->JumpCall(gatename);
}

void zayshowData::TryPythonRecvOnce()
{
    while(0 < Platform::Socket::RecvAvailable(mPython))
    {
        uint08 RecvTemp[4096];
        const sint32 RecvSize = Platform::Socket::Recv(mPython, RecvTemp, 4096);
        if(0 < RecvSize)
            Memory::Copy(mPythonQueue.AtDumpingAdded(RecvSize), RecvTemp, RecvSize);
        else if(RecvSize < 0)
            return;

        sint32 QueueEndPos = 0;
        for(sint32 iend = mPythonQueue.Count(), i = iend - RecvSize; i < iend; ++i)
        {
            if(mPythonQueue[i] == '#')
            {
                // 스트링 읽기
                const String Packet((chars) &mPythonQueue[QueueEndPos], i - QueueEndPos);
                QueueEndPos = i + 1;
                if(0 < Packet.Length())
                {
                    const Strings Params = String::Split(Packet);
                    if(0 < Params.Count())
                    {
                        const String Type = Params[0];
                        branch;
                        jump(!Type.Compare("set")) OnPython_set(Params);
                        jump(!Type.Compare("get")) OnPython_get(Params);
                        jump(!Type.Compare("call")) OnPython_call(Params);
                    }
                }
                invalidate();
            }
        }
        if(0 < QueueEndPos)
            mPythonQueue.SubtractionSection(0, QueueEndPos);
    }
}

void zayshowData::OnPython_set(const Strings& params)
{
    if(2 < params.Count())
    {
        UpdateDom(params[1], params[2]);
        invalidate();
    }
}

void zayshowData::OnPython_get(const Strings& params)
{
    if(1 < params.Count())
    {
        const chars Key = (chars) params[1];
        if(Key[0] == 'd' && Key[1] == '.')
        {
            const String Value = ZayWidgetDOM::GetValue(&Key[2]).ToText();
            PythonSend(String::Format("put,%s", (chars) Value));
        }
    }
}

void zayshowData::OnPython_call(const Strings& params)
{
    if(1 < params.Count())
    {
        if(mWidget)
            mWidget->JumpCall(params[1]);
    }
}

void zayshowData::PythonSend(const String& comma_params)
{
    Platform::Socket::Send(mPython, (bytes)(chars) comma_params,
        comma_params.Length(), 3000, true);
}

String zayshowData::FindPythonKey(sint32 keycode)
{
    String KeyText;
    switch(keycode)
    {
    case 8: KeyText = "Backspace"; break;
    case 13: KeyText = "Enter"; break;
    case 16: KeyText = "Shift"; break;
    case 17: KeyText = "Ctrl"; break;
    case 18: KeyText = "Alt"; break;
    case 21: KeyText = "Hangul"; break;
    case 25: KeyText = "Hanja"; break;
    case 32: KeyText = "Space"; break;
    case 37: KeyText = "Left"; break;
    case 38: KeyText = "Up"; break;
    case 39: KeyText = "Right"; break;
    case 40: KeyText = "Down"; break;
    case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57:
        KeyText = '0' + (keycode - 48); break;
    case 65: case 66: case 67: case 68: case 69: case 70: case 71: case 72: case 73: case 74:
    case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84:
    case 85: case 86: case 87: case 88: case 89: case 90:
        KeyText = 'A' + (keycode - 65); break;
    case 112: case 113: case 114: case 115: case 116: case 117:
    case 118: case 119: case 120: case 121: case 122: case 123:
        KeyText = String::Format("F%d", 1 + (keycode - 112)); break;
    }
    return KeyText;
}
