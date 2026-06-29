import 'package:flutter/material.dart';
import 'package:video_player/video_player.dart';

class CustomVideoController {
  // Video Controller
  late VideoPlayerController videoController;

  // Video Url
  final String videoUrl;

  // true : Mute
  final bool muteSound;

  // true : Looping
  final bool isLooping;

  // true : Auto Play
  final bool autoPlay;

  CustomVideoController({
    required this.videoUrl,
    this.muteSound = false,
    this.isLooping = false,
    this.autoPlay = false,
  }) {
    // For Preview Thumbnail
    videoStateNotifier = ValueNotifier(false);

    // Auto Play
    initVideoController().whenComplete(() {
      if (autoPlay) {
        playVideo();
      }
    });
  }

  // Video State Notifier
  late ValueNotifier<bool> videoStateNotifier;

  // getter : VideoPlayerController.value
  VideoPlayerValue get getVideoValue => videoController.value;

  // initVideoController : VideoPlayerController.initialize()
  // When videoStateNotifier is true, the thumbnail disappears and the video plays.
  Future<void> initVideoController() async {
    if (videoUrl.startsWith("http") == true) {
      videoController = VideoPlayerController.networkUrl(Uri.parse(videoUrl));
    } else {
      videoController = VideoPlayerController.asset(videoUrl);
    }

    await videoController.setLooping(isLooping);

    await videoController.initialize();

    videoStateNotifier.value = getVideoValue.isInitialized;
  }

  // playVideo : VideoPlayerController.play()
  Future<void> playVideo() async {
    if (!(getVideoValue.isPlaying)) {
      if (getVideoValue.position == Duration.zero) {
        await videoController.play();
      } else {
        await seekTo(getVideoValue.position);
        await videoController.play();
      }
    }
  }

  // pauseVideo : VideoPlayerController.pause()
  Future<void> pauseVideo() async {
    await videoController.pause();
  }

  // seekTo : VideoPlayerController.seekTo()
  Future<void> seekTo(Duration duration) async {
    await videoController.seekTo(duration);
  }

  // dispose : VideoPlayerController.dispose()
  Future<void> dispose() async {
    await videoController.dispose();
    videoStateNotifier.dispose();
  }

  // Only Support in Full Mode
  // For Timeline
  void addListener(Function() listener) {
    videoController.addListener(listener);
  }

  void removeListener(Function() listener) {
    videoController.removeListener(listener);
  }
}
