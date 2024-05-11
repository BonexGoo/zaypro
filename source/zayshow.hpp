#pragma once
#include <service/boss_zay.hpp>
#include <service/boss_zaywidget.hpp>

class zayshowData : public ZayObject
{
public:
    zayshowData();
    ~zayshowData();

public:
    void SetCursor(CursorRole role);
    sint32 MoveNcLeft(const rect128& rect, sint32 addx);
    sint32 MoveNcTop(const rect128& rect, sint32 addy);
    sint32 MoveNcRight(const rect128& rect, sint32 addx);
    sint32 MoveNcBottom(const rect128& rect, sint32 addy);
    void RenderWindowOutline(ZayPanel& panel);
    bool IsFullScreen();
    void SetFullScreen();
    void SetNormalWindow();
    void SetWidget(chars path);
    void InitWidget(ZayWidget& widget, chars name);
    void UpdateDom(chars key, chars value);
    void PlaySound(chars filename);
    void StopSound();
    void CallGate(chars gatename);

public: // 파이썬
    void TryPythonRecvOnce();
    void OnPython_set(const Strings& params);
    void OnPython_get(const Strings& params);
    void OnPython_call(const Strings& params);
    void PythonSend(const String& comma_params);
    String FindPythonKey(sint32 keycode);

public: // 윈도우
    static const sint32 mMinWindowWidth = 400;
    static const sint32 mMinWindowHeight = 400;
    CursorRole mNowCursor {CR_Arrow};
    bool mNcLeftBorder {false};
    bool mNcTopBorder {false};
    bool mNcRightBorder {false};
    bool mNcBottomBorder {false};
    bool mIsFullScreen {false};
    bool mIsWindowMoving {false};
    rect128 mSavedNormalRect {0, 0, 0, 0};
    size64 mSavedLobbySize {0, 0};

public: // 위젯
    ZayWidget* mWidget {nullptr};
    uint64 mWidgetUpdater {0};
    id_sound mSounds[4] {nullptr, nullptr, nullptr, nullptr};
    sint32 mSoundFocus {0};

public: // 파이썬통신
    id_socket mPython {nullptr};
    uint08s mPythonQueue;
};
