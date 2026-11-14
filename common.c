//								-*- c++ -*-

#include "common.h"
#include "setup.h"
#include <sstream>
#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/sources.h>
#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

const std::string SkinPath(void)
{
	// should never change
	static cString path;
	if ((*path == NULL) || (strlen(*path) == 0))
		path = cPlugin::ConfigDirectory(PLUGIN_NAME_I18N);
	return std::string(*path);
}

const char *ChannelNumber(const cChannel *Channel, int Number)
{
	static char buffer[256];
	buffer[0] = '\0';
  if (Channel) {
     if (!Channel->GroupSep())
        snprintf(buffer, sizeof(buffer), "%d%s", Channel->Number(), Number ? "-" : "");
     }
  else if (Number)
     snprintf(buffer, sizeof(buffer), "%d-", Number);
  return buffer;
}

const char *ChannelName(const cChannel *Channel, int Number)
{
	static char buffer[256];
	buffer[0] = '\0';
	if (Channel)
		snprintf(buffer, sizeof(buffer), "%s", Channel->Name());
	else if (!Number)
		snprintf(buffer, sizeof(buffer), "%s", trVDR("*** Invalid Channel ***"));
	return buffer;
}

const char *ChannelShortName(const cChannel *Channel, int Number)
{
	static char buffer[256];
	buffer[0] = '\0';
	if (Channel)
		snprintf(buffer, sizeof(buffer), "%s", Channel->ShortName(true));
	else if (!Number)
		snprintf(buffer, sizeof(buffer), "%s", trVDR("*** Invalid Channel ***"));
	return buffer;
}

const char *ChannelServiceReference(const cChannel *Channel, int Number)
{
	static char buffer[256];
	int hash = 0;
	buffer[0] = '\0';
	if (Channel) {
		if (cSource::IsSat(Channel->Source())) {
			hash = Channel->Source() & cSource::st_Pos;
			if (hash > 0x00007FFF)
				hash |= 0xFFFF0000;

			if (hash < 0)
				hash = -hash;
			else
				hash = 1800 + hash;

			hash = hash << 16;
		}
		else if (cSource::IsCable(Channel->Source()))
			hash = 0xFFFF0000;
		else if(cSource::IsTerr(Channel->Source()))
			hash = 0xEEEE0000;
		else if(cSource::IsAtsc(Channel->Source()))
			hash = 0xDDDD0000;

		snprintf(buffer, sizeof(buffer), "1_0_%i_%X_%X_%X_%X_0_0_0",
                (!(Channel->Vpid() == 0 && Channel->Apid(0) == 0) && (Channel->Vpid() == 0 || Channel->Vpid() == 1)) ? 2 : (Channel->Vtype() == 27) ? 19 : 1,
				Channel->Sid(),
				Channel->Tid(),
				Channel->Nid(),
				hash);
	}
	else if (!Number)
		snprintf(buffer, sizeof(buffer), "%s", trVDR("*** Invalid Channel ***"));
	return buffer;
}

void tChannelData::Set(const cChannel *Channel, int Number)
{
	*this = tChannelData();
	number = ChannelNumber(Channel, Number);
	if (Channel == NULL)
		return;
	valid            = true;
	name             = ChannelName(Channel, Number);
	shortName        = ChannelShortName(Channel, Number);
	serviceReference = ChannelServiceReference(Channel, Number);
	provider         = Channel->Provider();
	portal           = Channel->PortalName();
	if (const cSource *s = Sources.Get(Channel->Source()))
		source = s->Description();
	channelID        = *Channel->GetChannelID().ToString();
	hasTeletext      = Channel->Tpid() != 0;
	hasMultilang     = Channel->Apid(1) != 0;
	hasDolby         = Channel->Dpid(0) != 0;
	isEncrypted      = Channel->Ca() != 0;
	isRadio          = ISRADIO(Channel);
}

static void CopyComponents(tComponents &Dest, const cComponents *Components)
{
	Dest.clear();
	if (Components == NULL)
		return;
	for (int i = 0; i < Components->NumComponents(); i++) {
		const tComponent *c = Components->Component(i);
		tComponentData d;
		d.stream      = c->stream;
		d.type        = c->type;
		d.language    = c->language;
		d.description = c->description ? c->description : "";
		Dest.push_back(d);
	}
}

// Service "GetPoster" of a scraper plugin (tvscraper, scraper2vdr). The data
// structures must have the same layout as in their services.h.
namespace {
class cScraperMedia {
public:
	cScraperMedia(void): width(0), height(0) {}
	std::string path;
	int width;
	int height;
};

class cScraperGetPoster {
public:
	cScraperGetPoster(void): event(NULL), recording(NULL) {}
	const cEvent *event;
	const cRecording *recording;
	cScraperMedia poster;
};
}

// Path of the poster (or EPG image) of an event or a recording, "" if there is
// none or no scraper plugin. Called where VDR hands over the event or
// recording, the pointers are valid there.
static std::string ScraperPoster(const cEvent *Event, const cRecording *Recording)
{
	static cPlugin *scraper = NULL;
	static bool searched = false;
	if (!searched) {
		scraper = cPluginManager::GetPlugin("tvscraper");
		if (scraper == NULL)
			scraper = cPluginManager::GetPlugin("scraper2vdr");
		searched = true;
	}
	if (scraper == NULL || (Event == NULL && Recording == NULL))
		return "";

	cScraperGetPoster call;
	call.event = Event;
	call.recording = Recording;
	if (!scraper->Service("GetPoster", &call))
		return "";
	return call.poster.path;
}

void tEventData::Set(const cEvent *Event)
{
	*this = tEventData();
	if (Event == NULL)
		return;
	valid       = true;
	title       = Event->Title() ? Event->Title() : "";
	shortText   = Event->ShortText() ? Event->ShortText() : "";
	description = Event->Description() ? Event->Description() : "";
	startTime   = Event->StartTime();
	endTime     = Event->EndTime();
	vps         = Event->Vps();
	duration    = Event->Duration();
	hasTimer    = Event->HasTimer();
	isRunning   = Event->IsRunning();
	eventID     = Event->EventID();
	channelID   = Event->ChannelID();
	CopyComponents(components, Event->Components());
	poster      = ScraperPoster(Event, NULL);
}

void tRecordingData::Set(const cRecording *Recording)
{
	*this = tRecordingData();
	if (Recording == NULL)
		return;
	const cRecordingInfo *info = Recording->Info();
	valid           = true;
	name            = Recording->Name() ? Recording->Name() : "";
	fileName        = Recording->FileName() ? Recording->FileName() : "";
	priority        = Recording->Priority();
	lifetime        = Recording->Lifetime();
	start           = Recording->Start();
	framesPerSecond = Recording->FramesPerSecond();
	isPesRecording  = Recording->IsPesRecording();
	if (info) {
		title       = info->Title() ? info->Title() : "";
		shortText   = info->ShortText() ? info->ShortText() : "";
		description = info->Description() ? info->Description() : "";
		aux         = info->Aux() ? info->Aux() : "";
		channelID   = info->ChannelID();
		CopyComponents(components, info->Components());
	}
	poster          = ScraperPoster(NULL, Recording);
}

// stream_content of the component descriptor (ETSI EN 300 468), as VDR
// stores it: 0x09 HEVC video and 0x19 AC-4 audio carry stream_content_ext
enum {
	scMpeg2Video = 0x01,
	scMpeg1Audio = 0x02,
	scAc3Audio   = 0x04,
	scH264Video  = 0x05,
	scHeAacAudio = 0x06,
	scHevcVideo  = 0x09,
	scAc4Audio   = 0x19
};

static bool IsAudio(uchar Stream)
{
	return Stream == scMpeg1Audio || Stream == scAc3Audio || Stream == scHeAacAudio
	    || Stream == scAc4Audio;
}

// Number counts audio components only: every other component before the
// wanted one moves the index on (the way text2skin always counted).
static const tComponentData *AudioComponent(const tComponents &Components, int Number)
{
	int index = Number;
	for (int i = 0; i < (int)Components.size(); i++) {
		if (!IsAudio(Components[i].stream))
			index++;
		if (i == index)
			return &Components[i];
	}
	return NULL;
}

cxType ComponentLanguageCode(const tComponents &Components, int Number)
{
	const tComponentData *c = AudioComponent(Components, Number);
	if (c == NULL)
		return false;
	std::string buffer(c->language);
	if (c->stream == scAc3Audio)
		buffer.append("DD");
	else if (c->stream == scMpeg1Audio || c->stream == scHeAacAudio) {
		if (c->type == 1)
			buffer.append("MONO");
		if ((c->type == 2) || (c->type == 4))
			buffer.append("DUAL");
		if (c->type == 5)
			buffer.append("DD");
	}
	return buffer;
}

cxType ComponentLanguageDescription(const tComponents &Components, int Number)
{
	const tComponentData *c = AudioComponent(Components, Number);
	return c != NULL ? (cxType)c->description : (cxType)false;
}

// "4:3" or "16:9" (wider formats count as 16:9) of the first video component
cxType ComponentVideoAR(const tComponents &Components)
{
	for (int i = 0; i < (int)Components.size(); i++) {
		const tComponentData &c = Components[i];
		switch (c.stream) {
		case scMpeg2Video: // 0x01/0x05/0x09 4:3 at 25/30 Hz and HD
			return c.type == 0x01 || c.type == 0x05 || c.type == 0x09 ? "4:3" : "16:9";
		case scH264Video:  // 0x01/0x05 4:3 SD at 25/30 Hz
			return c.type == 0x01 || c.type == 0x05 ? "4:3" : "16:9";
		case scHevcVideo:  // HD and UHD are always 16:9
			return "16:9";
		}
	}
	return false;
}

const char *EventType(uint Number)
{
	static char buffer[25];
	buffer[0] = '\0';
	snprintf(buffer, sizeof(buffer), "%d", Number);
	return buffer;
}

int GetFrontendSTR(void)
{
	int SignalStrength = cDevice::ActualDevice()->SignalStrength();
	if ( SignalStrength < 0)
	return 0;

	return SignalStrength;
}

int GetFrontendSNR(void)
{
	int SignalQuality = cDevice::ActualDevice()->SignalQuality();
	if ( SignalQuality < 0)
	return 0;

	return SignalQuality;
}

bool GetFrontendHasLock(void)
{
	fe_status_t value;
	cString dev = cString::sprintf(FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);

	int fe = open(dev, O_RDONLY | O_NONBLOCK);
	if (fe < 0)
		return false;
	CHECK(ioctl(fe, FE_READ_STATUS, &value));
	close(fe);

	return value & FE_HAS_LOCK;
}

bool GetFrontendHasSignal(void)
{
	fe_status_t value;
	cString dev = cString::sprintf(FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);

	int fe = open(dev, O_RDONLY | O_NONBLOCK);
	if (fe < 0)
		return false;
	CHECK(ioctl(fe, FE_READ_STATUS, &value));
	close(fe);

	return value & FE_HAS_SIGNAL;
}

// Called in the update thread, which holds timers, channels and schedules
// (see cListReader), so epgsearch and the channel lookup can't wait for VDR.
std::string AddExtInfoToDescription(const char *Title, const char *ShortText, const char *Description, const char *Aux, bool StripAux)
{
	// prepare the description
	std::stringstream desc("");

	if (!isempty(Description)) {
		// it seems that sometimes the description ends with a newline
		// and sometimes it does not
		std::string buf(Description);
		while (!buf.empty() && buf[buf.size() - 1] == '\n') buf.erase(buf.size() - 1);
		desc << buf << "\n"; // keep one newline
	}

	// try to find a rerun of the show using epgsearch-service
	if (!isempty(Title)) {
		Epgsearch_searchresults_v1_0 data;
		data.query = strdup(Title);
		data.mode = 0;
		data.channelNr = 0;
		data.useTitle = true;
		data.useSubTitle = false;
		data.useDescription = false;
		if (cPluginManager::CallFirstService("Epgsearch-searchresults-v1.0", &data)) {
			cList<Epgsearch_searchresults_v1_0::cServiceSearchResult>* list = data.pResultList;
			if (list) {
				if (!desc.str().empty()) desc << "\n";
				desc << tr("RERUNS OF THIS SHOW") << ":\n";
				int i = 0;
				for (Epgsearch_searchresults_v1_0::cServiceSearchResult *r =
				list->First(); r && i < 5; r = list->Next(r)) {
					i++;
					if (!r->event || r->event->StartTime() < time(NULL))
						continue;
					std::stringstream buf;
					LOCK_CHANNELS_READ; // never waits, the caller holds the channels
					const cChannel *channel = Channels->GetByChannelID(r->event->ChannelID(), true, true);
					if (!channel)
						continue;
					buf << "\n";
					buf << " - ";
					buf << channel->ShortName(true);
					buf << " - ";
					buf << *DayDateTime(r->event->StartTime());
					buf << " - " << r->event->Title();

					if (!isempty(r->event->ShortText())) buf << "~" << r->event->ShortText();
					desc << FitToWidth(buf, Text2SkinSetup.MaxChars) << "\n";
				}
				delete list;
			}
		}
	}

	// Add the AUX-Info of the Recording
	if (Aux) {
		if (StripAux) {
			std::string auxRaw(Aux);
			std::string auxEpgsearch = StripXmlTag(auxRaw, "epgsearch");
			if (!auxEpgsearch.empty()) {
				std::string searchTimer = StripXmlTag(auxEpgsearch, "searchtimer");
				if (searchTimer.empty())
					// epgsearch < 0.9.21?
					searchTimer = StripXmlTag(auxEpgsearch, "Search timer");
				if (!searchTimer.empty()) {
					if (!desc.str().empty())
						desc << "\n";
					desc << tr("AUXILIARY INFOS") << ":\n";
					std::stringstream buf;
					buf << " - " << tr("Search timer") << ": " << searchTimer;
					desc << FitToWidth(buf, Text2SkinSetup.MaxChars) << "\n";
				}
			}
		}
		else {
			if (!desc.str().empty())
				desc << "\n";
			desc << tr("AUXILIARY INFOS") << ":\n";
			desc << Aux << "\n";
		}
	}

	return desc.str();
}

int GetRecordingLength(const char *FileName, double FramesPerSecond, bool IsPesRecording)
{
#define INDEXFILESUFFIX     "/index"
	struct tIndex { int offset; uchar type; uchar number; short reserved; };
	struct stat buf;
	cString fullname = cString::sprintf("%s%s", FileName, IsPesRecording ? INDEXFILESUFFIX ".vdr" : INDEXFILESUFFIX);
	if (FileName && *fullname && access(fullname, R_OK) == 0 && stat(fullname, &buf) == 0)
		return buf.st_size ? ((buf.st_size - 1) / sizeof(tIndex) + 1) / (60 * FramesPerSecond) : 0;
	return 0;
}

int GetRecordingCuttedLength(const char *FileName, double FramesPerSecond, bool IsPesRecording)
{
	cMarks marks;
	double length = 0;
	int totalLength = GetRecordingLength(FileName, FramesPerSecond, IsPesRecording);
	const double diffIFrame = FramesPerSecond / 2; // approx. 1/2 sec.

	marks.Load(FileName, FramesPerSecond, IsPesRecording);

	if (marks.Count()) {
		int start = 1; // first frame
		bool isStart = true;

		for (cMark *m = marks.First(); m; m = marks.GetNext(m->Position())) {
			if (isStart)
			        start = m->Position();
			else
				length += (double)(m->Position() - start + 1 + diffIFrame) / (60 * FramesPerSecond); // [min]

			isStart = !isStart;
		}

		// if there is no end-mark the last segment goes to the end of the rec.
		if (!isStart)
			length += totalLength - (double)(start - 1 - diffIFrame) / (60 * FramesPerSecond); // [min]
	}

	// just to avoid, that the cutted length is bigger than the total length
	return (int)length > totalLength ? totalLength : (int)length;
}

cxType TimeType(time_t Time, const std::string &Format)
{
	static char result[1000];
	struct tm tm_r, *tm;
	tm = localtime_r(&Time, &tm_r);

	if (Time > 0) {
		if (Format.length() > 0) {
			strftime(result, sizeof(result), Format.c_str(), tm);

			cxType r = result;
			/*if      (Format.find("%s") != -1 || Format.find("%S") != -1
			      || Format.find("%Es") != -1 || Format.find("%ES") != -1
			      || Format.find("%Os") != -1 || Format.find("%OS") != -1)
				r.SetUpdate(1000);
			else if (Format.find("%M") != -1 || Format.find("%EM") != -1
			      || Format.find("%OM") != -1)
				r.SetUpdate(1000*60);*/

			return r;
		} else
			return Time;
	}
	return false;
}

cxType DurationType(uint Index, double FramesPerSecond, const std::string &Format)
{
	if (FramesPerSecond && Format.length() > 0) {
		double Seconds = (Index + 0.5) / FramesPerSecond;
		int Frame = int((Seconds - int(Seconds)) * FramesPerSecond) + 1;
		return DurationType(int(Seconds), Format, Frame);
	}
	return int(Index);
}

cxType DurationType(int Seconds, const std::string &Format, int Frame)
{
	static char result[1000];
	if (Seconds >= 0) {
		if (Format.length() > 0) {
			uint update = 0;
			const char *ptr = Format.c_str();
			char *res = result;
			enum { normal, format } state = normal;
			int n = 0;
			int s = Seconds;
			int m = s / 60 % 60;
			int h = s / 3600;
			s %= 60;
			while (*ptr && res < result + sizeof(result)) {
				switch (state) {
				case normal:
					if (*ptr == '%')
						state = format;
					else
						*(res++) = *ptr;
					break;

				case format:
					switch (*ptr) {
					case 'H':
						n = snprintf(res, sizeof(result) - (res - result), "%02d", h);
						break;

					case 'k':
						n = snprintf(res, sizeof(result) - (res - result), "% 2d", h);
						break;

					case 'M':
						n = snprintf(res, sizeof(result) - (res - result), "%02d", m);
						update = 1000*60;
						break;

					case 'm':
						n = snprintf(res, sizeof(result) - (res - result), "%d", m + (h * 60));
						update = 1000*60;
						break;

					case 'S':
						n = snprintf(res, sizeof(result) - (res - result), "%02d", s);
						update = 1000;
						break;

					case 'f':
						if (Frame)
							n = snprintf(res, sizeof(result) - (res - result), "%02d", Frame);
						update = 1000;
						break;

					case '%':
						n = 1;
						*res = '%';
						break;
					}
					res += n;
					state = normal;
					break;
				}
				++ptr;
			}

			cxType r = result;
			r.SetUpdate(update);
			return r;
		} else
			return Seconds;
	}
	return false;
}

bool ParseVar(const char *Text, const char *Name, std::string &Value)
{
	const char *ptr1, *ptr2;
	char *str;
	bool res = false;
	if (asprintf(&str, ",%s=", Name) == -1)
		return res;
	if ((ptr1 = strstr(Text, str)) || (strncmp(ptr1 = Text, str + 1, strlen(str) - 1) == 0)) {
		if (ptr1 == Text)
			--ptr1;
		ptr1 += strlen(str);
		if ((ptr2 = strchr(ptr1, ',')) || (ptr2 = strchr(ptr1, ';'))) {
			int pos;
			Value = ptr1;
			Value.erase(ptr2 - ptr1);
			if (Value[0] == '"')
				SkipQuotes(Value);
			while ((pos = Value.find('$')) != -1)
				Value.replace(pos, 1, "{*}");
			res = true;
		}
	}
	free(str);
	return res;
}

bool ParseVar(const char *Text, const char *Name, tColor *Value) {
	std::string value;
	if (ParseVar(Text, Name, value) && value[0] == '#') {
		*Value = (tColor)strtoul(value.c_str() + 1, NULL, 16);
		return true;
	}
	return false;
}

void SkipQuotes(std::string &Value) {
	char quote = Value[0];
	int i;
	Value.erase(0, 1);
	for (i = 0; i < (int)Value.length() && Value[i] != quote; ++i) {
		if (Value[i] == '\\')
			Value.erase(i, 1);
	}
	if (Value[i] == quote)
		Value.erase(i, 1);
	else
		esyslog("ERROR: text2skin: missing closing %c", quote);
}

std::string FitToWidth(std::string &Line, uint Width)
{
	std::string buf(Line);
	if (buf.size() > Width) {
		buf.erase(Width - 3);
		buf.append("...");
	}
	return buf;
}

std::string FitToWidth(std::stringstream &Line, uint Width)
{
	std::string buf(Line.str());
	if (buf.size() > Width) {
		buf.erase(Width - 3);
		buf.append("...");
	}
	return buf;
}

std::string StripXmlTag(std::string &Line, const char *Tag)
{
	// set the search strings
	std::stringstream strStart, strStop;
	strStart << "<" << Tag << ">";
	strStop << "</" << Tag << ">";
	// find the strings
	std::string::size_type locStart = Line.find(strStart.str());
	std::string::size_type locStop = Line.find(strStop.str());
	if (locStart == std::string::npos || locStop == std::string::npos)
		return "";
	// extract relevant text
	int pos = locStart + strStart.str().size();
	int len = locStop - pos;
	return len < 0 ? "" : Line.substr(pos, len);
}
