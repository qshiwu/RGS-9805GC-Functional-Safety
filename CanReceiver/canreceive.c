
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "UseFunction.h"
#include "ThermalTable.h"
#include "canreceive.h"

int main(int argc, char **argv)
{
	int s, i; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
	ADCData ADC_DATA;
	ResultData RESULT_DATA;

	printf("CAN Sockets Receive Demo %s\r\n", argv[1]);

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return 1;
	}

	strcpy(ifr.ifr_name, argv[1] );
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return 1;
	}

	size_t tableSize = sizeof(Voltage2Temp) / sizeof(Voltage2Temp[0]);

	while(1)
	{
		nbytes = read(s, &frame, sizeof(struct can_frame));
			
		if (nbytes < 0) {
	    	perror("Read");
        	return 1;
 	    }


		
		frame.can_id &= 0x7FFFFFFF;

		if(frame.can_id == 0x0760)
		{
			printf("CAN ID : 0x%04X\r\n", frame.can_id);
			ADC_DATA.TEMP1 = (frame.data[0] << 8) | frame.data[1];
			ADC_DATA.TEMP3= (frame.data[2] << 8) | frame.data[3];
			ADC_DATA.P12VGFR = (frame.data[4] << 8) | frame.data[5];
			ADC_DATA.P12V_IMON = (frame.data[6] << 8) | frame.data[7];

			RESULT_DATA.TEMP1 = ((float)ADC_DATA.TEMP1 / pow(2,12)) * 3.3f;
			RESULT_DATA.TEMP3 = ((float)ADC_DATA.TEMP3 / pow(2,12)) * 3.3f;
			RESULT_DATA.P12VGFR = ((float)ADC_DATA.P12VGFR / pow(2,12)) * 3.3f;
			RESULT_DATA.P12V_IMON = ((float)ADC_DATA.P12V_IMON / pow(2,12)) * 3.3f;

			RESULT_DATA.TEMP1 = interpolate(RESULT_DATA.TEMP1, Voltage2Temp, tableSize);
			RESULT_DATA.TEMP3 = interpolate(RESULT_DATA.TEMP3, Voltage2Temp, tableSize);
			RESULT_DATA.P12VGFR = (RESULT_DATA.P12VGFR / 0.15f);
			RESULT_DATA.P12V_IMON = (RESULT_DATA.P12V_IMON - 0.01f) / 0.02f;

			printf("TEMP1 : 0x%04X, %.1fC\r\n", ADC_DATA.TEMP1, RESULT_DATA.TEMP1);
			printf("TEMP3 : 0x%04X, %.1fC\r\n", ADC_DATA.TEMP3, RESULT_DATA.TEMP3);
			printf("P12VGFR : 0x%04X, %.1fV\r\n", ADC_DATA.P12VGFR, RESULT_DATA.P12VGFR);
			printf("P12V_IMON : 0x%04X, %.1fA\r\n", ADC_DATA.P12V_IMON, RESULT_DATA.P12V_IMON);
			printf("\r\n");
		}
		else if(frame.can_id == 0x0761)
		{
			printf("CAN ID : 0x%04X\r\n", frame.can_id);
			ADC_DATA.P12V = (frame.data[0] << 8) | frame.data[1];
			ADC_DATA.DCIN_SUS1= (frame.data[2] << 8) | frame.data[3];
			ADC_DATA.P12VGFR_IMON = (frame.data[4] << 8) | frame.data[5];
			ADC_DATA.P12VGFL_IMON = (frame.data[6] << 8) | frame.data[7];

			RESULT_DATA.P12V = ((float)ADC_DATA.P12V / pow(2,12)) * 3.3f;
			RESULT_DATA.DCIN_SUS1 = ((float)ADC_DATA.DCIN_SUS1 / pow(2,12)) * 3.3f;
			RESULT_DATA.P12VGFR_IMON = ((float)ADC_DATA.P12VGFR_IMON / pow(2,12)) * 3.3f;
			RESULT_DATA.P12VGFL_IMON = ((float)ADC_DATA.P12VGFL_IMON / pow(2,12)) * 3.3f;

			RESULT_DATA.P12V = (RESULT_DATA.P12V / 0.15f);
			RESULT_DATA.DCIN_SUS1 = (RESULT_DATA.DCIN_SUS1 / 0.0966f);
			RESULT_DATA.P12VGFR_IMON = (RESULT_DATA.P12VGFR_IMON - 0.01f) / 0.02f;
			RESULT_DATA.P12VGFL_IMON = (RESULT_DATA.P12VGFL_IMON - 0.01f) / 0.02f;

			printf("P12V : 0x%04X, %.1fV\r\n", ADC_DATA.P12V, RESULT_DATA.P12V);
			printf("DCIN_SUS1 : 0x%04X, %.1fV\r\n", ADC_DATA.DCIN_SUS1, RESULT_DATA.DCIN_SUS1);
			printf("P12VGFR_IMON : 0x%04X, %.1fA\r\n", ADC_DATA.P12VGFR_IMON, RESULT_DATA.P12VGFR_IMON);
			printf("P12VGFL_IMON : 0x%04X, %.1fA\r\n", ADC_DATA.P12VGFL_IMON, RESULT_DATA.P12VGFL_IMON);
			printf("================================\r\n");
		}
		else
		{
			printf("%s 0x%03X [%d] ", ifr.ifr_name, frame.can_id, frame.can_dlc);
	    	
        	for (i = 0; i < frame.can_dlc; i++)
	    		printf("%02X ",frame.data[i]);
        	
	    	printf("\r\n");
		}	
	}

	

	if (close(s) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}
