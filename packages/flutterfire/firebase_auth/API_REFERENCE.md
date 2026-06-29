# API Reference — firebase_auth_webos

> **Note:** For standard API documentation, see [`firebase_auth` on pub.dev](https://pub.dev/packages/firebase_auth).
> This document covers webOS-specific behavior only.

## Features

- Email/password sign-in and account creation
- Anonymous authentication
- Custom token authentication
- User profile management (display name, photo URL)
- Real-time auth state and ID token change streams
- Password reset and email verification
- Firebase Auth emulator support

## Overview

This plugin provides the webOS platform implementation of `firebase_auth`.
It implements `FirebaseAuthPlatform` and communicates with the Firebase C++ Auth SDK
through Pigeon-generated platform channels.

## Supported APIs

### Authentication Methods

| API | Supported | Notes |
|-----|-----------|-------|
| `signInWithEmailAndPassword()` | Yes | |
| `createUserWithEmailAndPassword()` | Yes | Returns `isNewUser: true` |
| `signInAnonymously()` | Yes | Returns `isNewUser: true` |
| `signInWithCustomToken()` | Yes | |
| `signOut()` | Yes | Synchronous on native side |
| `signInWithCredential()` | No | |
| `signInWithPopup()` | No | Browser-specific, not applicable to TV |
| `signInWithRedirect()` | No | Browser-specific, not applicable to TV |
| `signInWithPhoneNumber()` | No | Firebase C++ SDK limitation |
| `sendSignInLinkToEmail()` | No | Email link auth not implemented |

### User Management

| API | Supported | Notes |
|-----|-----------|-------|
| `currentUser` | Yes | |
| `User.getIdToken()` | Yes | |
| `User.updateProfile()` | Yes | Supports `displayName` and `photoURL` only |
| `User.sendEmailVerification()` | Yes | `ActionCodeSettings` parameter ignored |
| `User.updatePassword()` | Yes | Requires current authentication |
| `User.reload()` | Yes | Fetches fresh data, notifies state streams |
| `User.delete()` | Yes | Permanently deletes account |
| `sendPasswordResetEmail()` | Yes | `ActionCodeSettings` parameter ignored |
| `User.linkWithCredential()` | No | |
| `User.reauthenticateWithCredential()` | No | |
| `User.updateEmail()` | No | |
| `User.updatePhoneNumber()` | No | |

### State Streams

| Stream | Supported | Notes |
|--------|-----------|-------|
| `authStateChanges()` | Yes | Emits on sign-in/sign-out via C++ `AuthStateListener` |
| `idTokenChanges()` | Yes | Emits on token refresh via C++ `IdTokenListener` |
| `userChanges()` | Yes | Combined stream from both auth state and token changes |

### Other

| API | Supported | Notes |
|-----|-----------|-------|
| `useAuthEmulator()` | Yes | |
| Multi-Factor Authentication | No | Placeholder only |

## webOS-Specific Behavior

- **Multi-app support**: Instances are cached per Firebase App name. Use
  `getInstance(appName)` for multi-app scenarios.
- **Auth listeners**: Auth state and ID token listeners are lazily registered
  on the C++ side on first access and removed on plugin destruction.
- **ActionCodeSettings ignored**: Both `sendPasswordResetEmail()` and
  `sendEmailVerification()` accept but ignore the `ActionCodeSettings` parameter.
- **Credential always null**: `UserCredential.credential` is always `null`
  on webOS (no platform credential object).
