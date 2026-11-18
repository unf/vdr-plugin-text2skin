//								-*- c++ -*-

#include "render.h"
#include "loader.h"
#include "display.h"
#include "scroller.h"
#include "status.h"
#include "common.h"
#include "xml/string.h"
#include <vdr/menu.h>
#include <vdr/plugin.h>

// --- cText2SkinDisplayChannel -----------------------------------------------

cText2SkinDisplayChannel::cText2SkinDisplayChannel(cText2SkinLoader *Loader, bool WithInfo):
		cText2SkinRender(Loader, WithInfo ? cxDisplay::channelInfo : cxDisplay::channelSmall),
		mFallbackDisplay(NULL),
		mChannelPtr(NULL),
		mNumber(0),
		mPresentPtr(NULL),
		mFollowingPtr(NULL),
		mType(mtStatus),
		mText("") ,
		mButtonRed(""),
		mButtonGreen(""),
		mButtonYellow(""),
		mButtonBlue("")
{
	if (Fallback() != NULL) {
		mFallbackDisplay = Fallback()->DisplayChannel(WithInfo);
		Skins.Message(mtError, tr("Skin too large or incorrectly aligned"), 2);
	}
}

cText2SkinDisplayChannel::~cText2SkinDisplayChannel()
{
	if (mFallbackDisplay != NULL)
		delete mFallbackDisplay;
}

void cText2SkinDisplayChannel::SetChannel(const cChannel *Channel, int Number)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetChannel(Channel, Number);
		return;
	}

	tChannelData channel;
	channel.Set(Channel, Number);

	UpdateLock();
	if (mChannelPtr != Channel || mNumber != Number) {
		mChannelPtr = Channel;
		mNumber = Number;
		SetDirty(cxRefresh::update);
	}
	mChannel = channel;
	UpdateUnlock();
}

void cText2SkinDisplayChannel::SetEvents(const cEvent *Present, const cEvent *Following)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetEvents(Present, Following);
		return;
	}

	// VDR calls this again after EPG updates, a changed text is shown as well
	tEventData present, following;
	present.Set(Present);
	following.Set(Following);

	UpdateLock();
	if (mPresentPtr != Present || mFollowingPtr != Following
	    || mPresent != present || mFollowing != following) {
		mPresentPtr = Present;
		mFollowingPtr = Following;
		mPresent = present;
		mFollowing = following;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayChannel::SetMessage(eMessageType Type, const char *Text)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetMessage(Type, Text);
		return;
	}

	UpdateLock();
	if (Text == NULL) Text = "";
	if (mType != Type || mText != Text) {
		mType = Type;
		mText = Text;
		SetDirty(cxRefresh::all);
	}
	UpdateUnlock();
}

void cText2SkinDisplayChannel::SetButtons(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetButtons(Red, Green, Yellow, Blue);
		return;
	}

	UpdateLock();
	Dprintf("SetButtons(%s, %s, %s, %s)\n", Red, Green, Yellow, Blue);
	if (Red    == NULL) Red    = "";
	if (Green  == NULL) Green  = "";
	if (Yellow == NULL) Yellow = "";
	if (Blue   == NULL) Blue   = "";
	if (mButtonRed != Red || mButtonGreen != Green || mButtonYellow != Yellow
			|| mButtonBlue != Blue) {
		mButtonRed    = Red;
		mButtonGreen  = Green;
		mButtonYellow = Yellow;
		mButtonBlue   = Blue;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

cxType cText2SkinDisplayChannel::GetTokenData(const txToken &Token)
{
	switch (Token.Type) {
	case tChannelNumber:
		return mChannel.number;

	case tChannelName:
		return mChannel.valid
		       ? (cxType)mChannel.name
		       : (cxType)false;

	case tChannelShortName:
		return mChannel.valid
		       ? (cxType)mChannel.shortName
		       : (cxType)false;

	case tChannelServiceReference:
		return mChannel.valid
		       ? (cxType)mChannel.serviceReference
		       : (cxType)false;

	case tChannelBouquet:
		return mChannel.valid
		       ? (cxType)mChannel.provider
		       : (cxType)false;

	case tChannelPortal:
		return mChannel.valid
		       ? (cxType)mChannel.portal
		       : (cxType)false;

	case tChannelSource:
		return mChannel.valid && !mChannel.source.empty()
		       ? (cxType)mChannel.source
		       : (cxType)false;

	case tTChannelID:
		return mChannel.valid
		       ? (cxType)mChannel.channelID
		       : (cxType)false;

	case tPresentStartDateTime:
		return mPresent.valid
		       ? (cxType)TimeType(mPresent.startTime, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentVPSDateTime:
		return mPresent.valid
		       ? (cxType)TimeType(mPresent.vps, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentEndDateTime: return mPresent.valid
		       ? (cxType)TimeType(mPresent.endTime, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentProgress:
		return mPresent.valid
		       ? (cxType)DurationType(time(NULL) - mPresent.startTime,
		                              Token.Attrib.Text)
		       : (cxType)false;

	case tPresentDuration:
		return mPresent.valid
		       ? (cxType)DurationType(mPresent.duration, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentRemaining:
		if (mPresent.valid && time(NULL) - mPresent.startTime
		                      <= mPresent.duration) {
			return (cxType)DurationType(mPresent.duration - (time(NULL) - mPresent.startTime),
		                                    Token.Attrib.Text);
		}
		return false;

	case tPresentTitle:
		return mPresent.valid
		       ? (cxType)mPresent.title
		       : (cxType)false;

	case tPresentShortText:
		return mPresent.valid
		       ? (cxType)mPresent.shortText
		       : (cxType)false;

	case tPresentDescription:
		return mPresent.valid
		       ? (cxType)mPresent.description
		       : (cxType)false;

	case tFollowingStartDateTime:
		return mFollowing.valid
		       ? (cxType)TimeType(mFollowing.startTime, Token.Attrib.Text)
		       : (cxType)false;

	case tFollowingVPSDateTime:
		return mFollowing.valid
		       ? (cxType)TimeType(mFollowing.vps, Token.Attrib.Text)
		       : (cxType)false;

	case tFollowingEndDateTime:
		return mFollowing.valid
		       ? (cxType)TimeType(mFollowing.endTime, Token.Attrib.Text)
		       : (cxType)false;

	case tFollowingDuration:
		return mFollowing.valid
		       ? (cxType)DurationType(mFollowing.duration, Token.Attrib.Text)
		       : (cxType)false;

	case tFollowingTitle:
		return mFollowing.valid
		       ? (cxType)mFollowing.title
		       : (cxType)false;

	case tPresentPoster:
		return mPresent.valid && !mPresent.poster.empty()
		       ? (cxType)mPresent.poster
		       : (cxType)false;

	case tFollowingPoster:
		return mFollowing.valid && !mFollowing.poster.empty()
		       ? (cxType)mFollowing.poster
		       : (cxType)false;

	case tFollowingShortText:
		return mFollowing.valid
		       ? (cxType)mFollowing.shortText
		       : (cxType)false;

	case tFollowingDescription:
		return mFollowing.valid
		       ? (cxType)mFollowing.description
		       : (cxType)false;

	case tLanguage: {
			cDevice *dev = cDevice::PrimaryDevice();
			eTrackType trackType = dev->GetCurrentAudioTrack();
			const tTrackId *track = dev->GetTrack(trackType);
			if (track) {
				std::string buffer(track->language);
				if (trackType >= ttDolby)
					buffer.append("DD");
				return (cxType)buffer.c_str();
			}
			return (cxType)false;
		}
		return false;

	case tVideoSizeWidth:    {
			int width, height;
			double aspect;
			cDevice *dev = cDevice::PrimaryDevice();
			dev->GetVideoSize(width, height, aspect);
			return (cxType)width;
		}

	case tVideoSizeHeight:    {
			int width, height;
			double aspect;
			cDevice *dev = cDevice::PrimaryDevice();
			dev->GetVideoSize(width, height, aspect);
			return (cxType)height;
		}

	case tHasTeletext:
	case tChannelHasTeletext:
		return mChannel.valid && mChannel.hasTeletext;

	case tHasMultilang:
	case tChannelHasMultilang:
		return mChannel.valid && mChannel.hasMultilang;

	case tHasDolby:
	case tChannelHasDolby:
		return mChannel.valid && mChannel.hasDolby;

	case tIsEncrypted:
	case tChannelIsEncrypted:
		return mChannel.valid && mChannel.isEncrypted;

	case tIsRadio:
	case tChannelIsRadio:
		return mChannel.valid && mChannel.isRadio;

	case tHasVPS:
	case tChannelHasVPS:
		return mPresent.valid && mPresent.vps != 0;

	case tPresentHasVPS:
		return mPresent.valid && mPresent.vps != 0 && mPresent.vps != mPresent.startTime;

	case tHasTimer:
	case tPresentHasTimer:
		return mPresent.valid && mPresent.hasTimer;

	case tIsRunning:
	case tPresentIsRunning:
		return mPresent.valid && mPresent.isRunning;

	case tFollowingHasTimer:
		return mFollowing.valid && mFollowing.hasTimer;

	case tFollowingIsRunning:
		return mFollowing.valid && mFollowing.isRunning;

	case tFollowingHasVPS:
		return mFollowing.valid && mFollowing.vps != 0
		    && mFollowing.vps != mFollowing.startTime;

	case tMessage:
		return mText;

	case tMessageInfo:
		return mType == mtInfo
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageWarning:
		return mType == mtWarning
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageStatus:
		return mType == mtStatus
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageError:
		return mType == mtError
		       ? (cxType)mText
		       : (cxType)false;

	case tButtonRed:
		return mButtonRed;

	case tButtonGreen:
		return mButtonGreen;

	case tButtonYellow:
		return mButtonYellow;

	case tButtonBlue:
		return mButtonBlue;

	default:
		return cText2SkinRender::GetTokenData(Token);
	}
}

// --- cText2SkinDisplayVolume ------------------------------------------------

cText2SkinDisplayVolume::cText2SkinDisplayVolume(cText2SkinLoader *Loader):
		cText2SkinRender(Loader, cxDisplay::volume),
		mCurrent(0),
		mTotal(0),
		mMute(false)
{
}

cText2SkinDisplayVolume::~cText2SkinDisplayVolume()
{
}

void cText2SkinDisplayVolume::SetVolume(int Current, int Total, bool Mute)
{
	UpdateLock();
	if (mCurrent != Current || mTotal != Total || mMute != Mute) {
		mCurrent = Current;
		mTotal = Total;
		mMute = Mute;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

cxType cText2SkinDisplayVolume::GetTokenData(const txToken &Token) {
	switch (Token.Type) {
	case tVolumeCurrent: return mCurrent;

	case tVolumeTotal:   return mTotal;

	case tIsMute:
	case tVolumeIsMute:  return mMute;

	default:             return cText2SkinRender::GetTokenData(Token);
	}
}

// --- cText2SkinDisplayReplay ------------------------------------------------

cText2SkinDisplayReplay::cText2SkinDisplayReplay(cText2SkinLoader *Loader, bool ModeOnly):
		cText2SkinRender(Loader, ModeOnly ? cxDisplay::replaySmall : cxDisplay::replayInfo),
		mTitle(""),
		mStateInfo(false),
		mPlay(false),
		mForward(false),
		mSpeed(0),
		mCurrent(0),
		mTotal(0),
		mMarks(NULL),
		mPrompt(""),
		mType(mtStatus),
		mText(""),
		mButtonRed(""),
		mButtonGreen(""),
		mButtonYellow(""),
		mButtonBlue(""),
		mMessageDisplay(ModeOnly && mDisplay != NULL && !mDisplay->UsesMessage())
{
}

cText2SkinDisplayReplay::~cText2SkinDisplayReplay()
{
}

void cText2SkinDisplayReplay::SetTitle(const char *Title)
{
	UpdateLock();
	if (Title == NULL) Title = "";
	if (mTitle != Title) {
		mTitle = Title;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetMode(bool Play, bool Forward, int Speed)
{
	Dprintf("SetMode(%d, %d, %d)\n", Play, Forward, Speed);
	UpdateLock();
	if (!mStateInfo || mPlay != Play || mForward != Forward || mSpeed != Speed) {
		mStateInfo = true;
		mPlay = Play;
		mForward = Forward;
		mSpeed = Speed;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetProgress(int Current, int Total)
{
	UpdateLock();
	if (mCurrent != Current || mTotal != Total) {
		mCurrent = Current;
		mTotal = Total;
		// SetDirty(cxRefresh::update); TODO: let this cause a display update every frame?
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetMarks(const cMarks *Marks)
{
	UpdateLock();
	mMarks = Marks;
	SetDirty(cxRefresh::update);
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetCurrent(const char *Current)
{
	UpdateLock();
	if (Current == NULL) Current = "";
	if (mPosition != Current) {
		mPosition = Current;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetTotal(const char *Total)
{
	UpdateLock();
	if (Total == NULL) Total = "";
	if (mDuration != Total) {
		mDuration = Total;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetJump(const char *Jump)
{
	UpdateLock();
	if (Jump == NULL) Jump = "";
	if (mPrompt != Jump) {
		mPrompt = Jump;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetMessage(eMessageType Type, const char *Text)
{
	UpdateLock();
	if (Text == NULL) Text = "";
	if (mType != Type || mText != Text) {
		mType = Type;
		mText = Text;
		if (mMessageDisplay)
			SwitchDisplay(*Text ? cxDisplay::message : cxDisplay::replaySmall);
		SetDirty(cxRefresh::all);
	}
	UpdateUnlock();
}

void cText2SkinDisplayReplay::SetButtons(const char *Red, const char *Green, const char *Yellow,
                                         const char *Blue)
{
	UpdateLock();
	Dprintf("SetButtons(%s, %s, %s, %s)\n", Red, Green, Yellow, Blue);
	if (Red    == NULL) Red    = "";
	if (Green  == NULL) Green  = "";
	if (Yellow == NULL) Yellow = "";
	if (Blue   == NULL) Blue   = "";
	if (mButtonRed != Red || mButtonGreen != Green || mButtonYellow != Yellow || mButtonBlue != Blue) {
		mButtonRed    = Red;
		mButtonGreen  = Green;
		mButtonYellow = Yellow;
		mButtonBlue   = Blue;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

cxType cText2SkinDisplayReplay::GetTokenData(const txToken &Token)
{
	switch (Token.Type) {
	case tReplayTitle:
		return mTitle;

	case tReplayPositionIndex:
		return DurationType(mCurrent,
		                    Text2SkinStatus.ReplayFramesPerSecond(),
		                    Token.Attrib.Text);

	case tReplayDurationIndex:
		return DurationType(mTotal,
		                    Text2SkinStatus.ReplayFramesPerSecond(),
		                    Token.Attrib.Text);

	case tReplayPosition:
		return mPosition;

	case tReplayDuration:
		return mDuration;

	case tReplayRemaining:
		return DurationType(mTotal - mCurrent,
		                    Text2SkinStatus.ReplayFramesPerSecond(),
		                    Token.Attrib.Text);

	case tReplayPrompt:
		return mPrompt;

	case tIsPlaying:
	case tReplayIsPlaying:
		return mStateInfo && mSpeed == -1 && mPlay;

	case tIsPausing:
	case tReplayIsPausing:
		return mStateInfo && mSpeed == -1 && !mPlay;

	case tIsFastForward:
	case tReplayIsFastForward:
		if (mStateInfo && mSpeed != -1 && mPlay && mForward) {
			return Token.Attrib.Type == aNumber
			       ? (cxType)(mSpeed == Token.Attrib.Number)
			       : (cxType)true;
		}
		return false;

	case tIsFastRewind:
	case tReplayIsFastRewind:
		if (mStateInfo && mSpeed != -1 && mPlay && !mForward) {
			return Token.Attrib.Type == aNumber
			       ? (cxType)(mSpeed == Token.Attrib.Number)
			       : (cxType)true;
		}
		return false;

	case tIsSlowForward:
	case tReplayIsSlowForward:
		if (mStateInfo && mSpeed != -1 && !mPlay && mForward) {
			return Token.Attrib.Type == aNumber
			       ? (cxType)(mSpeed == Token.Attrib.Number)
			       : (cxType)true;
		}
		return false;

	case tIsSlowRewind:
	case tReplayIsSlowRewind:
		if (mStateInfo && mSpeed != -1 && !mPlay && !mForward) {
			return Token.Attrib.Type == aNumber
			       ? (cxType)(mSpeed == Token.Attrib.Number)
			       : (cxType)true;
		}
		return false;

	case tLanguage: {
		cDevice *dev = cDevice::PrimaryDevice();
		eTrackType trackType = dev->GetCurrentAudioTrack();
		const tTrackId *track = dev->GetTrack(trackType);
		if (track) {
			std::string buffer(track->language);
			if (trackType >= ttDolby)
				buffer.append("DD");
			return (cxType)buffer.c_str();
		}
		return (cxType)false;
	}
	return false;

	case tMessage:
		return mText;

	case tMessageInfo:
		return mType == mtInfo
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageWarning:
		return mType == mtWarning
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageStatus:
		return mType == mtStatus
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageError:
		return mType == mtError
		       ? (cxType)mText
		       : (cxType)false;

	case tButtonRed:
		return mButtonRed;

	case tButtonGreen:
		return mButtonGreen;

	case tButtonYellow:
		return mButtonYellow;

	case tButtonBlue:
		return mButtonBlue;

	default:
		return cText2SkinRender::GetTokenData(Token);
	}
}

// --- cText2SkinDisplayMessage -----------------------------------------------

cText2SkinDisplayMessage::cText2SkinDisplayMessage(cText2SkinLoader *Loader):
		cText2SkinRender(Loader, cxDisplay::message),
		mText("")
{
}

cText2SkinDisplayMessage::~cText2SkinDisplayMessage()
{
}

void cText2SkinDisplayMessage::SetMessage(eMessageType Type, const char *Text)
{
	UpdateLock();
	if (Text == NULL) Text = "";
	if (mType != Type || mText != Text) {
		mType = Type;
		mText = Text;
		SetDirty(cxRefresh::all);
	}
	UpdateUnlock();
}

cxType cText2SkinDisplayMessage::GetTokenData(const txToken &Token)
{
	switch (Token.Type) {
	case tMessage:
		return mText;

	case tMessageInfo:
		return mType == mtInfo
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageWarning:
		return mType == mtWarning
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageStatus:
		return mType == mtStatus
		       ? (cxType)mText
		       : (cxType)false;

	case tMessageError:
		return mType == mtError
		       ? (cxType)mText
		       : (cxType)false;

	default:
		return cText2SkinRender::GetTokenData(Token);
	}
}

// --- cText2SkinDisplayMenu --------------------------------------------------

cText2SkinDisplayMenu::cText2SkinDisplayMenu(cText2SkinLoader *Loader):
		cText2SkinRender(Loader, cxDisplay::menu),
		mFallbackDisplay(NULL),
		mMaxItems(0),
		mTitle(""),
		mButtonRed(""),
		mButtonGreen(""),
		mButtonYellow(""),
		mButtonBlue(""),
		mMessageType(mtStatus),
		mMessageText(""),
		mEventPtr(NULL),
		mRecordingPtr(NULL),
		mItems(),
		mCurrentItem((uint)-1)
{
	if (Fallback() != NULL) {
		mFallbackDisplay = Fallback()->DisplayMenu();
		mMaxItems = mFallbackDisplay->MaxItems();
		Skins.Message(mtError, tr("Skin too large or incorrectly aligned"), 2);
		return;
	}

	cxDisplay *disp = Loader->Data()->Get(cxDisplay::menu);
	const cxObject *area = NULL;
	for (uint i = 0; i < disp->Objects(); ++i) {
		const cxObject *o = disp->GetObject(i);
		if (o->Type() == cxObject::list) {
			area = o;
			break;
		}
	}

	if (area != NULL) {
		const cxObject *item = area->GetObject(0);
		if (item != NULL && item->Type() == cxObject::item)
			mMaxItems = area->Size().h / item->Size().h;
	}
}

cText2SkinDisplayMenu::~cText2SkinDisplayMenu()
{
	if (mFallbackDisplay != NULL)
		delete mFallbackDisplay;
}

void cText2SkinDisplayMenu::Clear(void)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->Clear();
		return;
	}

	UpdateLock();
	mItems.clear();
	mCurrentItem = (uint)-1;
	mEventPtr = NULL;
	mEvent = tEventData();
	ExtPresentDescription = "";
	mRecordingPtr = NULL;
	mRecording = tRecordingData();
	ExtRecordingDescription = "";
	mText = "";
	cText2SkinRender::Clear();
	SetDirty(cxRefresh::all);
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetTitle(const char *Title)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetTitle(Title);
		return;
	}

	UpdateLock();
	if (Title == NULL) Title = "";
	if (mTitle != Title) {
		mUpdate.timerConflict = true;
		mUpdate.events = true;
		mUpdate.resetMarquee = true;
		mTitle = Title;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetButtons(const char *Red, const char *Green, const char *Yellow,
                                       const char *Blue)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetButtons(Red, Green, Yellow, Blue);
		return;
	}

	UpdateLock();
	if (Red    == NULL) Red    = "";
	if (Green  == NULL) Green  = "";
	if (Yellow == NULL) Yellow = "";
	if (Blue   == NULL) Blue   = "";
	if (mButtonRed != Red || mButtonGreen != Green || mButtonYellow != Yellow
			|| mButtonBlue != Blue) {
		mButtonRed    = Red;
		mButtonGreen  = Green;
		mButtonYellow = Yellow;
		mButtonBlue   = Blue;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetMessage(eMessageType Type, const char *Text)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetMessage(Type, Text);
		return;
	}

	UpdateLock();
	if (Text == NULL) Text = "";
	if (mMessageType != Type || mMessageText != Text) {
		mMessageType = Type;
		mMessageText = Text;
		SetDirty(cxRefresh::all);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetItem(const char *Text, int Index, bool Current, bool Selectable)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetItem(Text, Index, Current, Selectable);
		return;
	}

	UpdateLock();
	if (Text == NULL) Text = "";
	tListItem item(Text, Selectable);

	for (int i = 0; i < MaxTabs; ++i) {
		const char *tab = GetTabbedText(Text, i);
		if (tab)
			item.tabs[i] = tab;
		if (!Tab(i + 1))
			break;
	}

	if (mItems.size() <= (uint)Index) {
		mItems.push_back(item);
		mDirtyItems.push_back(Index);
		SetDirty(cxRefresh::list);
	}
	else if (mItems[Index] != item) {
		mItems[Index] = item;
		// refresh only the changed items
		mDirtyItems.push_back(Index);
		SetDirty(cxRefresh::list);
	}

	if (Current && mCurrentItem != (uint)Index) {
		// refresh only the changed items
		mDirtyItems.push_back(mCurrentItem);
		mDirtyItems.push_back(Index);
		SetDirty(cxRefresh::list);
		mCurrentItem = Index;
	}

	if (Current)
		mRender->mMenuScrollbar.currentOnScreen = (uint)Index;
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetEvent(const cEvent *Event)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetEvent(Event);
		return;
	}

	tEventData event;
	event.Set(Event);

	UpdateLock();
	if (mEventPtr != Event || mEvent != event) {
		mEventPtr = Event;
		mEvent = event;
		ExtPresentDescription = "";
		if (Event != NULL)
			SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetRecording(const cRecording *Recording)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetRecording(Recording);
		return;
	}

	tRecordingData recording;
	recording.Set(Recording);

	UpdateLock();
	if (mRecordingPtr != Recording) {
		mRecordingPtr = Recording;
		mRecording = recording;
		ExtRecordingDescription = "";
		if (Recording != NULL)
			SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetText(const char *Text, bool FixedFont)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetText(Text, FixedFont);
		return;
	}

	UpdateLock();
	if (Text == NULL) Text = "";
	if (mText != Text) {
		mText = Text;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

void cText2SkinDisplayMenu::SetTabs(int Tab1, int Tab2, int Tab3, int Tab4, int Tab5)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->SetTabs(Tab1, Tab2, Tab3, Tab4, Tab5);
		return;
	}

	UpdateLock();
	cSkinDisplayMenu::SetTabs(Tab1, Tab2, Tab3, Tab4, Tab5);
	UpdateUnlock();
}

void cText2SkinDisplayMenu::Scroll(bool Up, bool Page)
{
	if (mFallbackDisplay != NULL) {
		mFallbackDisplay->Scroll(Up, Page);
		return;
	}

	UpdateLock();
	cText2SkinRender::Scroll(Up, Page);
	SetDirty(cxRefresh::scroll);
	UpdateUnlock();
}

cxType cText2SkinDisplayMenu::GetTokenData(const txToken &Token)
{
	switch (Token.Type) {
	case tMenuItem:
	case tMenuGroup:
	case tMenuCurrent:
		break;
	default:
		if (Token.Tab >= 0) return false;
		break;
	}

	switch (Token.Type) {
	case tMenuTitle:
		return mTitle;

	case tMenuItem:
		if (Token.Index < 0)
			return false;

		if (mItems.size() <= (uint)Token.Index || !mItems[Token.Index].sel ||
		    mCurrentItem == (uint)Token.Index)
			return false;

		if (Token.Tab < 0)
			return Token.Attrib.Type == aNumber
			       ? (cxType)mItems[Token.Index].tabs[Token.Attrib.Number]
			       : (cxType)mItems[Token.Index].text;

		return (cxType)mItems[Token.Index].tabs[Token.Tab];

	case tIsMenuItem:
		return mItems.size() > (uint)Token.Index && mItems[Token.Index].sel
		       && mCurrentItem != (uint)Token.Index;

	case tMenuCurrent:
		if (mItems.size() <= mCurrentItem)
			return false;

		if (Token.Index < 0)
			return Token.Attrib.Type == aNumber
			       ? (cxType)mItems[mCurrentItem].tabs[Token.Attrib.Number]
			       : (cxType)mItems[mCurrentItem].text;

		if (mItems.size() <= (uint)Token.Index || !mItems[Token.Index].sel ||
		    mCurrentItem != (uint)Token.Index)
			return false;

		if (Token.Tab < 0)
			return Token.Attrib.Type == aNumber
			       ? (cxType)mItems[Token.Index].tabs[Token.Attrib.Number]
			       : (cxType)mItems[Token.Index].text;

		return (cxType)mItems[Token.Index].tabs[Token.Tab];

	case tIsMenuCurrent:
		return mItems.size() > (uint)Token.Index && mItems[Token.Index].sel
		       && mCurrentItem == (uint)Token.Index;

	case tMenuGroup:
		if (Token.Index < 0)
			return false;

		if (mItems.size() <= (uint)Token.Index || mItems[Token.Index].sel)
			return false;

		if (Token.Tab < 0)
			return Token.Attrib.Type == aNumber
			       ? (cxType)mItems[Token.Index].tabs[Token.Attrib.Number]
			       : (cxType)mItems[Token.Index].text;

		return (cxType)mItems[Token.Index].tabs[Token.Tab];

	case tIsMenuGroup:
		return mItems.size() > (uint)Token.Index && !mItems[Token.Index].sel;

	case tButtonRed:
		return mButtonRed;

	case tButtonGreen:
		return mButtonGreen;

	case tButtonYellow:
		return mButtonYellow;

	case tButtonBlue:
		return mButtonBlue;

	case tMessage:
		return mMessageText;

	case tMessageInfo:
		return mMessageType == mtInfo
		       ? (cxType)mMessageText
		       : (cxType)false;

	case tMessageWarning:
		return mMessageType == mtWarning
		       ? (cxType)mMessageText
		       : (cxType)false;

	case tMessageStatus:
		return mMessageType == mtStatus
		       ? (cxType)mMessageText
		       : (cxType)false;

	case tMessageError:
		return mMessageType == mtError
		       ? (cxType)mMessageText
		       : (cxType)false;

	case tPresentStartDateTime:
		return mEvent.valid
		       ? (cxType)TimeType(mEvent.startTime, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentVPSDateTime:
		return mEvent.valid
		       ? (cxType)TimeType(mEvent.vps, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentEndDateTime:
		return mEvent.valid
		       ? (cxType)TimeType(mEvent.endTime, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentProgress:
		return mEvent.valid
		       ? (cxType)DurationType(time(NULL) - mEvent.startTime,
		                              Token.Attrib.Text)
		       : (cxType)false;

	case tPresentDuration:
		return mEvent.valid
		       ? (cxType)DurationType(mEvent.duration, Token.Attrib.Text)
		       : (cxType)false;

	case tPresentRemaining:
		return mEvent.valid
		       ? (cxType)DurationType(mEvent.duration - (time(NULL) - mEvent.startTime),
		                              Token.Attrib.Text)
		       : (cxType)false;

	case tPresentTitle:
		return mEvent.valid
		       ? (cxType)mEvent.title
		       : (cxType)false;

	case tPresentShortText:
		return mEvent.valid
		       ? (cxType)mEvent.shortText
		       : (cxType)false;

	case tPresentDescription:
		if (mEvent.valid) {
			if (ExtPresentDescription == "") {
				// The timers give the aux info, the reruns come from epgsearch,
				// which locks channels and schedules itself. Holding them here
				// first means epgsearch can't wait for VDR (see cListReader).
				cListReader<cTimers> Timers(cTimers::GetTimersRead);
				cListReader<cChannels> Channels(cChannels::GetChannelsRead);
				cListReader<cSchedules> Schedules(cSchedules::GetSchedulesRead);
				if (!Timers || !Channels || !Schedules)
					return Busy();
				// find corresponding timer
				std::string aux;
				bool auxFound = false;
				for (const cTimer *tim = Timers->First(); tim; tim = Timers->Next(tim)) {
					// compares pointers only, the timer's event is not read
					if (tim->Event() == mEventPtr) {
						aux = tim->Aux() ? tim->Aux() : "";
						auxFound = tim->Aux() != NULL;
					}
				}
				ExtPresentDescription = AddExtInfoToDescription(mEvent.title.c_str(), mEvent.shortText.c_str(), mEvent.description.c_str(), Text2SkinSetup.ShowAux && auxFound ? aux.c_str() : NULL, Text2SkinSetup.StripAux);
			}
			return (cxType)ExtPresentDescription;
		} else
			return (cxType)false;

	case tPresentLanguageCode:
		return mEvent.valid
		       ? ComponentLanguageCode(mEvent.components, Token.Attrib.Number)
		       : (cxType)false;

	case tPresentLanguageDescription:
		return mEvent.valid
		       ? ComponentLanguageDescription(mEvent.components, Token.Attrib.Number)
		       : (cxType)false;

	case tPresentVideoAR:
		return mEvent.valid
		       ? ComponentVideoAR(mEvent.components)
		       : (cxType)false;

	case tPresentEventID:
		return mEvent.valid
		       ? (cxType)EventType(mEvent.eventID)
		       : (cxType)false;

	case tPresentPoster:
		return mEvent.valid && !mEvent.poster.empty()
		       ? (cxType)mEvent.poster
		       : (cxType)false;

	case tHasVPS:
	case tChannelHasVPS:
		return mEvent.valid && mEvent.vps != 0;

	case tChannelName:
	case tChannelShortName:
	case tChannelServiceReference:
		// extended EPG or recording info
		if (mEvent.valid || mRecording.valid) {
			cListReader<cChannels> Channels(cChannels::GetChannelsRead);
			if (!Channels)
				return Busy();
			const cChannel *channel = Channels->GetByChannelID(mEvent.valid ? mEvent.channelID : mRecording.channelID, true);
			if (channel == NULL)
				return false;
			switch (Token.Type) {
			case tChannelName:      return ChannelName(channel, 0);
			case tChannelShortName: return ChannelShortName(channel, 0);
			default:                return ChannelServiceReference(channel, 0);
			}
		}
		return false;

	case tPresentHasVPS:
		return mEvent.valid && mEvent.vps != 0 && mEvent.vps != mEvent.startTime;

	case tHasTimer:
	case tPresentHasTimer:
		return mEvent.valid && mEvent.hasTimer;

	case tIsRunning:
	case tPresentIsRunning:
		return mEvent.valid && mEvent.isRunning;

	case tMenuText:
		return mText;

	case tRecordingName:
		return mRecording.valid
		       ? (cxType)mRecording.name
		       : (cxType)false;

	case tRecordingPoster:
		return mRecording.valid && !mRecording.poster.empty()
		       ? (cxType)mRecording.poster
		       : (cxType)false;

	case tRecordingFilename:
		return mRecording.valid
		       ? (cxType)mRecording.fileName
		       : (cxType)false;

	case tRecordingPriority:
		return mRecording.valid
		       ? (cxType)mRecording.priority
		       : (cxType)false;

	case tRecordingLifetime:
		return mRecording.valid
		       ? (cxType)mRecording.lifetime
		       : (cxType)false;

	case tRecordingDateTime:
		return mRecording.valid
		       ? (cxType)TimeType(mRecording.start, Token.Attrib.Text)
		       : (cxType)false;

	case tRecordingTitle:
		return mRecording.valid
		       ? (cxType)mRecording.title
		       : (cxType)false;

	case tRecordingShortText:
		return mRecording.valid
		       ? (cxType)mRecording.shortText
		       : (cxType)false;

	case tRecordingDescription:
		if (mRecording.valid) {
			if (ExtRecordingDescription == "") {
				// reruns from epgsearch, see tPresentDescription
				cListReader<cTimers> Timers(cTimers::GetTimersRead);
				cListReader<cChannels> Channels(cChannels::GetChannelsRead);
				cListReader<cSchedules> Schedules(cSchedules::GetSchedulesRead);
				if (!Timers || !Channels || !Schedules)
					return Busy();
				ExtRecordingDescription = AddExtInfoToDescription(mRecording.title.c_str(), mRecording.shortText.c_str(), mRecording.description.c_str(), Text2SkinSetup.ShowAux && !mRecording.aux.empty() ? mRecording.aux.c_str() : NULL, Text2SkinSetup.StripAux);
			}
			return (cxType)ExtRecordingDescription;
		} else
			return (cxType)false;

	case tRecordingLanguageCode:
		return mRecording.valid
		       ? ComponentLanguageCode(mRecording.components, Token.Attrib.Number)
		       : (cxType)false;

	case tRecordingLanguageDescription:
		return mRecording.valid
		       ? ComponentLanguageDescription(mRecording.components, Token.Attrib.Number)
		       : (cxType)false;

	case tRecordingVideoAR:
		return mRecording.valid
		       ? ComponentVideoAR(mRecording.components)
		       : (cxType)false;

	case tRecordingSize:
		return mRecording.valid
		       ? (cxType)DirSizeMB(mRecording.fileName.c_str())
		       : (cxType)false;

	case tRecordingLength:
		return mRecording.valid
		       ? (cxType)GetRecordingLength(mRecording.fileName.c_str(), mRecording.framesPerSecond, mRecording.isPesRecording)
		       : (cxType)false;

	case tRecordingCuttedLength:
		return mRecording.valid
		       ? (cxType)GetRecordingCuttedLength(mRecording.fileName.c_str(), mRecording.framesPerSecond, mRecording.isPesRecording)
		       : (cxType)false;

	default:
		return cText2SkinRender::GetTokenData(Token);
	}
}

// --- cText2SkinDisplayTracks ------------------------------------------------

cText2SkinDisplayTracks::cText2SkinDisplayTracks(cText2SkinLoader *Loader, const char *Title,
                                                 int NumTracks, const char * const *Tracks):
		cText2SkinRender(Loader, cxDisplay::audioTracks),
		mTitle(Title),
		mItems(),
		mCurrentItem((uint)-1),
		mAudioChannel(-1)
{
	Dprintf("NumTracks: %d\n", NumTracks);
	for (int i = 0; i < NumTracks; ++i) {
		tListItem item(Tracks[i]);
		mItems.push_back(item);
	}
}

cText2SkinDisplayTracks::~cText2SkinDisplayTracks()
{
}

const char *cText2SkinDisplayTracks::ChannelName(int AudioChannel)
{
	switch (AudioChannel)
	{
		case 0: return "stereo";
		case 1: return "left";
		case 2: return "right";
	}
	return "";
}

void cText2SkinDisplayTracks::SetTrack(int Index, const char * const *Tracks)
{
	UpdateLock();
	Dprintf("SetTrack: %d (%s here, %s in array)\n", Index, Tracks[Index], mItems[Index].c_str());
	if (mCurrentItem != (uint)Index) {
		// refresh only the changed items
		mDirtyItems.push_back(mCurrentItem);
		mDirtyItems.push_back(Index);
		mCurrentItem = Index;
		SetDirty(cxRefresh::list);
	}
	UpdateUnlock();
}

void cText2SkinDisplayTracks::SetAudioChannel(int AudioChannel)
{
	UpdateLock();
	if (mAudioChannel != AudioChannel) {
		mAudioChannel = AudioChannel;
		SetDirty(cxRefresh::update);
	}
	UpdateUnlock();
}

cxType cText2SkinDisplayTracks::GetTokenData(const txToken &Token)
{
	if (Token.Tab >= 0)
		return false;

	int index = Token.Index;
	if (index >= 0 && mCurrentItem >= (uint)mMaxItems) {
		int offset = mCurrentItem - mMaxItems + 1;
		index += offset;
	}

	switch (Token.Type) {
	case tMenuTitle:
		return mTitle;

	case tMenuItem:
		if (index < 0)
			return false;

		if (mItems.size() <= (uint)index || mCurrentItem == (uint)index)
			return false;

		return (cxType)mItems[index];

	case tIsMenuItem:
		return mItems.size() > (uint)index && mCurrentItem != (uint)index;

	case tMenuCurrent:
		if (mItems.size() <= mCurrentItem)
			return false;

		if (index < 0)
			return (cxType)mItems[mCurrentItem];

		if (mItems.size() <= (uint)index || mCurrentItem != (uint)index)
			return false;

		return (cxType)mItems[index];

	case tIsMenuCurrent:
		return mItems.size() > (uint)index && mCurrentItem == (uint)index;

	case tAudioTrack:
		return mItems[mCurrentItem];

	case tAudioChannel:
		return ChannelName(mAudioChannel);

	default:
		return cText2SkinRender::GetTokenData(Token);
	}
}
