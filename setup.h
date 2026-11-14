//								-*- c++ -*-

#ifndef VDR_TEXT2SKIN_SETUP_H
#define VDR_TEXT2SKIN_SETUP_H

#include "common.h"
#include <vdr/thread.h>
#include <map>
#include <string>

class cText2SkinSetup {
public:
	cText2SkinSetup(void);

	bool SetupParse(const char *Name, const char *Value);

	int MenuScrollbar;
	int MarqueeLeftRight;
	int MarqueeReset;
	int ShowAux;
	int StripAux;
	int CheckTimerConflict;
	int MaxCacheFill;
	int MaxChars;
	int TabWidth;
};

extern cText2SkinSetup Text2SkinSetup;

// Values of the skin options, stored in setup.conf as text2skin.<Skin>.<Name>.
// Read by the render threads, changed by the setup menu, hence the lock.
class cText2SkinOptions {
private:
	mutable cMutex                     mMutex;
	std::map<std::string, std::string> mValues;

public:
	bool Parse(const char *Name, const char *Value);
	std::string Get(const std::string &Skin, const std::string &Name, const std::string &Default) const;
	void Set(const std::string &Skin, const std::string &Name, const std::string &Value);
	void Reset(const std::string &Skin, const std::string &Name);
};

extern cText2SkinOptions Text2SkinOptions;

#endif // VDR_TEXT2SKIN_SETUP_H
