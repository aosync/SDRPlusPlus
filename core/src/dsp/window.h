#pragma once
#include <dsp/block.h>
#include <dsp/types.h>
#include <dsp/utils/window_functions.h>

namespace dsp {
    namespace filter_window {
        class generic_window {
        public:
            virtual int getTapCount() { return -1; }
            virtual void createTaps(float* taps, int tapCount, float factor = 1.0f) {}
        };

        class BlackmanWindow : public filter_window::generic_window {
        public:
            BlackmanWindow() {}
            BlackmanWindow(float cutoff, float transWidth, float sampleRate) { init(cutoff, transWidth, sampleRate); }

            void init(float cutoff, float transWidth, float sampleRate) {
                _cutoff = cutoff;
                _transWidth = transWidth;
                _sampleRate = sampleRate;
            }

            void setSampleRate(float sampleRate) {
                _sampleRate = sampleRate;
            }

            void setCutoff(float cutoff) {
                _cutoff = cutoff;
            }
            
            void setTransWidth(float transWidth) {
                _transWidth = transWidth;
            }

            int getTapCount() {
                float fc = _cutoff / _sampleRate;
                if (fc > 1.0f) {
                    fc = 1.0f;
                }

                int _M = 4.0f / (_transWidth / _sampleRate);
                if (_M < 4) {
                    _M = 4;
                }

                if (_M % 2 == 0) { _M++; }

                return _M;
            }

            void createTaps(float* taps, int tapCount, float factor = 1.0f) {
                // Calculate cuttoff frequency
                float omega = 2.0f * FL_M_PI * (_cutoff / _sampleRate);
                if (omega > FL_M_PI) { omega = FL_M_PI; }

                // Generate taps
                float val;
                float sum = 0.0f;
                float tc = tapCount;
                for (int i = 0; i < tapCount; i++) {
                    val = math::sinc(omega, (float)i - (tc/2), FL_M_PI) * window_function::blackman(i, tc - 1);
                    taps[i] = val;
                    sum += val;
                }

                // Normalize taps and multiply by supplied factor
                for (int i = 0; i < tapCount; i++) {
                    taps[i] *= factor;
                    taps[i] /= sum;
                }
            }

        private:
            float _cutoff, _transWidth, _sampleRate;

        };
    }

    class RRCTaps : public filter_window::generic_window {
        public:
            RRCTaps() {}
            RRCTaps(int tapCount, float sampleRate, float baudRate, float alpha) { init(tapCount, sampleRate, baudRate, alpha); }

            void init(int tapCount, float sampleRate, float baudRate, float alpha) {
                _tapCount = tapCount;
                _sampleRate = sampleRate;
                _baudRate = baudRate;
                _alpha = alpha;
            }

            int getTapCount() {
                return _tapCount;
            }

            void setSampleRate(float sampleRate) {
                _sampleRate = sampleRate;
            }

            void setBaudRate(float baudRate) {
                _baudRate = baudRate;
            }

            void setTapCount(int count) {
                _tapCount = count;
            }

            void setAlpha(float alpha) {
                _alpha = alpha;
            }

            void createTaps(float* taps, int tapCount, float factor = 1.0f) {
                // ======== CREDIT: GNU Radio =========
                tapCount |= 1; // ensure that tapCount is odd

                double spb = _sampleRate / _baudRate; // samples per bit/symbol
                double scale = 0;
                for (int i = 0; i < tapCount; i++)
                {
                    double x1, x2, x3, num, den;
                    double xindx = i - tapCount / 2;
                    x1 = FL_M_PI * xindx / spb;
                    x2 = 4 * _alpha * xindx / spb;
                    x3 = x2 * x2 - 1;

                     // Avoid Rounding errors...
                    if (fabs(x3) >= 0.000001) {
                        if (i != tapCount / 2)
                            num = cos((1 + _alpha) * x1) +
                                sin((1 - _alpha) * x1) / (4 * _alpha * xindx / spb);
                        else
                            num = cos((1 + _alpha) * x1) + (1 - _alpha) * FL_M_PI / (4 * _alpha);
                        den = x3 * FL_M_PI;
                    }
                    else {
                        if (_alpha == 1)
                        {
                            taps[i] = -1;
                            scale += taps[i];
                            continue;
                        }
                        x3 = (1 - _alpha) * x1;
                        x2 = (1 + _alpha) * x1;
                        num = (sin(x2) * (1 + _alpha) * FL_M_PI -
                            cos(x3) * ((1 - _alpha) * FL_M_PI * spb) / (4 * _alpha * xindx) +
                            sin(x3) * spb * spb / (4 * _alpha * xindx * xindx));
                        den = -32 * FL_M_PI * _alpha * _alpha * xindx / spb;
                    }
                    taps[i] = 4 * _alpha * num / den;
                    scale += taps[i];
                }

                for (int i = 0; i < tapCount; i++) {
                    taps[i] = taps[i] / scale;
                }
            }

        private:
            int _tapCount;
            float _sampleRate, _baudRate, _alpha;

        };
}