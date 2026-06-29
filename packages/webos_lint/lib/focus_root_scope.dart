import 'package:analyzer/source/source_range.dart';
import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';

class FocusRootScopeRestrictFocusableUsage extends DartLintRule {
  FocusRootScopeRestrictFocusableUsage() : super(code: _code);

  static const _code = LintCode(
    name: 'focus_root_scope_restrict_focusable_usage',
    problemMessage:
        'Avoid using WFocusable or WFocusableScope on top FocusRootScope.',
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

      // Check if the instance being created is a FocusRootScope
      final typeName = node.constructorName.type.type
          ?.getDisplayString();
      if (typeName != 'FocusRootScope') {
        return;
      }

      // Traverse up the AST to check if any parent is a WFocusable or WFocusableScope
      AstNode? parent = node.parent;
      while (parent != null) {
        if (parent is InstanceCreationExpression &&
            (parent.constructorName.type.type
                        ?.getDisplayString() ==
                    'WFocusable' ||
                parent.constructorName.type.type
                        ?.getDisplayString() ==
                    'WFocusableScope')) {
          // Report a lint error if WFocusable or WFocusableScope is found above FocusRootScope
          reporter.atNode(parent, _code, data: parent);
          break;
        }
        parent = parent.parent;
      }
    });
  }

  @override
  List<Fix> getFixes() => [_FocusRootScopeFix()];
}

class _FocusRootScopeFix extends DartFix {
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
        message: 'Remove WFocusable or WFocusableScope above FocusRootScope',
        priority: 10,
      );
      changeBuilder.addDartFileEdit((builder) {
        for (var arg
            in node.argumentList.arguments.whereType<NamedExpression>()) {
          if (arg.name.label.name == 'child') {
            // Replace current node with child
            final expression = arg.expression;
            builder.addReplacement(SourceRange(node.offset, node.length),
                (builder) {
              builder.write(expression.toSource());
            });
            // Format the changed range
            builder.format(SourceRange(node.offset, node.length));

            break;
          }
        }
      });
    }
  }
}
