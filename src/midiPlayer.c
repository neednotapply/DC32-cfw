#include <string.h>
#include "midiPlayer.h"
#include "audioPwm.h"
#include "timebase.h"

#define MIDI_MAX_TRACKS 128u
#define MIDI_MAX_TEMPOS 256u

enum MidiEventKind { MidiEventNone, MidiEventNoteOff, MidiEventNoteOn, MidiEventTempo, MidiEventEnd };
struct MidiTrack { uint32_t start, end; bool hasNotes; };
struct MidiTempo { uint32_t tick, usec; };
struct MidiReader { struct FatfsFil *fil; uint32_t pos, end; uint8_t running, buffered, offset, data[128]; };
struct MidiState {
	struct FatfsFil *fil;
	struct MidiTrack tracks[MIDI_MAX_TRACKS];
	struct MidiTempo tempos[MIDI_MAX_TEMPOS];
	uint16_t trackMap[MIDI_MAX_TRACKS], trackCount, selected, ppqn;
	uint16_t tempoCount;
	uint32_t maxTick, currentTick;
	uint64_t deadline;
	struct MusicPlayerStatus *status;
	MusicPlayerControlF controlF;
	void *userData;
	uint8_t velocity[16][128];
	bool held[16][128];
};

static uint32_t midiBe16(const uint8_t *p) { return ((uint32_t)p[0] << 8) | p[1]; }
static uint32_t midiBe32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

static bool midiRead(struct MidiReader *r, uint8_t *vP)
{
	uint32_t n, base;
	if (r->pos >= r->end) return false;
	if (r->offset >= r->buffered) {
		base = r->pos;
		if (!fatfsFileSeek(r->fil, base) || !fatfsFileRead(r->fil, r->data, (r->end-base > sizeof(r->data) ? sizeof(r->data) : r->end-base), &n) || !n) return false;
		r->buffered=(uint8_t)n; r->offset=0;
	}
	*vP=r->data[r->offset++]; r->pos++; return true;
}

static bool midiBytes(struct MidiReader *r, uint8_t *buf, uint32_t len)
{
	if (len > r->end-r->pos) return false;
	if (!len) return true;
	if (!buf) { r->pos += len; r->buffered=0; r->offset=0; return true; }
	while (len--) if (!midiRead(r,buf++)) return false;
	return true;
}

static bool midiVar(struct MidiReader *r, uint32_t *vP)
{
	uint32_t value=0; uint8_t b, n=0;
	do { if (++n > 4 || !midiRead(r,&b)) return false; value=(value<<7)|(b&0x7f); } while (b&0x80);
	*vP=value; return true;
}

static bool midiNext(struct MidiReader *r, uint32_t *deltaP, enum MidiEventKind *kindP, uint8_t *chP, uint8_t *noteP, uint8_t *velP, uint32_t *tempoP)
{
	uint8_t st,b,type,ch; uint32_t len;
	*kindP=MidiEventNone;
	if (!midiVar(r,deltaP) || !midiRead(r,&st)) return false;
	if (st < 0x80) { if (r->running < 0x80 || r->running >= 0xf0) return false; b=st; st=r->running; }
	else { b=0; if (st < 0xf0) r->running=st; }
	if (st == 0xff) {
		if (!midiRead(r,&type) || !midiVar(r,&len) || len > r->end-r->pos) return false;
		if (type==0x2f) { if (len && !midiBytes(r,NULL,len)) return false; *kindP=MidiEventEnd; return true; }
		if (type==0x51) { if (len != 3) { if (!midiBytes(r,NULL,len)) return false; } else { uint8_t t[3]; if (!midiBytes(r,t,3)) return false; *kindP=MidiEventTempo; *tempoP=((uint32_t)t[0]<<16)|((uint32_t)t[1]<<8)|t[2]; return true; } }
		else if (!midiBytes(r,NULL,len)) return false;
		return true;
	}
	if (st==0xf0 || st==0xf7) { if (!midiVar(r,&len) || !midiBytes(r,NULL,len)) return false; return true; }
	if (st<0x80 || st>=0xf0) return false;
	ch=st&0x0f; if ((st&0xf0)==0xc0 || (st&0xf0)==0xd0) { if (!midiRead(r,&b)) return false; return true; }
	if (!midiRead(r,&b) || !midiRead(r,velP)) return false;
	*chP=ch; *noteP=b; if ((st&0xf0)==0x90 && *velP) *kindP=MidiEventNoteOn; else if ((st&0xf0)==0x80 || (st&0xf0)==0x90) *kindP=MidiEventNoteOff;
	return true;
}

static bool midiScanTrack(struct MidiState *s, uint16_t index)
{
	struct MidiReader r={s->fil,s->tracks[index].start,s->tracks[index].end,0}; uint32_t tick=0,delta,tempo; enum MidiEventKind kind; uint8_t ch,note,vel;
	while (r.pos < r.end) {
		if (!midiNext(&r,&delta,&kind,&ch,&note,&vel,&tempo) || tick > UINT32_MAX-delta) return false;
		tick += delta; if (tick>s->maxTick) s->maxTick=tick;
		if (kind==MidiEventTempo) { if (s->tempoCount>=MIDI_MAX_TEMPOS) return false; s->tempos[s->tempoCount++]=(struct MidiTempo){tick,tempo}; }
		else if (kind==MidiEventNoteOn || kind==MidiEventNoteOff) s->tracks[index].hasNotes=true;
		if (kind==MidiEventEnd) break;
	}
	return r.pos <= r.end;
}

static void midiSortTempos(struct MidiState *s)
{
	uint16_t i; for (i=1;i<s->tempoCount;i++) { struct MidiTempo t=s->tempos[i]; uint16_t j=i; while(j && t.tick<s->tempos[j-1].tick){s->tempos[j]=s->tempos[j-1];j--;} s->tempos[j]=t; }
}

static uint32_t midiTempoAt(const struct MidiState *s, uint32_t tick)
{
	uint32_t tempo=500000; uint16_t i; for(i=0;i<s->tempoCount && s->tempos[i].tick<=tick;i++) tempo=s->tempos[i].usec; return tempo;
}

static uint64_t midiTickDelta(const struct MidiState *s, uint32_t from, uint32_t to)
{
	uint32_t at=from; uint64_t total=0; uint16_t i;
	if (to<=from) return 0;
	for(i=0;i<s->tempoCount && s->tempos[i].tick<to;i++) if(s->tempos[i].tick>at){ total+=((uint64_t)(s->tempos[i].tick-at)*midiTempoAt(s,at)/s->ppqn)*TICKS_PER_SECOND/1000000u; at=s->tempos[i].tick; }
	total+=((uint64_t)(to-at)*midiTempoAt(s,at)/s->ppqn)*TICKS_PER_SECOND/1000000u; return total;
}

static uint32_t midiFreq(uint8_t note)
{
	static const uint16_t base[12]={262,277,294,311,330,349,370,392,415,440,466,494}; int octave=note/12,pitch=note%12; uint32_t f=base[pitch];
	while(octave>4){f*=2;octave--;} while(octave<4){f=(f+1)/2;octave++;} return f;
}

static void midiTone(struct MidiState *s)
{
	int ch,n,best=-1; for(ch=0;ch<16;ch++) if(ch!=9) for(n=0;n<128;n++) if(s->held[ch][n] && (best<0 || n>best)) best=n;
	if(best>=0) (void)audioPwmTone(midiFreq((uint8_t)best)); else audioPwmStop();
}

enum MidiWaitResult { MidiWaitOk, MidiWaitStop, MidiWaitPrev, MidiWaitNext, MidiWaitTrack };
static enum MidiWaitResult midiWait(struct MidiState *s, uint32_t tick)
{
	s->deadline += midiTickDelta(s,s->currentTick,tick);
	while(getTime()<s->deadline){ enum MusicPlayerControl c=s->controlF?s->controlF(s->userData,s->status):MusicPlayerControlNone;
		if(c==MusicPlayerControlTrackPrev || c==MusicPlayerControlTrackNext){ if(s->trackCount>1){ if(c==MusicPlayerControlTrackPrev)s->selected=(s->selected+s->trackCount-1)%s->trackCount;else s->selected=(s->selected+1)%s->trackCount; s->status->track=s->selected; } return MidiWaitTrack; }
		if(c==MusicPlayerControlStop)return MidiWaitStop;
		if(c==MusicPlayerControlPrev)return MidiWaitPrev;
		if(c==MusicPlayerControlNext)return MidiWaitNext;
		if(c==MusicPlayerControlPause){ uint64_t start=getTime(); s->status->paused=true; audioPwmStop(); while(1){uint64_t poll=getTime()+TICKS_PER_SECOND/100; enum MusicPlayerControl q=s->controlF?s->controlF(s->userData,s->status):MusicPlayerControlNone; if(q==MusicPlayerControlPause){s->status->paused=false;s->deadline+=getTime()-start;midiTone(s);break;} if(q==MusicPlayerControlStop)return MidiWaitStop;if(q==MusicPlayerControlPrev)return MidiWaitPrev;if(q==MusicPlayerControlNext)return MidiWaitNext;while(getTime()<poll);}}
		uint64_t poll=getTime()+TICKS_PER_SECOND/100; while(getTime()<poll && getTime()<s->deadline);
	}
	s->currentTick=tick; return MidiWaitOk;
}

static enum MidiWaitResult midiPlayTrack(struct MidiState *s)
{
	struct MidiReader r={s->fil,s->tracks[s->trackMap[s->selected]].start,s->tracks[s->trackMap[s->selected]].end,0}; uint32_t tick=0,delta,tempo; enum MidiEventKind kind; uint8_t ch,note,vel;
	memset(s->held,0,sizeof(s->held)); memset(s->velocity,0,sizeof(s->velocity));
	while(r.pos<r.end){ if(!midiNext(&r,&delta,&kind,&ch,&note,&vel,&tempo) || tick>UINT32_MAX-delta)return MidiWaitStop; tick+=delta;
		if(kind==MidiEventEnd)
			break;
		if(tick<s->currentTick){if(kind==MidiEventNoteOn&&ch!=9){s->held[ch][note]=true;s->velocity[ch][note]=vel;}else if(kind==MidiEventNoteOff&&ch!=9)s->held[ch][note]=false;continue;}
		{enum MidiWaitResult w=midiWait(s,tick); if(w!=MidiWaitOk)return w;}
		if(kind==MidiEventTempo) { (void)tempo; }
		else if(kind==MidiEventNoteOn&&ch!=9){s->held[ch][note]=true;s->velocity[ch][note]=vel;}
		else if(kind==MidiEventNoteOff&&ch!=9)s->held[ch][note]=false;
		midiTone(s); s->status->bpm=600000u/midiTempoAt(s,tick); s->status->bytesPlayed=s->status->fileSize && s->maxTick ? (uint32_t)(((uint64_t)tick*s->status->fileSize)/s->maxTick):s->status->fileSize;
	}
	return MidiWaitOk;
}

enum MusicPlayerResult midiPlayerPlayFile(struct FatfsFil *fil, void *scratch, uint32_t scratchSize, MusicPlayerControlF controlF, void *userData)
{
	struct MidiState s; struct MusicPlayerStatus status; uint8_t h[14],chunk[8]; uint32_t size,pos,fmt,ntracks,division,i; (void)scratch;(void)scratchSize;
	memset(&s,0,sizeof(s)); memset(&status,0,sizeof(status)); s.fil=fil;s.status=&status;s.controlF=controlF;s.userData=userData;status.fileSize=fatfsFileGetSize(fil); if(status.fileSize<14)return MusicPlayerResultDecodeError;
	for(pos=0;pos+14<=status.fileSize;pos++){if(!fatfsFileSeek(fil,pos)||!fatfsFileRead(fil,h,4,&i)||i!=4) return MusicPlayerResultFileError;if(!memcmp(h,"MThd",4))break;} if(pos+14>status.fileSize||!fatfsFileSeek(fil,pos)||!fatfsFileRead(fil,h,14,&i)||i!=14)return MusicPlayerResultDecodeError;
	if(midiBe32(h+4)!=6)return MusicPlayerResultDecodeError;
	fmt=midiBe16(h+8); ntracks=midiBe16(h+10); division=midiBe16(h+12);
	if(fmt>1||!ntracks||ntracks>MIDI_MAX_TRACKS||!division||(division&0x8000))return MusicPlayerResultUnsupported;
	s.ppqn=division; pos+=14;
	for(i=0;i<ntracks;i++){if(pos+8>status.fileSize||!fatfsFileSeek(fil,pos)||!fatfsFileRead(fil,chunk,8,&size)||size!=8||memcmp(chunk,"MTrk",4))return MusicPlayerResultDecodeError;size=midiBe32(chunk+4);if(size>status.fileSize-pos-8)return MusicPlayerResultDecodeError;s.tracks[i]=(struct MidiTrack){pos+8,pos+8+size,false};pos+=8+size;}
	for(i=0;i<ntracks;i++)
		if(!midiScanTrack(&s,(uint16_t)i))
			return MusicPlayerResultDecodeError;
	midiSortTempos(&s);
	for(i=0;i<ntracks;i++)
		if(s.tracks[i].hasNotes)
			s.trackMap[s.trackCount++]=(uint16_t)i;
	if(!s.trackCount){s.trackMap[0]=0;s.trackCount=1;}
	status.trackCount=s.trackCount; status.bpm=600000u/midiTempoAt(&s,0); s.deadline=getTime();
	while(1){enum MidiWaitResult w=midiPlayTrack(&s);if(w==MidiWaitTrack)continue;if(w==MidiWaitStop){audioPwmStop();return MusicPlayerResultStopped;}if(w==MidiWaitPrev){audioPwmStop();return MusicPlayerResultPrev;}if(w==MidiWaitNext){audioPwmStop();return MusicPlayerResultNext;}break;}
	audioPwmStop();status.bytesPlayed=status.fileSize;return MusicPlayerResultDone;
}
