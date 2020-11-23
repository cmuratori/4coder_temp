
//
// NOTE(casey): These are the platform services that 4coder would need to
// provide to have minimal audio support on Windows:
//

function b32
Win32SubmitAudio(THIS_WOULD_BE_THE_4CODER_HANDLE *Handle, HWAVEOUT WaveOut, WAVEHDR *Header)
{
    b32 Result = false;

    u32 SampleCount = Header->dwBufferLength/4;
    i16 *Samples = (i16 *)Header->lpData;
    
    THIS_IS_THE_MIXER_FUNCTION_THAT_WOULD_BE_PLATFORM_INDEPENDENT(Handle, SampleCount, Samples);

    DWORD Error = waveOutPrepareHeader(WaveOut, Header, sizeof(*Header));
    if(Error == MMSYSERR_NOERROR)
    {
        Error = waveOutWrite(WaveOut, Header, sizeof(*Header));
        if(Error == MMSYSERR_NOERROR)
        {
            // NOTE(casey): Success
            Result = true;
        }
    }

    return(Result);
}

function DWORD WINAPI
Win32AudioLoop(void *Passthrough)
{
    THIS_WOULD_BE_THE_4CODER_HANDLE *Handle = (THIS_WOULD_BE_THE_4CODER_HANDLE *)Passthrough;

    //
    // NOTE(casey): Set up our audio output buffer
    //
    u32 SamplesPerSecond = 48000;
    u32 SamplesPerBuffer = 16*SamplesPerSecond/1000;
    u32 ChannelCount = 2;
    u32 BytesPerChannelValue = 2;
    u32 BytesPerSample = ChannelCount*BytesPerChannelValue;

    u32 BufferCount = 3;
    u32 BufferSize = SamplesPerBuffer*BytesPerSample;
    u32 HeaderSize = sizeof(WAVEHDR);
    u32 TotalBufferSize = (BufferSize+HeaderSize);
    u32 TotalAudioMemorySize = BufferCount*TotalBufferSize;

    //
    // NOTE(casey): Initialize audio out
    //
    HWAVEOUT WaveOut = {};

    WAVEFORMATEX Format = {};
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = (WORD)ChannelCount;
    Format.wBitsPerSample = (WORD)(8*BytesPerChannelValue);
    Format.nSamplesPerSec = SamplesPerSecond;
    Format.nBlockAlign = (Format.nChannels*Format.wBitsPerSample)/8;
    Format.nAvgBytesPerSec = Format.nBlockAlign * Format.nSamplesPerSec;

    if(waveOutOpen(&WaveOut, WAVE_MAPPER, &Format, GetCurrentThreadId(), 0, CALLBACK_THREAD) == MMSYSERR_NOERROR)
    {
        void *AudioBufferMemory = VirtualAlloc(0, TotalAudioMemorySize, MEM_COMMIT, PAGE_READWRITE);
        if(AudioBufferMemory)
        {
            u8 *At = (u8 *)AudioBufferMemory;
            for(u32 BufferIndex = 0;
                BufferIndex < BufferCount;
                ++BufferIndex)
            {
                WAVEHDR *Header = (WAVEHDR *)At;
                At += sizeof(WAVEHDR);
                Header->lpData = (char *)At;
                Header->dwBufferLength = TotalBufferSize;

                At += TotalBufferSize;

                Win32SubmitAudio(Handle, WaveOut, Header);
            }
        }
        else
        {
            // TODO(casey): Report the error here, if you want
        }
    }
    else
    {
        // TODO(casey): Report the error here, if you want
    }

    //
    // NOTE(casey): Serve audio forever (until we are told to stop)
    //
    // SetTimer(0, 0, 100, 0);
    while(!Handle->Quit)
    {
        MSG Message = {};
        GetMessage(&Message, 0, 0, 0);
        if(Message.message == MM_WOM_DONE)
        {
            WAVEHDR *Header = (WAVEHDR *)Message.lParam;
            if(Header->dwFlags & WHDR_DONE)
            {
                Header->dwFlags &= ~WHDR_DONE;
            }

            waveOutUnprepareHeader(WaveOut, Header, sizeof(*Header));

            Win32SubmitAudio(Handle, WaveOut, Header);
        }
    }

    return(0);
}
