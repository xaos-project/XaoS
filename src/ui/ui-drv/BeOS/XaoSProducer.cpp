/*******************************************************************************
/
/	File:			XaoSProducer.cpp
/
/   Description:	Produce XaoS animation to pass along to some video-consuming Node.
/
*******************************************************************************/

#include <Mime.h>
#include <TimeSource.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include <Autolock.h>
#include <Debug.h>
#include <MediaNode.h>
#include <BufferProducer.h>

#include <scheduler.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "be_checkfile.h"
#include "XaoSProducer.h"
#define us_to_s(i) ((i)/1000000.0)

#undef NDEBUG
#define DEBUG 1
#if !NDEBUG
#define FPRINTF	fprintf
#else
#define FPRINTF
#endif
enum {
        MSG_QUIT_NOW = 0x60000000L
};

//	Comment out the FPRINTF part of these lines to reduce verbiage.
#define WARNING FPRINTF
#define LATE FPRINTF
#define TRANSPORT FPRINTF
#define NODE FPRINTF
#define TIMING FPRINTF
#define FORMAT FPRINTF

#define PREFFERED_FIELD_RATE 90 
#define PREFFERED_WIDTH 320
#define PREFFERED_HEIGHT 200
// is there a reason for higher framereate? We are handling real framerate
// by dropping frames anyway
//	These constants set a reasonable default format for us to use.
#define PREFFERED_COLOR_SPACE B_RGB32

static const bigtime_t HUGE_TIMEOUT = 6000000LL;

static const struct media_file_format fileformat={
	media_file_format::B_READABLE | /*media_file_format::B_IMPERFECTLY_SEEKABLE | */ media_file_format::B_KNOWS_VIDEO,
	{},
	"video/x-xaos-animation"
};

XaoSProducer::XaoSProducer(const char * name, BMediaAddOn *add)
:	BMediaNode(name ? name : "XaoS"), 
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BFileInterface(),
	addon(add),
	xaf_file(NULL)
{
	NODE(stderr, "SoundProducer::SoundProducer\n");

	AddNodeKind(B_FILE_INTERFACE);
	AddNodeKind(B_BUFFER_PRODUCER);
	if (!name) name = "XaoS";


	// Create the port that we publish as our ControlPort.
	// As a simple producer, our queue length doesn't need
	// to be very long.
	char pname[32];
	sprintf(pname, "%.20s Control", name);
	m_port = create_port(3, pname);

	// Finish specifying the media_output. Make sure
	// it knows the control port associated with the
	// source, and the index of the source (since we
	// only have one, that's trivial).
	m_output.source.port = m_port;
	m_output.source.id = 1;
	sprintf(m_output.name, "%.20s Output", name);

	// Set up the timing variables that we'll be using.
	m_frames_played = 0;
	// For downstream latency, we'll fill in a real value
	// when we're connected.
	m_downstream_latency = 15000;
	// For our own latency, we'll take a guess at the 
	// scheduling latency and multiply by a fudge
	// factor.
	m_private_latency = estimate_max_scheduling_latency(m_thread)*2;
	
	m_tpStart = 0;
	m_tpStop = 0;
	m_tpSeekAt = 0;
	m_tmSeekTo = 0;
	m_delta = 0;
	m_running = false;
	m_starting = false;
	m_stopping = false;
	m_seeking = false;
	m_buffers = 0;
	SetFormat(NULL);
#if !NDEBUG
	char fm2[100];
	string_for_format(m_output.format, fm2, 100);
	FORMAT(stderr, "XaoSProducer format: %s\n", fm2);
#endif
	m_sendBufferSem = create_sem(1, "send buffer sem");

	
	m_thread = spawn_thread(ThreadEntry, "XaoS Thread", B_DISPLAY_PRIORITY, this);
	resume_thread(m_thread);
}


XaoSProducer::~XaoSProducer()
{
	NODE(stderr, "XaoSProducer::~XaoSProducer()\n");

	// Tell our thread that it's bedtime.
	write_port(m_port, MSG_QUIT_NOW, 0, 0);
	status_t s;
	while (wait_for_thread(m_thread, &s) == B_INTERRUPTED)
		NODE(stderr, "wait_for_thread() B_INTERRUPTED\n");
	
	NODE(stderr, "XaoSProducer::~XaoSProducer(): thread %ld completed\n", m_thread);
	delete_port(m_port);
	delete_sem(m_sendBufferSem);
	if(xaf_file!=NULL) delete xaf_file;
}
status_t 
XaoSProducer::GetRef(entry_ref *outRef, char *outMimeType)
{
        strcpy(outMimeType,"video/x-xaos-animation");
	fprintf(stderr,"XaoSProducer::GetRef not implemented\n");
        *outRef=xaf_ref;
	return B_OK;
}
status_t 
XaoSProducer::SetRef(const entry_ref &file, bool create, bigtime_t *outDuration)
{
        xaf_ref=file;
	fprintf(stderr, "XaoSProducer::SetRef (partially implemented)\n");
        *outDuration=((bigtime_t)60)*60*1000000; /*We don't know time. Set one hour */
        if(xaf_file!=NULL) delete xaf_file;
        xaf_file=new BFile(&file, O_RDONLY);
        if(xaf_file->InitCheck() != B_OK) return B_ERROR;
	fprintf(stderr,"OK!\n");
	return B_OK;
}
status_t 
XaoSProducer::SniffRef(const entry_ref &file, char *outMimeType, float *outQuality)
{
	BFile node(&file, O_RDONLY);
	int r=XaoSCheckFile(&node);
	if(r) *outQuality=1.0, strcpy(outMimeType,"video/x-xaos-animation");
	fprintf(stderr,"XaoSProducer::SniffRef returns:%i\n",r);
	return(r?B_OK:B_MEDIA_NO_HANDLER);
}

status_t 
XaoSProducer::GetNextFileFormat(int32 *cookie, media_file_format *outFormat)
{
	fprintf(stderr, "XaoSProducer::GetNextFileFormat\n");
	if(!*cookie)  {
		*cookie=1;
		if(outFormat) *outFormat=fileformat;
		return B_OK;
	}
	return B_ERROR;
}
void XaoSProducer::DisposeFileFormatCookie(int32  cookie )
{
	fprintf(stderr, "XaoSProducer::DisposeFileFormatCookie\n");
	return;
}
status_t XaoSProducer::GetDuration(bigtime_t *outDuration)
{
	fprintf(stderr, "XaoSProducer::GetDuration (not implemented)\n");
	*outDuration=10*1000000*(bigtime_t)60;
	return B_OK;
}


void XaoSProducer::SetFormat(const media_raw_video_format* format)
{
	fprintf(stderr,"XaoSProducer::SetFormat\n");
	// m_raw_format is the format we've been told to use;
	// i.e. the format we'd like to use. The object
	// media_raw_audio_format::wildcard is a format
	// structure whose fields are all unitialized.
	m_raw_format = media_raw_video_format::wildcard;
	if (format != 0) {
		m_raw_format = *format;
	}

	// If anything has been left unspecified in the format
	// (== media_raw_video_format::wildcard.*), or if any
	// field is invalid (< media_raw_video_format::wildcard.*),
	// fill it in with a reasonable default.
	if (m_raw_format.field_rate <= media_raw_video_format::wildcard.field_rate) {
		m_raw_format.field_rate = PREFFERED_FIELD_RATE;
	}
	if (m_raw_format.interlace <= media_raw_video_format::wildcard.interlace) {
		m_raw_format.interlace = 1;
	}
	if (m_raw_format.first_active <= media_raw_video_format::wildcard.first_active) {
		m_raw_format.first_active = 0;
	}
	if (m_raw_format.last_active <= media_raw_video_format::wildcard.last_active) {
		m_raw_format.last_active = PREFFERED_HEIGHT-1;
	}
	if (m_raw_format.orientation <= media_raw_video_format::wildcard.orientation) {
		m_raw_format.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	}
	if (m_raw_format.pixel_width_aspect <= media_raw_video_format::wildcard.pixel_width_aspect) {
		m_raw_format.pixel_width_aspect = 1;
	}
	if (m_raw_format.pixel_height_aspect <= media_raw_video_format::wildcard.pixel_height_aspect) {
		m_raw_format.pixel_height_aspect = 1;
	}
	if (m_raw_format.display.format <= media_raw_video_format::wildcard.display.format) {
		m_raw_format.display.format = PREFFERED_COLOR_SPACE;
	}
	if (m_raw_format.display.line_width <= media_raw_video_format::wildcard.display.line_width) {
		m_raw_format.display.line_width = PREFFERED_WIDTH;
	}
	if (m_raw_format.display.line_count <= media_raw_video_format::wildcard.display.line_width) {
		m_raw_format.display.line_count = PREFFERED_HEIGHT;
	}
	if (m_raw_format.display.line_offset <= media_raw_video_format::wildcard.display.line_offset) {
		m_raw_format.display.line_offset = 0;
	}
	if (m_raw_format.display.pixel_offset <= media_raw_video_format::wildcard.display.pixel_offset) {
		m_raw_format.display.pixel_offset = 0;
	}
	if (m_raw_format.display.bytes_per_row <= media_raw_video_format::wildcard.display.bytes_per_row) {
                int mult;
		switch (m_raw_format.display.format) {	
                	/*The 24-bit versions are not compiled in, because BeOS don't
		  	support them in most cases. Use 32 instead.	*/
		case B_RGB24:
		case B_RGB24_BIG:
			mult=3*8;
			break;
		case B_RGB32: 
		case B_RGB32_BIG: 
		case B_RGBA32:
		case B_RGBA32_BIG:
			mult=4*8;
			break;
		case B_RGB16: 
		case B_RGB16_BIG: 
		case B_RGBA15: 
		case B_RGBA15_BIG:
		case B_RGB15: 
		case B_RGB15_BIG: 
			mult=2*8;
			break;
		case B_CMAP8: 
		case B_GRAY8: 
			mult=8;
		case B_GRAY1:
			mult=1;
	
		break;
		default: abort();
		}
		m_raw_format.display.bytes_per_row = (mult*m_raw_format.display.line_width+7)/8;
	}

	media_format proposedFormat;
	proposedFormat.type = B_MEDIA_RAW_VIDEO;
	proposedFormat.u.raw_video = media_raw_video_format::wildcard;
	proposedFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	FormatSuggestionRequested(B_MEDIA_RAW_VIDEO, 0, &proposedFormat);

	// Set the output format. m_output represents the
	// actual connection, so m_output.format.u.raw_audio
	// should be equal to the *actual* format we're using,
	// as opposed to m_raw_format, which is our preferred
	// format.
	if (m_output.destination != media_destination::null) {
		// If there's an existing connection, notify the
		// downstream consumer that we want to change the
		// format. The consumer may refuse to change to
		// the new format, in which case we're pretty much
		// out of luck.
		// 
		// Two notes:
		// 1. We need to stop sending buffers while
		// we're changing the format, so we guard with a
		// semaphore.
		// 2. Theoretically, this allows us to change the
		// format while we're connected and running. In
		// practice, however, few nodes right now support
		// format changes this way.
		if (acquire_sem(m_sendBufferSem) == B_OK) {
			NODE(stderr, "XaoSProducer::ChangeFormat(): src = %ld, dest = %ld\n",
				m_output.source.id, m_output.destination.id);
			if (ProposeFormatChange(&proposedFormat, m_output.destination) == B_OK
				&& ChangeFormat(m_output.source, m_output.destination,
					&proposedFormat) == B_OK) 
			{
				// OK by the consumer! Go ahead and update m_output.
				m_output.format = proposedFormat;
			} else {
				WARNING(stderr, "XaoSProducer::SetFormat(): couldn't change to new format!\n");
			}			
			release_sem(m_sendBufferSem);
		} else {
			WARNING(stderr, "XaoSProducer::SetFormat(): couldn't acquire SendBuffer semaphore!\n");
		}
	} else {
		// No active connection, so set m_output to the
		// desired format; it'll be adjusted when an
		// actual connection is made.
		m_output.format = proposedFormat;
	}
}





////////////////////////////////////////////////////////////////////////////////
//
//	BMediaNode-derived methods
//
////////////////////////////////////////////////////////////////////////////////



port_id XaoSProducer::ControlPort() const
{
	return m_port;
}


BMediaAddOn* XaoSProducer::AddOn(
	int32 * internal_id) const
{
	fprintf(stderr,"XaoSProducer::AddOn (OK)\n");
	if (internal_id) *internal_id = 0;
	return addon;
}


void XaoSProducer::Start(
	bigtime_t performance_time)
{
	fprintf(stderr,"XaoSProducer::Start (not implemented)\n");
	if (!m_stopping || performance_time > m_tpStop) {
		if (!m_running || m_stopping) {
#if 0
			if (m_notifyHook) {
				(*m_notifyHook)(m_cookie, B_WILL_START, performance_time);
			}
			else {
				Notify(B_WILL_START, performance_time);
			}
#endif
			
			m_tpStart = performance_time;
			m_starting = true;
			TRANSPORT(stderr, "XaoSProducer start at %.4f (now %.4f)\n",
				us_to_s(m_tpStart), us_to_s(TimeSource()->Now()));
		}
	}
}


void XaoSProducer::Stop(
	bigtime_t performance_time,
	bool immediate)
{
	bool notified = false;
	fprintf(stderr,"XaoSProducer::Stop (not implemented)\n");

	if (!m_starting || performance_time > m_tpStart) {
		if (m_running || m_starting) {
			// It's okay to handle this Stop request.
#if 0
			if (m_notifyHook) {
				(*m_notifyHook)(m_cookie, B_WILL_STOP, performance_time,
					immediate);
			}
			else {
				Notify(B_WILL_STOP, performance_time, immediate);
			}
#endif
			notified = true;
			m_tpStop = performance_time;
			m_stopping = true;
			TRANSPORT(stderr, "XaoSProducer stop at %.4f (now %.4f)\n",
				us_to_s(m_tpStop), us_to_s(TimeSource()->Now()));
		}
	}

#if 0
	if (immediate) {
		if (! notified) {
			if (m_notifyHook) {
				(*m_notifyHook)(m_cookie, B_WILL_STOP, performance_time,
					immediate);
			}
			else {
				Notify(B_WILL_STOP, performance_time, immediate);
			}
		}
		m_running = false;
	}
#endif
}


void XaoSProducer::Seek(
	bigtime_t media_time,
	bigtime_t performance_time)
{
	fprintf(stderr,"XaoSProducer::Seek (not implemented)\n");
#if 0
	// Seek sets the "media time" of a node. What the
	// media time is interpreted to be varies from node
	// to node. For a Producer that plays a sound, media
	// time is generally interpreted to be the time	
	// offset of the media being played (where 0 is
	// the beginning of the media).
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_WILL_SEEK, performance_time, media_time);
	}
	else {
		Notify(B_WILL_SEEK, performance_time, media_time);
	}
	m_tmSeekTo = media_time;
	TRANSPORT(stderr, "SEEK: Setting m_tmSeekTo to %.4f @ %.4f\n",
		us_to_s(m_tmSeekTo), us_to_s(performance_time));
	m_tpSeekAt = performance_time;
	m_seeking = true;
	TIMING(stderr, "XaoSProducer seek at %.4f (now %.4f)\n", us_to_s(m_tpSeekAt),
		us_to_s(TimeSource()->Now()));
#endif
}

void XaoSProducer::SetRunMode(
	run_mode mode)
{
	fprintf(stderr,"XaoSProducer::SetRunMode (not implemented)\n");
#if 0
	if (mode == BMediaNode::B_OFFLINE) {
		int32 new_prio = suggest_thread_priority(B_OFFLINE_PROCESSING);
		set_thread_priority(m_thread, new_prio);
	}
	else {
		bigtime_t period = 10000;
		if (buffer_duration(m_output.format.u.raw_audio) > 0) {
			period = buffer_duration(m_output.format.u.raw_audio);
		}
		int32 new_prio = suggest_thread_priority(B_AUDIO_PLAYBACK,
			period, 1000, ProcessingLatency());
		set_thread_priority(m_thread, new_prio);
	}
#endif
}


void XaoSProducer::TimeWarp(
	bigtime_t at_real_time,
	bigtime_t to_performance_time)
{
	fprintf(stderr,"XaoSProducer::TimeWarp (not implemented)\n");
#if 0
	// Not implemented for now -- future versions will handle this
	// correctly.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_WILL_TIMEWARP, at_real_time, to_performance_time);
	}
	else {
		Notify(B_WILL_TIMEWARP, at_real_time, to_performance_time);
	}
#endif
}


void XaoSProducer::Preroll()
{
	fprintf(stderr,"XaoSProducer::Preroll (not implemented)\n");
}


void XaoSProducer::SetTimeSource(
	BTimeSource * ts)
{
	fprintf(stderr,"XaoSProducer::SetTimeSource (not implemented)\n");
        BBufferProducer::SetTimeSource(ts);
}


status_t XaoSProducer::HandleMessage(
	int32 message,
	const void * data,
	size_t size)
{
	// Check with each of our superclasses to see if they
	// understand the message. If none of them do, call
	// BMediaNode::HandleBadMessage().
	if (BMediaNode::HandleMessage(message, data, size) && 
		BFileInterface::HandleMessage(message, data, size) &&
		BBufferProducer::HandleMessage(message, data, size)) {
		BMediaNode::HandleBadMessage(message, data, size);
		return B_ERROR;
	}
	return B_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//	BMediaNode-derived methods
//
////////////////////////////////////////////////////////////////////////////////



status_t XaoSProducer::FormatSuggestionRequested(
	media_type type,
	int32 /* quality */,
	media_format * format)
{
	NODE(stderr, "XaoSProducer::FormatSuggestionRequested()\n");

	if (type <= 0) type = B_MEDIA_RAW_VIDEO;
	if (type != B_MEDIA_RAW_VIDEO) return B_MEDIA_BAD_FORMAT;
	format->type = type;
	format->u.raw_video = media_raw_video_format::wildcard;
	// format->u.raw_video.field_rate = PREFFERED_FIELD_RATE;

	// Interlace is not supported by XaoS
	format->u.raw_video.interlace = 1;
	format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	//format->u.raw_video.display.format = B_RGB32;
#if !NDEBUG
	char fmt[100];
	string_for_format(*format, fmt, 100);
	FORMAT(stderr, "return format %s\n", fmt);
#endif
	return B_OK;
}


status_t XaoSProducer::FormatProposal(
	const media_source & output,
	media_format * format)
{
	fprintf(stderr,"XaoSProducer::FormatProposal!\n");
	if (output != m_output.source) {
		NODE(stderr, "XaoSProducer::FormatProposal(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (format->type <= 0) {
		FormatSuggestionRequested(B_MEDIA_RAW_VIDEO, 0, format);
	}
	else {
		if (format->type != B_MEDIA_RAW_VIDEO) {
			goto err;
		}

		/* We require non interlaced TOP_LEFT_RIGHT oriented mode */
		fprintf(stderr,"XaoSProducer::interlace %i!\n", format->u.raw_video.interlace);
		if (format->u.raw_video.interlace <= media_raw_video_format::wildcard.interlace) {
			format->u.raw_video.interlace = 1;
		} else if (format->u.raw_video.interlace!=1)  goto err;
		fprintf(stderr,"XaoSProducer::orientation %i!\n", format->u.raw_video.orientation);
		if (format->u.raw_video.orientation <= media_raw_video_format::wildcard.orientation) {
			format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		} else if (format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT) goto err;

#if 0
		fprintf(stderr,"XaoSProducer::field rate %f!\n", format->u.raw_video.field_rate);
		if (format->u.raw_video.field_rate <= media_raw_video_format::wildcard.field_rate) {
			format->u.raw_video.field_rate = PREFFERED_FIELD_RATE;
		}
		fprintf(stderr,"XaoSProducer::first_active %i!\n", format->u.raw_video.first_active);
		if (format->u.raw_video.first_active <= media_raw_video_format::wildcard.first_active) {
			format->u.raw_video.first_active = 0;
		}
		fprintf(stderr,"XaoSProducer::last_active %i!\n", format->u.raw_video.last_active);
		if (format->u.raw_video.last_active <= media_raw_video_format::wildcard.last_active) {
			format->u.raw_video.last_active = PREFFERED_HEIGHT-1;
		}
		fprintf(stderr,"XaoSProducer::width_aspect %i!\n", format->u.raw_video.pixel_width_aspect);
		if (format->u.raw_video.pixel_width_aspect <= media_raw_video_format::wildcard.pixel_width_aspect) {
			format->u.raw_video.pixel_width_aspect = 1;
		}
		fprintf(stderr,"XaoSProducer::height_aspect %i!\n", format->u.raw_video.pixel_height_aspect);
		if (format->u.raw_video.pixel_height_aspect <= media_raw_video_format::wildcard.pixel_height_aspect) {
			format->u.raw_video.pixel_height_aspect = 1;
		}
		fprintf(stderr,"XaoSProducer::display.format %i!\n", format->u.raw_video.display.format);
		if (format->u.raw_video.display.format <= media_raw_video_format::wildcard.display.format) {
			format->u.raw_video.display.format = PREFFERED_COLOR_SPACE;
		}
		fprintf(stderr,"XaoSProducer::display.line_width %i!\n", format->u.raw_video.display.line_width);
		if (format->u.raw_video.display.line_width <= media_raw_video_format::wildcard.display.line_width) {
			format->u.raw_video.display.line_width = PREFFERED_WIDTH;
		}
		fprintf(stderr,"XaoSProducer::display.line_count %i!\n", format->u.raw_video.display.line_count);
		if (format->u.raw_video.display.line_count <= media_raw_video_format::wildcard.display.line_width) {
			format->u.raw_video.display.line_count = PREFFERED_HEIGHT;
		}
		fprintf(stderr,"XaoSProducer::display.bytes_per_row %i!\n", format->u.raw_video.display.bytes_per_row);
		if (format->u.raw_video.display.bytes_per_row <= media_raw_video_format::wildcard.display.bytes_per_row) {
			int mult;
			switch (format->u.raw_video.display.format) {	
				/*The 24-bit versions are not compiled in, because BeOS don't
				support them in most cases. Use 32 instead.	*/
			case B_RGB24:
			case B_RGB24_BIG:
				mult=3*8;
				break;
			case B_RGB32: 
			case B_RGB32_BIG: 
			case B_RGBA32:
			case B_RGBA32_BIG:
				mult=4*8;
				break;
			case B_RGB16: 
			case B_RGB16_BIG: 
			case B_RGBA15: 
			case B_RGBA15_BIG:
			case B_RGB15: 
			case B_RGB15_BIG: 
				mult=2*8;
				break;
			case B_CMAP8: 
			case B_GRAY8: 
				mult=8;
			case B_GRAY1:
				mult=1;
		
			break;
			default: abort();
			}
		format->u.raw_video.display.bytes_per_row = (mult*m_raw_format.display.line_width+7)/8;
		}
		fprintf(stderr,"XaoSProducer::display.bytes_per_row %i!\n", format->u.raw_video.display.bytes_per_row);
#endif
	}
	//format->u.raw_audio.byte_order = media_raw_audio_format::wildcard.byte_order;
#if !NDEBUG
	char fmt[100];
	string_for_format(*format, fmt, 100);
	FORMAT(stderr, "FormatProposal: %s\n", fmt);
#endif
	// everything checks out and we've filled in the
	// wildcards that we care about.
	return B_OK;
err:
	// we didn't like whatever they suggested. Counter-
	// propose with something acceptable and return an
	// error.
	fprintf(stderr, "Incompatible format!\n");
	FormatSuggestionRequested(B_MEDIA_RAW_VIDEO, 0, format);
	return B_MEDIA_BAD_FORMAT;
}


status_t XaoSProducer::FormatChangeRequested(
	const media_source & source,
	const media_destination & /* destination */,
	media_format * io_format,
	int32 * out_change_count)
{
	fprintf(stderr,"XaoSProducer::FormatChangeRequested (not implemented)\n");
	status_t err = FormatProposal(source, io_format);
	if (err < B_OK) return err;
#if 0
	m_output.format = *io_format;
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_FORMAT_CHANGED, &m_output.format.u.raw_audio);
	}
	else {
		Notify(B_FORMAT_CHANGED, &m_output.format.u.raw_audio);
	}
#endif
	*out_change_count = IncrementChangeTag();
	alloc_buffers();
	return B_OK;
}


status_t XaoSProducer::GetNextOutput(
	int32 * cookie,
	media_output * out_output)
{
	NODE(stderr, "XaoSProducer: GetNextOutput( %i)\n", *cookie);
	if (*cookie == 0) {
		*out_output = m_output;
		*cookie = 1;
	}
	else {
		printf("Bad luck\n");
		// There's only one output.
		return B_BAD_INDEX;
	}
	return B_OK;
}


status_t XaoSProducer::DisposeOutputCookie(
	int32 /* cookie */)
{
	return B_OK;
}


status_t XaoSProducer::SetBufferGroup(
	const media_source & for_source,
	BBufferGroup * group)
{
	NODE(stderr, "XaoSProducer: SetBufferGroup\n");
	if (for_source != m_output.source) {
		// We only accept buffer groups aimed at our single output.
		NODE(stderr, "XaoSProducer::SetBufferGroup(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	// We always own our buffer group. If we're being told to use a
	// different buffer group, we delete our current buffer group
	// first.
	// 
	// Note: Deleting a buffer group attempts to reclaim all of its
	// outstanding buffers. So, if we're holding on to any buffers,
	// we must recycle them before deleting the group, unless we
	// crave a big hunk o' deadlock pie. 
	if (group != m_buffers) {
		delete m_buffers;
		m_buffers = group;
	}
	return B_OK;
}


status_t XaoSProducer::VideoClippingChanged(
	const media_source & /* for_source */,
	int16 /* num_shorts */,
	int16 * /* clip_data */,
	const media_video_display_info & /* display */,
	int32 * /* out_from_change_count */)
{
	NODE(stderr, "XaoSProducer: ClippingChanged (not implemented)\n");
	/* FIXME handle XaoS resize here and send the clipping info to the
	 * handler */
	return B_ERROR;
}


status_t XaoSProducer::GetLatency(
	bigtime_t * out_latency)
{
	NODE(stderr, "XaoSProducer: GetLatency\n");
	status_t err = BBufferProducer::GetLatency(out_latency);
	if (err >= B_OK) {
		*out_latency = TotalLatency();
	}
	return err;
}

status_t XaoSProducer::PrepareToConnect(
	const media_source & what,
	const media_destination & where,
	media_format * format,
	media_source * out_source,
	char * out_name)
{
	NODE(stderr, "XaoSProducer::PrepareToConnect()\n");
	// We only accept connections to our single output, and only
	// when that output isn't already connected to something.
	if (what != m_output.source) {
		NODE(stderr, "XaoSProducer::PrepareToConnect(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (m_output.destination != media_destination::null) {
		NODE(stderr, "XaoSProducer::PrepareToConnect(): already connected\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
#if DEBUG
		char fmt[100];
		string_for_format(m_output.format, fmt, 100);
		FORMAT(stderr, "we're suggesting %s\n", fmt);
		string_for_format(*format, fmt, 100);
		FORMAT(stderr, "he's expecting %s\n", fmt);
#endif

	if (!format_is_compatible(*format, m_output.format)) {
		NODE(stderr, "XaoSProducer::PrepareToConnect(): bad format\n");
		return B_MEDIA_BAD_FORMAT;
	}

		/* We require non interlaced TOP_LEFT_RIGHT oriented mode */
		fprintf(stderr,"XaoSProducer::interlace %i!\n", format->u.raw_video.interlace);
		if (format->u.raw_video.interlace <= media_raw_video_format::wildcard.interlace) {
			format->u.raw_video.interlace = 1;
		} 
		fprintf(stderr,"XaoSProducer::orientation %i!\n", format->u.raw_video.orientation);
		if (format->u.raw_video.orientation <= media_raw_video_format::wildcard.orientation) {
			format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		}

		fprintf(stderr,"XaoSProducer::field rate %f!\n", format->u.raw_video.field_rate);
		if (format->u.raw_video.field_rate <= media_raw_video_format::wildcard.field_rate) {
			format->u.raw_video.field_rate = PREFFERED_FIELD_RATE;
		}
		fprintf(stderr,"XaoSProducer::first_active %i!\n", format->u.raw_video.first_active);
		if (format->u.raw_video.first_active <= media_raw_video_format::wildcard.first_active) {
			format->u.raw_video.first_active = 0;
		}
		fprintf(stderr,"XaoSProducer::last_active %i!\n", format->u.raw_video.last_active);
		if (format->u.raw_video.last_active <= media_raw_video_format::wildcard.last_active) {
			format->u.raw_video.last_active = PREFFERED_HEIGHT-1;
		}
		fprintf(stderr,"XaoSProducer::width_aspect %i!\n", format->u.raw_video.pixel_width_aspect);
		if (format->u.raw_video.pixel_width_aspect <= media_raw_video_format::wildcard.pixel_width_aspect) {
			format->u.raw_video.pixel_width_aspect = 1;
		}
		fprintf(stderr,"XaoSProducer::height_aspect %i!\n", format->u.raw_video.pixel_height_aspect);
		if (format->u.raw_video.pixel_height_aspect <= media_raw_video_format::wildcard.pixel_height_aspect) {
			format->u.raw_video.pixel_height_aspect = 1;
		}
		fprintf(stderr,"XaoSProducer::display.format %i!\n", format->u.raw_video.display.format);
		if (format->u.raw_video.display.format <= media_raw_video_format::wildcard.display.format) {
			format->u.raw_video.display.format = PREFFERED_COLOR_SPACE;
		}
		fprintf(stderr,"XaoSProducer::display.line_width %i!\n", format->u.raw_video.display.line_width);
		if (format->u.raw_video.display.line_width <= media_raw_video_format::wildcard.display.line_width) {
			format->u.raw_video.display.line_width = PREFFERED_WIDTH;
		}
		fprintf(stderr,"XaoSProducer::display.line_count %i!\n", format->u.raw_video.display.line_count);
		if (format->u.raw_video.display.line_count <= media_raw_video_format::wildcard.display.line_width) {
			format->u.raw_video.display.line_count = PREFFERED_HEIGHT;
		}
		fprintf(stderr,"XaoSProducer::display.bytes_per_row %i!\n", format->u.raw_video.display.bytes_per_row);
		if (format->u.raw_video.display.bytes_per_row <= media_raw_video_format::wildcard.display.bytes_per_row) {
			int mult;
			switch (format->u.raw_video.display.format) {	
				/*The 24-bit versions are not compiled in, because BeOS don't
				support them in most cases. Use 32 instead.	*/
			case B_RGB24:
			case B_RGB24_BIG:
				mult=3*8;
				break;
			case B_RGB32: 
			case B_RGB32_BIG: 
			case B_RGBA32:
			case B_RGBA32_BIG:
				mult=4*8;
				break;
			case B_RGB16: 
			case B_RGB16_BIG: 
			case B_RGBA15: 
			case B_RGBA15_BIG:
			case B_RGB15: 
			case B_RGB15_BIG: 
				mult=2*8;
				break;
			case B_CMAP8: 
			case B_GRAY8: 
				mult=8;
			case B_GRAY1:
				mult=1;
		
			break;
			default: abort();
			}
		format->u.raw_video.display.bytes_per_row = (mult*m_raw_format.display.line_width+7)/8;
		}
		fprintf(stderr,"XaoSProducer::display.bytes_per_row %i!\n", format->u.raw_video.display.bytes_per_row);
	
		string_for_format(*format, fmt, 100);
		FORMAT(stderr, "We've decided to use %s\n", fmt);
	m_output.destination = where;
	m_output.format = *format;
	*out_source = m_output.source;
	strncpy(out_name, Name(), B_MEDIA_NAME_LENGTH);
	/*alloc_buffers();*/
	return B_OK;
}


void XaoSProducer::Connect(
	status_t error, 
	const media_source & source,
	const media_destination & destination,
	const media_format & format,
	char * io_name)
{
	ASSERT(source == m_output.source);
	NODE(stderr, "XaoSProducer: Connect\n");
	
	if (error < B_OK) {
		NODE(stderr, "XaoSProducer::Connect(): we were told about an error\n");
		m_output.destination = media_destination::null;
		return;
	}
#if !NDEBUG
	char fmt[100];
	string_for_format(format, fmt, 100);
	FORMAT(stderr, "Connect(): format %s\n", fmt);
#endif

	m_output.destination = destination;
	m_output.format = format;

	NODE(stderr, "XaoSProducer::Connect(): source = %ld, dest = %ld\n", source.id, destination.id);
	
	alloc_buffers();

	BBufferProducer::GetLatency(&m_downstream_latency);

	strncpy(io_name, Name(), B_MEDIA_NAME_LENGTH);
#if 0
	//	Tell whomever is interested that there's now a connection.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_CONNECTED, m_output.name);
	}
	else {
		Notify(B_CONNECTED, m_output.name);
	}
#endif
}


void XaoSProducer::Disconnect(
	const media_source & what,
	const media_destination & consumer)
{
	NODE(stderr, "XaoSProducer::Disconnect()\n");
	// We can't disconnect something which isn't us.
	if (what != m_output.source) {
		NODE(stderr, "XaoSProducer::Disconnect(): source is incorrect\t");
		return;
	}
	// We can't disconnect from someone who isn't connected to us.
	if (consumer != m_output.destination) {
		NODE(stderr, "XaoSProducer::Disconnect(): destination is incorrect\n");
		return;
	}
#if 0
	//	Tell the interested party that it's time to leave.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_DISCONNECTED);
	}
	else {
		Notify(B_DISCONNECTED);
	}
#endif
	// Mark ourselves as not-connected. We also clean up
	// the buffer group that we were using.
	m_output.destination = media_destination::null;
	//m_output.format.u.raw_audio.buffer_size = 0;
	delete m_buffers;
	m_buffers = 0;
}


void XaoSProducer::LateNoticeReceived(
	const media_source & /* what */,
	bigtime_t how_much,
	bigtime_t performance_time)
{
	LATE(stderr, "XaoSProducer::LateNoticeReceived(%.4f @ %.4f)\n",
		us_to_s(how_much), us_to_s(performance_time));
	switch (RunMode()) {
	case B_OFFLINE:
		// If we're not running in real time, we should never receive
		// late notices!
		break;
	case B_DECREASE_PRECISION:
		// We can't really do much here. Maybe our hook functions
		// will be able to do something about this?...
		break;
	case B_INCREASE_LATENCY:
		// Adjust our latencies so that we start sending buffers
		// earlier.
		if (how_much > 3000) {
			how_much = 3000;
		}
		m_downstream_latency += how_much;
		m_private_latency += how_much/2;
		break;
	default:
		// As a simple producer, we can't be recording, so
		// we must be in B_DROP_DATA. Just drop a buffer and
		// hope that catches us up.
		m_frames_played++;
		break;
	}
#if 0
	//	Tell whoever's interested that we're running behind.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_LATE_NOTICE, how_much, performance_time);
	}
	else {
		Notify(B_LATE_NOTICE, how_much, performance_time);
	}
#endif
}


void XaoSProducer::EnableOutput(
	const media_source & what,
	bool enabled,
	int32 * change_tag)
{
	NODE(stderr, "XaoSProducer: EnableOutput\n");
	if (what != m_output.source) {
		NODE(stderr, "XaoSProducer::EnableOutput(): bad source\n");
		return;
	}
	//m_muted = !enabled;
	// We increment our node's private change tag and pass it back to
	// the caller. The current change tag is always attached to the
	// header of each buffer we send. By setting the change tag pointer
	// we are given, downstream consumers know the tag corresponding
	// to this state, and can match that tag against buffers that they
	// receive from us, should they have a reason to know.
	*change_tag = IncrementChangeTag();
	if (m_output.destination != media_destination::null) {
		// It's important to tell the consumer that we will or
		// won't be sending it data, so it's ready for the data
		// or won't get stuck waiting for data that never comes.
		//SendDataStatus(m_muted ? B_DATA_NOT_AVAILABLE : B_DATA_AVAILABLE, 
		SendDataStatus(B_DATA_AVAILABLE, 
			m_output.destination, TimeSource()->Now());
	}
}





////////////////////////////////////////////////////////////////////////////////
//
//	Service thread methods
//
////////////////////////////////////////////////////////////////////////////////



#if 0
void
XaoSProducer::DoHookChange(
	void * msg)
{
	//	Tell the old guy we're changing the hooks ...
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_HOOKS_CHANGED);
	}
	else {
		Notify(B_HOOKS_CHANGED);
	}
	//	... and then do it.
	set_hooks_q * ptr = (set_hooks_q *)msg;
	m_playHook = ptr->process;
	m_notifyHook = ptr->notify;
	m_cookie = ptr->cookie;
}
#endif


status_t
XaoSProducer::ThreadEntry(
	void * obj)
{
	((XaoSProducer *)obj)->ServiceThread();
	return B_OK;
}


void XaoSProducer::ServiceThread()
{
	NODE(stderr, "XaoSProducer::ServiceThread() is alive!\n");

	//	A media kit message will never be bigger than B_MEDIA_MESSAGE_SIZE.
	//	Avoid wasing stack space by dynamically allocating at start.
	//char  msg = new char[B_MEDIA_MESSAGE_SIZE];
	char  msg[B_MEDIA_MESSAGE_SIZE];
	//array_delete<char> msg_delete(msg);
	int bad = 0;


	m_private_latency = estimate_max_scheduling_latency(find_thread(0));
	while (true) {
		// buffer_perf is the next performance time at which a buffer
		// needs to be performed.
		//
		// perf_target is the next performance time at which some event
		// has to occur. If we're running & connected, and don't have any
		// pending performance events (such as start or stop), our
		// perf_target will be buffer_perf.
		bigtime_t buffer_perf = (bigtime_t)(1000000/m_output.format.u.raw_video.field_rate * m_frames_played+m_delta);
		bigtime_t perf_target;
		
		bool ts_running = TimeSource()->IsRunning();
		bool connected = (m_output.destination != media_destination::null);
		bigtime_t timeout = 1000000000000LL;
		
		if (m_running && connected) {
			perf_target = buffer_perf;
		} else {
			// If we're not running or connected, we might as well
			// relax for a while (unless there are pending events
			// coming up sooner).
			perf_target = TimeSource()->Now() + HUGE_TIMEOUT;
		}
		fprintf(stderr,"Thread loop! %i %i %i\n",ts_running, m_starting, m_running);
		
		if (/*ts_running*/1) {
			bigtime_t now_real = BTimeSource::RealTime();
			if (m_stopping) {
				// There's a pending Stop request.
				if (now_real >= TimeSource()->RealTimeFor(m_tpStop, TotalLatency())) {
					// It's time to handle that Stop request.
					TRANSPORT(stderr, "XaoSProducer::Stop() takes effect\n");
					m_running = false;
					m_stopping = false; // we've now stopped
					if (m_seeking) {
						// There's a seek pending, but we'll defer any
						// pending seeks until we next start.
						m_seeking = false;
					}
					else {
						// Set the seek time so that, by default, we
						// restart where we stopped.
						m_tmSeekTo = m_tpStop-m_delta;
						TRANSPORT(stderr, "Setting m_tmSeekTo to %.4f\n", us_to_s(m_tmSeekTo));
					}
					// Very important! Tell the Consumer that we're no longer
					// sending data to it, so it doesn't sit around waiting for
					// the buffers that never come.
					if (connected) {
						SendDataStatus(B_DATA_NOT_AVAILABLE, m_output.destination, m_tpStop);
					}
					//	Restart the loop because we changed timing info.
					continue;
				}
				else {
					// It's not quite yet time to Stop, but if it's a sooner
					// event than our next perf_target, set it to be the
					// next perf_target.
					if (m_tpStop < perf_target) {
						TRANSPORT(stderr, "m_tpStop perf_target from %.4f to %.4f\n",
							us_to_s(perf_target), us_to_s(m_tpStop));
						perf_target = m_tpStop;
					}
				}
			}
			if (m_seeking) {
				// There's a pending Seek request.
				if (now_real >= TimeSource()->RealTimeFor(m_tpSeekAt, TotalLatency())) {
					// It's time to handle that Seek request.
					TRANSPORT(stderr, "XaoSProducer::Seek() takes effect\n");
					m_seeking = false;
					// Seek gives us the relationship between media time
					// and performance time. We represent this relationship
					// via m_delta -- see the description in XaoSProducer.h.
					//
					// Note: m_frames_played is not addressed here, which I
					// believe is incorrect. Something to look into when
					// we actually use Seek...
					m_delta = m_tpSeekAt-m_tmSeekTo;
					TRANSPORT(stderr, "setting m_delta to %.4f\n", us_to_s(m_delta));
					//	Restart the loop because we changed timing info.
					continue;
				}
				else {
					// It's not quite yet time to Seek, but if it's a sooner
					// event than our next perf_target, set it to be the
					// next perf_target.
					if (m_tpSeekAt < perf_target) {
						TRANSPORT(stderr, "m_tpSeek perf_target from %.4f to %.4f\n",
							us_to_s(perf_target), us_to_s(m_tpSeekAt));
						perf_target = m_tpSeekAt;
					}
				}
			}
			if (m_starting) {
				// There's a pending Start request.
				printf("Starting thread!\n");
				if (now_real >= TimeSource()->RealTimeFor(m_tpStart, TotalLatency())) {
					printf("Starting OK\n");
					// It's time to handle that Start request.
					TRANSPORT(stderr, "XaoSProducer::Start() takes effect\n");
					m_running = true;
					// Seek to the correct point in the media before
					// we begin. This offset might have been set by
					// a Seek operation or the last Stop operation.
					// We seek by setting the offset between media time
					// and performance time.
					m_delta = m_tpStart-m_tmSeekTo;
					TRANSPORT(stderr, "setting m_delta to %.4f\n", us_to_s(m_delta));
					m_frames_played = 0;
					m_starting = false; // we've now started
				
					// Very important! Tell the Consumer that we'll be sending
					// data to it, so it's expecting us.
					if (connected) {
						SendDataStatus(B_DATA_AVAILABLE, m_output.destination, m_tpStart);
					}
					//	Restart the loop because we changed timing info.
					continue;
				}
				else {
					// It's not quite yet time to Start, but if it's a sooner
					// event than our next perf_target, set it to be the
					// next perf_target.
					if (m_tpStart < perf_target) {
						TRANSPORT(stderr, "m_tpStart perf_target from %.4f to %.4f\n",
							us_to_s(perf_target), us_to_s(m_tpStart));
						perf_target = m_tpStart;
					}
				}
			}
			
			// Finally, calculate the all-important timeout value.
			// The timeout value is the difference in real time
			// between now and the real time at which our thread
			// needs to handle the next event which occurs at
			// perf_target.
			timeout = TimeSource()->RealTimeFor(perf_target, TotalLatency()) -
				BTimeSource::RealTime();
		}
		else if (m_running) {
			// pathological case: we set a strange timeout value that we
			// can recognize later on if necessary. If we're not connected,
			// we'll make it a huge timeout and catch that error later
			// on.
			timeout = connected ? 9999 : 9999999;
			WARNING(stderr, "XaoSProducer: m_running but not (connected && ts_running)\n"); 
		} else {
			WARNING(stderr, "XaoSProducer: ! ts_running\n");
		}
		
		// Adjust the timeout to make sure it's reasonable.
		if (timeout <= 0) {
			// We needed to start handling the next event before now.
			if ((RunMode() != B_OFFLINE) && (timeout < -50000)) {
				// We're way behind in a real-time run mode --
				// just skip forward in the media to catch up!
				m_delta -= timeout-5000;
			}
			
			// Give us some breathing room to check for messages.
			// We don't simply refuse to handle messages when
			// we're behind so that we can remain responsive even
			// when our buffer-producing ability is maxed out.
			timeout = 1000;
		}
		
		////////////////////////////////////////////////////////////
		// Step 2: Check for pending messages.
		
		// Conveniently enough, if there are no pending messages,
		// this call to read_port_etc will force our thread to
		// wait for the length of timeout. Recall, we just set
		// timeout to be the real time until the next performance
		// event or buffer production needs to happen, whichever
		// comes first.
		int32 code = 0;
		fprintf(stderr, "XaoS Thread:Waiting for messages\n");
		status_t err = read_port_etc(m_port, &code, msg, B_MEDIA_MESSAGE_SIZE, B_TIMEOUT, timeout);
		fprintf(stderr, "XaoS Thread:message received\n");
		//	If we received a message, err will be the size of the message (including 0).		
		if (err >= 0) {
			bad = 0;
			NODE(stderr, "XaoSProducer msg %#010lx\n", code);
			
			// Check for our private stop message.
			if (code == MSG_QUIT_NOW) {	//	quit now
				NODE(stderr, "XaoSProducer quitting\n");
#if 0
				if (m_notifyHook) {
					(*m_notifyHook)(m_cookie, B_NODE_DIES, 0);
				}
				else {
					Notify(B_NODE_DIES, 0);
				}
#endif
				break;
			}
			//	Else it is hopefully a regular media kit message; go ahead and
			//	dispatch it. (HandleMessage addresses the case that the message
			//	wasn't understood by anybody.)
			else {
				HandleMessage(code, msg, err);
			}
		}
		//	Timing out means that there was no buffer, which is ok.
		//	Other errors, though, are bad.
		else if (err != B_TIMED_OUT) {
			WARNING(stderr, "XaoSProducer::ServiceThread(): port says %#010lx (%s)\n", err, strerror(err));
			bad++;
			//	If we receive three bad reads with no good messages in between,
			//	things are probably not going to improve (like the port disappeared
			//	or something) so we call it a day.
			if (bad > 3) {
#if 0
				if (m_notifyHook) {
					(*m_notifyHook)(m_cookie, B_NODE_DIES, bad, err, code, msg);
				}
				else {
					Notify(B_NODE_DIES, bad, err, code, msg);
				}
#endif
				break;
			}
		}
		else {
			////////////////////////////////////////////////////////////
			// Step 3: Produce and send a buffer

			bad = 0;
			if (timeout > 1000000) {
				// We set a huge timeout when we were running, but the
				// time source isn't running, and we're not connected?
				continue;	// don't actually play
			}
			
			// Only make a buffer if time is running, and if we are
			// running, connected, enabled, and can get a buffer.
			if (ts_running) {
				if (connected && m_running) {
					char c[100];
					string_for_format(m_output.format,c,100);
					fprintf(stderr,"Buffer! %s\n",100);
					BBuffer * buffer = m_buffers->RequestBuffer(
	                                        m_output.format.u.raw_video.display.line_count*m_output.format.u.raw_video.display.bytes_per_row);
					fprintf(stderr,"Buffer! OK\n");
					if (buffer) {
						bigtime_t now = TimeSource()->Now();
						NODE(stderr, "XaoSProducer making a buffer at %Ld.\n", now);
						// Whee, we actually get to make a buffer!
						// Fill the buffer's header fields.
						buffer->Header()->start_time = buffer_perf;
						buffer->Header()->size_used = (m_output.format.u.raw_video.display.line_count*m_output.format.u.raw_video.display.bytes_per_row);
						//	If there is a play hook, let the interested party have at it!
#if 0
						if (m_playHook) {
							(*m_playHook)(m_cookie, buffer->Header()->start_time-m_delta,
								buffer->Data(), buffer->Header()->size_used,
								m_output.format.u.raw_video);
						}
						else {
							Play(buffer->Header()->start_time-m_delta,
								buffer->Data(), buffer->Header()->size_used,
								m_output.format.u.raw_video);
						}
#endif
						
						// Update our frame counter and send the buffer off!
						// If the send is successful, the last consumer to use
						// the buffer will recycle it for us.
						m_frames_played ++;
						if (acquire_sem_etc(m_sendBufferSem, 1, B_TIMEOUT, 30000) != B_OK) {
							WARNING(stderr, "XaoSProducer: couldn't acquire send buffer sem\n");
							buffer->Recycle();
						} else {
							if (SendBuffer(buffer, m_output.destination) < B_OK) {
								// On the other hand, if the send is unsuccessful,
								// we mustn't forget to recycle the buffer ourselves.
								buffer->Recycle();
							}
							release_sem(m_sendBufferSem);
						}
					}
					else {
						// Something has gone screwy with our buffer group. To
						// avoid spewing lots of debug output, we'll only print
						// a message once every 256 occurrences.
						static int32 warning_cnt = 0;
						if (!(atomic_add(&warning_cnt, 1) & 255)) {
							WARNING(stderr, "XaoSProducer: RequestBuffer() failed\n");
						}
					}
				}
				else {
					// Time is running, but there's no reason for us to
					// actually process a buffer, becuase we won't be
					// sending it -- we're not connected, running, or
					// we've been told to shut up. We'll fake up timing
					// values so that our next timeout is as short as
					// possible.
					bigtime_t now = TimeSource()->Now();

					// Pretend that our next performance is exactly our
					// latency's worth away (i.e. that we need to start
					// processing right away).
					buffer_perf = now+TotalLatency();
					// Set m_frames_played based on this value, so that the
					// top of the next loop will Do the Right Thing.
#if 0
					m_frames_played = frames_for_duration(m_output.format.u.raw_video,
						buffer_perf-m_delta);
#endif
					m_frames_played = (int)((buffer_perf - m_delta) * m_output.format.u.raw_video.field_rate / 1000000);
				}
			}
			else {
				// We can't do anything when time isn't running.
				TRANSPORT(stderr, "time source is not running\n");
			}
		}
	}
}


void XaoSProducer::alloc_buffers()
{
	delete m_buffers;
        int size;
	char fmt[100];

	bigtime_t latency = TotalLatency();
	int count = (int)(latency * m_output.format.u.raw_video.field_rate / 1000000+ 2);
	fprintf(stderr,"XaoSProducer:: alloc_buffers (latency:%i count:%i field rate:%i)\n",(int)latency, (int)count, (int)m_output.format.u.raw_video.field_rate);
		string_for_format(m_output.format, fmt, 100);
		FORMAT(stderr, "We've decided to use %s\n", fmt);

	// But if count is too small, we'd like to have three buffers
	// at the very least, to give us some stability.
	if (count < 3) count = 3;
	size = m_output.format.u.raw_video.display.line_count*m_output.format.u.raw_video.display.bytes_per_row;
#define MAXSIZE (1024*1024*4) /*Maximal bufer is 4MB*/
	if (count*(long long)size > (long long)MAXSIZE) {
		count = MAXSIZE/size;
		if (count < 1) count = 1;
         }
#if 0
	// We should set a reasonable maximum on the size of our memory
	// pool. We'll restrict it to 128K.
	if (count*m_output.format.u.raw_video.buffer_size > 128000) {
		count = 128000/m_output.format.u.raw_audio.buffer_size;
		// We do need to make sure there's at least one buffer, though.
		if (count < 1) count = 1;
	}
#endif
	NODE(stderr, "Need %d buffers of size %i\n", count, size);
	m_buffers = new BBufferGroup(size, count);
}


bigtime_t
XaoSProducer::ProcessingLatency()
{
       /* XaoS attempts to keep framerate higher than 5 frames per second, but sometimes it is imposible */

	return (1000000/25);
}


bigtime_t
XaoSProducer::TotalLatency()
{
	return ProcessingLatency() + m_downstream_latency + m_private_latency;
}


void
XaoSProducer::Play(
	bigtime_t /* time */,
	void * /* data */,
	size_t /* size */,
	const media_raw_video_format & /* format */)
{
	//	If there is no play hook installed, we instead call this function
	//	for received buffers.
}

void
XaoSProducer::Notify(
	int32 /* cause */,
	...)
{
	//	If there is no notification hook installed, we instead call this function
	//	for giving notification of various events.
}



