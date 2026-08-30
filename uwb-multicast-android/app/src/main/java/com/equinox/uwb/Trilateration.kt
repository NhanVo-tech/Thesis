package com.equinox.uwb

import android.util.Log
import kotlin.math.abs
import kotlin.math.sqrt

fun estimateTagPosition(samples: List<AnchorSample>): Pair<Double, Double> {
    if (samples.size < 2) {
        Log.w("UWB", "Need at least 2 anchor samples for trilateration (got ${samples.size})")
        return Pair(Double.NaN, Double.NaN)
    }

    // For 2 anchors, use geometric approach
    if (samples.size == 2) {
        return estimateWith2Anchors(samples[0], samples[1])
    }

    // For 3+ anchors, use Gauss Newton iterative approach
    return estimateWithGaussNewton(samples)
}

private fun estimateWith2Anchors(sample1: AnchorSample, sample2: AnchorSample): Pair<Double, Double> {
    val dx = sample2.x - sample1.x
    val dy = sample2.y - sample1.y
    val d = sqrt(dx * dx + dy * dy)

    // Check if circles intersect
    if (d > sample1.d + sample2.d || d < abs(sample1.d - sample2.d)) {
        // No intersection or one circle inside the other - return midpoint of closest approach
        val t = sample1.d / (sample1.d + sample2.d)
        return Pair(sample1.x + t * dx, sample1.y + t * dy)
    }

    // Calculate intersection points
    val a = (sample1.d * sample1.d - sample2.d * sample2.d + d * d) / (2 * d)
    val h = sqrt(sample1.d * sample1.d - a * a)

    val px = sample1.x + a * dx / d
    val py = sample1.y + a * dy / d

    // Return one of the intersection points (TODO: use more context, like last known position, to choose which one)
    return Pair(px + h * (-dy) / d, py + h * dx / d)
}

// For 3+ anchors, formulate it as a least squares problem and solve with Gauss-Newton
private fun estimateWithGaussNewton(samples: List<AnchorSample>): Pair<Double, Double> {
    // Initial guess: Arithmetic mean, weighted by inverse of distance to bias toward closer anchors
    var x = 0.0
    var y = 0.0
    var totalWeight = 0.0
    for (sample in samples) {
        val weight = 1.0 / (sample.d + 1e-6) // small epsilon to avoid division by zero
        x += sample.x * weight
        y += sample.y * weight
        totalWeight += weight
    }
    x /= totalWeight
    y /= totalWeight

    // Gauss-Newton iterations to refine the solution
    for (iteration in 0 until 10) { // Usually converges in ~5 iterations
        var sumXX = 0.0
        var sumXY = 0.0
        var sumYY = 0.0
        var sumX = 0.0
        var sumY = 0.0

        for (sample in samples) {
            val dx = x - sample.x
            val dy = y - sample.y
            val currentDist = sqrt(dx * dx + dy * dy)

            if (currentDist < 1e-6) continue // Skip if too close to avoid numerical issues

            val error = currentDist - sample.d
            val invDist = 1.0 / currentDist

            // Jacobian elements: partial derivatives of distance w.r.t. x and y
            val jx = dx * invDist
            val jy = dy * invDist

            // Build normal equations: J^T * J * delta = -J^T * error
            sumXX += jx * jx
            sumXY += jx * jy
            sumYY += jy * jy
            sumX += jx * error
            sumY += jy * error
        }

        // Solve 2x2 system for the update step
        val det = sumXX * sumYY - sumXY * sumXY

        if (abs(det) < 1e-12) {
            break // Singular, stop iterating
        }

        val deltaX = -(sumX * sumYY - sumY * sumXY) / det
        val deltaY = -(sumY * sumXX - sumX * sumXY) / det

        x += deltaX
        y += deltaY

        // Check for convergence
        if (abs(deltaX) < 1e-6 && abs(deltaY) < 1e-6) {
            break
        }
    }

    return x to y
}