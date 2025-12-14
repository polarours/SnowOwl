import 'dart:convert';
import 'package:http/http.dart' as http;

class DetectionService {
  final String baseUrl;

  DetectionService({required this.baseUrl});

  Future<bool> setDetectionEnabled(String detectionType, bool enabled) async {
    try {
      final url = Uri.parse('$baseUrl/api/v1/detection/control');
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'type': detectionType,
          'enabled': enabled,
        }),
      );

      if (response.statusCode == 200) {
        return true;
      } else {
        // print('Failed to set detection enabled: ${response.statusCode}');
        return false;
      }
    } catch (e) {
      // print('Error setting detection enabled: $e');
      return false;
    }
  }
  
  Future<bool> getDetectionStatus(String detectionType) async {
    try {
      final url = Uri.parse('$baseUrl/api/v1/detection/status?type=$detectionType');
      final response = await http.get(url);
      
      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        return data['enabled'] as bool? ?? false;
      } else {
        // print('Failed to get detection status: ${response.statusCode}');
        return false;
      }
    } catch (e) {
      // print('Error getting detection status: $e');
      return false;
    }
  }
}