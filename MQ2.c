//MQ2
#include "types.h"
#include "adc.h"
#include "adc_defines.h"
u32 MQ2(void)
{
	u32 dval;
	f32 eAR;
	Read_ADC(CH1,&dval,&eAR);
	return dval;
}

