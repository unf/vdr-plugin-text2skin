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
	// edit buffer of an option of the current skin
	struct tOptionEdit {
		int                      Index;  // in cxSkin::Options()
		int                      Value;  // bool, choice
		char                     String[256];
		std::vector<std::string> Labels; // translated values of a choice
		std::vector<const char*> LabelPtrs;
	};

	cText2SkinSetup           mData;
	cxSkin                   *mSkin;
	std::vector<tOptionEdit*> mOptions;

public:
	cText2SkinSetupPage(void);
	virtual ~cText2SkinSetupPage();

	void Setup(void);
	virtual void Store(void);
	eOSState ProcessKey(eKeys Key);
};

#endif // VDR_TEXT2SKIN_MENU_H
