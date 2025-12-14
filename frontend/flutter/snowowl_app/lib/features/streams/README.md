# Video Streams Feature

This feature provides video stream management functionality for the SnowOwl application. It allows users to:

1. View and manage video streams
2. Enable/disable various detection algorithms on streams
3. Monitor stream status and health

## Structure

- `domain/` - Contains data models for video streams and detection configurations
- `services/` - Contains API services for communicating with the backend
- `application/` - Contains Riverpod providers for state management
- `view/` - Contains UI components for displaying streams and stream viewers
- `widgets/` - Contains reusable UI components

## Key Components

### VideoStream Model
Represents a video stream with properties like:
- ID and name
- URL and protocol (RTMP, RTSP, HLS, etc.)
- Status (online, offline, degraded)
- Location and tags
- Detection configuration

### Stream Detection Configuration
Controls which detection algorithms are enabled for a stream:
- Motion detection
- Intrusion detection
- Fire detection
- Gas leak detection
- Equipment monitoring

## Usage

1. Navigate to the "Streams" section in the main navigation
2. View available video streams in a grid layout
3. Click "View Stream" to open the stream viewer
4. In the stream viewer, enable/disable detection algorithms using the filter chips
5. The video will be processed with the selected detection algorithms

## Backend Integration

The stream service currently uses mock data. To integrate with a real backend:

1. Update the `baseUrl` in `StreamApiService`
2. Implement real API endpoints for:
   - Fetching streams
   - Adding/updating streams
   - Deleting streams
   - Updating detection configurations
3. Handle authentication if required

## Future Improvements

- Add stream recording functionality
- Implement real-time detection result overlays
- Add support for more stream protocols
- Include stream analytics and statistics
- Add stream sharing capabilities