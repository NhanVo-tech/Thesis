// ignore_for_file: use_build_context_synchronously

import 'package:smart_car_app/screen/forgot_password.dart';
import 'package:smart_car_app/screen/dashboard.dart';
import 'package:smart_car_app/service/auth.dart';
import 'package:smart_car_app/screen/signup.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/app_components.dart';

class LogIn extends StatefulWidget {
  const LogIn({super.key});

  @override
  State<LogIn> createState() => _LogInState();
}

class _LogInState extends State<LogIn> {
  String email = "", password = "";

  TextEditingController mailcontroller = TextEditingController();
  TextEditingController passwordcontroller = TextEditingController();

  final _formkey = GlobalKey<FormState>();

  Future<void> _setLoggedIn(bool value) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('is_logged_in', value); // Android HCE reads this as 'flutter.is_logged_in'
  }

  userLogin() async {
    try {
      await FirebaseAuth.instance
          .signInWithEmailAndPassword(email: email, password: password);
      // Mark user as logged in for Android HCE gating
      await _setLoggedIn(true);
      // Clear stored credentials so returning to login screen won't show them
      mailcontroller.clear();
      passwordcontroller.clear();
      email = '';
      password = '';
      Navigator.push(context, MaterialPageRoute(builder: (context) => const Dashboard()));
    } on FirebaseAuthException catch (e) {
      if (e.code == 'user-not-found') {
        AppSnackBar.showError(context, "Email not found. Please check or create an account");
      } else if (e.code == 'wrong-password') {
        AppSnackBar.showError(context, "Incorrect password. Please try again");
      } else if (e.code == 'invalid-email') {
        AppSnackBar.showError(context, "Invalid email format. Please check and try again");
      } else if (e.code == 'user-disabled') {
        AppSnackBar.showError(context, "This account has been disabled. Contact support");
      } else {
        AppSnackBar.showError(context, "Login failed: ${e.message}");
      }
    } catch (e) {
      AppSnackBar.showError(context, "An unexpected error occurred. Please try again");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      resizeToAvoidBottomInset: true,
      body: SingleChildScrollView(
        child: Column(
            children: [
            SizedBox(
                width: MediaQuery.of(context).size.width,
                child: Image.asset(
                  'assets/images/car.PNG',
                  fit: BoxFit.cover,
                )),
            SizedBox(
              height: 10.0,
            ),
            Center(
              child: Text(
                'BK Smart Car',
                style: TextStyle(
                  color: AppColors.textPrimary,
                  fontSize: 28.0,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ),
            SizedBox(
              height: 20.0,
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0),
              child: Form(
                key: _formkey,
                child: Column(
                  children: [
                    AppTextField(
                      hintText: "Email",
                      controller: mailcontroller,
                      keyboardType: TextInputType.emailAddress,
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Please Enter E-mail';
                        }
                        return null;
                      },
                    ),
                    SizedBox(
                      height: 30.0,
                    ),
                    AppTextField(
                      hintText: "Password",
                      controller: passwordcontroller,
                      obscureText: true,
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Please Enter Password';
                        }
                        return null;
                      },
                    ),
                    SizedBox(
                      height: 30.0,
                    ),
                    AppButton(
                      label: "Sign In",
                      onPressed: () {
                        if(_formkey.currentState!.validate()) {
                          setState(() {
                            email = mailcontroller.text;
                            password = passwordcontroller.text;
                          });
                        }
                        userLogin();
                      },
                    ),
                  ],
                ),
              ),
            ),
            SizedBox(
              height: 20.0,
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0),
              child: Align(
                alignment: Alignment.centerLeft,
                child: GestureDetector(
                  onTap: (){
                    Navigator.push(context, MaterialPageRoute(builder: (context)=> ForgotPassword()));
                  },
                  child: Text("Forgot Password?",
                      style: TextStyle(
                          color: AppColors.textSecondary,
                          fontSize: 18.0,
                          fontWeight: FontWeight.w500)),
                ),
              ),
            ),
            SizedBox(
              height: 40.0,
            ),
            Text(
              "or LogIn with",
              style: TextStyle(
                  color: AppColors.textPrimary,
                  fontSize: 22.0,
                  fontWeight: FontWeight.w500),
            ),
            SizedBox(
              height: 30.0,
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                GestureDetector(
                  onTap: () async {
                    await AuthMethods().signInWithGoogle(context);
                    // If Google sign-in succeeded, mark as logged in
                    if (FirebaseAuth.instance.currentUser != null) {
                      await _setLoggedIn(true);
                    }
                    // clear local fields after initiating social login
                    mailcontroller.clear();
                    passwordcontroller.clear();
                  },
                  child: Image.asset(
                    'assets/images/google.png',
                    height: 45,
                    width: 45,
                    fit: BoxFit.contain,
                  ),
                ),
                // Apple sign-in removed
              ],
            ),
            SizedBox(
              height: 40.0,
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text("Don't have an account?",
                    style: TextStyle(
                        color: AppColors.textSecondary,
                        fontSize: 18.0,
                        fontWeight: FontWeight.w500)),
                SizedBox(
                  width: 5.0,
                ),
                GestureDetector(
                  onTap: () {
                    Navigator.push(context,
                        MaterialPageRoute(builder: (context) => SignUp()));
                  },
                  child: Text(
                    "SignUp",
                    style: TextStyle(
                        color: AppColors.textPrimary,
                        fontSize: 20.0,
                        fontWeight: FontWeight.w500),
                  ),
                ),
              ],
            )
          ],
        ),
      ),
    );
  }
}