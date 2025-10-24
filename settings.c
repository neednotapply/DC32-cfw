#include <string.h>
#include "settings.h"
#include "memMap.h"
#include "printf.h"
#include "qspi.h"



#define SETTINGS_MAGIC				0x4447687a
#define SETTINGS_CUR_VER			5


union SettingsPage {
	struct {
		uint64_t generation;
		uint32_t magic;
		uint32_t version;
		struct Settings settings;
	};
	uint8_t space[QSPI_WRITE_GRANULARITY];
};


static const union SettingsPage* settingsPrvFirstPage(void)
{
	return (const union SettingsPage*)QSPI_SETTINGS_START;
}

static bool settingsPrvIsPastLastPage(const union SettingsPage* sp)
{
	return ((uintptr_t)sp) == QSPI_SETTINGS_START + QSPI_SETTINGS_LEN;
}

static bool settingsPrvIsFirstPageInBlock(const union SettingsPage* sp)
{
	return !((((uintptr_t)sp) - QSPI_SETTINGS_START) % QSPI_ERASE_GRANULARITY);
}

static const union SettingsPage* settingsLocate(void)
{
	const union SettingsPage *sp, *best = NULL;
	
	if (sizeof(union SettingsPage) != QSPI_WRITE_GRANULARITY)
		return NULL;
	
	for (sp = settingsPrvFirstPage(); !settingsPrvIsPastLastPage(sp); sp++) {
		
		if (sp->magic != SETTINGS_MAGIC || sp->version > SETTINGS_CUR_VER)
			continue;
				
		if (!best || best->generation < sp->generation)
			best = sp;
	}
	
	return best;
}

void settingsGet(struct Settings *settings)
{
	const union SettingsPage* sp = settingsLocate();
	uint32_t curVer;
	
	if (sp) {
		
		*settings = sp->settings;
		curVer = sp->version;
	}
	else {

		curVer = 0;
	}
	switch (curVer) {
		
		case 0:				//upgrade settings structure from nothing
			settings->actLikeGBC = 1;
			//fallthrough
		
		case 1:				//upgrade structure from v1
			settings->speed = 2;
			//fallthrough
		
		case 2:				//upgrade from v2
			settings->contrast = 14;
			//fallthrough
		
		case 3:				//upgrade from v3
			settings->upscale = true;
			//fallthrough

		case 4:				//upgrade from v4
			settings->brightness = 15;
			settings->speed = 1;	//yes, reset user preference
			//fallthrough

		//other cases here, in increasing order
	}
}

bool settingsSet(const struct Settings *settings)
{
	const union SettingsPage *sp = settingsLocate();
	union SettingsPage msp;
	uint32_t numTries = 64;
	
	msp.magic = SETTINGS_MAGIC;
	msp.version = SETTINGS_CUR_VER;
	msp.settings = *settings;
	
	if (!sp) {
		msp.generation = 0;
		sp = settingsPrvFirstPage();
	}
	else {
		msp.generation = sp->generation + 1;
	}
	
	while (1) {
		
		if (settingsPrvIsPastLastPage(++sp))
			sp = settingsPrvFirstPage();
		
		if (!numTries--)
			return false;
				
		if (!flashWrite((uintptr_t)sp, settingsPrvIsFirstPageInBlock(sp) ? QSPI_ERASE_GRANULARITY : 0, &msp, sizeof(union SettingsPage)))
			continue;
		
		if (memcmp(sp, &msp, sizeof(union SettingsPage))) {
			
			//invalidate it
			msp.magic ^= 0x55555555;
			(void)flashWrite((uintptr_t)sp, 0, &msp, sizeof(union SettingsPage));
			msp.magic ^= 0x55555555;
			
			continue;
		}
		
		return true;
	}
}

