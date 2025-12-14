import 'package:flutter/material.dart';

class DetectionOverlay extends StatelessWidget {
  final List<DetectionResult> detections;
  final Size videoSize;

  const DetectionOverlay({
    super.key,
    required this.detections,
    required this.videoSize,
  });

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: detections.map((detection) => _buildDetectionBox(detection)).toList(),
    );
  }

  Widget _buildDetectionBox(DetectionResult detection) {
    final left = (detection.boundingBox.left / videoSize.width);
    final top = (detection.boundingBox.top / videoSize.height);
    final width = (detection.boundingBox.width / videoSize.width);
    final height = (detection.boundingBox.height / videoSize.height);

    Color boxColor = _getDetectionColor(detection.type);
    
    return Positioned(
      left: left,
      top: top,
      width: width,
      height: height,
      child: Container(
        decoration: BoxDecoration(
          border: Border.all(color: boxColor, width: 2),
        ),
        child: Align(
          alignment: Alignment.topLeft,
            child: Container(
            color: boxColor.withValues(alpha: 0.7),
            padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
            child: Text(
              '${detection.type}: ${(detection.confidence * 100).toStringAsFixed(1)}%',
              style: const TextStyle(
                color: Colors.white,
                fontSize: 12,
                fontWeight: FontWeight.bold,
              ),
            ),
          ),
        ),
      ),
    );
  }

  Color _getDetectionColor(String type) {
    switch (type.toLowerCase()) {
      case 'motion':
        return Colors.blue;
      case 'intrusion':
        return Colors.red;
      case 'fire':
        return Colors.orange;
      case 'gas_leak':
        return Colors.purple;
      case 'equipment':
        return Colors.green;
      default:
        return Colors.yellow;
    }
  }
}

class DetectionResult {
  final String type;
  final Rect boundingBox;
  final double confidence;

  DetectionResult({
    required this.type,
    required this.boundingBox,
    required this.confidence,
  });
}

// extension on Rect {
//   double get left => this.left;
//   double get top => this.top;
//   double get width => this.width;
//   double get height => this.height;
// }