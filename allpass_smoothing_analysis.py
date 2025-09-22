#!/usr/bin/env python3
"""
Allpass Smoothing Time Constant Analysis
Comprehensive mathematical and perceptual analysis of parameter smoothing trade-offs
"""

import math

class AllpassSmoothingAnalyzer:
    def __init__(self, sample_rate=44100):
        self.sample_rate = sample_rate

    def time_constant_to_smoothing_coeff(self, time_constant_ms):
        """Convert time constant in ms to exponential smoothing coefficient"""
        time_constant_s = time_constant_ms / 1000.0
        return math.exp(-1.0 / (time_constant_s * self.sample_rate))

    def smoothing_coeff_to_time_constant(self, eta):
        """Convert smoothing coefficient back to time constant in ms"""
        if eta >= 1.0:
            return float('inf')
        time_constant_s = -1.0 / (self.sample_rate * math.log(eta))
        return time_constant_s * 1000.0

    def calculate_response_time(self, time_constant_ms, threshold_percent=1.0):
        """Calculate 99% or other threshold response time"""
        # For exponential decay: y(t) = (1 - e^(-t/τ))
        # Solve for t when y(t) = threshold_percent/100
        settling_factor = -math.log(1.0 - threshold_percent/100.0)
        return time_constant_ms * settling_factor

    def allpass_frequency_response(self, eta, frequency):
        """Calculate allpass filter frequency response at a single frequency"""
        # Allpass coefficient: a = (1-η)/(1+η)
        a = (1.0 - eta) / (1.0 + eta)

        # H(z) = (a + z^-1) / (1 + a*z^-1)
        # Convert to frequency domain
        omega = 2 * math.pi * frequency / self.sample_rate

        # z = e^(jω)
        z_real = math.cos(omega)
        z_imag = math.sin(omega)

        # z^-1 = e^(-jω)
        z_inv_real = math.cos(-omega)
        z_inv_imag = math.sin(-omega)

        # numerator = a + z^-1
        num_real = a + z_inv_real
        num_imag = z_inv_imag

        # denominator = 1 + a*z^-1
        den_real = 1 + a * z_inv_real
        den_imag = a * z_inv_imag

        # H = numerator / denominator (complex division)
        den_mag_sq = den_real * den_real + den_imag * den_imag
        h_real = (num_real * den_real + num_imag * den_imag) / den_mag_sq
        h_imag = (num_imag * den_real - num_real * den_imag) / den_mag_sq

        magnitude = math.sqrt(h_real * h_real + h_imag * h_imag)
        phase = math.atan2(h_imag, h_real)

        return magnitude, phase

    def zipper_noise_threshold_analysis(self, modulation_rates_hz, time_constants_ms):
        """Analyze at what time constants zipper noise becomes audible"""
        results = {}

        for mod_rate in modulation_rates_hz:
            results[mod_rate] = {}

            # Nyquist criterion: smoothing frequency should be >> modulation rate
            # Rule of thumb: smoothing cutoff should be 10x higher than modulation rate
            min_smoothing_freq = mod_rate * 10  # Hz

            for tc_ms in time_constants_ms:
                eta = self.time_constant_to_smoothing_coeff(tc_ms)

                # Calculate effective cutoff frequency of smoothing filter
                # For one-pole LP: fc = fs / (2π * time_constant_samples)
                time_constant_samples = tc_ms * self.sample_rate / 1000.0
                cutoff_freq = self.sample_rate / (2 * math.pi * time_constant_samples)

                # Check if smoothing is adequate
                smoothing_ratio = cutoff_freq / mod_rate
                adequate_smoothing = smoothing_ratio >= 10

                results[mod_rate][tc_ms] = {
                    'cutoff_freq': cutoff_freq,
                    'smoothing_ratio': smoothing_ratio,
                    'adequate': adequate_smoothing,
                    'eta': eta
                }

        return results

    def parameter_tracking_accuracy(self, time_constants_ms, step_sizes):
        """Analyze parameter tracking accuracy for different step sizes"""
        results = {}

        for tc_ms in time_constants_ms:
            eta = self.time_constant_to_smoothing_coeff(tc_ms)
            results[tc_ms] = {}

            for step_size in step_sizes:
                # Calculate steady-state error for step input
                # For exponential smoothing: final_value = step_size (no steady-state error)
                # But analyze transient behavior

                # 99% settling time
                settling_time_99 = self.calculate_response_time(tc_ms, 99.0)

                # 90% settling time (practical responsiveness)
                settling_time_90 = self.calculate_response_time(tc_ms, 90.0)

                # Maximum slope during transition (derivative at t=0)
                initial_slope = step_size / (tc_ms / 1000.0)  # per second

                results[tc_ms][step_size] = {
                    'settling_99': settling_time_99,
                    'settling_90': settling_time_90,
                    'initial_slope': initial_slope,
                    'eta': eta
                }

        return results

    def perceptual_threshold_analysis(self):
        """Analyze perceptual thresholds for smooth vs responsive changes"""

        # Research-based thresholds from audio literature
        thresholds = {
            'just_noticeable_difference': {
                'amplitude': 0.5,  # dB
                'frequency': 0.3,  # % for mid frequencies
                'time': 10.0       # ms for temporal resolution
            },
            'zipper_noise_threshold': {
                'min_smoothing_time': 5.0,   # ms - below this, zipper noise likely
                'safe_smoothing_time': 10.0, # ms - generally safe
                'slow_automation': 50.0      # ms - for slow parameter changes
            },
            'real_time_control': {
                'max_latency': 20.0,         # ms - maximum acceptable latency
                'preferred_latency': 10.0,   # ms - preferred maximum
                'responsive_threshold': 5.0   # ms - feels "immediate"
            },
            'modulation_rates': {
                'lfo_typical': [0.1, 10.0],     # Hz range for typical LFOs
                'audio_rate': [20.0, 20000.0],  # Hz audio range
                'control_rate': [1.0, 100.0]    # Hz typical control rates
            }
        }

        return thresholds

def generate_comprehensive_analysis():
    """Generate comprehensive analysis of allpass smoothing trade-offs"""

    analyzer = AllpassSmoothingAnalyzer()

    # Time constants to analyze (ms)
    time_constants = [0.5, 1.0, 2.0, 3.0, 5.0, 7.5, 10.0, 15.0, 20.0, 30.0, 50.0]

    print("=== ALLPASS SMOOTHING TIME CONSTANT ANALYSIS ===\n")

    # 1. Time constant to response time relationship
    print("1. TIME CONSTANT TO RESPONSE TIME RELATIONSHIP")
    print("Time Constant (ms) | Smoothing Coeff | 90% Response | 99% Response | Cutoff Freq (Hz)")
    print("-" * 85)

    for tc in time_constants:
        eta = analyzer.time_constant_to_smoothing_coeff(tc)
        response_90 = analyzer.calculate_response_time(tc, 90.0)
        response_99 = analyzer.calculate_response_time(tc, 99.0)

        # Calculate equivalent cutoff frequency
        time_constant_samples = tc * analyzer.sample_rate / 1000.0
        cutoff_freq = analyzer.sample_rate / (2 * math.pi * time_constant_samples)

        print(f"{tc:14.1f} | {eta:14.6f} | {response_90:11.1f} | {response_99:11.1f} | {cutoff_freq:12.1f}")

    # 2. Zipper noise analysis
    print("\n2. ZIPPER NOISE THRESHOLD ANALYSIS")
    modulation_rates = [0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0]  # Hz

    zipper_results = analyzer.zipper_noise_threshold_analysis(modulation_rates, time_constants)

    print("Modulation Rate (Hz) | Time Constant (ms) | Smoothing Ratio | Adequate?")
    print("-" * 70)

    for mod_rate in modulation_rates:
        for tc in [1.0, 3.0, 5.0, 10.0, 20.0]:  # Subset for readability
            if tc in zipper_results[mod_rate]:
                result = zipper_results[mod_rate][tc]
                adequate = "YES" if result['adequate'] else "NO"
                print(f"{mod_rate:16.1f} | {tc:18.1f} | {result['smoothing_ratio']:14.1f} | {adequate}")

    # 3. Frequency response analysis
    print("\n3. FREQUENCY RESPONSE ANALYSIS")

    print("Time Constant (ms) | DC Gain (dB) | 1kHz Phase (deg) | Allpass Coeff")
    print("-" * 70)

    for tc in [1.0, 5.0, 10.0, 20.0]:
        eta = analyzer.time_constant_to_smoothing_coeff(tc)

        # DC response (0 Hz)
        dc_magnitude, dc_phase = analyzer.allpass_frequency_response(eta, 0.01)  # Near DC
        dc_gain_db = 20 * math.log10(dc_magnitude)

        # 1kHz response
        magnitude_1k, phase_1k = analyzer.allpass_frequency_response(eta, 1000.0)
        phase_1k_deg = math.degrees(phase_1k)

        # Allpass coefficient
        allpass_coeff = (1.0 - eta) / (1.0 + eta)

        print(f"{tc:14.1f} | {dc_gain_db:11.3f} | {phase_1k_deg:14.1f} | {allpass_coeff:12.6f}")

    # 4. Parameter tracking accuracy
    print("\n4. PARAMETER TRACKING ACCURACY")
    step_sizes = [0.01, 0.1, 0.5, 1.0]  # Normalized parameter changes

    tracking_results = analyzer.parameter_tracking_accuracy([1.0, 5.0, 10.0, 20.0], step_sizes)

    print("Time Constant (ms) | Step Size | 90% Settle (ms) | 99% Settle (ms) | Initial Slope (/s)")
    print("-" * 85)

    for tc in [1.0, 5.0, 10.0, 20.0]:
        for step in [0.1, 1.0]:  # Subset for readability
            result = tracking_results[tc][step]
            print(f"{tc:14.1f} | {step:8.1f} | {result['settling_90']:14.1f} | {result['settling_99']:14.1f} | {result['initial_slope']:15.1f}")

    # 5. Perceptual thresholds
    print("\n5. PERCEPTUAL THRESHOLD GUIDELINES")
    thresholds = analyzer.perceptual_threshold_analysis()

    print("Application Category | Recommended Time Constant | Rationale")
    print("-" * 65)
    print("Real-time Control   | 1-3 ms                   | Immediate response feel")
    print("Fast Modulation     | 3-5 ms                   | Avoid zipper noise")
    print("Musical Automation  | 5-10 ms                  | Smooth, musical feel")
    print("Slow Parameter      | 10-50 ms                 | Ultra-smooth changes")
    print("Safety/Fallback     | 10 ms                    | Conservative default")

    # 6. Optimization recommendations
    print("\n6. OPTIMIZATION RECOMMENDATIONS")
    print("=" * 50)

    optimal_configs = [
        {
            'use_case': 'Real-time knob control',
            'time_constant': 3.0,
            'rationale': 'Balance of responsiveness and smoothness'
        },
        {
            'use_case': 'Automation (slow)',
            'time_constant': 10.0,
            'rationale': 'Current default - proven smooth for automation'
        },
        {
            'use_case': 'Audio-rate modulation',
            'time_constant': 1.0,
            'rationale': 'Fast enough to track audio-rate changes'
        },
        {
            'use_case': 'LFO modulation (0.1-10 Hz)',
            'time_constant': 5.0,
            'rationale': 'Good compromise for musical modulation rates'
        },
        {
            'use_case': 'Safety/conservative',
            'time_constant': 10.0,
            'rationale': 'Current implementation - safe default'
        }
    ]

    for config in optimal_configs:
        eta = analyzer.time_constant_to_smoothing_coeff(config['time_constant'])
        response_90 = analyzer.calculate_response_time(config['time_constant'], 90.0)

        print(f"\nUse Case: {config['use_case']}")
        print(f"  Time Constant: {config['time_constant']:.1f} ms")
        print(f"  Smoothing Coeff: {eta:.6f}")
        print(f"  90% Response: {response_90:.1f} ms")
        print(f"  Rationale: {config['rationale']}")

    print("\n" + "=" * 70)
    print("SUMMARY: For optimal balance of responsiveness and smoothness,")
    print("consider reducing time constant from 10ms to 3-5ms for real-time control")
    print("while maintaining 10ms for automation to preserve musical character.")
    print("=" * 70)

if __name__ == "__main__":
    generate_comprehensive_analysis()