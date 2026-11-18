//								-*- c++ -*-

#ifndef VDR_TEXT2SKIN_MENU_H
#define VDR_TEXT2SKIN_MENU_H

#include "common.h"
#include "setup.h"
#include <vdr/menuitems.h>
#include <string>
#include <vector>

class cxSkin;

class cText2SkinSetupPage: public cMenuSetupPage {
private:
	cText2SkinSetup  mData;
	cxSkin          *mSkin; // current skin, if it is one of text2skin and has options

public:
	cText2SkinSetupPage(void);

	void Setup(void);
	virtual void Store(void);
	eOSState ProcessKey(eKeys Key);
};

// The options of the current skin, as a page of its own.
class cText2SkinOptionsPage: public cMenuSetupPage {
private:
	// edit buffer of an option of the skin
	struct tOptionEdit {
		int                      Index;  // in cxSkin::Options()
		int                      Value;  // bool, choice
		char                     String[256];
		std::vector<std::string> Labels; // translated values of a choice
		std::vector<const char*> LabelPtrs;
	};

	cxSkin                   *mSkin;
	std::vector<tOptionEdit*> mOptions;

public:
	cText2SkinOptionsPage(cxSkin *Skin);
	virtual ~cText2SkinOptionsPage();

	void Setup(void);
	virtual void Store(void);
};

#endif // VDR_TEXT2SKIN_MENU_H
