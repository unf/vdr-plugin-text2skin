//								-*- c++ -*-

#ifndef VDR_TEXT2SKIN_XML_SKIN_H
#define VDR_TEXT2SKIN_XML_SKIN_H

#include "xml/display.h"
#include "xml/type.h"
#include <vdr/osd.h>
#include <map>
#include <string>
#include <vector>

// --- cxSkin -----------------------------------------------------------------

class cText2SkinI18n;
class cText2SkinTheme;

class cxVersion {
public:
	cxVersion(int ma = 0, int min = 0);
	bool Parse(const std::string &Text);
	int Major(void) const { return mMajor; }
	int Minor(void) const { return mMinor; }
	bool Require(int ma, int min) const {
		return mMajor > ma ? true : (mMajor == ma ? mMinor >= min : false);
	}
	bool Limit(int ma, int min) const {
		return mMajor < ma ? true : (mMajor == ma ? mMinor <= min : false);
	}
	bool operator==(const cxVersion &v) const {
		return mMajor == v.mMajor && mMinor == v.mMinor;
	}
	bool operator>=(const cxVersion &v) const {
		return Require(v.mMajor, v.mMinor);
	}
	bool operator>=(const char *c) const {
		cxVersion v;
		if (!v.Parse(c))
			return false;
		return Require(v.mMajor, v.mMinor);
	}
	bool operator<=(const cxVersion &v) const {
		return Limit(v.mMajor, v.mMinor);
	}
	bool operator<=(const char *c) const {
		cxVersion v;
		if (!v.Parse(c))
			return false;
		return Limit(v.mMajor, v.mMinor);
	}

private:
	int mMajor;
	int mMinor;
};

// --- txOption ---------------------------------------------------------------

// An option a skin declares in <options>. The value is set in the setup menu
// of text2skin, stored in setup.conf as text2skin.<Skin>.<Name> and read in
// the skin with the token {SkinOption:<Name>}.
struct txOption {
	enum eType {
		oBool,
		oChoice,
		oString
	};

	std::string              Name;
	eType                    Type;
	std::string              Title;
	std::string              Default;
	std::vector<std::string> Values;

	txOption(void): Type(oBool) {}

	bool ParseType(const std::string &Text);
	bool ParseValues(const std::string &Text);
	std::string Normalize(const std::string &Value) const;
};

typedef std::vector<txOption> txOptions;

class cxSkin {
	friend bool xStartElem(const std::string &name, std::map<std::string,std::string> &attrs);
	friend bool xEndElem(const std::string &name);

	/* Skin Editor */
	friend class VSkinnerView;

public:
	enum eScreenBase {
		relative,
		absolute,
		abs1280x720,
		abs1920x1080,
#define __COUNT_BASE__ (abs1920x1080 + 1)
	};

private:
	eScreenBase      mBase;
	txPoint          mBaseOffset;
	txSize           mBaseSize;
	std::string      mName;
	double           mScale;
	std::string      mTitle;
	cxVersion        mVersion;

	cxDisplays       mDisplays;
	txOptions        mOptions;

	cText2SkinI18n  *mI18n; // TODO: should move here completely
	cText2SkinTheme *mTheme;

	void SetScaledBase(int Width, int Height);

public:
	cxSkin(const std::string &Name, cText2SkinI18n *I18n, cText2SkinTheme *Theme);

	cxDisplay *Get(cxDisplay::eType Type);

	bool ParseBase(const std::string &Text);
	void SetBase(eScreenBase Base = (eScreenBase)-1);

	eScreenBase        Base(void)       const { return mBase; }
	const txPoint     &BaseOffset(void) const { return mBaseOffset; }
	const txSize      &BaseSize(void)   const { return mBaseSize; }
	double             Scale(void)      const { return mScale; }
	const std::string &Name(void)       const { return mName; }
	const std::string &Title(void)      const { return mTitle; }
	const cxVersion   &Version(void)    const { return mVersion; }
	const txOptions   &Options(void)    const { return mOptions; }

	// current value of an option (setup.conf or default of the skin)
	cxType             OptionValue(const std::string &Name) const;

	// functions for object classes to obtain dynamic item information
	std::string        Translate(const std::string &Text);
};

inline cxDisplay *cxSkin::Get(cxDisplay::eType Type) {
	if (mDisplays.find(Type) != mDisplays.end())
		return mDisplays[Type];
	return NULL;
}

#endif // VDR_TEXT2SKIN_XML_SKIN_H
