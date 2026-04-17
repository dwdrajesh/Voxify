
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
