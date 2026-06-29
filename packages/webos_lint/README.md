# webos_lint

Custom lint rules for webOS Flutter applications. Built on top of [custom_lint](https://pub.dev/packages/custom_lint).

## Installation

1. Add `webos_lint` and `custom_lint` to your `dev_dependencies` in `pubspec.yaml`:

```yaml
dev_dependencies:
  custom_lint:
  webos_lint: ^1.0.0
```

2. Add the custom lint plugin to your `analysis_options.yaml`:

```yaml
analyzer:
  plugins:
    - custom_lint
```

## Lint Rules

| Rule | Severity | Description |
|------|----------|-------------|
| `appinfo_transparent_check` | Warning | Checks that `transparent` key is set to `true` in `appinfo.json` (when `video_player_webos` is used) |
| `json_access_must_be_checked` | Warning | JSON data access must be wrapped in try-catch or checked with `containsKey` |
| `timer_should_be_disposed` | Warning | `Timer` instances should be canceled when no longer needed |
| `material_must_be_wrapped_with_wtheme` | Warning | `MaterialApp` must be wrapped with `WTheme` (when `plover` is used) |
| `add_context_must_have_remove_context` | Warning | `WVoiceControl().addContext()` must be paired with `removeContext()` |
| `missing_localizations_delegates` | Error | `MaterialApp` must include webOS-specific localization delegates |
| `expanded_must_be_in_flex` | Error | `Expanded` widgets must be children of `Row`, `Column`, or `Flex` |
| `font_family_must_use_wfont` | Warning | `fontFamily` must use `WFont` (when `plover` is used) |
| `focus_root_scope_restrict_focusable_usage` | Warning | Avoid using `WFocusable` or `WFocusableScope` above `FocusRootScope` |

## Usage

After setup, lint rules are applied automatically during analysis. To manually run:

```bash
dart run custom_lint
```

## Example

See the [example application](example/) for a project with `webos_lint` configured.
