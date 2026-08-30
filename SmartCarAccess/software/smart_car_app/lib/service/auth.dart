import 'package:smart_car_app/screen/dashboard.dart';
import 'package:smart_car_app/service/database.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:google_sign_in/google_sign_in.dart';

class AuthMethods {
  final FirebaseAuth auth = FirebaseAuth.instance;

  Future<User?> getCurrentUser() async {
    return auth.currentUser;
  }

  signInWithGoogle(BuildContext context) async {
    try {
      final FirebaseAuth firebaseAuth = FirebaseAuth.instance;
      final GoogleSignIn googleSignIn = GoogleSignIn();

      print('Starting Google Sign-In...');

      final GoogleSignInAccount? googleSignInAccount = await googleSignIn.signIn();

      // user cancelled the Google sign-in
      if (googleSignInAccount == null) {
        print('User cancelled Google Sign-In');
        return;
      }

      print('Google Sign-In account obtained: ${googleSignInAccount.email}');

      final GoogleSignInAuthentication googleSignInAuthentication =
          await googleSignInAccount.authentication;

      print('Google Sign-In authentication obtained');

      final AuthCredential credential = GoogleAuthProvider.credential(
        idToken: googleSignInAuthentication.idToken,
        accessToken: googleSignInAuthentication.accessToken,
      );

      final UserCredential result = await firebaseAuth.signInWithCredential(credential);
      final User? userDetails = result.user;

      if (userDetails != null) {
        print('Firebase sign-in successful: ${userDetails.email}');
        
        final Map<String, dynamic> userInfoMap = {
          "email": userDetails.email,
          "name": userDetails.displayName,
          "imgUrl": userDetails.photoURL,
          "id": userDetails.uid
        };
        await DatabaseMethods().addUser(userDetails.uid, userInfoMap);
        // ignore: use_build_context_synchronously
        Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => Dashboard()));
      }
    } catch (e) {
      print('Google Sign-In error: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Google Sign-In failed: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  // Email/Password Sign-In for testing
  Future<void> signInWithEmailPassword(BuildContext context, String email, String password) async {
    try {
      final UserCredential result = await auth.signInWithEmailAndPassword(
        email: email, 
        password: password
      );
      final User? userDetails = result.user;

      if (userDetails != null) {
        print('Email sign-in successful: ${userDetails.email}');
        
        final Map<String, dynamic> userInfoMap = {
          "email": userDetails.email,
          "name": userDetails.displayName ?? "User",
          "imgUrl": userDetails.photoURL,
          "id": userDetails.uid
        };
        await DatabaseMethods().addUser(userDetails.uid, userInfoMap);
        // ignore: use_build_context_synchronously
        Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => Dashboard()));
      }
    } catch (e) {
      print('Email sign-in error: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Sign-in failed: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }
}