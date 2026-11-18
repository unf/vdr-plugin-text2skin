//								-*- c++ -*-

#include "menu.h"
#include "bitmap.h"
#include "render.h"
#include "loader.h"
#include "xml/skin.h"
#include <vdr/plugin.h>

// --- cText2SkinSetupPage ----------------------------------------------------

cText2SkinSetupPage::cText2SkinSetupPage(void):
	mData(Text2SkinSetup),
	mSkin(NULL)
{
	// the current skin gets a page for its options, if it is one of text2skin and has some
	cText2SkinLoader *loader = dynamic_cast<cText2SkinLoader*>(Skins.Current());
	if (loader != NULL && loader->Data() != NULL && !loader->Data()->Options().empty())
		mSkin = loader->Data();
	Setup();
}

void cText2SkinSetupPage::Setup(void) {
	int current = Current();
	Clear();

	Add(new cMenuEditBoolItem(tr("Show scrollbar in the menus"), &mData.MenuScrollbar));
	Add(new cMenuEditBoolItem(tr("Scrolling behaviour"), &mData.MarqueeLeftRight, tr("to the left"), tr("left and right")));
	Add(new cMenuEditBoolItem(tr("Reset Marquee for new item"), &mData.MarqueeReset));
	Add(new cMenuEditBoolItem(tr("Show auxiliary infos of recordings"), &mData.ShowAux));
	if (mData.ShowAux)
		// TRANSLATORS: note the leading blanks!
		Add(new cMenuEditBoolItem(tr("   Extract known tags"), &mData.StripAux));
	Add(new cMenuEditBoolItem(tr("Use 'epgsearch' to check timer-conflicts"), &mData.CheckTimerConflict));
	Add(new cOsdItem(tr("Flush image cache"), osUser1));
	Add(new cMenuEditIntItem(tr("Max. image cache size"), &mData.MaxCacheFill, 1));
	Add(new cMenuEditIntItem(tr("Max. characters per line"), &mData.MaxChars, 1));
	Add(new cMenuEditBoolItem(tr("Tabulator width"), &mData.TabWidth, tr("narrow"), tr("wide")));

	if (mSkin != NULL) {
		Add(new cOsdItem("", osUnknown, false));
		Add(new cOsdItem(cString::sprintf("%s...", *cString::sprintf(tr("Options of skin '%s'"), mSkin->Title().c_str())), osUser2));
	}

	SetCurrent(Get(current));
	Display();
}

void cText2SkinSetupPage::Store(void) {
	SetupStore("MenuScrollbar", mData.MenuScrollbar);
	SetupStore("MarqueeLeftRight", mData.MarqueeLeftRight);
	SetupStore("MarqueeReset", mData.MarqueeReset);
	SetupStore("ShowAux", mData.ShowAux);
	SetupStore("StripAux", mData.StripAux);
	SetupStore("CheckTimerConflict", mData.CheckTimerConflict);
	SetupStore("MaxCacheFill", mData.MaxCacheFill);
	SetupStore("MaxChars", mData.MaxChars);
	SetupStore("TabWidth", mData.TabWidth);
	Text2SkinSetup = mData;
}

eOSState cText2SkinSetupPage::ProcessKey(eKeys Key) {
	int oldShowAux = mData.ShowAux;

	eOSState state = cMenuSetupPage::ProcessKey(Key);
	if (state == osUser1) {
		Skins.Message(mtInfo, tr("Flushing image cache..."));
		cText2SkinBitmap::FlushCache();
		cText2SkinRender::FlushImagePaths();
		Skins.Message(mtInfo, NULL);
		return osContinue;
	}
	if (state == osUser2 && mSkin != NULL)
		return AddSubMenu(new cText2SkinOptionsPage(mSkin));

	if (mData.ShowAux != oldShowAux)
		Setup();

	return state;
}

// --- cText2SkinOptionsPage --------------------------------------------------

cText2SkinOptionsPage::cText2SkinOptionsPage(cxSkin *Skin):
	mSkin(Skin)
{
	// stores into the setup of text2skin
	cPlugin *plugin = cPluginManager::GetPlugin("text2skin");
	if (plugin != NULL)
		SetPlugin(plugin);
	// "Setup - Options of skin 'X'"
	SetSection(cString::sprintf(tr("Options of skin '%s'"), mSkin->Title().c_str()));

	const txOptions &options = mSkin->Options();
	for (uint i = 0; i < options.size(); ++i) {
		const txOption &option = options[i];
		std::string value = option.Normalize(Text2SkinOptions.Get(mSkin->Name(), option.Name, option.Default));
		tOptionEdit *edit = new tOptionEdit;
		edit->Index = i;
		edit->Value = 0;
		edit->String[0] = '\0';
		switch (option.Type) {
		case txOption::oBool:
			edit->Value = value == "1";
			break;

		case txOption::oChoice:
			for (uint v = 0; v < option.Values.size(); ++v) {
				if (option.Values[v] == value)
					edit->Value = v;
				edit->Labels.push_back(mSkin->Translate(option.Values[v]));
			}
			for (uint v = 0; v < edit->Labels.size(); ++v)
				edit->LabelPtrs.push_back(edit->Labels[v].c_str());
			break;

		case txOption::oString:
			strn0cpy(edit->String, value.c_str(), sizeof(edit->String));
			break;
		}
		mOptions.push_back(edit);
	}
	Setup();
}

cText2SkinOptionsPage::~cText2SkinOptionsPage() {
	for (uint i = 0; i < mOptions.size(); ++i)
		delete mOptions[i];
}

void cText2SkinOptionsPage::Setup(void) {
	int current = Current();
	Clear();

	for (uint i = 0; i < mOptions.size(); ++i) {
		tOptionEdit *edit = mOptions[i];
		const txOption &option = mSkin->Options()[edit->Index];
		std::string title = mSkin->Translate(option.Title.empty() ? option.Name : option.Title);
		switch (option.Type) {
		case txOption::oBool:
			Add(new cMenuEditBoolItem(title.c_str(), &edit->Value));
			break;

		case txOption::oChoice:
			Add(new cMenuEditStraItem(title.c_str(), &edit->Value, edit->LabelPtrs.size(), &edit->LabelPtrs[0]));
			break;

		case txOption::oString:
			Add(new cMenuEditStrItem(title.c_str(), edit->String, sizeof(edit->String)));
			break;
		}
	}

	SetCurrent(Get(current));
	Display();
}

void cText2SkinOptionsPage::Store(void) {
	// only values different from the default go to setup.conf
	for (uint i = 0; i < mOptions.size(); ++i) {
		tOptionEdit *edit = mOptions[i];
		const txOption &option = mSkin->Options()[edit->Index];
		std::string value;
		switch (option.Type) {
		case txOption::oBool:   value = edit->Value ? "1" : "0"; break;
		case txOption::oChoice: value = option.Values[edit->Value]; break;
		case txOption::oString: value = edit->String; break;
		}
		std::string key = mSkin->Name() + "." + option.Name;
		if (value == option.Default) {
			SetupStore(key.c_str());
			Text2SkinOptions.Reset(mSkin->Name(), option.Name);
		} else {
			SetupStore(key.c_str(), value.c_str());
			Text2SkinOptions.Set(mSkin->Name(), option.Name, value);
		}
	}
}
