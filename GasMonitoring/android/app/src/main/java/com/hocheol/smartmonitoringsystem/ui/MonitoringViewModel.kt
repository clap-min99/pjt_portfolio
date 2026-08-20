package com.hocheol.smartmonitoringsystem.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hocheol.smartmonitoringsystem.network.TcpSocketClient
import com.hocheol.smartmonitoringsystem.network.TcpSocketClient.SocketEvent
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 스마트 모니터링 시스템의 UI 상태 관리 및 네트워크 명령을 조율하는 ViewModel
 */
class MonitoringViewModel : ViewModel() {

    private val socketClient = TcpSocketClient(viewModelScope)

    private val _ip = MutableStateFlow("100.72.78.11")
    val ip = _ip.asStateFlow()

    private val _port = MutableStateFlow("8080")
    val port = _port.asStateFlow()

    private val _isConnected = MutableStateFlow(false)
    val isConnected = _isConnected.asStateFlow()

    private val _isStreaming = MutableStateFlow(false)
    val isStreaming = _isStreaming.asStateFlow()

    private val _cameraSelector = MutableStateFlow(0) // 0: 후면 카메라, 1: 전면 카메라
    val cameraSelector = _cameraSelector.asStateFlow()

    private val _isFlashEnabled = MutableStateFlow(false)
    val isFlashEnabled = _isFlashEnabled.asStateFlow()

    private val _gasValue = MutableStateFlow(0)
    val gasValue = _gasValue.asStateFlow()

    private val _threshold = MutableStateFlow(3000)
    val threshold = _threshold.asStateFlow()

    private val _logs = MutableStateFlow<List<String>>(emptyList())
    val logs = _logs.asStateFlow()

    init {
        observeSocketEvents()
    }

    private fun observeSocketEvents() {
        viewModelScope.launch {
            socketClient.events.collect { event ->
                when (event) {
                    is SocketEvent.Connected -> {
                        _isConnected.value = true
                        addLog("서버 연결 성공 (${event.ip}:${event.port})")
                    }

                    is SocketEvent.Disconnected -> {
                        _isConnected.value = false
                        _isStreaming.value = false
                        addLog(event.message ?: "서버 연결 해제됨")
                    }

                    is SocketEvent.MessageReceived -> {
                        val text = event.text.trim()
                        if (text.startsWith("GAS:")) {
                            parseGasData(text)
                        } else {
                            addLog("수신: $text")
                        }
                    }

                    is SocketEvent.Error -> {
                        addLog("오류: ${event.message}")
                    }
                }
            }
        }
    }

    // 서버 중계 가스 데이터 파싱 (포맷: GAS:수치:임계값)
    private fun parseGasData(text: String) {
        val parts = text.split(":")
        if (parts.size >= 3) {
            _gasValue.value = parts[1].toIntOrNull() ?: _gasValue.value
            _threshold.value = parts[2].toIntOrNull() ?: _threshold.value
        }
    }

    fun onIpChange(newIp: String) {
        _ip.value = newIp
    }

    fun onPortChange(newPort: String) {
        _port.value = newPort
    }

    fun toggleConnection() {
        if (_isConnected.value) {
            socketClient.disconnect()
        } else {
            val portInt = _port.value.toIntOrNull() ?: 8080
            socketClient.connect(_ip.value, portInt)
            addLog("서버 연결 시도 중...")
        }
    }

    fun toggleStreaming() {
        if (!_isConnected.value) {
            addLog("먼저 서버에 연결해 주세요.")
            return
        }
        _isStreaming.value = !_isStreaming.value
        addLog(if (_isStreaming.value) "비디오 스트리밍 시작" else "비디오 스트리밍 중지")
    }

    fun toggleCamera() {
        _cameraSelector.value = if (_cameraSelector.value == 0) 1 else 0
        addLog("카메라 전환: ${if (_cameraSelector.value == 0) "후면" else "전면"}")
    }

    fun toggleFlash() {
        _isFlashEnabled.value = !_isFlashEnabled.value
        addLog("플래시: ${if (_isFlashEnabled.value) "ON" else "OFF"}")
    }

    fun sendValveClose() {
        socketClient.sendText("1")
        addLog("명령 송신: 밸브 차단 ('1')")
    }

    fun sendValveOpen() {
        socketClient.sendText("0")
        addLog("명령 송신: 밸브 복구 ('0')")
    }

    fun sendVideoFrame(data: ByteArray) {
        if (_isConnected.value && _isStreaming.value) {
            socketClient.sendRaw(data)
        }
    }

    fun clearLogs() {
        _logs.value = emptyList()
    }

    // 슬라이딩 윈도우 방식으로 최대 100개 로그 유지
    private fun addLog(text: String) {
        val timestamp = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        val formattedLog = "[$timestamp] $text"
        _logs.update { (it + formattedLog).takeLast(100) }
    }

    override fun onCleared() {
        super.onCleared()
        socketClient.disconnect()
    }
}