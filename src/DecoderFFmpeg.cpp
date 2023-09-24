//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "DecoderFFmpeg.h"
#include "Logger.h"
#include <fstream>
#include <condition_variable>
#include <string>
#include <mutex>
#include <thread.h>
#include <chrono>

#include "DecodeConfig.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavutil/tx.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

typedef struct RTexture {
	int w, h, d;
	void* rawdata;
} RTexture;

typedef struct MyAVPacketList {
    AVPacket *pkt;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    AVFifo *pkt_list;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    std::recursive_mutex *mutex;
    std::condition_variable_any *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams {
    int freq;
    AVChannelLayout ch_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

typedef struct FrameData {
    int64_t pkt_pos;
} FrameData;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    std::recursive_mutex *mutex;
    std::condition_variable_any *cond;
    PacketQueue *pktq;
} FrameQueue;

typedef struct Decoder {
    AVPacket *pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    std::condition_variable_any *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    std::thread *decoder_tid;
} Decoder;

typedef struct VideoState {
    std::thread *read_tid;
    char* filename;
    const AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    struct AudioParams audio_src;
    struct AudioParams audio_filter_src;
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    enum ShowMode {
        SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
    } show_mode;
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    AVTXContext *rdft;
    av_tx_fn rdft_fn;
    int rdft_bits;
    float *real_data;
    AVComplexFloat *rdft_data;
    int xpos;
    double last_vis_time;
    RTexture *vis_texture;
    RTexture *sub_texture;
    RTexture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *sub_convert_ctx;

    int width, height, xleft, ytop;
    int step;

    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    std::condition_variable_any *continue_read_thread;
} VideoState;

void init_dynload(void)
{
#if HAVE_SETDLLDIRECTORY && defined(_WIN32)
    /* Calling SetDllDirectory with the empty string (but not NULL) removes the
     * current working directory from the DLL search path as a security pre-caution. */
    SetDllDirectory("");
#endif
}

void DestroyTexture(RTexture* tex){

}

static int is_realtime(AVFormatContext *s)
{
    if(   !strcmp(s->iformat->name, "rtp")
       || !strcmp(s->iformat->name, "rtsp")
       || !strcmp(s->iformat->name, "sdp")
    )
        return 1;

    if(s->pb && (   !strncmp(s->url, "rtp:", 4)
                 || !strncmp(s->url, "udp:", 4)
                )
    )
        return 1;
    return 0;
}

static int decode_interrupt_cb(void *ctx)
{
    VideoState *is = (VideoState*)ctx;
    return is->abort_request;
}

static void WaitThread(std::thread *thread)
{
    if (!thread) {
        return;
    }

    try {
        std::thread *cpp_thread = thread;
        if (cpp_thread->joinable()) {
            cpp_thread->join();
        }
    } catch (std::system_error &) {
		LOG("Thread waiting error");
        // An error occurred when joining the thread.  SDL_WaitThread does not,
        // however, seem to provide a means to report errors to its callers
        // though!
    }
}

static std::condition_variable_any* CreateCondition(void)
{
    /* Allocate and initialize the condition variable */
    try {
        std::condition_variable_any *cond = new std::condition_variable_any();
        return cond;
    } catch (std::system_error &ex) {
        LOG("unable to create a C++ condition variable: code=%d; %s", ex.code(), ex.what());
        return NULL;
    } catch (std::bad_alloc &) {
        LOG("out of memory");
        return NULL;
    }
}

/* Destroy a condition variable */
static void DestroyCondition(std::condition_variable_any *cond)
{
    if (cond != NULL) {
        delete cond;
    }
}

/* Restart one of the threads that are waiting on the condition variable */
static int SignalCondition(std::condition_variable_any *cond)
{
    if (cond == NULL) {
		LOG("Invaild param error cond");
        return -1;
    }

    cond->notify_one();
    return 0;
}

/* Restart all threads that are waiting on the condition variable */
static int BroadcastCondition(std::condition_variable_any *cond)
{
    if (cond == NULL) {
        LOG("Invaild param error cond");
        return -1;
    }

    cond->notify_all();
    return 0;
}

static int WaitConditionTimeoutNS(std::condition_variable_any *cond, std::recursive_mutex *mutex, int64_t timeoutNS)
{
    if (cond == NULL) {
        LOG("Invaild param error cond");
        return -1;
    }

    if (mutex == NULL) {
        LOG("Invaild param error mutex");
        return -1;
    }

    try {
        std::unique_lock<std::recursive_mutex> cpp_lock(*mutex, std::adopt_lock_t());
        if (timeoutNS < 0) {
            cond->wait(
                cpp_lock);
            cpp_lock.release();
            return 0;
        } else {
            auto wait_result = cond->wait_for(
                cpp_lock,
                std::chrono::duration<int64_t, std::nano>(timeoutNS));
            cpp_lock.release();
            if (wait_result == std::cv_status::timeout) {
                return MUTEX_TIMEDOUT;
            } else {
                return 0;
            }
        }
    } catch (std::system_error &ex) {
        LOG("Unable to wait on a C++ condition variable: code=%d; %s", ex.code(), ex.what());
		return -1;
    }
}

static std::recursive_mutex * CreateMutex(void)
{
    /* Allocate and initialize the mutex */
    try {
        std::recursive_mutex *mutex = new std::recursive_mutex();
        return mutex;
    } catch (std::system_error &ex) {
        LOG("unable to create a C++ mutex: code=%d; %s", ex.code(), ex.what());
        return NULL;
    } catch (std::bad_alloc &) {
        LOG("out of memory");
        return NULL;
    }
}

/* Free the mutex */
static void DestroyMutex(std::recursive_mutex *mutex)
{
    if (mutex != NULL) {
        delete mutex;
    }
}

/* Lock the mutex */
static int LockMutex(std::recursive_mutex *mutex) /* clang doesn't know about NULL mutexes */
{
    if (mutex == NULL) {
        return 0;
    }

    try {
        mutex->lock();
        return 0;
    } catch (std::system_error &ex) {
        LOG("unable to lock a C++ mutex: code=%d; %s", ex.code(), ex.what());
		return -1;
    }
}

/* TryLock the mutex */
static int TryLockMutex(std::recursive_mutex *mutex)
{
    int retval = 0;

    if (mutex == NULL) {
        return 0;
    }

    if (mutex->try_lock() == false) {
        retval = MUTEX_TIMEDOUT;
    }
    return retval;
}

/* Unlock the mutex */
static int UnlockMutex(std::recursive_mutex *mutex) /* clang doesn't know about NULL mutexes */
{
    if (mutex == NULL) {
        return 0;
    }

    mutex->unlock();
    return 0;
}

static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
    MyAVPacketList pkt1;
    int ret;

    if (q->abort_request)
       return -1;


    pkt1.pkt = pkt;
    pkt1.serial = q->serial;

    ret = av_fifo_write(q->pkt_list, &pkt1, 1);
    if (ret < 0)
        return ret;
    q->nb_packets++;
    q->size += pkt1.pkt->size + sizeof(pkt1);
    q->duration += pkt1.pkt->duration;
    /* XXX: should duplicate packet data in DV case */
    SignalCondition(q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacket *pkt1;
    int ret;

    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt1);
    UnlockMutex(q->mutex);

    if (ret < 0)
        av_packet_free(&pkt1);

    return ret;
}

static int packet_queue_put_nullpacket(PacketQueue *q, AVPacket *pkt, int stream_index)
{
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->pkt_list = av_fifo_alloc2(1, sizeof(MyAVPacketList), AV_FIFO_FLAG_AUTO_GROW);
    if (!q->pkt_list)
        return AVERROR(ENOMEM);
    q->mutex = CreateMutex();
    if (!q->mutex) {
        av_log(NULL, AV_LOG_FATAL, "CreateMutex(): %s\n", -1);
        return AVERROR(ENOMEM);
    }
    q->cond = CreateCondition();
    if (!q->cond) {
        av_log(NULL, AV_LOG_FATAL, "CreateCond(): %s\n", -1);
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

static void packet_queue_flush(PacketQueue *q)
{
    MyAVPacketList pkt1;

    LockMutex(q->mutex);
    while (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0)
        av_packet_free(&pkt1.pkt);
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    q->serial++;
    UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
    packet_queue_flush(q);
    av_fifo_freep2(&q->pkt_list);
    DestroyMutex(q->mutex);
    DestroyCondition(q->cond);
}

static void packet_queue_abort(PacketQueue *q)
{
    LockMutex(q->mutex);

    q->abort_request = 1;

    SignalCondition(q->cond);

    UnlockMutex(q->mutex);
}

static void packet_queue_start(PacketQueue *q)
{
    LockMutex(q->mutex);
    q->abort_request = 0;
    q->serial++;
    UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList pkt1;
    int ret;

    LockMutex(q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        if (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0) {
            q->nb_packets--;
            q->size -= pkt1.pkt->size + sizeof(pkt1);
            q->duration -= pkt1.pkt->duration;
            av_packet_move_ref(pkt, pkt1.pkt);
            if (serial)
                *serial = pkt1.serial;
            av_packet_free(&pkt1.pkt);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            WaitConditionTimeoutNS(q->cond, q->mutex, 0);
        }
    }
    UnlockMutex(q->mutex);
    return ret;
}


static void frame_queue_unref_item(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = CreateMutex())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", -1);
        return AVERROR(ENOMEM);
    }
    if (!(f->cond = CreateCondition())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", -1);
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

static void frame_queue_destroy(FrameQueue *f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    DestroyMutex(f->mutex);
    DestroyCondition(f->cond);
}

static void frame_queue_signal(FrameQueue *f)
{
    LockMutex(f->mutex);
    SignalCondition(f->cond);
    UnlockMutex(f->mutex);
}


static double get_clock(Clock *c)
{
    if (*c->queue_serial != c->serial)
        return NAN;
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}

static void set_clock_at(Clock *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

static void set_clock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void set_clock_speed(Clock *c, double speed)
{
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

static void init_clock(Clock *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static void decoder_destroy(Decoder *d) {
    av_packet_free(&d->pkt);
    avcodec_free_context(&d->avctx);
}

static void decoder_abort(Decoder *d, FrameQueue *fq)
{
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    WaitThread(d->decoder_tid);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

DecoderFFmpeg::DecoderFFmpeg() {
	is->ic = nullptr;
	is->video_st = nullptr;
	is->audio_st = nullptr;
    is->subtitle_st = nullptr;
	//mVideoCodec = nullptr;
	//mAudioCodec = nullptr;
	mVideoCodecContext = nullptr;
	mAudioCodecContext = nullptr;
	av_init_packet(mPacket);

	mSwrContext = nullptr;

	mVideoBuffMax = 64;
	mAudioBuffMax = 128;

	decoder_reorder_pts = -1;
	av_sync_type = AV_SYNC_AUDIO_MASTER;
	seek_interval = 10;
	framedrop = -1;
	infinite_buffer = -1;
	afilters = NULL;
	find_stream_info = 1;
	startup_volume = 100;

	memset(&mVideoInfo, 0, sizeof(VideoInfo));
	memset(&mAudioInfo, 0, sizeof(AudioInfo));
	mIsInitialized = false;
	mIsAudioAllChEnabled = false;
	mUseTCP = false;
	mIsSeekToAny = false;

#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();
}

DecoderFFmpeg::~DecoderFFmpeg() {
	destroy();
}

bool DecoderFFmpeg::init(const char* filePath) {
	if (mIsInitialized) {
		LOG("Decoder has been init. \n");
		return true;
	}

	if (filePath == nullptr) {
		LOG("File path is nullptr. \n");
		return false;
	}

	init_dynload();

    is = (VideoState*) av_mallocz(sizeof(VideoState));
	if (!is)
        return NULL;
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->last_subtitle_stream = is->subtitle_stream = -1;
	is->filename = av_strdup(filePath);
    if (!is->filename)
        goto fail;
    is->iformat = nullptr;
    is->ytop    = 0;
    is->xleft   = 0;

/* start video display */
    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        goto fail;
    if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
        goto fail;
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

	if (packet_queue_init(&is->videoq) < 0 ||
        packet_queue_init(&is->audioq) < 0 ||
        packet_queue_init(&is->subtitleq) < 0)
        goto fail;

    if (!(is->continue_read_thread = CreateCondition())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", -1);
        goto fail;
    }

	init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
	is->audio_clock_serial = -1;
    if (startup_volume < 0)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
    if (startup_volume > 100)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
    startup_volume = av_clip(startup_volume, 0, 100);
    startup_volume = av_clip(MIX_MAXVOLUME * startup_volume / 100, 0, MIX_MAXVOLUME);
    is->audio_volume = startup_volume;
    muted = 0;
    is->av_sync_type = av_sync_type;
	
    is->read_tid = &std::thread(read_thread, is);
	if (!is->read_tid) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", -1);
fail:
        stream_close(is);
        return NULL;
    }

	return is;
}

bool DecoderFFmpeg::decode() {
	
	if (!mIsInitialized) {
		LOG("Not initialized. \n");
		return false;
	}

	if (!isBuffBlocked()) {
		if (av_read_frame(is->ic, mPacket) < 0) {
			updateVideoFrame();
			LOG("End of file.\n");
			return false;
		}

		if (mVideoInfo.isEnabled && mPacket->stream_index == is->video_st->index) {
			updateVideoFrame();
		} else if (mAudioInfo.isEnabled && mPacket->stream_index == is->audio_st->index) {
			updateAudioFrame();
		}

		av_packet_unref(mPacket);
	}

	return true;
}

IDecoder::VideoInfo DecoderFFmpeg::getVideoInfo() {
	return mVideoInfo;
}

IDecoder::AudioInfo DecoderFFmpeg::getAudioInfo() {
	return mAudioInfo;
}

void DecoderFFmpeg::setVideoEnable(bool isEnable) {
	if (is->video_st == nullptr) {
		LOG("Video stream not found. \n");
		return;
	}

	mVideoInfo.isEnabled = isEnable;
}

void DecoderFFmpeg::setAudioEnable(bool isEnable) {
	if (is->audio_st == nullptr) {
		LOG("Audio stream not found. \n");
		return;
	}

	mAudioInfo.isEnabled = isEnable;
}

void DecoderFFmpeg::setAudioAllChDataEnable(bool isEnable) {
	mIsAudioAllChEnabled = isEnable;
	initSwrContext();
}

int DecoderFFmpeg::initSwrContext() {
	if (mAudioCodecContext == nullptr) {
		LOG("Audio context is null. \n");
		return -1;
	}

	int errorCode = 0;
	int64_t inChannelLayout = av_get_default_channel_layout(mAudioCodecContext->channels);
	uint64_t outChannelLayout = mIsAudioAllChEnabled ? inChannelLayout : AV_CH_LAYOUT_STEREO;
	AVSampleFormat inSampleFormat = mAudioCodecContext->sample_fmt;
	AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_FLT;
	int inSampleRate = mAudioCodecContext->sample_rate;
	int outSampleRate = inSampleRate;

	if (mSwrContext != nullptr) {
		swr_close(mSwrContext);
		swr_free(&mSwrContext);
		mSwrContext = nullptr;
	}

	mSwrContext = swr_alloc_set_opts(nullptr,
		outChannelLayout, outSampleFormat, outSampleRate,
		inChannelLayout, inSampleFormat, inSampleRate,
		0, nullptr);

	
	if (swr_is_initialized(mSwrContext) == 0) {
		errorCode = swr_init(mSwrContext);
	}

	//	Save the output audio format
	mAudioInfo.channels = av_get_channel_layout_nb_channels(outChannelLayout);
	mAudioInfo.sampleRate = outSampleRate;
	mAudioInfo.totalTime = is->audio_st->duration <= 0 ? (double)(is->ic->duration) / AV_TIME_BASE : is->audio_st->duration * av_q2d(is->audio_st->time_base);
	
	return errorCode;
}

double DecoderFFmpeg::getVideoFrame(void** frameData) {
	std::lock_guard<std::mutex> lock(mVideoMutex);
	
	if (!mIsInitialized || mVideoFrames.size() == 0) {
		LOG("Video frame not available. \n");
        *frameData = nullptr;
		return -1;
	}

	AVFrame* frame = mVideoFrames.front();
	*frameData = frame->data[0];

	int64_t timeStamp = frame->best_effort_timestamp;
	double timeInSec = av_q2d(is->video_st->time_base) * timeStamp;
	mVideoInfo.lastTime = timeInSec;

    printf("mVideoInfo.lastTime %f\n", timeInSec);

	return timeInSec;
}

double DecoderFFmpeg::getAudioFrame(unsigned char** outputFrame, int& frameSize) {
	std::lock_guard<std::mutex> lock(mAudioMutex);
	if (!mIsInitialized || mAudioFrames.size() == 0) {
		LOG("Audio frame not available. \n");
		*outputFrame = nullptr;
		return -1;
	}

	AVFrame* frame = mAudioFrames.front();
	*outputFrame = frame->data[0];
	frameSize = frame->nb_samples;
	int64_t timeStamp = frame->best_effort_timestamp;
	double timeInSec = av_q2d(is->audio_st->time_base) * timeStamp;
	mAudioInfo.lastTime = timeInSec;

	return timeInSec;
}

void DecoderFFmpeg::seek(double time) {
	if (!mIsInitialized) {
		LOG("Not initialized. \n");
		return;
	}

	uint64_t timeStamp = (uint64_t) time * AV_TIME_BASE;

	if (0 > av_seek_frame(is->ic, -1, timeStamp, mIsSeekToAny ? AVSEEK_FLAG_ANY : AVSEEK_FLAG_BACKWARD)) {
		LOG("Seek time fail.\n");
		return;
	}

	if (mVideoInfo.isEnabled) {
		if (mVideoCodecContext != nullptr) {
			avcodec_flush_buffers(mVideoCodecContext);
		}
		flushBuffer(&mVideoFrames, &mVideoMutex);
		mVideoInfo.lastTime = -1;
	}
	
	if (mAudioInfo.isEnabled) {
		if (mAudioCodecContext != nullptr) {
			avcodec_flush_buffers(mAudioCodecContext);
		}
		flushBuffer(&mAudioFrames, &mAudioMutex);
		mAudioInfo.lastTime = -1;
	}
}

int DecoderFFmpeg::getMetaData(char**& key, char**& value) {
	if (!mIsInitialized || key != nullptr || value != nullptr) {
		return 0;
	}

	AVDictionaryEntry *tag = nullptr;
	int metaCount = av_dict_count(is->ic->metadata);

	key = (char**)malloc(sizeof(char*) * metaCount);
	value = (char**)malloc(sizeof(char*) * metaCount);

	for (int i = 0; i < metaCount; i++) {
		tag = av_dict_get(is->ic->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
		key[i] = tag->key;
		value[i] = tag->value;
	}

	return metaCount;
}

void DecoderFFmpeg::destroy() {
	if (mVideoCodecContext != nullptr) {
		avcodec_close(mVideoCodecContext);
		mVideoCodecContext = nullptr;
	}
	
	if (mAudioCodecContext != nullptr) {
		avcodec_close(mAudioCodecContext);
		mAudioCodecContext = nullptr;
	}
	
	if (is->ic != nullptr) {
		avformat_close_input(&is->ic);
		avformat_free_context(is->ic);
		is->ic = nullptr;
	}
	
	if (mSwrContext != nullptr) {
		swr_close(mSwrContext);
		swr_free(&mSwrContext);
		mSwrContext = nullptr;
	}
	
	flushBuffer(&mVideoFrames, &mVideoMutex);
	flushBuffer(&mAudioFrames, &mAudioMutex);
	flushBuffer(&mSubtitleFrames, &mSubtitleMutex);
	
	//mVideoCodec = nullptr;
	//mAudioCodec = nullptr;
	
	is->video_st = nullptr;
	is->audio_st = nullptr;
	is->subtitle_st = nullptr;
	av_packet_unref(mPacket);
	
	memset(&mVideoInfo, 0, sizeof(VideoInfo));
	memset(&mAudioInfo, 0, sizeof(AudioInfo));

	
	mIsInitialized = false;
	mIsAudioAllChEnabled = false;
	mVideoBuffMax = 64;
	mAudioBuffMax = 128;
	mUseTCP = false;
	mIsSeekToAny = false;
}

bool DecoderFFmpeg::isBuffBlocked() {
	bool ret = false;
	if (mVideoInfo.isEnabled && mVideoFrames.size() >= mVideoBuffMax) {
		ret = true;
	}

	if (mAudioInfo.isEnabled && mAudioFrames.size() >= mAudioBuffMax) {
		ret = true;
	}

	return ret;
}

void DecoderFFmpeg::updateVideoFrame() {
	int isFrameAvailable = 0;
	AVFrame* srcFrame = av_frame_alloc();
	clock_t start = clock();

	if (avcodec_decode_video2(mVideoCodecContext, srcFrame, &isFrameAvailable, &mPacket) < 0) {
		LOG("Error processing data. \n");
		return;
	}

	if (isFrameAvailable) {
        int width = srcFrame->width;
        int height = srcFrame->height;

        const AVPixelFormat dstFormat = AV_PIX_FMT_RGB24;
        AVFrame* dstFrame = av_frame_alloc();
        av_frame_copy_props(dstFrame, srcFrame);

        dstFrame->format = dstFormat;

        //av_image_alloc(dstFrame->data, dstFrame->linesize, dstFrame->width, dstFrame->height, dstFormat, 0)
        int numBytes = av_image_get_buffer_size(dstFormat, width, height, 1);
        AVBufferRef* buffer = av_buffer_alloc(numBytes*sizeof(uint8_t));
        avpicture_fill((AVPicture *)dstFrame,buffer->data,dstFormat,width,height);
        dstFrame->buf[0] = buffer;

        SwsContext* conversion = sws_getContext(width,
                                                height,
                                                (AVPixelFormat)srcFrame->format,
                                                width,
                                                height,
                                                dstFormat,
                                                SWS_FAST_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);
        sws_scale(conversion, srcFrame->data, srcFrame->linesize, 0, height, dstFrame->data, dstFrame->linesize);
        sws_freeContext(conversion);

        dstFrame->format = dstFormat;
        dstFrame->width = srcFrame->width;
        dstFrame->height = srcFrame->height;

        av_frame_free(&srcFrame);

        LOG("updateVideoFrame = %f\n", (float)(clock() - start) / CLOCKS_PER_SEC);

		std::lock_guard<std::mutex> lock(mVideoMutex);
		mVideoFrames.push(dstFrame);
		updateBufferState();
	}
}

void DecoderFFmpeg::updateAudioFrame() {
	int isFrameAvailable = 0;
	AVFrame* frameDecoded = av_frame_alloc();
	if (avcodec_decode_audio4(mAudioCodecContext, frameDecoded, &isFrameAvailable, &mPacket) < 0) {
		LOG("Error processing data. \n");
		return;
	}

	AVFrame* frame = av_frame_alloc();
	frame->sample_rate = frameDecoded->sample_rate;
	frame->channel_layout = av_get_default_channel_layout(mAudioInfo.channels);
	frame->format = AV_SAMPLE_FMT_FLT;	//	For Unity format.
	frame->best_effort_timestamp = frameDecoded->best_effort_timestamp;
	swr_convert_frame(mSwrContext, frame, frameDecoded);

	std::lock_guard<std::mutex> lock(mAudioMutex);
	mAudioFrames.push(frame);
	updateBufferState();
	av_frame_free(&frameDecoded);
}

void DecoderFFmpeg::freeVideoFrame() {
	freeFrontFrame(&mVideoFrames, &mVideoMutex);
}

void DecoderFFmpeg::freeAudioFrame() {
	freeFrontFrame(&mAudioFrames, &mAudioMutex);
}

void DecoderFFmpeg::freeFrontFrame(std::queue<AVFrame*>* frameBuff, std::mutex* mutex) {
	std::lock_guard<std::mutex> lock(*mutex);
	if (!mIsInitialized || frameBuff->size() == 0) {
		LOG("Not initialized or buffer empty. \n");
		return;
	}

	AVFrame* frame = frameBuff->front();
	av_frame_free(&frame);
	frameBuff->pop();
	updateBufferState();
}

//	frameBuff.clear would only clean the pointer rather than whole resources. So we need to clear frameBuff by ourself.
void DecoderFFmpeg::flushBuffer(std::queue<AVFrame*>* frameBuff, std::mutex* mutex) {
	std::lock_guard<std::mutex> lock(*mutex);
	while (!frameBuff->empty()) {
		av_frame_free(&(frameBuff->front()));
		frameBuff->pop();
	}
}

//	Record buffer state either FULL or EMPTY. It would be considered by ViveMediaDecoder.cs for buffering judgement.
void DecoderFFmpeg::updateBufferState() {
	if (mVideoInfo.isEnabled) {
		if (mVideoFrames.size() >= mVideoBuffMax) {
			mVideoInfo.bufferState = BufferState::FULL;
		} else if(mVideoFrames.size() == 0) {
			mVideoInfo.bufferState = BufferState::EMPTY;
		} else {
			mVideoInfo.bufferState = BufferState::NORMAL;
		}
	}

	if (mAudioInfo.isEnabled) {
		if (mAudioFrames.size() >= mAudioBuffMax) {
			mAudioInfo.bufferState = BufferState::FULL;
		} else if (mAudioFrames.size() == 0) {
			mAudioInfo.bufferState = BufferState::EMPTY;
		} else {
			mAudioInfo.bufferState = BufferState::NORMAL;
		}
	}
}

int DecoderFFmpeg::loadConfig() {
	std::ifstream configFile("config", std::ifstream::in);
	if (!configFile) {
		LOG("config does not exist.\n");
		return -1;
	}

	std::string line;
	while (configFile >> line) {
		std::string token = line.substr(0, line.find("="));
		std::string value = line.substr(line.find("=") + 1);
		try {
			if (token == "USE_TCP") { mUseTCP = stoi(value) != 0; }
			else if (token == "BUFF_VIDEO_MAX") { mVideoBuffMax = stoi(value); }
			else if (token == "BUFF_AUDIO_MAX") { mAudioBuffMax = stoi(value); }
			else if (token == "BUFF_SUBTITLE_MAX") { mSubtitleBuffMax = stoi(value); }
			else if (token == "VIDEO_disable") { mSubtitleBuffMax = stoi(value); }
			else if (token == "BUFF_SUBTITLE_MAX") { mSubtitleBuffMax = stoi(value); }
			else if (token == "BUFF_SUBTITLE_MAX") { mSubtitleBuffMax = stoi(value); }
			else if (token == "SEEK_ANY") { mIsSeekToAny = stoi(value) != 0; }
		
		} catch (...) {
			return -1;
		}
	}
	LOG("config loading success.\n");
	LOG("USE_TCP=%s\n", mUseTCP ? "true" : "false");
	LOG("BUFF_VIDEO_MAX=%d\n", mVideoBuffMax);
	LOG("BUFF_AUDIO_MAX=%d\n", mAudioBuffMax);
	LOG("SEEK_ANY=%s\n", mIsSeekToAny ? "true" : "false");

	return 0;
}

/* this thread gets the stream from the disk or the network */
int DecoderFFmpeg::read_thread(void *arg)
{
    VideoState *is = (VideoState*)arg;
    is->ic = NULL;
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket *pkt = NULL;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    const AVDictionaryEntry *t;
    std::recursive_mutex *wait_mutex = CreateMutex();
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;

    if (!wait_mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", -1);
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    eof = 0;

    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate packet.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    is->ic = avformat_alloc_context();
    if (!is->ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    is->ic->interrupt_callback.callback = decode_interrupt_cb;
    is->ic->interrupt_callback.opaque = is;
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    err = avformat_open_input(&is->ic, is->filename, is->iformat, &format_opts);
    if (err < 0) {
        LOG(is->filename, err);
        ret = -1;
        goto fail;
    }
    if (scan_all_pmts_set)
        av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

    if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    if (genpts)
        is->ic->flags |= AVFMT_FLAG_GENPTS;

    av_format_inject_global_side_data(is->ic);

    if (find_stream_info) {
        AVDictionary **opts;
        int orig_nb_streams = is->ic->nb_streams;

        err = setup_find_stream_info_opts(is->ic, codec_opts, &opts);
        if (err < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Error setting up avformat_find_stream_info() options\n");
            ret = err;
            goto fail;
        }

        err = avformat_find_stream_info(is->ic, opts);

        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (err < 0) {
            av_log(NULL, AV_LOG_WARNING,
                   "%s: could not find codec parameters\n", is->filename);
            ret = -1;
            goto fail;
        }
    }

    if (is->ic->pb)
        is->ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

    is->max_frame_duration = (is->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        /* add the stream start time */
        if (is->ic->start_time != AV_NOPTS_VALUE)
            timestamp += is->ic->start_time;
        ret = avformat_seek_file(is->ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                    is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(is->ic);

    if (show_status)
        av_dump_format(ic, 0, is->filename, 0);

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (type >= 0 && wanted_stream_spec[type] && st_index[type] == -1)
            if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0)
                st_index[type] = i;
    }
    for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (wanted_stream_spec[i] && st_index[i] == -1) {
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string(i));
            st_index[i] = INT_MAX;
        }
    }

    if (!video_disable)
        st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    if (!audio_disable)
        st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                st_index[AVMEDIA_TYPE_AUDIO],
                                st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);
    if (!video_disable && !subtitle_disable)
        st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                st_index[AVMEDIA_TYPE_SUBTITLE],
                                (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 st_index[AVMEDIA_TYPE_AUDIO] :
                                 st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);

    is->show_mode = show_mode;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters *codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        if (codecpar->width)
            set_default_window_size(codecpar->width, codecpar->height, sar);
    }

    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (is->show_mode == SHOW_MODE_NONE)
        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               is->filename);
        ret = -1;
        goto fail;
    }

    if (infinite_buffer < 0 && is->realtime)
        infinite_buffer = 1;

    for (;;) {
        if (is->abort_request)
            break;
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (is->paused &&
                (!strcmp(ic->iformat->name, "rtsp") ||
                 (ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
            SDL_Delay(10);
            continue;
        }
#endif
        if (is->seek_req) {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
            int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", is->ic->url);
            } else {
                if (is->audio_stream >= 0)
                    packet_queue_flush(&is->audioq);
                if (is->subtitle_stream >= 0)
                    packet_queue_flush(&is->subtitleq);
                if (is->video_stream >= 0)
                    packet_queue_flush(&is->videoq);
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                   set_clock(&is->extclk, NAN, 0);
                } else {
                   set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused)
                step_to_next_frame(is);
        }
        if (is->queue_attachments_req) {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                if ((ret = av_packet_ref(pkt, &is->video_st->attached_pic)) < 0)
                    goto fail;
                packet_queue_put(&is->videoq, pkt);
                packet_queue_put_nullpacket(&is->videoq, pkt, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }

        /* if the queue are full, no need to read more */
        if (infinite_buffer<1 &&
              (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
            || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
                stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
                stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
            /* wait 10 ms */
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        }
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
            if (loop != 1 && (!loop || --loop)) {
                stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
            } else if (autoexit) {
                ret = AVERROR_EOF;
                goto fail;
            }
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0)
                    packet_queue_put_nullpacket(&is->videoq, pkt, is->video_stream);
                if (is->audio_stream >= 0)
                    packet_queue_put_nullpacket(&is->audioq, pkt, is->audio_stream);
                if (is->subtitle_stream >= 0)
                    packet_queue_put_nullpacket(&is->subtitleq, pkt, is->subtitle_stream);
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error) {
                if (autoexit)
                    goto fail;
                else
                    break;
            }
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        } else {
            is->eof = 0;
        }
        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range = duration == AV_NOPTS_VALUE ||
                (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                av_q2d(ic->streams[pkt->stream_index]->time_base) -
                (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
                <= ((double)duration / 1000000);
        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && pkt_in_play_range
                   && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            packet_queue_put(&is->videoq, pkt);
        } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
            packet_queue_put(&is->subtitleq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }

    ret = 0;
 fail:
    if (ic && !is->ic)
        avformat_close_input(&ic);

    av_packet_free(&pkt);
    if (ret != 0) {
        SDL_Event event;

        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
    }
    SDL_DestroyMutex(wait_mutex);
    return 0;
}

int DecoderFFmpeg::stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

int DecoderFFmpeg::frame_queue_nb_remaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}

void DecoderFFmpeg::stream_component_close(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    codecpar = ic->streams[stream_index]->codecpar;

    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        decoder_abort(&is->auddec, &is->sampq);
        SDL_CloseAudioDevice(audio_dev);
        decoder_destroy(&is->auddec);
        swr_free(&is->swr_ctx);
        av_freep(&is->audio_buf1);
        is->audio_buf1_size = 0;
        is->audio_buf = NULL;

        if (is->rdft) {
            av_tx_uninit(&is->rdft);
            av_freep(&is->real_data);
            av_freep(&is->rdft_data);
            is->rdft = NULL;
            is->rdft_bits = 0;
        }
        break;
    case AVMEDIA_TYPE_VIDEO:
        decoder_abort(&is->viddec, &is->pictq);
        decoder_destroy(&is->viddec);
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        decoder_abort(&is->subdec, &is->subpq);
        decoder_destroy(&is->subdec);
        break;
    default:
        break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_st = NULL;
        is->audio_stream = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = NULL;
        is->video_stream = -1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitle_st = NULL;
        is->subtitle_stream = -1;
        break;
    default:
        break;
    }
}

void DecoderFFmpeg::stream_close(VideoState *is)
{
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
	is->read_tid->join();
    WaitThread(is->read_tid);

    /* close each stream */
    if (is->audio_stream >= 0)
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
    if (is->subtitle_stream >= 0)
        stream_component_close(is, is->subtitle_stream);

    avformat_close_input(&is->ic);

    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    packet_queue_destroy(&is->subtitleq);

    /* free all pictures */
    frame_queue_destroy(&is->pictq);
    frame_queue_destroy(&is->sampq);
    frame_queue_destroy(&is->subpq);
    DestroyCondition(is->continue_read_thread);
    sws_freeContext(is->sub_convert_ctx);
    av_free(is->filename);
    if (is->vis_texture)
        DestroyTexture(is->vis_texture);
    if (is->vid_texture)
        DestroyTexture(is->vid_texture);
    if (is->sub_texture)
        DestroyTexture(is->sub_texture);
    av_free(is);
}

void DecoderFFmpeg::printErrorMsg(int errorCode) {
	char msg[500];
	av_strerror(errorCode, msg, sizeof(msg));
	LOG("Error massage: %s \n", msg);
}
