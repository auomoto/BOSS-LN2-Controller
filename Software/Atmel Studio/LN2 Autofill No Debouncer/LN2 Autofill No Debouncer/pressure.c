#include "globals.h"
#include "pressure.h"

/*----------------------------------------------------------------------
ADC INITIALIZATION

From page 398 of the ATmega4808/4809 Data Sheet 2020

 1. Configure the resolution in ADCn.CTRLA (p406; default is 10 bits)
 2. Optionally enable free-running mode (default is not)
 3. Optionally configure the # samples per conversion ADCn.CTRLB
 4. Configure voltage reference REFSEL in ADCn.CTRLC.
 5. Configure CLK_ADC by writing to prescaler PRESC in ADCn.CTRLC
 6. Configure an input by writing to the MUXPOS bit field ADCn.MUXPOS
 7. Optional: Enable start event input in event control
 8. Enable the ADC by writing 1 to ENABLE bit in ADCn.CTRLA
 
 Trigger a conversion by writing 1 to STCONV in ADCn.COMMAND
----------------------------------------------------------------------*/
void init_ADC()
{

	PORTD_PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;		// Disable digital input
	ADC0_CTRLA |= ADC_RESSEL_8BIT_gc;		// 8-bit resolution
//	ADC0_CTRLA |= ADC_RESSEL_10BIT_gc;		// 10-bit resolution
	VREF.CTRLA |= VREF_ADC0REFSEL_4V34_gc;	// Use 4.3V reference
//	VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;	// Use 2.5V reference
	ADC0_CTRLC |= ADC_REFSEL_INTREF_gc;		// Use internal vref
	ADC0_CTRLC |= ADC_SAMPCAP_bm;			// Reduce sampling capacitance
	ADC0_MUXPOS = ADC_MUXPOS_AIN0_gc;		// PD0 pin feeds ADC
	ADC0.CTRLA |= ADC_ENABLE_bm;			// Enable ADC
	ADC0_COMMAND |= ADC_STCONV_bm;			// Start ADC conversion
	while (ADC0_COMMAND & ADC_STCONV_bm) {	// Wait
		asm("nop");
	}
	status.pressure = ADC0_RES;				// Throw first one away
	status.pressure = 0;

}

/*----------------------------------------------------------------------
Read the pressure sensor and convert the output to kPa
	The sensor is a Panasonic ADP5151 with 0 kPa to 200 kPa range over
	0.5 V to 4.5 V.

	The ADC is set up for a 4.3 V reference. The PSSLOPE and PSINTERCEPT
	were determined from actual voltage measurements on a Curiosity
	Nano but it looks like data sheet values would be OK.
----------------------------------------------------------------------*/
uint8_t read_PRESSURE(void)
{

	uint8_t i, nsamples;
	float value;

	value = 0.0;
	nsamples = 10;
	for (i = 0; i < nsamples; i++) {
		ADC0_COMMAND |= ADC_STCONV_bm;
		while (ADC0_COMMAND & ADC_STCONV_bm) {
			asm("nop");
		}
		value += (float) ADC0_RESL;
	}

	value = PSSLOPE * (value/((float)nsamples)) + PSINTERCEPT;
	if (value < 0.0) {
		value = 0.0;
	}

	return ((uint8_t)(value+0.5));

}
