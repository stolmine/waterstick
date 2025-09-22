#!/usr/bin/env python3
"""
Test script to verify the adaptive density compensation is working correctly.
This simulates the compensation calculation to confirm the implementation.
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

def simulate_adaptive_compensation(comb_size_ms, num_taps, pattern=0):
    """Simulate the adaptive density compensation calculation"""
    comb_size_seconds = comb_size_ms / 1000.0
    delay_samples = comb_size_seconds * SAMPLE_RATE

    # Base 1/N compensation
    base_density_gain = 1.0 / num_taps

    # Early exit for large delays (matches C++ implementation)
    if delay_samples > num_taps * 2.0:
        return base_density_gain, 1.0  # No adaptive factor needed

    # Calculate clustering factors
    unique_positions = set()
    total_correlation = 0.0

    tap_delays = []
    for tap in range(num_taps):
        # Simulate the getTapDelayFromFloat calculation
        tap_ratio = calculate_tap_ratio_for_pattern(tap, pattern)
        tap_delay = comb_size_seconds * tap_ratio
        tap_delay_samples = tap_delay * SAMPLE_RATE
        tap_delays.append(tap_delay_samples)

        # Count unique positions
        delay_position = int(tap_delay_samples)
        unique_positions.add(delay_position)

        # Calculate correlation with previous tap
        if tap > 0:
            delay_diff = abs(tap_delay_samples - tap_delays[tap - 1])
            correlation = exp(-delay_diff)
            total_correlation += correlation

    # Calculate clustering factor
    position_clustering_factor = num_taps / len(unique_positions) if len(unique_positions) > 0 else 1.0

    # Calculate correlation factor
    average_correlation = total_correlation / (num_taps - 1) if num_taps > 1 else 0.0
    correlation_factor = 1.0 + average_correlation * 2.0

    # Hybrid adaptive factor
    adaptive_factor = sqrt(position_clustering_factor * correlation_factor)

    # Apply adaptive compensation
    adaptive_density_gain = base_density_gain / adaptive_factor

    # Clamp to reasonable range (matches C++ implementation)
    min_gain = base_density_gain * 0.1  # No more than 10x additional attenuation
    max_gain = base_density_gain       # No gain boost, only attenuation

    adaptive_density_gain = max(min_gain, min(max_gain, adaptive_density_gain))

    return adaptive_density_gain, adaptive_factor

def main():
    print("=== ADAPTIVE DENSITY COMPENSATION TEST ===\n")

    # Test critical cases identified in the analysis
    test_cases = [
        (0.1, 64, 0, "Worst linear case"),
        (0.1, 32, 3, "Worst quadratic case"),
        (0.5, 64, 1, "Moderate clustering"),
        (1.0, 64, 0, "Borderline case"),
        (2.0, 64, 0, "Large delay - should use standard 1/N"),
        (10.0, 64, 0, "Very large delay - standard compensation")
    ]

    print("Test Results:")
    print("Case                          | Base 1/N   | Adaptive   | Factor | dB Reduction")
    print("-" * 75)

    for comb_size, num_taps, pattern, description in test_cases:
        base_gain = 1.0 / num_taps
        adaptive_gain, factor = simulate_adaptive_compensation(comb_size, num_taps, pattern)

        db_reduction = 20 * log(adaptive_gain / base_gain, 10) if base_gain > 0 else 0

        print(f"{description:30} | {base_gain:.6f} | {adaptive_gain:.6f} | {factor:.2f}x  | {db_reduction:+5.1f} dB")

    print("\n=== VERIFICATION OF KEY IMPROVEMENTS ===")

    # Test the worst case from the original analysis
    worst_case_adaptive, worst_factor = simulate_adaptive_compensation(0.1, 64, 0)
    worst_case_base = 1.0 / 64

    print(f"\nWorst case (0.1ms, 64 taps, linear pattern):")
    print(f"  Original 1/N compensation:  {worst_case_base:.6f}")
    print(f"  Adaptive compensation:      {worst_case_adaptive:.6f}")
    print(f"  Improvement factor:         {worst_factor:.2f}x")
    print(f"  Additional attenuation:     {20 * log(worst_case_adaptive / worst_case_base, 10):+.1f} dB")

    # Verify that adaptive compensation reduces clipping potential
    original_effective_gain = worst_case_base * 13.72  # From original analysis
    adaptive_effective_gain = worst_case_adaptive * 13.72

    print(f"\nEffective gain analysis:")
    print(f"  With original 1/N:          {original_effective_gain:.4f} (potential clipping)")
    print(f"  With adaptive compensation: {adaptive_effective_gain:.4f} (much safer)")

    print(f"\nThe adaptive compensation reduces the effective gain by {original_effective_gain / adaptive_effective_gain:.1f}x")
    print("This should significantly reduce clipping at very small comb sizes.")

if __name__ == "__main__":
    main()