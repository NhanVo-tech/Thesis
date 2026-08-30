# Smart Car App 🚗

**Smart Car App** is a mobile application built with **Flutter**, designed to store and use **digital car keys** for unlocking vehicles and performing other smart functions.  
The app replaces traditional physical keys with a **secure, digital, and user-friendly** experience.

---

## Features

- 🔐 **Firebase Authentication** – Sign in using Email/Password or Google account.  
- 🗝️ **Digital Car Key Management** – Save and access your smart car key directly on your phone.  
- 🔓 **Unlock Vehicle** – Interact with your car via NFC or Bluetooth connection.  
- 🔢 **PIN Verification** – Require a security PIN before unlocking.  
- ☁️ **Firebase Integration** – Cloud Firestore for data storage and synchronization.  
- 🎨 **Modern UI** – Simple, clean, and responsive design.

---

## Tech Stack

| Component | Technology |
|------------|-------------|
| Framework | Flutter (Dart) |
| Authentication | Firebase Auth |
| Database | Cloud Firestore |
| Cloud Storage | Firebase Storage |
| Notification | Firebase Cloud Messaging (FCM) |
| Platforms | Android (iOS – coming soon) |

---

## Project Structure

smart_car_app/
├── lib/
│ ├── main.dart # Entry point
│ ├── service/ # Firebase Auth, KeyService, etc.
├── images/ # App images/icons
├── android/ # Android configuration
├── pubspec.yaml # Dependencies
└── README.md

---

## 🚀 Getting Started
### 1️⃣ Install Flutter

Follow the official guide:
➡️ Flutter Installation

### 2️⃣ Clone the Repository
    git clone https://github.com/<your-username>/smart_car_app.git
    cd smart_car_app
### 3️⃣ Install Dependencies
    flutter pub get
### 4️⃣ Configure Firebase

    Go to Firebase Console and create a new project.

    Download the google-services.json file and place it in:

        android/app/

    Enable Email/Password and Google Sign-In under Firebase Authentication.

    Create a Cloud Firestore Database.

### 5️⃣ Run the App
    flutter run
