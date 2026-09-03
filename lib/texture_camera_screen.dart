import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// M2a: 自研 CameraX → Flutter Texture 直通预览
/// 验证 texture 管道，不含调色（M2b 插入 C++ LUT）
class TextureCameraScreen extends StatefulWidget {
  const TextureCameraScreen({super.key});

  @override
  State<TextureCameraScreen> createState() => _TextureCameraScreenState();
}

class _TextureCameraScreenState extends State<TextureCameraScreen> {
  static const _channel = MethodChannel('com.illusory.camengine/engine');

  int? _textureId;
  String? _error;
  bool _starting = false;

  @override
  void initState() {
    super.initState();
    _start();
  }

  Future<void> _start() async {
    if (_starting) return;
    setState(() { _starting = true; _error = null; });
    try {
      final id = await _channel.invokeMethod<int>('startCameraTexture');
      if (!mounted) return;
      setState(() { _textureId = id; _starting = false; });
    } catch (e) {
      if (!mounted) return;
      setState(() { _error = e.toString(); _starting = false; });
    }
  }

  Future<void> _stop() async {
    try {
      await _channel.invokeMethod('stopCameraTexture');
    } catch (_) {}
    if (mounted) setState(() => _textureId = null);
  }

  @override
  void dispose() {
    _stop();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.black,
      appBar: AppBar(
        title: const Text('M2a 纹理直通预览'),
        backgroundColor: const Color(0xFF1a1a1a),
      ),
      body: Column(
        children: [
          Expanded(
            child: Center(
              child: _error != null
                  ? Padding(
                      padding: const EdgeInsets.all(16),
                      child: Text('错误\n$_error',
                          style: const TextStyle(color: Colors.redAccent),
                          textAlign: TextAlign.center),
                    )
                  : _textureId != null
                      ? AspectRatio(
                          aspectRatio: 3 / 4,
                          child: Texture(textureId: _textureId!),
                        )
                      : const CircularProgressIndicator(),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(16),
            child: Text(
              _textureId != null ? 'textureId=$_textureId 运行中' : '未启动',
              style: const TextStyle(color: Colors.white54, fontSize: 12),
            ),
          ),
        ],
      ),
    );
  }
}
