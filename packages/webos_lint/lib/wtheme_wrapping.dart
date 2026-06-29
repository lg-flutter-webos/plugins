import 'package:analyzer/source/source_range.dart';
import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';

class MaterialMustBeWrappedWithWTheme extends DartLintRule {
  MaterialMustBeWrappedWithWTheme() : super(code: _code);

  static const _code = LintCode(
    name: 'material_must_be_wrapped_with_wtheme',
    problemMessage: 'MaterialApp widgets must be wrapped with WTheme.',
    errorSeverity: error.ErrorSeverity.WARNING,
  );

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    context.registry
        .addInstanceCreationExpression((InstanceCreationExpression node) {
      if (!context.pubspec.dependencies.containsKey('plover')) {
        return;
      }

      // Check if the instance being created is a Material widget
      if (node.constructorName.type.type?.getDisplayString() == 'MaterialApp') {
        // Initialize a flag to check if Material is wrapped with WTheme
        bool isWrappedWithWTheme = false;
        AstNode? parent = node.parent;

        // Traverse up the AST to check if any parent is a WTheme widget
        while (parent != null) {
          if (parent is InstanceCreationExpression &&
              parent.constructorName.type.type?.getDisplayString() ==
                  'WTheme') {
            isWrappedWithWTheme = true;
            break;
          }
          parent = parent.parent;
        }

        // If MaterialApp is not wrapped with WTheme, report a lint error
        if (!isWrappedWithWTheme) {
          reporter.atNode(
            node,
            _code,
            data: node,
          );
        }
      }
    });
  }

  @override
  List<Fix> getFixes() {
    return [_WThemeFix()];
  }
}

class _WThemeFix extends DartFix {
  @override
  void run(
    CustomLintResolver resolver,
    ChangeReporter reporter,
    CustomLintContext context,
    error.AnalysisError analysisError,
    List<error.AnalysisError> others,
  ) {
    InstanceCreationExpression? node =
        analysisError.data as InstanceCreationExpression?;
    if (node != null) {
      final changeBuilder = reporter.createChangeBuilder(
        message: 'Wrap MaterialApp widget with WTheme',
        priority: 10,
      );
      changeBuilder.addDartFileEdit((builder) {
        // Wrap the Material widget with Wtheme
        builder.addReplacement(SourceRange(node.offset, node.length),
            (builder) {
          builder.write('WTheme(\n');
          builder.write(' child: ');
          builder.write(node.toSource());
          builder.write(',\n)');
        });
        builder.format(SourceRange(node.offset, node.length));
      });
    }
  }
}
