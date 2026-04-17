package com.example.voxify

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.modifier.modifierLocalMapOf
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.voxify.ui.theme.VoxifyTheme

enum class RecorderState { IDLE, RECORDING, PLAYING }

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            VoxifyTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) {
                    VoxifyScreen()
                }
            }
        }
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Composable
fun VoxifyScreen() {

    var state by remember { mutableStateOf(RecorderState.IDLE) }
    var hasRecording by remember { mutableStateOf(false) }
    var gain by remember { mutableFloatStateOf(1f) }

    Column (
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        // Title
        Text(
            text = "VoicePlay",
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
                    state = RecorderState.RECORDING
                    hasRecording = true
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
                    state = RecorderState.IDLE
                } else {
                    // TODO: start playback via Oboe
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

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    VoxifyTheme {
        Greeting("Android")
    }
}
