//								-*- c++ -*-

#include "xml/skin.h"
#include "i18n.h"
#include "setup.h"
#include <vdr/tools.h>
#include <vdr/config.h>
#include <vdr/device.h>

const std::string ScreenBases[] = { "relative", "absolute", "abs1280x720", "abs1920x1080" };

cxVersion::cxVersion(int ma, int min):
		mMajor(ma),
		mMinor(min)
{
}

bool cxVersion::Parse(const std::string &Text)
{
	int dot = Text.find(".");
	std::string ma(Text, 0, dot), min(Text, dot + 1);
	char *e = NULL;
	const char *t = NULL;
	long l = 0;

	t = ma.c_str();
	l = strtol(t, &e, 10);
	if (e == t || *e != '\0')
		return false;
	else
		mMajor = l;

	t = min.c_str();
	l = strtol(t, &e, 10);
	if (e == t || *e != '\0')
		return false;
	else
		mMinor = l;

	return true;
}

cxSkin::cxSkin(const std::string &Name, cText2SkinI18n *I18n, cText2SkinTheme *Theme):
		mName(Name),
		mScale(1.0),
		mI18n(I18n),
		mTheme(Theme)
{
}

// Skins with a fixed HD base are scaled to the real OSD size of the primary
// device, e.g. factor 2.0 for a 1920x1080 skin on a 3840x2160 OSD. With an
// OSD of the base size the factor is 1.0 and nothing changes.
void cxSkin::SetScaledBase(int Width, int Height)
{
	int osdWidth = 0, osdHeight = 0;
	double aspect = 1.0;
	cDevice *dev = cDevice::PrimaryDevice();
	if (dev)
		dev->GetOsdSize(osdWidth, osdHeight, aspect);

	mScale = 1.0;
	if (osdWidth > 0 && osdHeight > 0) {
		double sx = (double)osdWidth / Width;
		double sy = (double)osdHeight / Height;
		mScale = sx < sy ? sx : sy;
	}
	mBaseOffset = txPoint(0, 0);
	mBaseSize   = txSize((int)(Width * mScale + 0.5), (int)(Height * mScale + 0.5));
}

void cxSkin::SetBase(eScreenBase Base)
{
	if (Base != (eScreenBase)-1)
		mBase = Base;

	switch (mBase) {
	case relative:
		mScale      = 1.0;
		mBaseOffset = txPoint(Setup.OSDLeft, Setup.OSDTop);
		mBaseSize   = txSize(Setup.OSDWidth, Setup.OSDHeight);
		break;

	case absolute:
		mScale      = 1.0;
		mBaseOffset = txPoint(0, 0);
		mBaseSize   = txSize(720, 576); //XXX
		break;

	case abs1280x720:
		SetScaledBase(1280, 720);
		break;

	case abs1920x1080:
		SetScaledBase(1920, 1080);
		break;

	default:
		break;
	}
}

bool cxSkin::ParseBase(const std::string &Text)
{
	int i;
	for (i = 0; i < (int)__COUNT_BASE__; ++i) {
		if (ScreenBases[i] == Text)
			break;
	}
	if (i < (int)__COUNT_BASE__) {
		SetBase((eScreenBase)i);
		return true;
	}
	return false;
}

std::string cxSkin::Translate(const std::string &Text)
{
	if (mI18n != NULL)
		return mI18n->Translate(Text);
	return Text;
}

// --- txOption ---------------------------------------------------------------

bool txOption::ParseType(const std::string &Text)
{
	if      (Text == "bool")   Type = oBool;
	else if (Text == "choice") Type = oChoice;
	else if (Text == "string") Type = oString;
	else return false;
	return true;
}

bool txOption::ParseValues(const std::string &Text)
{
	Values.clear();
	std::string::size_type start = 0, pos;
	do {
		pos = Text.find(',', start);
		std::string value = Text.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
		if (value.empty())
			return false;
		Values.push_back(value);
		start = pos + 1;
	} while (pos != std::string::npos);
	return true;
}

// Brings a stored value into the form of the option type: "1"/"0" for bool,
// one of the values for choice (else the default), the text for string.
std::string txOption::Normalize(const std::string &Value) const
{
	switch (Type) {
	case oBool:
		return (Value != "" && Value != "0") ? "1" : "0";

	case oChoice:
		for (uint i = 0; i < Values.size(); ++i) {
			if (Values[i] == Value)
				return Value;
		}
		return Value == Default ? (Values.empty() ? "" : Values[0]) : Normalize(Default);

	default:
		return Value;
	}
}

cxType cxSkin::OptionValue(const std::string &Name) const
{
	for (txOptions::const_iterator it = mOptions.begin(); it != mOptions.end(); ++it) {
		if (it->Name == Name) {
			std::string value = it->Normalize(Text2SkinOptions.Get(mName, Name, it->Default));
			if (it->Type == txOption::oBool)
				return (cxType)(value == "1");
			return (cxType)value;
		}
	}
	return false;
}
