import 'package:flutter/material.dart';

class MyWidget extends StatelessWidget {
  const MyWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Expanded(
          child: Container(
            // expect_lint: expanded_must_be_in_flex
            child: Expanded(child: Text('data')),
          ),
        ),
        const Column(
          children: [
            Text('data 1'),
            Expanded(child: Text('data 2')),
          ],
        )
      ],
    );
  }
}
