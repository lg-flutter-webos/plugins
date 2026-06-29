
import 'dart:developer';

import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';

import 'auth.dart';
import 'main.dart';


const placeholderImage =
    'https://upload.wikimedia.org/wikipedia/commons/c/cd/Portrait_Placeholder_Square.png';

class ProfilePage extends StatefulWidget {
  const ProfilePage({Key? key}) : super(key: key);

  @override
  _ProfilePageState createState() => _ProfilePageState();
}

class _ProfilePageState extends State<ProfilePage> {
  late User user;
  late TextEditingController nameController;
  final TextEditingController photoUrlController = TextEditingController();
  final TextEditingController passwordController = TextEditingController();
  final TextEditingController newPasswordController = TextEditingController();

  String? idToken;
  bool showSaveButton = false;
  bool isLoading = false;

  @override
  void initState() {
    super.initState();
    user = auth.currentUser!;
    nameController = TextEditingController(text: user.displayName);
    nameController.addListener(_onNameChanged);

    auth.userChanges().listen((updatedUser) {
      if (updatedUser != null && mounted) {
        setState(() {
          user = updatedUser;
          nameController.text = user.displayName ?? '';
        });
      }
    });

    log('User initialized: ${user.uid}');
  }

  @override
  void dispose() {
    nameController.dispose();
    photoUrlController.dispose();
    passwordController.dispose();
    newPasswordController.dispose();
    super.dispose();
  }

  void setIsLoading() {
    setState(() {
      isLoading = !isLoading;
    });
  }

  void _onNameChanged() {
    setState(() {
      showSaveButton = nameController.text.isNotEmpty &&
          nameController.text != user.displayName;
    });
  }

  Future<void> updateDisplayName() async {
    setIsLoading();
    try {
      await user.updateProfile(displayName: nameController.text);
      setState(() {
        showSaveButton = false;
      });
      ScaffoldSnackbar.of(context).show('Display name updated successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> updatePhotoURL() async {
    final photoURL = photoUrlController.text.trim();
    if (photoURL.isEmpty) {
      ScaffoldSnackbar.of(context).show('Please enter a photo URL');
      return;
    }

    setIsLoading();
    try {
      await user.updateProfile(photoURL: photoURL);
      photoUrlController.clear();
      ScaffoldSnackbar.of(context).show('Photo URL updated successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> sendEmailVerification() async {
    setIsLoading();
    try {
      await user.sendEmailVerification();
      ScaffoldSnackbar.of(context).show('Verification email sent successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> updatePassword() async {
    final currentPassword = passwordController.text.trim();
    final newPassword = newPasswordController.text.trim();

    if (currentPassword.isEmpty || newPassword.isEmpty) {
      ScaffoldSnackbar.of(context).show('Please fill in both password fields');
      return;
    }

    setIsLoading();
    try {
      
      await user.updatePassword(newPassword);
      passwordController.clear();
      newPasswordController.clear();
      ScaffoldSnackbar.of(context).show('Password updated successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> reloadUser() async {
    setIsLoading();
    try {
      await user.reload();
      
      final updatedUser = auth.currentUser;
      if (updatedUser != null) {
        setState(() {
          user = updatedUser;
          nameController.text = user.displayName ?? '';
        });
      }
      ScaffoldSnackbar.of(context).show('User data reloaded successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> getIdToken() async {
    setIsLoading();
    try {
      final token = await user.getIdToken(true); // Force refresh
      setState(() {
        idToken = token;
      });
      ScaffoldSnackbar.of(context).show('ID token retrieved successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> deleteAccount() async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Account'),
        content: const Text('Are you sure you want to delete your account? This action cannot be undone.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Delete', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );

    if (confirmed != true) return;

    setIsLoading();
    try {
      await user.delete();
      ScaffoldSnackbar.of(context).show('Account deleted successfully');
    } on FirebaseAuthException catch (e) {
      ScaffoldSnackbar.of(context).show('Error: ${e.message}');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  Future<void> signOut() async {
    setIsLoading();
    try {
      await auth.signOut();
      ScaffoldSnackbar.of(context).show('Signed out successfully');
    } catch (e) {
      ScaffoldSnackbar.of(context).show('Error: $e');
    }
    setIsLoading();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Profile & Validation'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: reloadUser,
            tooltip: 'Reload User Data',
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // User Profile Section
            _buildSection('User Profile', [
              _buildProfileHeader(),
              const SizedBox(height: 20),
              _buildUserInfoDisplay(),
            ]),

            const SizedBox(height: 24),

            // Profile Management Section
            _buildSection('Profile Management', [
              _buildDisplayNameUpdate(),
              const SizedBox(height: 16),
              _buildPhotoUrlUpdate(),
              const SizedBox(height: 16),
              _buildEmailVerification(),
            ]),

            const SizedBox(height: 24),

            // Security Section
            _buildSection('Security', [
              _buildPasswordUpdate(),
              const SizedBox(height: 16),
              _buildIdTokenSection(),
            ]),

            const SizedBox(height: 24),

            // Account Actions Section
            _buildSection('Account Actions', [
              _buildAccountActions(),
            ]),

            const SizedBox(height: 24),

            // User Properties Validation Section
            _buildSection('User Properties Validation', [
              _buildUserPropertiesValidation(),
            ]),
          ],
        ),
      ),
    );
  }

  Widget _buildSection(String title, List<Widget> children) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              title,
              style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 16),
            ...children,
          ],
        ),
      ),
    );
  }

  Widget _buildProfileHeader() {
    return Center(
      child: Stack(
        children: [
          CircleAvatar(
            radius: 50,
            backgroundImage: NetworkImage(user.photoURL ?? placeholderImage),
          ),
          Positioned(
            right: 0,
            bottom: 0,
            child: Container(
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.secondary,
                shape: BoxShape.circle,
              ),
              child: IconButton(
                icon: const Icon(Icons.edit, size: 20),
                onPressed: () {
                  photoUrlController.text = user.photoURL ?? '';
                  _showPhotoUrlDialog();
                },
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildUserInfoDisplay() {
    return Column(
      children: [
        TextField(
          controller: nameController,
          textAlign: TextAlign.center,
          decoration: const InputDecoration(
            border: InputBorder.none,
            hintText: 'Display Name',
          ),
          style: Theme.of(context).textTheme.titleMedium,
        ),
        const SizedBox(height: 8),
        Text(
          user.email ?? 'Anonymous User',
          style: Theme.of(context).textTheme.bodyLarge,
        ),
        const SizedBox(height: 8),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            _buildStatusChip('Email Verified', user.emailVerified),
            const SizedBox(width: 8),
            _buildStatusChip('Anonymous', user.isAnonymous),
          ],
        ),
      ],
    );
  }

  Widget _buildStatusChip(String label, bool value) {
    return Chip(
      label: Text(label),
      backgroundColor: value ? Colors.green : Colors.grey,
      labelStyle: const TextStyle(color: Colors.white),
    );
  }

  Widget _buildDisplayNameUpdate() {
    return Row(
      children: [
        Expanded(
          child: TextField(
            controller: nameController,
            decoration: const InputDecoration(
              labelText: 'Display Name',
              border: OutlineInputBorder(),
            ),
          ),
        ),
        const SizedBox(width: 8),
        ElevatedButton(
          onPressed: showSaveButton && !isLoading ? updateDisplayName : null,
          child: const Text('Update'),
        ),
      ],
    );
  }

  Widget _buildPhotoUrlUpdate() {
    return Row(
      children: [
        Expanded(
          child: TextField(
            controller: photoUrlController,
            decoration: const InputDecoration(
              labelText: 'Photo URL',
              border: OutlineInputBorder(),
            ),
          ),
        ),
        const SizedBox(width: 8),
        ElevatedButton(
          onPressed: isLoading ? null : updatePhotoURL,
          child: const Text('Update'),
        ),
      ],
    );
  }

  Widget _buildEmailVerification() {
    return SizedBox(
      width: double.infinity,
      child: ElevatedButton.icon(
        onPressed: isLoading ? null : sendEmailVerification,
        icon: const Icon(Icons.email),
        label: const Text('Send Email Verification'),
      ),
    );
  }

  Widget _buildPasswordUpdate() {
    return Column(
      children: [
        TextField(
          controller: passwordController,
          obscureText: true,
          decoration: const InputDecoration(
            labelText: 'Current Password',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 12),
        TextField(
          controller: newPasswordController,
          obscureText: true,
          decoration: const InputDecoration(
            labelText: 'New Password',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: double.infinity,
          child: ElevatedButton(
            onPressed: isLoading ? null : updatePassword,
            child: const Text('Update Password'),
          ),
        ),
      ],
    );
  }

  Widget _buildIdTokenSection() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        ElevatedButton.icon(
          onPressed: isLoading ? null : getIdToken,
          icon: const Icon(Icons.vpn_key),
          label: const Text('Get ID Token'),
        ),
        if (idToken != null) ...[
          const SizedBox(height: 12),
          const Text('ID Token:', style: TextStyle(fontWeight: FontWeight.bold)),
          const SizedBox(height: 4),
          Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: Colors.grey[200],
              borderRadius: BorderRadius.circular(4),
            ),
            child: Text(
              idToken!,
              style: const TextStyle(fontFamily: 'monospace', fontSize: 10),
            ),
          ),
        ],
      ],
    );
  }

  Widget _buildAccountActions() {
    return Column(
      children: [
        SizedBox(
          width: double.infinity,
          child: OutlinedButton.icon(
            onPressed: isLoading ? null : reloadUser,
            icon: const Icon(Icons.refresh),
            label: const Text('Reload User Data'),
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: double.infinity,
          child: OutlinedButton.icon(
            onPressed: isLoading ? null : signOut,
            icon: const Icon(Icons.logout),
            label: const Text('Sign Out'),
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: double.infinity,
          child: ElevatedButton.icon(
            onPressed: isLoading ? null : deleteAccount,
            icon: const Icon(Icons.delete_forever),
            label: const Text('Delete Account'),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.red,
              foregroundColor: Colors.white,
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildUserPropertiesValidation() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildPropertyRow('UID', user.uid),
        _buildPropertyRow('Email', user.email ?? 'N/A'),
        _buildPropertyRow('Display Name', user.displayName ?? 'N/A'),
        _buildPropertyRow('Photo URL', user.photoURL ?? 'N/A'),
        _buildPropertyRow('Phone Number', user.phoneNumber ?? 'N/A'),
        _buildPropertyRow('Email Verified', user.emailVerified.toString()),
        _buildPropertyRow('Is Anonymous', user.isAnonymous.toString()),
      ],
    );
  }

  Widget _buildPropertyRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(
              '$label:',
              style: const TextStyle(fontWeight: FontWeight.bold),
            ),
          ),
          Expanded(
            child: Text(
              value,
              style: const TextStyle(fontFamily: 'monospace'),
            ),
          ),
        ],
      ),
    );
  }

  void _showPhotoUrlDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Update Photo URL'),
        content: TextField(
          controller: photoUrlController,
          decoration: const InputDecoration(
            hintText: 'Enter photo URL',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              updatePhotoURL();
            },
            child: const Text('Update'),
          ),
        ],
      ),
    );
  }
}