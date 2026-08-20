package com.hocheol.smartmonitoringsystem.util

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.Rect
import android.graphics.YuvImage
import androidx.camera.core.ImageProxy
import java.io.ByteArrayOutputStream

/**
 * CameraX ImageProxy 프레임을 네트워크 전송용 JPEG 바이너리로 변환하는 유틸리티 객체
 */
object ImageUtils {

    /**
     * YUV_420_888 포맷의 ImageProxy를 압축 및 회전 보정된 JPEG 바이트 배열로 변환
     *
     * @param imageProxy CameraX 분석기에서 전달된 프레임 객체
     * @param quality JPEG 압축 품질 (0 ~ 100)
     * @return 압축된 JPEG 바이트 배열 (변환 실패 시 null)
     */
    fun imageProxyToJpeg(imageProxy: ImageProxy, quality: Int = 50): ByteArray? {
        if (imageProxy.format != ImageFormat.YUV_420_888) return null

        val yBuffer = imageProxy.planes[0].buffer
        val uBuffer = imageProxy.planes[1].buffer
        val vBuffer = imageProxy.planes[2].buffer

        val ySize = yBuffer.remaining()
        val uSize = uBuffer.remaining()
        val vSize = vBuffer.remaining()

        // NV21 포맷 버퍼 구성 (Y + V + U)
        val nv21 = ByteArray(ySize + uSize + vSize)
        yBuffer.get(nv21, 0, ySize)
        vBuffer.get(nv21, ySize, vSize)
        uBuffer.get(nv21, ySize + vSize, uSize)

        val yuvImage = YuvImage(nv21, ImageFormat.NV21, imageProxy.width, imageProxy.height, null)
        val out = ByteArrayOutputStream()
        yuvImage.compressToJpeg(Rect(0, 0, imageProxy.width, imageProxy.height), quality, out)
        val imageBytes = out.toByteArray()

        val rotation = imageProxy.imageInfo.rotationDegrees
        if (rotation == 0) return imageBytes

        // 센서 회전 각도 보정
        val bitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.size) ?: return null
        val matrix = Matrix().apply { postRotate(rotation.toFloat()) }
        val rotatedBitmap =
            Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)

        val rotatedOut = ByteArrayOutputStream()
        rotatedBitmap.compress(Bitmap.CompressFormat.JPEG, quality, rotatedOut)

        bitmap.recycle()
        rotatedBitmap.recycle()

        return rotatedOut.toByteArray()
    }
}