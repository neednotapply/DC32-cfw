#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fatfs.h"



static bool myFatfsDiskRead(void *diskUserData, uint32_t sec, uint32_t numSec, void *dst)
{
	FILE *f = (FILE*)diskUserData;
	
	if (fseek(f, FATFS_DISK_SECT_SZ * sec, SEEK_SET))
		return false;
	
	fprintf(stderr, "   ** R %u + %u\n", sec, numSec);
	
	return numSec == fread(dst, FATFS_DISK_SECT_SZ, numSec, f);
}


static bool myFatfsDiskWrite(void *diskUserData, uint32_t sec, uint32_t numSec, const void *dst)
{
	FILE *f = (FILE*)diskUserData;
	
	if (fseek(f, FATFS_DISK_SECT_SZ * sec, SEEK_SET))
		return false;
	
	fprintf(stderr, "   ** W %u + %u\n", sec, numSec);
	
	return numSec == fwrite(dst, FATFS_DISK_SECT_SZ, numSec, f);
}

int main(int argc, char**argv)
{
	char name[FATFS_NAME_BUF_LEN], nameToDelete[FATFS_NAME_BUF_LEN];
	struct FatFileLocator loc, dirLoc;
	struct FatfsVol *vol;
	struct FatfsDir *dir;
	struct FatfsFil *fil;
	uint8_t fileAttr;
	uint32_t fileSz;
	FILE *f;
	uint32_t i;
	
	
	if (argc != 2 || !(f = fopen(argv[1], "r+b"))) {
		
		fprintf(stderr, "usage: %s <fatimg>\n", argv[0]);
		return -1;
	}
	
	vol = fatfsMount(myFatfsDiskRead, myFatfsDiskWrite, f);
	if (!vol) {
		fprintf(stderr, "mount failed\n");
		return -1;
	}
	
	
	fprintf(stderr, "volume mounted\n");
	
	dir = fatfsDirOpen(vol, "/");
	if (!dir) {
		fprintf(stderr, "open root failed\n");
		return -1;
	}
	
	fprintf(stderr, "root opened\n");
	
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " r -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	fprintf(stderr, "close dir = %d\n", fatfsDirClose(dir));
	
	fprintf(stderr, "DIRCR=%d\n", fatfsDirCreate(vol, "/LoNgDiR.xYz/SomeDirNameHere####", NULL));

	
	dir = fatfsDirOpen(vol, "AVeRYL~1");
	if (!dir)
		fprintf(stderr, "cannot open 'AVeRYL~1'\n");
	else {
		
		fprintf(stderr, "OPEN BY S-NAME OK, close dir0 = %d\n", fatfsDirClose(dir));
	}
	
	dir = fatfsDirOpen(vol, "AVeryLongDirectoryName");
	if (!dir)
		fprintf(stderr, "cannot open 'AVeryLongDirectoryName'\n");
	else {
		
		fprintf(stderr, "OPEN BY L-NAME OK, close dir0 = %d\n", fatfsDirClose(dir));
	}	
	
	dir = fatfsDirOpen(vol, "/AVeryLoNgDirectoryName/some_loNgr_name_omg_[][][][][]_$/woOhooo/../.././././some_loNgr_nAme_omg_[][][][][]_$/woohooo");
	if (!dir)
		fprintf(stderr, "cannot open nested dir\n");
	else {
		while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
			
			fprintf(stderr, " * -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
			if (fileAttr & FATFS_ATTR_DIR)
				dirLoc = loc;
		}
		fatfsDirClose(dir);
	}
	
	dir = fatfsDirOpenWithLocator(vol, &dirLoc);
	if (!dir) {
		fprintf(stderr, "open dir failed\n");
		return -1;
	}
	
	fil = fatfsFileOpenAt(dir, "Pokemon - Blue Version (USA, Europe) (SGB Enhanced).gb", OPEN_MODE_WRITE | OPEN_MODE_CREATE);
	
	if (!fil)
		fprintf(stderr, "open pokesave failed\n");
	else {
		
		uint8_t data[32768] = {1,2,3,4};
		uint32_t bytesWritten, savegameExportSz = 32768;
		bool ret = true;
		
		fprintf(stderr, "1file size is %u\n", fatfsFileGetSize(fil));
		
		if (fatfsFileGetSize(fil) > savegameExportSz)
			ret = ret && fatfsFileTruncate(fil, savegameExportSz);
		
		fprintf(stderr, "2file size is %u\n", fatfsFileGetSize(fil));
		
		ret = ret && fatfsFileWrite(fil, data, savegameExportSz, &bytesWritten) && bytesWritten == savegameExportSz;

		fprintf(stderr, "3file size is %u\n", fatfsFileGetSize(fil));
		
		ret = fatfsFileClose(fil) && ret;
	}
	
	fprintf(stderr, "unmount = %d\n", fatfsUnmount(vol));
	exit(-1);
	
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " d -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	
	fprintf(stderr, "dirCr = %d\n", fatfsDirCreateAt(dir, "qwertyuiop1234567890@@@@@_~3", NULL));

	if (!fatfsDirRewind(dir)) {
		fprintf(stderr, "rewind dir failed\n");
		return -1;
	}
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " D -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	fprintf(stderr, "close dir = %d\n", fatfsDirClose(dir));
	
	//open our new dir
	
	dir = fatfsDirOpenWithLocator(vol, &dirLoc);
	if (!dir) {
		fprintf(stderr, "open dir 2 failed\n");
		return -1;
	}
	if (!fatfsDirRewind(dir)) {
		fprintf(stderr, "rewind dir failed\n");
		return -1;
	}
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " A -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	
	for (i = 0; i < 101; i++) {
		
	#if 0		//SFN only (if sme case) ot simple LFN (else)
		char fname[128] = "Xx";		//SFN only
		
		sprintf(fname + strlen(fname), "%03d", i);
		
	#elif 0		//VERY LONG LFN
		char fname[128] = "123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_$^9876543210qwertyuiopasdfghjklzxcvbnm";
		
		sprintf(fname + strlen(fname), "%03d", i);
	#elif 1		//EXTREME LONG LFN
		char fname[261] = "123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_$^9876543210qwert63459124520456295yuiopasdfghjklzxcvbnmfdgfdgdfgfdg123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_$^9876543210qwert63459124520456295yuiopasdfghjklzxcvbnmfdgf";
		
		sprintf(fname + strlen(fname), "%03d", i);
	#elif 0		//short LFN with nonunique sfn
		const uint8_t forbidden4[] = {0x3b, 0x3d, 0x5b, 0x5d};
		uint32_t j = 0, k = i;
		char fname[128];		//LFN with expansion space for ~xx
			
		fname[j++] = 'x';			//	comment out if SFN should have NO printables?
		do {
			fname[j++] = forbidden4[k & 3];
			k >>= 2;
		} while(k);
		fname[j] = 0;
	#endif
		
		fprintf(stderr, "file cr%d('%s') = %d\n", i, fname, fatfsDirCreateAt(dir, fname, NULL));
		strcpy(nameToDelete, fname);
	}
	if (!fatfsDirRewind(dir)) {
		fprintf(stderr, "rewind dir failed\n");
		return -1;
	}
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " B -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	
	fprintf(stderr, "now deleting '%s'\n", nameToDelete);
	fprintf(stderr, "del = %d\n", fatfsDirDeleteAt(dir, nameToDelete));
	strcpy(nameToDelete + strlen(nameToDelete) - 3, "099");
	fprintf(stderr, "now deleting '%s'\n", nameToDelete);
	fprintf(stderr, "del2 = %d\n", fatfsDirDeleteAt(dir, nameToDelete));
	
	
	if (!fatfsDirRewind(dir)) {
		fprintf(stderr, "rewind dir failed\n");
		return -1;
	}
	while (fatfsDirRead(dir, name, &fileSz, &fileAttr, &loc)) {
		
		fprintf(stderr, " C -> '%s', %u bytes, attr %02xh\n", name, fileSz, fileAttr);
		if (fileAttr & FATFS_ATTR_DIR)
			dirLoc = loc;
	}
	
	fprintf(stderr, "close dir2 = %d\n", fatfsDirClose(dir));
	
	fil = fatfsFileOpen(vol, "/LONGdIR.XYZ/qwertYuiop1234567890@@@@@_~3/somefile", OPEN_MODE_WRITE | OPEN_MODE_CREATE | OPEN_MODE_TRUNCATE);
	if (!fil)
		fprintf(stderr, "open fail\n");
	
	uint32_t k = 0, sumExpected = 0, sumExpected2 = 0;
	for (i = 0; i < 241; i++) {
		uint32_t j, buf[251], numWritten;
		bool ret;
		
		for (j = 0; j < sizeof(buf) / sizeof(*buf); j++) {
			sumExpected += k;
			buf[j] = k++;
		}
		
		ret = fatfsFileWrite(fil, buf, sizeof(buf), &numWritten);
		
		fprintf(stderr, "write block %u = %d, %d\n", i, ret, numWritten);
		if (!ret || numWritten != sizeof(buf))
			break;
	}

	fprintf(stderr, "close = %d\n", fatfsFileClose(fil));
	
	fil = fatfsFileOpen(vol, "/LONGdIR.XYZ/qwertYuiop1234567890@@@@@_~3/somefile", 0);
	if (!fil)
		fprintf(stderr, "open 2 fail\n");
	
	uint32_t sum = 0;
	for (i = 0; i < 251; i++) {
		uint32_t j, buf[241], numRead;
		bool ret;
		
		ret = fatfsFileRead(fil, buf, sizeof(buf), &numRead);
		
		fprintf(stderr, "read block %u = %d, %d\n", i, ret, numRead);
		if (!ret || numRead != sizeof(buf))
			break;
		
		for (j = 0; j < sizeof(buf) / sizeof(*buf); j++)
			sum += buf[j];
	}
	
	fprintf(stderr, "close = %d\n", fatfsFileClose(fil));
	fprintf(stderr, "FFWW: sum = 0x%08x, expected 0x%08x\n", sum, sumExpected);
	
	fprintf(stderr, "truncate = %d\n", fatfsFileTruncate(fil, 200000));

	
	fil = fatfsFileOpen(vol, "/LONGdIR.XYZ/qwertYuiop1234567890@@@@@_~3/somefile", 0);
	if (!fil)
		fprintf(stderr, "open 3 fail\n");
	
	sum = 0;
	sumExpected = 0;
	uint32_t t = 0;
	for (i = 0; i < 13; i++) {
		uint32_t j, buf[101], numRead;
		bool ret;
		
		ret = fatfsFileRead(fil, buf, sizeof(buf), &numRead);
		
		fprintf(stderr, "read block %u = %d, %d\n", i, ret, numRead);
		if (!ret || numRead != sizeof(buf))
			break;
		
		for (j = 0; j < sizeof(buf) / sizeof(*buf); j++) {
			sumExpected += t++;
			sum += buf[j];
		}
	}
	
	fprintf(stderr, "close = %d\n", fatfsFileClose(fil));
	
	fprintf(stderr, "FFWW: sum = 0x%08x, expected 0x%08x\n", sum, sumExpected);
	
	
	fprintf(stderr, "unmount = %d\n", fatfsUnmount(vol));
	
	return 0;
}