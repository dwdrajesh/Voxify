
#include "AudioEngine.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "Voxify"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AudioEngine::AudioEngine() {
    LOGI("AudioEngine created");
}

AudioEngine::~AudioEngine() {
    stopRecording();
    stopPlayback();

    if (mWhisperContext) {
        whisper_free(mWhisperContext);
        mWhisperContext = nullptr;
    }
}


void AudioEngine::openRecordingStream() {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Input)
    ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
    ->setSharingMode(oboe::SharingMode::Exclusive)
    ->setFormat(oboe::AudioFormat::Float)
    ->setChannelCount(kChannelCount)
    ->setSampleRate(kSampleRate)
    ->setDataCallback(this);

    oboe::Result result = builder.openStream(mRecordingStream);
    if (result != oboe::Result::OK) {
    LOGE("Failed to open recording stream: %s", oboe::convertToText(result));
    } else {
    LOGI("Recording stream opened: sampleRate=%d, channelCount=%d, format=%d",
        mRecordingStream->getSampleRate(),
        mRecordingStream->getChannelCount(),
        (int)mRecordingStream->getFormat());
    }
}

void AudioEngine::openPlaybackStream() {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
    ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
    ->setSharingMode(oboe::SharingMode::Exclusive)
    ->setFormat(oboe::AudioFormat::Float)
    ->setChannelCount(kChannelCount)
    ->setSampleRate(kSampleRate)
    ->setDataCallback(this);

    oboe::Result result = builder.openStream(mPlaybackStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open playback stream: %s", oboe::convertToText(result));
    }
}

void AudioEngine::startRecording() {
    if (mIsRecording.load()) return;

    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        mRecordedSamples.clear();
    }

    openRecordingStream();
    if (mRecordingStream) {
        oboe::Result result = mRecordingStream->requestStart();
        if (result == oboe::Result::OK) {
            mIsRecording.store(true);
            LOGI("Recording started");
        } else {
            LOGE("Failed to start recording: %s", oboe::convertToText(result));
        }
    }
}
void AudioEngine::stopRecording() {
    if (!mIsRecording.load()) return;

    mIsRecording.store(false);
    if (mRecordingStream) {
        mRecordingStream->requestStop();
        mRecordingStream->close();
        mRecordingStream.reset();
    }
    mCurrentLevel.store(0.0f);
    LOGI("Recording stopped, samples: %zu", mRecordedSamples.size());
}



void AudioEngine::startPlayback() {
    if (mIsPlaying.load()) return;

    // guard by mutex
    {
            std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mRecordedSamples.empty()) {
        LOGI("No recorded audio to play");
        return;
        }
    }

    mPlaybackPosition = 0;
    openPlaybackStream();
    if (mPlaybackStream) {
        oboe::Result result = mPlaybackStream->requestStart();
        if (result == oboe::Result::OK) {
            mIsPlaying.store(true);
            LOGI("Playback started");
        } else {
            LOGE("Failed to start playback: %s", oboe::convertToText(result));
        }
    }
}

void AudioEngine::stopPlayback() {
    if (!mIsPlaying.load()) return;

    mIsPlaying.store(false);
    if (mPlaybackStream) {
        mPlaybackStream->requestStop();
        mPlaybackStream->close();
        mPlaybackStream.reset();
    }
    mCurrentLevel.store(0.0f);
    LOGI("Playback stopped");
}


oboe::DataCallbackResult AudioEngine::onAudioReady(
    oboe::AudioStream *stream, void *audioData, int32_t numFrames) {

    auto *data = static_cast<float *>(audioData);

    if (stream->getDirection() == oboe::Direction::Input && mIsRecording.load()) {
        // Compute level (RMS)
        float sumSquares = 0.0f;
        for (int i = 0; i < numFrames; i++) {
        sumSquares += data[i] * data[i];
        }

        float rms = std::sqrt(sumSquares / static_cast<float>(numFrames));
        mCurrentLevel.store(rms);

        // Store samples
        std::lock_guard<std::mutex> lock(mBufferMutex);
        mRecordedSamples.insert(mRecordedSamples.end(), data, data + numFrames);

    } else if (stream->getDirection() == oboe::Direction::Output && mIsPlaying.load()) {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        float gain = mGain.load();

        for (int i = 0; i < numFrames; i++) {
            if (mPlaybackPosition < static_cast<int64_t>(mRecordedSamples.size())) {
            data[i] = mRecordedSamples[mPlaybackPosition] * gain;
            mPlaybackPosition++;
            } else {
            data[i] = 0.0f;
            }
        }

        // Compute playback level
        float sumSquares = 0.0f;
        for (int i = 0; i < numFrames; i++) {
            sumSquares += data[i] * data[i];
        }
        mCurrentLevel.store(std::sqrt(sumSquares / static_cast<float>(numFrames)));

        // Stop when done
        if (mPlaybackPosition >= static_cast<int64_t>(mRecordedSamples.size())) {
            mIsPlaying.store(false);
            mCurrentLevel.store(0.0f);
            return oboe::DataCallbackResult::Stop;
        }
    }

    return oboe::DataCallbackResult::Continue;
}

bool AudioEngine::loadModel(const char *modelPath) {
    if (mWhisperContext) {
        whisper_free(mWhisperContext);
        mWhisperContext = nullptr;
    }

    struct whisper_context_params cparams = whisper_context_default_params();
    mWhisperContext = whisper_init_from_file_with_params(modelPath, cparams);

    if (mWhisperContext) {
        LOGI("Whisper model loaded: %s", modelPath);
        return true;
    } else {
        LOGE("Failed to load Whisper model: %s", modelPath);
        return false;
    }
}

std::string AudioEngine::transcribe(bool translate) {
    // stop both first
    stopRecording();
    stopPlayback();

    if (!mWhisperContext) {
        LOGE("Whisper model not loaded");
        return "[Error: model not loaded]";
    }

    std::vector<float> samples;
    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        samples = mRecordedSamples;
    }

    if (samples.empty()) {
        return "[No audio recorded]";
    }

    // Normalize samples
    float maxVal = 0.0f;
    for (size_t i = 0; i < samples.size(); i++) {
        float absVal = std::fabs(samples[i]);
        if (absVal > maxVal) maxVal = absVal;
    }

    LOGI("Audio max value before normalization: %f", maxVal);
//    if (maxVal > 0.0f) {
//        for (size_t i = 0; i < samples.size(); i++) {
//            samples[i] /= maxVal;
//        }
//    }

    LOGI("Transcribing %zu samples (%.1f seconds), translate: %d...",
        samples.size(), (float)samples.size() / kSampleRate, translate);

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress   = true;
    params.print_timestamps = true;
    params.print_realtime   = true;
    params.print_special    = false; // remove '_EOT_'
    params.translate        = translate;
    params.language         = "auto"; // "en";
    params.n_threads        = 4;
    params.single_segment   = true;
    params.no_timestamps    = true;

    std::string text;
    {
        std::lock_guard<std::mutex> lock(mWhisperMutex);
        int result = whisper_full(mWhisperContext, params, samples.data(), (int)samples.size());
        LOGI("whisper_full returned: %d", result);
        if (result != 0) {
            LOGE("Whisper transcription failed: %d", result);
            return "[Transcription failed]";
        }

        int nSegments = whisper_full_n_segments(mWhisperContext);
        LOGI("Number of segments: %d", nSegments);
        for (int i = 0; i < nSegments; i++) {
            const char *segText = whisper_full_get_segment_text(mWhisperContext, i);
            LOGI("Segment %d: '%s' (len=%d)", i, segText ? segText : "NULL", segText ? (int)strlen(segText) : -1);
            if (segText) {
                text += segText;
            }
        }
    }
    LOGI("Transcription result: %s", text.c_str());
    return text;
}
