import 'package:smart_car_app/screen/signup.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/app_components.dart';

class ForgotPassword extends StatefulWidget {
  const ForgotPassword({super.key});

  @override
  State<ForgotPassword> createState() => _ForgotPasswordState();
}

class _ForgotPasswordState extends State<ForgotPassword> {
  String email = "";
  TextEditingController mailcontroller = TextEditingController();

  final _formkey = GlobalKey<FormState>();
  resetPassword() async {
    // ensure email is trimmed and non-empty
    email = mailcontroller.text.trim();
    if (email.isEmpty) {
      if (!mounted) return;
      AppSnackBar.showError(context, "Email address is required. Please enter your email");
      return;
    }

    try {
      await FirebaseAuth.instance.sendPasswordResetEmail(email: email);
      if (!mounted) return;
      AppSnackBar.showSuccess(context, "Password reset link sent! Check your email (may take a few minutes)");
    } on FirebaseAuthException catch (e) {
      if (!mounted) return;
      if (e.code == "user-not-found") {
        AppSnackBar.showError(context, "Email not found. Please check or create an account");
      } else if (e.code == "too-many-requests") {
        AppSnackBar.showError(context, "Too many requests. Please try again later");
      } else if (e.code == "invalid-email") {
        AppSnackBar.showError(context, "Invalid email format. Please check and try again");
      } else {
        AppSnackBar.showError(context, "Reset failed: ${e.message}. Please try again");
      }
    } catch (e) {
      AppSnackBar.showError(context, "An unexpected error occurred. Please try again");
    }
  }

  @override
  void dispose() {
    mailcontroller.dispose();
    super.dispose();
  }
  
  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    precacheImage(AssetImage('assets/images/forgot.jpg'), context);
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
              height: 20.0,
            ),
            SizedBox(
              width: MediaQuery.of(context).size.width,
              height: MediaQuery.of(context).size.height * 0.25,
              child: Image.asset(
                'assets/images/forgot.jpg',
                fit: BoxFit.cover,
              ),
            ),
            SizedBox(
              height: 12.0,
            ),
            Container(
              alignment: Alignment.topCenter,
              child: Text(
                "Password Recovery",
                style: TextStyle(
                    color: AppColors.textPrimary,
                    fontSize: 30.0,
                    fontWeight: FontWeight.bold),
              ),
            ),
            SizedBox(height: 20.0),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 2.0),
              child: Align(
                alignment: Alignment.centerLeft,
                child: Text(
                  "Enter Your Email",
                  style: TextStyle(
                      color: AppColors.textPrimary,
                      fontSize: 20.0,
                      fontWeight: FontWeight.bold),
                ),
              ),
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 16.0),
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
                          return 'Please Enter Email';
                        }
                        return null;
                      },
                    ),
                    SizedBox(
                      height: 30.0,
                    ),
                    AppButton(
                      label: "Send Email",
                      width: 200,
                      onPressed: () {
                        if(_formkey.currentState!.validate()) {
                          setState(() {
                            email = mailcontroller.text;
                          });
                          resetPassword();
                        }
                      },
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(
                          "Don't have an account?",
                          style: TextStyle(
                              fontSize: 18.0, color: AppColors.textSecondary),
                        ),
                        SizedBox(
                          width: 5.0,
                        ),
                        GestureDetector(
                          onTap: () {
                            Navigator.push(
                                context,
                                MaterialPageRoute(
                                    builder: (context) => SignUp()));
                          },
                          child: Text(
                            "Sign In",
                            style: TextStyle(
                                color: AppColors.textPrimary,
                                fontSize: 20.0,
                                fontWeight: FontWeight.w500),
                          ),
                        )
                      ],
                    )
                  ],
                ),
              ),
            )
          ],
        ),
      ),
    );
  }
}