
#pragma once

#include <oboe/Oboe.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <whisper.h>
#include <string>

/*
 * AudioStreamDataCallback — Oboe's callback interface. Oboe calls onAudioReady() on a high-priority audio thread whenever it needs/has audio data
 * mRecordedSamples — a vector<float> that stores all recorded PCM samples in memory
 * 48000 Hz, mono — standard sample rate, single channel for voice
 * mBufferMutex — protects the sample buffer since it's accessed from both the audio thread and the main thread
 */
class AudioEngine: public oboe::AudioStreamDataCallback {
public:
    AudioEngine();
    ~AudioEngine();

    void startRecording();
    void stopRecording();

    void startPlayback();
    void stopPlayback();

    bool isRecording() const { return mIsRecording.load(); }
    bool isPlaying() const { return mIsPlaying.load(); }
    float getLevel() const { return mCurrentLevel.load(); }
    void setGain(float gain) { mGain.store(gain); }

    // Whisper

    bool loadModel(const char *modelPath);
    std::string transcribe(bool translate);

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *stream,
            void *audioData,
            int32_t numFrames
            ) override;

private:

      void openRecordingStream();
      void openPlaybackStream();

      std::shared_ptr<oboe::AudioStream> mRecordingStream;
      std::shared_ptr<oboe::AudioStream> mPlaybackStream;


      std::vector<float> mRecordedSamples;
      std::mutex mBufferMutex;
      std::atomic<bool> mIsRecording{false};
      std::atomic<bool> mIsPlaying{false};
      std::atomic<float> mCurrentLevel{0.0f};
      std::atomic<float> mGain{1.0f};

      int64_t mPlaybackPosition = 0;

      // Whisper
      struct whisper_context *mWhisperContext = nullptr;

      static constexpr int kSampleRate = 16000; // whisper needs 16000
      static constexpr int kChannelCount = 1;
};
