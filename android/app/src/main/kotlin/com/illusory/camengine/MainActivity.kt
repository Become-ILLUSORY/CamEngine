package com.illusory.camengine

import android.os.Bundle
import android.util.Size
import android.view.Surface
import androidx.annotation.NonNull
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import io.flutter.view.TextureRegistry
import java.util.concurrent.Executors

class MainActivity : FlutterActivity() {
    private val CHANNEL = "com.illusory.camengine/engine"

    private var textureEntry: TextureRegistry.SurfaceTextureEntry? = null
    private var cameraSurface: Surface? = null
    private var cameraProvider: ProcessCameraProvider? = null
    private val cameraExecutor = Executors.newSingleThreadExecutor()

    init {
        System.loadLibrary("camengine_jni")
    }

    private external fun nativeGetEngineVersion(): String

    private var glController: CameraGlController? = null

    override fun configureFlutterEngine(@NonNull flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL)
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "getEngineVersion" -> result.success(nativeGetEngineVersion())
                    "startCameraTexture" -> startCameraTexture(result)
                    "stopCameraTexture" -> stopCameraTexture(result)
                    "startGlCamera" -> {
                        try {
                            val ctrl = glController ?: CameraGlController(this, flutterEngine.renderer)
                            glController = ctrl
                            val id = ctrl.start()
                            result.success(id)
                        } catch (e: Exception) {
                            result.error("GL", "启动失败: ${e.message}", null)
                        }
                    }
                    "stopGlCamera" -> {
                        glController?.stop()
                        glController = null
                        result.success(null)
                    }
                    "setGradeParams" -> {
                        val a = call.arguments as? List<*>
                        if (a != null && a.size >= 5) {
                            glController?.setParams(
                                (a[0] as Number).toFloat(), (a[1] as Number).toFloat(),
                                (a[2] as Number).toFloat(), (a[3] as Number).toFloat(),
                                (a[4] as Number).toFloat()
                            )
                        }
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }
    }

    /**
     * M2a: 创建 Flutter 纹理，把 CameraX 预览帧直通到该纹理。
     * Dart 侧用 Texture(textureId:) 显示。
     */
    private fun startCameraTexture(result: MethodChannel.Result) {
        try {
            if (textureEntry != null) {
                // 已在运行，直接返回现有 id
                result.success(textureEntry!!.id())
                return
            }
            val fe = flutterEngine
            if (fe == null) {
                result.error("ENG", "Flutter 引擎未就绪", null)
                return
            }
            val entry = fe.renderer.createSurfaceTexture()
            textureEntry = entry
            val st = entry.surfaceTexture()
            st.setDefaultBufferSize(1280, 720)
            val surface = Surface(st)
            cameraSurface = surface

            val providerFuture = ProcessCameraProvider.getInstance(this)
            providerFuture.addListener({
                try {
                    val provider = providerFuture.get()
                    cameraProvider = provider
                    val preview = Preview.Builder()
                        .setTargetResolution(Size(1280, 720))
                        .build()
                    preview.setSurfaceProvider(cameraExecutor) { req ->
                        val res = req.resolution
                        st.setDefaultBufferSize(res.width, res.height)
                        req.provideSurface(surface, cameraExecutor) { _ -> }
                    }
                    provider.unbindAll()
                    provider.bindToLifecycle(this, CameraSelector.DEFAULT_BACK_CAMERA, preview)
                    runOnUiThread { result.success(entry.id()) }
                } catch (e: Exception) {
                    runOnUiThread { result.error("CAM", "相机绑定失败: ${e.message}", null) }
                }
            }, ContextCompat.getMainExecutor(this))
        } catch (e: Exception) {
            result.error("CAM", "启动失败: ${e.message}", null)
        }
    }

    private fun stopCameraTexture(result: MethodChannel.Result) {
        try {
            cameraProvider?.unbindAll()
            cameraProvider = null
            cameraSurface?.release()
            cameraSurface = null
            textureEntry?.release()
            textureEntry = null
            result.success(null)
        } catch (e: Exception) {
            result.error("CAM", "停止失败: ${e.message}", null)
        }
    }

    override fun onDestroy() {
        try {
            glController?.stop()
            glController = null
            cameraProvider?.unbindAll()
            cameraSurface?.release()
            textureEntry?.release()
            cameraExecutor.shutdown()
        } catch (_: Exception) {
        }
        super.onDestroy()
    }
}
