package com.illusory.camengine

import android.graphics.SurfaceTexture
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Surface
import androidx.camera.core.Camera
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import io.flutter.view.TextureRegistry
import java.util.concurrent.Executors

/**
 * M2b: CameraX → OES纹理 → C++ GL调色 → Flutter Texture
 * 实时滤镜相机管道。
 */
class CameraGlController(
    private val activity: LifecycleOwner,
    private val textureRegistry: TextureRegistry,
) {
    companion object {
        init { System.loadLibrary("camengine_jni") }
    }

    // ---- native 方法 ----
    private external fun nativeInit(): Long
    private external fun nativeGetOesTexture(h: Long): Int
    private external fun nativeSetOutputSurface(h: Long, surface: Surface?): Boolean
    private external fun nativeMakeCurrent(h: Long): Boolean
    private external fun nativeRenderFrame(
        h: Long, matrix: FloatArray, rot: Int,
        sat: Float, con: Float, exp: Float, temp: Float, str: Float
    ): Boolean
    private external fun nativeSwap(h: Long): Boolean
    private external fun nativeRelease(h: Long)

    // ---- 状态 ----
    private var nativeHandle = 0L
    private var glThread: HandlerThread? = null
    private var glHandler: Handler? = null
    private var flutterEntry: TextureRegistry.SurfaceTextureEntry? = null
    private var cameraSurfaceTexture: SurfaceTexture? = null
    private var cameraSurface: Surface? = null
    private var cameraProvider: ProcessCameraProvider? = null
    private val cameraExecutor = Executors.newSingleThreadExecutor()
    private val stMatrix = FloatArray(16)

    @Volatile var saturation = 1.4f
    @Volatile var contrast = 1.15f
    @Volatile var exposure = 1.1f
    @Volatile var temperature = 0.15f
    @Volatile var strength = 0.8f
    @Volatile private var rotationDeg = 0
    @Volatile private var running = false

    /** 启动管道，返回 Flutter textureId；失败抛异常 */
    fun start(): Long {
        if (running) return flutterEntry?.id() ?: -1L

        nativeHandle = nativeInit()
        if (nativeHandle == 0L) throw RuntimeException("GL 初始化失败")

        val oesTex = nativeGetOesTexture(nativeHandle)
        if (oesTex == 0) throw RuntimeException("OES 纹理创建失败")

        // GL 渲染线程
        val ht = HandlerThread("CamGL")
        ht.start()
        glThread = ht
        val gh = Handler(ht.looper)
        glHandler = gh

        // 相机 SurfaceTexture（绑定到 OES 纹理）
        val camST = SurfaceTexture(oesTex)
        camST.setDefaultBufferSize(1280, 720)
        cameraSurfaceTexture = camST
        val camSurf = Surface(camST)
        cameraSurface = camSurf

        // Flutter 输出纹理
        val entry = textureRegistry.createSurfaceTexture()
        flutterEntry = entry
        val outSurf = Surface(entry.surfaceTexture())

        gh.post {
            if (!nativeMakeCurrent(nativeHandle)) return@post
            nativeSetOutputSurface(nativeHandle, outSurf)
        }

        // 帧到达 → GL 线程渲染
        camST.setOnFrameAvailableListener({ _ ->
            gh.post {
                if (!running) return@post
                try {
                    nativeMakeCurrent(nativeHandle)
                    camST.updateTexImage()
                    camST.getTransformMatrix(stMatrix)
                    nativeRenderFrame(
                        nativeHandle, stMatrix, rotationDeg,
                        saturation, contrast, exposure, temperature, strength
                    )
                    nativeSwap(nativeHandle)
                } catch (e: Exception) {
                    android.util.Log.e("CamGL", "render error", e)
                }
            }
        }, gh)

        // 绑定 CameraX
        val future = ProcessCameraProvider.getInstance(activity as android.content.Context)
        future.addListener({
            try {
                val provider = future.get()
                cameraProvider = provider
                val preview = Preview.Builder()
                    .setTargetResolution(Size(1280, 720))
                    .build()
                preview.setSurfaceProvider(cameraExecutor) { req ->
                    val res = req.resolution
                    camST.setDefaultBufferSize(res.width, res.height)
                    req.provideSurface(camSurf, cameraExecutor) { }
                }
                provider.unbindAll()
                val cam = provider.bindToLifecycle(
                    activity, CameraSelector.DEFAULT_BACK_CAMERA, preview
                )
                rotationDeg = cam.cameraInfo.rotationDegrees
                running = true
            } catch (e: Exception) {
                android.util.Log.e("CamGL", "bind failed", e)
            }
        }, ContextCompat.getMainExecutor(activity as android.content.Context))

        return entry.id()
    }

    fun stop() {
        running = false
        try {
            cameraProvider?.unbindAll()
        } catch (_: Exception) {}
        cameraProvider = null
        glHandler?.post {
            try {
                nativeSetOutputSurface(nativeHandle, null)
                nativeRelease(nativeHandle)
            } catch (_: Exception) {}
            nativeHandle = 0L
        }
        glThread?.quitSafely()
        glThread = null
        glHandler = null
        try { cameraSurface?.release() } catch (_: Exception) {}
        cameraSurface = null
        try { cameraSurfaceTexture?.release() } catch (_: Exception) {}
        cameraSurfaceTexture = null
        try { flutterEntry?.release() } catch (_: Exception) {}
        flutterEntry = null
        cameraExecutor.shutdown()
    }

    fun setParams(sat: Float, con: Float, exp: Float, temp: Float, str: Float) {
        saturation = sat; contrast = con; exposure = exp
        temperature = temp; strength = str
    }
}
