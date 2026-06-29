import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';
import 'package:analyzer/source/source_range.dart';
import 'package:custom_lint_builder/custom_lint_builder.dart';

class FontFamilyMustUseWFont extends DartLintRule {
  FontFamilyMustUseWFont() : super(code: _code);

  static const _code = LintCode(
    name: 'font_family_must_use_wfont',
    problemMessage: 'If fontFamily is defined, Wfont must be used.',
    errorSeverity: error.ErrorSeverity.WARNING,
  );
  @override
  void run(CustomLintResolver resolver, ErrorReporter reporter,
      CustomLintContext context) {
    context.registry
        .addInstanceCreationExpression((InstanceCreationExpression node) {
      // Check if plover package exist
      if (!context.pubspec.dependencies.containsKey('plover')) {
        return;
      }
      // Check If fontFamily is defined
      bool hasFontFamily = false;
      for (var arg
          in node.argumentList.arguments.whereType<NamedExpression>()) {
        if (arg.name.label.name == 'fontFamily') {
          hasFontFamily = true;
          break;
        }
      }

      if (hasFontFamily) {
        // Check if the TextStyle is defined within the WFont class
        bool isWithinWfontClass = false;
        AstNode? parent = node.parent;
        while (parent != null) {
          if (parent is ClassDeclaration && parent.name.lexeme == 'WFont') {
            isWithinWfontClass = true;
            break;
          }
          parent = parent.parent;
        }
        if (isWithinWfontClass) {
          // Ignore TextStyle instances within the WFont class
          return;
        }

        // Check if fontFamily is using WFont
        bool isUsingWFont = false;
        for (var arg
            in node.argumentList.arguments.whereType<NamedExpression>()) {
          if (arg.name.label.name == 'fontFamily' &&
              arg.expression.toString().contains('WFont')) {
            isUsingWFont = true;
            break;
          }
        }

        // If TextStyle with fontFamily is not using WFont, report a lint error
        if (!isUsingWFont) {
          reporter.atNode(node, _code);
        }
      }
    });
  }

  @override
  List<Fix> getFixes() => [_UseWFontFix()];
}

class _UseWFontFix extends DartFix {
  @override
  void run(
    CustomLintResolver resolver,
    ChangeReporter reporter,
    CustomLintContext context,
    error.AnalysisError analysisError,
    List<error.AnalysisError> others,
  ) {
    context.registry.addInstanceCreationExpression((node) {
      if (!analysisError.sourceRange.intersects(node.sourceRange)) return;

      final changeBuilder = reporter.createChangeBuilder(
        message: 'Use WFont for fontFamily',
        priority: 10,
      );

      changeBuilder.addDartFileEdit((builder) {
        for (var arg
            in node.argumentList.arguments.whereType<NamedExpression>()) {
          if (arg.name.label.name == 'fontFamily') {
            final expression = arg.expression;
            builder.addSimpleReplacement(
                SourceRange(expression.offset, expression.length),
                'WFont.defaultFont().first');
          }
        }
      });
    });
  }
}
