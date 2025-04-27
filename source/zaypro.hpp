#pragma once
#include <service/boss_zay.hpp>
#include <service/boss_zayson.hpp>
#include <service/boss_zaywidget.hpp>
#include <element/boss_tween.hpp>
#include <zaybox.hpp>

class ZEFakeZaySon : public ZaySonInterface
{
public:
    class Component
    {
    public:
        Component() {mType = ZayExtend::ComponentType::Unknown;}
        ~Component() {}

    public:
        ZayExtend::ComponentType mType;
        String mName;
        Color mColor;
        String mColorRes;
        String mParamComments;
        String mInsideSamples;
    };

public:
    ZEFakeZaySon();
    ~ZEFakeZaySon();

public:
    static Color GetComponentColor(ZayExtend::ComponentType type, String& colorres);

public:
    void ResetBoxInfo();
    void SetViewAndDom(chars viewname, chars domheader) override;
    const String& ViewName() const override;
    const String& DomHeader() const override;
    ZaySonInterface& AddComponent(ZayExtend::ComponentType type, chars name,
        ZayExtend::ComponentCB cb, chars comments = nullptr, chars samples = nullptr) override;
    ZaySonInterface& AddGlue(chars name, ZayExtend::GlueCB cb) override;
    void JumpCall(chars gatename, sint32 runcount) override;
    void JumpCallWithArea(chars gatename, sint32 runcount, float x, float y, float w, float h) override;
    void JumpClear() override;

public:
    sint32 GetComponentCount() const;
    const Component& GetComponent(sint32 i) const;
    const Component* GetComponent(chars name) const;

private:
    Array<Component> mComponents;

public:
    sint32 mLastBoxID;
    sint32 mDraggingComponentID;
    Rect mDraggingComponentRect;
};

class ZEWidgetPipe
{
public:
    ZEWidgetPipe();
    ~ZEWidgetPipe();

private:
    void RunPipe();
    void StopPipe();

public:
    bool TickOnce();
    void ResetJsonPath(chars jsonpath);
    bool IsConnected() const;
    void SendToClient(chars json);
    void SetExpandDom(bool expand);
    void SetExpandAtlas(bool expand);
    void SetExpandLog(bool expand);

public:
    class Document
    {
    public:
        String mResult;
        String mFormula;
        uint64 mUpdateMsec {0};
    };
    class AtlasImage
    {
    public:
        Image mImage;
        String mAtlasPath; // C:/AllProjects/BossProjects/challengers/assets-rem/image/main_png
        rect128 mValidRect {0, 0, 0, 0};
        uint64 mUpdateMsec {0};
    };
    class LogElement
    {
    public:
        LogElement(chars text = "", sint32 groupid = -1)
        {
            mText = text;
            static sint32 LastID = -1;
            if(groupid == -1)
                mGroupID = (++LastID) << 4;
            else mGroupID = groupid;
            mValidValue = true;
            mNext = nullptr;
        }
        virtual ~LogElement()
        {
            LogElement* CurElement = nullptr;
            LogElement* NextElement = mNext;
            while(CurElement = NextElement)
            {
                NextElement = CurElement->mNext;
                CurElement->mNext = nullptr;
                delete CurElement;
            }
        }
    public:
        LogElement* Detach(chars text)
        {
            LogElement* CurElement = this;
            while(CurElement->mNext)
            {
                if(!CurElement->mNext->mText.Compare(text))
                {
                    LogElement* OldElement = CurElement->mNext;
                    CurElement->mNext = OldElement->mNext;
                    OldElement->mNext = nullptr;
                    return OldElement;
                }
                CurElement = CurElement->mNext;
            }
            return nullptr;
        }
        sint32 Attach(LogElement* element)
        {
            BOSS_ASSERT("잘못된 시나리오입니다", !element->mNext);
            element->mNext = mNext;
            mNext = element;
            return element->mGroupID;
        }
        void Remove(sint32 groupid)
        {
            LogElement* CurElement = this;
            while(CurElement->mNext)
            {
                if(CurElement->mNext->mGroupID == groupid)
                {
                    LogElement* RemoveElement = CurElement->mNext;
                    CurElement->mNext = RemoveElement->mNext;
                    RemoveElement->mNext = nullptr;
                    delete RemoveElement;
                }
                else CurElement = CurElement->mNext;
            }
            if(unfocusmap().Access(mGroupID))
                unfocusmap().Remove(mGroupID);
        }
        void RemoveAll()
        {
            LogElement* CurElement = this;
            while(CurElement->mNext)
            {
                LogElement* RemoveElement = CurElement->mNext;
                CurElement->mNext = RemoveElement->mNext;
                RemoveElement->mNext = nullptr;
                delete RemoveElement;
            }
            unfocusmap().Reset();
        }
        bool TickOnce()
        {
            bool NeedUpdate = false;
            LogElement* CurElement = this;
            while(CurElement = CurElement->mNext)
            {
                const bool NewValid = CurElement->valid();
                NeedUpdate |= (CurElement->mValidValue != NewValid);
                CurElement->mValidValue = NewValid;
            }
            return NeedUpdate;
        }
    public:
        virtual void Render(ZayPanel& panel, sint32 index) {}
    public:
        LogElement* next() const {return mNext;}
        Map<bool>& unfocusmap() const {static Map<bool> _; return _;}
        virtual bool valid() const {return true;}
    protected:
        String mText;
        sint32 mGroupID;
        bool mValidValue;
        LogElement* mNext;
    };

public:
    // Dom
    void SetDomFilter(chars text);
    void SetDomCount(sint32 count);
    // Atlas
    void SetAtlasFilter(chars text);
    void SetAtlasCount(sint32 count);
    // Log
    void AddLog(chars type, chars title, chars detail);
    void RemoveLog(sint32 groupid);
    sint32 GetCalcedLogCount();
    LogElement* GetLogElement();
    LogElement* NextLogElement(LogElement* element);

public:
    inline const String& jsonpath() const {return mLastJsonPath;}
    // Dom
    inline bool expanddom() const {return mExpandedDom;}
    inline const Map<Document>& dom() const {return mDom;}
    inline const String& domfilter() const {return mDomFilter;}
    inline sint32 domcount() const {return mDomCount;}
    // Atlas
    inline bool expandatlas() const {return mExpandedAtlas;}
    inline const Map<AtlasImage>& atlas() const {return mAtlas;}
    inline const String& atlasfilter() const {return mAtlasFilter;}
    inline sint32 atlascount() const {return mAtlasCount;}
    // Log
    inline bool expandlog() const {return mExpandedLog;}

private:
    id_pipe mPipe;
    String mLastJsonPath;
    bool mJsonPathUpdated;
    bool mConnected;
    // Dom
    bool mExpandedDom;
    Map<Document> mDom; // [변수명]
    String mDomFilter;
    sint32 mDomCount;
    // Atlas
    bool mExpandedAtlas;
    Map<AtlasImage> mAtlas; // [이미지명]
    String mAtlasFilter;
    sint32 mAtlasCount;
    // Log
    bool mExpandedLog;
    LogElement mLogTitleTop;
    LogElement mLogDetailTop;
    bool mLogTitleTurn;
};

class zayproData : public ZayObject
{
public:
    zayproData();
    ~zayproData();

public:
    void ResetBoxes();
    void NewProject();
    void FastSave();
    void LoadCore(const Context& json);
    void SaveCore(chars filename) const;

public:
    void RenderButton(ZayPanel& panel, chars name, Color color, ZayPanel::SubGestureCB cb);
    void RenderComponent(ZayPanel& panel, sint32 i, bool enable, bool blank);
    void RenderDomTab(ZayPanel& panel);
    void RenderAtlasTab(ZayPanel& panel);
    void RenderMiniMap(ZayPanel& panel);
    void RenderLogTab(ZayPanel& panel);
    void RenderScrollBar(ZayPanel& panel, chars name, sint32 content);
    void RenderTitleBar(ZayPanel& panel);
    void RenderOutline(ZayPanel& panel);

public:
    void SetCursor(CursorRole role);
    sint32 MoveNcLeft(const rect128& rect, sint32 addx);
    sint32 MoveNcTop(const rect128& rect, sint32 addy);
    sint32 MoveNcRight(const rect128& rect, sint32 addx);
    sint32 MoveNcBottom(const rect128& rect, sint32 addy);

public:
    String mBuildTag;
    ZEFakeZaySon mZaySonAPI;
    sint32 mDraggingHook;
    Tween1D mEasySaveEffect;
    ZEWidgetPipe mPipe;
    uint64 mShowCommentTagMsec;
    Size mWorkViewSize;
    Point mWorkViewDrag;
    Point mWorkViewScroll;
    sint32 mEditorDragBoxID;
    ZEZayBox::InputType mEditorDragInputType;
    sint32 mEditorDragInputIdx;
    bool mEditorDropIsSwap;
    sint32 mEditorDropBoxID;
    sint32 mEditorDropInputIdx;
    Color mCapturedColor;
    Tween1D mCapturedEffect;

public: // 윈도우상태
    static const sint32 mTitleHeight = 37;
    static const sint32 mMinWindowWidth = 1000;
    static const sint32 mMinWindowHeight = 600;
    String mWindowName;
    CursorRole mNowCursor;
    bool mNcLeftBorder;
    bool mNcTopBorder;
    bool mNcRightBorder;
    bool mNcBottomBorder;
};
