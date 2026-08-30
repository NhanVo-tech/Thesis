import 'package:smart_car_app/screen/login.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:smart_car_app/theme/app_colors.dart';
import 'package:smart_car_app/widgets/app_components.dart';

class SignUp extends StatefulWidget {
  const SignUp({super.key});

  @override
  State<SignUp> createState() => _SignUpState();
}

class _SignUpState extends State<SignUp> {
  String email = "", password = "", name = "";
  TextEditingController namecontroller = TextEditingController();
  TextEditingController passwordcontroller = TextEditingController();
  TextEditingController mailcontroller = TextEditingController();

  final _formkey = GlobalKey<FormState>();

  registration() async {
    // Ensure all fields are non-empty (password/name/email)
    if (password.isNotEmpty && namecontroller.text.isNotEmpty && mailcontroller.text.isNotEmpty) {
      try {
        await FirebaseAuth.instance
            .createUserWithEmailAndPassword(email: email, password: password);
        if (!mounted) return;
        AppSnackBar.showSuccess(context, "Registered Successfully");
        // After sign-up, navigate user to login screen instead of Home
        Navigator.pushReplacement(
            context, MaterialPageRoute(builder: (context) => LogIn()));
      } on FirebaseAuthException catch (e) {
        if (!mounted) return;
        if (e.code == 'weak-password') {
          AppSnackBar.showError(context, "Password too weak. Use 8+ characters with numbers and symbols");
        } else if (e.code == "email-already-in-use") {
          AppSnackBar.showError(context, "Email already registered. Try logging in instead");
        } else if (e.code == 'invalid-email') {
          AppSnackBar.showError(context, "Invalid email format. Please check and try again");
        } else {
          AppSnackBar.showError(context, "Sign up failed: ${e.message}");
        }
      } catch (e) {
        AppSnackBar.showError(context, "An unexpected error occurred. Please try again");
      }
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
              height: 30.0,
            ),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0),
              child: Form(
                key: _formkey,
                child: Column(
                  children: [
                    AppTextField(
                      hintText: "Name",
                      controller: namecontroller,
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Please Enter Name';
                        }
                        return null;
                      },
                    ),
                    SizedBox(
                      height: 30.0,
                    ),
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
                      label: "Sign Up",
                      onPressed: () {
                        if(_formkey.currentState!.validate()) {
                          setState(() {
                            email = mailcontroller.text;
                            name = namecontroller.text;
                            password = passwordcontroller.text;
                          });
                        }
                        registration();
                      },
                    ),
                  ],
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
                Image.asset(
                  'assets/images/google.png',
                  height: 50,
                  width: 50,
                  fit: BoxFit.contain,
                ),
              ],
            ),
            SizedBox(
              height: 40.0,
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text("Already have an account?",
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
                        MaterialPageRoute(builder: (context) => LogIn()));
                  },
                  child: Text(
                    "LogIn",
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