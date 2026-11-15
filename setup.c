//								-*- c++ -*-

#include "setup.h"

cText2SkinSetup Text2SkinSetup;
cText2SkinOptions Text2SkinOptions;

// --- cText2SkinSetup --------------------------------------------------------

cText2SkinSetup::cText2SkinSetup(void):
	MenuScrollbar(false),
	MarqueeLeftRight(true),
	MarqueeReset(false),
	ShowAux(true),
	StripAux(true),
	CheckTimerConflict(false),
	MaxCacheFill(25),
	MaxChars(100),
	TabWidth(false)
{
}

bool cText2SkinSetup::SetupParse(const char *Name, const char *Value) {
	if      (strcmp(Name, "MenuScrollbar") == 0) MenuScrollbar = atoi(Value);
	else if (strcmp(Name, "MarqueeLeftRight") == 0) MarqueeLeftRight = atoi(Value);
	else if (strcmp(Name, "MarqueeReset") == 0) MarqueeReset = atoi(Value);
	else if (strcmp(Name, "ShowAux") == 0) ShowAux = atoi(Value);
	else if (strcmp(Name, "StripAux") == 0) StripAux = atoi(Value);
	else if (strcmp(Name, "CheckTimerConflict") == 0) CheckTimerConflict = atoi(Value);
	else if (strcmp(Name, "MaxCacheFill") == 0) MaxCacheFill = atoi(Value);
	else if (strcmp(Name, "MaxChars") == 0) MaxChars = atoi(Value);
	else if (strcmp(Name, "TabWidth") == 0) TabWidth = atoi(Value);
	else return Text2SkinOptions.Parse(Name, Value);
	return true;
}

// --- cText2SkinOptions ------------------------------------------------------

bool cText2SkinOptions::Parse(const char *Name, const char *Value)
{
	// "<Skin>.<Name>"
	if (strchr(Name, '.') == NULL)
		return false;
	cMutexLock lock(&mMutex);
	mValues[Name] = Value;
	return true;
}

std::string cText2SkinOptions::Get(const std::string &Skin, const std::string &Name,
                                   const std::string &Default) const
{
	cMutexLock lock(&mMutex);
	std::map<std::string, std::string>::const_iterator it = mValues.find(Skin + "." + Name);
	return it != mValues.end() ? it->second : Default;
}

void cText2SkinOptions::Set(const std::string &Skin, const std::string &Name, const std::string &Value)
{
	cMutexLock lock(&mMutex);
	mValues[Skin + "." + Name] = Value;
}

void cText2SkinOptions::Reset(const std::string &Skin, const std::string &Name)
{
	cMutexLock lock(&mMutex);
	mValues.erase(Skin + "." + Name);
}

