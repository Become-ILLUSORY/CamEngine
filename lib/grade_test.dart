import 'dart:ui' as ui;
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/painting.dart';

// M1: 实时调色验证
// 一张测试图 + 实时滑杆，GPU fragment shader 实时改色
class GradeTestScreen extends StatefulWidget {
  const GradeTestScreen({super.key});
  @override
  State<GradeTestScreen> createState() => _GradeTestScreenState();
}

class _GradeTestScreenState extends State<GradeTestScreen> {
  ui.Image? _testImage;
  ui.FragmentShader? _shader;
  bool _loading = true;
  String? _error;

  double _saturation = 1.0;
  double _contrast = 1.0;
  double _exposure = 1.0;
  double _temperature = 0.0;
  double _strength = 1.0;

  @override
  void initState() {
    super.initState();
    _init();
  }

  Future<void> _init() async {
    try {
      final img = await _makeTestImage();
      final program = await ui.FragmentProgram.fromAsset('shaders/grade.frag');
      if (!mounted) return;
      setState(() {
        _testImage = img;
        _shader = program.fragmentShader();
        _loading = false;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() { _error = e.toString(); _loading = false; });
    }
  }

  // 生成一张有色彩层次感的测试图
  Future<ui.Image> _makeTestImage() async {
    const w = 400, h = 400;
    final recorder = ui.PictureRecorder();
    final canvas = Canvas(recorder);
    // 渐变背景
    for (int x = 0; x < w; x++) {
      final t = x / w;
      canvas.drawRect(
        Rect.fromLTWH(x.toDouble(), 0, 1, h.toDouble()),
        Paint()
          ..shader = ui.Gradient.linear(
            Offset(x.toDouble(), 0),
            Offset(x.toDouble(), h.toDouble()),
            [HSLColor.fromAHSL(1, t * 360, 0.7, 0.5).toColor(),
             HSLColor.fromAHSL(1, t * 360, 0.9, 0.3).toColor()],
          ),
      );
    }
    // 灰阶带（用于看对比度/曝光）
    for (int x = 0; x < w; x += 20) {
      final g = (x / w) * 255;
      canvas.drawRect(
        Rect.fromLTWH(x.toDouble(), 0, 20, 60),
        Paint()..color = Color.fromARGB(255, g.toInt(), g.toInt(), g.toInt()),
      );
    }
    // 色块
    canvas.drawRect(const Rect.fromLTWH(0, 340, 100, 60), Paint()..color = Colors.red);
    canvas.drawRect(const Rect.fromLTWH(100, 340, 100, 60), Paint()..color = Colors.green);
    canvas.drawRect(const Rect.fromLTWH(200, 340, 100, 60), Paint()..color = Colors.blue);
    canvas.drawRect(const Rect.fromLTWH(300, 340, 100, 60), Paint()..color = Colors.white);

    final picture = recorder.endRecording();
    final image = await picture.toImage(w, h);
    picture.dispose();
    return image;
  }

  @override
  void dispose() {
    _testImage?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF111111),
      appBar: AppBar(
        title: const Text('M1 实时调色'),
        backgroundColor: const Color(0xFF1a1a1a),
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : _error != null
              ? Center(child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Text('shader 加载失败\n$_error', style: const TextStyle(color: Colors.redAccent)),
                ))
              : _buildBody(),
    );
  }

  Widget _buildBody() {
    return Column(
      children: [
        // 实时预览
        Expanded(
          child: Center(
            child: _ShaderPreview(
              image: _testImage!,
              makeShader: () {
                final s = _shader!;
                s.setFloat(0, 400); s.setFloat(1, 400); // uSize
                s.setFloat(2, _saturation);
                s.setFloat(3, _contrast);
                s.setFloat(4, _exposure);
                s.setFloat(5, _temperature);
                s.setFloat(6, _strength);
                return s;
              },
            ),
          ),
        ),
        // 调色面板
        _buildPanel(),
      ],
    );
  }

  Widget _buildPanel() {
    Widget slider(String label, double value, double min, double max, ValueChanged<double> onChanged,
        {String Function(double)? fmt}) {
      return Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 2),
        child: Row(
          children: [
            SizedBox(width: 76, child: Text(label, style: const TextStyle(color: Colors.white70, fontSize: 12))),
            Expanded(
              child: Slider(value: value, min: min, max: max, onChanged: (v) => setState(() => onChanged(v))),
            ),
            SizedBox(width: 48, child: Text(fmt?.call(value) ?? value.toStringAsFixed(2),
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
          slider('饱和度', _saturation, 0.0, 2.0, (v) => _saturation = v, fmt: (v) => '${(v * 100).round()}%'),
          slider('对比度', _contrast, 0.0, 2.0, (v) => _contrast = v, fmt: (v) => '${(v * 100).round()}%'),
          slider('曝光', _exposure, 0.25, 2.0, (v) => _exposure = v, fmt: (v) => v.toStringAsFixed(1)),
          slider('色温', _temperature, -1.0, 1.0, (v) => _temperature = v, fmt: (v) => v == 0 ? '标准' : (v > 0 ? '暖' : '冷')),
          slider('强度', _strength, 0.0, 1.0, (v) => _strength = v, fmt: (v) => '${(v * 100).round()}%'),
          const SizedBox(height: 4),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              TextButton.icon(
                onPressed: () => setState(() {
                  _saturation = 1; _contrast = 1; _exposure = 1;
                  _temperature = 0; _strength = 1;
                }),
                icon: const Icon(Icons.refresh),
                label: const Text('重置'),
              ),
              const SizedBox(width: 16),
              const Text('拖滑杆 → GPU实时调色', style: TextStyle(color: Colors.white38, fontSize: 12)),
            ],
          ),
        ],
      ),
    );
  }
}

// 用 FragmentShader 渲染测试图
class _ShaderPreview extends StatelessWidget {
  final ui.Image image;
  final ui.FragmentShader Function() makeShader;
  const _ShaderPreview({required this.image, required this.makeShader});

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size(image.width.toDouble(), image.height.toDouble()),
      painter: _ShaderPainter(image: image, makeShader: makeShader),
    );
  }
}

class _ShaderPainter extends CustomPainter {
  final ui.Image image;
  final ui.FragmentShader Function() makeShader;
  _ShaderPainter({required this.image, required this.makeShader}) : super(repaint: Listenable.merge([]));

  @override
  void paint(Canvas canvas, Size size) {
    final shader = makeShader();
    shader.setImageSampler(0, image);
    final rect = Rect.fromLTWH(0, 0, size.width, size.height);
    canvas.drawRect(rect, Paint()..shader = shader);
  }

  @override
  bool shouldRepaint(_ShaderPainter old) => true;
}
