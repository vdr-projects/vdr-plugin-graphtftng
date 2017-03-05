/*
 * dspitems.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 26.05.07
 */

//***************************************************************************
// Class Span Service
//***************************************************************************

#define SPAN_PROVIDER_CHECK_ID  "Span-ProviderCheck-v1.0"
#define SPAN_CLIENT_CHECK_ID 	  "Span-ClientCheck-v1.0"
#define SPAN_SET_PCM_DATA_ID 	  "Span-SetPcmData-v1.0"
#define SPAN_GET_BAR_HEIGHTS_ID "Span-GetBarHeights-v1.0"

//***************************************************************************
// Calss cSpanService
//***************************************************************************

class cSpanService
{
   public:
      
      // Span requests to collect possible providers / clients

      struct Span_Provider_Check_1_0 
      {
         bool* isActive;
         bool* isRunning;
      };

      struct Span_Client_Check_1_0 
      {
         bool* isActive;
         bool* isRunning;
      };

      // Span data

      struct Span_SetPcmData_1_0 
      {
         unsigned int length;	// the length of the PCM-data
         int* data;				// the PCM-Data as 32-bit int, however only the lower 16-bit are used
                              // and you have to take care to hand in such data!
      };

      struct Span_GetBarHeights_v1_0 
      {
         unsigned int bands;						    // number of bands to compute
         unsigned int* barHeights;				    // the heights of the bars of the two channels combined
         unsigned int* barHeightsLeftChannel;	 // the heights of the bars of the left channel
         unsigned int* barHeightsRightChannel;	 // the heights of the bars of the right channel
         unsigned int* volumeLeftChannel;		    // the volume of the left channels
         unsigned int* volumeRightChannel;		 // the volume of the right channels
         unsigned int* volumeBothChannels;		 // the combined volume of the two channels
         const char* name;					       	 // name of the plugin that wants to get the data
                                                 // (must be unique for each client!)
         unsigned int falloff;                   // bar falloff value
         unsigned int* barPeaksBothChannels;     // bar peaks of the two channels combined
         unsigned int* barPeaksLeftChannel;      // bar peaks of the left channel
         unsigned int* barPeaksRightChannel;     // bar peaks of the right channel
      };
};
