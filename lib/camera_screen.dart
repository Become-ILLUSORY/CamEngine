import 'dart:io';
import 'package:flutter/material.dart';
import 'package:camera/camera.dart';
import 'package:path_provider/path_provider.dart';

class CameraScreen extends StatefulWidget {
  const CameraScreen({super.key});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  List<CameraDescription>? _cameras;
  CameraController? _controller;
  int _cameraIndex = 0;
  bool _ready = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    _initCameras();
  }

  Future<void> _initCameras() async {
    try {
      final cameras = await availableCameras();
      if (!mounted) return;
      setState(() => _cameras = cameras);
      await _initCamera();
    } catch (e) {
      if (!mounted) return;
      setState(() => _error = '相机列表获取失败: $e');
    }
  }

  Future<void> _initCamera() async {
    final cameras = _cameras;
    if (cameras == null || cameras.isEmpty) return;
    setState(() { _ready = false; _error = null; });
    final c = CameraController(
      cameras[_cameraIndex],
      ResolutionPreset.high,
      enableAudio: false,
      imageFormatGroup: ImageFormatGroup.jpeg,
    );
    _controller = c;
    try {
      await c.initialize();
      if (!mounted) return;
      setState(() { _ready = true; });
    } catch (e) {
      if (!mounted) return;
      setState(() { _error = '相机初始化失败: $e'; });
    }
  }

  Future<void> _takePicture() async {
    final c = _controller;
    if (c == null || !c.value.isInitialized) return;
    try {
      final dir = await getApplicationDocumentsDirectory();
      final file = File('${dir.path}/cam_${DateTime.now().millisecondsSinceEpoch}.jpg');
      await c.takePicture();
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('已拍照：${file.path}')),
      );
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('拍照失败：$e')),
      );
    }
  }

  Future<void> _switchCamera() async {
    final cameras = _cameras;
    if (cameras == null || cameras.length < 2) return;
    await _controller?.dispose();
    setState(() { _cameraIndex = (_cameraIndex + 1) % cameras.length; });
    await _initCamera();
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final c = _controller;
    final cameras = _cameras;
    return Scaffold(
      backgroundColor: Colors.black,
      body: SafeArea(
        child: Column(
          children: [
            Expanded(
              child: Container(
                color: Colors.black,
                child: _error != null
                    ? Center(child: Text('相机错误\n$_error', style: const TextStyle(color: Colors.redAccent)))
                    : (cameras == null)
                        ? const Center(child: CircularProgressIndicator())
                        : (c != null && c.value.isInitialized)
                            ? ClipRect(child: CameraPreview(c))
                            : const Center(child: CircularProgressIndicator()),
              ),
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text('相机数: ${cameras?.length ?? '-'} | 当前: $_cameraIndex',
                      style: const TextStyle(color: Colors.white70, fontSize: 13)),
                  Text(_ready ? '已就绪' : '加载中',
                      style: TextStyle(color: _ready ? Colors.greenAccent : Colors.white54, fontSize: 13)),
                ],
              ),
            ),
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 20),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  IconButton(
                    icon: const Icon(Icons.flip_camera_android, color: Colors.white, size: 32),
                    onPressed: _switchCamera,
                    tooltip: '切换镜头',
                  ),
                  const SizedBox(width: 32),
                  GestureDetector(
                    onTap: _takePicture,
                    child: Container(
                      width: 72, height: 72,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        border: Border.all(color: Colors.white, width: 4),
                      ),
                      child: const Center(
                        child: Icon(Icons.camera_alt, color: Colors.white, size: 30),
                      ),
                    ),
                  ),
                  const SizedBox(width: 80),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
