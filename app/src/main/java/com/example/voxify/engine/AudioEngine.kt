package com.example.voxify.engine

class AudioEngine {
    init {
        System.loadLibrary("voxify")
        nativeCreate()
    }

    fun startRecording() = nativeStartRecording()
    fun stopRecording() = nativeStopRecording()
    fun startPlayback() = nativeStartPlayback()
    fun stopPlayback() = nativeStopPlayback()
    fun isRecording(): Boolean = nativeIsRecording()
    fun isPlaying(): Boolean = nativeIsPlaying()
    fun getLevel(): Float = nativeGetLevel()
    fun setGain(gain: Float) = nativeSetGain(gain)


    fun loadModel(path: String): Boolean = nativeLoadModel(path)
    fun transcribe(translate: Boolean) = nativeTranscribe(translate)

    fun destroy() = nativeDestroy()

    private external fun nativeCreate()
    private external fun nativeDestroy()
    private external fun nativeStartRecording()
    private external fun nativeStopRecording()
    private external fun nativeStartPlayback()
    private external fun nativeStopPlayback()
    private external fun nativeIsRecording(): Boolean
    private external fun nativeIsPlaying(): Boolean
    private external fun nativeGetLevel(): Float
    private external fun nativeSetGain(gain: Float)

    // whisper link
    private external fun nativeLoadModel(path: String): Boolean
    private external fun nativeTranscribe(translate: Boolean): String
}
