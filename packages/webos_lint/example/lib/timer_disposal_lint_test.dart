import 'dart:async';

import 'package:flutter/material.dart';

/// Perform command: dart run custom_lint
/// If any expected lint is missing, the command will fail.
/// But if your plugin correctly emits the lint, the command will succeed.
class TimerDisposalLintTestWidget extends StatefulWidget {
  const TimerDisposalLintTestWidget({super.key});

  @override
  State<TimerDisposalLintTestWidget> createState() =>
      _TimerDisposalLintTestWidgetState();
}

class _TimerDisposalLintTestWidgetState
    extends State<TimerDisposalLintTestWidget> {
  // expect_lint: timer_should_be_disposed
  Timer? myFirstTimer;

  Timer? mySecondTimer;

  @override
  void initState() {
    myFirstTimer = Timer(const Duration(seconds: 1), () {});
    mySecondTimer = Timer(const Duration(seconds: 1), () {});
    super.initState();
  }

  @override
  void dispose() {
    mySecondTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}
