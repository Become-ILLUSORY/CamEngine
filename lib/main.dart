import 'dart:io';
import 'package:flutter/material.dart';
import 'package:camera/camera.dart';
import 'package:path_provider/path_provider.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final cameras = await availableCameras();
  runApp(CamEngineApp(cameras: cameras));
}

class CamEngineApp extends StatelessWidget {
  final List<CameraDescription> cameras;
  const CamEngineApp({super.key, required this.cameras});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'CamEngine',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorSchemeSeed: Colors.blueGrey,
        useMaterial3: true,
        brightness: Brightness.dark,
      ),
      home: CameraScreen(cameras: cameras),
    );
  }
}

class CameraScreen extends StatefulWidget {
  final List<CameraDescription> cameras;
  const CameraScreen({super.key, required this.cameras});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  CameraController? _controller;
  int _cameraIndex = 0;
  bool _ready = false;
  String? _error;

  CameraDescription get _current => widget.cameras[_cameraIndex];

  @override
  void initState() {
    super.initState();
    _initCamera();
  }

  Future<void> _initCamera() async {
    setState(() { _ready = false; _error = null; });
    final c = CameraController(
      _current,
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
      setState(() { _error = e.toString(); });
    }
  }

  Future<void> _takePicture() async {
    final c = _controller;
    if (c == null || !c.value.isInitialized) return;
    try {
      final dir = await getApplicationDocumentsDirectory();
      final file = File('${dir.path}/cam_${DateTime.now().millisecondsSinceEpoch}.jpg');
      await c.takePicture();
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('已拍照：$file')),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('拍照失败：$e')),
      );
    }
  }

  Future<void> _switchCamera() async {
    if (widget.cameras.length < 2) return;
    await _controller?.dispose();
    setState(() {
      _cameraIndex = (_cameraIndex + 1) % widget.cameras.length;
    });
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
    return Scaffold(
      backgroundColor: Colors.black,
      body: SafeArea(
        child: Column(
          children: [
            // 相机预览
            Expanded(
              child: Container(
                color: Colors.black,
                child: _error != null
                    ? Center(child: Text('相机错误\n$_error', style: const TextStyle(color: Colors.redAccent)))
                    : (c != null && c.value.isInitialized)
                        ? ClipRect(child: CameraPreview(c))
                        : const Center(child: CircularProgressIndicator()),
              ),
            ),
            // 顶部信息条
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text('相机数: ${widget.cameras.length} | 当前: $_cameraIndex',
                      style: const TextStyle(color: Colors.white70, fontSize: 13)),
                  Text(_ready ? '已就绪' : '加载中',
                      style: TextStyle(color: _ready ? Colors.greenAccent : Colors.white54, fontSize: 13)),
                ],
              ),
            ),
            // 底部工具栏
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
                  const SizedBox(width: 80), // 平衡左侧图标
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
