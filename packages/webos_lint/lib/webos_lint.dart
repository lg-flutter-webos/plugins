import 'dart:convert';
import 'dart:io';

import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';
import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:webos_lint/expanded_Must_Be_In_Flex.dart';
import 'package:webos_lint/focus_root_scope.dart';
import 'package:webos_lint/font_family_must_use_wfont.dart';
import 'package:webos_lint/timer_disposal_lint.dart';
import 'wtheme_wrapping.dart';
import 'jsonaccess.dart';
import 'voicecontrol_add_removecontext.dart';
import 'CheckLocalizationsDelegates.dart';

// This is the entrypoint of our custom linter
PluginBase createPlugin() => _ExampleLinter();

/// A plugin class is used to list all the assists/lints defined by a plugin.
class _ExampleLinter extends PluginBase {
  /// We list all the custom warnings/infos/errors
  @override
  List<LintRule> getLintRules(CustomLintConfigs configs) => [
        // MyCustomLintCode(),
        AppInfoLintRule(code: appInfoLintCode),
        MaterialMustBeWrappedWithWTheme(),
        JsonAccessMustBeChecked(),
        AddContextMustHaveRemoveContext(),
        CheckLocalizationsDelegates(),
        ExpandedMustBeInFlex(),
        TimerDisposalLint(),
        FontFamilyMustUseWFont(),
        FocusRootScopeRestrictFocusableUsage(),
      ];
}

final appInfoLintCode = LintCode(
  name: 'appinfo_transparent_check',
  problemMessage: "'transparent' key is missing or set to false",
  errorSeverity: error.ErrorSeverity.WARNING,
);

class MyCustomLintCode extends DartLintRule {
  MyCustomLintCode() : super(code: _code);

  /// Metadata about the warning that will show-up in the IDE.
  /// This is used for `// ignore: code` and enabling/disabling the lint
  static const _code = LintCode(
    name: 'my_custom_lint_code',
    problemMessage: 'This is the description of our custom lint',
    errorSeverity: error.ErrorSeverity.WARNING,
  );

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    // Our lint will highlight all variable declarations with our custom warning.
    context.registry.addVariableDeclaration((node) {
      reporter.atNode(node, code);
    });
  }
}

class AppInfoLintRule extends LintRule {
  const AppInfoLintRule({required super.code});

  @override
  List<String> get filesToAnalyze => const ['webos/meta/appinfo.json'];

  @override
  Future<void> startUp(
    CustomLintResolver resolver,
    CustomLintContext context,
  ) async {
    // No additional startup logic needed for this rule.
  }

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) async {
    final file = File(resolver.path);
    if (!await file.exists()) {
      return;
    }

    final content = await file.readAsString();
    final jsonContent = jsonDecode(content);

    if (!context.pubspec.dependencies.containsKey('video_player_webos')) {
      return;
    }
    if (jsonContent is Map<String, dynamic>) {
      if (!jsonContent.containsKey('transparent') ||
          jsonContent['transparent'] == false) {
        reporter.atOffset(
          offset: 0,
          length: content.length,
          errorCode: code,
          arguments: ["'transparent' key is missing or set to false"],
        );
      }
    }
  }
}
