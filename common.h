//								-*- c++ -*-

#ifndef VDR_TEXT2SKIN_COMMON_H
#define VDR_TEXT2SKIN_COMMON_H

#include "xml/string.h"
#include <string>
#include <vector>
#include <vdr/osd.h>
#include <vdr/config.h>
#include <vdr/epg.h>
#include <vdr/recording.h>
#include <vdr/timers.h>
#include <vdr/channels.h>
#include <vdr/thread.h>

// The update thread must never wait for VDR's global lists: VDR may hold one
// of them for writing while it calls the display and waits for the update
// thread, e.g. when a timer starts a recording and reports it. So the update
// thread tries a short time and draws again a little later if a list is busy.
#define LIST_LOCK_TIMEOUT 20  // ms
#define LIST_LOCK_RETRY   100 // ms

// Read lock on one of VDR's lists for the scope of the object, e.g.
//   cListReader<cTimers> Timers(cTimers::GetTimersRead);
//   if (!Timers) ... busy ...
template<class T> class cListReader {
private:
	cStateKey mKey;
	const T  *mList;
	cListReader(const cListReader &);
	cListReader &operator=(const cListReader &);
public:
	cListReader(const T *(*Get)(cStateKey &, int)): mList(Get(mKey, LIST_LOCK_TIMEOUT)) {}
	~cListReader() { if (mList) mKey.Remove(); }
	operator bool(void) const { return mList != NULL; }
	const T *operator->(void) const { return mList; }
};

#if defined(DEBUG) || defined(BENCH)
#	ifdef DEBUG
#		define Dprintf(x...) fprintf(stderr, x)
#	else
#		define Dprintf(x...)
#	endif
#	define __STDC_FORMAT_MACROS
#	define Dbench(x) uint64_t bench_##x = cTimeMs::Now()
#	define Ddiff(t,x) fprintf(stderr, "%s took %" PRIu64 " ms\n", t, cTimeMs::Now() - bench_##x)
#else
#	define Dprintf(x...)
#	define Dbench(x)
#	define Ddiff(t,x)
#endif

/* Check if a channel is a radio station. */
#define ISRADIO(x) ((x)->Vpid()==0||(x)->Vpid()==1||(x)->Vpid()==0x1fff)

// class forwards

class cMarks;
class cChannel;
class cRecording;

// Copies of VDR's channel, event and recording data. The update thread draws
// after VDR has called the display, when VDR's objects may already be changed
// or deleted (EPG updates replace texts, events and recordings get removed).
// So the displays copy what they show while VDR calls them.

struct tComponentData {
	uchar       stream;
	uchar       type;
	std::string language;
	std::string description;
};
typedef std::vector<tComponentData> tComponents;

struct tChannelData {
	bool        valid; // false: no channel, maybe a number being entered
	std::string number;
	std::string name;
	std::string shortName;
	std::string serviceReference;
	std::string provider;
	std::string portal;
	std::string source;
	std::string channelID;
	bool        hasTeletext;
	bool        hasMultilang;
	bool        hasDolby;
	bool        isEncrypted;
	bool        isRadio;

	tChannelData(void): valid(false), hasTeletext(false), hasMultilang(false), hasDolby(false),
	                    isEncrypted(false), isRadio(false) {}
	void Set(const cChannel *Channel, int Number);
};

struct tEventData {
	bool        valid;
	std::string title;
	std::string shortText;
	std::string description;
	time_t      startTime;
	time_t      endTime;
	time_t      vps;
	int         duration;
	bool        hasTimer;
	bool        isRunning;
	tEventID    eventID;
	tChannelID  channelID;
	tComponents components;
	std::string poster;      // image of a scraper plugin (path), "" if none

	tEventData(void): valid(false), startTime(0), endTime(0), vps(0), duration(0),
	                  hasTimer(false), isRunning(false), eventID(0) {}
	void Set(const cEvent *Event);
	// what a display shows of an event
	bool operator!=(const tEventData &b) const {
		return valid != b.valid || title != b.title || shortText != b.shortText
		    || description != b.description || startTime != b.startTime
		    || duration != b.duration || vps != b.vps || hasTimer != b.hasTimer
		    || isRunning != b.isRunning || poster != b.poster;
	}
};

struct tRecordingData {
	bool        valid;
	std::string name;
	std::string fileName;
	std::string title;
	std::string shortText;
	std::string description;
	std::string aux;
	int         priority;
	int         lifetime;
	time_t      start;
	double      framesPerSecond;
	bool        isPesRecording;
	tChannelID  channelID;
	tComponents components;
	std::string poster;      // image of a scraper plugin (path), "" if none

	tRecordingData(void): valid(false), priority(0), lifetime(0), start(0),
	                      framesPerSecond(DEFAULTFRAMESPERSECOND), isPesRecording(false) {}
	void Set(const cRecording *Recording);
};

// tokens from the components of an event or a recording
cxType ComponentLanguageCode(const tComponents &Components, int Number);
cxType ComponentLanguageDescription(const tComponents &Components, int Number);
cxType ComponentVideoAR(const tComponents &Components);

// helper functions

const std::string SkinPath(void);
const char *ChannelNumber(const cChannel *Channel, int Number);
const char *ChannelName(const cChannel *Channel, int Number);
const char *ChannelShortName(const cChannel *Channel, int Number);
const char *ChannelServiceReference(const cChannel *Channel, int Number);
const char *EventType(uint Number);
//const char *ChannelBouquet(const cChannel *Channel, int Number);

int GetFrontendSTR(void); // Signal strength [%]
int GetFrontendSNR(void); // Signal to Noise ratio [%]
bool GetFrontendHasLock(void);
bool GetFrontendHasSignal(void);
std::string AddExtInfoToDescription(const char *Title, const char *ShortText, const char *Description, const char *Aux = NULL, bool StripAux = false);
int GetRecordingLength(const char *FileName, double FramesPerSecond, bool IsPesRecording); // [min]
int GetRecordingCuttedLength(const char *FileName, double FramesPerSecond, bool IsPesRecording); // [min]

cxType TimeType(time_t Time, const std::string &Format);
cxType DurationType(uint Index, double FramesPerSecond, const std::string &Format);
cxType DurationType(int Seconds, const std::string &Format, int Frame = 0);

bool ParseVar(const char *Text, const char *Name, std::string &Value);
bool ParseVar(const char *Text, const char *Name, tColor *Value);

void SkipQuotes(std::string &Value);
std::string FitToWidth(std::string &Line, uint Width);
std::string FitToWidth(std::stringstream &Line, uint Width);
std::string StripXmlTag(std::string &Line, const char *Tag);

// Data structure for service "Epgsearch-searchresults-v1.0"
struct Epgsearch_searchresults_v1_0 {
// in
	char* query;               // search term
	int mode;                  // search mode (0=phrase, 1=and, 2=or, 3=regular expression)
	int channelNr;             // channel number to search in (0=any)
	bool useTitle;             // search in title
	bool useSubTitle;          // search in subtitle
	bool useDescription;       // search in description
// out

	class cServiceSearchResult : public cListObject {
	public:
		const cEvent* event;
		cServiceSearchResult(const cEvent* Event) : event(Event) {}
	};

	cList<cServiceSearchResult>* pResultList;   // pointer to the results
};

// Data structure for service "Epgsearch-lastconflictinfo-v1.0"
struct Epgsearch_lastconflictinfo_v1_0 {
// in
// out
	time_t nextConflict;       // next conflict date, 0 if none
	int relevantConflicts;     // number of relevant conflicts
	int totalConflicts;        // total number of conflicts
};

#endif // VDR_TEXT2SKIN_COMMON_H
