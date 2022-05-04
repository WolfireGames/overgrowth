//-----------------------------------------------------------------------------
//           Name: soundtrack.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "soundtrack.h"

#include <Sound/Loader/void_loader.h>
#include <Sound/Loader/ogg_loader.h>

#include <Logging/logdata.h>
#include <Utility/strings.h>

#include <algorithm>
#include <cmath>

static void ToHighResBufferSegment( HighResBufferSegment& out, const BufferSegment& in ) {
    out.sample_rate = in.sample_rate;
    out.channels = in.channels;
    out.data_size = in.FlatSampleCount() * out.SampleSize();

    for( size_t i = 0; i < in.FlatSampleCount(); i++ ) {
        int32_t hbuf;
        char* hbuf_c = (char*)&hbuf;
        int16_t lbuf;
        char* lbuf_c = (char*)&lbuf;

        lbuf_c[0] = in.buf[i*2+0];
        lbuf_c[1] = in.buf[i*2+1];

        hbuf = lbuf;

        out.buf[i*4+0] = hbuf_c[0];
        out.buf[i*4+1] = hbuf_c[1];
        out.buf[i*4+2] = hbuf_c[2];
        out.buf[i*4+3] = hbuf_c[3];
    }
}

static void ToBufferSegment( BufferSegment& out, const HighResBufferSegment& in ) {
    out.sample_rate = in.sample_rate;
    out.channels = in.channels;
    out.data_size = in.FlatSampleCount() * out.SampleSize();
    for( size_t i = 0; i < in.FlatSampleCount(); i++ ) {
        int32_t hbuf;
        char* hbuf_c = (char*)&hbuf;
        int16_t lbuf;
        char* lbuf_c = (char*)&lbuf;

        hbuf_c[0] = in.buf[i*4+0];
        hbuf_c[1] = in.buf[i*4+1];
        hbuf_c[2] = in.buf[i*4+2];
        hbuf_c[3] = in.buf[i*4+3];

        lbuf = hbuf;

        out.buf[i*2+0] = lbuf_c[0];
        out.buf[i*2+1] = lbuf_c[1];
    }
}

PlayedInterface::PlayedInterface( Soundtrack* _owner ) : owner(_owner) {
    
}

PlayedSongInterface::PlayedSongInterface( Soundtrack* _owner ) : PlayedInterface(_owner), song() {

}

PlayedSongInterface::PlayedSongInterface(Soundtrack* _owner, const MusicXMLParser::Song& _song) : PlayedInterface(_owner), song(_song) {

}

const std::string PlayedSongInterface::GetSongName() const {
    return std::string(song.name);
}

const std::string PlayedSongInterface::GetSongType() const {
    return std::string(song.type);
}

const MusicXMLParser::Song& PlayedSongInterface::GetSong() const {
    return song;
}

Soundtrack::PlayedSegment::PlayedSegment(Soundtrack* _owner ) : 
PlayedInterface(_owner),
data(rc_baseLoader(new voidLoader()) ), 
overlapped_transition(false), 
started(false) {
}

Soundtrack::PlayedSegment::PlayedSegment( Soundtrack* _owner, const MusicXMLParser::Segment& _segment, bool overlapped_transition ) :
PlayedInterface(_owner),
data(rc_baseLoader(new voidLoader()) ),
segment( _segment ),
overlapped_transition( overlapped_transition ),
started(false) {
    if( strlen( segment.path ) > 0 ) {
        Path p = FindFilePath( segment.path, kAnyPath );
        if( p.isValid() ) {
            data = rc_baseLoader(new oggLoader(p));
        } else {
            LOGE << "Segment " << segment << " doesn't refer to a valid resource" << std::endl;
        }
    }
}

void Soundtrack::PlayedSegment::update( HighResBufferSegment* out ) {
    int result = 1;

    BufferSegment buffer;

    buffer.channels = data->get_channels();
    buffer.sample_rate = data->get_sample_rate();

    while( result > 0 && buffer.data_size < buffer.BUF_SIZE ) {
        result = data->stream_buffer_int16( buffer.buf + buffer.data_size, (int) (buffer.BUF_SIZE - buffer.data_size) );

        if( result > 0 ) {
            buffer.data_size += (size_t)result;
        } else if( result < 0 ) {
            LOGE << "Got error from underlying stream_buffer_int16 call" << std::endl;
            buffer.error_code = result;
        } else {
            //End of stream.
        }
    }

    ToHighResBufferSegment(*out, buffer);
}

const MusicXMLParser::Segment& Soundtrack::PlayedSegment::GetSegment()
{
    return segment;
}

bool Soundtrack::PlayedSegment::IsAtEnd()
{
    return data->is_at_end();
}

void Soundtrack::PlayedSegment::Rewind()
{
    data->rewind();
}

int64_t  Soundtrack::PlayedSegment::GetPCMPos()
{
    return data->get_pcm_pos();
}

void Soundtrack::PlayedSegment::SetPCMPos( int64_t pos )
{
    data->set_pcm_pos(pos);
}

int64_t  Soundtrack::PlayedSegment::GetPCMCount()
{
    return data->get_sample_count();
}

int Soundtrack::PlayedSegment::SampleRate()
{
    return data->get_sample_rate();
}

int Soundtrack::PlayedSegment::Channels()
{
    return data->get_channels();
}

const std::string Soundtrack::PlayedSegment::GetSegmentName() const
{
    return segment.name;
}

Soundtrack::PlayedSegmentedSong::PlayedSegmentedSong(Soundtrack* _owner, const MusicXMLParser::Song& _song) : 
PlayedSongInterface(_owner, _song),
currentSegment(_owner, _song.GetStartSegment(), false), 
mixer(TransitionMixer::Linear,1.0f)
{
 
}

Soundtrack::PlayedSegmentedSong::~PlayedSegmentedSong( ) {
}


void  Soundtrack::PlayedSegmentedSong::SetGain(float v) {
    //nil
}

float Soundtrack::PlayedSegmentedSong::GetGain() {
    return 1.0f;
}

void Soundtrack::PlayedSegmentedSong::update( HighResBufferSegment* buffer ) {
    //If we _just_ started playing this song, set position of segment to be what it was last this song played.
    if( currentSegment.started == false ) {
        currentSegment.started = true;
    }

    //Aligned transition is a quick transition where the next song is set to the same raw position
    //And we mix together the two buffers.
    if( segmentQueue.size() > 0 && segmentQueue.front().overlapped_transition )
    {
        HighResBufferSegment first;
        HighResBufferSegment second;

        if( segmentQueue.front().started == false )
        {
            int64_t pos = currentSegment.GetPCMPos(); 
            segmentQueue.front().SetPCMPos(pos);
            segmentQueue.front().started = true;
            mixer.Reset();
        } 

        currentSegment.update(&first);
        segmentQueue.front().update(&second);

        if( currentSegment.IsAtEnd() || segmentQueue.front().IsAtEnd() )
        {
            currentSegment.Rewind();
            segmentQueue.front().Rewind(); 
        }
        
        //If we're done transitioning, go to next.
        if( mixer.Step( buffer, &first, &second ) )
        {
            currentSegment = segmentQueue.front();
            segmentQueue.pop_front();
        }
    }
    else
    {
        currentSegment.update(buffer);

        if( currentSegment.IsAtEnd() )
        {
            if(  segmentQueue.size() > 0 )
            {
                currentSegment = segmentQueue.front();
                segmentQueue.pop_front();
            } 
            else
            {
                currentSegment.Rewind();
            }
        }
    }

    if( buffer->data_size == 0  ) {
        LOGE << "Buffer empty in PlayedSegmentedSong" << std::endl;
    }
     
}

bool Soundtrack::PlayedSegmentedSong::QueueSegment( const MusicXMLParser::Segment& nextSeg ) 
{
    if( (segmentQueue.empty() && currentSegment.GetSegment() != nextSeg ) || (segmentQueue.size() > 0 && segmentQueue.back().GetSegment() != nextSeg) )
    {
        LOGI << "Queueing segment " << nextSeg << std::endl;

        while( segmentQueue.size() > 1 )
        {
            segmentQueue.pop_back();
        } 

        segmentQueue.push_back(PlayedSegment(owner,nextSeg,false));

        return true;
    }
    else
    {
        return false;
    }
    return false;
}


bool Soundtrack::PlayedSegmentedSong::TransitionIntoSegment( const MusicXMLParser::Segment& newSeg )
{
    if( (segmentQueue.empty() && currentSegment.GetSegment() != newSeg ) || (segmentQueue.size() > 0 && segmentQueue.back().GetSegment() != newSeg) )
    {
        LOGI << "Transitioning into segment " << newSeg << std::endl;
        while( segmentQueue.size() > 1 )
        {
            segmentQueue.pop_back();
        } 

        segmentQueue.push_back(PlayedSegment(owner, newSeg,true));

        return true;
    }
    else
    {
        return false;
    }

}

bool Soundtrack::PlayedSegmentedSong::SetSegment( const MusicXMLParser::Segment& nextSeg ) {
    LOGI << "Setting segment " << nextSeg << std::endl;
    segmentQueue.clear();
    currentSegment = PlayedSegment(owner, nextSeg,false);
    return true;
}

const std::string Soundtrack::PlayedSegmentedSong::GetSegmentName() const {
    return currentSegment.GetSegmentName();
}

int Soundtrack::PlayedSegmentedSong::SampleRate() {
    return currentSegment.SampleRate(); 
}

int Soundtrack::PlayedSegmentedSong::Channels() {
    return currentSegment.Channels();
}

void Soundtrack::PlayedSegmentedSong::Rewind() {
    //Nil
}

bool Soundtrack::PlayedSegmentedSong::IsAtEnd() {
    return false; 
}

void Soundtrack::PlayedSegmentedSong::SetPCMPos( int64_t c ) {
    currentSegment.SetPCMPos(c);
}

int64_t Soundtrack::PlayedSegmentedSong::GetPCMPos() {
    return currentSegment.GetPCMPos();
}

int64_t Soundtrack::PlayedSegmentedSong::GetPCMCount() {
    return currentSegment.GetPCMCount();
}

Soundtrack::PlayedSingleSong::PlayedSingleSong(Soundtrack* _owner) :
PlayedSongInterface(_owner),
data(rc_baseLoader(new voidLoader()) ), 
started(false),
target_gain(1.0f),
current_gain(0.0f),
from_gain(0.0f),
current_volume_step(0),
volume_change_per_second(1.0f)
{

}

Soundtrack::PlayedSingleSong::PlayedSingleSong(Soundtrack* _owner ,const MusicXMLParser::Song& _song ) :
PlayedSongInterface(_owner, _song),
data(rc_baseLoader(new voidLoader())),
started(false) ,
target_gain(1.0f),
from_gain(0.0f),
current_gain(0.0f),
current_volume_step(0),
volume_change_per_second(1.0f)
{
    if( strlen(song.file_path) > 0 ) {
        Path p = FindFilePath( song.file_path, kAnyPath );
        if( p.isValid() ) {
            data = rc_baseLoader(new oggLoader(p));
        } else {
            LOGE << "Segment " << song << " doesn't refer to a valid resource" << std::endl;
        }
    }
}

void Soundtrack::PlayedSingleSong::update( HighResBufferSegment* buffer ) {
    int result = 1;

    BufferSegment songbuf;
    songbuf.channels = data->get_channels();
    songbuf.sample_rate = data->get_sample_rate();

    while( result > 0 && songbuf.data_size < songbuf.BUF_SIZE ) {
        result = data->stream_buffer_int16( songbuf.buf + songbuf.data_size, (int)(songbuf.BUF_SIZE - songbuf.data_size) );

        if( result > 0 ) {
            songbuf.data_size += (size_t)result;
        } else if( result < 0 ) {
            LOGE << "Got error from underlying stream_buffer_int16 call" << std::endl;
            songbuf.error_code = result;
        } else {
            //End of stream.
        }
    }

    if( songbuf.data_size > 0 ) {
        const size_t start = current_volume_step;
        //How many bytes for a second of transition times the time of transition.
        const size_t end = (size_t) std::abs(((from_gain-target_gain)*volume_change_per_second) * songbuf.sample_rate * songbuf.channels * 2);

        for( size_t i = 0; i < songbuf.data_size/songbuf.channels; i += 2 ) {
            float d = 1.0f;

            if( end > 0 ) {
                d = (start+i*songbuf.channels)/(float)end;
                if( d > 1.0f ) {
                    d = 1.0f;
                }
                if( d < 0.0f ) {
                    d = 0.0f;
                }
            }
                
            current_gain = from_gain*(1.0f-d)+target_gain*d;

            union {
                int16_t full;
                char part[2];
            } cast;

            for( size_t c = 0; c < (size_t)songbuf.channels; c++ ) {
                cast.part[0] = songbuf.buf[i*songbuf.channels+(c*2)+0];
                cast.part[1] = songbuf.buf[i*songbuf.channels+(c*2)+1];

                cast.full = (int16_t) (cast.full * current_gain);

                songbuf.buf[i*songbuf.channels+(c*2)+0] = cast.part[0];
                songbuf.buf[i*songbuf.channels+(c*2)+1] = cast.part[1];
            }
        }

        current_volume_step += songbuf.data_size;
        if( (size_t)current_volume_step > end ) {
            current_volume_step = end;
            from_gain = target_gain;
        } 
    }

    if( IsAtEnd() ) {
        Rewind();
    }

    ToHighResBufferSegment( *buffer, songbuf  );
}

bool Soundtrack::PlayedSingleSong::IsAtEnd() {
    return data->is_at_end();
}

void Soundtrack::PlayedSingleSong::Rewind() {
    data->rewind();
}

int64_t  Soundtrack::PlayedSingleSong::GetPCMPos() {
    return data->get_pcm_pos();
}

void Soundtrack::PlayedSingleSong::SetPCMPos( int64_t pos ) {
    data->set_pcm_pos(pos);
}

int64_t  Soundtrack::PlayedSingleSong::GetPCMCount() {
    return data->get_sample_count();
}

void Soundtrack::PlayedSingleSong::SetGain( float v ) {
    from_gain = current_gain;
    target_gain = v;
}

float Soundtrack::PlayedSingleSong::GetGain() {
    return current_gain;
}

int Soundtrack::PlayedSingleSong::SampleRate() {
    return data->get_sample_rate();
}

int Soundtrack::PlayedSingleSong::Channels() {
    return data->get_channels();
}

Soundtrack::PlayedLayeredSong::PlayedLayeredSong( Soundtrack* _owner ) : 
PlayedSongInterface(_owner),
pcm_count(0),
pcm_pos(0),
sample_rate(0),
channels(0) {

}

Soundtrack::PlayedLayeredSong::PlayedLayeredSong(Soundtrack* _owner, const MusicXMLParser::Song& _song) : 
PlayedSongInterface(_owner, _song),
pcm_pos(0),
pcm_count(0),
sample_rate(0),
channels(0) {
    bool first_creation = true;
    for(auto & songref : song.songrefs) {
        PlayedSongInterface *ps = NULL;
        MusicXMLParser::Song ns = owner->GetSong(songref.name);
    
        LOGI << "Creating a PlayedLayeredSong layer: " << ns.name << std::endl;
        
        if( strmtch( ns.type, "segmented" ) ) {
            LOGE << "No support for segmented songs in layered songs" << std::endl;
        } else if( strmtch( ns.type, "single" ) ) {
            ps = new PlayedSingleSong( owner, ns );
        } else if( strmtch( ns.type, "layered" ) ) {
            LOGE << "No support for layered songs in layered songs" << std::endl;
        } else {
            LOGE << "Unknown song type in " << ns << std::endl;
        }

        if( ps ) {
            if( channels == 0 ) {
                channels = ps->Channels();
            } else if( channels != ps->Channels() ) {
                LOGE << "Mimatching sample rates in layered song " << song << std::endl;
            }
            
            if( sample_rate == 0 ) {
                sample_rate = ps->SampleRate();
            } else if( sample_rate != ps->SampleRate() ) {
                LOGE << "Misamtching sample rates in layered Song " << song << std::endl;
            }

            if( pcm_count < ps->GetPCMCount() ) {
                pcm_count = ps->GetPCMCount();
            }

            if( first_creation == false ) {
                ps->SetGain(0.0f);
            } else {
                first_creation = false;
            }
    
            subsongs.push_back(rc_PlayedSongInterface(ps));
        }
    }
}

Soundtrack::PlayedLayeredSong::~PlayedLayeredSong()  {

}

void Soundtrack::PlayedLayeredSong::SetGain( float v ) {
    //nil
}

float Soundtrack::PlayedLayeredSong::GetGain() {
    return 1.0f;
}

void Soundtrack::PlayedLayeredSong::update( HighResBufferSegment* buffer ) {
    HighResBufferSegment& first = *buffer;
    HighResBufferSegment second;

    memset(first.buf,0,HighResBufferSegment::BUF_SIZE);

    first.data_size = HighResBufferSegment::BUF_SIZE;

    int64_t max_pcm_pos = 0; 
    int16_t active_layer_count = 0;
    unsigned max_sample_count = 0;

    for( unsigned i = 0; i < subsongs.size(); i++ ) {
        second = HighResBufferSegment();
        subsongs[i]->update(&second);

        if( i != 0 ) {
            LOG_ASSERT(first.sample_rate == second.sample_rate);
            LOG_ASSERT(first.channels == second.channels);
        } else {
            first.sample_rate = second.sample_rate;
            first.channels = second.channels;
        }

        unsigned first_sample_count = first.FlatSampleCount();
        unsigned second_sample_count = second.FlatSampleCount();

        if( second_sample_count > max_sample_count ) {
            max_sample_count = second_sample_count;
        }

        if( subsongs[i]->GetGain() > 0.0f ) {
            active_layer_count++;

            for( unsigned k = 0; k < second_sample_count; k++ ) {
                int32_t buf1;
                char* buf1a = (char*)&buf1;

                int32_t buf2;
                char* buf2a = (char*)&buf2;

                buf1a[0] = first.buf[k*4+0];
                buf1a[1] = first.buf[k*4+1];
                buf1a[2] = first.buf[k*4+2];
                buf1a[3] = first.buf[k*4+3];

                buf2a[0] = second.buf[k*4+0];
                buf2a[1] = second.buf[k*4+1];
                buf2a[2] = second.buf[k*4+2];
                buf2a[3] = second.buf[k*4+3];

                buf1 += buf2;

                first.buf[k*4+0] = buf1a[0];
                first.buf[k*4+1] = buf1a[1];
                first.buf[k*4+2] = buf1a[2];
                first.buf[k*4+3] = buf1a[3];
            }
        }

        int64_t pcm_pos = subsongs[i]->GetPCMPos();
        if( max_pcm_pos < pcm_pos ) {
            max_pcm_pos = pcm_pos;
        }
    }

    this->pcm_pos = max_pcm_pos;

    first.data_size = max_sample_count*first.SampleSize();

    if( IsAtEnd() ) {
        Rewind();
    }
}

bool Soundtrack::PlayedLayeredSong::IsAtEnd() {
    bool is_at_end = true;
    for(auto & subsong : subsongs) {
        is_at_end = is_at_end && subsong->IsAtEnd();
    }
    return is_at_end;
}

void Soundtrack::PlayedLayeredSong::Rewind() {
    pcm_pos = 0;
    for(auto & subsong : subsongs) {
        subsong->Rewind(); 
    } 
}

int64_t  Soundtrack::PlayedLayeredSong::GetPCMPos() {
    return pcm_pos; 
}

void Soundtrack::PlayedLayeredSong::SetPCMPos( int64_t pos ) { 
    pcm_pos = pos;
    for(auto & subsong : subsongs) {
        if( pos < subsong->GetPCMCount() ) {
            subsong->SetPCMPos(pos);
        }
    }
}

int64_t Soundtrack::PlayedLayeredSong::GetPCMCount() {
    int64_t max = 0;
    for(auto & subsong : subsongs) {
        if( max < subsong->GetPCMCount() ) {
            max = subsong->GetPCMCount();
        }
    }
    return max;
}

int Soundtrack::PlayedLayeredSong::SampleRate() {
    return sample_rate;
}

int Soundtrack::PlayedLayeredSong::Channels() {
    return channels;
}

bool Soundtrack::PlayedLayeredSong::SetLayerGain( const std::string& name, float gain ) {
    if( gain > 1.0f ) {
        gain = 1.0f;
    }
    if( gain < 0.0f ) {
        gain = 0.0f;
    }

    for(auto & subsong : subsongs) {
        if( subsong->GetSongName() == name ) {
            subsong->SetGain(gain);
            return true;
        }
    }
    return false;
}

float Soundtrack::PlayedLayeredSong::GetLayerGain( const std::string& name ) {
    for(auto & subsong : subsongs) {
        if( subsong->GetSongName() == name ) {
            return subsong->GetGain();
        }
    }
    return 0.0f;
}

const std::map<std::string,float> Soundtrack::PlayedLayeredSong::GetLayerGains() {
    std::map<std::string,float> gains;
    for(auto & subsong : subsongs) {
        gains[subsong->GetSongName()] = subsong->GetGain();
    }
    return gains;
}

std::vector<std::string> Soundtrack::PlayedLayeredSong::GetLayerNames() const {
    std::vector<std::string> layers;
    for(const auto & songref : song.songrefs) {
        layers.push_back(songref.name);
    }
    return layers;
}

Soundtrack::TransitionPlayer::TransitionPlayer(Soundtrack* _owner) : 
PlayedInterface(_owner),
playing(true), 
mixer(TransitionMixer::Sinusoid,2.0f),
currentSong(new PlayedSingleSong(owner))
{
    
}

Soundtrack::TransitionPlayer::~TransitionPlayer() {

}

void Soundtrack::TransitionPlayer::update( HighResBufferSegment* buffer ) {
    //Play normally.
    if( songQueue.empty() ) {
        currentSong->update(buffer);
        
        if( buffer->data_size <= 0 ) {
            LOGE << "Empty buffer in transition" << std::endl;
        }
        mixer.Reset();
    }
    else //Transition until we have reached end.
    {
        if( currentSong->SampleRate() == songQueue.front()->SampleRate() )
        {
            if( currentSong->Channels() == songQueue.front()->Channels() )
            {
                HighResBufferSegment first;
                HighResBufferSegment second;

                currentSong->update(&first); 
                songQueue.front()->update(&second);

                if( mixer.Step(buffer,&first,&second) )
                {
                    stored_pcm_pos[currentSong->GetSongName()] = currentSong->GetPCMPos();

                    currentSong = songQueue.front(); 
                    songQueue.pop_front();
                    mixer.Reset();
                }
            }
            else
            {
                LOGE << "Unable to transition as the two segments " 
                     << currentSong->GetSongName() << "(" << currentSong->Channels() << ")" 
                     << " and " 
                     << songQueue.front()->GetSongName() << "(" << songQueue.front()->Channels() << ")" 
                     << " don't have the same number of Channels, skipping transition" << std::endl;

                currentSong = songQueue.front();
                songQueue.pop_front();
                mixer.Reset();
            }
        }
        else
        {
                LOGE << "Unable to transition as the two segments " 
                     << currentSong->GetSongName() << "(" << currentSong->SampleRate() << ")" 
                     << " and " 
                     << songQueue.front()->GetSongName() << "(" << songQueue.front()->SampleRate() << ")"
                     << " don't have the same sample rate, skipping transition" << std::endl;

            currentSong = songQueue.front();
            songQueue.pop_front();
            mixer.Reset();
        }
    }
}

bool Soundtrack::TransitionPlayer::TransitionToSong( const MusicXMLParser::Song& ns )
{
    if( (songQueue.size() == 0 && currentSong->GetSong() != ns) 
        || (songQueue.size() > 0 && songQueue.back()->GetSong() != ns ) ) {

        while( songQueue.size() > 1 ) {
            songQueue.pop_back(); 
        }

        PlayedSongInterface *ps = NULL;

        if( strmtch( ns.type, "segmented" ) ) {
            ps = new PlayedSegmentedSong( owner, ns );  
        } else if( strmtch( ns.type, "single" ) ) {
            ps = new PlayedSingleSong( owner, ns );
        } else if( strmtch( ns.type, "layered" ) ) {
            ps = new PlayedLayeredSong( owner, ns );
        } else {
            LOGE << "Unknown song type " << ns.type << std::endl;
        }

        if( ps ) {
            std::map<std::string, int64_t>::iterator it = stored_pcm_pos.find(ps->GetSongName());
            if( it != stored_pcm_pos.end() ) {
                int64_t size = ps->GetPCMCount();
                if( size > 0 ) {
                    int64_t v = it->second % size;
                    ps->SetPCMPos(v);
                }
            }

            songQueue.push_back(rc_PlayedSongInterface(ps));
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool Soundtrack::TransitionPlayer::SetSong( const MusicXMLParser::Song& ns ) {
    mixer.Reset();

    LOGI << "Setting song to " << ns << std::endl;

    if( strmtch( ns.type, "segmented" ) ) {
        currentSong.Set(new PlayedSegmentedSong(owner,ns));
    } else if( strmtch( ns.type, "single" ) ) {
        currentSong.Set(new PlayedSingleSong(owner,ns));
    } else if( strmtch( ns.type, "layered" ) ) {
        currentSong.Set(new PlayedLayeredSong(owner,ns));
    } else {
        LOGE << "Unknown song type " << ns.type << std::endl;
    }

    while( songQueue.size() > 0 ) {
        songQueue.pop_front();    
    }
    return true;
}

const std::string Soundtrack::TransitionPlayer::GetSegmentName() const {
    if( strmtch( GetSongType().c_str(), "segmented" ) ) {
        const PlayedSegmentedSong *pss = currentSong.GetConstPtr<PlayedSegmentedSong>();
        return pss->GetSegmentName();
    } else {
        return std::string();
    }
}

const std::string Soundtrack::TransitionPlayer::GetSongName() const
{
    return currentSong.GetConst().GetSongName();
}

const std::string Soundtrack::TransitionPlayer::GetSongType() const 
{
    return currentSong.GetConst().GetSongType();
}

const std::string Soundtrack::TransitionPlayer::GetNextSongName() const
{
    if( songQueue.size() > 0 ) {
        return songQueue[0].GetConst().GetSongName();
    } else {
        return std::string();
    }
}

const std::string Soundtrack::TransitionPlayer::GetNextSongType() const 
{
    if( songQueue.size() > 0 ) {
        return songQueue[0].GetConst().GetSongType();
    } else {
        return std::string();
    }
}

bool Soundtrack::TransitionPlayer::QueueSegment( const std::string& segment_name ) 
{
    bool found_match = false;
    std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
    for( segmentit = currentSong->GetSong().segments.begin(); 
         segmentit != currentSong->GetSong().segments.end(); 
         segmentit++ )
    {
        if( segmentit->name == segment_name ) {
            if( strmtch(currentSong->GetSongType(), "segmented") ) {
                PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                pss->QueueSegment(*segmentit);
                found_match = true;
            }
        }
    }

    std::deque<rc_PlayedSongInterface>::const_iterator songit;
    for( songit = songQueue.begin();
         songit != songQueue.end();
         songit++ )
    {
        std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
        for( segmentit = (*songit).GetConst().GetSong().segments.begin(); 
             segmentit != (*songit).GetConst().GetSong().segments.end(); 
             segmentit++ ) {
            if( segmentit->name == segment_name ) {
                if( strmtch(currentSong->GetSongType(), "segmented") ) {
                    PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                    pss->QueueSegment(*segmentit);
                    found_match = true;
                }
            }
        }
    }
    if( !found_match )
        LOGE << "Unable to find matching segment " << segment_name << " in current song: " << currentSong->GetSong() << std::endl;
    return found_match;
}

bool Soundtrack::TransitionPlayer::TransitionIntoSegment( const std::string& segment_name )
{
    bool found_match = false;
    std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
    for( segmentit = currentSong->GetSong().segments.begin(); 
         segmentit != currentSong->GetSong().segments.end(); 
         segmentit++ ) {
        if( segmentit->name == segment_name ) {
            if( strmtch(currentSong->GetSongType(), "segmented") ) {
                PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                pss->TransitionIntoSegment(*segmentit);
                found_match = true;
            }
        }
    }

    std::deque<rc_PlayedSongInterface>::const_iterator songit;
    for( songit = songQueue.begin();
         songit != songQueue.end();
         songit++ )
    {
        std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
        for( segmentit = (*songit).GetConst().GetSong().segments.begin(); 
             segmentit != (*songit).GetConst().GetSong().segments.end(); 
             segmentit++ ) {
            if( segmentit->name == segment_name ) {
                if( strmtch(currentSong->GetSongType(), "segmented") ) {
                    PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                    pss->TransitionIntoSegment(*segmentit);
                    found_match = true;
                }
            }
        }
    }

    if( !found_match )
        LOGE << "Unable to find matching segment " << segment_name << " in current song: " << currentSong->GetSong() << std::endl;
    return found_match;
}

bool Soundtrack::TransitionPlayer::SetSegment( const std::string& segment_name )
{
    bool found_match = false;
    std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
    for( segmentit = currentSong->GetSong().segments.begin(); 
         segmentit != currentSong->GetSong().segments.end(); 
         segmentit++ )
    {
        if( segmentit->name == segment_name ) {
            if( strmtch(currentSong->GetSongType(), "segmented") ) {
                PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                pss->SetSegment(*segmentit);
                found_match = true;
            }
        }
    }

    std::deque<rc_PlayedSongInterface>::const_iterator songit;
    for( songit = songQueue.begin();
         songit != songQueue.end();
         songit++ )
    {
        std::vector<MusicXMLParser::Segment>::const_iterator segmentit;
        for( segmentit = (*songit).GetConst().GetSong().segments.begin(); 
             segmentit != (*songit).GetConst().GetSong().segments.end(); 
             segmentit++ )
        {
            if( segmentit->name == segment_name )
            {
                if( strmtch(currentSong->GetSongType(), "segmented") ) {
                    PlayedSegmentedSong *pss = currentSong.GetPtr<PlayedSegmentedSong>();
                    pss->SetSegment(*segmentit);
                    found_match = true;
                }
            }
        }
    }

    if( !found_match ) {
        LOGE << "Unable to find matching segment " << segment_name << " in current song: " << currentSong->GetSong() << std::endl;
    }
    return found_match;
}

void Soundtrack::TransitionPlayer::SetPCMPos( int64_t c ) {
    currentSong->SetPCMPos(c);
}

int64_t Soundtrack::TransitionPlayer::GetPCMCount() {
    return currentSong->GetPCMCount();
}

int64_t Soundtrack::TransitionPlayer::GetPCMPos() {
    return currentSong->GetPCMPos();
}

int Soundtrack::TransitionPlayer::SampleRate() {
    return currentSong->SampleRate();
}

int Soundtrack::TransitionPlayer::Channels() {
    return currentSong->Channels();
}

bool Soundtrack::TransitionPlayer::SetLayerGain(const std::string& name, float gain) {
    if( songQueue.size() > 0 && strmtch(songQueue[0]->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = songQueue[0].GetPtr<PlayedLayeredSong>();
        return pss->SetLayerGain(name,gain);
    } else  if( strmtch(currentSong->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = currentSong.GetPtr<PlayedLayeredSong>();
        return pss->SetLayerGain(name,gain);
    } else {
        return false;
    }
}

float Soundtrack::TransitionPlayer::GetLayerGain(const std::string& name) {
    if( songQueue.size() > 0 && strmtch(songQueue[0]->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = songQueue[0].GetPtr<PlayedLayeredSong>();
        return pss->GetLayerGain(name);
    } else  if( strmtch(currentSong->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = currentSong.GetPtr<PlayedLayeredSong>();
        return pss->GetLayerGain(name);
    }
    return 0.0f;
}

std::vector<std::string> Soundtrack::TransitionPlayer::GetLayerNames() const {
    if( songQueue.size() > 0 && strmtch(songQueue[0].GetConst().GetSongType(), "layered") ) {
        const PlayedLayeredSong *pss = songQueue[0].GetConstPtr<PlayedLayeredSong>();
        return pss->GetLayerNames();
    } else  if( strmtch(currentSong.GetConst().GetSongType(), "layered") ) {
        const PlayedLayeredSong *pss = currentSong.GetConstPtr<PlayedLayeredSong>();
        return pss->GetLayerNames();
    }
    return std::vector<std::string>();
}

const std::map<std::string,float> Soundtrack::TransitionPlayer::GetLayerGains() {
    if( songQueue.size() > 0 && strmtch(songQueue[0]->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = songQueue[0].GetPtr<PlayedLayeredSong>();
        return pss->GetLayerGains();
    } else  if( strmtch(currentSong->GetSongType(), "layered") ) {
        PlayedLayeredSong *pss = currentSong.GetPtr<PlayedLayeredSong>();
        return pss->GetLayerGains();
    }
    return std::map<std::string,float>();
}

bool Soundtrack::QueueSegment( const std::string& name )
{
    return transitionPlayer.QueueSegment(name);
}

bool Soundtrack::TransitionIntoSegment( const std::string& name )
{
    return transitionPlayer.TransitionIntoSegment(name);
}

bool Soundtrack::SetSegment( const std::string& name )
{
    return transitionPlayer.SetSegment(name);
}

const std::string Soundtrack::GetSegment( ) const
{
    return transitionPlayer.GetSegmentName();
}

bool Soundtrack::TransitionToSong( const std::string& name )
{
    std::map<std::string,MusicXMLParser::Music>::iterator musicit;

    if(name == "overgrowth_silence") {
        MusicXMLParser::Song song;
        strcpy(song.type, "single");
        strcpy(song.file_path, "Data/Music/silence.ogg");
        return transitionPlayer.TransitionToSong(song);
    }

    for( musicit = music.begin(); musicit != music.end(); musicit++ )
    {
        std::vector<MusicXMLParser::Song>::iterator songit ;

        for( songit = musicit->second.songs.begin();
             songit != musicit->second.songs.end();
             songit++ )
        {
            if( songit->name == name )
            {
                return transitionPlayer.TransitionToSong(*songit);
            }  
        }
        
    }
    LOGW_ONCE("Did not find song \"" << name  << "\"");
    return false;
}

bool Soundtrack::SetSong( const std::string& name )
{
    std::map<std::string,MusicXMLParser::Music>::iterator musicit;

    for( musicit = music.begin(); musicit != music.end(); musicit++ )
    {
        std::vector<MusicXMLParser::Song>::iterator songit ;

        for( songit = musicit->second.songs.begin();
             songit != musicit->second.songs.end();
             songit++ )
        {
            if( songit->name == name )
            {
                return transitionPlayer.SetSong(*songit);
            }  
        }
        
    }
    return false;
}

void Soundtrack::SetLayerGain(const std::string& layer, float v) 
{
    transitionPlayer.SetLayerGain(layer, v);
}

const std::string Soundtrack::GetSongName() const
{
    return transitionPlayer.GetSongName();
}

const std::string Soundtrack::GetSongType() const
{
    return transitionPlayer.GetSongType();
}

const std::string Soundtrack::GetNextSongName() const
{
    return transitionPlayer.GetNextSongName();
}

const std::string Soundtrack::GetNextSongType() const
{
    return transitionPlayer.GetNextSongType();
}

const MusicXMLParser::Song Soundtrack::GetSong(const std::string& name) {
    std::map<std::string,MusicXMLParser::Music>::iterator musicit;
    for( musicit = music.begin(); musicit != music.end(); musicit++ )
    {
        std::vector<MusicXMLParser::Song>::iterator songit ;

        for( songit = musicit->second.songs.begin();
             songit != musicit->second.songs.end();
             songit++ )
        {
            if( songit->name == name )
            {
                return *songit;
            }  
        }
        
    }
    LOGW_ONCE("Did not find song \"" << name  << "\"");
    return MusicXMLParser::Song();
}

const std::map<std::string,float> Soundtrack::GetLayerGains() {
    return transitionPlayer.GetLayerGains();
}

const std::vector<std::string> Soundtrack::GetLayerNames() const {
    return transitionPlayer.GetLayerNames();
}

void Soundtrack::PostProcessVolume( HighResBufferSegment* buffer )
{
    const size_t start = current_volume_step;
    //How many bytes for a second of transition times the time of transition.
    const size_t end = (size_t) std::abs(((volume_start-volume_target)*volume_change_per_second) * buffer->sample_rate);

    for( size_t i = 0; i < buffer->FullSampleCount(); i++ )
    {
        float d = 1.0f;

        if( end > 0 )
        {
            d = (start+i*buffer->channels)/(float)end;
            if( d > 1.0f )
            {
                d = 1.0f;
            }

            if( d < 0.0f )
            {
                d = 0.0f;
            }
        }
            
        float current_volume = volume_start*(1.0f-d)+volume_target*d;

        union
        {
            int32_t full;
            char part[4];
        } cast;

        for( size_t c = 0; c < buffer->channels; c++ ) {
            cast.part[0] = buffer->buf[i*buffer->channels*4+c*4+0];
            cast.part[1] = buffer->buf[i*buffer->channels*4+c*4+1];
            cast.part[2] = buffer->buf[i*buffer->channels*4+c*4+2];
            cast.part[3] = buffer->buf[i*buffer->channels*4+c*4+3];

            cast.full = (int32_t) (cast.full * current_volume);

            buffer->buf[i*buffer->channels*4+c*4+0] = cast.part[0];
            buffer->buf[i*buffer->channels*4+c*4+1] = cast.part[1];
            buffer->buf[i*buffer->channels*4+c*4+2] = cast.part[2];
            buffer->buf[i*buffer->channels*4+c*4+3] = cast.part[3];
        }
    }

    current_volume_step += buffer->data_size;
    if( (size_t)current_volume_step > end )
    {
        current_volume_step = end;
        volume_start = volume_target;
    }
}

void Soundtrack::Dispose()
{

}

void Soundtrack::update(rc_alAudioBuffer buffer)
{
    HighResBufferSegment hbs;

    transitionPlayer.update(&hbs);

    PostProcessVolume(&hbs);
    if( g_sound_enable_layered_soundtrack_limiter ) {
        limiter.Step(&hbs); 
    }

    if( hbs.data_size > 0 && hbs.channels > 0 )
    {
        BufferSegment bs;

        ToBufferSegment(bs,hbs);
        if( bs.FullSampleCount() == 0 ) {
            LOGI << "Feeding a null buffer to soundtrack" << std::endl;
            memset(bs.buf,0,BufferSegment::BUF_SIZE);
            bs.channels = 2;
            bs.sample_rate = 44100;
            bs.data_size = BufferSegment::BUF_SIZE;
        }

        buffer->BufferData( ( bs.channels == 1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16, bs.buf, bs.data_size, bs.sample_rate);
    } 
    else
    {
        LOGE << "Unable to stream data from file" << std::endl;
    }
}

unsigned long Soundtrack::required_buffers()
{
    return 4;
}

void Soundtrack::Stop()
{
    LOGI << "Stopping playing" << std::endl;
    transitionPlayer = TransitionPlayer(this);
}

Soundtrack::Soundtrack(float volume) : 
audioStreamer(),
volume_change_per_second(1.0f), 
volume_start(volume), 
volume_target(volume), 
current_volume_step(0),
transitionPlayer(this)
{

}

Soundtrack::~Soundtrack()
{
    
}

void Soundtrack::AddMusic( const Path &path ) 
{
    LOGI << "Adding music sheet:" << path << std::endl;
    MusicXMLParser parser;  

    if( music.find( path.GetOriginalPathStr() ) == music.end() )
    {
        if( parser.Load( path.GetFullPathStr() ) == MusicXMLParser::kErrorNoError )
        {
            std::map<std::string,MusicXMLParser::Music>::iterator musit;
            for(musit = music.begin(); musit != music.end(); ++musit) {
                bool replace = false;
                for(auto & song : parser.music.songs) {
                    for(size_t old_i = 0; old_i < musit->second.songs.size(); ++old_i) {
                        if(strmtch(song.name, musit->second.songs[old_i].name)) {
                            LOGW << "Track " << song.name << " already found in " << musit->first << ", old music set will be overwritten" << std::endl;
                            replace = true;
                            if(transitionPlayer.GetSongName() == std::string(song.name))
                                Stop();
                            break;
                        }
                    }
                }

                // Loop to print all track collisions, but break here
                if(replace) {
                    music.erase(musit);
                    break;
                }
            }

            music[path.GetOriginalPathStr()] = parser.music;
            LOGD << "Loaded music sheet " << path << std::endl;
        }
        else
        {
            LOGE << "Unable to load music file " << path << std::endl;
        }
    }
    else
    {
        LOGW << "Music is already loaded, will not reload: " << path << std::endl;
    }
}

void Soundtrack::RemoveMusic( const Path &path )
{
    std::map<std::string,MusicXMLParser::Music>::iterator musit = music.find( path.GetOriginalPathStr() );

    if( musit != music.end() )
    {
        music.erase( musit ); 
    }
    else
    {
        LOGW << "Tried to remove not loaded music file: " << path << std::endl;
    }
}

bool Soundtrack::KeepPlaying()
{
    return true;
}

bool Soundtrack::GetPosition( vec3 &p )
{
    p = vec3(0.0f);
    return false;
}

void Soundtrack::GetDirection( vec3 &p )
{
    p = vec3(0.0f);
}

const vec3& Soundtrack::GetVelocity()
{
    static vec3 v(0.0f);
    return v;
}

const vec3 Soundtrack::GetPosition()
{
    return vec3(0.0f);
}

const vec3 Soundtrack::GetOcclusionPosition()
{
    return vec3(0.0f);
}

unsigned char Soundtrack::GetPriority()
{
    return -1;
}

void Soundtrack::SetVolume(float volume)
{
    if( volume > 1.0f )
        volume = 1.0f;
    if( volume < 0.0f )
        volume = 0.0f;
    current_volume_step = 0;
    volume_target = volume;
}

bool Soundtrack::IsTransient()
{
    return false;
}
