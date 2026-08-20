package com.hocheol.smartmonitoringsystem

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.hocheol.smartmonitoringsystem.ui.MonitoringScreen
import com.hocheol.smartmonitoringsystem.ui.theme.SmartMonitoringSystemTheme

/**
 * 스마트 모니터링 시스템 안드로이드 클라이언트의 메인 Activity
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SmartMonitoringSystemTheme {
                MonitoringScreen()
            }
        }
    }
}