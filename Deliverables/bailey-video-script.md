# Bailey's video segment

About 4 minutes. The whole group video is capped at 15 minutes, so this is your share, not the full talk. Read the lines under each time. The slides carry the pictures.

Slides: `Downloads\adv-algo-assign-2-bailey-slides.pptx`

## 0:00 to 0:25. Slide 1

I'm Bailey Tucker. My file is OpenAiMul.c. It multiplies two n-limb integers and writes a 2n-limb product, the same contract as GMP.

## 0:25 to 0:55. Slide 2

I started from the OpenAI preprint of 23 September 2026. That paper proves a multitape bound. Kappa is 2 to the minus 182. The machine uses schoolbook below a cutoff past log n equals 2 to the 2 to the 75. We cannot run that input. The function I timed is a 16-bit digit convolution.

## 0:55 to 1:25. Slide 3

Each 64-bit limb becomes four 16-bit digits, low digit first. Multiplying the integers is then multiplying those digit lists and carrying in base 2 to the 16.

## 1:25 to 1:50. Slide 4

Load the digits. Take a number-theoretic transform modulo 2 to the 64, minus 2 to the 32, plus 1. Multiply pointwise. Invert the transform. Carry. The transform length is at least twice the number of digits.

## 1:50 to 2:35. Slide 5

Here is one I can do by hand. Digits 3 and 5, times 7 and 11. 3 times 7 is 21. The middle place is 3 times 11 plus 5 times 7, 33 plus 35, which is 68. 5 times 11 is 55. None of those exceeds 2 to the 16, so this example has no carry. The product is 21, plus 68 times 2 to the 16, plus 55 times 2 to the 32.

## 2:35 to 3:10. Slide 6

If the length is only 2, the 55 wraps onto the 21 and the low digit becomes 76. That is a different integer. The carry cannot fix it, because the 55 is already in the wrong place. After the inverse transform, bucket k has to hold the full sum of digit products for shift k, with no wrap from a higher shift, and that sum has to be smaller than the modulus. The length gives you the first half. Dividing by the length in the inverse transform gives you the scale. Drop the scale and every coefficient is too big by the length.

## 3:10 to 3:50. Slide 7

The harness checked every product against GMP before the clock started. I kept the table through 16,384 limbs. GMP is fastest on every row. At 16 limbs mine takes about 27 microseconds and schoolbook takes about a quarter of a microsecond. At 2,048 limbs I am still behind schoolbook, 4.6 milliseconds against 4.0. At 4,096 I pass schoolbook, 10.2 against 15.5, and the FFT is still faster at 5.3. At 8,192 I pass the FFT, 20.9 against 27.8. At 16,384 I take 43 milliseconds, the FFT takes 66, schoolbook takes 233, and GMP takes 2.9. That is about 15 times GMP.

## 3:50 to 4:05. Slide 8

The FFT switches to 8-bit digits above 2,048 limbs, so its transform gets longer. Mine stays on 16-bit digits. Next I would build the twiddle powers in setup, outside the timer. A larger n on this file will not catch GMP.
