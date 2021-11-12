//------------------------------------------------------------------------------
#define  IWM_VERSION         "iwmtime_20211111"
#define  IWM_COPYRIGHT       "Copyright (C)2021 iwm-iwama"
//------------------------------------------------------------------------------
#include "lib_iwmutil.h"

INT  main();
VOID print_version();
VOID print_help();

// [�����F] + ([�w�i�F] * 16)
//  0 = Black    1 = Navy     2 = Green    3 = Teal
//  4 = Maroon   5 = Purple   6 = Olive    7 = Silver
//  8 = Gray     9 = Blue    10 = Lime    11 = Aqua
// 12 = Red     13 = Fuchsia 14 = Yellow  15 = White

// �^�C�g��
#define  COLOR01             (15 + ( 9 * 16))
// ���͗�^��
#define  COLOR11             (15 + (12 * 16))
#define  COLOR12             (13 + ( 0 * 16))
#define  COLOR13             (12 + ( 0 * 16))
// �I�v�V����
#define  COLOR21             (14 + ( 0 * 16))
#define  COLOR22             (11 + ( 0 * 16))
// ����
#define  COLOR91             (15 + ( 0 * 16))
#define  COLOR92             ( 7 + ( 0 * 16))

INT
main()
{
	// lib_iwmutil ������
	iCLI_getARGV();      //=> $CMD, $ARGV, $ARGC
	iConsole_getColor(); //=> $ColorDefault, $StdoutHandle

	// -h | -help
	if(! $ARGC || iCLI_getOptMatch(0, "-h", "-help"))
	{
		print_help();
		imain_end();
	}

	// -v | -version
	if(iCLI_getOptMatch(0, "-v", "-version"))
	{
		print_version();
		imain_end();
	}

	// -q | -quiet
	UINT iOptionQuiet = (
		iCLI_getOptMatch(0, "-q", "-quiet") ?
		imi_len($ARGV[0]) + 1 :
		0
	);

	MBS *cmd = iary_join($ARGV, " ");

	MEMORYSTATUSEX memex = { sizeof(MEMORYSTATUSEX) };
	DWORDLONG iBgnEmpMem = 0;
	DWORDLONG iEndEmpMem = 0;

	GlobalMemoryStatusEx(&memex);
	iBgnEmpMem = memex.ullAvailPhys;

	// �v���J�n
	iExecSec_init(); //=> $ExecSecBgn

	if(iOptionQuiet)
	{
		system(ims_cats((cmd + iOptionQuiet), " > ", NULL_DEVICE, NULL));
		PZ(COLOR21, NULL);
		P2("[Quiet Mode]");
	}
	else
	{
		system(cmd);
	}

	// �v���I��
	DOUBLE dPassedSec = iExecSec_next();

	GlobalMemoryStatusEx(&memex);
	iEndEmpMem = memex.ullAvailPhys;

	PZ(COLOR91, NULL);
	LN();
	// Program
	P ("  Program  %s\n", (cmd + iOptionQuiet));
	// Memory
	P("  Memory   %s KB (%s KB => %s KB)\n",
		ims_addTokenNStr(ims_sprintf("%d", ((iEndEmpMem - iBgnEmpMem) / 1024))),
		ims_addTokenNStr(ims_sprintf("%d", (iBgnEmpMem / 1024))),
		ims_addTokenNStr(ims_sprintf("%d", (iEndEmpMem / 1024)))
	);
	// Exec
	P("  Exec     %s sec\n", ims_addTokenNStr(ims_sprintf("%.4f", dPassedSec)));
	LN();
	PZ(-1, NULL);

	// Debug
	/// icalloc_mapPrint(); ifree_all(); icalloc_mapPrint();

	// �ŏI����
	imain_end();
}

VOID
print_version()
{
	PZ(COLOR92, NULL);
	LN();
	P(" %s\n", IWM_COPYRIGHT);
	P("   Ver.%s+%s\n", IWM_VERSION, LIB_IWMUTIL_VERSION);
	LN();
	PZ(-1, NULL);
}

VOID
print_help()
{
	print_version();
	PZ(COLOR01, " �R�}���h�̎��s���Ԃ��v�� \n\n");
	PZ(COLOR11, " %s [�R�}���h] [�I�v�V����] [����] ... \n\n", $CMD);
	PZ(COLOR12, " (��P)\n");
	PZ(COLOR91, "   > %s notepad\n\n", $CMD);
	PZ(COLOR12, " (��Q)\n");
	PZ(COLOR91, "   > %s dir \"..\" /b\n\n", $CMD);
	PZ(COLOR21, " [�I�v�V����]\n");
	PZ(COLOR22, "   -quiet | -q\n");
	PZ(COLOR91, "       �R�}���h�o�͂�\\�����Ȃ�\n\n");
	PZ(COLOR92, NULL);
	LN();
	PZ(-1, NULL);
}
