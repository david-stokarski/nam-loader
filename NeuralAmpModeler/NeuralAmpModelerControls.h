#pragma once

#include <algorithm> // std::min
#include <cmath> // std::round
#include <cstdio> // FILE, fclose
#include <sstream> // std::stringstream
#include <unordered_map> // std::unordered_map
#include "IControls.h"
#include "IPlugPaths.h"
#include "Colors.h"

#ifdef OS_WIN
  #include <Windows.h>
  #include <Shellapi.h>
#endif

#define PLUG() static_cast<PLUG_CLASS_NAME*>(GetDelegate())
#define NAM_KNOB_HEIGHT 120.0f
#define NAM_SWTICH_HEIGHT 50.0f

using namespace iplug;
using namespace igraphics;

enum class NAMBrowserState
{
  Empty, // when no file loaded, show "Get" button
  Loaded // when file loaded, show "Clear" button
};

// Where the corner button on the plugin (settings, close settings) goes
// :param rect: Rect for the whole plugin's UI
IRECT CornerButtonArea(const IRECT& rect)
{
  const auto mainArea = rect.GetPadded(-20);
  return mainArea.GetFromTRHC(50, 50).GetCentredInside(20, 20);
};

/// Full-window background: a soft vertical gradient with a subtle vignette.
class NAMBackgroundControl : public IControl
{
public:
  NAMBackgroundControl(const IRECT& bounds)
  : IControl(bounds)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    // Flat, solid canvas matching the header chrome. The amp faceplate PNG has a
    // transparent background, so this color shows through behind it.
    g.FillRect(PluginColors::CHROME, mRECT);
  }
};

/// A flat, minimally-rounded panel: a subtly-lifted fill and a single hairline
/// border. No shadow, no highlight — clean and modern.
class NAMPanelControl : public IControl
{
public:
  NAMPanelControl(const IRECT& bounds, float roundness = 4.f)
  : IControl(bounds)
  , mRoundness(roundness)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    g.FillRoundRect(PluginColors::PANEL, mRECT, mRoundness);
    g.DrawRoundRect(PluginColors::PANEL_BORDER, mRECT, mRoundness, &mBlend, 1.f);
  }

private:
  float mRoundness;
};

/// A flat chrome strip (top toolbar / bottom status bar) with a hairline on one edge.
class NAMStripControl : public IControl
{
public:
  NAMStripControl(const IRECT& bounds, bool hairlineOnBottom)
  : IControl(bounds)
  , mHairlineOnBottom(hairlineOnBottom)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(PluginColors::CHROME, mRECT);
    const IRECT line = mHairlineOnBottom ? mRECT.GetFromBottom(1.f) : mRECT.GetFromTop(1.f);
    g.FillRect(PluginColors::HAIRLINE, line);
  }

private:
  bool mHairlineOnBottom;
};

/// A thin vertical hairline used as a divider between control groups.
class NAMDividerControl : public IControl
{
public:
  NAMDividerControl(const IRECT& bounds)
  : IControl(bounds)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    const float x = mRECT.MW();
    g.PathClear();
    g.DrawLine(PluginColors::HAIRLINE, x, mRECT.T, x, mRECT.B, &mBlend, 1.f);
  }
};

/// A small glowing LED dot (purely decorative "active" indicator).
class NAMLEDControl : public IControl
{
public:
  NAMLEDControl(const IRECT& bounds, const IColor& color)
  : IControl(bounds)
  , mColor(color)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    const float cx = mRECT.MW(), cy = mRECT.MH();
    const float r = std::min(mRECT.W(), mRECT.H()) * 0.5f;
    // Glow
    g.PathClear();
    g.PathCircle(cx, cy, r * 2.2f);
    g.PathFill(IPattern::CreateRadialGradient(
      cx, cy, r * 2.2f, {{mColor.WithOpacity(0.45f), 0.f}, {COLOR_TRANSPARENT, 1.f}}));
    // Core
    g.FillCircle(mColor, cx, cy, r);
    g.FillCircle(COLOR_WHITE.WithOpacity(0.5f), cx - r * 0.25f, cy - r * 0.25f, r * 0.35f);
  }

private:
  IColor mColor;
};

class NAMSquareButtonControl : public ISVGButtonControl
{
public:
  NAMSquareButtonControl(const IRECT& bounds, IActionFunction af, const ISVG& svg)
  : ISVGButtonControl(bounds, af, svg, svg)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (mMouseIsOver)
      g.FillRoundRect(PluginColors::MOUSEOVER, mRECT, 2.f);

    ISVGButtonControl::Draw(g);
  }
};

class NAMCircleButtonControl : public ISVGButtonControl
{
public:
  NAMCircleButtonControl(const IRECT& bounds, IActionFunction af, const ISVG& svg)
  : ISVGButtonControl(bounds, af, svg, svg)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (mMouseIsOver)
      g.FillEllipse(PluginColors::MOUSEOVER, mRECT);

    ISVGButtonControl::Draw(g);
  }
};

/// Full-window dim layer; click dismisses (used for Slim overlay).
class NAMSlimOverlayBackdropControl : public IControl
{
public:
  NAMSlimOverlayBackdropControl(const IRECT& bounds, IActionFunction dismiss)
  : IControl(bounds, dismiss)
  , mDismiss(dismiss)
  {
  }

  void Draw(IGraphics& g) override { g.FillRect(COLOR_BLACK.WithOpacity(0.45f), mRECT); }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mDismiss)
      mDismiss(this);
  }

private:
  IActionFunction mDismiss;
};

/// Draws a bitmap scaled to fit its bounds (used for the amp faceplate art).
class NAMImageControl : public IControl, public IBitmapBase
{
public:
  NAMImageControl(const IRECT& bounds, const IBitmap& bitmap)
  : IControl(bounds)
  , IBitmapBase(bitmap)
  {
    mIgnoreMouse = true;
  }

  void OnRescale() override
  {
    if (mBitmap.IsValid())
      mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  void Draw(IGraphics& g) override { g.DrawFittedBitmap(mBitmap, mRECT); }
};

/// A knob rendered from a photorealistic bitmap that is rotated to indicate the
/// value. The source image's pointer sits at 6 o'clock, so a 180 degree offset
/// is applied. The bitmap is scaled to fill mRECT so it tracks UI resizing.
class NAMImageKnobControl : public IKnobControlBase, public IBitmapBase
{
public:
  // drawKnob: draw the knob bitmap (medium amp). When false, only the indicator
  // is drawn — used when the knob artwork is already baked into the faceplate.
  // indInner/indOuter: indicator length as a fraction of the control radius.
  NAMImageKnobControl(const IRECT& bounds, int paramIdx, const IBitmap& bitmap, bool drawKnob = true,
                      float indInner = 0.16f, float indOuter = 0.52f)
  : IKnobControlBase(bounds, paramIdx)
  , IBitmapBase(bitmap)
  , mDrawKnob(drawKnob)
  , mIndInner(indInner)
  , mIndOuter(indOuter)
  {
  }

  void OnRescale() override
  {
    if (mBitmap.IsValid())
      mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  // Reverse the scroll-wheel direction over the knob.
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    IKnobControlBase::OnMouseWheel(x, y, mod, -d);
  }

  void Draw(IGraphics& g) override
  {
    const float cx = mRECT.MW();
    const float cy = mRECT.MH();
    const float R = mRECT.W() * 0.5f;

    // 1) Knob artwork, drawn static (no rotation), scaled to fill mRECT.
    if (mDrawKnob && mBitmap.IsValid())
    {
      const float w = mBitmap.W() / mBitmap.GetDrawScale();
      const float h = mBitmap.H() / mBitmap.GetDrawScale();
      const float s = mRECT.W() / w;
      g.PathTransformSave();
      g.PathTransformTranslate(cx, cy);
      g.PathTransformScale(s);
      g.DrawBitmap(mBitmap, IRECT(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f), 0, 0, &mBlend);
      g.PathTransformRestore();
    }

    // 2) Indicator rendered on top, rotated to the value (-135..+135, 0 = up).
    const float angle = -135.0f + static_cast<float>(GetValue()) * 270.0f;
    const float ri = R * mIndInner;
    const float ro = R * mIndOuter;
    const float tk = std::max(1.6f, R * 0.075f);
    // Dark under-stroke for contrast, then a bright pointer.
    g.DrawRadialLine(COLOR_BLACK.WithOpacity(0.55f), cx, cy, angle, ri, ro, &mBlend, tk + 1.4f);
    g.DrawRadialLine(IColor(255, 246, 244, 235), cx, cy, angle, ri, ro, &mBlend, tk);
  }

private:
  bool mDrawKnob;
  float mIndInner, mIndOuter;
};

class NAMKnobControl : public IVKnobControl, public IBitmapBase
{
public:
  NAMKnobControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, IBitmap bitmap)
  : IVKnobControl(bounds, paramIdx, label, style, true)
  , IBitmapBase(bitmap)
  {
    mInnerPointerFrac = 0.55;
  }

  void OnRescale() override
  {
    if (mBitmap.IsValid())
      mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  // Reverse the scroll-wheel direction over the knob.
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    IKnobControlBase::OnMouseWheel(x, y, mod, -d);
  }

  // A flat, modern knob: a thin indicator arc, a subtly-lit dark face and a
  // clean pointer line. No skeuomorphic bitmap.
  void DrawWidget(IGraphics& g) override
  {
    const float d = std::min(mWidgetBounds.W(), mWidgetBounds.H());
    const IRECT knobRect = mWidgetBounds.GetCentredInside(d, d);
    const float cx = knobRect.MW();
    const float cy = knobRect.MH();
    const float r = d * 0.5f;
    const bool hover = mMouseIsOver;

    const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));

    // Indicator arc — thin full track then the filled value portion.
    const float arcR = r * 0.95f;
    const float arcThickness = 2.0f;
    g.DrawArc(PluginColors::ARC_TRACK, cx, cy, arcR, mAngle1, mAngle2, &mBlend, arcThickness);
    const IColor arcColor = hover ? PluginColors::ACCENT : PluginColors::ACCENT.WithOpacity(0.85f);
    g.DrawArc(arcColor, cx, cy, arcR, angle >= mAnchorAngle ? mAnchorAngle : angle,
              angle >= mAnchorAngle ? angle : mAnchorAngle, &mBlend, arcThickness);

    // Body — flat, single fill with a hairline rim. No gradient, no shadow.
    const float bodyR = r * 0.76f;
    g.FillCircle(PluginColors::KNOB_FACE_BOTTOM, cx, cy, bodyR, &mBlend);
    g.DrawCircle(PluginColors::KNOB_RIM, cx, cy, bodyR, &mBlend, 1.f);

    // Pointer line
    const IColor pointer = hover ? PluginColors::ACCENT : PluginColors::TEXT_HI;
    g.DrawRadialLine(pointer, cx, cy, angle, bodyR * 0.34f, bodyR * 0.82f, &mBlend, 2.0f);
  }
};

/// A clean, modern pill toggle: a rounded capsule track with a sliding knob.
/// Draws no text (so nothing can clip), and toggles the bound bool parameter on
/// click via ISwitchControlBase.
class NAMSwitchControl : public ISwitchControlBase
{
public:
  NAMSwitchControl(const IRECT& bounds, int paramIdx)
  : ISwitchControlBase(bounds, paramIdx)
  {
  }

  void Draw(IGraphics& g) override
  {
    // Fit a capsule inside the bounds
    const float h = std::min(mRECT.H(), 20.0f);
    const float w = std::min(mRECT.W(), h * 1.9f);
    const IRECT track = mRECT.GetCentredInside(w, h);
    const float r = h * 0.5f;
    const bool on = GetSelectedIdx() > 0;
    const float alpha = mDisabled ? 0.4f : 1.0f;

    // Track
    const IColor trackCol =
      on ? PluginColors::ACCENT_DIM.WithOpacity(alpha) : PluginColors::KNOB_RIM.WithOpacity(alpha);
    g.FillRoundRect(trackCol, track, r, &mBlend);
    if (!on)
      g.DrawRoundRect(PluginColors::PANEL_BORDER.WithOpacity(alpha), track, r, &mBlend, 1.0f);
    if (mMouseIsOver && !mDisabled)
      g.FillRoundRect(PluginColors::MOUSEOVER, track, r, &mBlend);

    // Knob
    const float knobR = r - 2.5f;
    const float cy = track.MH();
    const float cx = on ? (track.R - r) : (track.L + r);
    const IColor knobCol =
      on ? PluginColors::TEXT_HI.WithOpacity(alpha) : PluginColors::TEXT_MID.WithOpacity(alpha);
    g.FillCircle(knobCol, cx, cy, knobR, &mBlend);
  }
};

class NAMFileNameControl : public IVButtonControl
{
public:
  NAMFileNameControl(const IRECT& bounds, const char* label, const IVStyle& style)
  : IVButtonControl(bounds, DefaultClickActionFunc, label, style)
  {
  }

  void SetLabelAndTooltip(const char* str)
  {
    SetLabelStr(str);
    SetTooltip(str);
  }

  void SetLabelAndTooltipEllipsizing(const WDL_String& fileName)
  {
    auto EllipsizeFilePath = [](const char* filePath, size_t prefixLength, size_t suffixLength, size_t maxLength) {
      const std::string ellipses = "...";
      assert(maxLength <= (prefixLength + suffixLength + ellipses.size()));
      std::string str{filePath};

      if (str.length() <= maxLength)
      {
        return str;
      }
      else
      {
        return str.substr(0, prefixLength) + ellipses + str.substr(str.length() - suffixLength);
      }
    };

    auto ellipsizedFileName = EllipsizeFilePath(fileName.get_filepart(), 22, 22, 45);
    SetLabelStr(ellipsizedFileName.c_str());
    SetTooltip(fileName.get_filepart());
  }
};

// URL control for the "Get" models/irs links
class NAMGetButtonControl : public NAMSquareButtonControl
{
public:
  NAMGetButtonControl(const IRECT& bounds, const char* label, const char* url, const ISVG& globeSVG)
  : NAMSquareButtonControl(
      bounds,
      [url](IControl* pCaller) {
        WDL_String fullURL(url);
        pCaller->GetUI()->OpenURL(fullURL.Get());
      },
      globeSVG)
  {
    SetTooltip(label);
  }
};

class NAMFileBrowserControl : public IDirBrowseControlBase
{
public:
  NAMFileBrowserControl(const IRECT& bounds, int clearMsgTag, const char* labelStr, const char* fileExtension,
                        IFileDialogCompletionHandlerFunc ch, const IVStyle& style, const ISVG& loadSVG,
                        const ISVG& clearSVG, const ISVG& leftSVG, const ISVG& rightSVG, const IBitmap& bitmap,
                        const ISVG& globeSVG, const char* getButtonLabel, const char* getButtonURL,
                        bool compact = false)
  : IDirBrowseControlBase(bounds, fileExtension, false, false)
  , mClearMsgTag(clearMsgTag)
  , mDefaultLabelStr(labelStr)
  , mCompletionHandlerFunc(ch)
  , mStyle(style.WithColor(kFG, COLOR_TRANSPARENT).WithDrawFrame(false))
  , mBitmap(bitmap)
  , mLoadSVG(loadSVG)
  , mClearSVG(clearSVG)
  , mLeftSVG(leftSVG)
  , mRightSVG(rightSVG)
  , mGlobeSVG(globeSVG)
  , mGetButtonLabel(getButtonLabel)
  , mGetButtonURL(getButtonURL)
  , mBrowserState(NAMBrowserState::Empty)
  , mCompact(compact)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    // Flat field.
    g.FillRoundRect(PluginColors::KNOB_RIM, mRECT, 4.0f);
    g.DrawRoundRect(PluginColors::PANEL_BORDER, mRECT, 4.0f, &mBlend, 1.0f);

    if (mCompact)
    {
      // Draw a small chevron on the right to read as a dropdown.
      const float cx = mRECT.R - 13.f;
      const float cy = mRECT.MH();
      g.DrawLine(PluginColors::TEXT_MID, cx - 4.f, cy - 2.f, cx, cy + 2.5f, &mBlend, 1.4f);
      g.DrawLine(PluginColors::TEXT_MID, cx + 4.f, cy - 2.f, cx, cy + 2.5f, &mBlend, 1.4f);
    }
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        mSelectedItemIndex = mItems.Find(pItem);
        LoadFileAtCurrentIndex();
      }
    }
  }

  void OnAttached() override
  {
    auto prevFileFunc = [&](IControl* pCaller) {
      const auto nItems = NItems();
      if (nItems == 0)
        return;
      mSelectedItemIndex--;

      if (mSelectedItemIndex < 0)
        mSelectedItemIndex = nItems - 1;

      LoadFileAtCurrentIndex();
    };

    auto nextFileFunc = [&](IControl* pCaller) {
      const auto nItems = NItems();
      if (nItems == 0)
        return;
      mSelectedItemIndex++;

      if (mSelectedItemIndex >= nItems)
        mSelectedItemIndex = 0;

      LoadFileAtCurrentIndex();
    };

    auto loadFileFunc = [&](IControl* pCaller) {
      WDL_String fileName;
      WDL_String path;
      GetSelectedFileDirectory(path);
#ifdef NAM_PICK_DIRECTORY
      pCaller->GetUI()->PromptForDirectory(path, [&](const WDL_String& fileName, const WDL_String& path) {
        if (path.GetLength())
        {
          ClearPathList();
          AddPath(path.Get(), "");
          SetupMenu();
          SelectFirstFile();
          LoadFileAtCurrentIndex();
        }
      });
#else
      pCaller->GetUI()->PromptForFile(
        fileName, path, EFileAction::Open, mExtension.Get(), [&](const WDL_String& fileName, const WDL_String& path) {
          if (fileName.GetLength())
          {
            ClearPathList();
            AddPath(path.Get(), "");
            SetupMenu();
            SetSelectedFile(fileName.Get());
            LoadFileAtCurrentIndex();
          }
        });
#endif
    };

    auto clearFileFunc = [&](IControl* pCaller) {
      pCaller->GetDelegate()->SendArbitraryMsgFromUI(mClearMsgTag);
      mFileNameControl->SetLabelAndTooltip(mDefaultLabelStr.Get());
      SetBrowserState(NAMBrowserState::Empty);
      // FIXME disabling output mode...
      //      pCaller->GetUI()->GetControlWithTag(kCtrlTagOutputMode)->SetDisabled(false);
    };

    auto chooseFileFunc = [&, loadFileFunc](IControl* pCaller) {
      if (std::string_view(pCaller->As<IVButtonControl>()->GetLabelStr()) == mDefaultLabelStr.Get())
      {
        loadFileFunc(pCaller);
      }
      else
      {
        CheckSelectedItem();

        if (!mMainMenu.HasSubMenus())
        {
          mMainMenu.SetChosenItemIdx(mSelectedItemIndex);
        }
        pCaller->GetUI()->CreatePopupMenu(*this, mMainMenu, pCaller->GetRECT());
      }
    };

    if (mCompact)
    {
      // Dropdown style: just the file-name field (click = load or choose),
      // with room for a chevron on the right. No load/prev/next/clear/get icons.
      const IRECT padded = mRECT.GetPadded(-3.f).GetReducedFromLeft(6.f).GetReducedFromRight(20.f);
      AddChildControl(mFileNameControl = new NAMFileNameControl(padded, mDefaultLabelStr.Get(), mStyle))
        ->SetAnimationEndActionFunction(chooseFileFunc);
      mClearButton = nullptr;
      mGetButton = nullptr;
      SetBrowserState(NAMBrowserState::Empty);
      return;
    }

    IRECT padded = mRECT.GetPadded(-6.f).GetHPadded(-2.f);
    const auto buttonWidth = padded.H();
    const auto loadFileButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto clearAndGetButtonBounds = padded.ReduceFromRight(buttonWidth);
    const auto leftButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto rightButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto fileNameButtonBounds = padded;

    AddChildControl(new NAMSquareButtonControl(loadFileButtonBounds, DefaultClickActionFunc, mLoadSVG))
      ->SetAnimationEndActionFunction(loadFileFunc);
    AddChildControl(new NAMSquareButtonControl(leftButtonBounds, DefaultClickActionFunc, mLeftSVG))
      ->SetAnimationEndActionFunction(prevFileFunc);
    AddChildControl(new NAMSquareButtonControl(rightButtonBounds, DefaultClickActionFunc, mRightSVG))
      ->SetAnimationEndActionFunction(nextFileFunc);
    AddChildControl(mFileNameControl = new NAMFileNameControl(fileNameButtonBounds, mDefaultLabelStr.Get(), mStyle))
      ->SetAnimationEndActionFunction(chooseFileFunc);

    // creates both right-side controls but only show one based on state
    mClearButton = new NAMSquareButtonControl(clearAndGetButtonBounds, DefaultClickActionFunc, mClearSVG);
    mClearButton->SetAnimationEndActionFunction(clearFileFunc);
    AddChildControl(mClearButton);

    mGetButton = new NAMGetButtonControl(clearAndGetButtonBounds, mGetButtonLabel, mGetButtonURL, mGlobeSVG);
    AddChildControl(mGetButton);

    // initialize control visibility
    SetBrowserState(NAMBrowserState::Empty);
  }

  void LoadFileAtCurrentIndex()
  {
    if (mSelectedItemIndex > -1 && mSelectedItemIndex < NItems())
    {
      WDL_String fileName, path;
      GetSelectedFile(fileName);
      mFileNameControl->SetLabelAndTooltipEllipsizing(fileName);
      mCompletionHandlerFunc(fileName, path);
    }
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    switch (msgTag)
    {
      case kMsgTagLoadFailed:
        // Honestly, not sure why I made a big stink of it before. Why not just say it failed and move on? :)
        {
          std::string label(std::string("(FAILED) ") + std::string(mFileNameControl->GetLabelStr()));
          mFileNameControl->SetLabelAndTooltip(label.c_str());
          SetBrowserState(NAMBrowserState::Empty);
        }
        break;
      case kMsgTagLoadedModel:
      case kMsgTagLoadedIR:
      {
        WDL_String fileName, directory;
        fileName.Set(reinterpret_cast<const char*>(pData));
        directory.Set(reinterpret_cast<const char*>(pData));
        directory.remove_filepart(true);

        ClearPathList();
        AddPath(directory.Get(), "");
        SetupMenu();
        SetSelectedFile(fileName.Get());
        mFileNameControl->SetLabelAndTooltipEllipsizing(fileName);
        SetBrowserState(NAMBrowserState::Loaded);
      }
      break;
      default: break;
    }
  }

private:
  void SelectFirstFile() { mSelectedItemIndex = mFiles.GetSize() ? 0 : -1; }

  void GetSelectedFileDirectory(WDL_String& path)
  {
    GetSelectedFile(path);
    path.remove_filepart();
    return;
  }

  // set the state of the browser and the visibility of the "Get" vs. "Clear" buttons
  void SetBrowserState(NAMBrowserState newState)
  {
    mBrowserState = newState;

    if (mClearButton == nullptr || mGetButton == nullptr)
      return; // compact/dropdown mode has no clear/get buttons

    switch (mBrowserState)
    {
      case NAMBrowserState::Empty:
        mClearButton->Hide(true);
        mGetButton->Hide(false);
        break;
      case NAMBrowserState::Loaded:
        mClearButton->Hide(false);
        mGetButton->Hide(true);
        break;
    }
  }

  WDL_String mDefaultLabelStr;
  IFileDialogCompletionHandlerFunc mCompletionHandlerFunc;
  NAMFileNameControl* mFileNameControl = nullptr;
  IVStyle mStyle;
  IBitmap mBitmap;
  ISVG mLoadSVG, mClearSVG, mLeftSVG, mRightSVG, mGlobeSVG;
  int mClearMsgTag;

  // new members for the "Get" button
  const char* mGetButtonLabel;
  const char* mGetButtonURL;
  NAMBrowserState mBrowserState;
  NAMSquareButtonControl* mClearButton = nullptr;
  NAMGetButtonControl* mGetButton = nullptr;
  bool mCompact = false;
};

class NAMMeterControl : public IVPeakAvgMeterControl<>, public IBitmapBase
{
  static constexpr float KMeterMin = -70.0f;
  static constexpr float KMeterMax = -0.01f;

public:
  NAMMeterControl(const IRECT& bounds, const IBitmap& bitmap, const IVStyle& style)
  : IVPeakAvgMeterControl<>(bounds, "", style.WithShowValue(false).WithDrawFrame(false).WithWidgetFrac(1.0f),
                            EDirection::Vertical, {}, 0, KMeterMin, KMeterMax, {})
  , IBitmapBase(bitmap)
  {
    SetPeakSize(1.0f);
  }

  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    // Fill the whole bounds — no extra top/bottom or side padding.
    mWidgetBounds = mRECT;
    MakeTrackRects(mWidgetBounds);
    MakeStepRects(mWidgetBounds, mNSteps);
    SetDirty(false);
  }

  // Flat, borderless track: a slim rounded gray bar.
  void DrawBackground(IGraphics& g, const IRECT& r) override
  {
    g.FillRoundRect(IColor(255, 52, 53, 58), r, r.W() * 0.5f);
  }

  void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    if (r.H() > 1)
      g.FillRoundRect(PluginColors::TEXT_HI, r, r.W() * 0.5f, &mBlend);
  }

  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    g.FillRect(COLOR_WHITE, r, &mBlend);
  }
};

// Container where we can refer to children by names instead of indices
class IContainerBaseWithNamedChildren : public IContainerBase
{
public:
  IContainerBaseWithNamedChildren(const IRECT& bounds)
  : IContainerBase(bounds) {};
  ~IContainerBaseWithNamedChildren() = default;

protected:
  IControl* AddNamedChildControl(IControl* control, std::string name, int ctrlTag = kNoTag, const char* group = "")
  {
    // Make sure we haven't already used this name
    assert(mChildNameIndexMap.find(name) == mChildNameIndexMap.end());
    mChildNameIndexMap[name] = NChildren();
    return AddChildControl(control, ctrlTag, group);
  };

  IControl* GetNamedChild(std::string name)
  {
    const int index = mChildNameIndexMap[name];
    return GetChild(index);
  };


private:
  std::unordered_map<std::string, int> mChildNameIndexMap;
}; // class IContainerBaseWithNamedChildren


struct PossiblyKnownParameter
{
  bool known = false;
  double value = 0.0;
};

struct ModelInfo
{
  PossiblyKnownParameter sampleRate;
  PossiblyKnownParameter inputCalibrationLevel;
  PossiblyKnownParameter outputCalibrationLevel;
};

class ModelInfoControl : public IContainerBaseWithNamedChildren
{
public:
  ModelInfoControl(const IRECT& bounds, const IVStyle& style)
  : IContainerBaseWithNamedChildren(bounds)
  , mStyle(style) {};

  void ClearModelInfo()
  {
    static_cast<IVLabelControl*>(GetNamedChild(mControlNames.sampleRate))->SetStr("");
    mHasInfo = false;
  };

  void Hide(bool hide) override
  {
    // Don't show me unless I have info to show!
    IContainerBase::Hide(hide || (!mHasInfo));
  };

  void OnAttached() override
  {
    AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(4, 0), "Model information:", mStyle));
    AddNamedChildControl(new IVLabelControl(GetRECT().SubRectVertical(4, 1), "", mStyle), mControlNames.sampleRate);
    // AddNamedChildControl(
    //   new IVLabelControl(GetRECT().SubRectVertical(4, 2), "", mStyle), mControlNames.inputCalibrationLevel);
    // AddNamedChildControl(
    //   new IVLabelControl(GetRECT().SubRectVertical(4, 3), "", mStyle), mControlNames.outputCalibrationLevel);
  };

  void SetModelInfo(const ModelInfo& modelInfo)
  {
    auto SetControlStr = [&](const std::string& name, const PossiblyKnownParameter& p, const std::string& units,
                             const std::string& childName) {
      std::stringstream ss;
      ss << name << ": ";
      if (p.known)
      {
        ss << p.value << " " << units;
      }
      else
      {
        ss << "(Unknown)";
      }
      static_cast<IVLabelControl*>(GetNamedChild(childName))->SetStr(ss.str().c_str());
    };

    SetControlStr("Sample rate", modelInfo.sampleRate, "Hz", mControlNames.sampleRate);
    // SetControlStr(
    //   "Input calibration level", modelInfo.inputCalibrationLevel, "dBu", mControlNames.inputCalibrationLevel);
    // SetControlStr(
    //   "Output calibration level", modelInfo.outputCalibrationLevel, "dBu", mControlNames.outputCalibrationLevel);

    mHasInfo = true;
  };

private:
  const IVStyle mStyle;
  struct
  {
    const std::string sampleRate = "sampleRate";
    // const std::string inputCalibrationLevel = "inputCalibrationLevel";
    // const std::string outputCalibrationLevel = "outputCalibrationLevel";
  } mControlNames;
  // Do I have info?
  bool mHasInfo = false;
};

class OutputModeControl : public IVRadioButtonControl
{
public:
  OutputModeControl(const IRECT& bounds, int paramIdx, const IVStyle& style, float buttonSize)
  : IVRadioButtonControl(bounds, paramIdx, {}, "", style, EVShape::Ellipse, EDirection::Vertical, buttonSize) {};

  void SetNormalizedDisable(const bool disable)
  {
    // HACK non-DRY string and hard-coded indices
    std::stringstream ss;
    ss << "Normalized";
    if (disable)
    {
      ss << " [Not supported by model]";
    }
    mTabLabels.Get(1)->Set(ss.str().c_str());
  };
  void SetCalibratedDisable(const bool disable)
  {
    // HACK non-DRY string and hard-coded indices
    std::stringstream ss;
    ss << "Calibrated";
    if (disable)
    {
      ss << " [Not supported by model]";
    }
    mTabLabels.Get(2)->Set(ss.str().c_str());
  };
};

class NAMSettingsPageControl : public IContainerBaseWithNamedChildren
{
public:
  NAMSettingsPageControl(const IRECT& bounds, const IBitmap& bitmap, const IBitmap& inputLevelBackgroundBitmap,
                         const IBitmap& switchBitmap, ISVG closeSVG, const IVStyle& style,
                         const IVStyle& radioButtonStyle)
  : IContainerBaseWithNamedChildren(bounds)
  , mAnimationTime(0)
  , mBitmap(bitmap)
  , mInputLevelBackgroundBitmap(inputLevelBackgroundBitmap)
  , mSwitchBitmap(switchBitmap)
  , mStyle(style)
  , mRadioButtonStyle(radioButtonStyle)
  , mCloseSVG(closeSVG)
  {
    mIgnoreMouse = false;
  }

  void ClearModelInfo()
  {
    auto* modelInfoControl = static_cast<ModelInfoControl*>(GetNamedChild(mControlNames.modelInfo));
    assert(modelInfoControl != nullptr);
    modelInfoControl->ClearModelInfo();
  }

  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    if (key.VK == kVK_ESCAPE)
    {
      HideAnimated(true);
      return true;
    }

    return false;
  }

  void HideAnimated(bool hide)
  {
    mWillHide = hide;

    if (hide == false)
    {
      mHide = false;
    }
    else // hide subcontrols immediately
    {
      ForAllChildrenFunc([hide](int childIdx, IControl* pChild) { pChild->Hide(hide); });
    }

    SetAnimation(
      [&](IControl* pCaller) {
        auto progress = static_cast<float>(pCaller->GetAnimationProgress());

        if (mWillHide)
          SetBlend(IBlend(EBlend::Default, 1.0f - progress));
        else
          SetBlend(IBlend(EBlend::Default, progress));

        if (progress > 1.0f)
        {
          pCaller->OnEndAnimation();
          IContainerBase::Hide(mWillHide);
          GetUI()->SetAllControlsDirty();
          return;
        }
      },
      mAnimationTime);

    SetDirty(true);
  }

  // The centered card. mRECT spans the whole window so the backdrop can dim
  // everything behind and swallow clicks; the popover itself is this sub-rect.
  IRECT PopoverRect() const { return GetRECT().GetCentredInside(kPopoverW, kPopoverH); }

  void Draw(IGraphics& g) override
  {
    // Dim the window behind the popover (fades with the container's blend).
    g.FillRect(COLOR_BLACK.WithOpacity(0.55f), mRECT, &mBlend);

    const IRECT card = PopoverRect();
    // Soft drop shadow, then the card fill + hairline border.
    g.FillRoundRect(COLOR_BLACK.WithOpacity(0.30f), card.GetPadded(3.f).GetVShifted(5.f), 14.f, &mBlend);
    g.FillRoundRect(PluginColors::PANEL, card, 14.f, &mBlend);
    g.DrawRoundRect(PluginColors::PANEL_BORDER, card, 14.f, &mBlend, 1.f);

    // Section dividers (kept in sync with the layout in OnAttached()).
    const IRECT inner = card.GetPadded(-kPopoverPad);
    const float dyTitle = inner.T + kTitleH + 9.f;
    g.DrawLine(PluginColors::HAIRLINE, inner.L, dyTitle, inner.R, dyTitle, &mBlend, 1.f);
    const float dyAbout = inner.B - kAboutH - 8.f;
    g.DrawLine(PluginColors::HAIRLINE, inner.L, dyAbout, inner.R, dyAbout, &mBlend, 1.f);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    // Clicking the dimmed area outside the card dismisses.
    if (!PopoverRect().Contains(x, y))
      HideAnimated(true);
  }

  void OnAttached() override
  {
    const IRECT card = PopoverRect();
    const IRECT inner = card.GetPadded(-kPopoverPad);

    const auto bodyText = IText(DEFAULT_TEXT_SIZE, EAlign::Center, PluginColors::HELP_TEXT);
    const auto leftText = bodyText.WithAlign(EAlign::Near);
    const auto baseStyle = mStyle.WithDrawFrame(false).WithValueText(bodyText);
    const IVStyle leftStyle = baseStyle.WithValueText(leftText);
    // Radios get section headers of their own, so suppress the built-in group label.
    const IVStyle radioStyle = mRadioButtonStyle.WithShowLabel(false);

    const IVStyle titleStyle =
      DEFAULT_STYLE
        .WithValueText(
          IText(19.f, PluginColors::TEXT_HI, "Michroma-Regular").WithAlign(EAlign::Near).WithVAlign(EVAlign::Middle))
        .WithDrawFrame(false)
        .WithShadowOffset(0.f);
    const IVStyle sectionStyle =
      DEFAULT_STYLE.WithDrawFrame(false).WithValueText(
        IText(10.f, PluginColors::TEXT_MID, "Michroma-Regular").WithAlign(EAlign::Near).WithVAlign(EVAlign::Middle));

    auto sectionLabel = [&](const IRECT& r, const char* str) {
      AddChildControl(new IVLabelControl(r, str, sectionStyle));
    };

    // --- Title bar + close button ---
    const IRECT titleRow = inner.GetFromTop(kTitleH);
    AddNamedChildControl(new IVLabelControl(titleRow, "Settings", titleStyle), mControlNames.title);
    auto closeAction = [&](IControl* pCaller) {
      static_cast<NAMSettingsPageControl*>(pCaller->GetParent())->HideAnimated(true);
    };
    AddNamedChildControl(
      new NAMSquareButtonControl(titleRow.GetFromRight(20.f).GetCentredInside(18.f), closeAction, mCloseSVG),
      mControlNames.close);

    // Content sits below the title divider and above the About footer.
    const IRECT content = inner.GetReducedFromTop(kTitleH + 18.f).GetReducedFromBottom(kAboutH + 16.f);
    const float colGap = 28.f;
    const float colW = (content.W() - colGap) * 0.5f;
    const IRECT leftCol = content.GetFromLeft(colW);
    const IRECT rightCol = content.GetFromRight(colW);

    const float labelH = 14.f;
    const float sectionGap = 22.f;

    // --- Left column: input calibration + output mode ---
    {
      float y = leftCol.T;
      sectionLabel(IRECT(leftCol.L, y, leftCol.R, y + labelH), "INPUT CALIBRATION");
      y += labelH + 6.f;

      const IRECT fieldArea = IRECT(leftCol.L, y, leftCol.R, y + 28.f);
      auto* inputLevelControl = AddNamedChildControl(
        new InputLevelControl(fieldArea, kInputCalibrationLevel, mInputLevelBackgroundBitmap, bodyText),
        mControlNames.inputCalibrationLevel, kCtrlTagInputCalibrationLevel);
      inputLevelControl->SetTooltip(
        "The analog level, in dBu RMS, that corresponds to digital level of 0 dBFS peak in the host as its signal "
        "enters this plugin.");
      y += 28.f + 12.f;

      const IRECT calRow = IRECT(leftCol.L, y, leftCol.R, y + 24.f);
      AddChildControl(new IVLabelControl(calRow.GetReducedFromRight(52.f), "Calibrate Input", leftStyle));
      AddNamedChildControl(new NAMSwitchControl(calRow.GetFromRight(46.f).GetCentredInside(46.f, 22.f), kCalibrateInput),
                           mControlNames.calibrateInput, kCtrlTagCalibrateInput);
      y += 24.f + sectionGap;

      sectionLabel(IRECT(leftCol.L, y, leftCol.R, y + labelH), "OUTPUT MODE");
      y += labelH + 6.f;
      const IRECT outputRadioArea = IRECT(leftCol.L, y, leftCol.R, y + 90.f);
      auto* outputModeControl =
        AddNamedChildControl(new OutputModeControl(outputRadioArea, kOutputMode, radioStyle, 9.f),
                             mControlNames.outputMode, kCtrlTagOutputMode);
      outputModeControl->SetTooltip(
        "How to adjust the level of the output.\nRaw=No adjustment.\nNormalized=Adjust the level so that all models "
        "are about the same loudness.\nCalibrated=Match the input's digital-analog calibration.");
    }

    // --- Right column: amp model + model info ---
    {
      float y = rightCol.T;
      sectionLabel(IRECT(rightCol.L, y, rightCol.R, y + labelH), "AMP MODEL");
      y += labelH + 6.f;
      const IRECT ampArea = IRECT(rightCol.L, y, rightCol.R, y + 90.f);
      auto* ampControl = AddNamedChildControl(
        new IVRadioButtonControl(ampArea, kAmpType, {}, "", radioStyle, EVShape::Ellipse, EDirection::Vertical, 9.f),
        "ampType", kCtrlTagAmpType);
      ampControl->SetTooltip("Choose the amp's look. Medium Gain and Modern Gain use different faceplates.");
      y += 90.f + sectionGap;

      const IRECT modelInfoArea = IRECT(rightCol.L, y, rightCol.R, y + 4 * 15.f);
      AddNamedChildControl(new ModelInfoControl(modelInfoArea, leftStyle), mControlNames.modelInfo);
    }

    // --- Footer: about / credits ---
    const IRECT aboutArea = inner.GetFromBottom(kAboutH);
    AddNamedChildControl(new AboutControl(aboutArea, leftStyle, leftText), mControlNames.about);

    OnResize();
  }

  void SetModelInfo(const ModelInfo& modelInfo)
  {
    auto* modelInfoControl = static_cast<ModelInfoControl*>(GetNamedChild(mControlNames.modelInfo));
    assert(modelInfoControl != nullptr);
    modelInfoControl->SetModelInfo(modelInfo);
  };

private:
  IBitmap mBitmap;
  IBitmap mInputLevelBackgroundBitmap;
  IBitmap mSwitchBitmap;
  IVStyle mStyle;
  IVStyle mRadioButtonStyle;
  ISVG mCloseSVG;
  int mAnimationTime = 200;
  bool mWillHide = false;

  // Popover geometry (the card is centered inside the full-window bounds).
  static constexpr float kPopoverW = 500.f;
  static constexpr float kPopoverH = 448.f;
  static constexpr float kPopoverPad = 26.f;
  static constexpr float kTitleH = 28.f;
  static constexpr float kAboutH = 80.f;

  // Names for controls
  // Make sure that these are all unique and that you use them with AddNamedChildControl
  struct ControlNames
  {
    const std::string about = "About";
    const std::string bitmap = "Bitmap";
    const std::string calibrateInput = "CalibrateInput";
    const std::string close = "Close";
    const std::string inputCalibrationLevel = "InputCalibrationLevel";
    const std::string modelInfo = "ModelInfo";
    const std::string outputMode = "OutputMode";
    const std::string title = "Title";
  } mControlNames;

  class InputLevelControl : public IEditableTextControl
  {
  public:
    InputLevelControl(const IRECT& bounds, int paramIdx, const IBitmap& bitmap, const IText& text = DEFAULT_TEXT,
                      const IColor& BGColor = DEFAULT_BGCOLOR)
    : IEditableTextControl(bounds, "", text, BGColor)
    , mBitmap(bitmap)
    {
      SetParamIdx(paramIdx);
    };

    void Draw(IGraphics& g) override
    {
      g.DrawFittedBitmap(mBitmap, mRECT);
      ITextControl::Draw(g);
    };

    void SetValueFromUserInput(double normalizedValue, int valIdx) override
    {
      IControl::SetValueFromUserInput(normalizedValue, valIdx);
      const std::string s = ConvertToString(normalizedValue);
      OnTextEntryCompletion(s.c_str(), valIdx);
    };

    void SetValueFromDelegate(double normalizedValue, int valIdx) override
    {
      IControl::SetValueFromDelegate(normalizedValue, valIdx);
      const std::string s = ConvertToString(normalizedValue);
      SetStr(s.c_str());
      SetDirty(false);
    };

  private:
    std::string ConvertToString(const double normalizedValue)
    {
      const double naturalValue = GetParam()->FromNormalized(normalizedValue);
      // And make the value to display
      std::stringstream ss;
      ss << naturalValue << " dBu";
      std::string s = ss.str();
      return s;
    };

    IBitmap mBitmap;
  };

  class AboutControl : public IContainerBase
  {
  public:
    AboutControl(const IRECT& bounds, const IVStyle& style, const IText& text)
    : IContainerBase(bounds)
    , mStyle(style)
    , mText(text) {};

    void OnAttached() override
    {
      WDL_String verStr, buildInfoStr;
      PLUG()->GetPluginVersionStr(verStr);

      buildInfoStr.SetFormatted(100, "Version %s %s %s", verStr.Get(), PLUG()->GetArchStr(), PLUG()->GetAPIStr());

      AddChildControl(new IURLControl(GetRECT().SubRectVertical(5, 0), "NEURAL AMP MODELER",
                                      "https://www.neuralampmodeler.com", mText, COLOR_TRANSPARENT,
                                      PluginColors::HELP_TEXT_MO, PluginColors::HELP_TEXT_CLICKED));
      AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(5, 1), "By Steven Atkinson", mStyle));
      AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(5, 2), buildInfoStr.Get(), mStyle));
      AddChildControl(new IURLControl(GetRECT().SubRectVertical(5, 3),
                                      "Plug-in development: Steve Atkinson, Oli Larkin, ... ",
                                      "https://github.com/sdatkinson/NeuralAmpModelerPlugin/graphs/contributors", mText,
                                      COLOR_TRANSPARENT, PluginColors::HELP_TEXT_MO, PluginColors::HELP_TEXT_CLICKED));
      AddChildControl(new ThirdPartyNoticesControl(GetRECT().SubRectVertical(5, 4), mText));
    };

  private:
    class ThirdPartyNoticesControl : public IURLControl
    {
    public:
      ThirdPartyNoticesControl(const IRECT& bounds, const IText& text)
      : IURLControl(bounds, "Third party notices", "", text, COLOR_TRANSPARENT, PluginColors::HELP_TEXT_MO,
                    PluginColors::HELP_TEXT_CLICKED)
      {
      }

      void OnMouseDown(float x, float y, const IMouseMod& mod) override
      {
        WDL_String path;
        bool opened = false;

        if (ResolveNoticesPath(GetUI(), path))
          opened = OpenNoticesPath(GetUI(), path);

        if (!opened)
          ShowOpenError(GetUI());

        GetUI()->ReleaseMouseCapture();
        mClicked = true;
        SetDirty(false);
      }

    private:
      static bool FileExists(const WDL_String& path)
      {
        if (!CStringHasContents(path.Get()))
          return false;

        FILE* file = WDL_fopenA(path.Get(), "rb");
        if (file == nullptr)
          return false;

        fclose(file);
        return true;
      }

      static bool TryNoticePathInDirectory(WDL_String& result, const WDL_String& directory)
      {
        if (!CStringHasContents(directory.Get()))
          return false;

        WDL_String candidate(directory);
        const char lastChar = candidate.Get()[candidate.GetLength() - 1];

        if (!WDL_IS_DIRCHAR(lastChar))
          candidate.Append(WDL_DIRCHAR_STR);

        candidate.Append(kNoticesFileName);

        if (!FileExists(candidate))
          return false;

        result.Set(candidate.Get());
        return true;
      }

      // AAX (and similar) load the binary from Contents\x64 or Contents\Win32 while notices live in
      // Contents\Resources. Same layout as a VST3 bundle; this path is not covered by BundleResourcePath
      // when the plug-in is built as AAX_API only (no VST3_API).
      static bool TryNoticePathSiblingResources(WDL_String& result, const WDL_String& moduleDirectory)
      {
        if (!CStringHasContents(moduleDirectory.Get()))
          return false;

        WDL_String candidate(moduleDirectory);
        const char lastChar = candidate.Get()[candidate.GetLength() - 1];
        if (!WDL_IS_DIRCHAR(lastChar))
          candidate.Append(WDL_DIRCHAR_STR);

        candidate.Append("..");
        candidate.Append(WDL_DIRCHAR_STR);
        candidate.Append("Resources");
        candidate.Append(WDL_DIRCHAR_STR);
        candidate.Append(kNoticesFileName);

        if (!FileExists(candidate))
          return false;

        result.Set(candidate.Get());
        return true;
      }

      static bool ResolveNoticesPath(IGraphics* pGraphics, WDL_String& path)
      {
        path.Set("");

        if (pGraphics == nullptr)
          return false;

#ifdef OS_WIN
        WDL_String directory;
        const auto moduleHandle = static_cast<PluginIDType>(pGraphics->GetWinModuleHandle());

        BundleResourcePath(directory, moduleHandle);
        if (TryNoticePathInDirectory(path, directory))
          return true;

        directory.Set("");
        PluginPath(directory, moduleHandle);
        if (TryNoticePathInDirectory(path, directory))
          return true;

        if (TryNoticePathSiblingResources(path, directory))
          return true;
#endif

        const auto resourceLocation =
          LocateResource(kNoticesFileName, "txt", path, pGraphics->GetBundleID(), pGraphics->GetWinModuleHandle(),
                         pGraphics->GetSharedResourcesSubPath());

        return resourceLocation == EResourceLocation::kAbsolutePath && FileExists(path);
      }

      static bool OpenNoticesPath(IGraphics* pGraphics, const WDL_String& path)
      {
        if (pGraphics == nullptr || !CStringHasContents(path.Get()))
          return false;

#ifdef OS_WIN
        WCHAR pathWide[IPLUG_WIN_MAX_WIDE_PATH];
        UTF8ToUTF16(pathWide, path.Get(), IPLUG_WIN_MAX_WIDE_PATH);

        if (pathWide[0] == 0)
          return false;

        WCHAR canon[IPLUG_WIN_MAX_WIDE_PATH];
        const DWORD nCanon = GetFullPathNameW(pathWide, IPLUG_WIN_MAX_WIDE_PATH, canon, nullptr);
        const WCHAR* const launchPath = (nCanon > 0 && nCanon < IPLUG_WIN_MAX_WIDE_PATH) ? canon : pathWide;

        return ShellExecuteW(nullptr, L"open", launchPath, nullptr, nullptr, SW_SHOWNORMAL) > HINSTANCE(32);
#else
        return pGraphics->OpenURL(path.Get());
#endif
      }

      static void ShowOpenError(IGraphics* pGraphics)
      {
        if (pGraphics == nullptr)
          return;

        const char* const title = "Third party notices";
        const char* const message = "Could not open ThirdPartyNotices.txt.";

#ifdef OS_MAC
        pGraphics->ShowMessageBox(title, message, kMB_OK);
#else
        pGraphics->ShowMessageBox(message, title, kMB_OK);
#endif
      }

      static constexpr const char* kNoticesFileName = "ThirdPartyNotices.txt";
    };

    IVStyle mStyle;
    IText mText;
  };
};
