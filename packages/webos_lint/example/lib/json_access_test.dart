class ExampleModel {
  final String? error;
  final String? notError;

  ExampleModel({
    this.error,
    this.notError,
  });

  factory ExampleModel.fromJson(dynamic json) {
    return ExampleModel(
      // expect_lint: json_access_must_be_checked
      error: json['error'],
      notError: (json is Map && json.containsKey('notError'))
          ? json['notError']
          : null,
    );
  }
}
