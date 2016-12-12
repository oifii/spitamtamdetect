/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

//2012mar17, spi, begin
#include "WavFile.h"
#include "SoundTouch.h"
#include "BPMDetect.h"
using namespace soundtouch;
#define BUFF_SIZE	2048
//2012mar17, spi, end

/* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (1024)
#define NUM_SECONDS     (16)
//#define NUM_SECONDS     (20)
#define NUM_CHANNELS    (2)
/* #define DITHER_FLAG     (paDitherOff)  */
#define DITHER_FLAG     (0) /**/

/* Select sample format. */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif


/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;
    SAMPLE *recordedSamples;
    int i;
    int totalFrames;
    int numSamples;
    int numBytes;
    SAMPLE max, average, val;
    
    
    printf("spi_record_read_bpmdetect.c\n"); fflush(stdout);

    totalFrames = NUM_SECONDS * SAMPLE_RATE; // Record for a few seconds. 
    numSamples = totalFrames * NUM_CHANNELS;  

    numBytes = numSamples * sizeof(SAMPLE);
    recordedSamples = (SAMPLE *) malloc( numBytes );
    if( recordedSamples == NULL )
    {
        printf("Could not allocate record array.\n");
        exit(1);
    }
    for( i=0; i<numSamples; i++ ) recordedSamples[i] = 0;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    inputParameters.device = Pa_GetDefaultInputDevice(); // default input device 
    if (inputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default input device.\n");
      goto error;
    }
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // Record some audio. --------------------------------------------
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,                  // &outputParameters, 
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      // we won't output out of range samples so don't bother clipping them 
              NULL, // no callback, use blocking API 
              NULL ); // no callback, so no callback userData 
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    printf("Now recording!!\n"); fflush(stdout);

    err = Pa_ReadStream( stream, recordedSamples, totalFrames ); //numSamples = totalFrames * NUM_CHANNELS;
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    // Measure maximum peak amplitude. 
    max = 0;
    average = 0;
    for( i=0; i<numSamples; i++ ) //numSamples = totalFrames * NUM_CHANNELS;
    {
        val = recordedSamples[i];
        if( val < 0 ) val = -val; // ABS
        if( val > max )
        {
            max = val;
        }
        average += val;
    }

    average = average / numSamples;

    printf("Sample max amplitude = "PRINTF_S_FORMAT"\n", max );
    printf("Sample average = "PRINTF_S_FORMAT"\n", average );
/*  Was as below. Better choose at compile time because this
    keeps generating compiler-warnings:
    if( PA_SAMPLE_TYPE == paFloat32 )
    {
        printf("sample max amplitude = %f\n", max );
        printf("sample average = %f\n", average );
    }
    else
    {
        printf("sample max amplitude = %d\n", max );
        printf("sample average = %d\n", average );
    }
*/
    /* Write recorded data to a file. */
#if 0
    {
        FILE  *fid;
        fid = fopen("recorded.raw", "wb");
        if( fid == NULL )
        {
            printf("Could not open file.");
        }
        else
        {
            fwrite( recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), totalFrames, fid );
            fclose( fid );
            printf("Wrote data to 'recorded.raw'\n");
        }
    }
#endif
	///////////////////////////////////////////////////////////////////////////////////////////////////
	//Write recorded data to a .WAV file (relies on sountouch wavefile.cpp/.h included in this project)
	///////////////////////////////////////////////////////////////////////////////////////////////////
#if 1
	{
		WavOutFile* pWavOutFile = new WavOutFile("recorded.wav", SAMPLE_RATE, 16, NUM_CHANNELS);
		if(pWavOutFile)
		{
			pWavOutFile->write(recordedSamples, totalFrames * NUM_CHANNELS);
			//pWavOutFile->close(); //close was provoquing a crash, file is automatically closed when WavOutFile object is deleted
			delete pWavOutFile;
		}
	}
#endif


#if 1
	{
		////////////////////////////////////////////////////////////////////////
		// Playback recorded data.  -------------------------------------------- 
		////////////////////////////////////////////////////////////////////////    
		outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device 
		if (outputParameters.device == paNoDevice) {
		  fprintf(stderr,"Error: No default output device.\n");
		  goto error;
		}
		outputParameters.channelCount = NUM_CHANNELS;
		outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;

		printf("Begin playback.\n"); fflush(stdout);
		err = Pa_OpenStream(
				  &stream,
				  NULL, // no input
				  &outputParameters,
				  SAMPLE_RATE,
				  FRAMES_PER_BUFFER,
				  paClipOff,      // we won't output out of range samples so don't bother clipping them 
				  NULL, // no callback, use blocking API 
				  NULL ); // no callback, so no callback userData 
		if( err != paNoError ) goto error;

		if( stream )
		{
			err = Pa_StartStream( stream );
			if( err != paNoError ) goto error;
			printf("Waiting for playback to finish.\n"); fflush(stdout);

			err = Pa_WriteStream( stream, recordedSamples, totalFrames );
			if( err != paNoError ) goto error;

			err = Pa_CloseStream( stream );
			if( err != paNoError ) goto error;
			printf("Done.\n"); fflush(stdout);
		}
	}
#endif

	//2012mar17, spi, begin
	printf("Detecting BPM rate ..."); fflush(stdout);
	float bpmValue;
	int nChannels = inputParameters.channelCount;
	class BPMDetect* pBPMDetect = new BPMDetect(nChannels, SAMPLE_RATE);

	/*
	//recordedSamples dynamically allocatd. recordedSamples is non-interleaved?  
	pBPMDetect->inputSamples(recordedSamples, totalFrames); //numSamples = totalFrames * NUM_CHANNELS;
	*/

	/*
	numBytes = numSamples * sizeof(SAMPLETYPE);
    SAMPLETYPE* analysisBuffer = (SAMPLETYPE*) malloc( numBytes );
    for( i=0; i<numSamples; i++ ) //numSamples = totalFrames * NUM_CHANNELS;
    {
        analysisBuffer[i] = recordedSamples[i]; //both are float type
	}
	//pBPMDetect->inputSamples(analysisBuffer, numSamples);
	pBPMDetect->inputSamples(analysisBuffer, totalFrames);
	*/

	
	SAMPLETYPE analysisBuffer[BUFF_SIZE]; 
	/*
	int iResult = numSamples/BUFF_SIZE;
	if(iResult>0)
	{
		for(int ii=0; ii<iResult; ii++)
		{
			for( i=0; i<BUFF_SIZE; i++ ) //numSamples = totalFrames * NUM_CHANNELS; //numSamples = NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS
			{
				analysisBuffer[i] = recordedSamples[i]; //both are float type
			}
			pBPMDetect->inputSamples(analysisBuffer, BUFF_SIZE/nChannels);
		}
	}
	iResult = numSamples-iResult*BUFF_SIZE;
	for( i=0; i<iResult; i++ ) //numSamples = totalFrames * NUM_CHANNELS; //numSamples = NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS
	{
		analysisBuffer[i] = recordedSamples[i]; //both are float type
	}
	pBPMDetect->inputSamples(analysisBuffer, iResult/nChannels);
	*/
	int ii, j;
	int n = totalFrames/BUFF_SIZE; //numSamples = totalFrames * NUM_CHANNELS; //numSamples = NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS
	if(n>0)
	{
		for(ii=0; ii<n; ii++)
		{
			for( i=0; i<(BUFF_SIZE/nChannels); i++ ) 
			{
				for( j=0; j<nChannels; j++ ) 
				{
					//analysisBuffer[i*2+j] = recordedSamples[ii*BUFF_SIZE+i*2+j]; //if both are float types and channel interleaved buffers
					analysisBuffer[i*2+j] = recordedSamples[ii*BUFF_SIZE+i+j*totalFrames]; //if both are float types but recordedSamples is a non-interleaved buffer
				}
			}
			pBPMDetect->inputSamples(analysisBuffer, BUFF_SIZE/nChannels); //inputSamples() function seems to require a channel interleaved buffer
		}
	}
	int nn = totalFrames-n*BUFF_SIZE;
	if(nn>0)
	{
		for( i=0; i<(nn/nChannels); i++ ) //numSamples = totalFrames * NUM_CHANNELS; //numSamples = NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS
		{
			for( j=0; j<nChannels; j++ ) 
			{
				//analysisBuffer[i*2+j] = recordedSamples[ii*BUFF_SIZE+i*2+j]; //if both are float types and channel interleaved buffers
				analysisBuffer[i*2+j] = recordedSamples[ii*BUFF_SIZE+i+j*totalFrames]; //if both are float types but recordedSamples is a non-interleaved buffer
			}
		}
		pBPMDetect->inputSamples(analysisBuffer, nn/nChannels); //inputSamples() function seems to require a channel interleaved buffer
	}

	bpmValue = pBPMDetect->getBpm();
	printf("Done!\n"); fflush(stdout);
	if(bpmValue>0) { printf("Detected BPM rate %.1f\n\n", bpmValue); fflush(stdout); }
	  else { printf("Could not detected BPM rate.\n\n", bpmValue); fflush(stdout); }
	if(pBPMDetect) 
	{
		delete pBPMDetect;
		pBPMDetect = NULL; 
	}
	/*
	free( analysisBuffer );
	*/
	//2012mar17, spi, end

    free( recordedSamples );
    Pa_Terminate();



	//////////////////////////
	//test using an input file
	//////////////////////////

	// open input file...
    //WavInFile* pWavInFile = new WavInFile("testbeat.wav"); //60bpm
    WavInFile* pWavInFile = new WavInFile("recorded.wav");
	//BPMDetect* pBPMDetect = NULL; 
	if(pWavInFile)
	{
		nChannels = pWavInFile->getNumChannels();
		pBPMDetect = new BPMDetect(pWavInFile->getNumChannels(), pWavInFile->getSampleRate());
		if(pBPMDetect) 
		{
			printf("Detecting BPM rate ..."); fflush(stdout);
			// Process samples read from the input file
			while (pWavInFile->eof() == 0)
			{
				int num, nSamples;

				// Read a chunk of samples from the input file
				num = pWavInFile->read(analysisBuffer, BUFF_SIZE);
				nSamples = num / nChannels;

				// Feed the samples into SoundTouch processor
				pBPMDetect->inputSamples(analysisBuffer, nSamples);
			}
			delete pWavInFile;
			bpmValue = pBPMDetect->getBpm();
			printf("Done!\n"); fflush(stdout);
			if(bpmValue>0) { printf("Detected BPM rate %.1f\n\n", bpmValue); fflush(stdout); }
			  else { printf("Could not detected BPM rate.\n\n", bpmValue); fflush(stdout); }
			delete pBPMDetect;
		}
	}


	///////////////////////////////
	//test using marsyas' tempo.exe
	///////////////////////////////
	printf("Calling marsyas' tempo.exe ...\n"); fflush(stdout);
	printf("the estimated BPM is: "); fflush(stdout);
	int result;
    result = system("tempo.exe ""recorded.wav""");
	printf("Done!\n"); fflush(stdout);

	/////////////////////////////
	//test using marsyas' ibt.exe
	/////////////////////////////
	printf("Calling marsyas' ibt.exe ...\n"); fflush(stdout);
	//int result;
    result = system("ibt.exe ""recorded.wav""");
	printf("Done!\n"); fflush(stdout);

	return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return -1;
}

