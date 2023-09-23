//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "DecoderFFmpeg.h"
#include "Logger.h"
#include <fstream>
#include <condition_variable>
#include <string>
#include <mutex>
#include <chrono>

#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavutil/tx.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

/* Synchronization functions which can time out return this value, if they time out. */
#define MUTEX_TIMEDOUT  1
/* This is the timeout value which corresponds to never time out. */
#define MUTEX_MAXWAIT   -1

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

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

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

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
    const AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int paused;
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
    int muted;
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
    int eof;

    char *filename;
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

std::condition_variable_any* CreateCondition(void)
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
void DestroyCondition(std::condition_variable_any *cond)
{
    if (cond != NULL) {
        delete cond;
    }
}

/* Restart one of the threads that are waiting on the condition variable */
int SignalCondition(std::condition_variable_any *cond)
{
    if (cond == NULL) {
		LOG("Invaild param error cond");
        return -1;
    }

    cond->notify_one();
    return 0;
}

/* Restart all threads that are waiting on the condition variable */
int BroadcastCondition(std::condition_variable_any *cond)
{
    if (cond == NULL) {
        LOG("Invaild param error cond");
        return -1;
    }

    cond->notify_all();
    return 0;
}

int WaitConditionTimeoutNS(std::condition_variable_any *cond, std::recursive_mutex *mutex, int64_t timeoutNS)
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

std::recursive_mutex * CreateMutex(void)
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
void DestroyMutex(std::recursive_mutex *mutex)
{
    if (mutex != NULL) {
        delete mutex;
    }
}

/* Lock the mutex */
int LockMutex(std::recursive_mutex *mutex) /* clang doesn't know about NULL mutexes */
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
int TryLockMutex(std::recursive_mutex *mutex)
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
int UnlockMutex(std::recursive_mutex *mutex) /* clang doesn't know about NULL mutexes */
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


DecoderFFmpeg::DecoderFFmpeg() {
	mAVFormatContext = nullptr;
	mVideoStream = nullptr;
	mAudioStream = nullptr;
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

	VideoState *is;

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

	if (!is->read_tid) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", -1);
fail:
        stream_close(is);
        return NULL;
    }

	mPacket = av_packet_alloc();

	if (mAVFormatContext == nullptr) {
		mAVFormatContext = avformat_alloc_context();
	}

	int errorCode = 0;
	errorCode = loadConfig();
	if (errorCode < 0) {
		LOG("config loading error. \n");
		LOG("Use default settings. \n");
		mVideoBuffMax = 64;
		mAudioBuffMax = 128;
		mUseTCP = false;
		mIsSeekToAny = false;
	}

	AVDictionary* opts = nullptr;
	if (mUseTCP) {
		av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	}
	
	errorCode = avformat_open_input(&mAVFormatContext, filePath, nullptr, &opts);
	av_dict_free(&opts);
	if (errorCode < 0) {
		LOG("avformat_open_input error(%x). \n", errorCode);
		printErrorMsg(errorCode);
		return false;
	}

	errorCode = avformat_find_stream_info(mAVFormatContext, nullptr);
	if (errorCode < 0) {
		LOG("avformat_find_stream_info error(%x). \n", errorCode);
		printErrorMsg(errorCode);
		return false;
	}

	if (mAVFormatContext->pb)
        mAVFormatContext->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

	mIsInitialized = true;

	return true;
}

bool DecoderFFmpeg::decode() {
	
	if (!mIsInitialized) {
		LOG("Not initialized. \n");
		return false;
	}

	if (!isBuffBlocked()) {
		if (av_read_frame(mAVFormatContext, mPacket) < 0) {
			updateVideoFrame();
			LOG("End of file.\n");
			return false;
		}

		if (mVideoInfo.isEnabled && mPacket->stream_index == mVideoStream->index) {
			updateVideoFrame();
		} else if (mAudioInfo.isEnabled && mPacket->stream_index == mAudioStream->index) {
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
	if (mVideoStream == nullptr) {
		LOG("Video stream not found. \n");
		return;
	}

	mVideoInfo.isEnabled = isEnable;
}

void DecoderFFmpeg::setAudioEnable(bool isEnable) {
	if (mAudioStream == nullptr) {
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
	mAudioInfo.totalTime = mAudioStream->duration <= 0 ? (double)(mAVFormatContext->duration) / AV_TIME_BASE : mAudioStream->duration * av_q2d(mAudioStream->time_base);
	
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
	double timeInSec = av_q2d(mVideoStream->time_base) * timeStamp;
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
	double timeInSec = av_q2d(mAudioStream->time_base) * timeStamp;
	mAudioInfo.lastTime = timeInSec;

	return timeInSec;
}

void DecoderFFmpeg::seek(double time) {
	if (!mIsInitialized) {
		LOG("Not initialized. \n");
		return;
	}

	uint64_t timeStamp = (uint64_t) time * AV_TIME_BASE;

	if (0 > av_seek_frame(mAVFormatContext, -1, timeStamp, mIsSeekToAny ? AVSEEK_FLAG_ANY : AVSEEK_FLAG_BACKWARD)) {
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
	int metaCount = av_dict_count(mAVFormatContext->metadata);

	key = (char**)malloc(sizeof(char*) * metaCount);
	value = (char**)malloc(sizeof(char*) * metaCount);

	for (int i = 0; i < metaCount; i++) {
		tag = av_dict_get(mAVFormatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
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
	
	if (mAVFormatContext != nullptr) {
		avformat_close_input(&mAVFormatContext);
		avformat_free_context(mAVFormatContext);
		mAVFormatContext = nullptr;
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
	
	mVideoStream = nullptr;
	mAudioStream = nullptr;
	mSubtitleStream = nullptr;
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
        int numBytes = avpicture_get_size(dstFormat, width, height);
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

void DecoderFFmpeg::printErrorMsg(int errorCode) {
	char msg[500];
	av_strerror(errorCode, msg, sizeof(msg));
	LOG("Error massage: %s \n", msg);
}
