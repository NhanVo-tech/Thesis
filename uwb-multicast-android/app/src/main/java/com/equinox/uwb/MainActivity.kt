package com.equinox.uwb

import android.Manifest.permission.RANGING
import android.content.pm.PackageManager
import android.os.Bundle
import android.ranging.RangingManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.ui.Modifier
import com.equinox.uwb.ui.theme.UwbTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        enableEdgeToEdge()

        val permissions = arrayOf(RANGING)

        if (permissions.all { checkSelfPermission(it) == PackageManager.PERMISSION_GRANTED }) {
            launchApp()
        } else {
            registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { result ->
                if (permissions.all { result[it] == true }) {
                    launchApp()
                } else {
                    Toast.makeText(this, "Permission denied", Toast.LENGTH_LONG).show()
                    finish()
                }
            }.launch(permissions)
        }
    }

    private fun launchApp() {
        val rangingManager = getSystemService(RangingManager::class.java)

        setContent {
            UwbTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding).fillMaxSize()) {
                        EstimateUwbLocation(rangingManager)
                    }
                }
            }
        }
    }
}
