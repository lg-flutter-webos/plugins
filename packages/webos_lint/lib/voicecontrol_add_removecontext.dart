import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';
import 'package:analyzer/dart/ast/visitor.dart';

class AddContextMustHaveRemoveContext extends DartLintRule {
  AddContextMustHaveRemoveContext() : super(code: _code);

  static const _code = LintCode(
    name: 'add_context_must_have_remove_context',
    problemMessage:
        'WVoiceControl().addContext() must be paired with WVoiceControl().removeContext() in the same file.',
    errorSeverity: error.ErrorSeverity.WARNING,
  );

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    context.registry.addCompilationUnit((CompilationUnit unit) {
      final visitor = _AddRemoveContextVisitor();
      unit.visitChildren(visitor);
      if (visitor.hasAddContext && !visitor.hasRemoveContext) {
        reporter.atOffset(
            offset: unit.offset, length: unit.length, errorCode: _code);
      }
    });
  }
}

class _AddRemoveContextVisitor extends RecursiveAstVisitor<void> {
  bool hasAddContext = false;
  bool hasRemoveContext = false;

  @override
  void visitMethodInvocation(MethodInvocation node) {
    //print('$node');
    if (node.methodName.name == 'addContext' &&
        node.realTarget?.toString() == 'WVoiceControl()') {
      hasAddContext = true;
    }
    if (node.methodName.name == 'removeContext' &&
        node.realTarget?.toString() == 'WVoiceControl()') {
      hasRemoveContext = true;
      super.visitMethodInvocation(node);
    }
  }
}
