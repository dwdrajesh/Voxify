package com.example.voxify

import android.content.pm.PackageManager
import android.Manifest
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.example.voxify.ui.theme.VoxifyTheme


import com.example.voxify.engine.AudioEngine
import kotlinx.coroutines.delay

enum class RecorderState { IDLE, RECORDING, PLAYING }

class MainActivity : ComponentActivity() {
    private lateinit var audioEngine: AudioEngine

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        audioEngine = AudioEngine()

        enableEdgeToEdge()
        setContent {
            VoxifyTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) {
                    VoxifyScreen(audioEngine)
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        audioEngine.destroy()
    }
}


@Composable
fun VoxifyScreen(audioEngine: AudioEngine) {
    val context = LocalContext.current
    var state by remember { mutableStateOf(RecorderState.IDLE) }
    var hasPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO)
                    == PackageManager.PERMISSION_GRANTED
        )
    }

    var hasRecording by remember { mutableStateOf(false) }
    var level by remember { mutableFloatStateOf(0f) }

    var gain by remember { mutableFloatStateOf(1f) }

    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) {
        granted -> hasPermission = granted
    }

    // Poll level while recording or playing
    LaunchedEffect(state) {
        while (state == RecorderState.RECORDING || state == RecorderState.PLAYING) {
            level = audioEngine.getLevel()
            if (state == RecorderState.PLAYING && !audioEngine.isPlaying()) {
                state = RecorderState.IDLE
            }
            delay(50)
        }
        level = 0f
    }

    Column (
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        // Title
        Text(
            text = "Voxify",
            fontSize = 28.sp,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.primary
        )

        // Recording playing status
        Text (
            text = when (state) {
                RecorderState.RECORDING -> "Recording..."
                RecorderState.PLAYING -> "Playing..."
                RecorderState.IDLE -> if (hasRecording) "Ready to play or record" else "Tap Record to start"
            },
            fontSize = 14.sp,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        Spacer(modifier = Modifier.height(48.dp))

        // Level meter (static placeholder for now)
        Box (
            modifier = Modifier.fillMaxWidth()
                .height(24.dp)
                .clip(RoundedCornerShape(12.dp))
                .background(MaterialTheme.colorScheme.surfaceVariant)
        )

        Spacer(modifier = Modifier.height(48.dp))

        // Button for record/play
        if (state == RecorderState.RECORDING) {
            Button(
                onClick = {
                    audioEngine.stopRecording()
                    state = RecorderState.IDLE
                    hasRecording = true
                },
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFE53935)),
                shape = CircleShape,
                modifier = Modifier
                    .size(80.dp),
//                    .clip(CircleShape),
                contentPadding = PaddingValues(0.dp)
            ) {
                Box (
                    modifier = Modifier
                        .size(28.dp)
                        .background(Color.White, RoundedCornerShape(4.dp))
                )
            }
            Spacer(modifier = Modifier.height(8.dp))

            Text("Stop", fontSize = 14.sp)

        } else {
            Button(
                onClick = {

                    // First tap on record requests permission
                    if (!hasPermission) {
                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                        return@Button
                    }

                    if (state == RecorderState.PLAYING) {
                        audioEngine.stopPlayback()
                    }
                    audioEngine.startRecording()
                    state = RecorderState.RECORDING
                },
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFE53935)),
                shape = CircleShape,
                modifier = Modifier
                    .size(80.dp),
//                    .clip(CircleShape),
                contentPadding = PaddingValues(0.dp),
                enabled = state != RecorderState.PLAYING

            ) {
                Box (
                    modifier = Modifier
                        .size(28.dp)
                        .background(Color.White, RoundedCornerShape(4.dp))
                )
            }
            Spacer(modifier = Modifier.height(8.dp))

            Text("Record", fontSize = 14.sp)
        }

        // Play Button
        Button(
            onClick = {
                if (state == RecorderState.PLAYING) {
                    // TODO: stop playback via Oboe
                    audioEngine.stopPlayback()
                    state = RecorderState.IDLE
                } else {
                    // TODO: start playback via Oboe
                    audioEngine.setGain(gain)
                    audioEngine.startPlayback()
                    state = RecorderState.PLAYING
                }
            },
            enabled = hasRecording && state != RecorderState.RECORDING,
            modifier = Modifier
                .fillMaxWidth()
                .height(48.dp)
        ) {
            Text(if (state == RecorderState.PLAYING) "Stop Playback" else "Play")
        }

        Spacer(modifier = Modifier.height(32.dp))

        // Gain slider
        Text("Gain: ${"%.1f".format(gain)}x", fontSize = 14.sp)
        Slider(
            value = gain,
            onValueChange = { gain = it },
            valueRange = 0.1f..3.0f,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

