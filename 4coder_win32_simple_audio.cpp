
//
// NOTE(casey): These are the platform services that 4coder would need to
// provide to have minimal audio support on Windows:
//

function b32
Win32SubmitAudio(CrunkAudio *Crunky, HWAVEOUT WaveOut, WAVEHDR *Header, u32 SampleCount, void *MixBuffer)
{
    b32 Result = false;

    i16 *Samples = (i16 *)Header->lpData;
    F4_MixAudio(Crunky, SampleCount, Samples, MixBuffer);

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
    CrunkAudio *Crunky = (CrunkAudio *)Passthrough;

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
    u32 MixBufferSize = (SamplesPerBuffer*ChannelCount*sizeof(f32));
    u32 TotalAudioMemorySize = BufferCount*TotalBufferSize + MixBufferSize;

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

    void *MixBuffer = 0;
    void *AudioBufferMemory = 0;
    if(waveOutOpen(&WaveOut, WAVE_MAPPER, &Format, GetCurrentThreadId(), 0, CALLBACK_THREAD) == MMSYSERR_NOERROR)
    {
        AudioBufferMemory = VirtualAlloc(0, TotalAudioMemorySize, MEM_COMMIT, PAGE_READWRITE);
        if(AudioBufferMemory)
        {
            u8 *At = (u8 *)AudioBufferMemory;
			MixBuffer = At;
            At += MixBufferSize;
            
            for(u32 BufferIndex = 0;
                BufferIndex < BufferCount;
                ++BufferIndex)
            {
                WAVEHDR *Header = (WAVEHDR *)At;
                Header->lpData = (char *)(Header + 1);
                Header->dwBufferLength = BufferSize;

                At += TotalBufferSize;

                Win32SubmitAudio(Crunky, WaveOut, Header, SamplesPerBuffer, MixBuffer);
            }
        }
        else
        {
            Crunky->Quit = true;
        }
    }
    else
    {
        Crunky->Quit = true;
    }

    //
    // NOTE(casey): Serve audio forever (until we are told to stop)
    //
    
    SetTimer(0, 0, 100, 0);
    while(!Crunky->Quit)
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

            Win32SubmitAudio(Crunky, WaveOut, Header, SamplesPerBuffer, MixBuffer);
        }
    }
    
    if(WaveOut)
    {
        waveOutClose(WaveOut);
    }
    
    if(AudioBufferMemory)
    {
        VirtualFree(AudioBufferMemory, 0, MEM_RELEASE);
    }

    return(0);
}
