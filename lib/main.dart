import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() {
  runApp(const CamEngineApp());
}

class CamEngineApp extends StatelessWidget {
  const CamEngineApp({super.key});

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
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  static const _channel = MethodChannel('com.illusory.camengine/engine');
  String _engineVersion = '...';
  String _status = '加载中...';

  @override
  void initState() {
    super.initState();
    _loadEngineInfo();
  }

  Future<void> _loadEngineInfo() async {
    try {
      final version = await _channel.invokeMethod<String>('getEngineVersion');
      setState(() {
        _engineVersion = version ?? 'unknown';
        _status = '引擎就绪';
      });
    } on MissingPluginException {
      setState(() {
        _status = '原生插件未加载';
      });
    } catch (e) {
      setState(() {
        _status = '错误: $e';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('CamEngine')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.camera_alt_outlined, size: 64, color: Colors.white54),
            const SizedBox(height: 24),
            Text('CamEngine', style: Theme.of(context).textTheme.headlineMedium),
            const SizedBox(height: 8),
            Text('引擎版本: $_engineVersion',
                style: Theme.of(context).textTheme.bodyLarge),
            const SizedBox(height: 16),
            Text(_status, style: Theme.of(context).textTheme.bodyMedium),
            const SizedBox(height: 32),
            FilledButton.icon(
              onPressed: _loadEngineInfo,
              icon: const Icon(Icons.refresh),
              label: const Text('刷新'),
            ),
          ],
        ),
      ),
    );
  }
}