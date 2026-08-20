package com.hocheol.smartmonitoringsystem.network

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.io.PrintWriter
import java.net.InetSocketAddress
import java.net.Socket

/**
 * Qt GUI 서버와의 비동기 소켓 통신(명령 송수신 및 영상 바이너리 스트리밍)을 담당하는 네트워크 엔진
 */
class TcpSocketClient(
    private val scope: CoroutineScope
) {
    private var socket: Socket? = null
    private var writer: PrintWriter? = null
    private var reader: BufferedReader? = null
    private var outputStream: OutputStream? = null
    private var receiveJob: Job? = null

    // 송신 스레드 안전성 보장을 위한 동기화 락
    private val sendMutex = Mutex()

    private val _events = MutableSharedFlow<SocketEvent>(extraBufferCapacity = 64)
    val events = _events.asSharedFlow()

    sealed class SocketEvent {
        data class Connected(val ip: String, val port: Int) : SocketEvent()
        data class Disconnected(val message: String? = null) : SocketEvent()
        data class MessageReceived(val text: String) : SocketEvent()
        data class Error(val message: String) : SocketEvent()
    }

    /**
     * 지정된 IP와 포트로 TCP 소켓 비동기 연결 시도 (타임아웃 5초)
     */
    fun connect(ip: String, port: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                disconnectInternal()

                val newSocket = Socket()
                newSocket.connect(InetSocketAddress(ip, port), 5000)

                val outStream = newSocket.getOutputStream()
                val inStream = newSocket.getInputStream()

                socket = newSocket
                outputStream = outStream
                writer = PrintWriter(outStream, true)
                reader = BufferedReader(InputStreamReader(inStream))

                _events.emit(SocketEvent.Connected(ip, port))
                startReceiveLoop()
            } catch (e: Exception) {
                _events.emit(SocketEvent.Error(e.message ?: "서버 연결에 실패했습니다."))
                disconnectInternal()
            }
        }
    }

    // 서버 메시지 실시간 수신 루프
    private fun startReceiveLoop() {
        receiveJob = scope.launch(Dispatchers.IO) {
            try {
                while (isActive && socket?.isConnected == true) {
                    val receivedMessage = reader?.readLine()
                    if (receivedMessage != null) {
                        _events.emit(SocketEvent.MessageReceived(receivedMessage))
                    } else {
                        _events.emit(SocketEvent.Disconnected("서버에 의해 연결이 종료되었습니다."))
                        disconnectInternal()
                        break
                    }
                }
            } catch (e: Exception) {
                if (isActive) {
                    _events.emit(SocketEvent.Error("수신 오류: ${e.message}"))
                    disconnectInternal()
                }
            }
        }
    }

    /**
     * 텍스트 기반 제어 명령 전송 ('1': 차단, '0': 복구)
     */
    fun sendText(text: String) {
        scope.launch(Dispatchers.IO) {
            sendMutex.withLock {
                try {
                    writer?.println(text)
                } catch (e: Exception) {
                    _events.emit(SocketEvent.Error("명령 전송 실패: ${e.message}"))
                }
            }
        }
    }

    /**
     * 4바이트 Big-Endian 페이로드 헤더를 포함한 비디오 프레임 바이너리 스트리밍
     */
    fun sendRaw(data: ByteArray) {
        scope.launch(Dispatchers.IO) {
            sendMutex.withLock {
                try {
                    outputStream?.let { os ->
                        val size = data.size
                        os.write(
                            byteArrayOf(
                                (size ushr 24).toByte(),
                                (size ushr 16).toByte(),
                                (size ushr 8).toByte(),
                                size.toByte()
                            )
                        )
                        os.write(data)
                        os.flush()
                    }
                } catch (_: Exception) {
                    // 고빈도 스트리밍 패킷 에러는 채널 혼잡 방지를 위해 로깅 생략
                }
            }
        }
    }

    fun disconnect() {
        scope.launch(Dispatchers.IO) {
            disconnectInternal()
            _events.emit(SocketEvent.Disconnected("연결이 종료되었습니다."))
        }
    }

    private fun disconnectInternal() {
        try {
            receiveJob?.cancel()
            receiveJob = null
            socket?.close()
            socket = null
            writer = null
            reader = null
            outputStream = null
        } catch (_: Exception) {
        }
    }
}