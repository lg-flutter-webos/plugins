import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/dart/ast/visitor.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';
import 'package:custom_lint_builder/custom_lint_builder.dart';

class TimerDisposalLint extends DartLintRule {
  TimerDisposalLint()
      : super(
          code: _code,
        );

  static const _code = LintCode(
    name: 'timer_should_be_disposed',
    problemMessage:
        'Timer should be disposed (canceled) when no longer needed.',
    errorSeverity: error.ErrorSeverity.WARNING,
  );

  @override
  List<Fix> getFixes() => [_TimerDisposalFix()];

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    resolver.getResolvedUnitResult().then((unit) {
      unit.unit.visitChildren(_TimerVisitor(reporter));
    });
  }
}

class _TimerVisitor extends RecursiveAstVisitor<void> {
  final ErrorReporter _reporter;

  _TimerVisitor(this._reporter);

  @override
  void visitVariableDeclaration(VariableDeclaration node) {
    final type = node.declaredElement?.type.toString();
    if (type == 'Timer' || type == 'Timer?') {
      // Check if this Timer instance is disposed
      final parent = node.parent;
      if (parent is VariableDeclarationList) {
        final classDeclaration =
            parent.thisOrAncestorOfType<ClassDeclaration>();

        if (classDeclaration != null) {
          bool isDisposed = false;
          final isStateClass = classDeclaration.extendsClause
                  ?.toString()
                  .contains(RegExp(r'extends (State<|ConsumerState<)')) ??
              false;

          for (final member in classDeclaration.members) {
            /// if isStateClass, Timer should be canceled in dispose()
            /// else Timer should be canceled in any method
            if (member is MethodDeclaration &&
                (!isStateClass || member.name.lexeme == 'dispose')) {
              final body = member.body.toString();
              if (body.contains('${node.name}.cancel()') ||
                  body.contains('${node.name}?.cancel()')) {
                isDisposed = true;
                break;
              }
            }
          }

          if (!isDisposed) {
            _reporter.atNode(
              node,
              TimerDisposalLint._code,
              data: node,
            );
          }
        }
      }
    }
  }
}

class _TimerDisposalFix extends DartFix {
  @override
  void run(
    CustomLintResolver resolver,
    ChangeReporter reporter,
    CustomLintContext context,
    error.AnalysisError analysisError,
    List<error.AnalysisError> others,
  ) {
    resolver.getResolvedUnitResult().then((unit) {
      final node = analysisError.data as VariableDeclaration;
      final bool isNonNullable =
          node.declaredElement?.type.toString() == 'Timer';

      for (final classDecl
          in unit.unit.declarations.whereType<ClassDeclaration>()) {
        final isStateClass = classDecl.extendsClause
                ?.toString()
                .contains(RegExp(r'extends (State<|ConsumerState<)')) ??
            false;

        if (isStateClass) {
          final methods = classDecl.members.whereType<MethodDeclaration>();
          MethodDeclaration? disposeMethod;
          for (final method in methods) {
            if (method.name.lexeme == 'dispose') {
              disposeMethod = method;
              break;
            }
          }

          final changeBuilder = reporter.createChangeBuilder(
            message:
                "Add `${node.name.lexeme}${isNonNullable ? '' : '?'}.cancel();` in `dispose()`",
            priority: 1,
          );

          changeBuilder.addDartFileEdit((builder) {
            if (disposeMethod != null) {
              // If `dispose` exists, insert `_timer?.cancel();` at the beginning
              final insertOffset = disposeMethod.body.beginToken.offset + 1;
              builder.addInsertion(insertOffset, (editBuilder) {
                editBuilder.write(
                    '\n    ${node.name.lexeme}${isNonNullable ? '' : '?'}.cancel();');
              });
            } else {
              // If `dispose` does not exist, add the entire method
              final insertOffset = classDecl.end - 1;
              builder.addInsertion(insertOffset, (editBuilder) {
                editBuilder.write("""

  @override
  void dispose() {
    ${node.name.lexeme}${isNonNullable ? '' : '?'}.cancel();
    super.dispose();
  }
  """);
              });
            }
          });
        } else {
          final changeBuilder = reporter.createChangeBuilder(
            message:
                "Create disposeTimer() function and add `${node.name.lexeme}${isNonNullable ? '' : '?'}.cancel();` in `disposeTimer()`",
            priority: 1,
          );
          changeBuilder.addDartFileEdit((builder) {
            final insertOffset = classDecl.end - 1;
            builder.addInsertion(insertOffset, (editBuilder) {
              editBuilder.write("""

  void disposeTimer() {
    ${node.name.lexeme}${isNonNullable ? '' : '?'}.cancel();
  }
        """);
            });
          });
        }
      }
    });
  }
}
