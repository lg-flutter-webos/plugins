import 'package:custom_lint_builder/custom_lint_builder.dart';
import 'package:analyzer/dart/ast/ast.dart';
import 'package:analyzer/error/error.dart' as error;
import 'package:analyzer/error/listener.dart';

class CheckLocalizationsDelegates extends DartLintRule {
  CheckLocalizationsDelegates() : super(code: _code);

  static const _code = LintCode(
    name: 'missing_localizations_delegates',
    problemMessage:
        'MaterialApp must include specific localizationsDelegates for webOS.',
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
      if (node.constructorName.type.type?.getDisplayString() == 'MaterialApp') {
        var localizationsDelegates = null;
        for (var arg
            in node.argumentList.arguments.whereType<NamedExpression>()) {
          if (arg.name.label.name == 'localizationsDelegates') {
            localizationsDelegates = arg;
            break;
          }
        }

        if (localizationsDelegates == null) {
          return;
        }
        if (!localizationsDelegates.expression
                .toString()
                .contains('GlobalwebOSMaterialLocalizations.delegate') ||
            !localizationsDelegates.expression
                .toString()
                .contains('GlobalwebOSWidgetsLocalizations.delegate') ||
            !localizationsDelegates.expression
                .toString()
                .contains('GlobalwebOSCupertinoLocalizations.delegate')) {
          reporter.atNode(node, _code);
        }
      }
    });
  }
}
