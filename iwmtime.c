//------------------------------------------------------------------------------
#define   IWM_VERSION         "iwmtime_20210317"
#define   IWM_COPYRIGHT       "Copyright (C)2021 iwm-iwama"
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
#define   COLOR01             (15 + ( 9 * 16))
// ���͗�^��
#define   COLOR11             (15 + (12 * 16))
#define   COLOR12             (13 + ( 0 * 16))
#define   COLOR13             (12 + ( 0 * 16))
// ����
#define   COLOR21             (14 + ( 0 * 16))
#define   COLOR22             (11 + ( 0 * 16))
// ����
#define   COLOR91             (15 + ( 0 * 16))
#define   COLOR92             ( 7 + ( 0 * 16))

MBS  *$program     = "";
MBS  **$args       = {0};
UINT $argsSize     = 0;
UINT $colorDefault = 0;
UINT $execMS       = 0;

MEMORYSTATUSEX $msex = { sizeof(MEMORYSTATUSEX) };

INT
main()
{
	$program      = iCmdline_getCmd();
	$args         = iCmdline_getArgs();
	$argsSize     = iary_size($args);
	$colorDefault = iConsole_getBgcolor();

	MBS *cmd = iary_join($args, " ");

	if(! imi_len(cmd))
	{
		print_help();
		imain_end();
	}

	UINT iBgnMem = 0;
	UINT iEndMem = 0;

	GlobalMemoryStatusEx(&$msex);
	iBgnMem = $msex.ullAvailPhys;
	$execMS = iExecSec_init();

	system(cmd);

	DOUBLE dPassedSec = iExecSec_next($execMS);
	GlobalMemoryStatusEx(&$msex);
	iEndMem = $msex.ullAvailPhys;

	MBS s1[32] = "";

	PZ(COLOR91, NULL);
	LN();
	P ("  Program  %s\n", cmd);
	sprintf(s1, "%d", ((iEndMem - iBgnMem) / 1024));
	P ("  Memory   %s KB (Including System Usage)\n", ims_addTokenNStr(s1));
	sprintf(s1, "%.4f", dPassedSec);
	P ("  Exec     %s SEC\n", ims_addTokenNStr(s1));
	LN();
	PZ($colorDefault, NULL);

	// Debug
	/// icalloc_mapPrint(); ifree_all(); icalloc_mapPrint();

	// �ŏI����
	imain_end();
}

VOID
print_version()
{
	LN();
	P (" %s\n", IWM_COPYRIGHT);
	P ("   Ver.%s+%s\n", IWM_VERSION, LIB_IWMUTIL_VERSION);
	LN();
}

VOID
print_help()
{
	PZ(COLOR92, NULL);
		print_version();
	PZ(COLOR01, " �R�}���h�̎��s���Ԃ��v�� \n\n");
	PZ(COLOR11, " %s [�R�}���h] [����] ... \n\n", $program);
	PZ(COLOR12, " (��P)\n");
	PZ(COLOR91, "   > %s notepad\n\n", $program);
	PZ(COLOR12, " (��Q)\n");
	PZ(COLOR91, "   > %s dir \"..\" /b\n\n", $program);
	PZ(COLOR92, NULL);
		LN();
	PZ($colorDefault, NULL);
}
