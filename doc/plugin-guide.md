# Build an Existing Flutter-webOS Plugin Example

 1. Clone the SDK Repository

	Clone the plugins repository:

	`git clone https://github.com/lg-flutter-webos/plugins.git`

2. Navigate to the Plugin Example

	`cd packages/gamepads/example`

 3. Build the Example

	`flutter-webos build webos $BUILD_MODE`
	
	- BUILD_MODE
		-  `--release`: Optimized for production (AOT, no VM service).
		- `--profile`: Performance analysis (AOT, VM service enabled).
		- `--debug`: Development mode (JIT, asserts, VM service).

 4. Run on Device

	Make sure a custom device is configured.
	
	`flutter-webos run -d tv`
	

# Using plugin dependency

 1. Add Plugin to `pubspec.yaml`

```
dependencies:
  audioplayers_webos:
    git:
      url: https://github.com/lg-flutter-webos/plugins.git
      path: packages/audioplayers
      ref: main

```

> Make sure `flutter-webos` is the active Flutter binary.

 2. Run `flutter-webos pub get`

	`flutter-webos pub get`

 3. Import and Use the Plugin

	In your Dart code example :
	
	`import 'package:audioplayers_webos/audioplayers_webos.dart';`
	
	Use the plugin as you would normally in Flutter.

# Create Flutter-webOS plugin

1. Run the Plugin Template Generator

	`flutter-webos create --platforms webos --template plugin my_plugin`

2. Navigate to the Example App

	`cd my_plugin/example`

3. Build the Example App

	`flutter-webos build webos --debug`

	This allows you to test your plugin within the generated example app.
	- BUILD_MODE
		- --debug : For development. No optimizations (JIT), includes asserts and VM service.
		- --release : For production. Fully optimized (AOT), no debugging tools.
		- --profile : or performance testing. Some optimizations (AOT), includes VM service.
