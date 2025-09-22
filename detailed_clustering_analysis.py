#!/usr/bin/env python3
"""
Detailed analysis of tap clustering and proposed solutions for the comb processor.
This script provides specific mathematical formulations for fixing the 1/N compensation.
"""

import numpy as np
from math import log, exp, sqrt, pow

# Constants from the codebase
MAX_TAPS = 64
SAMPLE_RATE = 44100.0

def calculate_tap_ratio_for_pattern(tap_index, pattern):
    """Calculate tap ratio for a specific pattern (based on CombProcessor.cpp)"""
    if pattern == 0:
        return (tap_index + 1) / MAX_TAPS
    elif pattern == 1:
        return log(tap_index + 1.0) / log(MAX_TAPS)
    elif pattern == 2:
        return (exp((tap_index + 1) / MAX_TAPS) - 1.0) / (exp(1.0) - 1.0)
    elif pattern == 3:
        return pow((tap_index + 1) / MAX_TAPS, 2.0)
    elif pattern == 4:
        return sqrt((tap_index + 1) / MAX_TAPS)
    else:
        normalized_index = (tap_index + 1) / MAX_TAPS
        pattern_offset = (pattern - 5) / 10.0
        return pow(normalized_index, 1.0 + pattern_offset)

def analyze_clustering_detailed(comb_size_ms, num_taps, pattern=0):
    """Detailed clustering analysis with mathematical precision"""
    comb_size_seconds = comb_size_ms / 1000.0
    delay_samples = comb_size_seconds * SAMPLE_RATE

    # Calculate exact delays for each tap
    tap_delays = []
    for tap in range(num_taps):
        tap_ratio = calculate_tap_ratio_for_pattern(tap, pattern)
        actual_delay = delay_samples * tap_ratio
        tap_delays.append(actual_delay)

    # Group taps by integer sample position
    position_groups = {}
    for tap_idx, delay in enumerate(tap_delays):
        pos = int(delay)
        if pos not in position_groups:
            position_groups[pos] = []
        position_groups[pos].append({
            'tap_index': tap_idx,
            'delay': delay,
            'fraction': delay - pos
        })

    # Calculate correlation matrix between all taps
    correlation_matrix = np.zeros((num_taps, num_taps))
    for i in range(num_taps):
        for j in range(num_taps):
            delay_diff = abs(tap_delays[i] - tap_delays[j])
            # Correlation decreases exponentially with delay difference
            correlation_matrix[i][j] = exp(-delay_diff)

    # Calculate effective gain multiplication factor
    # This accounts for both integer position clustering and fractional correlation
    total_effective_gain = 0.0

    for pos, group in position_groups.items():
        if len(group) == 1:
            # Single tap at this position - no clustering
            total_effective_gain += 1.0
        else:
            # Multiple taps at same integer position
            # Calculate their fractional correlation
            group_correlation = 0.0
            for i in range(len(group)):
                for j in range(len(group)):
                    frac_diff = abs(group[i]['fraction'] - group[j]['fraction'])
                    # Fractional correlation for interpolation
                    frac_corr = exp(-frac_diff * 10.0)  # More sensitive to fractional differences
                    group_correlation += frac_corr

            # Average correlation within this position group
            avg_correlation = group_correlation / (len(group) * len(group))
            effective_taps = len(group) * avg_correlation
            total_effective_gain += effective_taps

    gain_multiplication = total_effective_gain / num_taps

    return {
        'comb_size_ms': comb_size_ms,
        'num_taps': num_taps,
        'pattern': pattern,
        'delay_samples': delay_samples,
        'tap_delays': tap_delays,
        'position_groups': position_groups,
        'correlation_matrix': correlation_matrix,
        'gain_multiplication': gain_multiplication,
        'total_effective_gain': total_effective_gain
    }

def calculate_adaptive_compensation(analysis):
    """Calculate adaptive density compensation factor"""
    num_taps = analysis['num_taps']
    gain_mult = analysis['gain_multiplication']

    # Base 1/N compensation
    base_compensation = 1.0 / num_taps

    # Correlation-aware compensation
    correlation_compensation = base_compensation / sqrt(gain_mult)

    # Position-clustering compensation
    unique_positions = len(analysis['position_groups'])
    clustering_factor = num_taps / unique_positions if unique_positions > 0 else 1.0
    clustering_compensation = base_compensation / clustering_factor

    # Hybrid approach - geometric mean of correlation and clustering compensation
    hybrid_compensation = sqrt(correlation_compensation * clustering_compensation)

    return {
        'base_compensation': base_compensation,
        'correlation_compensation': correlation_compensation,
        'clustering_compensation': clustering_compensation,
        'hybrid_compensation': hybrid_compensation,
        'clustering_factor': clustering_factor
    }

def recommend_max_taps(comb_size_ms, min_spacing_samples=1.0):
    """Recommend maximum tap count for a given comb size to avoid severe clustering"""
    comb_size_seconds = comb_size_ms / 1000.0
    delay_samples = comb_size_seconds * SAMPLE_RATE

    # Simple approach: ensure at least min_spacing between taps
    max_taps_simple = int(delay_samples / min_spacing_samples)

    # More sophisticated: ensure reasonable distribution
    # Allow some clustering but prevent extreme cases
    max_safe_clustering = 3.0  # Allow up to 3x gain multiplication

    for test_taps in range(1, MAX_TAPS + 1):
        analysis = analyze_clustering_detailed(comb_size_ms, test_taps, 0)  # Test linear pattern
        if analysis['gain_multiplication'] > max_safe_clustering:
            return test_taps - 1

    return MAX_TAPS

def main():
    print("=== DETAILED CLUSTERING ANALYSIS AND SOLUTIONS ===\n")

    # Analyze critical cases
    critical_cases = [
        (0.1, 64, 0),  # Worst linear case
        (0.1, 32, 3),  # Worst quadratic case
        (0.5, 64, 1),  # Medium case with clustering
        (1.0, 64, 0),  # Borderline case
        (2.0, 64, 0),  # Mostly safe case
    ]

    print("=== CRITICAL CASE ANALYSIS ===")
    for comb_size, num_taps, pattern in critical_cases:
        print(f"\nCase: {comb_size}ms, {num_taps} taps, pattern {pattern}")

        analysis = analyze_clustering_detailed(comb_size, num_taps, pattern)
        compensation = calculate_adaptive_compensation(analysis)

        print(f"  Delay samples: {analysis['delay_samples']:.3f}")
        print(f"  Unique positions: {len(analysis['position_groups'])}")
        print(f"  Gain multiplication: {analysis['gain_multiplication']:.2f}x")
        print(f"  Position clustering factor: {compensation['clustering_factor']:.2f}x")

        print(f"  Compensation factors:")
        print(f"    Base (1/N): {compensation['base_compensation']:.6f}")
        print(f"    Correlation-aware: {compensation['correlation_compensation']:.6f}")
        print(f"    Clustering-aware: {compensation['clustering_compensation']:.6f}")
        print(f"    Hybrid (recommended): {compensation['hybrid_compensation']:.6f}")

        # Calculate dB reduction
        db_reduction = 20 * log(compensation['hybrid_compensation'] / compensation['base_compensation'], 10)
        print(f"    Additional attenuation: {db_reduction:.1f} dB")

    print("\n=== RECOMMENDED MAXIMUM TAP COUNTS ===")
    test_sizes = [0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0]

    for size in test_sizes:
        max_taps_safe = recommend_max_taps(size, 1.0)
        max_taps_conservative = recommend_max_taps(size, 2.0)

        print(f"  {size:4.1f}ms: {max_taps_safe:2d} taps (safe), {max_taps_conservative:2d} taps (conservative)")

    print("\n=== PROPOSED IMPLEMENTATION STRATEGIES ===")

    print("\n1. ADAPTIVE DENSITY COMPENSATION:")
    print("   Replace the current 1/N factor with hybrid compensation:")
    print("   float densityGain = 1.0f / (activeTapCount * adaptiveClusteringFactor);")
    print("   where adaptiveClusteringFactor = sqrt(correlationFactor * positionClusteringFactor)")

    print("\n2. CLUSTERING FACTOR CALCULATION:")
    print("   // Count unique integer delay positions")
    print("   std::set<int> uniquePositions;")
    print("   for (int tap = 0; tap < activeTapCount; ++tap) {")
    print("       float delaySamples = getTapDelay(tap) * mSampleRate;")
    print("       uniquePositions.insert(static_cast<int>(delaySamples));")
    print("   }")
    print("   float clusteringFactor = static_cast<float>(activeTapCount) / uniquePositions.size();")

    print("\n3. CORRELATION FACTOR CALCULATION:")
    print("   // Calculate average correlation between adjacent taps")
    print("   float totalCorrelation = 0.0f;")
    print("   for (int i = 1; i < activeTapCount; ++i) {")
    print("       float delayDiff = abs(getTapDelay(i) - getTapDelay(i-1)) * mSampleRate;")
    print("       totalCorrelation += exp(-delayDiff);")
    print("   }")
    print("   float avgCorrelation = totalCorrelation / (activeTapCount - 1);")
    print("   float correlationFactor = 1.0f + avgCorrelation * 2.0f; // Scale correlation impact")

    print("\n4. ADAPTIVE TAP LIMITING:")
    print("   // Optionally limit tap count for very short delays")
    print("   float delaySamples = getSyncedCombSize() * mSampleRate;")
    print("   int maxSafeTaps = static_cast<int>(delaySamples / 1.0f); // 1 sample min spacing")
    print("   int effectiveTapCount = std::min(requestedTapCount, maxSafeTaps);")

    print("\n5. IMPLEMENTATION PRIORITY:")
    print("   - HIGH: Implement adaptive density compensation (strategies 1-3)")
    print("   - MEDIUM: Add tap limiting for extreme cases (strategy 4)")
    print("   - LOW: Add user control for clustering behavior")

    # Generate specific numbers for the worst case
    worst_case = analyze_clustering_detailed(0.1, 64, 0)
    worst_compensation = calculate_adaptive_compensation(worst_case)

    print(f"\n=== SPECIFIC FIX FOR 0.1ms/64-tap CASE ===")
    print(f"Current 1/N factor: {worst_compensation['base_compensation']:.6f}")
    print(f"Recommended factor: {worst_compensation['hybrid_compensation']:.6f}")
    print(f"Ratio: {worst_compensation['hybrid_compensation'] / worst_compensation['base_compensation']:.3f}x")
    print(f"This provides {20 * log(worst_compensation['base_compensation'] / worst_compensation['hybrid_compensation'], 10):.1f} dB additional attenuation")

if __name__ == "__main__":
    main()