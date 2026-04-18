
#include <jni.h>
#include "AudioEngine.h"

static AudioEngine *engine = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeCreate(JNIEnv *, jobject) {
	if (engine == nullptr) {
		engine = new AudioEngine();
	}
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeDestroy(JNIEnv *, jobject) {
	delete engine;
	engine = nullptr;
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeStartRecording(JNIEnv *, jobject) {
	if (engine) engine->startRecording();
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeStopRecording(JNIEnv *, jobject) {
	if (engine) engine->stopRecording();
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeStartPlayback(JNIEnv *, jobject) {
	if (engine) engine->startPlayback();
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeStopPlayback(JNIEnv *, jobject) {
	if (engine) engine->stopPlayback();
}

JNIEXPORT jboolean JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeIsRecording(JNIEnv *, jobject) {
	return engine ? engine->isRecording() : false;
}

JNIEXPORT jboolean JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeIsPlaying(JNIEnv *, jobject) {
	return engine ? engine->isPlaying() : false;
}

JNIEXPORT jfloat JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeGetLevel(JNIEnv *, jobject) {
	return engine ? engine->getLevel() : 0.0f;
}

JNIEXPORT void JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeSetGain(JNIEnv *, jobject, jfloat gain) {
	if (engine) engine->setGain(gain);
}

JNIEXPORT jboolean JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeLoadModel(JNIEnv *env, jobject, jstring modelPath) {
	const char *path = env->GetStringUTFChars(modelPath, nullptr);
	bool result = engine ? engine->loadModel(path) : false;
	env->ReleaseStringUTFChars(modelPath, path);
	return result;
}

JNIEXPORT jstring JNICALL
Java_com_example_voxify_engine_AudioEngine_nativeTranscribe(JNIEnv *env, jobject, jboolean translate) {
	bool translateBoolean = (translate == 1) ? true : false;
	std::string text = engine ? engine->transcribe(translate) : "[Engine not initialized]";
	return env->NewStringUTF(text.c_str());
}

} // extern "C"
