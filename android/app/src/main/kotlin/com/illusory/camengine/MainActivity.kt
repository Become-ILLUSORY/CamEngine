package com.illusory.camengine

import android.os.Bundle
import androidx.annotation.NonNull
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.Result
import io.flutter.plugin.common.MethodChannel.MethodCallHandler

class MainActivity : FlutterActivity() {
    private val CHANNEL = "com.illusory.camengine/engine"

    init {
        System.loadLibrary("camengine_jni")
    }

    private external fun nativeGetEngineVersion(): String

    override fun configureFlutterEngine(@NonNull flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL)
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "getEngineVersion" -> {
                        val version = nativeGetEngineVersion()
                        result.success(version)
                    }
                    else -> result.notImplemented()
                }
            }
    }
}