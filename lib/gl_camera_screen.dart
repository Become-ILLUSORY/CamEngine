import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// M2b: 实时滤镜相机
/// 相机帧经 C++ GL 调色后显示，滑杆实时改变预览（所见即所拍）
class GlCameraScreen extends StatefulWidget {
  const GlCameraScreen({super.key});

  @override
  State<GlCameraScreen> createState() => _GlCameraScreenState();
}

class _GlCameraScreenState extends State<GlCameraScreen> {
  static const _channel = MethodChannel('com.illusory.camengine/engine');

  int? _textureId;
  String? _error;
  bool _starting = false;

  double _saturation = 1.4;
  double _contrast = 1.15;
  double _exposure = 1.1;
  double _temperature = 0.15;
  double _strength = 0.8;

  @override
  void initState() {
    super.initState();
    _start();
  }

  Future<void> _start() async {
    if (_starting) return;
    setState(() { _starting = true; _error = null; });
    try {
      final id = await _channel.invokeMethod<int>('startGlCamera');
      if (!mounted) return;
      setState(() { _textureId = id; _starting = false; });
    } catch (e) {
      if (!mounted) return;
      setState(() { _error = e.toString(); _starting = false; });
    }
  }

  Future<void> _stop() async {
    try {
      await _channel.invokeMethod('stopGlCamera');
    } catch (_) {}
  }

  void _pushParams() {
    _channel.invokeMethod('setGradeParams', [
      _saturation, _contrast, _exposure, _temperature, _strength,
    ]);
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
        title: const Text('M2b 实时滤镜相机'),
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
          _buildPanel(),
        ],
      ),
    );
  }

  Widget _buildPanel() {
    Widget slider(String label, double value, double min, double max,
        ValueChanged<double> onChanged, String Function(double) fmt) {
      return Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 1),
        child: Row(
          children: [
            SizedBox(width: 64, child: Text(label, style: const TextStyle(color: Colors.white70, fontSize: 12))),
            Expanded(
              child: Slider(
                value: value, min: min, max: max,
                onChanged: (v) { setState(() => onChanged(v)); _pushParams(); },
              ),
            ),
            SizedBox(width: 44, child: Text(fmt(value),
                style: const TextStyle(color: Colors.white70, fontSize: 12), textAlign: TextAlign.right)),
          ],
        ),
      );
    }

    return Container(
      color: const Color(0xFF1a1a1a),
      padding: const EdgeInsets.only(bottom: 12),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          slider('饱和度', _saturation, 0.0, 2.0, (v) => _saturation = v, (v) => '${(v * 100).round()}%'),
          slider('对比度', _contrast, 0.0, 2.0, (v) => _contrast = v, (v) => '${(v * 100).round()}%'),
          slider('曝光', _exposure, 0.25, 2.0, (v) => _exposure = v, (v) => v.toStringAsFixed(1)),
          slider('色温', _temperature, -1.0, 1.0, (v) => _temperature = v, (v) => v == 0 ? '标准' : (v > 0 ? '暖' : '冷')),
          slider('强度', _strength, 0.0, 1.0, (v) => _strength = v, (v) => '${(v * 100).round()}%'),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              TextButton.icon(
                onPressed: () {
                  setState(() {
                    _saturation = 1.4; _contrast = 1.15; _exposure = 1.1;
                    _temperature = 0.15; _strength = 0.8;
                  });
                  _pushParams();
                },
                icon: const Icon(Icons.refresh),
                label: const Text('胶片预设'),
              ),
              TextButton.icon(
                onPressed: () {
                  setState(() {
                    _saturation = 1.0; _contrast = 1.0; _exposure = 1.0;
                    _temperature = 0.0; _strength = 0.0;
                  });
                  _pushParams();
                },
                icon: const Icon(Icons.circle_outlined),
                label: const Text('原图'),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
