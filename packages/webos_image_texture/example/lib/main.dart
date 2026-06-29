import 'package:flutter/material.dart';
import 'dart:io';

import 'package:webos_image_texture/image_texture.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  FlutterError.onError = (details) {
    FlutterError.presentError(details);
    debugPrint(
        'error Flutter : ${details.exceptionAsString()}, stackTrace: ${details.stack}');
  };

  ImageTextureCache cache = ImageTextureCache();
  cache.maximumSizeBytes = 20 * 1024 * 1024;
  cache.clear();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    List<ImageTexture> listImageTexture = <ImageTexture>[
      ImageTexture.network(
        "https://influencermarketinghub.com/wp-content/uploads/2019/06/Animated-GIF-with-Background.gif",
        loadingBuilder: (
          BuildContext context,
          Widget child,
          ImageChunkEvent? loadingProgress,
        ) {
          return child;
        },
      ),
      ImageTexture.network(
        "https://apng.onevcat.com/assets/elephant.png",
      ),
      ImageTexture.network(
        "https://mathiasbynens.be/demo/animated-webp-supported.webp",
      ),
      ImageTexture.network(
        "https://www.seekpng.com/png/detail/7-70033_pnggrad16rgba-rgba-png.png",
        cacheHeight: 300,
      ),
      ImageTexture.network(
        "https://picsum.photos/200/300",
        loadingBuilder: (
          BuildContext context,
          Widget child,
          ImageChunkEvent? loadingProgress,
        ) {
          return child;
        },
      ),
      ImageTexture.network(
        "https://upload.wikimedia.org/wikipedia/commons/d/d2/Ghostscript_tiger_%28original_background%29.svg",
        cacheHeight: 300,
      ),
      ImageTexture.network(
        "https://www.gstatic.com/webp/gallery/4.sm.webp",
      ),
      ImageTexture.asset("assets/test3_linear_ldr.astc"),
      ImageTexture.asset("assets/ETCMipMap.ktx", matchTextDirection: true),
      ImageTexture.asset("assets/ball.pkm"),
      ImageTexture.file(
        File("/usr/palm/applications/com.webos.app.lgchannels/icon.png"),
      ),
    ];

    return MaterialApp(
      home: Scaffold(
        appBar: null,
        body: ListView.separated(
          itemCount: listImageTexture.length,
          itemBuilder: (BuildContext context, int index) {
            return SizedBox(
              height: 300,
              child: listImageTexture[index],
            );
          },
          separatorBuilder: (BuildContext context, int index) {
            return const Divider();
          },
        ),
      ),
    );
  }
}
