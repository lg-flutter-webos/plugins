import 'dart:ui';
import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'image_texture_platform_interface.dart';

export 'image_texture_platform_interface.dart' show DataSourceType;

/////////////////////////////////////////////////////////////////////
////                    ImageTextureCache                        ////
/////////////////////////////////////////////////////////////////////
int _maximumCacheSizeBytes = 20 * 1024 * 1024;
bool _clearCache = false;

class ImageTextureCache {
  int get maximumSizeBytes => _maximumCacheSizeBytes;
  set maximumSizeBytes(int bytes) {
    _maximumCacheSizeBytes = bytes;
  }

  void clear() {
    _clearCache = true;
    _lastImageTexturePlatform?.clearCache();
  }
}

/////////////////////////////////////////////////////////////////////
////                    ImageTexturePlatform                     ////
/////////////////////////////////////////////////////////////////////
ImageTexturePlatform? _lastImageTexturePlatform;

ImageTexturePlatform get _imageTexturePlatform {
  final ImageTexturePlatform currentInstance = ImageTexturePlatform.instance;
  if (_lastImageTexturePlatform != currentInstance) {
    currentInstance.init(_maximumCacheSizeBytes);
    _lastImageTexturePlatform = currentInstance;
    if (_clearCache) {
      _lastImageTexturePlatform?.clearCache();
    }
  }
  return currentInstance;
}

/////////////////////////////////////////////////////////////////////
////                        Controller                           ////
/////////////////////////////////////////////////////////////////////
class ImageTextureController {
  ImageTextureController.dummy()
      : dataSourceType = DataSourceType.memory,
        dataSource = null,
        dataBytes = null,
        cacheWidth = 0,
        cacheHeight = 0;

  ImageTextureController.asset(
    int widgetId,
    this.dataSource,
    this.cacheWidth,
    this.cacheHeight,
  )   : dataBytes = null,
        dataSourceType = DataSourceType.asset {
    _initialize(widgetId);
  }

  ImageTextureController.network(
    int widgetId,
    this.dataSource,
    this.cacheWidth,
    this.cacheHeight,
  )   : dataBytes = null,
        dataSourceType = DataSourceType.network {
    _initialize(widgetId);
  }

  ImageTextureController.file(
    int widgetId,
    this.dataSource,
    this.cacheWidth,
    this.cacheHeight,
  )   : dataBytes = null,
        dataSourceType = DataSourceType.file {
    _initialize(widgetId);
  }

  ImageTextureController.memory(
    int widgetId,
    this.dataBytes,
    this.cacheWidth,
    this.cacheHeight,
  )   : dataSource = null,
        dataSourceType = DataSourceType.memory {
    _initialize(widgetId);
  }

  static const int kUninitializedTextureId = -1;
  static Map<ImageTextureController, bool?> controllers = {};
  final DataSourceType dataSourceType;
  final String? dataSource;
  final Uint8List? dataBytes;
  final int cacheWidth;
  final int cacheHeight;

  int _textureId = kUninitializedTextureId;
  Size? _size;
  String? _errorMsg;
  bool _isError = false;
  bool _isInitialized = false;
  bool _isStarted = false;
  bool _isActivated = false;
  bool _isDisposed = false;
  bool _isPlaying = true;
  int _holders = 0;
  int _frameCnt = 0;
  final Map<int, ImageTextureState?> _widgetStates = {};

  _ImageAppLifeCycleObserver? _lifeCycleObserver;
  Completer<void>? _creatingCompleter;
  StreamSubscription<ImageEvent>? _eventSubscription;

  ///////////////////////////////
  int get textureId => _textureId;
  bool get isInitialized => _isInitialized;
  int get frame => _frameCnt;
  String? get errorMsg => _errorMsg;
  bool get isError => _isError;
  bool get isPlaying => _isPlaying;
  Size? get size => _size;

  Future<Size?> getSize() async {
    if (_size != null) {
      return _size;
    }

    var completed = _creatingCompleter?.isCompleted;
    if (completed != null && completed == false) {
      await _creatingCompleter!.future;
    }
    var bounds = await _imageTexturePlatform.getSize(_textureId);
    int width = 0;
    int height = 0;
    if (bounds != null) {
      width = bounds[0];
      height = bounds[1];
    }
    return Size(width.toDouble(), height.toDouble());
  }

  Future<void> play() async {
    _isPlaying = true;
    await _applyPlay();
  }

  Future<void> stop() async {
    _isPlaying = false;
    await _applyStop();
  }

  ImageTextureController? find(
    DataSourceType type,
    int widgetId,
    String src,
    int cacheWidth,
    int cacheHeight,
  ) {
    for (var c in controllers.keys) {
      if (c.dataSourceType != type) {
        continue;
      }
      if (c.dataSource == src &&
          c.cacheWidth == cacheWidth &&
          c.cacheHeight == cacheHeight) {
        c._widgetStates[widgetId] = null;
        return c;
      }
    }
    return null;
  }

  ImageTextureController? findMemory(
    int widgetId,
    Uint8List bytes,
    int cacheWidth,
    int cacheHeight,
  ) {
    for (var c in controllers.keys) {
      if (c.dataBytes == null) {
        continue;
      }
      // Not used listEquals for performance.
      if (c.dataBytes == bytes &&
          c.cacheWidth == cacheWidth &&
          c.cacheHeight == cacheHeight) {
        c._widgetStates[widgetId] = null;
        return c;
      }
    }
    return null;
  }

  void insert(int widgetId,
              ImageTextureState? state) async {
    if (_isDisposed) {
      _initialize(widgetId);
    }
    _widgetStates[widgetId] = state;

    if (!_isStarted) {
      _isStarted = true;
      _start();
    }
  }

  void remove(int widgetId) {
    bool update = false;
    if (_widgetStates.containsKey(widgetId)) {
      _widgetStates.remove(widgetId);
      update = true;
    }
    if (_widgetStates.isEmpty && _holders == 0) {
      update = false;
      controllers.remove(this);
      _dispose();
    }
    if (update) {
      visibilityChanged();
    }
  }

  void insertHolder() {
    _holders++;
  }

  void removeHolder() {
    if (_holders > 0) {
      _holders--;
    }

    if (_widgetStates.isEmpty && _holders == 0) {
      controllers.remove(this);
      _dispose();
    }
  }

  void visibilityChanged() {
    var visibility = _visibility();
    var prevVisibility = controllers[this];
    if (visibility == null ||
        visibility == prevVisibility) {
      return;
    }
    controllers[this] = visibility;
    if (visibility) {
      _activate();
    } else {
      _deactivate();
    }
  }

  ////////////////////////////////////////////////////////
  // Private Methods
  ////////////////////////////////////////////////////////
  bool get _isDisposedOrNotInitialized => _isDisposed || !_isInitialized;

  void _initialize(int widgetId) {
    _textureId = kUninitializedTextureId;
    _size = null;
    _errorMsg = null;
    _isError = false;
    _isInitialized = false;
    _isStarted = false;
    _isActivated = false;
    _isDisposed = false;
    _isPlaying = true;
    _holders = 0;
    _frameCnt = 0;
    _widgetStates.clear();

    controllers[this] = null;
    _widgetStates[widgetId] = null;

    _lifeCycleObserver = _ImageAppLifeCycleObserver(this);
    _lifeCycleObserver?.initialize();
    _creatingCompleter = Completer<void>();
  }

  Future<void> _start() async {
    _isActivated = true;

    late DataSource dataSourceDescription;
    switch (dataSourceType) {
      case DataSourceType.asset:
        dataSourceDescription = DataSource(
          sourceType: DataSourceType.asset,
          cacheWidth: cacheWidth,
          cacheHeight: cacheHeight,
          uri: dataSource,
        );
        break;
      case DataSourceType.network:
        dataSourceDescription = DataSource(
          sourceType: DataSourceType.network,
          cacheWidth: cacheWidth,
          cacheHeight: cacheHeight,
          uri: dataSource,
        );
        break;
      case DataSourceType.file:
        dataSourceDescription = DataSource(
          sourceType: DataSourceType.file,
          cacheWidth: cacheWidth,
          cacheHeight: cacheHeight,
          uri: dataSource,
        );
        break;
      case DataSourceType.memory:
        dataSourceDescription = DataSource(
          sourceType: DataSourceType.memory,
          cacheWidth: cacheWidth,
          cacheHeight: cacheHeight,
          bytes: dataBytes,
        );
        break;
      case DataSourceType.unknown:
        break;
    }

    _textureId = (await _imageTexturePlatform.create(dataSourceDescription)) ??
        kUninitializedTextureId;
    _creatingCompleter!.complete(null);

    _eventSubscription =
        _imageTexturePlatform.imageEventsFor(_textureId).listen(_eventListener);
  }

  Future<void> _activate() async {
    if (_isDisposed || _isActivated) {
      return;
    }
    _isActivated = true;
    await _imageTexturePlatform.activate(_textureId);
  }

  Future<void> _deactivate() async {
    if (_isDisposed || (!_isError && !_isInitialized)) {
      return;
    }

    _errorMsg = null;
    _isError = false;
    _isInitialized = false;
    _isActivated = false;
    await _imageTexturePlatform.deactivate(_textureId);
  }

  Future<void> _dispose() async {
    if (_isDisposed) {
      return;
    }

    var creatingCompleter = _creatingCompleter;
    var eventSubscription =  _eventSubscription;
    var lifeCycleObserver = _lifeCycleObserver;
    int textureId = _textureId;
    _isDisposed = true;

    var completed = creatingCompleter?.isCompleted;
    if (completed != null && completed == false) {
      await creatingCompleter!.future;
    }
    await eventSubscription?.cancel();
    if (textureId != kUninitializedTextureId) {
      await _imageTexturePlatform.dispose(textureId);
    }
    lifeCycleObserver?.dispose();
  }

  bool? _visibility() {
    bool? visibility;
    for (var v in _widgetStates.values) {
      var value = v?.visibility();
      if (value == null) {
        continue;
      }
      if (value == true) {
        visibility = true;
        break;
      }
      visibility = false;
    }
    return visibility;
  }

  void _eventListener(ImageEvent event) {
    if (_isDisposed) {
      return;
    }
    switch (event.eventType) {
      case ImageEventType.initialized:
        _size = event.size;
        _frameCnt = 0;
        _isInitialized = true;
        _isError = false;
        for (var v in _widgetStates.values) {
          v?.redraw();
        }

        if (controllers[this] == false) {
          _deactivate();
        } else if (_isPlaying) {
          _applyPlay();
        }
        break;

      case ImageEventType.error:
        _errorMsg = event.errorMsg;
        _isError = true;
        for (var v in _widgetStates.values) {
          v?.redraw();
        }
        if (controllers[this] == false) {
          _deactivate();
        }
        break;

      case ImageEventType.frame:
        _frameCnt++;
        for (var v in _widgetStates.values) {
          v?.redraw();
        }
        break;

      case ImageEventType.completed:
        _isPlaying = false;
        break;

      case ImageEventType.unknown:
      default:
        break;
    }
  }

  Future<void> _applyPlay() async {
    if (_isDisposedOrNotInitialized) {
      return;
    }
    await _imageTexturePlatform.play(_textureId);
  }

  Future<void> _applyStop() async {
    if (_isDisposedOrNotInitialized) {
      return;
    }
    await _imageTexturePlatform.stop(_textureId);
  }
}

/////////////////////////////////////////////////////////////////////
////                   LifeCycle Observer                        ////
/////////////////////////////////////////////////////////////////////
class _ImageAppLifeCycleObserver extends Object with WidgetsBindingObserver {
  _ImageAppLifeCycleObserver(this._controller);

  bool _wasPlayingBeforePause = false;
  final ImageTextureController _controller;

  void initialize() {
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused) {
      _wasPlayingBeforePause = _controller.isPlaying;
      _controller.stop();
    } else if (state == AppLifecycleState.resumed) {
      if (_wasPlayingBeforePause) {
        _controller.play();
      }
    }
  }

  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
  }
}

/////////////////////////////////////////////////////////////////////
////                        Widget                               ////
/////////////////////////////////////////////////////////////////////
class ImageTexture extends StatefulWidget {
  ImageTexture.asset(
    String src, {
    super.key,
    this.errorBuilder,
    this.frameBuilder,
    BoxFit? fit,
    this.alignment = Alignment.center,
    this.matchTextDirection = false,
    int? cacheWidth,
    int? cacheHeight,
    this.width,
    this.height,
    this.color,
    BlendMode? colorBlendMode,
    this.opacity,
    this.gaplessPlayback = false,
    this.semanticLabel,
    this.excludeFromSemantics = false,
  })  : loadingBuilder = null,
        fit = fit ?? BoxFit.scaleDown,
        colorBlendMode = colorBlendMode ?? BlendMode.srcIn,
        widgetId= uid++,
        super() {
    controller = ImageTextureController.dummy().find(
          DataSourceType.asset,
          widgetId,
          src,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        ) ??
        ImageTextureController.asset(
          widgetId,
          src,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        );
  }

  ImageTexture.network(
    String src, {
    super.key,
    this.errorBuilder,
    this.frameBuilder,
    this.loadingBuilder,
    BoxFit? fit,
    this.alignment = Alignment.center,
    this.matchTextDirection = false,
    int? cacheWidth,
    int? cacheHeight,
    this.width,
    this.height,
    this.color,
    BlendMode? colorBlendMode,
    this.opacity,
    this.gaplessPlayback = false,
    this.semanticLabel,
    this.excludeFromSemantics = false,
  })  : fit = fit ?? BoxFit.scaleDown,
        colorBlendMode = colorBlendMode ?? BlendMode.srcIn,
        widgetId = uid++,
        super() {
    // Apply encodeFull if the URL contains non-ASCII or special characters (e.g. Korean, spaces, etc.)
    final bool needEncoding =
        RegExp(r"[^A-Za-z0-9\-._~:/?#\[\]@!\$&'()*+,;=%]").hasMatch(src);
    final bool alreadyEncoded = src.contains('%');
    controller = ImageTextureController.dummy().find(
          DataSourceType.network,
          widgetId,
          (needEncoding && !alreadyEncoded) ? Uri.encodeFull(src) : src,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        ) ??
        ImageTextureController.network(
          widgetId,
          (needEncoding && !alreadyEncoded) ? Uri.encodeFull(src) : src,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        );
  }

  ImageTexture.file(
    File file, {
    super.key,
    this.errorBuilder,
    this.frameBuilder,
    BoxFit? fit,
    this.alignment = Alignment.center,
    this.matchTextDirection = false,
    int? cacheWidth,
    int? cacheHeight,
    this.width,
    this.height,
    this.color,
    BlendMode? colorBlendMode,
    this.opacity,
    this.gaplessPlayback = false,
    this.semanticLabel,
    this.excludeFromSemantics = false,
  })  : loadingBuilder = null,
        fit = fit ?? BoxFit.scaleDown,
        colorBlendMode = colorBlendMode ?? BlendMode.srcIn,
        widgetId = uid++,
        super() {
    controller = ImageTextureController.dummy().find(
          DataSourceType.file,
          widgetId,
          Uri.file(file.absolute.path).toString(),
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        ) ??
        ImageTextureController.file(
          widgetId,
          Uri.file(file.absolute.path).toString(),
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        );
  }

  ImageTexture.memory(
    Uint8List bytes, {
    super.key,
    this.errorBuilder,
    this.frameBuilder,
    BoxFit? fit,
    this.alignment = Alignment.center,
    this.matchTextDirection = false,
    int? cacheWidth,
    int? cacheHeight,
    this.width,
    this.height,
    this.color,
    BlendMode? colorBlendMode,
    this.opacity,
    this.gaplessPlayback = false,
    this.semanticLabel,
    this.excludeFromSemantics = false,
  })  : loadingBuilder = null,
        fit = fit ?? BoxFit.scaleDown,
        colorBlendMode = colorBlendMode ?? BlendMode.srcIn,
        widgetId = uid++,
        super() {
    controller = ImageTextureController.dummy().findMemory(
          widgetId,
          bytes,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        ) ??
        ImageTextureController.memory(
          widgetId,
          bytes,
          cacheWidth ?? 0,
          cacheHeight ?? 0,
        );
  }

  static int uid = 0;
  final int widgetId;
  final ImageErrorWidgetBuilder? errorBuilder;
  final ImageFrameBuilder? frameBuilder;
  final ImageLoadingBuilder? loadingBuilder;
  final BoxFit fit;
  final AlignmentGeometry alignment;
  final bool matchTextDirection;
  final double? width;
  final double? height;
  final Color? color;
  final BlendMode colorBlendMode;
  final Animation<double>? opacity;
  final bool gaplessPlayback;
  late final ImageTextureController controller;
  final String? semanticLabel;
  final bool excludeFromSemantics;

  Future<Size?> size() async {
    return await controller.getSize();
  }

  @override
  State<ImageTexture> createState() => ImageTextureState();
}

class ImageTextureState extends State<ImageTexture> {
  ImageTextureState();

  final GlobalKey _boxKey = GlobalKey();
  bool? _visibility;
  ImageTexture? _oldWidget;

  void redraw() {
    setState((){});
  }

  bool? visibility() {
    return _visibility;
  }

  @override
  void initState() {
    super.initState();

    widget.controller.insert(widget.widgetId, this);
    WidgetsBinding.instance.addPostFrameCallback(_afterFrameLayout);
  }

  @override
  void didUpdateWidget(ImageTexture oldWidget) {
    super.didUpdateWidget(oldWidget);

    if (widget.gaplessPlayback &&
        !widget.controller.isInitialized &&
        oldWidget.controller.isInitialized) {
      _resetHolder(oldWidget);
    } else {
      _resetHolder(null);
    }
    widget.controller.insert(widget.widgetId, this);
    oldWidget.controller.remove(oldWidget.widgetId);
  }

  void _resetHolder(ImageTexture? oldWidget) {
    oldWidget?.controller.insertHolder();
    _oldWidget?.controller.removeHolder();
    _oldWidget = oldWidget;
  }

  @override
  void dispose() {

    _resetHolder(null);
    widget.controller.remove(widget.widgetId);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (widget.opacity != null) {
      return Container(
        key: _boxKey,
        child: FadeTransition(
          opacity: widget.opacity!,
          child: _build(context)));
    }
    return Container(key: _boxKey, child: _build(context));
  }

  Widget _build(BuildContext context) {
    var controller = widget.controller;

    // errorBuilder
    if (controller.isError && controller.errorMsg != null) {
      String errorMsg = controller.errorMsg!;
      var dataSourceType = controller.dataSourceType;
      var dataSource = controller.dataSource;
      debugPrint('Controller OnError $dataSourceType $dataSource $errorMsg');

      if (widget.errorBuilder != null) {
        ImageErrorWidgetBuilder builder = widget.errorBuilder!;
        return Container(child: builder(context, errorMsg, null));
      }
    }

    Widget? child;
    if (controller.isInitialized == false) {
      if (_oldWidget != null) {
        child = _buildWidget(_oldWidget!);
      }
    } else {
      _resetHolder(null);
    }

    // loadingBuilder
    if (widget.loadingBuilder != null) {
      var builder = widget.loadingBuilder!;
      return builder(
          context,
          (child == null) ? _buildWidget(widget) : child,
          widget.controller.isInitialized
              ? null
              : const ImageChunkEvent(
                  cumulativeBytesLoaded: 0, expectedTotalBytes: null));
    }

    // frameBuilder
    if (widget.frameBuilder != null) {
      var builder = widget.frameBuilder!;
      return builder(
          context,
          _buildWidget(widget),
          widget.controller.isInitialized ? widget.controller.frame : null,
          widget.controller.isInitialized ? true : false);
    }

    return _buildWidget(widget);
  }

  Widget _buildWidget(ImageTexture img) {
    Widget imageWidget;
    if (!img.controller.isInitialized) {
      imageWidget = Container();
    } else if (widget.color == null) {
      imageWidget = _buildImage(img);
    } else {
      var color = widget.color!;
      var mode = widget.colorBlendMode;
      imageWidget = ColorFiltered(
          colorFilter: ColorFilter.mode(color, mode), child: _buildImage(img));
    }

    // Wrap with Semantics if semanticLabel is provided and excludeFromSemantics is false
    if (widget.semanticLabel != null && !widget.excludeFromSemantics) {
      imageWidget = Semantics(
        container: true,
        image: true,
        label: widget.semanticLabel!,
        child: imageWidget,
      );
    }
    return imageWidget;
  }

  Widget _buildImage(ImageTexture img) {
    Widget child;

    if (!img.controller.isInitialized) {
      child = Container();
    } else {
      final TextDirection currentDirection = Directionality.maybeOf(context) ?? TextDirection.ltr;
      final bool isRTL = currentDirection == TextDirection.rtl;
      int textureId = img.controller.textureId;
      Size size = img.controller.size!;

      if (img.controller.cacheWidth > 0) {
        size = Size(img.controller.cacheWidth.toDouble(), size.height);
      }
      if (img.controller.cacheHeight > 0) {
        size = Size(size.width, img.controller.cacheHeight.toDouble());
      }

      child = SizedBox(
        width: size.width,
        height: size.height,
        child: isRTL
            ? Transform(
                alignment: Alignment.center,
                transform: Matrix4.rotationY(math.pi),
                child: _imageTexturePlatform.buildView(textureId))
            : _imageTexturePlatform.buildView(textureId),
      );
    }

    return SizedBox(
      width: widget.width,
      height: widget.height,
      child: FittedBox(
        fit: widget.fit,
        alignment: widget.alignment,
        clipBehavior: Clip.hardEdge,
        child: child,
      ),
    );
  }

  void _afterFrameLayout(_) {
    var prevVisibility = _visibility;
    Rect? rect = _getCurrentRect();
    if (rect != null) {
      const double MARGIN = 0.1;
      FlutterView view = WidgetsBinding.instance.platformDispatcher.views.first;
      var windowWidth = view.physicalSize.width;
      var windowHeight = view.physicalSize.height;
      double winX0 = -windowWidth * MARGIN;
      double winY0 = -windowHeight * MARGIN;
      double winX1 = windowWidth + windowWidth * MARGIN;
      double winY1 = windowHeight + windowHeight * MARGIN;
      double x = rect.left;
      double y = rect.top;
      double w = rect.width;
      double h = rect.height;
      bool visibility = true;
      if (x + w < winX0) {
        visibility = false;
      } else if (y + h < winY0) {
        visibility = false;
      } else if (x > winX1) {
        visibility = false;
      } else if (y > winY1) {
        visibility= false;
      }
      _visibility = visibility;
    } else if (_visibility == true) {
      _visibility = false;
    }

    if (prevVisibility != _visibility) {
      widget.controller.visibilityChanged();
    }
    WidgetsBinding.instance.addPostFrameCallback(_afterFrameLayout);
  }

  Rect? _getCurrentRect() {
    final box = _boxKey.currentContext?.findRenderObject() as RenderBox?;
    if (box == null) {
      return null;
    }
    final Offset offset = box.localToGlobal(Offset.zero);
    if (offset.dx.isNaN && offset.dy.isNaN) {
      return null;
    }

    final Size size = box.size;
    return offset & size;
  }
}
