import 'package:flutter/material.dart';
import 'camera_screen.dart';
import 'grade_test.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
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

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('CamEngine')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.camera_alt_outlined, size: 64, color: Colors.white54),
            const SizedBox(height: 16),
            const Text('CamEngine 引擎', style: TextStyle(fontSize: 20, color: Colors.white70)),
            const SizedBox(height: 32),
            FilledButton.icon(
              onPressed: () {
                Navigator.push(context,
                    MaterialPageRoute(builder: (_) => const CameraScreen()));
              },
              icon: const Icon(Icons.videocam),
              label: const Text('相机预览 (P0.5)'),
              style: FilledButton.styleFrom(minimumSize: const Size(260, 52)),
            ),
            const SizedBox(height: 16),
            FilledButton.tonalIcon(
              onPressed: () {
                Navigator.push(context,
                    MaterialPageRoute(builder: (_) => const GradeTestScreen()));
              },
              icon: const Icon(Icons.auto_fix_high),
              label: const Text('实时调色验证 (M1)'),
              style: FilledButton.styleFrom(minimumSize: const Size(260, 52)),
            ),
          ],
        ),
      ),
    );
  }
}
