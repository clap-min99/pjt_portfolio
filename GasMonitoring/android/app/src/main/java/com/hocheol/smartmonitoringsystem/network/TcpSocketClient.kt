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
 * Qt GUI 서버와의 비동기 소켓 통신을 담당하는 네트워크 엔진
 */
class TcpSocketClient(
    private val scope: CoroutineScope
) {
    private var socket: Socket? = null
    private var writer: PrintWriter? = null
    private var reader: BufferedReader? = null
    private var outputStream: OutputStream? = null
    private var receiveJob: Job? = null

    private val sendMutex = Mutex()

    private val _events = MutableSharedFlow<SocketEvent>(extraBufferCapacity = 64)
    val events = _events.asSharedFlow()

    sealed class SocketEvent {
        data class Connected(val ip: String, val port: Int) : SocketEvent()
        data class Disconnected(val message: String? = null) : SocketEvent()
        data class MessageReceived(val text: String) : SocketEvent()
        data class Error(val message: String) : SocketEvent()
    }

    fun connect(ip: String, port: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                disconnectInternal()

                val newSocket = Socket()
                newSocket.connect(InetSocketAddress(ip, port), 5000)
                // 소켓 타임아웃 설정 (비정상 종료 감지 도움)
                newSocket.soTimeout = 10000

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
                handleDisconnect()
            }
        }
    }

    private fun startReceiveLoop() {
        receiveJob = scope.launch(Dispatchers.IO) {
            try {
                while (isActive && isSocketActive()) {
                    val receivedMessage = reader?.readLine()
                    if (receivedMessage != null) {
                        _events.emit(SocketEvent.MessageReceived(receivedMessage))
                    } else {
                        handleDisconnect("서버에서 연결을 종료했습니다.")
                        break
                    }
                }
            } catch (e: Exception) {
                if (isActive) {
                    handleDisconnect("수신 오류: ${e.message}")
                }
            }
        }
    }

    fun sendText(text: String) {
        scope.launch(Dispatchers.IO) {
            sendMutex.withLock {
                try {
                    val w = writer ?: throw Exception("연결되지 않음")
                    w.println(text)
                    if (w.checkError()) throw Exception("스트림 쓰기 오류")
                } catch (e: Exception) {
                    handleDisconnect("명령 전송 중 연결 유실")
                }
            }
        }
    }

    fun sendRaw(data: ByteArray) {
        scope.launch(Dispatchers.IO) {
            sendMutex.withLock {
                try {
                    val os = outputStream ?: throw Exception("연결되지 않음")
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
                } catch (e: Exception) {
                    // 데이터 전송 실패는 소켓이 닫혔을 가능성이 큼
                    if (!isSocketActive()) {
                        handleDisconnect("데이터 전송 중 연결 유실")
                    }
                }
            }
        }
    }

    fun disconnect() {
        scope.launch(Dispatchers.IO) {
            handleDisconnect("사용자가 연결을 종료했습니다.")
        }
    }

    private fun isSocketActive(): Boolean {
        val s = socket
        return s != null && s.isConnected && !s.isClosed
    }

    private suspend fun handleDisconnect(message: String? = null) {
        if (socket != null || receiveJob != null) {
            disconnectInternal()
            _events.emit(SocketEvent.Disconnected(message))
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