/*******************************************************************************
/
/	File:			XaoSProducer.h
/
/   Description:	Produce an XaoS animation to pass along to some video-consuming Node.
/
*******************************************************************************/

#ifndef XAOSPRODUCER_H
#define XAOSPRODUCER_H

#include <BufferProducer.h>
#include <FileInterface.h>
#include <MediaAddOn.h>
#include <File.h>


class XaoSProducer :
	public BBufferProducer,
	public BFileInterface
{
public:
		XaoSProducer(const char * name, BMediaAddOn *addon);
		~XaoSProducer();


		// This is our conception of where we are in
		// terms of playing sounds -- it's the last
		// media time of the last valid buffer we sent.
		bigtime_t CurrentMediaTime();
		
		//	The MediaNode interface
public:
virtual	void	DisposeFileFormatCookie(int32);
virtual status_t GetDuration(bigtime_t *outduration);
virtual	status_t GetNextFileFormat(int32 *cookie, media_file_format *outFormat);
virtual		port_id ControlPort() const;
virtual		BMediaAddOn* AddOn(int32 * internal_id) const;
virtual status_t GetRef(entry_ref *outRef, char *outMimeType);
virtual status_t SetRef(const entry_ref &file, bool create, bigtime_t *outDuration);
virtual status_t SniffRef(const entry_ref &file, char *outMimeType, float *outQuality);
protected:
virtual void SetFormat(const struct media_raw_video_format *);
virtual	void Start(bigtime_t performance_time);
virtual	void Stop(bigtime_t performance_time, bool immediate);
virtual	void Seek(bigtime_t media_time, bigtime_t performance_time);
virtual	void SetRunMode(run_mode mode);
virtual	void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);
virtual	void Preroll();
virtual	void SetTimeSource(BTimeSource * time_source);
virtual	status_t HandleMessage(int32 message, const void * data, size_t size);

		//	The BufferProducer interface
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format);
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format);
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * out_change_count);
virtual	status_t GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output);
virtual	status_t DisposeOutputCookie(
				int32 cookie);
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
virtual	status_t VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * out_from_change_count);
virtual	status_t GetLatency(
				bigtime_t * out_lantency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name);
virtual	void Disconnect(
				const media_source & what,
				const media_destination & consumer);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * change_tag);

protected:
virtual	void Play(
				bigtime_t time,
				void * data,
				size_t size,
				const media_raw_video_format & format);
virtual	void Notify(
				int32 cause,
				...);

private:
		BMediaAddOn *addon;
		entry_ref xaf_ref;
		BFile	*xaf_file;
		media_output m_output;
		int offline;
		thread_id m_thread;
		port_id m_port;
		media_raw_video_format m_raw_format;
		int64 m_frames_played;
		BBufferGroup * m_buffers;
		bigtime_t m_downstream_latency;
		bigtime_t m_private_latency;
		sem_id m_sendBufferSem;
		
		// State variables
		bool m_running;		// currently producing buffers
		bool m_starting;	// a Start is pending
		bool m_stopping;	// a Stop is pending
		bool m_seeking;		// a Seek is pending

		// The "queue" of start, stop, and seek events
		// that we'll be handling. We don't currently
		// handle time warps.
		// My notation for times: tr = real time,
		// tp = performance time, tm = media time.
		bigtime_t m_tpStart;	// when we Start
		bigtime_t m_tpStop;		// when we Stop
		bigtime_t m_tpSeekAt;	// when we Seek
		bigtime_t m_tmSeekTo;	// target time for Seek
		
		// The transformation from media to peformance time.
		// d = p - m, so m + d = p.
		bigtime_t m_delta;

		// ProcessingLatency is the time it takes to process
		// a buffer. Override it if you're doing something
		// time-intensive in the play hook function.
virtual bigtime_t ProcessingLatency();

		void alloc_buffers();
		bigtime_t TotalLatency();

		// The actual thread doing the work.
static		status_t ThreadEntry(
				void * obj);
		void ServiceThread();
};

#endif 
