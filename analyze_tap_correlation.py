#!/usr/bin/env python3
"""
Analyze tap correlation issues in comb processor at very low comb sizes.
This script investigates why 1/N density compensation isn't sufficient when
many taps are reading from nearly identical positions in the delay buffer.
"""

import numpy as np
from math import log, exp, sqrt, pow

# Constants from the codebase
MAX_TAPS = 64
SAMPLE_RATE = 44100.0
kNumCombPatterns = 16

def calculate_tap_ratio_for_pattern(tap_index, pattern):
    """Calculate tap ratio for a specific pattern (based on CombProcessor.cpp)"""
    if pattern == 0:
        # Linear pattern
        return (tap_index + 1) / MAX_TAPS
    elif pattern == 1:
        # Logarithmic pattern
        return log(tap_index + 1.0) / log(MAX_TAPS)
    elif pattern == 2:
        # Exponential pattern
        return (exp((tap_index + 1) / MAX_TAPS) - 1.0) / (exp(1.0) - 1.0)
    elif pattern == 3:
        # Quadratic pattern
        return pow((tap_index + 1) / MAX_TAPS, 2.0)
    elif pattern == 4:
        # Square root pattern
        return sqrt((tap_index + 1) / MAX_TAPS)
    else:
        # Parameterized power patterns (5-15)
        normalized_index = (tap_index + 1) / MAX_TAPS
        pattern_offset = (pattern - 5) / 10.0
        return pow(normalized_index, 1.0 + pattern_offset)

def analyze_tap_clustering(comb_size_ms, num_taps, pattern=0):
    """Analyze how taps cluster at small comb sizes"""
    comb_size_seconds = comb_size_ms / 1000.0
    delay_samples = comb_size_seconds * SAMPLE_RATE

    # Calculate actual delays for each tap
    tap_delays = []
    for tap in range(num_taps):
        tap_ratio = calculate_tap_ratio_for_pattern(tap, pattern)
        actual_delay = delay_samples * tap_ratio
        tap_delays.append(actual_delay)

    # Convert to integer sample positions
    tap_positions = [int(delay) for delay in tap_delays]
    tap_fractions = [delay - int(delay) for delay in tap_delays]

    # Count how many taps read from each buffer position
    position_counts = {}
    for pos in tap_positions:
        position_counts[pos] = position_counts.get(pos, 0) + 1

    # Calculate clustering metrics
    unique_positions = len(position_counts)
    max_taps_per_position = max(position_counts.values()) if position_counts else 0
    avg_taps_per_position = num_taps / unique_positions if unique_positions > 0 else float('inf')

    # Calculate correlation between adjacent taps
    correlations = []
    for i in range(1, len(tap_delays)):
        delay_diff = abs(tap_delays[i] - tap_delays[i-1])
        # Correlation is high when delay difference is small
        correlation = exp(-delay_diff)  # Exponential decay with distance
        correlations.append(correlation)

    avg_correlation = np.mean(correlations) if correlations else 0

    return {
        'comb_size_ms': comb_size_ms,
        'num_taps': num_taps,
        'pattern': pattern,
        'delay_samples': delay_samples,
        'tap_delays': tap_delays,
        'tap_positions': tap_positions,
        'tap_fractions': tap_fractions,
        'unique_positions': unique_positions,
        'max_taps_per_position': max_taps_per_position,
        'avg_taps_per_position': avg_taps_per_position,
        'position_counts': position_counts,
        'avg_correlation': avg_correlation,
        'correlations': correlations
    }

def calculate_effective_gain_multiplication(analysis):
    """Calculate how tap clustering affects effective gain"""
    # When multiple taps read from same position, they effectively multiply the signal
    total_gain_factor = 0
    for position, count in analysis['position_counts'].items():
        # Each position contributes its count squared (due to correlation)
        total_gain_factor += count * count

    # Compare to ideal case where each tap reads from unique position
    ideal_gain_factor = analysis['num_taps']

    gain_multiplication = total_gain_factor / ideal_gain_factor
    return gain_multiplication

def analyze_interpolation_artifacts(analysis):
    """Analyze how interpolation between clustered samples affects gain"""
    # When taps have very similar fractional delays, interpolation can create constructive interference
    artifacts = []

    for i in range(len(analysis['tap_fractions']) - 1):
        frac1 = analysis['tap_fractions'][i]
        frac2 = analysis['tap_fractions'][i + 1]

        # Calculate interpolation weights for both taps
        weight1_a = 1.0 - frac1  # Weight for integer sample
        weight1_b = frac1        # Weight for next sample
        weight2_a = 1.0 - frac2
        weight2_b = frac2

        # If taps read from same integer positions, calculate interference
        if analysis['tap_positions'][i] == analysis['tap_positions'][i + 1]:
            # Constructive interference when weights are similar
            weight_similarity = 1.0 - abs((weight1_a - weight2_a) + (weight1_b - weight2_b)) / 2.0
            artifacts.append(weight_similarity)

    avg_artifact = np.mean(artifacts) if artifacts else 0
    return avg_artifact

def main():
    print("=== TAP CORRELATION ANALYSIS FOR LOW COMB SIZES ===\n")

    # Test parameters
    comb_sizes_ms = [0.1, 0.5, 1.0, 2.0, 5.0, 10.0]
    tap_counts = [16, 32, 48, 64]
    patterns_to_test = [0, 1, 2, 3, 4]  # Linear, log, exp, quad, sqrt

    results = []

    for comb_size in comb_sizes_ms:
        for num_taps in tap_counts:
            for pattern in patterns_to_test:
                analysis = analyze_tap_clustering(comb_size, num_taps, pattern)
                gain_mult = calculate_effective_gain_multiplication(analysis)
                interp_artifacts = analyze_interpolation_artifacts(analysis)

                analysis['gain_multiplication'] = gain_mult
                analysis['interpolation_artifacts'] = interp_artifacts
                results.append(analysis)

                # Report severe clustering cases
                if gain_mult > 2.0 or analysis['avg_correlation'] > 0.8:
                    print(f"SEVERE CLUSTERING DETECTED:")
                    print(f"  Comb Size: {comb_size}ms, Taps: {num_taps}, Pattern: {pattern}")
                    print(f"  Delay samples: {analysis['delay_samples']:.3f}")
                    print(f"  Unique positions: {analysis['unique_positions']}/{num_taps}")
                    print(f"  Max taps per position: {analysis['max_taps_per_position']}")
                    print(f"  Gain multiplication: {gain_mult:.2f}x")
                    print(f"  Avg correlation: {analysis['avg_correlation']:.3f}")
                    print(f"  Interpolation artifacts: {interp_artifacts:.3f}")
                    print()

    # Find worst cases
    worst_gain = max(results, key=lambda x: x['gain_multiplication'])
    worst_correlation = max(results, key=lambda x: x['avg_correlation'])

    print("=== WORST CASE SCENARIOS ===")
    print(f"Highest gain multiplication: {worst_gain['gain_multiplication']:.2f}x")
    print(f"  Size: {worst_gain['comb_size_ms']}ms, Taps: {worst_gain['num_taps']}, Pattern: {worst_gain['pattern']}")
    print(f"  {worst_gain['unique_positions']} unique positions for {worst_gain['num_taps']} taps")
    print()

    print(f"Highest correlation: {worst_correlation['avg_correlation']:.3f}")
    print(f"  Size: {worst_correlation['comb_size_ms']}ms, Taps: {worst_correlation['num_taps']}, Pattern: {worst_correlation['pattern']}")
    print()

    # Analyze specific problematic case: 0.1ms with 64 taps
    print("=== DETAILED ANALYSIS: 0.1ms with 64 taps ===")
    problem_case = analyze_tap_clustering(0.1, 64, 0)  # Linear pattern
    gain_mult = calculate_effective_gain_multiplication(problem_case)
    interp_artifacts = analyze_interpolation_artifacts(problem_case)

    print(f"Total delay: {problem_case['delay_samples']:.3f} samples")
    print(f"Tap positions (first 10): {problem_case['tap_positions'][:10]}")
    print(f"Tap fractions (first 10): [" + ", ".join(f"{f:.3f}" for f in problem_case['tap_fractions'][:10]) + "]")
    print(f"Position distribution: {problem_case['position_counts']}")
    print()

    # Calculate required compensation
    baseline_gain = 1.0 / 64  # Standard 1/N compensation
    actual_gain_mult = gain_mult
    required_compensation = baseline_gain / actual_gain_mult

    print(f"Standard 1/N compensation: {baseline_gain:.6f}")
    print(f"Actual gain multiplication: {actual_gain_mult:.2f}x")
    print(f"Required compensation factor: {required_compensation:.6f}")
    print(f"Additional attenuation needed: {1.0/actual_gain_mult:.3f}x")
    print()

    # Suggest solutions
    print("=== PROPOSED SOLUTIONS ===")
    print("1. CORRELATION-AWARE DENSITY COMPENSATION:")
    print(f"   Instead of 1/N, use 1/(N * sqrt(correlation_factor))")
    print(f"   For this case: 1/(64 * sqrt({actual_gain_mult:.2f})) = {required_compensation:.6f}")
    print()

    print("2. MINIMUM DELAY SPACING:")
    print("   Enforce minimum 1-sample spacing between taps")
    print("   This would limit max taps at very short delays")
    print()

    print("3. ADAPTIVE TAP COUNT:")
    print("   Automatically reduce tap count when delay < (num_taps * min_spacing)")
    max_taps_for_0_1ms = int(problem_case['delay_samples'])
    print(f"   For 0.1ms delay: limit to ~{max_taps_for_0_1ms} taps maximum")
    print()

    print("4. DECORRELATION FILTERING:")
    print("   Apply slight frequency-dependent phase shifts to reduce correlation")
    print("   Trade some accuracy for reduced clustering artifacts")

if __name__ == "__main__":
    main()