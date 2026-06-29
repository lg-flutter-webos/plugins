import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';

class ExpandedMustBeInFlex extends DartLintRule {
  ExpandedMustBeInFlex() : super(code: _code);

  static const _code = LintCode(
    name: 'expanded_must_be_in_flex',
    problemMessage:
        'Expanded widgets must be children of Row, Column, or Flex widgets.',
    errorSeverity: error.ErrorSeverity.ERROR,
  );

  @override
  void run(
    CustomLintResolver resolver,
    ErrorReporter reporter,
    CustomLintContext context,
  ) {
    context.registry
        .addInstanceCreationExpression((InstanceCreationExpression node) {
      if (node.constructorName.type.type?.getDisplayString() == 'Expanded') {
        // Check if the parent is Row, Column, or Flex
        bool isInFlex = false;
        AstNode? parent = node.parent;

        while (parent != null) {
          if (parent is InstanceCreationExpression &&
              (parent.constructorName.type.type?.getDisplayString() == 'Row' ||
                  parent.constructorName.type.type?.getDisplayString() ==
                      'Column' ||
                  parent.constructorName.type.type?.getDisplayString() ==
                      'Flex')) {
            isInFlex = true;
            parent = null;
            break;
          }

          // Check if the parent is Stack or Container
          if (parent is InstanceCreationExpression &&
              (parent.constructorName.type.type?.getDisplayString() ==
                      'Stack' ||
                  parent.constructorName.type.type?.getDisplayString() ==
                      'Container')) {
            reporter.atNode(node, _code);
            return;
          }

          parent = parent.parent;
        }

        // If not in Row, Column, or Flex, report an error
        if (!isInFlex) {
          reporter.atNode(node, _code);
        }
      }
    });
  }
}
