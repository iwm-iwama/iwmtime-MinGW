#include "lib_iwmutil.h"
/* 2011-04-16
	�����ł̃`�F�b�N�́A���p�҂Ɂw���S�x��񋟂���B
	�m�蓾�Ă���댯�Ɏ�𔲂��Ă͂����Ȃ��B���Ƃ��A�R�[�h�ʂ��������悤�Ƃ��B
	�w���S�x�Ɓw���x�x�͔���Ⴕ�Ȃ��B
*/
/* 2013-01-31
	�}�N���Ŋ֐��������ȁB�f�o�b�O������Ȃ�B
	�u�R�[�h�̒Z���v���u�R�[�h�̐��Y���v��D�悹��B
*/
/* 2014-01-03
	���X�Ȃ���E�E�E�A
	����֐��̖ߒl�͈ȉ��̃��[���ɋ���B
		��BOOL�n
			TRUE || FALSE ��Ԃ��B
		��MBS*�n
			��{�A�����������Ԃ��B
			(��)
				MBS *function(MBS *����)
				{
					if(���s)           return NULL;
					if(���� == �󕶎�) return "";
					if(���� == �ߒl)   return ims_clone(����); // ����
					MBS *rtn = icalloc_MBS(Byte��); // �V�K
					...
					return rtn;
				}
*/
/* 2014-01-03
	���I�������m��( = icalloc_XXX()��)�Ɖ���𒚔J�ɍs���΁A
	�\���ȁw���S�x�Ɓw���x�x�������̃n�[�h�E�F�A�͒񋟂���B
*/
/* 2014-02-13 (2021-11-12�C��)
	�ϐ����[���^Minqw-w64
		��INT    : (�}31bit) �W��
		��UINT   : (�{32bit) NTFS�֌W
		��INT64  : (�}63bit) �t�@�C���T�C�Y, �
		��DOUBLE : (�}64bit) ����
*/
/* 2016-08-19 (2021-11-11�C��)
	���ϐ��E�萔�\�L
		��iwmutil���ʂ̕ϐ� // "$" + �啶��
			$CMD, $ARGV �Ȃ�
		������ȑ��ϐ� // "$" + ������
			$struct_func() <=> $struct_var
			$union_func() <=> $union_var
		������ȑ��ϐ�����h���������ϐ�
		���֐��ɕt���������ϐ�
			__func_str
		���ʏ�̑��ϐ� // "_"�̎��͏�����
			_str
		��#define(�萔�̂�) // �P�����ڂ͑啶��
			STR�^Str
*/
/* 2016-01-27
	�֐������[��
		[1] i = iwm-iwama
		[2] m = MBS(byte�)�^j = MBS(word�����)�^w = WCS
		[3] a = array�^b = boolean�^i = integer(unsigned, double�܂�)�^n = null�^p = pointer�^s = strings�^v = void
		[4] _
*/
/* 2016-09-09
	�߂�l�ɂ���
		�֐��ɂ����āA�g�p���Ȃ��߂�l�͐ݒ肹�� VOID�^ �Ƃ��邱�ƁB
		���x�ቺ�̍��{�����B
*/
/* 2021-11-18
	�|�C���^�H *(p + n)
	�z��H     p[n]
		Mingw-w64(gcc -O0�^-Os)�ɂ����Ă͑啝�ȑ��x�ቺ���Ȃ��B
		����āA
		�]���A�����������Ƃ���Ă����u�|�C���^�L�q�v�ɂ��Ă������A
		����A�ǐ����l�������u�z��L�q�v�Ƃ���B
*/
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	���ϐ�
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
MBS      *$CMD         = "";  // �R�}���h�����i�[
MBS      **$ARGV       = {0}; // ARGV���i�[
UINT     $ARGC         = 0;   // ARGC���i�[
HANDLE   $StdoutHandle = 0;   // ��ʐ���p�n���h��
UINT     $ColorDefault = 0;   // �R���\�[���F
UINT     $ExecSecBgn   = 0;   // ���s�J�n����

/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	���s�J�n����
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/* Win32 SDK Reference Help�uwin32.hlp(1996/11/26)�v���
	�o�ߎ��Ԃ�DWORD�^�ŕۑ�����Ă��܂��B
	�V�X�e����49.7���ԘA�����ē��삳����ƁA
	�o�ߎ��Ԃ� 0 �ɖ߂�܂��B
*/
/* (��)
	iExecSec_init(); //=> $ExecSecBgn
	Sleep(2000);
	P("-- %.6fsec\n\n", iExecSec_next());
	Sleep(1000);
	P("-- %.6fsec\n\n", iExecSec_next());
*/
// v2021-03-19
UINT
iExecSec(
	CONST UINT microSec // 0 �̂Ƃ� Init
)
{
	UINT microSec2 = GetTickCount();
	if(!microSec)
	{
		$ExecSecBgn = microSec2;
	}
	return (microSec2 < microSec ? 0 : (microSec2 - microSec)); // Err = 0
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	�������m��
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/*
	icalloc() �p�Ɋm�ۂ����z��
	IcallocDiv �͕K�v�ɉ����ĕύX
*/
typedef struct
{
	VOID *ptr; // �|�C���^�ʒu
	UINT num;  // �z����i�z��ȊO = 0�j
	UINT size; // �A���P�[�g��
	UINT id;   // ����
}
$struct_icallocMap;

$struct_icallocMap *__icallocMap; // �ϒ�
UINT __icallocMapSize = 0;        // *__icallocMap �̃T�C�Y�{1
UINT __icallocMapEOD = 0;         // *__icallocMap �̌��݈ʒu�{1
UINT __icallocMapFreeCnt = 0;     // *__icallocMap ���̋󔒗̈�
UINT __icallocMapId = 0;          // *__icallocMap �̏���
CONST UINT __sizeof_icallocMap = sizeof($struct_icallocMap);
// *__icallocMap �̊�{���T�C�Y(�K�X�ύX > 0)
#define  IcallocDiv          (1 << 5)
// �m�ۂ�������������ɍŒ�4byte�̋󔒂��m��
#define  ceilX(n)            ((((n - 5) >> 3) << 3) + (1 << 4))

//---------
// calloc
//---------
/* (��)
	MBS *p1 = icalloc_MBS(100);
	INT *ai = icalloc_INT(100);
*/
/* (��)
	calloc()�\�l�́A1�v���Z�X�^32bitOS��1.5GB���x(OS�ˑ�)
*/
// v2016-01-31
VOID
*icalloc(
	UINT n,    // ��
	UINT size, // �錾�q�T�C�Y
	BOOL aryOn // TRUE=�z��
)
{
	UINT size2 = sizeof($struct_icallocMap);

	// ���� __icallocMap ���X�V
	if(__icallocMapSize == 0)
	{
		__icallocMapSize = IcallocDiv;
		__icallocMap = ($struct_icallocMap*)calloc(__icallocMapSize, size2);
			icalloc_err(__icallocMap);
		__icallocMapId = 0;
	}
	else if(__icallocMapSize <= __icallocMapEOD)
	{
		__icallocMapSize += IcallocDiv;
		__icallocMap = ($struct_icallocMap*)realloc(__icallocMap, __icallocMapSize * size2);
			icalloc_err(__icallocMap);
	}

	// �����Ƀ|�C���^����
	VOID *rtn = calloc(ceilX(n), size);
		icalloc_err(rtn); // �ߒl�� 0 �Ȃ� exit()

	// �|�C���^
	(__icallocMap + __icallocMapEOD)->ptr = rtn;

	// �z��
	(__icallocMap + __icallocMapEOD)->num = (aryOn ? n : 0);

	// �T�C�Y
	(__icallocMap + __icallocMapEOD)->size = ceilX(n) * size;

	// ����
	++__icallocMapId;
	(__icallocMap + __icallocMapEOD)->id = __icallocMapId;
	++__icallocMapEOD;

	return rtn;
}
//----------
// realloc
//----------
/* (��)
	// icalloc() �ŗ̈�m�ۂ�����g�p�B
	MBS *pM = icalloc_MBS(1000);
	pM = irealloc_MBS(pM, 2000);

	// �ȉ��A���ʂ�������B
	//   (��1)�̓f�t���O�����g�ɒ���
	//   (��2)�͗v�f���������قǌ������ǂ�
	//   (��3)��printf()�n�̊��ɑ���
	// ����āA
	//   �\�ł����(��2)�𐄏��B
	//   �����E�������݂�(��3)����B

	// (��1)
	MBS *p11 = ims_clone("ABCDEFGH");
	MBS *p12 = "12345678";
	UINT u11 = imi_len(p11);
	UINT u12 = imi_len(p12);
	p11 = irealloc_MBS(p11, (u11 + u12));
	imi_cpy((p11 + u11), p12);
	P0(p11);
	ifree(p11);

	// (��2) (��1)�Ɠ����x�B
	MBS *p21 = "ABCDEFGH";
	MBS *p22 = ims_cats(2, p21, "12345678");
	P0(p22);
	ifree(p22);

	// (��3) (��1)��菭���x�����x�B
	MBS *p31 = "ABCDEFGH";
	MBS *p32 = ims_sprintf("%s%s", p31, "12345678");
	P0(p32);
	ifree(p32);
*/
// v2021-04-21
VOID
*irealloc(
	VOID *ptr, // icalloc()�|�C���^
	UINT n,    // ��
	UINT size  // �錾�q�T�C�Y
)
{
	// �k���̂Ƃ��������Ȃ�
	if((n * size) <= imi_len(ptr))
	{
		return ptr;
	}
	VOID *rtn = 0;
	UINT u1 = ceilX(n * size);
	// __icallocMap ���X�V
	UINT u2 = 0;
	while(u2 < __icallocMapEOD)
	{
		if(ptr == (__icallocMap + u2)->ptr)
		{
			rtn = (VOID*)realloc(ptr, u1); // �x�����m��
				icalloc_err(rtn); // �ߒl�� 0 �Ȃ� exit()
			(__icallocMap + u2)->ptr = rtn;
			(__icallocMap + u2)->num = ((__icallocMap + u2)->num ? n : 0);
			(__icallocMap + u2)->size = u1;
			break;
		}
		++u2;
	}
	return rtn;
}
//--------------------------------
// icalloc, irealloc�̃G���[����
//--------------------------------
/* (��)
	// �ʏ�
	MBS *p1 = icalloc_MBS(1000);
	icalloc_err(p1);
	// �����I�ɃG���[�𔭐�������
	icalloc_err(NULL);
*/
// v2016-08-30
VOID
icalloc_err(
	VOID *ptr // icalloc()�|�C���^
)
{
	if(!ptr)
	{
		ierr_end("Can't allocate memories!");
	}
}
//-------------------------
// (__icallocMap+n)��free
//-------------------------
// v2021-04-22
VOID
icalloc_free(
	VOID *ptr // icalloc()�|�C���^
)
{
	$struct_icallocMap *map = 0;
	UINT u1 = 0, u2 = 0;
	while(u1 < __icallocMapEOD)
	{
		map = (__icallocMap + u1);
		if(ptr == (map->ptr))
		{
			// �z�񂩂��� free
			if(map->num)
			{
				// 1�����폜
				u2 = 0;
				while(u2 < (map->num))
				{
					if(!(*((MBS**)(map->ptr) + u2)))
					{
						break;
					}
					icalloc_free(*((MBS**)(map->ptr) + u2));
					++u2;
				}
				++__icallocMapFreeCnt;

				// memset() + NULL��� �� free() �̑��
				// 2�����폜
				memset(map->ptr, 0, map->size);
				map->ptr = 0;
				memset(map, 0, __sizeof_icallocMap);
				return;
			}
			else
			{
				memset(map->ptr, 0, map->size);
				map->ptr = 0;
				memset(map, 0, __sizeof_icallocMap);
				++__icallocMapFreeCnt;
				return;
			}
		}
		++u1;
	}
}
//---------------------
// __icallocMap��free
//---------------------
// v2016-01-10
VOID
icalloc_freeAll()
{
	// [0]�̓|�C���^�Ȃ̂Ŏc��
	// [1..]��free
	while(__icallocMapEOD)
	{
		icalloc_free(__icallocMap->ptr);
		--__icallocMapEOD;
	}
	__icallocMap = ($struct_icallocMap*)realloc(__icallocMap, 0); // free()�s��
	__icallocMapSize = 0;
	__icallocMapFreeCnt = 0;
}
//---------------------
// __icallocMap��|��
//---------------------
// v2016-09-09
VOID
icalloc_mapSweep()
{
	// ����Ăяo���Ă��e���Ȃ�
	UINT uSweep = 0;
	$struct_icallocMap *map1 = 0, *map2 = 0;
	UINT u1 = 0, u2 = 0;

	// ���Ԃ��l�߂�
	while(u1 < __icallocMapEOD)
	{
		map1 = (__icallocMap + u1);
		if(!(MBS**)(map1->ptr))
		{
			++uSweep; // sweep��
			u2 = u1 + 1;
			while(u2 < __icallocMapEOD)
			{
				map2 = (__icallocMap + u2);
				if((MBS**)(map2->ptr))
				{
					*map1 = *map2; // �\���̃R�s�[
					memset(map2, 0, __sizeof_icallocMap);
					--uSweep; // sweep��
					break;
				}
				++u2;
			}
		}
		++u1;
	}
	// ������
	__icallocMapFreeCnt -= uSweep;
	__icallocMapEOD -= uSweep;
	/// PL23("__icallocMapFreeCnt=", __icallocMapFreeCnt);
	/// PL23("__icallocMapEOD=", __icallocMapEOD);
	/// PL23("SweepCnt=", uSweep);
}
//---------------------------
// __icallocMap�����X�g�o��
//---------------------------
// v2021-11-29
VOID
icalloc_mapPrint1()
{
	if(!__icallocMapSize)
	{
		return;
	}
	iConsole_getColor();

	iConsole_setTextColor(9 + (0 * 16));
	P2("-1 ----------- 8 ------------ 16 ------------ 24 ------------ 32--------");
	CONST UINT _rowsCnt = 32;
	UINT uRowsCnt = _rowsCnt;
	UINT u1 = 0, u2 = 0;
	while(u1 < __icallocMapSize)
	{
		iConsole_setTextColor(10 + (1 * 16));
		while(u1 < uRowsCnt)
		{
			if((__icallocMap + u1)->ptr)
			{
				P0("��");
				++u2;
			}
			else
			{
				P0("��");
			}
			++u1;
		}
		P(" %7u", u2);
		uRowsCnt += _rowsCnt;

		iConsole_setTextColor(-1);
		NL();
	}
}
// v2020-05-12
VOID
icalloc_mapPrint2()
{
	iConsole_getColor();

	iConsole_setTextColor(9 + (0 * 16));
	P2("------- id ---- pointer -- array --- byte ------------------------------");
	$struct_icallocMap *map = 0;
	UINT uUsedCnt = 0, uUsedSize = 0;
	UINT u1 = 0;
	while(u1 < __icallocMapEOD)
	{
		map = (__icallocMap + u1);
		if((map->ptr))
		{
			++uUsedCnt;
			uUsedSize += (map->size);
			iConsole_setTextColor(15 + (((map->num) ? 4 : 0) * 16));
			P(
				"%-7u %07u [%p] (%2u)%10u => '%s'",
				(u1 + 1),
				(map->id),
				(map->ptr),
				(map->num),
				(map->size),
				(map->ptr)
			);
			iConsole_setTextColor(-1);
			NL();
		}
		++u1;
	}
	iConsole_setTextColor(9 + (0 * 16));
	P(
		"------- Usage %-7u ---- %14u byte -------------------------\n\n",
		uUsedCnt,
		uUsedSize
	);
	iConsole_setTextColor(-1);
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Print�֌W
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-----------
// printf()
//-----------
/* (��)
	P("abc");         //=> "abc"
	P("%s\n", "abc"); //=> "abc\n"

	// printf()�n�͒x���B�\�ł���� P0(), P2() ���g�p����B
	P0("abc");        //=> "abc"
	P2("abc");        //=> "abc\n"
*/
// v2015-01-24
VOID
P(
	MBS *format,
	...
)
{
	va_list va;
	va_start(va, format);
		vfprintf(stdout, format, va);
	va_end(va);
}
//--------
// Print
//--------
/* (��)
	P0("abc"); //=> "abc"
*/
// v2021-11-30
void
P0(
	MBS *pM // ������
)
{
	fputs(pM, stdout);
}
//-------
// Puts
//-------
/* (��)
	P2("abc"); //=> "abc\n"
*/
// v2021-11-30
void
P2(
	MBS *pM // ������
)
{
	P0(pM);
	fputc('\n', stdout);
}
//---------------
// �J��Ԃ��\��
//---------------
/* (��)
	PR("abc", 3); //=> "abcabcabc"
*/
// v2021-11-29
VOID
PR(
	MBS *pM,   // ������
	INT repeat // �J��Ԃ���
)
{
	while(repeat > 0)
	{
		P0(pM);
		--repeat;
	}
}
//---------------
// �F������\��
//---------------
/* (��)
	iConsole_getColor();              //=> $ColorDefault // ���̐F��ۑ�
	PZ(15, "�J���[�����F%s\n", "��"); //=> "�J���[�����F��\n"
	PZ(-1, NULL);                     // ���̐F�ɖ߂�
*/
// v2021-11-10
VOID
PZ(
	INT rgb, // iConsole_setTextColor() �Q��
	MBS *format,
	...
)
{
	if(rgb < 0)
	{
		rgb = $ColorDefault;
	}

	if(!format)
	{
		format = "";
	}

	iConsole_setTextColor(rgb);

	va_list va;
	va_start(va, format);
		vfprintf(stdout, format, va);
	va_end(va);
}
//--------------
// Quick Print
//--------------
/* (��)
	INT iMax = 100;
	MBS *rtn = icalloc_MBS(10 * iMax);
	MBS *pEnd = rtn;
	INT iCnt = 0;

	for(INT _i1 = 1; _i1 <= iMax; _i1++)
	{
		pEnd += sprintf(pEnd, "%d\n", _i1);

		++iCnt;
		if(iCnt == 30)
		{
			iCnt = 0;

			QP(rtn, (pEnd - rtn));
			pEnd = rtn;

			Sleep(1000);
		}
	}
	QP(rtn, (pEnd - rtn));
*/
// v2021-09-19
VOID
QP(
	MBS *pM,   // ������
	UINT sizeM // ������
)
{
	// Buffer ������ΐ�ɏo��
	fflush(stdout);

	// "\n" �� "\r\n" �Ɏ����ϊ�����Ȃ������Ȃ�
	DWORD wfWriten;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteFile(hCon, pM, sizeM, &wfWriten, NULL);
}
//-----------------------
// EscapeSequence�֕ϊ�
//-----------------------
/* (��)
	// '\a' �� '\a' �ƈ���
	// (��)�ʏ�� '\\a' �� '\a' �Ƃ���
*/
// v2021-11-17
MBS
*ims_conv_escape(
	MBS *pM // ������
)
{
	if(!pM)
	{
		return NULL;
	}
	MBS *rtn = ims_clone(pM);
	INT i1 = 0;
	while(*pM)
	{
		if(*pM == '\\')
		{
			++pM;
			switch(*pM)
			{
				case('a'):
					rtn[i1] = '\a';
					break;

				case('b'):
					rtn[i1] = '\b';
					break;

				case('t'):
					rtn[i1] = '\t';
					break;

				case('n'):
					rtn[i1] = '\n';
					break;

				case('v'):
					rtn[i1] = '\v';
					break;

				case('f'):
					rtn[i1] = '\f';
					break;

				case('r'):
					rtn[i1] = '\r';
					break;

				default:
					rtn[i1] = '\\';
					++i1;
					rtn[i1] = *pM;
					break;
			}
		}
		else
		{
			rtn[i1] = *pM;
		}
		++pM;
		++i1;
	}
	rtn[i1] = 0;
	return rtn;
}
//--------------------
// sprintf()�̊g����
//--------------------
/* (��)
	MBS *p1 = ims_sprintf("%s-%s%05d", "ABC", "123", 456); //=> "ABC-12300456"
		PL2(p1);
	ifree(p1);
*/
// v2021-11-14
MBS
*ims_sprintf(
	MBS *format,
	...
)
{
	FILE *oFp = fopen(NULL_DEVICE, "wb");
		va_list va;
		va_start(va, format);
			MBS *rtn = icalloc_MBS(vfprintf(oFp, format, va));
			vsprintf(rtn, format, va);
		va_end(va);
	fclose(oFp);
	return rtn;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	MBS�^WCS�^U8N�ϊ�
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
// v2016-09-05
WCS
*icnv_M2W(
	MBS *pM
)
{
	if(!pM)
	{
		return NULL;
	}
	UINT uW = iji_len(pM);
	WCS *pW = icalloc_WCS(uW);
	MultiByteToWideChar(CP_OEMCP, 0, pM, -1, pW, uW);
	return pW;
}
// v2020-05-30
U8N
*icnv_W2U(
	WCS *pW
)
{
	if(!pW)
	{
		return NULL;
	}
	UINT uW = iwi_len(pW);
	UINT uU = uW * 3;
	U8N *pU = icalloc_MBS(uU);
	WideCharToMultiByte(CP_UTF8, 0, pW, uW, pU, uU, 0, 0);
	return pU;
}
// v2016-09-05
U8N
*icnv_M2U(
	MBS *pM
)
{
	WCS *pW = icnv_M2W(pM);
	U8N *pU = icnv_W2U(pW);
	ifree(pW);
	return pU;
}
// v2021-09-23
WCS
*icnv_U2W(
	U8N *pU
)
{
	if(!pU)
	{
		return NULL;
	}
	UINT uW = imi_len(pU) * 2; // ���m�ɂ� x1.5
	WCS *pW = icalloc_WCS(uW);
	MultiByteToWideChar(CP_UTF8, 0, pU, -1, pW, uW);
	return pW;
}
// v2020-05-30
MBS
*icnv_W2M(
	WCS *pW
)
{
	if(!pW)
	{
		return NULL;
	}
	UINT uW = iwi_len(pW);
	UINT uM = uW * 2;
	MBS *pM = icalloc_MBS(uM);
	WideCharToMultiByte(CP_OEMCP, 0, pW, uW, pM, uM, 0, 0);
	return pM;
}
// v2016-09-05
MBS
*icnv_U2M(
	U8N *pU // UTF-8
)
{
	WCS *pW = icnv_U2W(pU);
	MBS *pM = icnv_W2M(pW);
	ifree(pW);
	return pM;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	�����񏈗�
		p : return Pointer
		s : return String
		1byte     MBS : imp_xxx(), imp_xxx()
		1 & 2byte MBS : ijp_xxx(), ijs_xxx()
		UTF-8     U8N : iup_xxx(), ius_xxx()
		UTF-16    WCS : iwp_xxx(), iws_xxx()
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/* (��)
	MBS *mp1 = "����������aiueo";
	PL3(imi_len(mp1)); //=> 15
	PL3(iji_len(mp1)); //=> 10
	WCS *wp1 = M2W(mp1);
	PL3(iwi_len(wp1)); //=> 10
*/
// v2021-09-23
UINT
imi_len(
	MBS *pM
)
{
	if(!pM)
	{
		return 0;
	}
	UINT rtn = 0;
	while(*pM++)
	{
		++rtn;
	}
	return rtn;
}
// v2021-09-23
UINT
iji_len(
	MBS *pM
)
{
	if(!pM)
	{
		return 0;
	}
	UINT rtn = 0;
	while(*pM)
	{
		pM = CharNextA(pM);
		++rtn;
	}
	return rtn;
}
// v2021-11-17
UINT
iui_len(
	U8N *pU
)
{
	if(!pU)
	{
		return 0;
	}
	UINT rtn = 0;
	// BOM��ǂݔ�΂�(UTF-8N �͊Y�����Ȃ�)
	if(*pU == (CHAR)0xEF && pU[1] == (CHAR)0xBB && pU[2] == (CHAR)0xBF)
	{
		pU += 3;
	}
	INT c = 0;
	while(*pU)
	{
		if(*pU & 0x80)
		{
			// ���o�C�g����
			c = (*pU & 0xfc);
			while(c & 0x80)
			{
				++pU;
				c <<= 1;
			}
		}
		else
		{
			// 1�o�C�g����
			++pU;
		}
		++rtn;
	}
	return rtn;
}
// v2021-09-23
UINT
iwi_len(
	WCS *pW
)
{
	if(!pW)
	{
		return 0;
	}
	UINT rtn = 0;
	while(*pW++)
	{
		++rtn;
	}
	return rtn;
}
//-------------------------
// size��̃|�C���^��Ԃ�
//-------------------------
/* (��)
	MBS *p1 = "AB������";
	PL2(ijp_forwardN(p1, 3)); //=> "����"
*/
// v2020-05-30
MBS
*ijp_forwardN(
	MBS *pM,   // �J�n�ʒu
	UINT sizeJ // ������
)
{
	if(!pM)
	{
		return pM;
	}
	while(*pM && sizeJ > 0)
	{
		pM = CharNextA(pM);
		--sizeJ;
	}
	return pM;
}
// v2021-11-17
U8N
*iup_forwardN(
	U8N *pU,   // �J�n�ʒu
	UINT sizeU // ������
)
{
	if(!pU)
	{
		return pU;
	}
	// BOM��ǂݔ�΂�(UTF-8N �͊Y�����Ȃ�)
	if(*pU == (CHAR)0xEF && pU[1] == (CHAR)0xBB && pU[2] == (CHAR)0xBF)
	{
		pU += 3;
	}
	INT c = 0;
	while(*pU && sizeU > 0)
	{
		if(*pU & 0x80)
		{
			// ���o�C�g����
			c = (*pU & 0xfc);
			while(c & 0x80)
			{
				++pU;
				c <<= 1;
			}
		}
		else
		{
			// 1�o�C�g����
			++pU;
		}
		--sizeU;
	}
	return pU;
}
//---------------
// �召�����u��
//---------------
/* (��)
	MBS *p1 = "aBC";
	PL2(ims_upper(p1)); //=> "ABC"
	PL2(ims_lower(p1)); //=> "abc"
*/
// v2021-09-21
MBS
*ims_UpperLower(
	MBS *pM,
	INT option // 1=Upper�^2=Lower
)
{
	MBS *rtn = ims_clone(pM);
	switch(option)
	{
		case(1): return CharUpperA(rtn); break;
		default: return CharLowerA(rtn); break;
	}
}
//-------------------------
// �R�s�[������������Ԃ�
//-------------------------
/* (��)
	MBS *to = icalloc_MBS(100);
	PL3(imi_cpy(to, "abcde")); //=> 5
	ifree(to);
*/
// v2021-09-22
UINT
imi_cpy(
	MBS *to,
	MBS *from
)
{
	if(!from)
	{
		return 0;
	}
	MBS *pBgn = from;
	while(*from)
	{
		*to++ = *from++;
	}
	*to = 0;
	return (from - pBgn);
}
// v2021-09-22
UINT
iwi_cpy(
	WCS *to,
	WCS *from
)
{
	if(!from)
	{
		return 0;
	}
	WCS *pBgn = from;
	while(*from)
	{
		*to++ = *from++;
	}
	*to = 0;
	return (from - pBgn);
}
/* (��)
	MBS *to = icalloc_MBS(100);
	MBS *p1 = "abcde12345";
	PL3(imi_pcpy(to, p1 + 3, p1 + 8)); //=> 5
	ifree(to);
*/
// v2021-09-22
UINT
imi_pcpy(
	MBS *to,
	MBS *from1,
	MBS *from2
)
{
	if(!from1 || !from2)
	{
		return 0;
	}
	MBS *pBgn = from1;
	while(*from1 && from1 < from2)
	{
		*to++ = *from1++;
	}
	*to = 0;
	return (from1 - pBgn);
}
//-----------------------
// �V�K����������𐶐�
//-----------------------
/* (��)
	MBS *from="abcde";
	PL2(ims_clone(from)); //=> "abcde"
*/
// v2021-04-19
MBS
*ims_clone(
	MBS *from
)
{
	MBS *rtn = icalloc_MBS(imi_len(from));
	imi_cpy(rtn, from);
	return rtn;
}
// v2021-04-19
WCS
*iws_clone(
	WCS *from
)
{
	WCS *rtn = icalloc_WCS(iwi_len(from));
	iwi_cpy(rtn, from);
	return rtn;
}
/* (��)
	MBS *from = "abcde";
	PL2(ims_pclone(from, from + 3)); //=> "abc"
*/
// v2021-04-19
MBS
*ims_pclone(
	MBS *from1,
	MBS *from2
)
{
	MBS *rtn = icalloc_MBS(from2 - from1);
	imi_pcpy(rtn, from1, from2);
	return rtn;
}
/* (��)
	// �v�f���Ăяo���x realloc ��������X�}�[�g�����A
	// ���x�ɕs��������̂� icalloc �P��ōς܂���B
	MBS *p1 = "123";
	MBS *p2 = "abcde";
	MBS *p3 = "�����";
	MBS *rtn = ims_cats(3, p1, p2, p3); //=> "123abcde�����"
	PL2(rtn);
	ifree(rtn);
*/
// v2021-12-01
MBS
*ims_cats(
	UINT size, // �v�f��(n+1)
	...        // ary[0..n]
)
{
	UINT uSize = 0;
	UINT u1 = 0;

	va_list va;
	va_start(va, size);
		u1 = size;
		while(u1--)
		{
			uSize += imi_len(va_arg(va, MBS*));
		}
	va_end(va);

	MBS *rtn = icalloc_MBS(uSize);
	MBS *pEnd = rtn;

	va_start(va, size);
		u1 = size;
		while(u1--)
		{
			pEnd += imi_cpy(pEnd, va_arg(va, MBS*));
		}
	va_end(va);

	return rtn;
}
//--------------------------------
// lstrcmp()�^lstrcmpi()�����S
//--------------------------------
/*
	lstrcmp() �͑召��r�������Ȃ�(TRUE = 0, FALSE = 1 or -1)�̂ŁA
	��r���镶���񒷂𑵂��Ă��K�v������B
*/
/* (��)
	PL3(imb_cmp("", "abc", FALSE, FALSE));   //=> FALSE
	PL3(imb_cmp("abc", "", FALSE, FALSE));   //=> TRUE
	PL3(imb_cmp("", "", FALSE, FALSE));      //=> TRUE
	PL3(imb_cmp(NULL, "abc", FALSE, FALSE)); //=> FALSE
	PL3(imb_cmp("abc", NULL, FALSE, FALSE)); //=> FALSE
	PL3(imb_cmp(NULL, NULL, FALSE, FALSE));  //=> FALSE
	PL3(imb_cmp(NULL, "", FALSE, FALSE));    //=> FALSE
	PL3(imb_cmp("", NULL, FALSE, FALSE));    //=> FALSE
	NL();
	// imb_cmpf(p, search)  <= imb_cmp(p, search, FALSE, FALSE)
	PL3(imb_cmp("abc", "AB", FALSE, FALSE)); //=> FALSE
	// imb_cmpfi(p, search) <= imb_cmp(p, search, FALSE, TRUE)
	PL3(imb_cmp("abc", "AB", FALSE, TRUE));  //=> TRUE
	// imb_cmpp(p, search)  <= imb_cmp(p, search, TRUE, FALSE)
	PL3(imb_cmp("abc", "AB", TRUE, FALSE));  //=> FALSE
	// imb_cmppi(p, search) <= imb_cmp(p, search, TRUE, TRUE)
	PL3(imb_cmp("abc", "AB", TRUE, TRUE));   //=> FALSE
	NL();
	// search�ɂP�����ł����v�����TRUE��Ԃ�
	PL3(imb_cmp_leq(""   , "..", TRUE));     //=> TRUE
	PL3(imb_cmp_leq("."  , "..", TRUE));     //=> TRUE
	PL3(imb_cmp_leq(".." , "..", TRUE));     //=> TRUE
	PL3(imb_cmp_leq("...", "..", TRUE));     //=> FALSE
	PL3(imb_cmp_leq("...", ""  , TRUE));     //=> FALSE
	NL();
*/
// v2021-04-20
BOOL
imb_cmp(
	MBS *pM,      // �����Ώ�
	MBS *search,  // ����������
	BOOL perfect, // TRUE=������v�^FALSE=�O����v
	BOOL icase    // TRUE=�啶����������ʂ��Ȃ�
)
{
	// NULL �͑��݂��Ȃ��̂� FALSE
	if(!pM || !search)
	{
		return FALSE;
	}
	// ��O "" == "
	if(!*pM && !*search)
	{
		return TRUE;
	}
	// �����Ώ� == "" �̂Ƃ��� FALSE
	if(!*pM)
	{
		return FALSE;
	}
	// int�^�Ŕ�r������������
	INT i1 = 0, i2 = 0;
	if(icase)
	{
		while(*pM && *search)
		{
			i1 = tolower(*pM);
			i2 = tolower(*search);
			if(i1 != i2)
			{
				break;
			}
			++pM;
			++search;
		}
	}
	else
	{
		while(*pM && *search)
		{
			i1 = *pM;
			i2 = *search;
			if(i1 != i2)
			{
				break;
			}
			++pM;
			++search;
		}
	}
	if(perfect)
	{
		// ���������� \0 �Ȃ� ���S��v
		return (!*pM && !*search ? TRUE : FALSE);
	}
	// searchE �̖����� \0 �Ȃ� �O����v
	return (!*search ? TRUE : FALSE);
}
// v2021-04-20
BOOL
iwb_cmp(
	WCS *pW,      // �����Ώ�
	WCS *search,  // ����������
	BOOL perfect, // TRUE=������v�^FALSE=�O����v
	BOOL icase    // TRUE=�啶����������ʂ��Ȃ�
)
{
	// NULL �͑��݂��Ȃ��̂� FALSE
	if(!pW || !search)
	{
		return FALSE;
	}
	// ��O
	if(!*pW && !*search)
	{
		return TRUE;
	}
	// �����Ώ� == "" �̂Ƃ��� FALSE
	if(!*pW)
	{
		return FALSE;
	}
	// int�^�Ŕ�r������������
	INT i1 = 0, i2 = 0;
	if(icase)
	{
		while(*pW && *search)
		{
			i1 = towlower(*pW);
			i2 = towlower(*search);
			if(i1 != i2)
			{
				break;
			}
			++pW;
			++search;
		}
	}
	else
	{
		while(*pW && *search)
		{
			i1 = *pW;
			i2 = *search;
			if(i1 != i2)
			{
				break;
			}
			++pW;
			++search;
		}
	}
	if(perfect)
	{
		// ���������� \0 �Ȃ� ���S��v
		return (!*pW && !*search ? TRUE : FALSE);
	}
	// searchE �̖����� \0 �Ȃ� �O����v
	return (!*search ? TRUE : FALSE);
}
//-------------------------------
// ��������͈͂̏I���ʒu��Ԃ�
//-------------------------------
/* (��)
	MBS *p1 = "<-123, <-4, 5->, 6->->";
	PL2(ijp_bypass(p1, "<-", "->")); //=> "->, 6->->"
*/
// v2014-08-16
MBS
*ijp_bypass(
	MBS *pM,   // ������
	MBS *from, // �����J�n
	MBS *to    // �����I��
)
{
	if(!imb_cmpf(pM, from))
	{
		return pM;
	}
	MBS *rtn = ijp_searchL(CharNextA(pM), to); // *from == *to �΍�
	return (*rtn ? rtn : pM);
}
//-------------------
// ��v��������Ԃ�
//-------------------
/* (��)
	PL3(iji_searchCnti("�\\�\\123�\\", "�\\"));  //=> 3 Word
	PL3(iji_searchCntLi("�\\�\\123�\\", "�\\")); //=> 2 Word
	PL3(iji_searchLenLi("�\\�\\123�\\", "�\\")); //=> 2 Word
	PL3(imi_searchLenLi("�\\�\\123�\\", "�\\")); //=> 4 byte
	PL3(iji_searchCntRi("�\\�\\123�\\", "�\\")); //=> 1 Word
	PL3(iji_searchLenRi("�\\�\\123�\\", "�\\")); //=> 1 Word
	PL3(imi_searchLenRi("�\\�\\123�\\", "�\\")); //=> 2 byte
*/
// v2021-09-20
UINT
iji_searchCntA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	WCS *pW = M2W(pM);
	WCS *wp1 = M2W(search);
	UINT rtn = iwi_searchCntW(pW, wp1, icase);
	ifree(wp1);
	ifree(pW);
	return rtn;
}
// v2014-11-30
UINT
iwi_searchCntW(
	WCS *pW,     // ������
	WCS *search, // ����������
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	if(!pW || !search)
	{
		return 0;
	}
	UINT rtn = 0;
	CONST UINT _searchLen = iwi_len(search);
	while(*pW)
	{
		if(iwb_cmp(pW, search, FALSE, icase))
		{
			pW += _searchLen;
			++rtn;
		}
		else
		{
			++pW;
		}
	}
	return rtn;
}
// v2016-02-11
UINT
iji_searchCntLA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase,  // TRUE=�啶����������ʂ��Ȃ�
	INT option   // 0=���^1=Byte���^2=Word��
)
{
	if(!pM || !search)
	{
		return 0;
	}
	UINT rtn = 0;
	CONST UINT _searchLen = imi_len(search);
	while(*pM)
	{
		if(imb_cmp(pM, search, FALSE, icase))
		{
			pM += _searchLen;
			++rtn;
		}
		else
		{
			break;
		}
	}
	switch(option)
	{
		case(1): rtn *= imi_len(search); break; // Byte��
		case(2): rtn *= iji_len(search); break; // Word��
		default: break;                         // ��
	}
	return rtn;
}
// v2016-02-11
UINT
iji_searchCntRA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase,  // TRUE=�啶����������ʂ��Ȃ�
	INT option   // 0=���^1=Byte���^2=Word��
)
{
	if(!pM || !search)
	{
		return 0;
	}
	UINT rtn = 0;
	CONST UINT _searchLen = imi_len(search);
	MBS *pEnd = pM + imi_len(pM) - _searchLen;
	while(pM <= pEnd)
	{
		if(imb_cmp(pEnd, search, FALSE, icase))
		{
			pEnd -= _searchLen;
			++rtn;
		}
		else
		{
			break;
		}
	}
	switch(option)
	{
		case(1): rtn *= imi_len(search); break; // Byte��
		case(2): rtn *= iji_len(search); break; // Word��
		default: break;                         // ��
	}
	return rtn;
}
//---------------------------------
// ��v����������̃|�C���^��Ԃ�
//---------------------------------
/*
	      "\0����������\0"
	         <= TRUE =>
	R_FALSE��          ��L_FALSE
*/
/* (��)
	PL2(ijp_searchLA("ABCABCDEFABC", "ABC", FALSE)); //=> "ABCABCDEFABC"
*/
// v2014-11-29
MBS
*ijp_searchLA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	if(!pM)
	{
		return pM;
	}
	while(*pM)
	{
		if(imb_cmp(pM, search, FALSE, icase))
		{
			break;
		}
		pM = CharNextA(pM);
	}
	return pM;
}
//-------------------------
// ��r�w���q�𐔎��ɕϊ�
//-------------------------
/*
	[-2] "<"  | "!>="
	[-1] "<=" | "!>"
	[ 0] "="  | "!<>" | "!><"
	[ 1] ">=" | "!<"
	[ 2] ">"  | "!<="
	[ 3] "!=" | "<>"  | "><"
*/
// v2021-11-17
INT
icmpOperator_extractHead(
	MBS *pM
)
{
	INT rtn = INT_MAX; // Err�̂Ƃ��� MAX ��Ԃ�
	if(!pM || !*pM || !(*pM == ' ' || *pM == '<' || *pM == '=' || *pM == '>' || *pM == '!'))
	{
		return rtn;
	}

	// �擪�̋󔒂̂ݓ���
	while(*pM == ' ')
	{
		++pM;
	}
	BOOL bNot = FALSE;
	if(*pM == '!')
	{
		++pM;
		bNot = TRUE;
	}
	switch(*pM)
	{
		// [2]">" | [1]">=" | [3]"><"
		case('>'):
			if(pM[1] == '<')
			{
				rtn = 3;
			}
			else
			{
				rtn = (pM[1] == '=' ? 1 : 2);
			}
			break;

		// [0]"="
		case('='):
			rtn = 0;
			break;

		// [-2]"<" | [-1]"<=" | [3]"<>"
		case('<'):
			if(pM[1] == '>')
			{
				rtn = 3;
			}
			else
			{
				rtn = (pM[1] == '=' ? -1 : -2);
			}
			break;
	}
	if(bNot)
	{
		rtn += (rtn > 0 ? -3 : 3);
	}
	return rtn;
}
//---------------------------------------------------
// icmpOperator_extractHead()�Ŏ擾�����������Ԃ�
//---------------------------------------------------
// v2016-02-11
MBS
*icmpOperator_toHeadA(
	INT operator
)
{
	if(operator > 3 || operator < -2)
	{
		return NULL;
	}
	if(operator == -2)
	{
		return "<";
	}
	if(operator == -1)
	{
		return "<=";
	}
	if(operator ==  0)
	{
		return "=" ;
	}
	if(operator ==  1)
	{
		return ">=";
	}
	if(operator ==  2)
	{
		return ">" ;
	}
	if(operator ==  3)
	{
		return "!=";
	}
	return NULL;
}
//-------------------------------------------------------
// icmpOperator_extractHead()�Ŏ擾������r�w���q�Ŕ�r
//-------------------------------------------------------
// v2015-12-31
BOOL
icmpOperator_chk_INT(
	INT i1,
	INT i2,
	INT operator // [-2..3]
)
{
	if(operator == -2 && i1 < i2)
	{
		return TRUE;
	}
	if(operator == -1 && i1 <= i2)
	{
		return TRUE;
	}
	if(operator == 0 && i1 == i2)
	{
		return TRUE;
	}
	if(operator == 1 && i1 >= i2)
	{
		return TRUE;
	}
	if(operator == 2 && i1 > i2)
	{
		return TRUE;
	}
	if(operator == 3 && i1 != i2)
	{
		return TRUE;
	}
	return FALSE;
}
// v2015-12-31
BOOL
icmpOperator_chk_INT64(
	INT64 i1,
	INT64 i2,
	INT operator // [-2..3]
)
{
	if(operator == -2 && i1 < i2)
	{
		return TRUE;
	}
	if(operator == -1 && i1 <= i2)
	{
		return TRUE;
	}
	if(operator == 0 && i1 == i2)
	{
		return TRUE;
	}
	if(operator == 1 && i1 >= i2)
	{
		return TRUE;
	}
	if(operator == 2 && i1 > i2)
	{
		return TRUE;
	}
	if(operator == 3 && i1 != i2)
	{
		return TRUE;
	}
	return FALSE;
}
// v2015-12-26
BOOL
icmpOperator_chkDBL(
	DOUBLE d1,   //
	DOUBLE d2,   //
	INT operator // [-2..3]
)
{
	if(operator == -2 && d1 < d2)
	{
		return TRUE;
	}
	if(operator == -1 && d1 <= d2)
	{
		return TRUE;
	}
	if(operator == 0 && d1 == d2)
	{
		return TRUE;
	}
	if(operator == 1 && d1 >= d2)
	{
		return TRUE;
	}
	if(operator == 2 && d1 > d2)
	{
		return TRUE;
	}
	if(operator == 3 && d1 != d2)
	{
		return TRUE;
	}
	return FALSE;
}
//---------------------------
// ������𕪊����z����쐬
//---------------------------
/* (��)
	MBS *pM = "2014�N 4��29���@18��42��00�b";
	MBS *tokensM = "�N���������b�@ ";
	MBS **as1 =
		ija_split(pM, tokensM); //=> [0]2014, [1]4, [2]29, [3]18, [4]42, [5]00
	//	ija_split(pM, "");      // 1�������Ԃ�
	//	ija_split_zero(pM);     // ��
	//	ija_split(pM, NULL);    // ��
		iary_print(as1);
	ifree(as1);
*/
// v2021-11-17
MBS
**ija_split(
	MBS *pM,     // ��������
	MBS *tokensM // ��ؕ����^���� (��) "\t\r\n"
)
{
	MBS **rtn = {0};

	if(!pM || !*pM)
	{
		rtn = icalloc_MBS_ary(1);
		rtn[0] = ims_clone(pM);
		return rtn;
	}

	if(!tokensM || !*tokensM)
	{
		return ija_split_zero(pM);
	}

	WCS *pW = M2W(pM);
	WCS *tokensW = M2W(tokensM);

	UINT uAry = 0;
	rtn = icalloc_MBS_ary(uAry + 1);

	WCS *wp1 = wcstok(pW, tokensW);
	rtn[uAry] = W2M(wp1);
	while(wp1)
	{
		wp1 = wcstok(NULL, tokensW);
		++uAry;
		rtn = irealloc_MBS_ary(rtn, (uAry + 1));
		rtn[uAry] = W2M(wp1);
	}

	ifree(tokensW);
	ifree(pW);

	return rtn;
}
//---------------------------
// �P�����Â�؂��Ĕz��
//---------------------------
/* (��)
	MBS **as1 = ija_split_zero("ABC������");
		iary_print(as1); //=> A, B, C, ��, ��, ��
	ifree(as1);
*/
// v2021-11-17
MBS
**ija_split_zero(
	MBS *pM
)
{
	if(!pM)
	{
		return ima_null();
	}
	UINT pLen = iji_len(pM);
	MBS **rtn = icalloc_MBS_ary(pLen);
	MBS *pBgn = pM;
	MBS *pEnd = 0;
	UINT u1 = 0;
	while(u1 < pLen)
	{
		pEnd = CharNextA(pBgn);
		rtn[u1] = ims_pclone(pBgn, pEnd);
		pBgn = pEnd;
		++u1;
	}
	return rtn;
}
//----------------------
// quote��������������
//----------------------
/* (��)
	PL2(ijs_rm_quote("[[ABC]", "[", "]", TRUE, TRUE));            //=> "[ABC"
	PL2(ijs_rm_quote("[[ABC]", "[", "]", TRUE, FALSE));           //=> "ABC"
	PL2(ijs_rm_quote("<A>123</A>", "<A>", "</A>", TRUE,  TRUE));  //=> "123"
	PL2(ijs_rm_quote("<A>123</A>", "<A>", "</A>", FALSE, TRUE));  //=> "123"
	PL2(ijs_rm_quote("<A>123</A>", "<A>", "</a>", TRUE,  TRUE));  //=> "123"
	PL2(ijs_rm_quote("<A>123</A>", "<A>", "</a>", FALSE, FALSE)); //=> "123</A>"
*/
// v2021-11-17
MBS
*ijs_rm_quote(
	MBS *pM,      // ������
	MBS *quoteL,  // ��������擪������
	MBS *quoteR,  // �������閖��������
	BOOL icase,   // TRUE=�啶����������ʂ��Ȃ�
	BOOL oneToOne // TRUE=quote�����O���[�v�Ƃ��ď���
)
{
	if(!pM || !*pM)
	{
		return pM;
	}
	MBS *rtn = 0, *quoteL2 = 0, *quoteR2 = 0;
	// �召���
	if(icase)
	{
		rtn = ims_lower(pM);
		quoteL2 = ims_lower(quoteL);
		quoteR2 = ims_lower(quoteR);
	}
	else
	{
		rtn = ims_clone(pM);
		quoteL2 = ims_clone(quoteL);
		quoteR2 = ims_clone(quoteR);
	}
	// �擪��quote��
	CONST UINT quoteL2Len = imi_len(quoteL2);
	UINT quoteL2Cnt = iji_searchCntL(rtn, quoteL2);
	// ������quote��
	CONST UINT quoteR2Len = imi_len(quoteR2);
	UINT quoteR2Cnt = iji_searchCntR(rtn, quoteR2);
	// ifree()
	ifree(quoteR2);
	ifree(quoteL2);
	// �΂̂Ƃ��A�Ⴂ����quote�����擾
	if(oneToOne)
	{
		quoteL2Cnt = quoteR2Cnt = (quoteL2Cnt < quoteR2Cnt ? quoteL2Cnt : quoteR2Cnt);
	}
	// �召���
	if(icase)
	{
		ifree(rtn);
		rtn = ims_clone(pM); // ���̕�����ɒu��
	}
	// �擪�Ɩ�����quote��΂ŏ���
	imb_shiftL(rtn, (quoteL2Len * quoteL2Cnt));        // �擪�ʒu���V�t�g
	rtn[imi_len(rtn) - (quoteR2Len * quoteR2Cnt)] = 0; // ������NULL���
	return rtn;
}
//---------------------
// �������������؂�
//---------------------
/* (��)
	PL2(ims_addTokenNStr("+000123456.7890"));    //=> "+123,456.7890"
	PL2(ims_addTokenNStr(".000123456.7890"));    //=> "0.000123456.7890"
	PL2(ims_addTokenNStr("+.000123456.7890"));   //=> "+0.000123456.7890"
	PL2(ims_addTokenNStr("0000abcdefg.7890"));   //=> "0abcdefg.7890"
	PL2(ims_addTokenNStr("1234abcdefg.7890"));   //=> "1,234abcdefg.7890"
	PL2(ims_addTokenNStr("+0000abcdefg.7890"));  //=> "+0abcdefg.7890"
	PL2(ims_addTokenNStr("+1234abcdefg.7890"));  //=> "+1,234abcdefg.7890"
	PL2(ims_addTokenNStr("+abcdefg.7890"));      //=> "+abcdefg0.7890"
	PL2(ims_addTokenNStr("�}1234567890.12345")); //=> "�}1,234,567,890.12345"
	PL2(ims_addTokenNStr("aiu������@���"));     //=> "aiu������@���"
*/
// v2021-04-19
MBS
*ims_addTokenNStr(
	MBS *pM
)
{
	if(!pM || !*pM)
	{
		return pM;
	}
	UINT u1 = imi_len(pM);
	UINT u2 = 0;
	MBS *rtn = icalloc_MBS(u1 * 2);
	MBS *pRtnE = rtn;
	MBS *p1 = 0;

	// "-000123456.7890" �̂Ƃ�
	// (1) �擪�� [\S*] ��T��
	MBS *pBgn = pM;
	MBS *pEnd = pM;
	while(*pEnd)
	{
		if((*pEnd >= '0' && *pEnd <= '9') || *pEnd == '.')
		{
			break;
		}
		++pEnd;
	}
	pRtnE += imi_pcpy(pRtnE, pBgn, pEnd);

	// (2) [0-9] �Ԃ�T�� => "000123456"
	pBgn = pEnd;
	while(*pEnd)
	{
		if(*pEnd < '0' || *pEnd > '9')
		{
			break;
		}
		++pEnd;
	}

	// (2-11) �擪�� [.] ���H
	if(*pBgn == '.')
	{
		*pRtnE = '0';
		++pRtnE;
		imi_cpy(pRtnE, pBgn);
	}

	// (2-21) �A������ �擪��[0] �𒲐� => "123456"
	else
	{
		while(*pBgn)
		{
			if(*pBgn != '0')
			{
				break;
			}
			++pBgn;
		}
		if(*(pBgn - 1) == '0' && (*pBgn < '0' || *pBgn > '9'))
		{
			--pBgn;
		}

		// (2-22) ", " �t�^ => "123,456"
		p1 = ims_pclone(pBgn, pEnd);
			u1 = pEnd - pBgn;
			if(u1 > 3)
			{
				u2 = u1 % 3;
				if(u2)
				{
					pRtnE += imi_pcpy(pRtnE, p1, p1 + u2);
				}
				while(u2 < u1)
				{
					if(u2 > 0 && u2 < u1)
					{
						*pRtnE = ',';
						++pRtnE;
					}
					pRtnE += imi_pcpy(pRtnE, p1 + u2, p1 + u2 + 3);
					u2 += 3;
				}
			}
			else
			{
				pRtnE += imi_cpy(pRtnE, p1);
			}
		ifree(p1);

		// (2-23) �c�� => ".7890"
		imi_cpy(pRtnE, pEnd);
	}
	return rtn;
}
//-----------------------
// ���E�̕�������������
//-----------------------
/* (��)
	PL2(ijs_cut(" \tABC\t ", " \t", " \t")); //=> "ABC"
*/
// v2021-09-24
MBS
*ijs_cut(
	MBS *pM,
	MBS *rmLs,
	MBS *rmRs
)
{
	MBS *rtn = 0;
	MBS **aryLs = ija_split_zero(rmLs);
	MBS **aryRs = ija_split_zero(rmRs);
		rtn = ijs_cutAry(pM, aryLs, aryRs);
	ifree(aryRs);
	ifree(aryLs);
	return rtn;
}
// v2021-11-17
MBS
*ijs_cutAry(
	MBS *pM,
	MBS **aryLs,
	MBS **aryRs
)
{
	if(!pM)
	{
		return NULL;
	}
	BOOL execL = (aryLs && *aryLs ? TRUE : FALSE);
	BOOL execR = (aryRs && *aryRs ? TRUE : FALSE);
	UINT i1 = 0;
	MBS *pBgn = pM;
	MBS *pEnd = pBgn + imi_len(pBgn);
	MBS *p1 = 0;
	// �擪
	if(execL)
	{
		while(*pBgn)
		{
			i1 = 0;
			while((p1 = aryLs[i1]))
			{
				if(imb_cmpf(pBgn, p1))
				{
					break;
				}
				++i1;
			}
			if(!p1)
			{
				break;
			}
			pBgn = CharNextA(pBgn);
		}
	}
	// ����
	if(execR)
	{
		pEnd = CharPrevA(0, pEnd);
		while(*pEnd)
		{
			i1 = 0;
			while((p1 = aryRs[i1]))
			{
				if(imb_cmpf(pEnd, p1))
				{
					break;
				}
				++i1;
			}
			if(!p1)
			{
				break;
			}
			pEnd = CharPrevA(0, pEnd);
		}
		pEnd = CharNextA(pEnd);
	}
	return ims_pclone(pBgn, pEnd);
}
// v2021-04-20
MBS
*ARY_SPACE[] = {
	"\n",
	"\r",
	"\t",
	"\x20",     // " "
	"\x81\x40", // "�@"
	NULL
};
// v2014-11-05
MBS
*ijs_trim(
	MBS *pM
)
{
	return ijs_cutAry(pM, ARY_SPACE, ARY_SPACE);
}
// v2014-11-05
MBS
*ijs_trimL(
	MBS *pM
)
{
	return ijs_cutAry(pM, ARY_SPACE, NULL);
}
// v2014-11-05
MBS
*ijs_trimR(
	MBS *pM
)
{
	return ijs_cutAry(pM, NULL, ARY_SPACE);
}
// v2014-11-05
MBS
*ARY_CRLF[] = {
	"\r",
	"\n",
	NULL
};
// v2014-11-05
MBS
*ijs_chomp(
	MBS *pM
)
{
	return ijs_cutAry(pM, NULL, ARY_CRLF);
}
//-------------
// ������u��
//-------------
/* (��)
	PL2(ijs_replace("100YEN yen", "YEN", "�~", TRUE));  //=> "100�~ �~"
	PL2(ijs_replace("100YEN yen", "YEN", "�~", FALSE)); //=> "100�~ yen"
*/
// v2021-09-22
MBS
*ijs_replace(
	MBS *from,   // ������
	MBS *before, // �ϊ��O�̕�����
	MBS *after,  // �ϊ���̕�����
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	// "\\" "�\\" �ɑΉ����邽�� MBS => WCS => MBS �̏��ŕϊ�
	WCS *fromW = M2W(from);
	WCS *beforeW = M2W(before);
	WCS *afterW = M2W(after);
	WCS *rtnW = iws_replace(fromW, beforeW, afterW, icase);

	MBS *rtn = W2M(rtnW);

	ifree(rtnW);
	ifree(afterW);
	ifree(beforeW);
	ifree(fromW);

	return rtn;
}
// v2021-09-22
WCS
*iws_replace(
	WCS *from,   // ������
	WCS *before, // �ϊ��O�̕�����
	WCS *after,  // �ϊ���̕�����
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	if(!from || !*from || !before || !*before)
	{
		return iws_clone(from);
	}

	WCS *fW = 0;
	WCS *bW = 0;

	if(icase)
	{
		fW = iws_clone(from);
		CharLowerW(fW);
		bW = iws_clone(before);
		CharLowerW(bW);
	}
	else
	{
		fW = from;
		bW = before;
	}

	UINT fWLen = iwi_len(fW);
	UINT bWLen = iwi_len(bW);
	UINT afterLen = iwi_len(after);

	WCS *fWB = fW;

	WCS *rtn = icalloc_WCS(fWLen * afterLen);
	WCS *rtnE = rtn;

	while(*fWB)
	{
		if(!wcsncmp(fWB, bW, bWLen))
		{
			rtnE += iwi_cpy(rtnE, after);
			fWB += bWLen;
		}
		else
		{
			*rtnE++ = *fWB++;
		}
	}

	if(icase)
	{
		ifree(bW);
		ifree(fW);
	}

	return rtn;
}
//-----------------------------
// ���I�����̕����ʒu���V�t�g
//-----------------------------
/* (��)
	MBS *p1 = ims_clone("123456789");
	PL2(p1); //=> "123456789"
	imb_shiftL(p1, 3);
	PL2(p1); //=> "456789"
	imb_shiftR(p1, 3);
	PL2(p1); //=> "456"
*/
// v2016-02-16
BOOL
imb_shiftL(
	MBS *pM,
	UINT byte
)
{
	if(!byte || !pM || !*pM)
	{
		return FALSE;
	}
	UINT u1 = imi_len(pM);
	if(byte > u1)
	{
		byte = u1;
	}
	memcpy(pM, (pM + byte), (u1 - byte + 1)); // NULL���R�s�[
	return TRUE;
}
// v2021-11-17
BOOL
imb_shiftR(
	MBS *pM,
	UINT byte
)
{
	if(!byte || !pM || !*pM)
	{
		return FALSE;
	}
	UINT u1 = imi_len(pM);
	if(byte > u1)
	{
		byte = u1;
	}
	pM[u1 - byte] = 0;
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	�����֌W
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-------------------------------------
// �����𖳎������ʒu�Ő��l�ɕϊ�����
//-------------------------------------
/* (��)
	PL3(inum_atoi64("-0123.45")); //=> -123
*/
// v2015-12-31
INT64
inum_atoi(
	MBS *pM // ������
)
{
	if(!pM || !*pM)
	{
		return 0;
	}
	while(*pM)
	{
		if(inum_chkM(pM))
		{
			break;
		}
		++pM;
	}
	return _atoi64(pM);
}
/* (��)
	PL4(inum_atof("-0123.45")); //=> -123.45000000
*/
// v2015-12-31
DOUBLE
inum_atof(
	MBS *pM // ������
)
{
	if(!pM || !*pM)
	{
		return 0;
	}
	while(*pM)
	{
		if(inum_chkM(pM))
		{
			break;
		}
		++pM;
	}
	return atof(pM);
}
//-----------------------------------------------------------------------------------------
// Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura, All rights reserved.
//  A C-program for MT19937, with initialization improved 2002/1/26.
//  Coded by Takuji Nishimura and Makoto Matsumoto.
//-----------------------------------------------------------------------------------------
/*
	http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/mt19937ar.c
	��L�R�[�h�����Ɉȉ��̊֐��ɂ��ăJ�X�^�}�C�Y���s�����B
	MT�֘A�̍ŐV���i�h���ł�SFMT�ATinyMT�Ȃǁj�ɂ��Ă͉��L���Q�Ƃ̂���
		http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
*/
/* (��)
INT
main()
{
	CONST INT Output = 10; // �o�͐�
	CONST INT Min    = -5; // �ŏ��l(>=INT_MIN)
	CONST INT Max    =  5; // �ő�l(<=INT_MAX)

	MT_initByAry(TRUE); // ������

	for(INT _i1 = 0; _i1 < Output; _i1++)
	{
		P4(MT_irandDBL(Min, Max, 10));
	}

	MT_freeAry(); // ���

	return 0;
}
*/
/*
	Period parameters
*/
#define  MT_N 624
#define  MT_M 397
#define  MT_MATRIX_A         0x9908b0dfUL // constant vector a
#define  MT_UPPER_MASK       0x80000000UL // most significant w-r bits
#define  MT_LOWER_MASK       0x7fffffffUL // least significant r bits
static UINT MT_i1 = (MT_N + 1);           // MT_i1 == MT_N + 1 means au1[MT_N] is not initialized
static UINT *MT_au1 = 0;                  // the array forthe state vector
// v2021-11-15
VOID
MT_initByAry(
	BOOL fixOn
)
{
	// ��dAlloc���
	MT_freeAry();
	MT_au1 = icalloc(MT_N, sizeof(UINT), FALSE);
	// Seed�ݒ�
	UINT init_key[4];
	INT *ai1 = idate_now_to_iAryYmdhns_localtime();
		init_key[0] = ai1[3] * ai1[5];
		init_key[1] = ai1[3] * ai1[6];
		init_key[2] = ai1[4] * ai1[5];
		init_key[3] = ai1[4] * ai1[6];
	ifree(ai1);
	// fixOn == FALSE �̂Ƃ����ԂŃV���b�t��
	if(!fixOn)
	{
		init_key[3] &= (INT)GetTickCount();
	}
	INT i = 1, j = 0, k = 4;
	while(k)
	{
		MT_au1[i] = (MT_au1[i] ^ ((MT_au1[i - 1] ^ (MT_au1[i - 1] >> 30)) * 1664525UL)) + init_key[j] + j; // non linear
		MT_au1[i] &= 0xffffffffUL; // for WORDSIZE>32 machines
		++i, ++j;
		if(i >= MT_N)
		{
			MT_au1[0] = MT_au1[MT_N - 1];
			i = 1;
		}
		if(j >= MT_N)
		{
			j = 0;
		}
		--k;
	}
	k = MT_N - 1;
	while(k)
	{
		MT_au1[i] = (MT_au1[i] ^ ((MT_au1[i - 1] ^ (MT_au1[i - 1] >> 30)) * 1566083941UL)) - i; // non linear
		MT_au1[i] &= 0xffffffffUL; // for WORDSIZE>32 machines
		++i;
		if(i >= MT_N)
		{
			MT_au1[0] = MT_au1[MT_N - 1];
			i = 1;
		}
		--k;
	}
	MT_au1[0] = 0x80000000UL; // MSB is 1;assuring non-zero initial array
}
/*
	generates a random number on [0, 0xffffffff]-interval
	generates a random number on [0, 0xffffffff]-interval
*/
// v2015-12-31
UINT
MT_genrandUint32()
{
	UINT y = 0;
	static UINT mag01[2] = {0x0UL, MT_MATRIX_A};
	if(MT_i1 >= MT_N)
	{
		// generate N words at one time
		INT kk = 0;
		while(kk < MT_N - MT_M)
		{
			y = (MT_au1[kk] & MT_UPPER_MASK) | (MT_au1[kk + 1] & MT_LOWER_MASK);
			MT_au1[kk] = MT_au1[kk + MT_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
			++kk;
		}
		while(kk < MT_N - 1)
		{
			y = (MT_au1[kk] & MT_UPPER_MASK) | (MT_au1[kk + 1] & MT_LOWER_MASK);
			MT_au1[kk] = MT_au1[kk + (MT_M - MT_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
			++kk;
		}
		y = (MT_au1[MT_N - 1] & MT_UPPER_MASK) | (MT_au1[0] & MT_LOWER_MASK);
		MT_au1[MT_N - 1] = MT_au1[MT_M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];
		MT_i1 = 0;
	}
	y = MT_au1[++MT_i1];
	// Tempering
	y ^= (y >> 11);
	y ^= (y <<  7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);
	return y;
}
// v2015-11-15
VOID
MT_freeAry()
{
	ifree(MT_au1);
}
//----------------
// INT�����𔭐�
//----------------
/* (��)
	MT_initByAry(TRUE);               // ������
	P("%3d\n", MT_irand_INT(0, 100)); // [0..100]
	MT_freeAry();                     // ���
*/
// v2015-12-30
INT
MT_irand_INT(
	INT posMin,
	INT posMax
)
{
	if(posMin > posMax)
	{
		return 0;
	}
	INT rtn = ((MT_genrandUint32() >> 1) % (posMax - posMin + 1)) + posMin;
	return rtn;
}
//-------------------
// DOUBLE�����𔭐�
//-------------------
/* (��)
	MT_initByAry(TRUE);                    // ������
	P("%20.12f\n", MT_irandDBL(0, 10, 5)); // [0.00000..10.00000]
	MT_freeAry();                          // ���
*/
// v2015-12-30
DOUBLE
MT_irandDBL(
	INT posMin,
	INT posMax,
	UINT decRound // [0..10]�^[0]"1", [1]"0.1", .., [10]"0.0000000001"
)
{
	if(posMin > posMax)
	{
		return 0.0;
	}
	if(decRound > 10)
	{
		decRound = 0;
	}
	INT i1 = 1;
	while(decRound > 0)
	{
		i1 *= 10;
		--decRound;
	}
	return (DOUBLE)MT_irand_INT(posMin, (posMax - 1)) + (DOUBLE)MT_irand_INT(0, i1) / i1;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Command Line
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-------------------------
// �R�}���h���^�������擾
//-------------------------
/* (��)
	iCLI_getARGV();
	PL2($CMD);
	iary_print($ARGV);
	PL3($ARGC);
*/
// v2021-11-17
UINT
iCLI_getARGV()
{
	INT iArgc;
	WCS **aW = CommandLineToArgvW(GetCommandLineW(), &iArgc);

	$ARGC = iArgc - 1;
	$CMD = W2M(aW[0]);

	MBS **rtn = icalloc_MBS_ary(iArgc);
	INT i1 = 1;
	while(i1 < iArgc)
	{
		rtn[i1 - 1] = W2M(aW[i1]);
		++i1;
	}
	$ARGV = rtn;

	return iArgc;
}
//---------------------------------------
// ���� [Key+Value] ���� [Value] ���擾
//---------------------------------------
/* (��)
	iCLI_getARGV();
	// $ARGV[0] => "-w=size <= 1000"
	P2(iCLI_getOptValue(0, "-w=", NULL)); //=> "size <= 1000"
*/
// v2021-11-24
MBS
*iCLI_getOptValue(
	UINT argc, // $ARGV[argc]
	MBS *opt1, // (��) "-w="
	MBS *opt2  // (��) "-where=", NULL
)
{
	if(argc >= $ARGC)
	{
		return NULL;
	}

	if(!imi_len(opt1))
	{
		return NULL;
	}
	else if(!imi_len(opt2))
	{
		opt2 = opt1;
	}

	MBS *p1 = $ARGV[argc];

	// �O����v
	if(imb_cmpf(p1, opt1))
	{
		return (p1 + imi_len(opt1));
	}
	else if(imb_cmpf(p1, opt2))
	{
		return (p1 + imi_len(opt2));
	}

	return NULL;
}
//--------------------------
// ���� [Key] �ƈ�v���邩
//--------------------------
/* (��)
	iCLI_getARGV();
	// $ARGV[0] => "-repeat"
	P3(iCLI_getOptMatch(0, "-repeat", NULL)); //=> TRUE
	// $ARGV[0] => "-w=size <= 1000"
	P3(iCLI_getOptMatch(0, "-w=", NULL));     //=> FALSE
*/
// v2021-11-24
BOOL
iCLI_getOptMatch(
	UINT argc, // $ARGV[argc]
	MBS *opt1, // (��) "-r"
	MBS *opt2  // (��) "-repeat", NULL
)
{
	if(argc >= $ARGC)
	{
		return FALSE;
	}

	if(!imi_len(opt1))
	{
		return FALSE;
	}
	else if(!imi_len(opt2))
	{
		opt2 = opt1;
	}

	MBS *p1 = $ARGV[argc];

	// ���S��v
	if(imb_cmpp(p1, opt1) || imb_cmpp(p1, opt2))
	{
		return TRUE;
	}

	return FALSE;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Array
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-----------------
// NULL�z���Ԃ�
//-----------------
/* (��)
	// if(!p) return (MBS**)ima_null();
	PP(ima_null()); NL();  //=> �L���A�h���X
	PP(*ima_null()); NL(); //=> NULL
*/
// v2016-01-19
MBS
**ima_null()
{
	static MBS *ary[1] = {0};
	return (MBS**)ary;
}
//-------------------
// �z��T�C�Y���擾
//-------------------
/* (��)
	MBS **as1 = iCLI_getARGS(); // {"123", "45", "abc", NULL}
	INT i1 = iary_size(as1);    //=> 3
*/
// v2021-09-23
UINT
iary_size(
	MBS **ary // ������
)
{
	UINT rtn = 0;
	while(*ary++)
	{
		++rtn;
	}
	return rtn;
}
//---------------------
// �z��̍��v�����擾
//---------------------
/* (��)
	MBS **as1 = iCLI_getARGS(); // {"123", "���", NULL}
	INT i1 = iary_Mlen(as1);    //=> 7
	INT i2 = iary_Jlen(as1);    //=> 5
*/
// v2021-09-22
UINT
iary_Mlen(
	MBS **ary
)
{
	UINT rtn = 0;
	while(*ary)
	{
		 rtn += imi_len(*ary++);
	}
	return rtn;
}
// v2021-09-22
UINT
iary_Jlen(
	MBS **ary
)
{
	UINT rtn = 0;
	while(*ary)
	{
		 rtn += iji_len(*ary++);
	}
	return rtn;
}
//--------------
// �z���qsort
//--------------
/* (��)
	MBS **as1 = iCLI_getARGS();
	// ���f�[�^
	PL();
	iary_print(as1);
	NL();
	// ���\�[�g
	PL();
	iary_sortAsc(as1);
	iary_print(as1);
	NL();
	// �t���\�[�g
	PL();
	iary_sortDesc(as1);
	iary_print(as1);
	NL();
*/
// v2014-02-07
INT
iary_qsort_cmp(
	CONST VOID *p1, //
	CONST VOID *p2, //
	BOOL asc        // TRUE=�����^FALSE=�~��
)
{
	MBS **p11 = (MBS**)p1;
	MBS **p21 = (MBS**)p2;
	INT rtn = lstrcmpA(*p11, *p21); // �召��ʂ���
	return rtn *= (asc > 0 ? 1 : -1);
}
// v2014-02-07
INT
iary_qsort_cmpAsc(
	CONST VOID *p1,
	CONST VOID *p2
)
{
	return iary_qsort_cmp(p1, p2, TRUE);
}
// v2014-02-07
INT
iary_qsort_cmpDesc(
	CONST VOID *p1,
	CONST VOID *p2
)
{
	return iary_qsort_cmp(p1, p2, FALSE);
}
//---------------------
// �z��𕶎���ɕϊ�
//---------------------
/* (��)
	MBS **as1 = iCLI_getARGS();
	MBS *p1 = iary_join(as1, "\t");
	PL2(p1);
*/
// v2021-09-22
MBS
*iary_join(
	MBS **ary, // �z��
	MBS *token // ��ؕ���
)
{
	UINT arySize = iary_size(ary);
	UINT tokenSize = imi_len(token);
	MBS *rtn = icalloc_MBS(iary_Mlen(ary) + (tokenSize * arySize));
	MBS *pEnd = rtn;
	while(*ary)
	{
		pEnd += imi_cpy(pEnd, *ary++);
		pEnd += imi_cpy(pEnd, token);
	}
	*(pEnd - tokenSize) = 0;
	return rtn;
}
//---------------------------
// �z�񂩂�󔒁^�d��������
//---------------------------
/*
	MBS *args[] = {"aaa", "AAA", "BBB", "", "bbb", NULL};
	MBS **as1 = {0};
	INT i1 = 0;
	//
	// TRUE = �召��ʂ��Ȃ�
	//
	as1 = iary_simplify(args, TRUE);
	i1 = 0;
	while((as1[i1]))
	{
		PL2(as1[i1]); //=> "aaa", "BBB"
		++i1;
	}
	ifree(as1);
	//
	// FALSE = �召��ʂ���
	//
	as1 = iary_simplify(args, FALSE);
	i1 = 0;
	while((as1[i1]))
	{
		PL2(as1[i1]); //=> "aaa", "AAA", "BBB", "bbb"
		++i1;
	}
	ifree(as1);
*/
// v2021-11-17
MBS
**iary_simplify(
	MBS **ary,
	BOOL icase // TRUE=�啶����������ʂ��Ȃ�
)
{
	CONST UINT uArySize = iary_size(ary);
	UINT u1 = 0, u2 = 0;
	// iAryFlg ����
	INT *iAryFlg = icalloc_INT(uArySize); // �����l = 0
	// ��ʂ֏W��
	u1 = 0;
	while(u1 < uArySize)
	{
		if(*ary[u1] && iAryFlg[u1] > -1)
		{
			iAryFlg[u1] = 1; // ��
			u2 = u1 + 1;
			while(u2 < uArySize)
			{
				if(icase)
				{
					if(imb_cmppi(ary[u1], ary[u2]))
					{
						iAryFlg[u2] = -1; // �~
					}
				}
				else
				{
					if(imb_cmpp(ary[u1], ary[u2]))
					{
						iAryFlg[u2] = -1; // �~
					}
				}
				++u2;
			}
		}
		++u1;
	}
	// rtn�쐬
	UINT uAryUsed = 0;
	u1 = 0;
	while(u1 < uArySize)
	{
		if(iAryFlg[u1] == 1)
		{
			++uAryUsed;
		}
		++u1;
	}
	MBS **rtn = icalloc_MBS_ary(uAryUsed);
	u1 = u2 = 0;
	while(u1 < uArySize)
	{
		if(iAryFlg[u1] == 1)
		{
			rtn[u2] = ims_clone(ary[u1]);
			++u2;
		}
		++u1;
	}
	ifree(iAryFlg);
	return rtn;
}
//----------------
// ���Dir�𒊏o
//----------------
/*
	// ���݂��Ȃ�Dir�͖�������
	// �啶������������ʂ��Ȃ�
	MBS *args[] = {"", "D:", "c:\\Windows\\", "a:", "C:", "d:\\TMP", NULL};
	MBS **as1 = iary_higherDir(args);
		iary_print(as1); //=> 'C:\' 'D:\'
	ifree(as1);
*/
// v2021-11-18
MBS
**iary_higherDir(
	MBS **ary
)
{
	UINT uArySize = iary_size(ary);
	MBS **rtn = icalloc_MBS_ary(uArySize);
	UINT u1 = 0, u2 = 0;
	// Dir�����`�^���݂���Dir�̂ݒ��o
	u1 = u2 = 0;
	while(u1 < uArySize)
	{
		if(iFchk_typePathA(ary[u1]) == 1)
		{
			rtn[u2] = iFget_AdirA(ary[u1]);
			++u2;
		}
		++u1;
	}
	uArySize = iary_size(rtn);
	// ���\�[�g
	iary_sortAsc(rtn);
	// ���Dir���擾
	u1 = 0;
	while(u1 < uArySize)
	{
		u2 = u1 + 1;
		while(u2 < uArySize)
		{
			// �O����v�^�召��ʂ��Ȃ�
			if(imb_cmpfi(rtn[u2], rtn[u1]))
			{
				rtn[u2] = "";
				++u2;
			}
			else
			{
				break;
			}
		}
		u1 = u2;
	}
	// ���\�[�g
	iary_sortAsc(rtn);
	// �擪�� "" ��ǂݔ�΂�
	// �L���z���Ԃ�
	u1 = 0;
	while(u1 < uArySize)
	{
		if(*rtn[u1])
		{
			break;
		}
		++u1;
	}
	return (rtn + u1);
}
//-----------
// �z��ꗗ
//-----------
/* (��)
	// �R�}���h���C������
	iary_print($ARGV);

	// �����z��
	MBS *ary[] = {"ABC", "", "12345", NULL};
	iary_print(ary);
*/
// v2021-09-24
VOID
iary_print(
	MBS **ary // ������
)
{
	UINT u1 = 1;
	while(*ary)
	{
		P("[%04d] '%s'\n", u1++, *ary++);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	File/Dir����(WIN32_FIND_DATAA)
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/*
	typedef struct _WIN32_FIND_DATAA
	{
		DWORD dwFileAttributes;
		FILETIME ftCreationTime;   // ctime
		FILETIME ftLastAccessTime; // mtime
		FILETIME ftLastWriteTime;  // atime
		DWORD nFileSizeHigh;
		DWORD nFileSizeLow;
		MBS cFileName[MAX_PATH];
	}
	WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

	typedef struct _FILETIME
	{
		DWORD dwLowDateTime;
		DWORD dwHighDateTime;
	}
	FILETIME;

	typedef struct _SYSTEMTIME
	{
		INT wYear;
		INT wMonth;
		INT wDayOfWeek;
		INT wDay;
		INT wHour;
		INT wMinute;
		INT wSecond;
		INT wMilliseconds;
	}
	SYSTEMTIME;
*/
//-------------------
// �t�@�C�����擾
//-------------------
/* (��1) �����擾����ꍇ
VOID
ifindA(
	$struct_iFinfoA *FI,
	MBS *dir,
	UINT dirLenA
)
{
	WIN32_FIND_DATAA F;
	MBS *p1 = FI->fullnameA + imi_cpy(FI->fullnameA, dir);
		*p1 = '*';
		*(++p1) = 0;
	HANDLE hfind = FindFirstFileA(FI->fullnameA, &F);
		// Dir
		iFinfo_initA(FI, &F, dir, dirLenA, NULL);
			P2(FI->fullnameA);
		// File
		do
		{
			/// PL2(F.cFileName);
			if(iFinfo_initA(FI, &F, dir, dirLenA, F.cFileName))
			{
				// Dir
				if((FI->iFtype) == 1)
				{
					p1 = ims_nclone(FI->fullnameA, FI->iEnd);
						ifindA(FI, p1, FI->iEnd); // Dir(����)
					ifree(p1);
				}
				// File
				else
				{
					P2(FI->fullnameA);
				}
			}
		}
		while(FindNextFileA(hfind, &F));
	FindClose(hfind);
}
// main()
	$struct_iFinfoA *FI = iFinfo_allocA();
		MBS *p1 = ".\\";
		MBS *dir = iFget_AdirA(p1);
		if(dir)
		{
			ifindA(FI, dir, imi_len(dir));
		}
		ifree(dir);
	iFinfo_freeA(FI);
*/
/* (��2) �P��t�@�C��������擾����ꍇ
// main()
	$struct_iFinfoA *FI = iFinfo_allocA();
	MBS *fn = "w32.s";
	if(iFinfo_init2M(FI, fn))
	{
		PL32(FI->iFsize, FI->fullnameA);
		PL2(ijp_forwardN(FI->fullnameA, FI->iFname));
	}
	iFinfo_freeA(FI);
*/
// v2016-08-09
$struct_iFinfoA
*iFinfo_allocA()
{
	return icalloc(1, sizeof($struct_iFinfoA), FALSE);
}
// v2016-08-09
$struct_iFinfoW
*iFinfo_allocW()
{
	return icalloc(1, sizeof($struct_iFinfoW), FALSE);
}
// v2016-08-09
VOID
iFinfo_clearA(
	$struct_iFinfoA *FI
)
{
	*FI->fullnameA = 0;
	FI->iFname = 0;
	FI->iExt = 0;
	FI->iEnd = 0;
	FI->iAttr = 0;
	FI->iFtype = 0;
	FI->cjdCtime = 0.0;
	FI->cjdMtime = 0.0;
	FI->cjdAtime = 0.0;
	FI->iFsize = 0;
}
// v2016-08-09
VOID
iFinfo_clearW(
	$struct_iFinfoW *FI
)
{
	*FI->fullnameW = 0;
	FI->iFname = 0;
	FI->iExt = 0;
	FI->iEnd = 0;
	FI->iAttr = 0;
	FI->iFtype = 0;
	FI->cjdCtime = 0.0;
	FI->cjdMtime = 0.0;
	FI->cjdAtime = 0.0;
	FI->iFsize = 0;
}
//---------------------------
// �t�@�C�����擾�̑O����
//---------------------------
// v2021-04-19
BOOL
iFinfo_initA(
	$struct_iFinfoA *FI,
	WIN32_FIND_DATAA *F,
	MBS *dir, // "\"��t�^���ČĂԁ^iFget_AdirA()�Ő�Βl�ɂ��Ă���
	UINT dirLenA,
	MBS *name
)
{
	// "\." "\.." �͏��O
	if(name && imb_cmp_leqf(name, ".."))
	{
		return FALSE;
	}
	// FI->iAttr
	FI->iAttr = (UINT)F->dwFileAttributes; // DWORD => UINT

	// <32768
	if((FI->iAttr) >> 15)
	{
		iFinfo_clearA(FI);
		return FALSE;
	}

	// FI->fullnameW
	// FI->iFname
	// FI->iEnd
	MBS *p1 = FI->fullnameA + imi_cpy(FI->fullnameA, dir);
	UINT u1 = imi_cpy(p1, name);

	// FI->iFtype
	// FI->iExt
	// FI->iFsize
	if((FI->iAttr & FILE_ATTRIBUTE_DIRECTORY))
	{
		if(u1)
		{
			dirLenA += u1 + 1;
			*((FI->fullnameA) + dirLenA - 1) = '\\'; // "\" �t�^
			*((FI->fullnameA) + dirLenA) = 0;        // NULL �t�^
		}
		(FI->iFtype) = 1;
		(FI->iFname) = (FI->iExt) = (FI->iEnd) = dirLenA;
		(FI->iFsize) = 0;
	}
	else
	{
		(FI->iFtype) = 2;
		(FI->iFname) = dirLenA;
		(FI->iEnd) = (FI->iFname) + u1;
		(FI->iExt) = PathFindExtensionA(FI->fullnameA) - (FI->fullnameA);
		if((FI->iExt) < (FI->iEnd))
		{
			++(FI->iExt);
		}
		(FI->iFsize) = (INT64)F->nFileSizeLow + (F->nFileSizeHigh ? (INT64)(F->nFileSizeHigh) * MAXDWORD + 1 : 0);
	}

	// JST�ϊ�
	// FI->cftime
	// FI->mftime
	// FI->aftime
	if((FI->iEnd) <= 3)
	{
		(FI->cjdCtime) = (FI->cjdMtime) = (FI->cjdAtime) = 2444240.0; // 1980-01-01
	}
	else
	{
		FILETIME ft;
		FileTimeToLocalFileTime(&F->ftCreationTime, &ft);
			(FI->cjdCtime) = iFinfo_ftimeToCjd(ft);
		FileTimeToLocalFileTime(&F->ftLastWriteTime, &ft);
			(FI->cjdMtime) = iFinfo_ftimeToCjd(ft);
		FileTimeToLocalFileTime(&F->ftLastAccessTime, &ft);
			(FI->cjdAtime) = iFinfo_ftimeToCjd(ft);
	}

	return TRUE;
}
// v2021-04-19
BOOL
iFinfo_initW(
	$struct_iFinfoW *FI, //
	WIN32_FIND_DATAW *F, //
	WCS *dir,            // "\"��t�^���ČĂ�
	UINT dirLenW,        //
	WCS *name            //
)
{
	// "\." "\.." �͏��O
	if(name && *name && iwb_cmp_leqf(name, L".."))
	{
		return FALSE;
	}

	// FI->iAttr
	FI->iAttr = (UINT)F->dwFileAttributes; // DWORD => UINT

	// <32768
	if((FI->iAttr) >> 15)
	{
		iFinfo_clearW(FI);
		return FALSE;
	}

	// FI->fullnameW
	// FI->iFname
	// FI->iEnd
	WCS *p1 = FI->fullnameW + iwi_cpy(FI->fullnameW, dir);
	UINT u1 = iwi_cpy(p1, name);

	// FI->iFtype
	// FI->iExt
	// FI->iFsize
	if((FI->iAttr & FILE_ATTRIBUTE_DIRECTORY))
	{
		if(u1)
		{
			dirLenW += u1 + 1;
			*((FI->fullnameW) + dirLenW - 1) = L'\\'; // "\" �t�^
			*((FI->fullnameW) + dirLenW) = 0;         // NULL �t�^
		}
		(FI->iFtype) = 1;
		(FI->iFname) = (FI->iExt) = (FI->iEnd) = dirLenW;
		(FI->iFsize) = 0;
	}
	else
	{
		(FI->iFtype) = 2;
		(FI->iFname) = dirLenW;
		(FI->iEnd) = (FI->iFname) + u1;
		(FI->iExt) = PathFindExtensionW(FI->fullnameW) - (FI->fullnameW);
		if((FI->iExt) < (FI->iEnd))
		{
			++(FI->iExt);
		}
		(FI->iFsize) = (INT64)F->nFileSizeLow + (F->nFileSizeHigh ? (INT64)(F->nFileSizeHigh) * MAXDWORD + 1 : 0);
	}

	// JST�ϊ�
	// FI->cftime
	// FI->mftime
	// FI->aftime
	if((FI->iEnd) <= 3)
	{
		(FI->cjdCtime) = (FI->cjdMtime) = (FI->cjdAtime) = 2444240.0; // 1980-01-01
	}
	else
	{
		FILETIME ft;
		FileTimeToLocalFileTime(&F->ftCreationTime, &ft);
			(FI->cjdCtime) = iFinfo_ftimeToCjd(ft);
		FileTimeToLocalFileTime(&F->ftLastWriteTime, &ft);
			(FI->cjdMtime) = iFinfo_ftimeToCjd(ft);
		FileTimeToLocalFileTime(&F->ftLastAccessTime, &ft);
			(FI->cjdAtime) = iFinfo_ftimeToCjd(ft);
	}
	return TRUE;
}
// v2021-04-19
BOOL
iFinfo_init2M(
	$struct_iFinfoA *FI, //
	MBS *path            // �t�@�C���p�X
)
{
	// ���݃`�F�b�N
	/// PL3(iFchk_existPathA(path));
	if(!iFchk_existPathA(path))
	{
		return FALSE;
	}
	MBS *path2 = iFget_AdirA(path); // ���path��Ԃ�
		INT iFtype = iFchk_typePathA(path2);
		UINT uDirLen = (iFtype == 1 ? imi_len(path2) : (UINT)(PathFindFileNameA(path2) - path2));
		MBS *pDir = (FI->fullnameA); // tmp
			imi_pcpy(pDir, path2, path2 + uDirLen);
		MBS *sName = 0;
			if(iFtype == 1)
			{
				// Dir
				imi_cpy(path2 + uDirLen, "."); // Dir�����p "." �t�^
			}
			else
			{
				// File
				sName = ims_clone(path2 + uDirLen);
			}
			WIN32_FIND_DATAA F;
			HANDLE hfind = FindFirstFileA(path2, &F);
				iFinfo_initA(FI, &F, pDir, uDirLen, sName);
			FindClose(hfind);
		ifree(sName);
	ifree(path2);
	return TRUE;
}
// v2016-08-09
VOID
iFinfo_freeA(
	$struct_iFinfoA *FI
)
{
	ifree(FI);
}
// v2016-08-09
VOID
iFinfo_freeW(
	$struct_iFinfoW *FI
)
{
	ifree(FI);
}
//---------------------
// �t�@�C������ϊ�
//---------------------
/*
	// 1: READONLY
		FILE_ATTRIBUTE_READONLY
	// 2: HIDDEN
		FILE_ATTRIBUTE_HIDDEN
	// 4: SYSTEM
		FILE_ATTRIBUTE_SYSTEM
	// 16: DIRECTORY
		FILE_ATTRIBUTE_DIRECTORY
	// 32: ARCHIVE
		FILE_ATTRIBUTE_ARCHIVE
	// 64: DEVICE
		FILE_ATTRIBUTE_DEVICE
	// 128: NORMAL
		FILE_ATTRIBUTE_NORMAL
	// 256: TEMPORARY
		FILE_ATTRIBUTE_TEMPORARY
	// 512: SPARSE FILE
		FILE_ATTRIBUTE_SPARSE_FILE
	// 1024: REPARSE_POINT
		FILE_ATTRIBUTE_REPARSE_POINT
	// 2048: COMPRESSED
		FILE_ATTRIBUTE_COMPRESSED
	// 8192: NOT CONTENT INDEXED
		FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
	// 16384: ENCRYPTED
		FILE_ATTRIBUTE_ENCRYPTED
*/
// v2021-11-15
MBS
*iFinfo_attrToA(
	UINT iAttr
)
{
	MBS *rtn = icalloc_MBS(6);
	if(!rtn)
	{
		return NULL;
	}
	rtn[0] = (iAttr & FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-'); // 16: Dir
	rtn[1] = (iAttr & FILE_ATTRIBUTE_READONLY  ? 'r' : '-'); //  1: ReadOnly
	rtn[2] = (iAttr & FILE_ATTRIBUTE_HIDDEN    ? 'h' : '-'); //  2: Hidden
	rtn[3] = (iAttr & FILE_ATTRIBUTE_SYSTEM    ? 's' : '-'); //  4: System
	rtn[4] = (iAttr & FILE_ATTRIBUTE_ARCHIVE   ? 'a' : '-'); // 32: Archive
	return rtn;
}
// v2021-09-24
UINT
iFinfo_attrAtoUINT(
	MBS *sAttr // "r, h, s, d, a" => 55
)
{
	if(!sAttr || !*sAttr)
	{
		return 0;
	}
	MBS **ap1 = ija_split_zero(sAttr);
	MBS **ap2 = iary_simplify(ap1, TRUE);
		MBS *p1 = iary_join(ap2, "");
	ifree(ap2);
	ifree(ap1);
	// �������ɕϊ�
	CharLowerA(p1);
	UINT rtn = 0;
	MBS *pE = p1;
	while(*pE)
	{
		// �p�o��
		switch(*pE)
		{
			// 32: ARCHIVE
			case('a'):
				rtn += FILE_ATTRIBUTE_ARCHIVE;
				break;

			// 16: DIRECTORY
			case('d'):
				rtn += FILE_ATTRIBUTE_DIRECTORY;
				break;

			// 4: SYSTEM
			case('s'):
				rtn += FILE_ATTRIBUTE_SYSTEM;
				break;

			// 2: HIDDEN
			case('h'):
				rtn += FILE_ATTRIBUTE_HIDDEN;
				break;

			// 1: READONLY
			case('r'):
				rtn += FILE_ATTRIBUTE_READONLY;
				break;
		}
		++pE;
	}
	ifree(p1);
	return rtn;
}
//* 2015-12-23
MBS
*iFinfo_ftypeToA(
	UINT iFtype
)
{
	MBS *rtn = icalloc_MBS(2);
	switch(iFtype)
	{
		case(1): *rtn = 'd'; break;
		case(2): *rtn = 'f'; break;
		default: *rtn = '-'; break;
	}
	return rtn;
}
/*
	(Local)"c:\" => 0
	(Network)"\\localhost\" => 0
*/
// v2016-08-09
INT
iFinfo_depthA(
	$struct_iFinfoA *FI
)
{
	if(!FI->fullnameA)
	{
		return -1;
	}
	return iji_searchCnt(FI->fullnameA + 2, "\\") - 1;
}
// v2016-08-09
INT
iFinfo_depthW(
	$struct_iFinfoW *FI
)
{
	if(!FI->fullnameW)
	{
		return -1;
	}
	return iwi_searchCnt(FI->fullnameW + 2, L"\\") - 1;
}
//---------------------------
// �t�@�C���T�C�Y�擾�ɓ���
//---------------------------
// v2016-08-09
INT64
iFinfo_fsizeA(
	MBS *Fn // ���̓t�@�C����
)
{
	$struct_iFinfoA *FI = iFinfo_allocA();
	iFinfo_init2M(FI, Fn);
	INT64 rtn = FI->iFsize;
	iFinfo_freeA(FI);
	return rtn;
}
//---------------
// FileTime�֌W
//---------------
/*
	��{�AFILETIME(UTC)�ŏ����B
	�K�v�ɉ����āAJST(UTC+9h)�ɕϊ������l��n�����ƁB
*/
// v2015-12-23
MBS
*iFinfo_ftimeToA(
	FILETIME ft
)
{
	MBS *rtn = icalloc_MBS(32);
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);
	if(st.wYear <= 1980 || st.wYear >= 2099)
	{
		rtn = 0;
	}
	else
	{
		sprintf(
			rtn,
			ISO_FORMAT_DATETIME,
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond
		);
	}
	return rtn;
}
// v2015-01-03
DOUBLE
iFinfo_ftimeToCjd(
	FILETIME ftime
)
{
	INT64 i1 = ((INT64)ftime.dwHighDateTime << 32) + ftime.dwLowDateTime;
	i1 /= 10000000; // (�d�v) MicroSecond �폜
	return ((DOUBLE)i1 / 86400) + 2305814.0;
}
// v2021-11-15
FILETIME
iFinfo_ymdhnsToFtime(
	INT i_y,   // �N
	INT i_m,   // ��
	INT i_d,   // ��
	INT i_h,   // ��
	INT i_n,   // ��
	INT i_s,   // �b
	BOOL reChk // TRUE=�N�����𐳋K���^FALSE=���͒l��M�p
)
{
	SYSTEMTIME st;
	FILETIME ft;
	if(reChk)
	{
		INT *ai = idate_reYmdhns(i_y, i_m, i_d, i_h, i_n, i_s); // ���K��
			i_y = ai[0];
			i_m = ai[1];
			i_d = ai[2];
			i_h = ai[3];
			i_n = ai[4];
			i_s = ai[5];
		ifree(ai);
	}
	st.wYear         = i_y;
	st.wMonth        = i_m;
	st.wDay          = i_d;
	st.wHour         = i_h;
	st.wMinute       = i_n;
	st.wSecond       = i_s;
	st.wMilliseconds = 0;
	SystemTimeToFileTime(&st, &ft);
	return ft;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	File/Dir����
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------
// �t�@�C�������݂���Ƃ�TRUE��Ԃ�
//-----------------------------------
/* (��)
	PL3(iFchk_existPathA("."));    //=> 1
	PL3(iFchk_existPathA(""));     //=> 0
	PL3(iFchk_existPathA("\\"));   //=> 1
	PL3(iFchk_existPathA("\\\\")); //=> 0
*/
// v2015-11-26
BOOL
iFchk_existPathA(
	MBS *path // �t�@�C���p�X
)
{
	if(!path || !*path)
	{
		return FALSE;
	}
	return (PathFileExistsA(path) ? TRUE : FALSE);
}
//---------------------------------
// Dir�^File �����݂��邩�`�F�b�N
//---------------------------------
/* (��)
	// �Ԃ�l
	//  Err  : 0
	//  Dir  : 1
	//  File : 2
	//
	// ���݃`�F�b�N�͂��Ȃ�
	// �K�v�ɉ����� iFchk_existPathA() �őO����
	PL3(iFchk_typePathA("."));                       //=> 1
	PL3(iFchk_typePathA(".."));                      //=> 1
	PL3(iFchk_typePathA("\\"));                      //=> 1
	PL3(iFchk_typePathA("c:\\windows\\"));           //=> 1
	PL3(iFchk_typePathA("c:\\windows\\system.ini")); //=> 2
	PL3(iFchk_typePathA("\\\\Network\\"));           //=> 2(�s���ȂƂ���)
*/
// v2015-11-26
UINT
iFchk_typePathA(
	MBS *path // �t�@�C���p�X
)
{
	if(!path || !*path)
	{
		return 0;
	}
	return (PathIsDirectoryA(path) ? 1 : 2);
}
//-------------------------------
// Binary File �̂Ƃ�TRUE��Ԃ�
//-------------------------------
/* (��)
	PL3(iFchk_Bfile("aaa.exe")); //=> TRUE
	PL3(iFchk_Bfile("aaa.txt")); //=> FALSE
	PL3(iFchk_Bfile("???"));     //=> FALSE (���݂��Ȃ��Ƃ�)
*/
// v2019-08-15
BOOL
iFchk_Bfile(
	MBS *Fn
)
{
	FILE *Fp = fopen(Fn, "rb");
	if(!Fp)
	{
		return FALSE;
	}
	UINT cnt = 0, c = 0, u1 = 0;
	// 64byte�ł͕s���S
	while((c = getc(Fp)) != (UINT)EOF && u1 < 128)
	{
		if(c == 0)
		{
			++cnt;
			break;
		}
		++u1;
	}
	fclose(Fp);
	return (0 < cnt ? TRUE : FALSE);
}
//---------------------
// �t�@�C�������𒊏o
//---------------------
/* (��)
	// ���݂��Ȃ��Ă��@�B�I�ɒ��o
	// �K�v�ɉ����� iFchk_existPathA() �őO����
	MBS *p1 = "c:\\windows\\win.ini";
	PL2(iFget_extPathname(p1, 0)); //=>"c:\windows\win.ini"
	PL2(iFget_extPathname(p1, 1)); //=>"win.ini"
	PL2(iFget_extPathname(p1, 2)); //=>"win"
*/
// v2021-04-19
MBS
*iFget_extPathname(
	MBS *path,
	UINT option
)
{
	if(!path || !*path)
	{
		return 0;
	}
	MBS *rtn = icalloc_MBS(imi_len(path) + 3); // CRLF+NULL
	MBS *pBgn = 0;
	MBS *pEnd = 0;

	// Dir or File ?
	if(PathIsDirectoryA(path))
	{
		if(option < 1)
		{
			pEnd = rtn + imi_cpy(rtn, path);
			// "�\\"�΍�
			if(*CharPrevA(0, pEnd) != '\\')
			{
				*pEnd = '\\'; // "\"
				++pEnd;
				*pEnd = 0;
			}
		}
	}
	else
	{
		switch(option)
		{
			// path
			case(0):
				imi_cpy(rtn, path);
				break;

			// name + ext
			case(1):
				pBgn = PathFindFileNameA(path);
				imi_cpy(rtn, pBgn);
				break;

			// name
			case(2):
				pBgn = PathFindFileNameA(path);
				pEnd = PathFindExtensionA(pBgn);
				imi_pcpy(rtn, pBgn, pEnd);
				break;
		}
	}
	return rtn;
}
//-------------------------------------
// ����Dir �� ���Dir("\"�t��) �ɕϊ�
//-------------------------------------
/* (��)
	// _fullpath() �̉��p
	PL2(iFget_AdirA(".\\"));
*/
// v2021-12-01
MBS
*iFget_AdirA(
	MBS *path // �t�@�C���p�X
)
{
	MBS *p1 = icalloc_MBS(IMAX_PATH);
	MBS *p2 = 0;
	switch(iFchk_typePathA(path))
	{
		// Dir
		case(1):
			p2 = ims_cats(2, path, "\\");
				_fullpath(p1, p2, IMAX_PATH);
			ifree(p2);
			break;
		// File
		case(2):
			_fullpath(p1, path, IMAX_PATH);
			break;
	}
	MBS *rtn = ims_clone(p1);
	ifree(p1);
	return rtn;
}
//----------------------------
// ����Dir �� "\" �t���ɕϊ�
//----------------------------
/* (��)
	// _fullpath() �̉��p
	PL2(iFget_RdirA(".")); => ".\\"
*/
// v2021-11-17
MBS
*iFget_RdirA(
	MBS *path // �t�@�C���p�X
)
{
	MBS *rtn = 0;
	if(PathIsDirectoryA(path))
	{
		rtn = ims_clone(path);
		UINT u1 = iji_searchLenR(rtn, "\\");
		MBS *pEnd = rtn + imi_len(rtn) - u1;
		pEnd[0] = '\\';
		pEnd[1] = 0;
	}
	else
	{
		rtn = ijs_replace(path, "\\\\", "\\", FALSE);
	}
	return rtn;
}
//--------------------
// ���K�w��Dir���쐬
//--------------------
/* (��)
	PL3(imk_dir("aaa\\bbb"));
*/
// v2014-05-03
BOOL
imk_dir(
	MBS *path // �t�@�C���p�X
)
{
	UINT flg = 0;
	MBS *p1 = 0;
	MBS *pBgn = ijs_cut(path, "\\", "\\"); // �O���'\'������
	MBS *pEnd = pBgn;
	while(*pEnd)
	{
		pEnd = ijp_searchL(pEnd, "\\");
		p1 = ims_pclone(pBgn, pEnd);
			if(CreateDirectory(p1, 0))
			{
				++flg;
			}
		ifree(p1);
		++pEnd;
	}
	ifree(pBgn);
	return (flg ? TRUE : FALSE);
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Console
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//---------------------------------
// ���݂̕����E��ʂ̕\���F�𓾂�
//---------------------------------
/* (��)
	��ʐF��ݒ肷��ꍇ�A
	�o�b�t�@�s���ɂȂ�ƕ\���ɕs�����������i�o�O�ł͂Ȃ��j�B
	�s�x cls ����Ύ���B
*/
/* (��)
	// [�����F]+([�w�i�F]*16)
	//  0 = Black    1 = Navy     2 = Green    3 = Teal
	//  4 = Maroon   5 = Purple   6 = Olive    7 = Silver
	//  8 = Gray     9 = Blue    10 = Lime    11 = Aqua
	// 12 = Red     13 = Fuchsia 14 = Yellow  15 = White
	iConsole_getColor();
		//=> $ColorDefault // ���ݐF��ۑ�
		//=> $StdoutHandle // ��ʃn���h����ۑ�
	iConsole_setTextColor(9 + (15 * 16)); // �ύX
	iConsole_setTextColor(-1); // ���̐F�ɖ߂�
*/
// v2021-03-19
UINT
iConsole_getColor()
{
	$StdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo($StdoutHandle, &info);
	$ColorDefault = info.wAttributes;
	return $ColorDefault;
}
// v2021-03-19
VOID
iConsole_setTextColor(
	INT rgb // �\���F
)
{
	if(rgb < 0)
	{
		rgb = $ColorDefault;
	}
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, rgb);
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	��
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//--------------------------
// �󔒗�F1582/10/5-10/14
//--------------------------
// {-4712-1-1����̒ʎZ��, yyyymmdd}
INT NS_BEFORE[2] = {2299160, 15821004};
INT NS_AFTER[2]  = {2299161, 15821015};
//-------------------------
// �j���\���ݒ� [7]=Err�l
//-------------------------
MBS *WDAYS[8] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", "**"};
//-----------------------
// ������(-1y12m - 12m)
//-----------------------
INT MDAYS[13] = {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//---------------
// �[�N�`�F�b�N
//---------------
/* (��)
	idate_chk_uruu(2012); //=> TRUE
	idate_chk_uruu(2013); //=> FALSE
*/
// v2013-03-21
BOOL
idate_chk_uruu(
	INT i_y // �N
)
{
	if(i_y > (INT)(NS_AFTER[1] / 10000))
	{
		if((i_y % 400) == 0)
		{
			return TRUE;
		}
		if((i_y % 100) == 0)
		{
			return FALSE;
		}
	}
	return ((i_y % 4) == 0 ? TRUE : FALSE);
}
//-------------
// ���𐳋K��
//-------------
/* (��)
	INT *ai = idate_cnv_month(2011, 14, 1, 12);
	for(INT _i1 = 0; _i1 < 2; _i1++)
	{
		PL3(ai[_i1]); //=> 2012, 2
	}
	ifree(ai);
*/
// v2021-11-15
INT
*idate_cnv_month(
	INT i_y,    // �N
	INT i_m,    // ��
	INT from_m, // �J�n��
	INT to_m    // �I����
)
{
	INT *rtn = icalloc_INT(2);
	while(i_m < from_m)
	{
		i_m += 12;
		i_y -= 1;
	}
	while(i_m > to_m)
	{
		i_m -= 12;
		i_y += 1;
	}
	rtn[0] = i_y;
	rtn[1] = i_m;
	return rtn;
}
//---------------
// ��������Ԃ�
//---------------
/* (��)
	idate_month_end(2012, 2); //=> 29
	idate_month_end(2013, 2); //=> 28
*/
// v2021-11-15
INT
idate_month_end(
	INT i_y, // �N
	INT i_m  // ��
)
{
	INT *ai = idate_cnv_month1(i_y, i_m);
	INT i_d = MDAYS[ai[1]];
	// �[�Q������
	if(ai[1] == 2 && idate_chk_uruu(ai[0]))
	{
		i_d = 29;
	}
	ifree(ai);
	return i_d;
}
//-----------------
// ���������ǂ���
//-----------------
/* (��)
	PL3(idate_chk_month_end(2012, 2, 28, FALSE)); //=> FALSE
	PL3(idate_chk_month_end(2012, 2, 29, FALSE)); //=> TRUE
	PL3(idate_chk_month_end(2012, 1, 60, TRUE )); //=> TRUE
	PL3(idate_chk_month_end(2012, 1, 60, FALSE)); //=> FALSE
*/
// v2021-11-15
BOOL
idate_chk_month_end(
	INT i_y,   // �N
	INT i_m,   // ��
	INT i_d,   // ��
	BOOL reChk // TRUE=�N�����𐳋K���^FALSE=���͒l��M�p
)
{
	if(reChk)
	{
		INT *ai1 = idate_reYmdhns(i_y, i_m, i_d, 0, 0, 0); // ���K��
			i_y = ai1[0];
			i_m = ai1[1];
			i_d = ai1[2];
		ifree(ai1);
	}
	return (i_d == idate_month_end(i_y, i_m) ? TRUE : FALSE);
}
//-----------------------
// str��������CJD�ɕϊ�
//-----------------------
/* (��)
	PL4(idate_MBStoCjd(">[1970-12-10] and <[2015-12-10]")); //=> 2440931.00000000
	PL4(idate_MBStoCjd(">[1970-12-10]"));                   //=> 2440931.00000000
	PL4(idate_MBStoCjd(">[+1970-12-10]"));                  //=> 2440931.00000000
	PL4(idate_MBStoCjd(">[0000-00-00]"));                   //=> 1721026.00000000
	PL4(idate_MBStoCjd(">[-1970-12-10]"));                  //=> 1001859.00000000
	PL4(idate_MBStoCjd(">[2016-01-02]"));                   //=> 2457390.00000000
	PL4(idate_MBStoCjd(">[0D]"));                           // "2016-01-02 10:44:08" => 2457390.00000000
	PL4(idate_MBStoCjd(">[]"));                             // "2016-01-02 10:44:08" => 2457390.44731481
	PL4(idate_MBStoCjd(""));                                //=> DBL_MAX
*/
// v2021-11-15
DOUBLE
idate_MBStoCjd(
	MBS *pM
)
{
	if(!pM || !*pM)
	{
		return DBL_MAX; // Err�̂Ƃ��� MAX ��Ԃ�
	}
	INT *ai = idate_now_to_iAryYmdhns_localtime();
	MBS *p1 = idate_replace_format_ymdhns(
		pM,
		"[", "]",
		"\t", // flg
		ai[0], ai[1], ai[2], ai[3], ai[4], ai[5]
	);
	ifree(ai);
	// flg�`�F�b�N
	MBS *p2 = p1;
	while(*p2)
	{
		if(*p2 == '\t')
		{
			++p2;
			break;
		}
		++p2;
	}
	ai = idate_MBS_to_iAryYmdhns(p2);
	DOUBLE rtn = idate_ymdhnsToCjd(ai[0], ai[1], ai[2], ai[3], ai[4], ai[5]);
	ifree(p1);
	ifree(ai);
	return rtn;
}
//--------------------
// str��N�����ɕ���
//--------------------
/* (��)
	MBS **as1 = idate_MBS_to_mAryYmdhns("-2012-8-12 12:45:00");
		iary_print(as1); //=> -2012, 8, 12, 12, 45, 00
	ifree(as1);
*/
// v2021-11-17
MBS
**idate_MBS_to_mAryYmdhns(
	MBS *pM // (��) "2012-03-12 13:40:00"
)
{
	BOOL bMinus = (pM && *pM == '-' ? TRUE : FALSE);
	MBS **rtn = ija_split(pM, "/.-: ");
	if(bMinus)
	{
		MBS *p1 = rtn[0];
		memmove(p1 + 1, p1, imi_len(p1) + 1);
		p1[0] = '-';
	}
	return rtn;
}
//-------------------------
// str��N�����ɕ���(int)
//-------------------------
/* (��)
	INT *ai1 = idate_MBS_to_iAryYmdhns("-2012-8-12 12:45:00");
	for(INT _i1 = 0; _i1 < 6; _i1++)
	{
		PL3(ai1[_i1]); //=> -2012, 8, 12, 12, 45, 0
	}
	ifree(ai1);
*/
// v2021-11-17
INT
*idate_MBS_to_iAryYmdhns(
	MBS *pM // (��) "2012-03-12 13:40:00"
)
{
	INT *rtn = icalloc_INT(6);
	MBS **as1 = idate_MBS_to_mAryYmdhns(pM);
	INT i1 = 0;
	while(as1[i1])
	{
		rtn[i1] = atoi(as1[i1]);
		++i1;
	}
	ifree(as1);
	return rtn;
}
//---------------------
// �N�����𐔎��ɕϊ�
//---------------------
/* (��)
	idate_ymd_num(2012, 6, 17); //=> 20120617
*/
// v2013-03-17
INT
idate_ymd_num(
	INT i_y, // �N
	INT i_m, // ��
	INT i_d  // ��
)
{
	return (i_y * 10000) + (i_m * 100) + (i_d);
}
//-----------------------------------------------
// �N������CJD�ɕϊ�
//   (��)�󔒓��̂Ƃ��A�ꗥ NS_BEFORE[0] ��Ԃ�
//-----------------------------------------------
// v2021-11-15
DOUBLE
idate_ymdhnsToCjd(
	INT i_y, // �N
	INT i_m, // ��
	INT i_d, // ��
	INT i_h, // ��
	INT i_n, // ��
	INT i_s  // �b
)
{
	DOUBLE cjd = 0.0;
	INT i1 = 0, i2 = 0;
	INT *ai = idate_cnv_month1(i_y, i_m);
		i_y = ai[0];
		i_m = ai[1];
	ifree(ai);
	// 1m=>13m, 2m=>14m
	if(i_m <= 2)
	{
		i_y -= 1;
		i_m += 12;
	}
	// �󔒓�
	i1 = idate_ymd_num(i_y, i_m, i_d);
	if(i1 > NS_BEFORE[1] && i1 < NS_AFTER[1])
	{
		return (DOUBLE)NS_BEFORE[0];
	}
	// �����E�X�ʓ�
	cjd = floor((DOUBLE)(365.25 * (i_y + 4716)) + floor(30.6001 * (i_m + 1)) + i_d - 1524.0);
	if((INT)cjd >= NS_AFTER[0])
	{
		i1 = (INT)floor(i_y / 100.0);
		i2 = 2 - i1 + (INT)floor(i1 / 4);
		cjd += (DOUBLE)i2;
	}
	return cjd + ((i_h * 3600 + i_n * 60 + i_s) / 86400.0);
}
//--------------------
// CJD�������b�ɕϊ�
//--------------------
// v2021-11-15
INT
*idate_cjd_to_iAryHhs(
	DOUBLE cjd
)
{
	INT *rtn = icalloc_INT(3);
	INT i_h = 0, i_n = 0, i_s = 0;

	// �����_�����𒊏o
	// [sec][�␳�O]  [cjd]
	//   0  =  0   -  0.00000000000000000
	//   1  =  1   -  0.00001157407407407
	//   2  =  2   -  0.00002314814814815
	//   3  >  2  NG  0.00003472222222222
	//   4  =  4   -  0.00004629629629630
	//   5  =  5   -  0.00005787037037037
	//   6  >  5  NG  0.00006944444444444
	//   7  =  7   -  0.00008101851851852
	//   8  =  8   -  0.00009259259259259
	//   9  =  9   -  0.00010416666666667
	DOUBLE d1 = (cjd - (INT)cjd);
		d1 += 0.00001; // �␳
		d1 *= 24.0;
	i_h = (INT)d1;
		d1 -= i_h;
		d1 *= 60.0;
	i_n = (INT)d1;
		d1 -= i_n;
		d1 *= 60.0;
	i_s = (INT)d1;
	rtn[0] = i_h;
	rtn[1] = i_n;
	rtn[2] = i_s;

	return rtn;
}
//--------------------
// CJD��YMDHNS�ɕϊ�
//--------------------
// v2021-11-15
INT
*idate_cjd_to_iAryYmdhns(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *rtn = icalloc_INT(6);
	INT i_y = 0, i_m = 0, i_d = 0;
	INT iCJD = (INT)cjd;
	INT i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	if((INT)cjd >= NS_AFTER[0])
	{
		i1 = (INT)floor((cjd - 1867216.25) / 36524.25);
		iCJD += (i1 - (INT)floor(i1 / 4.0) + 1);
	}
	i1 = iCJD + 1524;
	i2 = (INT)floor((i1 - 122.1) / 365.25);
	i3 = (INT)floor(365.25 * i2);
	i4 = (INT)((i1 - i3) / 30.6001);
	// d
	i_d = (i1 - i3 - (INT)floor(30.6001 * i4));
	// y, m
	if(i4 <= 13)
	{
		i_m = i4 - 1;
		i_y = i2 - 4716;
	}
	else
	{
		i_m = i4 - 13;
		i_y = i2 - 4715;
	}
	// h, n, s
	INT *ai2 = idate_cjd_to_iAryHhs(cjd);
		rtn[0] = i_y;
		rtn[1] = i_m;
		rtn[2] = i_d;
		rtn[3] = ai2[0];
		rtn[4] = ai2[1];
		rtn[5] = ai2[2];
	ifree(ai2);
	return rtn;
}
//----------------------
// CJD��FILETIME�ɕϊ�
//----------------------
// v2021-11-15
FILETIME
idate_cjdToFtime(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd);
	INT i_y = ai1[0], i_m = ai1[1], i_d = ai1[2], i_h = ai1[3], i_n = ai1[4], i_s = ai1[5];
	ifree(ai1);
	return iFinfo_ymdhnsToFtime(i_y, i_m, i_d, i_h, i_n, i_s, FALSE);
}
//---------
// �Čv�Z
//---------
// v2013-03-21
INT
*idate_reYmdhns(
	INT i_y, // �N
	INT i_m, // ��
	INT i_d, // ��
	INT i_h, // ��
	INT i_n, // ��
	INT i_s  // �b
)
{
	DOUBLE cjd = idate_ymdhnsToCjd(i_y, i_m, i_d, i_h, i_n, i_s);
	return idate_cjd_to_iAryYmdhns(cjd);
}
//-------------------------------------------
// cjd�ʓ�����j��(�� = 0, �� = 1...)��Ԃ�
//-------------------------------------------
// v2013-03-21
INT
idate_cjd_iWday(
	DOUBLE cjd // cjd�ʓ�
)
{
	return (INT)((INT)cjd + 1) % 7;
}
//-----------------------------------------
// cjd�ʓ������="Su", ��="Mo", ...��Ԃ�
//-----------------------------------------
// v2021-11-17
MBS
*idate_cjd_mWday(
	DOUBLE cjd // cjd�ʓ�
)
{
	return WDAYS[idate_cjd_iWday(cjd)];
}
//------------------------------
// cjd�ʓ�����N���̒ʓ���Ԃ�
//------------------------------
// v2021-11-15
INT
idate_cjd_yeardays(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *ai = idate_cjd_to_iAryYmdhns(cjd);
	INT i1 = ai[0];
	ifree(ai);
	return (INT)(cjd-idate_ymdhnsToCjd(i1, 1, 0, 0, 0, 0));
}
//--------------------------------------
// ���t�̑O�� [6] = {y, m, d, h, n, s}
//--------------------------------------
/* (��)
	INT *ai = idate_add(2012, 1, 31, 0, 0, 0, 0, 1, 0, 0, 0, 0);
	for(INT _i1 = 0; _i1 < 6; _i1++)
	{
		PL3(ai[_i1]); //=> 2012, 2, 29, 0, 0, 0
	}
*/
// v2021-11-15
INT
*idate_add(
	INT i_y,   // �N
	INT i_m,   // ��
	INT i_d,   // ��
	INT i_h,   // ��
	INT i_n,   // ��
	INT i_s,   // �b
	INT add_y, // �N
	INT add_m, // ��
	INT add_d, // ��
	INT add_h, // ��
	INT add_n, // ��
	INT add_s  // �b
)
{
	INT *ai1 = 0;
	INT *ai2 = idate_reYmdhns(i_y, i_m, i_d, i_h, i_n, i_s); // ���K��
	INT i1 = 0, i2 = 0, flg = 0;
	DOUBLE cjd = 0.0;

	// �X�Ɍv�Z
	// ��𔲂��Ɓu1582-11-10 -1m, -1d�v�̂Ƃ��A1582-10-04(���Ғl��1582-10-03)�ƂȂ�
	if(add_y != 0 || add_m != 0)
	{
		i1 = (INT)idate_month_end(ai2[0] + add_y, ai2[1] + add_m);
		if(ai2[2] > (INT)i1)
		{
			ai2[2] = (INT)i1;
		}
		ai1 = idate_reYmdhns(ai2[0] + add_y, ai2[1] + add_m, ai2[2], ai2[3], ai2[4], ai2[5]);
		i2 = 0;
		while(i2 < 6)
		{
			ai2[i2] = ai1[i2];
			++i2;
		}
		ifree(ai1);
		++flg;
	}
	if(add_d != 0)
	{
		cjd = idate_ymdhnsToCjd(ai2[0], ai2[1], ai2[2], ai2[3], ai2[4], ai2[5]);
		ai1 = idate_cjd_to_iAryYmdhns(cjd + add_d);
		i2 = 0;
		while(i2 < 6)
		{
			ai2[i2] = ai1[i2];
			++i2;
		}
		ifree(ai1);
		++flg;
	}
	if(add_h != 0 || add_n != 0 || add_s != 0)
	{
		ai1 = idate_reYmdhns(ai2[0], ai2[1], ai2[2], ai2[3] + add_h, ai2[4] + add_n, ai2[5] + add_s);
		i2 = 0;
		while(i2 < 6)
		{
			ai2[i2] = ai1[i2];
			++i2;
		}
		ifree(ai1);
		++flg;
	}
	if(flg)
	{
		ai1 = icalloc_INT(6);
		i2 = 0;
		while(i2 < 6)
		{
			ai1[i2] = ai2[i2];
			++i2;
		}
	}
	else
	{
		ai1 = idate_reYmdhns(ai2[0], ai2[1], ai2[2], ai2[3], ai2[4], ai2[5]);
	}
	ifree(ai2);

	return ai1;
}
//-------------------------------------------------------
// ���t�̍� [8] = {sign, y, m, d, h, n, s, days}
//   (��)�֋X��A(���t1<=���t2)�ɕϊ����Čv�Z���邽�߁A
//       ���ʂ͈ȉ��̂Ƃ���ƂȂ�̂Œ��ӁB
//       �E5/31��6/30 : + 1m
//       �E6/30��5/31 : -30d
//-------------------------------------------------------
/* (��)
	INT *ai = idate_diff(2012, 1, 31, 0, 0, 0, 2012, 2, 29, 0, 0, 0); //=> sign = 1, y = 0, m = 1, d = 0, h = 0, n = 0, s = 0, days = 29
	for(INT _i1 = 0; _i1 < 7; _i1++)
	{
		PL3(ai[_i1]); //=> 2012, 2, 29, 0, 0, 0, 29
	}
*/
// v2021-11-15
INT
*idate_diff(
	INT i_y1, // �N1
	INT i_m1, // ��1
	INT i_d1, // ��1
	INT i_h1, // ��1
	INT i_n1, // ��1
	INT i_s1, // �b1
	INT i_y2, // �N2
	INT i_m2, // ��2
	INT i_d2, // ��2
	INT i_h2, // ��2
	INT i_n2, // ��2
	INT i_s2  // �b2
)
{
	INT *rtn = icalloc_INT(8);
	INT i1 = 0, i2 = 0, i3 = 0, i4 = 0;
	DOUBLE cjd = 0.0;
	/*
		���K��1
	*/
	DOUBLE cjd1 = idate_ymdhnsToCjd(i_y1, i_m1, i_d1, i_h1, i_n1, i_s1);
	DOUBLE cjd2 = idate_ymdhnsToCjd(i_y2, i_m2, i_d2, i_h2, i_n2, i_s2);

	if(cjd1 > cjd2)
	{
		rtn[0] = -1; // sign(-)
		cjd  = cjd1;
		cjd1 = cjd2;
		cjd2 = cjd;
	}
	else
	{
		rtn[0] = 1; // sign(+)
	}
	/*
		days
	*/
	rtn[7] = (INT)(cjd2 - cjd1);
	/*
		���K��2
	*/
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd1);
	INT *ai2 = idate_cjd_to_iAryYmdhns(cjd2);
	/*
		ymdhns
	*/
	rtn[1] = ai2[0] - ai1[0];
	rtn[2] = ai2[1] - ai1[1];
	rtn[3] = ai2[2] - ai1[2];
	rtn[4] = ai2[3] - ai1[3];
	rtn[5] = ai2[4] - ai1[4];
	rtn[6] = ai2[5] - ai1[5];
	/*
		m ����
	*/
	INT *ai3 = idate_cnv_month2(rtn[1], rtn[2]);
		rtn[1] = ai3[0];
		rtn[2] = ai3[1];
	ifree(ai3);
	/*
		hns ����
	*/
	while(rtn[6] < 0)
	{
		rtn[6] += 60;
		rtn[5] -= 1;
	}
	while(rtn[5] < 0)
	{
		rtn[5] += 60;
		rtn[4] -= 1;
	}
	while(rtn[4] < 0)
	{
		rtn[4] += 24;
		rtn[3] -= 1;
	}
	/*
		d ����
		�O����
	*/
	if(rtn[3] < 0)
	{
		rtn[2] -= 1;
		if(rtn[2] < 0)
		{
			rtn[2] += 12;
			rtn[1] -= 1;
		}
	}
	/*
		�{����
	*/
	if(rtn[0] > 0)
	{
		ai3 = idate_add(ai1[0], ai1[1], ai1[2], ai1[3], ai1[4], ai1[5], rtn[1], rtn[2], 0, 0, 0, 0);
			i1 = (INT)idate_ymdhnsToCjd(ai2[0], ai2[1], ai2[2], 0, 0, 0);
			i2 = (INT)idate_ymdhnsToCjd(ai3[0], ai3[1], ai3[2], 0, 0, 0);
		ifree(ai3);
	}
	else
	{
		ai3 = idate_add(ai2[0], ai2[1], ai2[2], ai2[3], ai2[4], ai2[5], -rtn[1], -rtn[2], 0, 0, 0, 0);
			i1 = (INT)idate_ymdhnsToCjd(ai3[0], ai3[1], ai3[2], 0, 0, 0);
			i2 = (INT)idate_ymdhnsToCjd(ai1[0], ai1[1], ai1[2], 0, 0, 0);
		ifree(ai3);
	}
	i3 = idate_ymd_num(ai1[3], ai1[4], ai1[5]);
	i4 = idate_ymd_num(ai2[3], ai2[4], ai2[5]);
	/* �ϊ���
		"05-31" "06-30" �̂Ƃ� m = 1, d = 0
		"06-30" "05-31" �̂Ƃ� m = 0, d = -30
		����������͔��ɃV�r�A�Ȃ̂ň��ՂɕύX�����!!
	*/
	if(rtn[0] > 0                                                 // +�̂Ƃ��̂�
		&& i3 <= i4                                           // (��) "12:00:00 =< 23:00:00"
		&& idate_chk_month_end(ai2[0], ai2[1], ai2[2], FALSE) // (��) 06-"30" �͌������H
		&& ai1[2] > ai2[2]                                    // (��) 05-"31" > 06-"30"
	)
	{
		rtn[2] += 1;
		rtn[3] = 0;
	}
	else
	{
		rtn[3] = i1 - i2 - (INT)(i3 > i4 ? 1 : 0);
	}
	ifree(ai2);
	ifree(ai1);

	return rtn;
}
//--------------------------
// diff()�^add()�̓���m�F
//--------------------------
/* (��) ����1�N����2000�N���̃T���v��100��ɂ��ĕ]��
	idate_diff_checker(1, 2000, 100);
*/
// v2021-11-15
/*
VOID
idate_diff_checker(
	INT from_year, // ���N����
	INT to_year,   // ���N�܂�
	UINT repeat    // �T���v�����o��(����)
)
{
	INT rnd_y = to_year - from_year;
	if(rnd_y < 0)
	{
		rnd_y = -(rnd_y);
	}
	INT *ai1 = 0, *ai2 = 0, *ai3 = 0, *ai4 = 0;
	INT y1 = 0, y2 = 0, m1 = 0, m2 = 0, d1 = 0, d2 = 0;
	MBS s1[16] = "", s2[16] = "";
	MBS *err = 0;
	P2("-cnt--From----------To----------sign,    y,  m,  d---DateAdd------chk---");
	MT_initByAry(TRUE);
	for(UINT _u1 = 1; _u1 <= repeat; _u1++)
	{
		y1 = from_year + MT_irand_INT(0, rnd_y);
		y2 = from_year + MT_irand_INT(0, rnd_y);
		m1 = 1 + MT_irand_INT(0, 11);
		m2 = 1 + MT_irand_INT(0, 11);
		d1 = 1 + MT_irand_INT(0, 30);
		d2 = 1 + MT_irand_INT(0, 30);
		// �Čv�Z
		ai1 = idate_reYmdhns(y1, m1, d1, 0, 0, 0);
		ai2 = idate_reYmdhns(y2, m2, d2, 0, 0, 0);
		// diff & add
		ai3 = idate_diff(ai1[0], ai1[1], ai1[2], 0, 0, 0, ai2[0], ai2[1], ai2[2], 0, 0, 0);
		ai4 = (
			ai3[0] > 0 ?
			idate_add(ai1[0], ai1[1], ai1[2], 0, 0, 0, ai3[1], ai3[2], ai3[3], 0, 0, 0) :
			idate_add(ai1[0], ai1[1], ai1[2], 0, 0, 0, -(ai3[1]), -(ai3[2]), -(ai3[3]), 0, 0, 0)
		);
		// �v�Z���ʂ̏ƍ�
		sprintf(s1, "%d%02d%02d", ai2[0], ai2[1], ai2[2]);
		sprintf(s2, "%d%02d%02d", ai4[0], ai4[1], ai4[2]);
		err = (imb_cmpp(s1, s2) ? "ok" : "NG");
		P(
			"%4d  %5d-%02d-%02d : %5d-%02d-%02d  [%2d, %4d, %2d, %2d]  %5d-%02d-%02d  %s\n",
			_u1,
			ai1[0], ai1[1], ai1[2], ai2[0], ai2[1], ai2[2],
			ai3[0], ai3[1], ai3[2], ai3[3], ai4[0], ai4[1], ai4[2],
			err
		);
		ifree(ai4);
		ifree(ai3);
		ifree(ai2);
		ifree(ai1);
	}
	MT_freeAry();
}
*/
/*
	// Ymdhns
	%a : �j��(��:Su)
	%A : �j����(��:0)
	%c : �N���̒ʎZ��
	%C : CJD�ʎZ��
	%J : JD�ʎZ��
	%e : �N���̒ʎZ�T

	// Diff
	%Y : �ʎZ�N
	%M : �ʎZ��
	%D : �ʎZ��
	%H : �ʎZ��
	%N : �ʎZ��
	%S : �ʎZ�b
	%W : �ʎZ�T
	%w : �ʎZ�T�̗]��

	// ����
	%g : Sign "-" "+"
	%G : Sign "-" �̂�
	%y : �N(0000)
	%m : ��(00)
	%d : ��(00)
	%h : ��(00)
	%n : ��(00)
	%s : �b(00)
	%% : "%"
	\a
	\n
	\t
*/
/* (��) ymdhns
	INT *ai = idate_reYmdhns(1970, 12, 10, 0, 0, 0);
	MBS *s1 = 0;
		s1 = idate_format_ymdhns("%g%y-%m-%d(%a), %c/%C", ai[0], ai[1], ai[2], ai[3], ai[4], ai[5]);
		PL2(s1);
	ifree(s1);
	ifree(ai);
*/
/* (��) diff
	INT *ai = idate_diff(1970, 12, 10, 0, 0, 0, 2021, 4, 18, 0, 0, 0);
	MBS *s1 = idate_format_diff("%g%y-%m-%d\t%W�T\t%D��\t%S�b", ai[0], ai[1], ai[2], ai[3], ai[4], ai[5], ai[6], ai[7]);
		PL2(s1);
	ifree(s1);
	ifree(ai);
*/
// v2021-11-17
MBS
*idate_format_diff(
	MBS   *format, //
	INT   i_sign,  // �����^-1="-", 0<="+"
	INT   i_y,     // �N
	INT   i_m,     // ��
	INT   i_d,     // ��
	INT   i_h,     // ��
	INT   i_n,     // ��
	INT   i_s,     // �b
	INT64 i_days   // �ʎZ���^idate_diff()�Ŏg�p
)
{
	if(!format)
	{
		return "";
	}

	CONST UINT BufSizeMax = 512;
	CONST UINT BufSizeDmz = 64;
	MBS *rtn = icalloc_MBS(BufSizeMax + BufSizeDmz);
	MBS *pEnd = rtn;
	UINT uPos = 0;

	// Ymdhns �Ŏg�p
	DOUBLE cjd = (i_days ? 0.0 : idate_ymdhnsToCjd(i_y, i_m, i_d, i_h, i_n, i_s));
	DOUBLE jd = idate_cjdToJd(cjd);

	// �����`�F�b�N
	if(i_y < 0)
	{
		i_sign = -1;
		i_y = -(i_y);
	}
	while(*format)
	{
		if(*format == '%')
		{
			++format;
			switch(*format)
			{
				// Ymdhns
				case 'a': // �j��(��:Su)
					pEnd += imi_cpy(pEnd, idate_cjd_mWday(cjd));
					break;

				case 'A': // �j����
					pEnd += sprintf(pEnd, "%d", idate_cjd_iWday(cjd));
					break;

				case 'c': // �N���̒ʎZ��
					pEnd += sprintf(pEnd, "%d", idate_cjd_yeardays(cjd));
					break;

				case 'C': // CJD�ʎZ��
					pEnd += sprintf(pEnd, CJD_FORMAT, cjd);
					break;

				case 'J': // JD�ʎZ��
					pEnd += sprintf(pEnd, CJD_FORMAT, jd);
					break;

				case 'e': // �N���̒ʎZ�T
					pEnd += sprintf(pEnd, "%d", idate_cjd_yearweeks(cjd));
					break;

				// Diff
				case 'Y': // �ʎZ�N
					pEnd += sprintf(pEnd, "%d", i_y);
					break;

				case 'M': // �ʎZ��
					pEnd += sprintf(pEnd, "%d", (i_y * 12) + i_m);
					break;

				case 'D': // �ʎZ��
					pEnd += sprintf(pEnd, "%lld", i_days);
					break;

				case 'H': // �ʎZ��
					pEnd += sprintf(pEnd, "%lld", (i_days * 24) + i_h);
					break;

				case 'N': // �ʎZ��
					pEnd += sprintf(pEnd, "%lld", (i_days * 24 * 60) + (i_h * 60) + i_n);
					break;

				case 'S': // �ʎZ�b
					pEnd += sprintf(pEnd, "%lld", (i_days * 24 * 60 * 60) + (i_h * 60 * 60) + (i_n * 60) + i_s);
					break;

				case 'W': // �ʎZ�T
					pEnd += sprintf(pEnd, "%lld", (i_days / 7));
					break;

				case 'w': // �ʎZ�T�̗]��
					pEnd += sprintf(pEnd, "%d", (INT)(i_days % 7));
					break;

				// ����
				case 'g': // Sign "-" "+"
					*pEnd = (i_sign < 0 ? '-' : '+');
					++pEnd;
					break;

				case 'G': // Sign "-" �̂�
					if(i_sign < 0)
					{
						*pEnd = '-';
						++pEnd;
					}
					break;

				case 'y': // �N
					pEnd += sprintf(pEnd, "%04d", i_y);
					break;

				case 'm': // ��
					pEnd += sprintf(pEnd, "%02d", i_m);
					break;

				case 'd': // ��
					pEnd += sprintf(pEnd, "%02d", i_d);
					break;

				case 'h': // ��
					pEnd += sprintf(pEnd, "%02d", i_h);
					break;

				case 'n': // ��
					pEnd += sprintf(pEnd, "%02d", i_n);
					break;

				case 's': // �b
					pEnd += sprintf(pEnd, "%02d", i_s);
					break;

				case '%':
					*pEnd = '%';
					++pEnd;
					break;

				default:
					--format; // else �ɏ�����U��
					break;
			}
			uPos = pEnd - rtn;
		}
		else if(*format == '\\')
		{
			switch(format[1])
			{
				case('a'): *pEnd = '\a';      break;
				case('n'): *pEnd = '\n';      break;
				case('t'): *pEnd = '\t';      break;
				default:   *pEnd = format[1]; break;
			}
			++format;
			++pEnd;
			++uPos;
		}
		else
		{
			*pEnd = *format;
			++pEnd;
			++uPos;
		}
		++format;

		if(BufSizeMax < uPos)
		{
			break;
		}
	}
	return rtn;
}
/* (��)
	MBS *p1 = "1970/12/10 00:00:00";
	INT *ai1 = idate_MBS_to_iAryYmdhns(p1);
	PL2(idate_format_iAryToA(IDATE_FORMAT_STD, ai1));
*/
// v2021-11-15
MBS
*idate_format_iAryToA(
	MBS *format, //
	INT *ymdhns  // {y, m, d, h, n, s}
)
{
	INT *ai1 = ymdhns;
	MBS *rtn = idate_format_ymdhns(format, ai1[0], ai1[1], ai1[2], ai1[3], ai1[4], ai1[5]);
	return rtn;
}
// v2021-11-15
MBS
*idate_format_cjdToA(
	MBS *format,
	DOUBLE cjd
)
{
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd);
	MBS *rtn = idate_format_ymdhns(format, ai1[0], ai1[1], ai1[2], ai1[3], ai1[4], ai1[5]);
	ifree(ai1);
	return rtn;
}
//---------------------
// CJD �𕶎���ɕϊ�
//---------------------
/*
	�啶�� => "yyyy-mm-dd 00:00:00"
	������ => "yyyy-mm-dd hh:nn:ss"
		Y, y : �N
		M, m : ��
		W, w : �T
		D, d : ��
		H, h : ��
		N, n : ��
		S, s : �b
*/
/*
	"[-20d]"  "2015-12-10 00:25:00"
	"[-20D]"  "2015-12-10 00:00:00"
	"[-20d%]" "2015-12-10 %"
	"[]"      "2015-12-30 00:25:00"
	"[%]"     "2015-12-30 %"
	"[Err]"   ""
	"[Err%]"  ""
*/
/* (��)
	MBS *pM = "date>[-1m%] and date<[1M]";
	INT *ai1 = idate_now_to_iAryYmdhns_localtime();
	MBS *p1 = idate_replace_format_ymdhns(
		pM,
		"[", "]",
		"'",
		ai1[0], ai1[1], ai1[2], ai1[3], ai1[4], ai1[5]
	);
	PL2(p1);
	ifree(p1);
	ifree(ai1);
*/
// v2021-11-30
MBS
*idate_replace_format_ymdhns(
	MBS *pM,        // �ϊ��Ώە�����
	MBS *quoteBgn,  // �͕��� 1���� (��) "["
	MBS *quoteEnd,  // �͕��� 1���� (��) "]"
	MBS *add_quote, // �o�͕����ɒǉ�����quote (��) "'"
	INT i_y,        // �x�[�X�N
	INT i_m,        // �x�[�X��
	INT i_d,        // �x�[�X��
	INT i_h,        // �x�[�X��
	INT i_n,        // �x�[�X��
	INT i_s         // �x�[�X�b
)
{
	if(!pM)
	{
		return NULL;
	}
	MBS *p1 = 0, *p2 = 0, *p3 = 0, *p4 = 0, *p5 = 0;
	INT i1 = iji_searchCnt(pM, quoteBgn);
	MBS *rtn = icalloc_MBS(imi_len(pM) + (32 * i1));
	MBS *pEnd = rtn;

	// quoteBgn ���Ȃ��Ƃ��Ap�̃N���[����Ԃ�
	if(!i1)
	{
		imi_cpy(rtn, pM);
		return rtn;
	}

	// add_quote�̏��O����
	MBS *pAddQuote = (
		(*add_quote >= '0' && *add_quote <= '9') || *add_quote == '+' || *add_quote == '-' ?
		"" :
		add_quote
	);
	INT iQuote1 = imi_len(quoteBgn), iQuote2 = imi_len(quoteEnd);
	INT add_y = 0, add_m = 0, add_d = 0, add_h = 0, add_n = 0, add_s = 0;
	INT cntPercent = 0;
	INT *ai = 0;
	BOOL bAdd = FALSE;
	BOOL flg = FALSE;
	BOOL zero = FALSE;
	MBS *ts1 = icalloc_MBS(imi_len(pM));

	// quoteBgn - quoteEnd ����t�ɕϊ�
	p1 = p2 = pM;
	while(*p2)
	{
		// "[" ��T��
		// pM = "[[", quoteBgn = "["�̂Ƃ���z�肵�Ă���
		if(*quoteBgn && imb_cmpf(p2, quoteBgn) && !imb_cmpf(p2 + iQuote1, quoteBgn))
		{
			bAdd = FALSE;  // ������
			p2 += iQuote1; // quoteBgn.len
			p1 = p2;

			// "]" ��T��
			if(*quoteEnd)
			{
				p2 = ijp_searchL(p1, quoteEnd);
				imi_pcpy(ts1, p1, p2); // ��͗p������

				// "[]" �̒��ɐ������܂܂�Ă��邩
				i1 = 0;
				p3 = ts1;
				while(*p3)
				{
					if(*p3 >= '0' && *p3 <= '9')
					{
						i1 = 1;
						break;
					}
					++p3;
				}

				// ��O
				p3 = 0;
				switch((p2 - p1))
				{
					case(0): // "[]"
						p3 = "y";
						i1 = 1;
						break;

					case(1): // "[%]"
						if(*ts1 == '%')
						{
							p3 = "y%";
							i1 = 1;
						}
						break;

					default:
						p3 = ts1;
						break;
				}
				if(i1)
				{
					zero = FALSE; // "00:00:00" ���ǂ���
					i1 = (INT)inum_atoi(p3); // �������琔���𒊏o
					while(*p3)
					{
						switch(*p3)
						{
							case 'Y': // �N => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'y': // �N => "yyyy-mm-dd hh:nn:ss"
								add_y = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'M': // �� => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'm': // �� => "yyyy-mm-dd hh:nn:ss"
								add_m = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'W': // �T => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'w': // �T => "yyyy-mm-dd hh:nn:ss"
								add_d = i1 * 7;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'D': // �� => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'd': // �� => "yyyy-mm-dd hh:nn:ss"
								add_d = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'H': // �� => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'h': // �� => "yyyy-mm-dd hh:nn:ss"
								add_h = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'N': // �� => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 'n': // �� => "yyyy-mm-dd hh:nn:ss"
								add_n = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;

							case 'S': // �b => "yyyy-mm-dd 00:00:00"
								zero = TRUE;
								/* Falls through. */ // -Wimplicit-fallthrough=3
							case 's': // �b => "yyyy-mm-dd hh:nn:ss"
								add_s = i1;
								flg = TRUE;
								bAdd = TRUE;
								break;
						}
						// [1y6m] �̂悤�ȂƂ� [1y] �ŏ�������
						if(flg)
						{
							break;
						}
						++p3;
					}

					// ���L�����t��
					cntPercent = 0;
					if(bAdd)
					{
						// "Y-s" ������ "%" ������
						while(*p3)
						{
							if(*p3 == '%')
							{
								++cntPercent;
							}
							++p3;
						}

						// �}����
						ai = idate_add(
							i_y, i_m, i_d, i_h, i_n, i_s,
							add_y, add_m, add_d, add_h, add_n, add_s
						);
						// ���̓����� 00:00 �����߂�
						if(zero)
						{
							ai[3] = ai[4] = ai[5] = 0;
						}

						// ������
						flg = FALSE;
						add_y = add_m = add_d = add_h = add_n = add_s = 0;
					}
					else
					{
						// "%" ���܂܂�Ă���΁u���Ȃ��v��������
						p4 = ts1;
						while(*p4)
						{
							if(*p4 == '%')
							{
								++cntPercent;
							}
							++p4;
						}
						ai = idate_MBS_to_iAryYmdhns(ts1);
					}

					// bAdd�̏����������p��
					p5 = idate_format_ymdhns(
						IDATE_FORMAT_STD,
						ai[0], ai[1], ai[2], ai[3], ai[4], ai[5]
					);
						// "2015-12-30 00:00:00" => "2015-12-30 %"
						if(cntPercent)
						{
							p4 = p5;
							while(*p4 != ' ')
							{
								++p4;
							}
							*(++p4) = '%'; // SQLite��"%"��t�^
							*(++p4) = 0;
						}
						pEnd += imi_cpy(pEnd, pAddQuote);
						pEnd += imi_cpy(pEnd, p5);
						pEnd += imi_cpy(pEnd, pAddQuote);
					ifree(p5);
					ifree(ai);
				}
				p2 += (iQuote2); // quoteEnd.len
				p1 = p2;
			}
		}
		else
		{
			pEnd += imi_pcpy(pEnd, p2, p2 + 1);
			++p2;
		}
		add_y = add_m = add_d = add_h = add_n = add_s = 0;
	}
	ifree(ts1);
	return rtn;
}
//---------------------
// ������ymdhns��Ԃ�
//---------------------
/* (��)
	// ���� = 2012-06-19 00:00:00 �̂Ƃ��A
	idate_now_to_iAryYmdhns(0); // System(-9h) => 2012, 6, 18, 15, 0, 0
	idate_now_to_iAryYmdhns(1); // Local       => 2012, 6, 19,  0, 0, 0
*/
// v2021-11-15
INT
*idate_now_to_iAryYmdhns(
	BOOL area // TRUE=LOCAL�^FALSE=SYSTEM
)
{
	SYSTEMTIME st;
	if(area)
	{
		GetLocalTime(&st);
	}
	else
	{
		GetSystemTime(&st);
	}
	/*
		[Pending] 2021-11-15
			���L�R�[�h�Ńr�[�v���𔭐����邱�Ƃ�����B
				INT *rtn = icalloc_INT(n) ��INT�n�S�ʁ^DOUBL�n�͖��Ȃ�
				rtn[n] = 1793..2047
			2021-11-10�ɃR���p�C���� MInGW(32bit) ���� Mingw-w64(64bit) �ɕύX�����e�����H
			���ʁA�l�q������B
	*/
	INT *rtn = icalloc_INT(7);
		rtn[0] = st.wYear;
		rtn[1] = st.wMonth;
		rtn[2] = st.wDay;
		rtn[3] = st.wHour;
		rtn[4] = st.wMinute;
		rtn[5] = st.wSecond;
		rtn[6] = st.wMilliseconds;
	return rtn;
}
//------------------
// ������cjd��Ԃ�
//------------------
/* (��)
	idate_nowToCjd(0); // System(-9h)
	idate_nowToCjd(1); // Local
*/
// v2021-11-15
DOUBLE
idate_nowToCjd(
	BOOL area // TRUE=LOCAL, FALSE=SYSTEM
)
{
	INT *ai = idate_now_to_iAryYmdhns(area);
	INT y = ai[0], m = ai[1], d = ai[2], h = ai[3], n = ai[4], s = ai[5];
	ifree(ai);
	return idate_ymdhnsToCjd(y, m, d, h, n, s);
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Geography
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-------------------
// �x���b => �\�i�@
//-------------------
/* (��)
	P("%f�x\n", rtnGeoIBLto10A(24, 26, 58.495200));
*/
// v2019-12-19
DOUBLE
rtnGeoIBLto10A(
	INT deg,   // �x
	INT min,   // ��
	DOUBLE sec // �b
)
{
	return (DOUBLE)deg + ((DOUBLE)min / 60.0) + (sec / 3600.0);
}
/* (��)
	P("%f�x\n", rtnGeoIBLto10B(242658.495200));
*/
// v2019-12-19
DOUBLE
rtnGeoIBLto10B(
	DOUBLE ddmmss // ddmmss.s...
)
{
	DOUBLE sec = fmod(ddmmss, 100.0);
	INT min = (INT)(ddmmss / 100) % 100;
	INT deg = (INT)(ddmmss / 10000);
	return deg + (min / 60.0) + (sec / 3600.0);
}
//-------------------
// �\�i�@ => �x���b
//-------------------
/* (��)
	$Geo geo = rtnGeo10toIBL(24.449582);
	P("%d�x%d��%f�b\n", geo.deg, geo.min, geo.sec);
*/
// v2019-12-18
$Geo
rtnGeo10toIBL(
	DOUBLE angle // �\�i�@
)
{
	INT deg = (INT)angle;
		angle = (angle - (DOUBLE)deg) * 60.0;
	INT min = (INT)angle;
		angle -= min;
	DOUBLE sec = (DOUBLE)angle * 60.0;

	// 0.999... * 60 => 60.0 �΍�
	if(sec == 60.0)
	{
		min += 1;
		sec = 0;
	}
	return ($Geo){0, 0, deg, min, sec};
}
//-------------------------------
// Vincenty�@�ɂ��Q�_�Ԃ̋���
//-------------------------------
/* (�Q�l)
	http://tancro.e-central.tv/grandmaster/script/vincentyJS.html
*/
/* (��)
	$Geo geo = rtnGeoVincentry(35.685187, 139.752274, 24.449582, 122.934340);
	P("%fkm %f�x\n", geo.dist, geo.angle);
*/
// v2021-03-05
$Geo
rtnGeoVincentry(
	DOUBLE lat1, // �J�n�`�ܓx
	DOUBLE lng1, // �J�n�`�o�x
	DOUBLE lat2, // �I���`�ܓx
	DOUBLE lng2  // �I���`�o�x
)
{
	if(lat1 == lat2 && lng1 == lng2)
	{
		return ($Geo){0, 0, 0, 0, 0};
	}

	/// CONST DOUBLE _A = 6378137.0;
	CONST DOUBLE _B   = 6356752.314140356;    // GRS80
	CONST DOUBLE _F   = 0.003352810681182319; // 1 / 298.257222101
	CONST DOUBLE _RAD = 0.017453292519943295; // �� / 180

	CONST DOUBLE latR1 = lat1 * _RAD;
	CONST DOUBLE lngR1 = lng1 * _RAD;
	CONST DOUBLE latR2 = lat2 * _RAD;
	CONST DOUBLE lngR2 = lng2 * _RAD;

	CONST DOUBLE f1 = 1 - _F;

	CONST DOUBLE omega = lngR2 - lngR1;
	CONST DOUBLE tanU1 = f1 * tan(latR1);
	CONST DOUBLE cosU1 = 1 / sqrt(1 + tanU1 * tanU1);
	CONST DOUBLE sinU1 = tanU1 * cosU1;
	CONST DOUBLE tanU2 = f1 * tan(latR2);
	CONST DOUBLE cosU2 = 1 / sqrt(1 + tanU2 * tanU2);
	CONST DOUBLE sinU2 = tanU2 * cosU2;

	DOUBLE lamda  = omega;
	DOUBLE dLamda = 0.0;

	DOUBLE sinLamda  = 0.0;
	DOUBLE cosLamda  = 0.0;
	DOUBLE sin2sigma = 0.0;
	DOUBLE sinSigma  = 0.0;
	DOUBLE cosSigma  = 0.0;
	DOUBLE sigma     = 0.0;
	DOUBLE sinAlpha  = 0.0;
	DOUBLE cos2alpha = 0.0;
	DOUBLE cos2sm    = 0.0;
	DOUBLE c = 0.0;

	while(TRUE)
	{
		sinLamda = sin(lamda);
		cosLamda = cos(lamda);
		sin2sigma = (cosU2 * sinLamda) * (cosU2 * sinLamda) + (cosU1 * sinU2 - sinU1 * cosU2 * cosLamda) * (cosU1 * sinU2 - sinU1 * cosU2 * cosLamda);
		if(sin2sigma < 0.0)
		{
			return ($Geo){0, 0, 0, 0, 0};
		}
		sinSigma = sqrt(sin2sigma);
		cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLamda;
		sigma = atan2(sinSigma, cosSigma);
		sinAlpha = cosU1 * cosU2 * sinLamda / sinSigma;
		cos2alpha = 1 - sinAlpha * sinAlpha;
		cos2sm = cosSigma - 2 * sinU1 * sinU2 / cos2alpha;
		c = _F / 16 * cos2alpha * (4 + _F * (4 - 3 * cos2alpha));
		dLamda = lamda;
		lamda = omega + (1 - c) * _F * sinAlpha * (sigma + c * sinSigma * (cos2sm + c * cosSigma * (-1 + 2 * cos2sm * cos2sm)));
		if(fabs(lamda - dLamda) <= 1e-12)
		{
			break;
		}
	}

	DOUBLE u2 = cos2alpha * (1 - f1 * f1) / (f1 * f1);
	DOUBLE a = 1 + u2 / 16384 * (4096 + u2 * (-768 + u2 * (320 - 175 * u2)));
	DOUBLE b = u2 / 1024 * (256 + u2 * (-128 + u2 * (74 - 47 * u2)));
	DOUBLE dSigma = b * sinSigma * (cos2sm + b / 4 * (cosSigma * (-1 + 2 * cos2sm * cos2sm) - b / 6 * cos2sm * (-3 + 4 * sinSigma * sinSigma) * (-3 + 4 * cos2sm * cos2sm)));
	DOUBLE angle = atan2(cosU2 * sinLamda, cosU1 * sinU2 - sinU1 * cosU2 * cosLamda) * 57.29577951308232;
	DOUBLE dist = _B * a * (sigma - dSigma);

	// �ϊ�
	if(angle < 0)
	{
		angle += 360.0; // 360�x�\�L
	}
	dist /= 1000.0; // m => km

	return ($Geo){dist, angle, 0, 0, 0};
}
