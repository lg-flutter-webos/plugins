import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';

class JsonAccessMustBeChecked extends DartLintRule {
  JsonAccessMustBeChecked() : super(code: _code);

  static const _code = LintCode(
    name: 'json_access_must_be_checked',
    problemMessage:
        'JSON data access must be wrapped in try-catch or checked with isMap and containsKey.',
    errorSeverity: error.ErrorSeverity.WARNING,
  );

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    context.registry.addIndexExpression((IndexExpression node) {
      // `json['key']` parses as an IndexExpression, not a MethodInvocation.
      // Walk up the AST: skip the access when it sits inside a try block,
      // an `if` whose condition mentions isMap/containsKey, or a ternary
      // whose condition mentions isMap/containsKey.
      AstNode? ancestor = node.parent;
      while (ancestor != null) {
        if (ancestor is TryStatement) {
          return;
        }
        if (ancestor is IfStatement) {
          final cond = ancestor.expression.toString();
          if (cond.contains('isMap') || cond.contains('containsKey')) {
            return;
          }
        }
        if (ancestor is ConditionalExpression) {
          final cond = ancestor.condition.toString();
          if (cond.contains('isMap') || cond.contains('containsKey')) {
            return;
          }
        }
        ancestor = ancestor.parent;
      }
      reporter.atNode(node, _code);
    });
  }
}
