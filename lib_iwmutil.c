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
/* 2014-02-13
	�ϐ����[��
		��INT    �F ���ʂ̕W��
		��UINT   �F (�v����)�uFILE�֌W�v�u�����v�̂� �� �����݂� INT64 �Ɉڍs����
		��DOUBLE �F �����_�̈����S��
*/
/* 2016-08-19
	���ϐ��E�萔�\�L
		��iwmutil���ʂ̕ϐ� // "$IWM_"+�^���L+�啶���Ŏn�܂�
			$IWM_(b|i|p|u)Str
		������ȑ��ϐ� // "$"�̎��͏�����
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
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	���ϐ�
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
MBS      *$IWM_CMD         = "";  // �R�}���h�����i�[
MBS      **$IWM_ARGV       = {0}; // ARGV���i�[
UINT     $IWM_ARGC         = 0;   // ARGC���i�[
HANDLE   $IWM_StdoutHandle = 0;   // ��ʐ���p�n���h��
UINT     $IWM_ColorDefault = 0;   // �R���\�[���F
UINT     $IWM_ExecSecBgn   = 0;   // ���s�J�n����
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	���s�J�n����
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/*
	Win32 SDK Reference Help�uwin32.hlp(1996/11/26)�v���
		�o�ߎ��Ԃ�DWORD�^�ŕۑ�����Ă��܂��B
		�V�X�e����49.7���ԘA�����ē��삳����ƁA
		�o�ߎ��Ԃ� 0 �ɖ߂�܂��B
*/
/* (��)
	iExecSec_init(); //=> $IWM_ExecSecBgn // ���s�J�n����
	Sleep(2000);
	P("-- %.6fsec\n\n", iExecSec_next());
	Sleep(1000);
	P("-- %.6fsec\n\n", iExecSec_next());
*/
// v2021-03-19
UINT
iExecSec(
	CONST UINT microSec // 0�̂Ƃ� Init
)
{
	UINT microSec2 = GetTickCount();
	if(!microSec)
	{
		$IWM_ExecSecBgn = microSec2;
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
// *__icallocMap �̊�{���T�C�Y(�K�X�ύX>0)
#define   IcallocDiv          32
// 8�̔{���Ɋۂ߂�
#define   ceilX8(n)           (UINT)((((n) >> 3) + 2) << 3)
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
		UINT uOldSize = __icallocMapSize;
		__icallocMapSize += IcallocDiv;
		__icallocMap = ($struct_icallocMap*)realloc(__icallocMap, __icallocMapSize * size2);
			icalloc_err(__icallocMap);
		memset(__icallocMap + uOldSize, 0, (IcallocDiv * size2)); // realloc() ������������
	}

	// �����Ƀ|�C���^����
	VOID *rtn = calloc(ceilX8(n), size);
		icalloc_err(rtn); // �ߒl�� 0 �Ȃ� exit()

	// �|�C���^
	(__icallocMap + __icallocMapEOD)->ptr = rtn;

	// �z��
	(__icallocMap + __icallocMapEOD)->num = (aryOn ? n : 0);

	// �T�C�Y
	(__icallocMap + __icallocMapEOD)->size = ceilX8(n) * size;

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
	// icalloc() �ŗ̈�m�ۂ�����g�p.
	MBS *pM = icalloc_MBS(1000);
	pM = irealloc_MBS(pM, 2000);
*/
// v2016-01-31
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
	UINT u1 = ceilX8(n * size);
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
// v2016-08-30
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

				// 2�����폜
				memset(map->ptr, 0, map->size);
				free(map->ptr);
				memset(map, 0, __sizeof_icallocMap);
				return;
			}
			else
			{
				memset(map->ptr, 0, map->size);
				free(map->ptr);
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
	u1 = 0;
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
	/// P823("__icallocMapFreeCnt=", __icallocMapFreeCnt);
	/// P823("__icallocMapEOD=", __icallocMapEOD);
	/// P823("SweepCnt=", uSweep);
}
//---------------------------
// __icallocMap�����X�g�o��
//---------------------------
// v2020-05-12
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
				P("��");
				++u2;
			}
			else
			{
				P("��");
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
	printf()�n
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-----------
// printf()
//-----------
/* (��)
	P("%s", "abc"); //=> "abc"
	P("abc");       //=> "abc"
*/
// v2015-01-24
VOID
P(
	CONST MBS *format,
	...
)
{
	va_list va;
	va_start(va, format);
		vfprintf(stdout, format, va);
	va_end(va);
}
//---------------
// �J��Ԃ��\��
//---------------
/* (��)
	PR("abc", 3);  //=> "abcabcabc"
*/
// v2020-05-28
VOID
PR(
	MBS *pM,   // ������
	INT repeat // �J��Ԃ���
)
{
	if(repeat <= 0)
	{
		return;
	}
	while(repeat--)
	{
		P(pM);
	}
}
//---------------
// �F������\��
//---------------
/* (��)
	iConsole_getColor(); //=> $IWM_ColorDefault // ���̐F��ۑ�
	PZ(15, "�J���[�����F%s\n", "��"); //=> "�J���[�����F��\n"
	PZ(-1, NULL); // ���̐F�ɖ߂�
*/
// v2021-03-19
VOID
PZ(
	INT rgb, // iConsole_setTextColor() �Q��
	MBS *format,
	...
)
{
	if(rgb < 0)
	{
		rgb = $IWM_ColorDefault;
	}
	SetConsoleTextAttribute($IWM_StdoutHandle, rgb);
	va_list va;
	va_start(va, format);
		vfprintf(stdout, format, va);
	va_end(va);
}
//-----------------------
// EscapeSequence�֕ϊ�
//-----------------------
/* (��)
	// '\a' �� '\a' �ƈ���
	// (��)�ʏ�� '\\a' �� '\a' �Ƃ���
*/
// v2013-02-20
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
					*(rtn + i1) = '\a';
					break;

				case('b'):
					*(rtn + i1) = '\b';
					break;

				case('t'):
					*(rtn + i1) = '\t';
					break;

				case('n'):
					*(rtn + i1) = '\n';
					break;

				case('v'):
					*(rtn + i1) = '\v';
					break;

				case('f'):
					*(rtn + i1) = '\f';
					break;

				case('r'):
					*(rtn + i1) = '\r';
					break;

				default:
					*(rtn + i1) = '\\';
					++i1;
					*(rtn + i1) = *pM;
					break;
			}
		}
		else
		{
			*(rtn + i1) = *pM;
		}
		++pM;
		++i1;
	}
	*(rtn + i1) = 0;
	return rtn;
}
//--------------------
// sprintf()�̊g����
//--------------------
/* (��)
	// NUL�֏o�́i�����o�͂��Ȃ��j
	FILE *oFp = fopen(NULL_DEVICE, "wb");
		MBS *p1 = ims_fprintf(oFp, "%s-%s", "ABC", "123"); //=> "ABC-123"
			P82(p1);
		ifree(p1);
	fclose(oFp);
	// ��ʂɏo��
	P82(ims_fprintf(stdout, "%s-%s", "ABC", "123")); //=> "ABC-123"
*/
// v2021-03-21
MBS
*ims_fprintf(
	FILE *oFp,
	MBS *format,
	...
)
{
	va_list va;
	va_start(va, format);
		// ���������߂邾��
		UINT len = vfprintf(oFp, format, va);
		// �t�H�[�}�b�g
		MBS *rtn = icalloc_MBS(len);
		vsprintf(rtn, format, va);
	va_end(va);
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
// v2016-09-05
WCS
*icnv_U2W(
	U8N *pU
)
{
	if(!pU)
	{
		return NULL;
	}
	UINT uW = iui_len(pU);
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
	P83(imi_len(mp1)); //=> 15
	P83(iji_len(mp1)); //=> 10
	WCS *wp1 = M2W(mp1);
	P83(iwi_len(wp1)); //=> 10
*/
// v2016-09-06
UINT
imi_len(
	MBS *pM
)
{
	if(!pM)
	{
		return 0;
	}
	UINT uCnt = 0;
	while(*pM)
	{
		++pM;
		++uCnt;
	}
	return uCnt;
}
// v2016-09-06
UINT
iji_len(
	MBS *pM
)
{
	if(!pM)
	{
		return 0;
	}
	UINT uCnt = 0;
	while(*pM)
	{
		pM = CharNextA(pM);
		++uCnt;
	}
	return uCnt;
}
// v2020-05-30
UINT
iui_len(
	U8N *p
)
{
	if(!p)
	{
		return 0;
	}
	UINT rtn = 0;
	// BOM��ǂݔ�΂�(UTF-8N �͊Y�����Ȃ�)
	if(*p == (CHAR)0xEF && *(p + 1) == (CHAR)0xBB && *(p + 2) == (CHAR)0xBF)
	{
		p += 3;
	}
	INT c = 0;
	while(*p)
	{
		if(*p & 0x80)
		{
			// ���o�C�g����
			c = (*p & 0xfc);
			while(c & 0x80)
			{
				++p;
				c <<= 1;
			}
		}
		else
		{
			// 1�o�C�g����
			++p;
		}
		++rtn;
	}
	return rtn;
}
// v2016-09-06
UINT
iwi_len(
	WCS *p
)
{
	if(!p)
	{
		return 0;
	}
	UINT rtn = 0;
	while(*p)
	{
		++p;
		++rtn;
	}
	return rtn;
}
//-------------------------
// size��̃|�C���^��Ԃ�
//-------------------------
/* (��)
	MBS *p1 = "AB������";
	P82(imp_forwardN(p1, 4)); //=> "����"
*/
// v2021-03-20
MBS
*imp_forwardN(
	MBS *pM,   // �J�n�ʒu
	UINT sizeM // ������
)
{
	if(!pM)
	{
		return 0;
	}
	UINT uCnt = imi_len(pM);

	return (uCnt < sizeM ?
		(pM + uCnt) :
		(pM + sizeM)
	);
}
/* (��)
	MBS *p1 = "AB������";
	P82(ijp_forwardN(p1, 3)); //=> "����"
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
		return 0;
	}
	while(*pM && sizeJ > 0)
	{
		pM = CharNextA(pM);
		--sizeJ;
	}
	return pM;
}
// v2020-05-30
U8N
*iup_forwardN(
	U8N *p,    // �J�n�ʒu
	UINT sizeU // ������
)
{
	if(!p)
	{
		return p;
	}
	// BOM��ǂݔ�΂�(UTF-8N �͊Y�����Ȃ�)
	if(*p == (CHAR)0xEF && *(p + 1) == (CHAR)0xBB && *(p + 2) == (CHAR)0xBF)
	{
		p += 3;
	}
	INT c = 0;
	while(*p && sizeU > 0)
	{
		if(*p & 0x80)
		{
			// ���o�C�g����
			c = (*p & 0xfc);
			while(c & 0x80)
			{
				++p;
				c <<= 1;
			}
		}
		else
		{
			// 1�o�C�g����
			++p;
		}
		--sizeU;
	}
	return p;
}
// v2014-11-23
MBS
*imp_sod(
	MBS *pM
)
{
	while(*pM)
	{
		--pM;
	}
	return (++pM);
}
// v2014-11-27
WCS
*iwp_sod(
	WCS *pW
)
{
	while(*pW)
	{
		--pW;
	}
	return (++pW);
}
//---------------
// �召�����u��
//---------------
/* (��)
	MBS *p1 = "aBC";
	P82(ims_upper(p1)); //=> "ABC"
	P82(ims_lower(p1)); //=> "abc"
*/
// v2016-02-15
MBS
*ims_upper(
	MBS *pM
)
{
	MBS *rtn = ims_clone(pM);
	return CharUpperA(rtn);
}
// v2016-02-15
MBS
*ims_lower(
	MBS *pM
)
{
	MBS *rtn = ims_clone(pM);
	return CharLowerA(rtn);
}
//---------------------------
// �R�s�[��A�R�s�[����Ԃ�
//---------------------------
// v2016-05-04
UINT
iji_plen(
	MBS *pBgn,
	MBS *pEnd
)
{
	if(!pBgn || !pEnd)
	{
		return 0;
	}
	UINT rtn = 0;
	while(*pBgn && pBgn < pEnd)
	{
		pBgn = CharNextA(pBgn);
		++rtn;
	};
	return rtn;
}
//-------------------------------------------
// �R�s�[��A�I�[�ʒu(NULL)�̃|�C���^��Ԃ�
//-------------------------------------------
/* (��)
	MBS *to = icalloc_MBS(100);
	MBS *pEnd = imp_cpy(to, "Iwama, "); //=> "Iwama, "
		imp_cpy(pEnd, "Yoshiyuki");     //=> "Iwama, Yoshiyuki"
	P82(to);
	ifree(to);
*/
// v2020-05-30
MBS
*imp_cpy(
	MBS *to,
	MBS *from
)
{
	if(!from)
	{
		return to;
	}
	while(*from)
	{
		*(to++) = *(from++);
	}
	*to = 0;
	return to; // �����̃|�C���^(\0)��Ԃ�
}
// v2020-05-30
WCS
*iwp_cpy(
	WCS *to,
	WCS *from
)
{
	if(!from)
	{
		return to;
	}
	while(*from)
	{
		*(to++) = *(from++);
	}
	*to = 0;
	return to; // �����̃|�C���^(\0)��Ԃ�
}
// v2020-05-30
MBS
*imp_pcpy(
	MBS *to,
	MBS *from1,
	MBS *from2
)
{
	if(!from1 || !from2)
	{
		return to;
	}
	while(*from1 && from1 < from2)
	{
		*(to++) = *(from1++);
	}
	*to = 0;
	return to; // �����̃|�C���^(\0)��Ԃ�
}
//-------------------------
// �R�s�[������������Ԃ�
//-------------------------
/* (��)
	MBS *to = icalloc_MBS(100);
	P83(imi_cpy(to, "abcde")); //=> 5
	ifree(to);
*/
// v2020-05-30
UINT
imi_cpy(
	MBS *to,
	MBS *from
)
{
	UINT rtn = 0;
	if(!from)
	{
		return 0;
	}
	while(*from)
	{
		*(to++) = *(from++);
		++rtn;
	}
	*to = 0;
	return rtn;
}
//-----------------------
// �V�K����������𐶐�
//-----------------------
/* (��)
	MBS *from="abcde";
	P82(ims_clone(from)); //=> "abcde"
*/
// v2020-05-30
MBS
*ims_clone(
	MBS *from
)
{
	MBS *rtn = icalloc_MBS(imi_len(from));
	imp_cpy(rtn, from);
	return rtn;
}
// v2020-05-30
WCS
*iws_clone(
	WCS *from
)
{
	WCS *rtn = icalloc_WCS(iwi_len(from));
	iwp_cpy(rtn, from);
	return rtn;
}
/* (��)
	MBS *from = "abcde";
	P82(ims_pclone(from, from + 3)); //=> "abc"
*/
// v2020-05-30
MBS
*ims_pclone(
	MBS *from1,
	MBS *from2
)
{
	MBS *rtn = icalloc_MBS(from2 - from1);
	imp_pcpy(rtn, from1, from2);
	return rtn;
}
/* (��)
	MBS *to = "123";
	MBS *from = "abcde";
	P82(ims_cat_pclone(to, from, from + 3)); //=> "123abc"
*/
// v2020-05-30
MBS
*ims_cat_pclone(
	MBS *to,    // ��
	MBS *from1, // ���̊J�n�ʒu
	MBS *from2  // ���̏I���ʒu(�܂܂��)
)
{
	MBS *rtn = icalloc_MBS(imi_len(to) + (from2 - from1));
	MBS *pEnd = imp_cpy(rtn, to);
		imp_pcpy(pEnd, from1, from2);
	return rtn;
}
/* (��)
	// �v�f���Ăяo���x realloc ��������X�}�[�g�����A
	// ���x�̓_�ŕs���Ȃ̂� calloc �P��ōς܂���B
	MBS *p1 = "123";
	MBS *p2 = "abcde";
	MBS *p3 = "�����";
	MBS *rtn = ims_ncat_clone(p1, p2, p3, NULL); //=> "123abcde"
		P82(rtn);
	ifree(rtn);
*/
// v2020-05-30
MBS
*ims_ncat_clone(
	MBS *pM, // ary[0]
	...      // ary[1...]�A�Ō�͕K�� NULL
)
{
	if(!pM)
	{
		return pM;
	}
	UINT u1 = 0;

	// [0]
	UINT uSize = imi_len(pM);

	// [1..n]
	va_list va;
	va_start(va, pM);
		UINT uCnt = 1;
		while(uCnt < IVA_LIST_MAX)
		{
			if(!(u1 = imi_len((MBS*)va_arg(va, MBS*))))
			{
				break;
			}
			uSize += u1;
			++uCnt;
		}
	va_end(va);

	MBS *rtn = icalloc_MBS(uSize);
	MBS *pEnd = imp_cpy(rtn, pM);

	va_start(va, pM);
		while(--uCnt)
		{
			pEnd = imp_cpy(pEnd, (MBS*)va_arg(va, MBS*)); // [0..n]
		}
	va_end(va);

	return rtn;
}
//-----------------------
// �V�K����������𐶐�
//-----------------------
/* (��)
	//----------------------------
	//  [ A  B  C  D  E ] �̂Ƃ�
	//    0  1  2  3  4
	//   -5 -4 -3 -2 -1
	//----------------------------
	MBS *p1 = "ABCDE";
	P82(ijs_sub_clone(p1,  0, 2)); //=> "AB"
	P82(ijs_sub_clone(p1,  1, 2)); //=> "BC"
	P82(ijs_sub_clone(p1,  2, 2)); //=> "CD"
	P82(ijs_sub_clone(p1,  3, 2)); //=> "DE"
	P82(ijs_sub_clone(p1,  4, 2)); //=> "E"
	P82(ijs_sub_clone(p1,  5, 2)); //=> ""
	P82(ijs_sub_clone(p1, -1, 2)); //=> "E"
	P82(ijs_sub_clone(p1, -2, 2)); //=> "DE"
	P82(ijs_sub_clone(p1, -3, 2)); //=> "CD"
	P82(ijs_sub_clone(p1, -4, 2)); //=> "BC"
	P82(ijs_sub_clone(p1, -5, 2)); //=> "AB"
	P82(ijs_sub_clone(p1, -6, 2)); //=> "A"
	P82(ijs_sub_clone(p1, -7, 2)); //=> ""
*/
// v2019-08-15
MBS
*ijs_sub_clone(
	MBS *pM,  // ������
	INT jpos, // �J�n�ʒu
	INT sizeJ // ������
)
{
	if(!pM)
	{
		return pM;
	}
	INT len = iji_len(pM);
	if(jpos >= len)
	{
		return imp_eod(pM);
	}
	if(jpos < 0)
	{
		jpos += len;
		if(jpos < 0)
		{
			sizeJ += jpos;
			jpos = 0;
		}
	}
	MBS *pBgn = ijp_forwardN(pM, jpos);
	MBS *pEnd = ijp_forwardN(pBgn, sizeJ);
	return ims_cat_pclone(NULL, pBgn, pEnd);
}
//--------------------------------
// lstrcmp()�^lstrcmpi()�����S
//--------------------------------
/*
	lstrcmp() �͑召��r�������Ȃ�(TRUE = 0, FALSE = 1 or -1)�̂ŁA
	��r���镶���񒷂𑵂��Ă��K�v������.
*/
/* (��)
	P83(imb_cmp("", "abc", FALSE, FALSE));   //=> FALSE
	P83(imb_cmp("abc", "", FALSE, FALSE));   //=> TRUE
	P83(imb_cmp("", "", FALSE, FALSE));      //=> TRUE
	P83(imb_cmp(NULL, "abc", FALSE, FALSE)); //=> FALSE
	P83(imb_cmp("abc", NULL, FALSE, FALSE)); //=> FALSE
	P83(imb_cmp(NULL, NULL, FALSE, FALSE));  //=> FALSE
	P83(imb_cmp(NULL, "", FALSE, FALSE));    //=> FALSE
	P83(imb_cmp("", NULL, FALSE, FALSE));    //=> FALSE
	NL();
	// imb_cmpf(p, search)  <= imb_cmp(p, search, FALSE, FALSE)
	P83(imb_cmp("abc", "AB", FALSE, FALSE)); //=> FALSE
	// imb_cmpfi(p, search) <= imb_cmp(p, search, FALSE, TRUE)
	P83(imb_cmp("abc", "AB", FALSE, TRUE));  //=> TRUE
	// imb_cmpp(p, search)  <= imb_cmp(p, search, TRUE, FALSE)
	P83(imb_cmp("abc", "AB", TRUE, FALSE));  //=> FALSE
	// imb_cmppi(p, search) <= imb_cmp(p, search, TRUE, TRUE)
	P83(imb_cmp("abc", "AB", TRUE, TRUE));   //=> FALSE
	NL();
	// search�ɂP�����ł����v�����TRUE��Ԃ�
	P83(imb_cmp_leq(""   , "..", TRUE)); //=> TRUE
	P83(imb_cmp_leq("."  , "..", TRUE)); //=> TRUE
	P83(imb_cmp_leq(".." , "..", TRUE)); //=> TRUE
	P83(imb_cmp_leq("...", "..", TRUE)); //=> FALSE
	P83(imb_cmp_leq("...", ""  , TRUE)); //=> FALSE
	NL();
*/
// v2016-02-27
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
	INT i1 = 0, i2 = 0;
	while(*pM && *search)
	{
		i1 = *pM;
		i2 = *search;
		if(icase)
		{
			i1 = tolower(i1);
			i2 = tolower(i2);
		}
		if(i1 != i2)
		{
			break;
		}
		++pM;
		++search;
	}
	if(perfect)
	{
		// ���������� \0 �Ȃ� ���S��v����
		return (!*pM && !*search ? TRUE : FALSE);
	}
	// searchE�̖����� \0 �Ȃ� �O����v����
	return (!*search ? TRUE : FALSE);
}
// v2016-02-27
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
	// ��O "" == "
	if(!*pW && !*search)
	{
		return TRUE;
	}
	// �����Ώ� == "" �̂Ƃ��� FALSE
	if(!*pW)
	{
		return FALSE;
	}
	INT i1 = 0, i2 = 0;
	while(*pW && *search)
	{
		i1 = *pW;
		i2 = *search;
		if(icase)
		{
			i1 = towlower(i1);
			i2 = towlower(i2);
		}
		if(i1 != i2)
		{
			break;
		}
		++pW;
		++search;
	}
	if(perfect)
	{
		// ���������� \0 �Ȃ� ���S��v����
		return (!*pW && !*search ? TRUE : FALSE);
	}
	// searchE�̖����� \0 �Ȃ� �O����v����
	return (!*search ? TRUE : FALSE);
}
//-------------------------------
// ��������͈͂̏I���ʒu��Ԃ�
//-------------------------------
/* (��)
	MBS *p1 = "<-123, <-4, 5->, 6->->";
	P82(ijp_bypass(p1, "<-", "->")); //=> "->, 6->->"
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
	P83(iji_searchCnti("d:\\folder1\\�\\", "\\"));  //=> 2
	P83(iji_searchCntLi("�\\�\\123�\\�\\", "�\\")); //=> 2
	P83(iji_searchCntRi("�\\�\\123�\\�\\", "�\\")); //=> 2
	P83(iji_searchLenLi("�\\�\\123�\\�\\", "�\\")); //=> 2
	P83(imi_searchLenLi("�\\�\\123�\\�\\", "�\\")); //=> 4
	P83(iji_searchLenRi("�\\�\\123�\\�\\", "�\\")); //=> 2
	P83(imi_searchLenRi("�\\�\\123�\\�\\", "�\\")); //=> 4
*/
// v2014-11-29
UINT
iji_searchCntA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	if(!pM || !search)
	{
		return 0;
	}
	UINT rtn = 0;
	CONST UINT _searchLen = iji_len(search);
	while(*pM)
	{
		if(imb_cmp(pM, search, FALSE, icase))
		{
			pM = ijp_forwardN(pM, _searchLen);
			++rtn;
		}
		else
		{
			pM = CharNextA(pM);
		}
	}
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
	INT option   // 0=���^1=Byte���^2=������
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
		case(0):                         break;
		case(1): rtn *= imi_len(search); break;
		case(2): rtn *= iji_len(search); break;
	}
	return rtn;
}
// v2016-02-11
UINT
iji_searchCntRA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase,  // TRUE=�啶����������ʂ��Ȃ�
	INT option   // 0=���^1=Byte���^2=������
)
{
	if(!pM || !search)
	{
		return 0;
	}
	UINT rtn = 0;
	CONST UINT _searchLen = imi_len(search);
	MBS *pEnd = imp_eod(pM) - _searchLen;
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
		case(0):                         break;
		case(1): rtn *= imi_len(search); break;
		case(2): rtn *= iji_len(search); break;
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
	P82(ijp_searchLA("ABCABCDEFABC", "ABC", FALSE)); //=> "ABCABCDEFABC"
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
/* (��)
	P82(ijp_searchRA("ABCABCDEFABC", "ABC", FALSE)); //=> "ABC"
*/
// v2014-11-29
MBS
*ijp_searchRA(
	MBS *pM,     // ������
	MBS *search, // ����������
	BOOL icase   // TRUE=�啶����������ʂ��Ȃ�
)
{
	if(!pM)
	{
		return pM;
	}
	MBS *pEnd = pM + imi_len(pM) - imi_len(search);
	while(pM <= pEnd)
	{
		if(imb_cmp(pEnd, search, FALSE, icase))
		{
			break;
		}
		--pEnd;
	}
	return pEnd;
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
// v2016-02-11
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
			if(*(pM + 1) == '<')
			{
				rtn = 3;
			}
			else
			{
				rtn = (*(pM + 1) == '=' ? 1 : 2);
			}
			break;

		// [0]"="
		case('='):
			rtn = 0;
			break;

		// [-2]"<" | [-1]"<=" | [3]"<>"
		case('<'):
			if(*(pM + 1) == '>')
			{
				rtn = 3;
			}
			else
			{
				rtn = (*(pM + 1) == '=' ? -1 : -2);
			}
			break;
	}
	if(bNot)
	{
		rtn += (rtn>0 ? -3 : 3);
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
	MBS *p1 = "2014�N 4��29��  {(18�� 42��) 00�b}";
	MBS *tokens = "�N���������b ";
	MBS **ary = {0};
	// ary = ija_split(p1, tokens, "{}[]", FALSE); //=> [0]"2014" [1]"4" [2]"29" [3]"{(18�� 42��) 00�b}"
	// ary = ija_split(p1, tokens, "{}[]", TRUE);  //=> [0]"2014" [1]"4" [2]"29" [3]"(18�� 42��) 00�b"
	// ��3���� = NULL | "" �̂Ƃ��A�ȍ~�̈����͖��������
	// ary = ija_split(p1, tokens, "", FALSE);     //=> [0]"2014" [1]"4" [2]"29" [3]"{<18" [4]"42" [5]")" [6]"00" [7]"}"
	// ary = ija_split(p1, tokens, "", TRUE);      //=> [0]"2014" [1]"4" [2]"29" [3]"{<18" [4]"42" [5]")" [6]"00" [7]"}"
	// ary = ija_split(p1, "", "", FALSE);         // 1�������Ԃ�
	// ary = ija_split(p1, "", "", TRUE);          // 1�������Ԃ�
	// ary = ija_split(p1, NULL, "", FALSE);       // p��Ԃ�
	// ary = ija_split(p1, NULL, "", TRUE);        // p��Ԃ�
	iary_print(ary);
	ifree(ary);
*/
// v2020-05-30
MBS
**ija_split(
	MBS *pM,       // ��������
	MBS *tokens,   // ��ؕ����i�����j
	MBS *quotes,   // quote�����i2����1�Z�b�g�^�����j�^NULL | "" �̂Ƃ�����
	BOOL quote_cut // TRUE=quote�����������^FALSE=���Ȃ�
)
{
	if(!pM || !*pM || !tokens)
	{
		return ija_token_eod(pM);
	}
	if(!*tokens)
	{
		return ija_token_zero(pM);
	}
	// �O��̋󔒂𖳎�
	MBS *sPtr = ijs_trim(pM);
	UINT arySize = 0, u1 = 0, u2 = 0;
	MBS *pBgn = 0, *pEnd = 0, *p1 = 0, *p2 = 0, *p3 = 0;
	MBS **aToken = ija_token_zero(tokens);
	MBS **aQuote = ija_token_zero((quotes ? quotes: NULL));
	u1 = 0;
	while((p1 = *(aToken + u1)))
	{
		arySize += iji_searchCnt(sPtr, p1);
		++u1;
	}
	++arySize;
	MBS **rtn = icalloc_MBS_ary(arySize);
	pBgn = pEnd = sPtr;
	u1 = 0;
	while(*pEnd)
	{
		// quote����
		u2 = 0;
		while((p1 = *(aQuote + u2)) && (p2 = *(aQuote + u2 + 1)))
		{
			p3 = pEnd;
			pEnd = ijp_bypass(p3, p1, p2);
			if(pEnd > p3)
			{
				++pEnd;
			}
			u2 += 2;
		}
		// token����
		u2 = 0;
		while((p1 = *(aToken + u2)))
		{
			if(imb_cmpf(pEnd, p1))
			{
				if(pEnd > pBgn)
				{
					*(rtn + u1) = ims_pclone(pBgn, pEnd);
					pBgn = CharNextA(pEnd);
					++u1;
				}
				pBgn = CharNextA(pEnd);
				break;
			}
			++u2;
		}
		pEnd = CharNextA(pEnd);
	}
	*(rtn + u1) = ims_pclone(pBgn, pEnd);
	// quote������
	if(quote_cut)
	{
		arySize = iary_size(rtn);
		u1 = 0;
		while(u1 < arySize)
		{
			p1 = *(rtn + u1);
			u2 = 0;
			while(*(aQuote + u2) && *(aQuote + u2 + 1))
			{
				if(imb_cmpf(p1, *(aQuote + u2)))
				{
					p2 = ijs_rm_quote(p1, *(aQuote + u2), *(aQuote + u2 + 1), FALSE, TRUE);
						imp_cpy(p1, p2);
					ifree(p2);
					break;
				}
				u2 += 2;
			}
			++u1;
		}
	}
	ifree(aQuote);
	ifree(aToken);
	ifree(sPtr);
	return rtn;
}
//-----------------------------
// NULL(�s��)�ŋ�؂��Ĕz��
//-----------------------------
/* (��)
	INT i1 = 0;
	MBS **ary = ija_token_eod("ABC");
	iary_print(ary); //=> [ABC]
*/
// v2020-05-30
MBS
**ija_token_eod(
	MBS *pM
)
{
	MBS **rtn = icalloc_MBS_ary(1);
	*(rtn + 0) = ims_clone(pM);
	return rtn;
}
//---------------------------
// �P�����Â�؂��Ĕz��
//---------------------------
/* (��)
	INT i1 = 0;
	MBS **ary = ija_token_zero("ABC");
	iary_print(ary); //=> "A" "B" "C"
*/
// v2020-05-30
MBS
**ija_token_zero(
	MBS *pM
)
{
	MBS **rtn = 0;
	if(!pM)
	{
		rtn = ima_null();
		return rtn;
	}
	CONST UINT pLen = iji_len(pM);
	rtn = icalloc_MBS_ary(pLen);
	MBS *pBgn = pM;
	MBS *pEnd = 0;
	UINT u1 = 0;
	while(u1 < pLen)
	{
		pEnd = CharNextA(pBgn);
		*(rtn + u1) = ims_pclone(pBgn, pEnd);
		pBgn = pEnd;
		++u1;
	}
	return rtn;
}
//----------------------
// quote��������������
//----------------------
/* (��)
	P82(ijs_rm_quote("[[ABC]", "[", "]", TRUE, TRUE));           //=> "[ABC"
	P82(ijs_rm_quote("[[ABC]", "[", "]", TRUE, FALSE));          //=> "ABC"
	P82(ijs_rm_quote("<A>123</A>", "<a>", "</a>", FALSE, TRUE)); //=> "123"
*/
// v2016-02-16
MBS
*ijs_rm_quote(
	MBS *pM,      // ������
	MBS *quoteL,  // ��������擪������
	MBS *quoteR,  // �������閖��������
	BOOL icase,   // TRUE=�啶����������ʂ��Ȃ�
	BOOL oneToOne // TRUE=quote����΂ŏ���
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
	imb_shiftL(rtn, (quoteL2Len*quoteL2Cnt));      // �擪�ʒu���V�t�g
	*(imp_eod(rtn) - (quoteR2Len*quoteR2Cnt)) = 0; // ������NULL���
	return rtn;
}
//---------------------
// �������������؂�
//---------------------
/* (��)
	P82(ims_addTokenNStr("+000123456.7890"));    //=> "+123,456.7890"
	P82(ims_addTokenNStr(".000123456.7890"));    //=> "0.000123456.7890"
	P82(ims_addTokenNStr("+.000123456.7890"));   //=> "+0.000123456.7890"
	P82(ims_addTokenNStr("0000abcdefg.7890"));   //=> "0abcdefg.7890"
	P82(ims_addTokenNStr("1234abcdefg.7890"));   //=> "1,234abcdefg.7890"
	P82(ims_addTokenNStr("+0000abcdefg.7890"));  //=> "+0abcdefg.7890"
	P82(ims_addTokenNStr("+1234abcdefg.7890"));  //=> "+1,234abcdefg.7890"
	P82(ims_addTokenNStr("+abcdefg.7890"));      //=> "+abcdefg0.7890"
	P82(ims_addTokenNStr("�}1234567890.12345")); //=> "�}1,234,567,890.12345"
	P82(ims_addTokenNStr("aiu������@���"));     //=> "aiu������@���"
*/
// v2016-02-18
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
	MBS *pB = pM;
	MBS *pE = pM;
	while(*pE)
	{
		if((*pE >= '0' && *pE <= '9') || *pE == '.')
		{
			break;
		}
		++pE;
	}
	pRtnE = imp_pcpy(pRtnE, pB, pE);

	// (2) [0-9] �Ԃ�T�� => "000123456"
	pB = pE;
	while(*pE)
	{
		if(*pE < '0' || *pE > '9')
		{
			break;
		}
		++pE;
	}

	// (2-11) �擪�� [.] ���H
	if(*pB == '.')
	{
		pRtnE = imp_cpy(pRtnE, "0");
		imp_cpy(pRtnE, pB);
	}

	// (2-21) �A������ �擪��[0] �𒲐� => "123456"
	else
	{
		while(*pB)
		{
			if(*pB != '0')
			{
				break;
			}
			++pB;
		}
		if(*(pB - 1) == '0' && (*pB < '0' || *pB > '9'))
		{
			--pB;
		}

		// (2-22) ", " �t�^ => "123,456"
		p1 = ims_pclone(pB, pE);
			u1 = pE - pB;
			if(u1 > 3)
			{
				u2 = u1 % 3;
				if(u2)
				{
					pRtnE = imp_pcpy(pRtnE, p1, p1 + u2);
				}
				while(u2 < u1)
				{
					if(u2 > 0 && u2 < u1)
					{
						pRtnE = imp_cpy(pRtnE, ",");
					}
					pRtnE = imp_pcpy(pRtnE, p1 + u2, p1 + u2 + 3);
					u2 += 3;
				}
			}
			else
			{
				pRtnE = imp_cpy(pRtnE, p1);
			}
		ifree(p1);

		// (2-23) �c�� => ".7890"
		imp_cpy(pRtnE, pE);
	}
	return rtn;
}
//-----------------------
// ���E�̕�������������
//-----------------------
/* (��)
	P82(ijs_cut(" \tABC\t ", " \t", " \t")); //=> "ABC"
*/
// v2014-10-13
MBS
*ijs_cut(
	MBS *pM,
	MBS *rmLs,
	MBS *rmRs
)
{
	MBS *rtn = 0;
	MBS **aryLs = ija_token_zero(rmLs);
	MBS **aryRs = ija_token_zero(rmRs);
		rtn = ijs_cutAry(pM, aryLs, aryRs);
	ifree(aryLs);
	ifree(aryRs);
	return rtn;
}
// v2014-11-05
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
	MBS *pEnd = imp_eod(pBgn);
	MBS *p1 = 0;
	// �擪
	if(execL)
	{
		while(*pBgn)
		{
			i1 = 0;
			while((p1 = *(aryLs + i1)))
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
			while((p1 = *(aryRs + i1)))
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
// v2014-11-05
MBS
*ARY_SPACE[] = {
	"\x20",     // " "
	"\x81\x40", // "�@"
	"\t",
	EOD
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
	EOD
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
	P82(ijs_replace("100YEN", "YEN", "�~")); //=> "100�~"
*/
// v2016-08-24
MBS
*ijs_replace(
	MBS *from,   // ������
	MBS *before, // �ϊ��O�̕�����
	MBS *after   // �ϊ���̕�����
)
{
	if(!from || !*from || !before || !*before)
	{
		return ims_clone(from);
	}
	UINT beforeLen = iji_len(before);
	UINT cnvCnt = iji_searchCntA(from, before, FALSE);
	MBS *rtn = icalloc_MBS(imi_len(from) + (cnvCnt*(imi_len(after) - imi_len(before))));
	MBS *rtnE = rtn;
	MBS *pBgn = 0, *pEnd = 0;
	pBgn = pEnd = from;
	while((cnvCnt--))
	{
		pEnd = ijp_searchLA(pEnd, before, FALSE);
			rtnE = imp_pcpy(rtnE, pBgn, pEnd);
			rtnE = imp_cpy(rtnE, after);
		pBgn = pEnd = ijp_forwardN(pEnd, beforeLen);
	}
	imp_cpy(rtnE, pBgn);
	return rtn;
}
//-------------------------------
// ������̏d����������ɂ���
//-------------------------------
/* (��)
	P82(ijs_simplify("123abcabcabc456", "abc")); //=> "123abc456"
*/
// v2014-11-30
MBS
*ijs_simplify(
	MBS *pM,    // ������
	MBS *search // ����������
)
{
	if(!pM)
	{
		return NULL;
	}
	if(!search || !*search)
	{
		return ims_clone(pM);
	}
	MBS *rtn = icalloc_MBS(imi_len(pM));
	MBS *rtnE = rtn;
	UINT iLen = 0;
	MBS *pEnd = pM;
	while(*pEnd)
	{
		iLen = iji_searchLenL(pEnd, search);
		if(iLen > 0)
		{
			--iLen;
			pEnd += iLen;
		}
		rtnE = ijp_cpy(rtnE, pEnd);
		pEnd = CharNextA(pEnd);
	}
	return rtn;
}
//-----------------------------
// ���I�����̕����ʒu���V�t�g
//-----------------------------
/* (��)
	MBS *p1 = ims_clone("123456789");
	P82(p1); //=> "123456789"
	imb_shiftL(p1, 3);
	P82(p1); //=> "456789"
	imb_shiftR(p1, 3);
	P82(p1); //=> "456"
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
	memcpy(pM, pM + byte, (u1-byte + 1)); // NULL���R�s�[
	return TRUE;
}
// v2016-02-16
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
	*(pM + u1 - byte) = 0;
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
	P83(inum_atoi("-0123.45")); //=> -123
*/
// v2015-12-31
INT
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
	return atoi(pM);
}
/* (��)
	P83(inum_atoi64("-0123.45")); //=> -123
*/
// v2015-12-31
INT64
inum_atoi64(
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
	P84(inum_atof("-0123.45")); //=> -123.45000000
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
	http: //www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/mt19937ar.c
	��L�R�[�h�����Ɉȉ��̊֐��ɂ��ăJ�X�^�}�C�Y���s�����B
	MT�֘A�̍ŐV���i�h���ł�SFMT�ATinyMT�Ȃǁj�ɂ��Ă͉��L���Q�Ƃ̂���
		http: //www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
*/
/* �T���v��
INT
main()
{
	#define Output 1000 // �o�͐�
	#define Min      -5 // �ŏ��l(> = 0)
	#define Max       5 // �ő�l(<UINT_MAX)
	#define Row       2 // �s������o�͐�
	INT i1 = 0;
	MT_initByAry(TRUE); // ������
	for(i1 = 0; i1 < Output; i1++)
	{
		P(
			"%20.12f%c",
			MT_irandDBL(Min, Max, 10),
			((i1 % Row) == (Row - 1) ? '\n' : ' ')
		);
	}
	MT_freeAry(); // ���
	#undef Output
	#undef Min
	#undef Max
	#undef Row
	return 0;
}
*/
/*
	Period parameters
*/
#define   MT_N 624
#define   MT_M 397
#define   MT_MATRIX_A         0x9908b0dfUL // constant vector a
#define   MT_UPPER_MASK       0x80000000UL // most significant w-r bits
#define   MT_LOWER_MASK       0x7fffffffUL // least significant r bits
static UINT MT_i1 = (MT_N + 1); // MT_i1 == MT_N + 1 means au1[MT_N] is not initialized
static UINT *MT_au1 = 0;        // the array forthe state vector
// v2015-12-31
VOID
MT_initByAry(
	BOOL fixOn
)
{
	// ��dAlloc���
	MT_freeAry();
	MT_au1 = icalloc(MT_N, sizeof(UINT), FALSE);
	// Seed�ݒ�
	#define InitLen 4
	UINT init_key[InitLen] = {0x217, 0x1226, 0x1210, 0xBBBB};
	// fixOn == FALSE �̂Ƃ����ԂŃV���b�t��
	if(!fixOn)
	{
		init_key[3] &= (INT)GetTickCount();
	}
	INT i = 1, j = 0, k = InitLen;
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
	#undef InitLen
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
//---------------
// �������𔭐�
//---------------
/* (��)
	MT_initByAry(FALSE); // ������
	INT i1 = 0;
	for(i1 = 8; i1 <= 128; i1 *= 2)
	{
		P832(i1, MT_irand_words(i1, FALSE));
		P832(i1, MT_irand_words(i1, TRUE));
	}
	MT_freeAry(); // ���
*/
// v2015-12-30
MBS
*MT_irand_words(
	UINT size, // ������
	BOOL ext   // FALSE�̂Ƃ��p�����̂݁^TRUE�̂Ƃ�"!#$%&@"���܂�
)
{
	MBS *rtn = icalloc_MBS(size);
	MBS *w = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&@"; // FALSE = 61�^TRUE = 67
	UINT uMax = (ext ? 67 : 61);
	UINT u1 = 0;
	while(u1 < size)
	{
		*(rtn + u1) = *(w + MT_irand_INT(0, uMax));
		++u1;
	}
	return rtn;
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Command Line
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//-------------------
// �R�}���h�����擾
//-------------------
/* (��)
	iCLI_getCMD(); //=> $IWM_CMD
	// "a.exe 123 abc ..."
	P82($IWM_CMD); //=> "a.exe"
*/
// v2021-03-19
MBS
*iCLI_getCMD()
{
	MBS *pBgn = GetCommandLineA();
	MBS *pEnd = pBgn;
	for(; *pEnd && *pEnd != ' '; pEnd++);
	$IWM_CMD = ims_pclone(pBgn, pEnd);
	return $IWM_CMD;
}
//--------------------------------
// �������擾�i�R�}���h���͏����j
//--------------------------------
/* (��)
	// "a.exe 123 abc"
	iCLI_getARGS();
		//=> $IWM_ARGV => ["123", "abc"]
		//=> $IWM_ARGC => 2
*/
// v2021-03-20
MBS
**iCLI_getARGS()
{
	MBS *pBgn = ijs_trim(GetCommandLineA());
	for(; *pBgn && *pBgn != ' '; pBgn++); // �R�}���h�������X���[
	$IWM_ARGV = (*pBgn ?
		ija_split(pBgn, " ", "\"\"\'\'", TRUE) : // quote = [""]['']�̂ݑΏ�
		ima_null()
	);
	$IWM_ARGC = iary_size($IWM_ARGV);
	return $IWM_ARGV;
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
// v2016-01-19
WCS
**iwa_null()
{
	static WCS *ary[1] = {0};
	return (WCS**)ary;
}
//-------------------
// �z��T�C�Y���擾
//-------------------
/* (��)
	MBS **ary = iCLI_getARGS(); // {"123", "45", "abc", NULL}
	INT i1 = iary_size(ary);    //=> 3
*/
// v2016-01-19
UINT
iary_size(
	MBS **ary // ������
)
{
	UINT rtn = 0;
	while(*(ary + rtn))
	{
		++rtn;
	}
	return rtn;
}
// v2016-01-19
UINT
iwary_size(
	WCS **ary // ������
)
{
	UINT rtn = 0;
	while(*(ary + rtn))
	{
		++rtn;
	}
	return rtn;
}
//---------------------
// �z��̍��v�����擾
//---------------------
/* (��)
	MBS **ary = iCLI_getARGS(); // {"123", "���", NULL}
	INT i1 = iary_Mlen(ary);    //=> 7
	INT i2 = iary_Jlen(ary);    //=> 5
*/
// v2016-01-19
UINT
iary_Mlen(
	MBS **ary
)
{
	UINT size = 0, cnt = 0;
	while(*(ary + cnt))
	{
		size += imi_len(*(ary + cnt));
		++cnt;
	}
	return size;
}
// v2016-01-19
UINT
iary_Jlen(
	MBS **ary
)
{
	UINT size = 0, cnt = 0;
	while(*(ary + cnt))
	{
		size += iji_len(*(ary + cnt));
		++cnt;
	}
	return size;
}
//--------------
// �z���qsort
//--------------
/* (��)
	MBS **ary = iCLI_getARGS();
	// ���f�[�^
	P8();
	iary_print(ary);
	NL();
	// ���\�[�g
	P8();
	iary_sortAsc(ary);
	iary_print(ary);
	NL();
	// �t���\�[�g
	P8();
	iary_sortDesc(ary);
	iary_print(ary);
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
// v2014-02-07
/*
VOID
iary_sort(
	MBS **ary, // �z��
	BOOL asc   // TRUE=�����^FALSE=�~��
)
{
	qsort(
		(MBS*)ary,
		iary_size(ary),
		sizeof(MBS**),
		(asc ? iary_qsort_cmpAsc : iary_qsort_cmpDesc)
	);
}
*/
//---------------------
// �z��𕶎���ɕϊ�
//---------------------
/* (��)
	MBS **ary = iCLI_getARGS();
	MBS *p1 = iary_join(ary, "\t");
	P82(p1);
*/
// v2020-05-14
MBS
*iary_join(
	MBS **ary, // �z��
	MBS *token // ��ؕ���
)
{
	UINT arySize = iary_size(ary);
	UINT tokenSize = imi_len(token);
	UINT u1 = iary_Mlen(ary) + (tokenSize*arySize);
	MBS *rtn = icalloc_MBS(u1);
	MBS *p1 = rtn;
	u1 = 0;
	while(u1 < arySize)
	{
		p1 = imp_cpy(p1, *(ary + u1));
		if(token)
		{
			p1 = imp_cpy(p1, token);
		}
		++u1;
	}
	*(p1-tokenSize) = 0;
	return rtn;
}
//---------------------------
// �z�񂩂�󔒁^�d��������
//---------------------------
/*
	// �L���z��}�b�s���O
	//  (��)TRUE
	//    {"aaa", "AAA", "BBB", "", "bbb"}
	//    { 1   ,  -1  ,  -1  ,  0,  1   } // Flg
	MBS *args[] = {"aaa", "AAA", "BBB", "", "bbb", NULL};

	// �召���
	//   TRUE  => "aaa", "bbb"
	//   FALSE => "aaa", "AAA", "bbb", "BBB"
	MBS **pAryUsed = iary_simplify(args, TRUE);
	UINT uAryUsed = iary_size(pAryUsed);
	UINT u1 = 0;
		while(u1 < uAryUsed)
		{
			P832(u1, *(pAryUsed + u1));
			++u1;
		}
	ifree(pAryUsed);
*/
// v2020-05-30
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
		if(**(ary + u1) && iAryFlg[u1] > -1)
		{
			iAryFlg[u1] = 1; // ��
			u2 = u1 + 1;
			while(u2 < uArySize)
			{
				if(icase)
				{
					if(imb_cmppi(*(ary + u1), *(ary + u2)))
					{
						iAryFlg[u2] = -1; // �~
					}
				}
				else
				{
					if(imb_cmpp(*(ary + u1), *(ary + u2)))
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
			*(rtn + u2) = ims_clone(*(ary + u1));
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
	// �L���z��}�b�s���O
	//  (��)
	//    {"c:\", "", "c:\A\B\", "d:\A\B\", "C:\"}
	//    { 0   ,  0,  2       ,  2       ,  0   } // Depth : 0-MAX_PATH
	//    { 1   ,  0,  -1      ,  -1      ,  -1  } // Flg   : 1=OK , 0="" , -1=�_�u��
	//  (��)���݂��Ȃ�Dir�͖��������
	MBS *args[] = {"d:", "d:\\A\\B", "d:\\A\\B\\C\\D\\E\\", "", "D:\\A\\B\\", NULL};
	MBS **pAryUsed = iary_higherDir(args, 2); // �K�w = 2
	UINT uAryUsed = iary_size(pAryUsed);

	// depth
	//   0 => "d:\", "d:\A\B\", "d:\A\B\C\D\E\"
	//   1 => "d:\", "d:\A\B\", "d:\A\B\C\D\E\"
	//   2 => "d:\", "d:\A\B\C\D\E\"
	//   3 => "d:\"
	UINT u1 = 0;
		while(u1 < uAryUsed)
		{
			P832(u1, *(pAryUsed + u1));
			++u1;
		}
	ifree(pAryUsed);
*/
// v2020-05-30
MBS
**iary_higherDir(
	MBS **ary,
	UINT depth // �K�w = 0..MAX_PATH
)
{
	CONST UINT uArySize = iary_size(ary);
	UINT u1 = 0, u2 = 0;
	// ����Dir�̂ݒ��o
	MBS **sAry = icalloc_MBS_ary(uArySize);
	u1 = 0;
	while(u1 < uArySize)
	{
		*(sAry + u1) = (iFchk_typePathA(*(ary + u1)) == 1 ? iFget_AdirA(*(ary + u1)) : "");
		++u1;
	}
	// ���\�[�g
	iary_sortAsc(sAry);
	// iAryDepth ����
	INT *iAryDepth = icalloc_INT(uArySize); // �����l = 0
	u1 = 0;
	while(u1 < uArySize)
	{
		iAryDepth[u1] = iji_searchCnt(*(sAry + u1), "\\");
		++u1;
	}
	// iAryFlg ����
	INT *iAryFlg = icalloc_INT(uArySize); // �����l = 0
	// ��ʂ֏W��
	u1 = 0;
	while(u1 < uArySize)
	{
		if(iAryDepth[u1])
		{
			if(iAryFlg[u1] > -1)
			{
				iAryFlg[u1] = 1;
			}
			u2 = u1 + 1;
			while(u2 < uArySize)
			{
				// �O����v�^�召��ʂ��Ȃ�
				if(imb_cmpfi(*(sAry + u2), *(sAry + u1)))
				{
					// ���\�[�g�Ȃ̂� u2, u1
					iAryFlg[u2] = (iAryDepth[u2] <= (iAryDepth[u1] + (INT)depth) ? -1 : 1);
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
			*(rtn + u2) = ims_clone(*(sAry + u1));
			++u2;
		}
		++u1;
	}
	ifree(iAryFlg);
	ifree(iAryDepth);
	ifree(sAry);
	return rtn;
}
//---------------------
// �z��̃N���[���쐬
//---------------------
/* (��)
	MBS **ary1 = iCLI_getARGS();
	MBS **ary2 = iary_clone(ary1);
	P83(iary_size(ary1));
		iary_print(ary1);
	P83(iary_size(ary2));
		iary_print(ary2);
	ifree(ary2);
	ifree(ary1);
*/
// v2015-12-31
MBS
**iary_clone(
	MBS **ary
)
{
	UINT size = iary_size(ary);
	MBS **rtn = icalloc_MBS_ary(size);
	MBS **ps = rtn;
	while(size)
	{
		*(ps++) = ims_clone(*(ary++));
		--size;
	}
	return rtn;
}
//-----------
// �z��ꗗ
//-----------
/* (��)
	// **ary = {"a", "b", "c"} �̂Ƃ�
	iget_ary_print(ary); //=> [0]"a", [1]"b", [2]"c"
*/
// v2014-10-16
VOID
iary_print(
	MBS **ary // ������
)
{
	UINT aSize = iary_size(ary);
	UINT u1 = 0;
	while(u1 < aSize)
	{
		P("[%04d] \"%s\"\n", u1 + 1, *(ary + u1));
		++u1;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	File/Dir����(WIN32_FIND_DATAA)
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
/*
	typedef struct _WIN32_FIND_DATAA {
		DWORD dwFileAttributes;
		FILETIME ftCreationTime;   // ctime
		FILETIME ftLastAccessTime; // mtime
		FILETIME ftLastWriteTime;  // atime
		DWORD nFileSizeHigh;
		DWORD nFileSizeLow;
		MBS cFileName[MAX_PATH];
	} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
	typedef struct _FILETIME {
		DWORD dwLowDateTime;
		DWORD dwHighDateTime;
	} FILETIME;
	typedef struct _SYSTEMTIME {
		INT wYear;
		INT wMonth;
		INT wDayOfWeek;
		INT wDay;
		INT wHour;
		INT wMinute;
		INT wSecond;
		INT wMilliseconds;
	} SYSTEMTIME;
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
	MBS *p1 = imp_cpy(FI->fullnameA, dir);
		imp_cpy(p1, "*");
	HANDLE hfind = FindFirstFileA(FI->fullnameA, &F);
		// Dir
		iFinfo_initA(FI, &F, dir, dirLenA, NULL);
			P2(FI->fullnameA);
		// File
		do
		{
			/// P82(F.cFileName);
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
		P832(FI->iFsize, FI->fullnameA);
		P82(ijp_forwardN(FI->fullnameA, FI->iFname));
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
// v2016-05-06
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
	MBS *p1 = imp_cpy(FI->fullnameA, dir);
	MBS *p2 = imp_cpy(p1, name);
	UINT u1 = p2 - p1;

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
	// (��)"c:\"
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
// v2016-05-06
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
	WCS *p1 = iwp_cpy(FI->fullnameW, dir);
	WCS *p2 = iwp_cpy(p1, name);
	UINT u1 = p2 - p1;

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
	// (��)"c:\"
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
// v2019-08-15
BOOL
iFinfo_init2M(
	$struct_iFinfoA *FI, //
	MBS *path            // �t�@�C���p�X
)
{
	// ���݃`�F�b�N
	/// P83(iFchk_existPathA(path));
	if(!iFchk_existPathA(path))
	{
		return FALSE;
	}
	MBS *path2 = iFget_AdirA(path); // ���path��Ԃ�
		INT iFtype = iFchk_typePathA(path2);
		UINT uDirLen = (iFtype == 1 ? imi_len(path2) : (UINT)(PathFindFileNameA(path2) - path2));
		MBS *pDir = (FI->fullnameA); // tmp
			imp_pcpy(pDir, path2, path2 + uDirLen);
		MBS *sName = NULL;
			if(iFtype == 1)
			{
				// Dir
				imp_cpy(path2 + uDirLen, "."); // Dir�����p "." �t�^
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
// v2017-10-11
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
	*(rtn + 0) = (iAttr & FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-'); // 16: Dir
	*(rtn + 1) = (iAttr & FILE_ATTRIBUTE_READONLY  ? 'r' : '-'); //  1: ReadOnly
	*(rtn + 2) = (iAttr & FILE_ATTRIBUTE_HIDDEN    ? 'h' : '-'); //  2: Hidden
	*(rtn + 3) = (iAttr & FILE_ATTRIBUTE_SYSTEM    ? 's' : '-'); //  4: System
	*(rtn + 4) = (iAttr & FILE_ATTRIBUTE_ARCHIVE   ? 'a' : '-'); // 32: Archive
	return rtn;
}
// v2016-02-11
UINT
iFinfo_attrAtoUINT(
	MBS *sAttr // "r, h, s, d, a" => 55
)
{
	if(!sAttr || !*sAttr)
	{
		return 0;
	}
	MBS **ap1 = ija_token(sAttr, "");
	MBS **ap2 = iary_simplify(ap1, TRUE);
		MBS *p1 = iary_join(ap2, "");
	ifree(ap2);
	ifree(ap1);
	// �������ɕϊ�
	CharLowerA(p1);
	UINT rtn = 0;
	MBS *p2 = p1;
	while(*p2)
	{
		// �p�o��
		switch(*p2)
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
		++p2;
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
	INT64 I1 = ((INT64)ftime.dwHighDateTime << 32) + ftime.dwLowDateTime;
	I1 /= 10000000; // (�d�v) MicroSecond �폜
	return ((DOUBLE)I1 / 86400) + 2305814.0;
}
// v2014-11-21
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
			i_y = *(ai + 0);
			i_m = *(ai + 1);
			i_d = *(ai + 2);
			i_h = *(ai + 3);
			i_n = *(ai + 4);
			i_s = *(ai + 5);
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
/*
//-------------------
// FileCopy�T���v��
//-------------------
MBS *iFn = "infile.dat";
MBS *oFn = "outfile.dat";
$struct_iFinfoA *FI = iFinfo_allocA();
	if(iFinfo_init2M(FI, iFn))
	{
		FILE *iFp = ifopen(iFn, "r");
		FILE *oFp = ifopen(oFn, "w");
			$struct_ifreadBuf *Buf = ifreadBuf_alloc(FI->iFsize);
				UINT bufSize = 0;
				while((bufSize = ifreadBuf(iFp, Buf)))
				{
					P83(ifwrite(oFp, Buf->ptr, bufSize));
				}
			ifreadBuf_free(Buf);
		ifclose(oFp);
		ifclose(iFp);
	}
iFinfo_freeA(FI);
*/
// v2016-01-18
FILE
*ifopen(
	MBS *Fn,  //
	MBS *mode // fopen()�Ɠ����^���"b"���[�h�ɊJ��
)
{
	MBS mode2[4] = "";
		imp_pcpy(mode2, mode, mode + 3);
	INT c = 0, chk = 0;
	// (��)"r", "r+"�̂Ƃ�"b"��t�^����
	UINT u1 = 1;
	while((c = mode2[u1]))
	{
		if(c == 'b')
		{
			++chk;
		}
		++u1;
	}
	if(!chk)
	{
		strcat(mode2, "b");
	}
	FILE *Fp = fopen(Fn, mode2);
	if(!Fp)
	{
		P("'%s' <= ", Fn);
		ierr_end("Can't open a file!");
	}
	return Fp;
}
/*
	//--------------------------
	// TSV������f�[�^�̂ݏo��
	//--------------------------
	MBS *iFn = "sample.tsv";
	if(iFchk_Tfile(iFn))
	{
		FILE *iFp = ifopen(iFn, "r");
			MBS *rtn = 0;
			MBS **aryX2 = 0;
			UINT u1 = 0;
			while((rtn = ifreadLine(iFp, TRUE)))
			{
				aryX2 = ija_token(rtn, "\t");
				if(**aryX2)
				{
					u1 = 0;
					while(*(aryX2 + u1))
					{
						P82(*(aryX2 + u1));
						++u1;
					}
					NL();
				}
				ifree(aryX2);
				ifree(rtn);
			}
		ifclose(iFp);
	}
	else
	{
		P2("Not a Text File!!");
	}
*/
//v2015-05-15
MBS
*ifreadLine(
	FILE *iFp,
	BOOL rmCrlf // TRUE=�s����"\r\n"��"\0"�ɕϊ�
)
{
	CONST UINT _buf = 128; // 128Byte="76Byte�ȉ�"��"256Byte��"�̗����ɑΉ�
	UINT uBuf = _buf;
	MBS *rtn = icalloc_MBS(uBuf);
	UINT u1 = 0;
	INT c1 = 0;
	while(TRUE)
	{
		if(u1 < uBuf)
		{
			c1 = getc(iFp);
			if(c1 == EOF)
			{
				break;
			}
			*(rtn + u1) = c1;
			if(*(rtn + u1) == '\n')
			{
				if(rmCrlf)
				{
					*(rtn + u1) = 0;
					if(*(rtn + u1 - 1) == '\r')
					{
						*(rtn + u1 - 1) = 0;
					}
				}
				break;
			}
			++u1;
		}
		else
		{
			uBuf += _buf;
			rtn = irealloc_MBS(rtn, uBuf);
		}
	}
	return (c1 == EOF ? (*rtn ? rtn : NULL) : rtn);
}
// v2015-05-12
$struct_ifreadBuf
*ifreadBuf_alloc(
	INT64 fsize // ifopen()����t�@�C���̃T�C�Y
)
{
	if(fsize < 1)
	{
		fsize = 0;
	}
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	CONST INT64 _freeMem = (stat.dwAvailPhys * 0.5); // �ő�50%�ȉ�
	CONST INT64 _maxByte = 1024 * 1024 * 512;        // �ő�512MB
	if(fsize > _freeMem)
	{
		fsize = _freeMem;
	}
	if(fsize > _maxByte)
	{
		fsize = _maxByte;
	}
	$struct_ifreadBuf *Buf = ($struct_ifreadBuf*)icalloc(1, sizeof($struct_ifreadBuf), FALSE);
		icalloc_err(Buf);
	(Buf->size) = (UINT)fsize;
	(Buf->ptr) = icalloc_MBS(Buf->size);
	return Buf;
}
// v2015-01-03
UINT
ifreadBuf(
	FILE *Fp,              // ���̓t�@�C���|�C���^
	$struct_ifreadBuf *Buf // ifreadBuf_alloc(Fn)�Ŏ擾
)
{
	UINT size = (Buf->size);
	if(size<1 || feof(Fp))
	{
		return 0;
	}
	MBS *p1 = (Buf->ptr);
	UINT rtn = fread(p1, 1, size, Fp);
	*(p1 + rtn) = EOF;
	return rtn;
}
// v2016-08-30
VOID
ifreadBuf_free(
	$struct_ifreadBuf *Buf
)
{
	if(Buf)
	{
		ifree(Buf->ptr);
		ifree(Buf);
	}
}
//-----------------------------------
// �t�@�C�������݂���Ƃ�TRUE��Ԃ�
//-----------------------------------
/* (��)
	P83(iFchk_existPathA("."));    //=> 1
	P83(iFchk_existPathA(""));     //=> 0
	P83(iFchk_existPathA("\\"));   //=> 1
	P83(iFchk_existPathA("\\\\")); //=> 0
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
	P83(iFchk_typePathA("."));                       //=> 1
	P83(iFchk_typePathA(".."));                      //=> 1
	P83(iFchk_typePathA("\\"));                      //=> 1
	P83(iFchk_typePathA("c:\\windows\\"));           //=> 1
	P83(iFchk_typePathA("c:\\windows\\system.ini")); //=> 2
	P83(iFchk_typePathA("\\\\Network\\"));           //=> 2(�s���ȂƂ���)
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
	P83(iFchk_Bfile("aaa.exe")); //=> TRUE
	P83(iFchk_Bfile("aaa.txt")); //=> FALSE
	P83(iFchk_Bfile("???"));     //=> FALSE (���݂��Ȃ��Ƃ�)
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
	return (0<cnt ? TRUE : FALSE);
}
//---------------------
// �t�@�C�������𒊏o
//---------------------
/* (��)
	// ���݂��Ȃ��Ă��@�B�I�ɒ��o
	// �K�v�ɉ����� iFchk_existPathA() �őO����
	MBS *p1 = "c:\\windows\\win.ini";
	P82(iget_regPathname(p1, 0)); //=>"c:\windows\win.ini"
	P82(iget_regPathname(p1, 1)); //=>"win.ini"
	P82(iget_regPathname(p1, 2)); //=>"win"
*/
// v2016-02-11
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
	MBS *pBgn = 0, *pEnd = 0;
	// Dir or File ?
	if(PathIsDirectoryA(path))
	{
		if(option < 1)
		{
			pEnd = imp_cpy(rtn, path);
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
				imp_cpy(rtn, path);
				break;

			// name+ext
			case(1):
				pBgn = PathFindFileNameA(path);
				imp_cpy(rtn, pBgn);
				break;

			// name
			case(2):
				pBgn = PathFindFileNameA(path);
				pEnd = PathFindExtensionA(pBgn);
				imp_pcpy(rtn, pBgn, pEnd);
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
	P82(iFget_AdirA(".\\"));
*/
// v2020-05-29
MBS
*iFget_AdirA(
	MBS *path // �t�@�C���p�X
)
{
	MBS *rtn = icalloc_MBS(IMAX_PATHA);
	MBS *p1 = 0;
	switch(iFchk_typePathA(path))
	{
		// Dir
		case(1):
			p1 = ims_ncat_clone(path, "\\", NULL);
				_fullpath(rtn, p1, IMAX_PATHA);
			ifree(p1);
			break;
		// File
		case(2):
			_fullpath(rtn, path, IMAX_PATHA);
			break;
	}
	return rtn;
}
//---------------------------
// ����Dir �� "\"�t���ɕϊ�
//---------------------------
/* (��)
	// _fullpath() �̉��p
	P82(iFget_RdirA(".")); => ".\\"
*/
// v2014-10-25
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
		MBS *pEnd = imp_eod(rtn) - u1;
		*(pEnd + 0) = '\\';
		*(pEnd + 1) = 0;
	}
	else
	{
		rtn = ijs_simplify(path, "\\");
	}
	return rtn;
}
//--------------------
// ���K�w��Dir���쐬
//--------------------
/* (��)
	P83(imk_dir("aaa\\bbb"));
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
		//=> $IWM_ColorDefault // ���ݐF��ۑ�
		//=> $IWM_StdoutHandle // ��ʃn���h����ۑ�
	iConsole_setTextColor(9 + (15 * 16)); // �ύX
	iConsole_setTextColor(-1); // ���̐F�ɖ߂�
*/
// v2021-03-19
UINT
iConsole_getColor()
{
	$IWM_StdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo($IWM_StdoutHandle, &info);
	$IWM_ColorDefault = info.wAttributes;
	return $IWM_ColorDefault;
}
// v2021-03-19
VOID
iConsole_setTextColor(
	INT rgb // �\���F
)
{
	if(rgb < 0)
	{
		rgb = $IWM_ColorDefault;
	}
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, rgb);
}
/////////////////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
	Clipboard
---------------------------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////////////////
//---------------------------
// �N���b�v�{�[�h���g�p����
//---------------------------
/* (��)
	iClipboard_setText("abcde"); // �R�s�[
	iClipboard_addText("123");   // �ǉ�
	P82(iClipboard_getText());   // ������擾
	iclipboard_erase();          // �N���A
*/
// v2015-11-26
BOOL
iClipboard_erase()
{
	if(OpenClipboard(0))
	{
		EmptyClipboard();
		CloseClipboard();
		return TRUE;
	}
	return FALSE;
}
// v2015-12-05
BOOL
iClipboard_setText(
	MBS *pM
)
{
	if(!pM)
	{
		return FALSE;
	}
	BOOL rtn = FALSE;
	UINT size = imi_len(pM) + 1;       // +NULL ��
	MBS *p1 = GlobalAlloc(GPTR, size); // �p�����Ȃ��̂ŌŒ胁�����ŏ\��
	if(p1)
	{
		imp_cpy(p1, pM);
		if(OpenClipboard(0))
		{
			SetClipboardData(CF_TEXT, p1);
			rtn = TRUE;
		}
		CloseClipboard();
	}
	GlobalFree(p1);
	return rtn;
}
// v2015-11-26
MBS
*iClipboard_getText()
{
	MBS *rtn = 0;
	if(OpenClipboard(0))
	{
		HANDLE handle = GetClipboardData(CF_TEXT);
		MBS *p1 = GlobalLock(handle);
		rtn = ims_clone(p1);
		GlobalUnlock(handle);
		CloseClipboard();
	}
	return rtn;
}
// v2020-05-29
BOOL
iClipboard_addText(
	MBS *pM
)
{
	if(!pM)
	{
		return FALSE;
	}
	MBS *p1 = iClipboard_getText();         // Clipboard��ǂ�
	MBS *p2 = ims_ncat_clone(p1, pM, NULL); // pM��ǉ�
	BOOL rtn = iClipboard_setText(p2);      // Clipboard�֏�������
	ifree(p2);
	ifree(p1);
	return rtn;
}
// v2015-12-05
/*
	�G�N�X�v���[���ɂăR�s�[���ꂽ�t�@�C������t�@�C�����𒊏o
	���������ł͒�������������̂ŁA��������g�p����
*/
MBS
*iClipboard_getDropFn(
	UINT option // iFget_extPathname()�Q��
)
{
	MBS *rtn = 0;
	MBS *pBgn = 0, *pEnd = 0;
	UINT u1 = 0;
	OpenClipboard(0);
	HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
	if(hDrop)
	{
		MBS *buf = icalloc_MBS(IMAX_PATHA);
		//�t�@�C�������擾
		CONST UINT cnt = DragQueryFileA(hDrop, 0XFFFFFFFF, NULL, 0);
		// rtn�쐬
		UINT size = 0;
		u1 = 0;
		while(u1 < cnt)
		{
			size += DragQueryFileA(hDrop, u1, buf, IMAX_PATHA) + 4; // CRLF+NULL+"\" ��
			++u1;
		}
		pEnd = rtn = icalloc_MBS(size);
		// �p�X�����K��
		size = 0;
		u1 = 0;
		while(u1 < cnt)
		{
			size = DragQueryFileA(hDrop, u1, buf, IMAX_PATHA); //�t�@�C�������擾
			// ���x���d�����Ȃ��ˑ��x�d���̂Ƃ��̓C�����C����
			pBgn = iFget_extPathname(buf, option);
			if(*pBgn)
			{
				pEnd = imp_cpy(pEnd, pBgn);
				pEnd = imp_cpy(pEnd, "\r\n"); // CRLF
			}
			ifree(pBgn);
			++u1;
		}
		ifree(buf);
	}
	// ��x�AiClipboard_setText()�˃y�[�X�g����ƁA
	// �N���b�v�{�[�h�ɔ��f����Ȃ��Ȃ�̂� EmptyClipboard()����.
	EmptyClipboard();
	CloseClipboard();
	return rtn;
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
	INT i1 = 0;
	INT *ai = idate_cnv_month(2011, 14, 1, 12);
	for(i1 = 0; i1 < 2; i1++) P("[%d]", *(ai + i1)); //=> [2012][2]
	ifree(ai);
	NL();
*/
// v2013-03-21
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
	*(rtn + 0) = i_y;
	*(rtn + 1) = i_m;
	return rtn;
}
//---------------
// ��������Ԃ�
//---------------
/* (��)
	idate_month_end(2012, 2); //=> 29
	idate_month_end(2013, 2); //=> 28
*/
// v2013-03-21
INT
idate_month_end(
	INT i_y, // �N
	INT i_m  // ��
)
{
	INT *ai = idate_cnv_month1(i_y, i_m);
	INT i_d = MDAYS[*(ai + 1)];
	// �[�Q������
	if(*(ai + 1) == 2 && idate_chk_uruu(*(ai + 0)))
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
	P83(idate_chk_month_end(2012, 2, 28, FALSE)); //=> FALSE
	P83(idate_chk_month_end(2012, 2, 29, FALSE)); //=> TRUE
	P83(idate_chk_month_end(2012, 1, 60, TRUE )); //=> TRUE
	P83(idate_chk_month_end(2012, 1, 60, FALSE)); //=> FALSE
*/
// v2014-11-21
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
			i_y = *(ai1 + 0);
			i_m = *(ai1 + 1);
			i_d = *(ai1 + 2);
		ifree(ai1);
	}
	return (i_d == idate_month_end(i_y, i_m) ? TRUE : FALSE);
}
//-----------------------
// str��������CJD�ɕϊ�
//-----------------------
/* (��)
	P84(idate_MBStoCjd(">[1970-12-10] and <[2015-12-10]")); //=> 2440931.00000000
	P84(idate_MBStoCjd(">[1970-12-10]"));                   //=> 2440931.00000000
	P84(idate_MBStoCjd(">[+1970-12-10]"));                  //=> 2440931.00000000
	P84(idate_MBStoCjd(">[0000-00-00]"));                   //=> 1721026.00000000
	P84(idate_MBStoCjd(">[-1970-12-10]"));                  //=> 1001859.00000000
	P84(idate_MBStoCjd(">[2016-01-02]"));                   //=> 2457390.00000000
	P84(idate_MBStoCjd(">[0D]"));    // "2016-01-02 10:44:08" => 2457390.00000000
	P84(idate_MBStoCjd(">[]"));      // "2016-01-02 10:44:08" => 2457390.44731481
	P84(idate_MBStoCjd(""));                                //=> DBL_MAX
*/
// v2016-01-03
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
		*(ai + 0), *(ai + 1), *(ai + 2), *(ai + 3), *(ai + 4), *(ai + 5)
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
	DOUBLE rtn = idate_ymdhnsToCjd(*(ai + 0), *(ai + 1), *(ai + 2), *(ai + 3), *(ai + 4), *(ai + 5));
	ifree(ai);
	ifree(p1);
	return rtn;
}
//--------------------
// str��N�����ɕ���
//--------------------
/* (��)
	MBS **ary = idate_MBS_to_mAryYmdhns("-2012-8-12 12:45:00");
		iary_print(ary); //=> "-2012" "8" "12" "12" "45" "00"
	ifree(ary);
*/
// v2016-01-20
MBS
**idate_MBS_to_mAryYmdhns(
	MBS *pM // (��)"2012-03-12 13:40:00"
)
{
	BOOL bMinus = (pM && *pM == '-' ? TRUE : FALSE);
	MBS **rtn = ija_token(pM, "/.-: "); // NULL�Ή�
	if(bMinus)
	{
		MBS *p1 = *(rtn + 0);
		memmove(p1 + 1, p1, imi_len(p1) + 1); // NULL���ړ�
		*p1 = '-';
	}
	return rtn;
}
//-------------------------
// str��N�����ɕ���(int)
//-------------------------
/* (��)
	INT i1 = 0;
	INT *ai = idate_MBS_to_iAryYmdhns("-2012-8-12 12:45:00");
	for(i1 = 0; *i1 < 6; i1++)
	{
		P83(*(ai + i1)); //=> -2012 8 12 12 45 00
	}
	ifree(ary);
*/
// v2014-10-19
INT
*idate_MBS_to_iAryYmdhns(
	MBS *pM // (��)"2012-03-12 13:40:00"
)
{
	INT *rtn = icalloc_INT(6);
	MBS **ary = idate_MBS_to_mAryYmdhns(pM);
	INT i1 = 0;
	while(i1 < 6)
	{
		*(rtn + i1) = atoi(*(ary + i1));
		++i1;
	}
	ifree(ary);
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
// v2013-03-21
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
		i_y = *(ai + 0);
		i_m = *(ai + 1);
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
// v2016-01-06
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
	*(rtn + 0) = i_h;
	*(rtn + 1) = i_n;
	*(rtn + 2) = i_s;
	return rtn;
}
//--------------------
// CJD��YMDHNS�ɕϊ�
//--------------------
// v2014-10-19
INT
*idate_cjd_to_iAryYmdhns(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *ai1 = icalloc_INT(6);
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
		*(ai1 + 0) = i_y;
		*(ai1 + 1) = i_m;
		*(ai1 + 2) = i_d;
		*(ai1 + 3) = *(ai2 + 0);
		*(ai1 + 4) = *(ai2 + 1);
		*(ai1 + 5) = *(ai2 + 2);
	ifree(ai2);
	return ai1;
}
//----------------------
// CJD��FILETIME�ɕϊ�
//----------------------
// v2014-11-21
FILETIME
idate_cjdToFtime(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd);
	INT i_y = *(ai1 + 0), i_m = *(ai1 + 1), i_d = *(ai1 + 2), i_h = *(ai1 + 3), i_n = *(ai1 + 4), i_s = *(ai1 + 5);
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
// v2013-03-21
MBS
*idate_cjd_mWday(
	DOUBLE cjd // cjd�ʓ�
)
{
	return *(WDAYS + idate_cjd_iWday(cjd));
}
//------------------------------
// cjd�ʓ�����N���̒ʓ���Ԃ�
//------------------------------
// v2013-03-21
INT
idate_cjd_yeardays(
	DOUBLE cjd // cjd�ʓ�
)
{
	INT *ai = idate_cjd_to_iAryYmdhns(cjd);
	INT i1 = *(ai + 0);
	ifree(ai);
	return (INT)(cjd-idate_ymdhnsToCjd(i1, 1, 0, 0, 0, 0));
}
//--------------------------------------
// ���t�̑O�� [6] = {y, m, d, h, n, s}
//--------------------------------------
/* (��)
	INT *ai = idate_add(2012, 1, 31, 0, 0, 0, 0, 1, 0, 0, 0, 0);
	INT i1 = 0;
	for(i1 = 0; i1 < 6; i1++)
	{
		P("[%d]%d\n", i1, *(ai + i1)); //=> 2012, 2, 29, 0, 0, 0
	}
*/
// v2015-12-24
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
		i1 = (INT)idate_month_end(*(ai2 + 0) + add_y, *(ai2 + 1) + add_m);
		if(*(ai2 + 2) > (INT)i1)
		{
			*(ai2 + 2) = (INT)i1;
		}
		ai1 = idate_reYmdhns(*(ai2 + 0) + add_y, *(ai2 + 1) + add_m, *(ai2 + 2), *(ai2 + 3), *(ai2 + 4), *(ai2 + 5));
		i2 = 0;
		while(i2 < 6)
		{
			*(ai2 + i2) = *(ai1 + i2);
			++i2;
		}
		ifree(ai1);
		++flg;
	}
	if(add_d != 0)
	{
		cjd = idate_ymdhnsToCjd(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), *(ai2 + 3), *(ai2 + 4), *(ai2 + 5));
		ai1 = idate_cjd_to_iAryYmdhns(cjd + add_d);
		i2 = 0;
		while(i2 < 6)
		{
			*(ai2 + i2) = *(ai1 + i2);
			++i2;
		}
		ifree(ai1);
		++flg;
	}
	if(add_h != 0 || add_n != 0 || add_s != 0)
	{
		ai1 = idate_reYmdhns(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), *(ai2 + 3) + add_h, *(ai2 + 4) + add_n, *(ai2 + 5) + add_s);
		i2 = 0;
		while(i2 < 6)
		{
			*(ai2 + i2) = *(ai1 + i2);
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
			*(ai1 + i2) = *(ai2 + i2);
			++i2;
		}
	}
	else
	{
		ai1 = idate_reYmdhns(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), *(ai2 + 3), *(ai2 + 4), *(ai2 + 5));
	}
	ifree(ai2);
	return ai1;
}
//-------------------------------------------------------
// ���t�̍� [8] = {sign, y, m, d, h, n, s, days}
//   (��)�֋X��A(���t1<=���t2)�ɕϊ����Čv�Z���邽�߁A
//       ���ʂ͈ȉ��̂Ƃ���ƂȂ�̂Œ��ӁB
//       �E5/31��6/30 :  + 1m
//       �E6/30��5/31 : -30d
//-------------------------------------------------------
/* (��)
	INT *ai = idate_diff(2012, 1, 31, 0, 0, 0, 2012, 2, 29, 0, 0, 0); //=> sign = 1, y = 0, m = 1, d = 0, h = 0, n = 0, s = 0, days = 29
	INT i1 = 0;
	for(i1 = 0; i1 < 7; i1++) P("[%d]%d\n", i1, *(ai + i1)); //=> 2012, 2, 29, 0, 0, 0, 29
*/
// v2014-11-23
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
	/*
		cjd2>cjd1 �ɓ���
	*/
	if(cjd1 > cjd2)
	{
		*(rtn + 0) = -1; // sign(-)
		cjd  = cjd1;
		cjd1 = cjd2;
		cjd2 = cjd;
	}
	else
	{
		*(rtn + 0) = 1; // sign(+)
	}
	/*
		days
	*/
	*(rtn + 7) = (INT)(cjd2 - cjd1);
	/*
		���K��2
	*/
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd1);
	INT *ai2 = idate_cjd_to_iAryYmdhns(cjd2);
	/*
		ymdhns
	*/
	*(rtn + 1) = *(ai2 + 0) - *(ai1 + 0);
	*(rtn + 2) = *(ai2 + 1) - *(ai1 + 1);
	*(rtn + 3) = *(ai2 + 2) - *(ai1 + 2);
	*(rtn + 4) = *(ai2 + 3) - *(ai1 + 3);
	*(rtn + 5) = *(ai2 + 4) - *(ai1 + 4);
	*(rtn + 6) = *(ai2 + 5) - *(ai1 + 5);
	/*
		m ����
	*/
	INT *ai3 = idate_cnv_month2(*(rtn + 1), *(rtn + 2));
		*(rtn + 1) = *(ai3 + 0);
		*(rtn + 2) = *(ai3 + 1);
	ifree(ai3);
	/*
		hns ����
	*/
	while(*(rtn + 6) < 0)
	{
		*(rtn + 6) += 60;
		*(rtn + 5) -= 1;
	}
	while(*(rtn + 5) < 0)
	{
		*(rtn + 5) += 60;
		*(rtn + 4) -= 1;
	}
	while(*(rtn + 4) < 0)
	{
		*(rtn + 4) += 24;
		*(rtn + 3) -= 1;
	}
	/*
		d ����
		�O����
	*/
	if(*(rtn + 3) < 0)
	{
		*(rtn + 2) -= 1;
		if(*(rtn + 2) < 0)
		{
			*(rtn + 2) += 12;
			*(rtn + 1) -= 1;
		}
	}
	/*
		�{����
	*/
	if(*(rtn + 0) > 0)
	{
		ai3 = idate_add(*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), *(ai1 + 3), *(ai1 + 4), *(ai1 + 5), *(rtn + 1), *(rtn + 2), 0, 0, 0, 0);
			i1 = (INT)idate_ymdhnsToCjd(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), 0, 0, 0);
			i2 = (INT)idate_ymdhnsToCjd(*(ai3 + 0), *(ai3 + 1), *(ai3 + 2), 0, 0, 0);
		ifree(ai3);
	}
	else
	{
		ai3 = idate_add(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), *(ai2 + 3), *(ai2 + 4), *(ai2 + 5), -*(rtn + 1), -*(rtn + 2), 0, 0, 0, 0);
			i1 = (INT)idate_ymdhnsToCjd(*(ai3 + 0), *(ai3 + 1), *(ai3 + 2), 0, 0, 0);
			i2 = (INT)idate_ymdhnsToCjd(*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), 0, 0, 0);
		ifree(ai3);
	}
	i3 = idate_ymd_num(*(ai1 + 3), *(ai1 + 4), *(ai1 + 5));
	i4 = idate_ymd_num(*(ai2 + 3), *(ai2 + 4), *(ai2 + 5));
	/* �ϊ���
		"05-31" "06-30" �̂Ƃ� m = 1, d = 0
		"06-30" "05-31" �̂Ƃ� m = 0, d = -30
		����������͔��ɃV�r�A�Ȃ̂ň��ՂɕύX�����!!
	*/
	if(*(rtn + 0) > 0                                                     // +�̂Ƃ��̂�
		&& i3 <= i4                                                       // (��) "12:00:00 =< 23:00:00"
		&& idate_chk_month_end(*(ai2 + 0), *(ai2 + 1), *(ai2 + 2), FALSE) // (��) 06-"30" �͌������H
		&& *(ai1 + 2) > *(ai2 + 2)                                        // (��) 05-"31" > 06-"30"
	)
	{
		*(rtn + 2) += 1;
		*(rtn + 3) = 0;
	}
	else
	{
		*(rtn + 3) = i1 - i2 - (INT)(i3 > i4 ? 1 : 0);
	}
	ifree(ai2);
	ifree(ai1);
	return rtn;
}
//--------------------------
// diff()�^add()�̓���m�F
//--------------------------
/* (��)����1�N����2000�N���̃����_����100��ɂ��ĕ]��
	idate_diff_checker(1, 2000, 100);
*/
// v2015-12-30
/*
VOID
idate_diff_checker(
	INT from_year, // ���N����
	INT to_year,   // ���N�܂�
	INT repeat     // ����������
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
	INT i = 0;
	P("+----From---+------To-----+---sign, y, m, d--+---DateAdd--+----+\n");
	MT_initByAry(TRUE); // ������
	for(i = repeat; i; i--)
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
		ai3 = idate_diff(*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), 0, 0, 0, *(ai2 + 0), *(ai2 + 1), *(ai2 + 2), 0, 0, 0);
		ai4 = (*(ai3 + 0) > 0 ?
				idate_add(*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), 0, 0, 0, *(ai3 + 1), *(ai3 + 2), *(ai3 + 3), 0, 0, 0) :
				idate_add(*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), 0, 0, 0, -(*(ai3 + 1)), -(*(ai3 + 2)), -(*(ai3 + 3)), 0, 0, 0)
			);
		// �v�Z���ʂ̏ƍ�
		sprintf(s1, "%d%02d%02d", *(ai2 + 0), *(ai2 + 1), *(ai2 + 2));
		sprintf(s2, "%d%02d%02d", *(ai4 + 0), *(ai4 + 1), *(ai4 + 2));
		err = (imb_cmpp(s1, s2) ? "" : "NG");
		P(
			"%5d-%02d-%02d : %5d-%02d-%02d  [%2d, %4d, %2d, %2d]  %5d-%02d-%02d  %s\n",
			*(ai1 + 0), *(ai1 + 1), *(ai1 + 2), *(ai2 + 0), *(ai2 + 1), *(ai2 + 2),
			*(ai3 + 0), *(ai3 + 1), *(ai3 + 2), *(ai3 + 3), *(ai4 + 0), *(ai4 + 1), *(ai4 + 2),
			err
		);
		ifree(ai4);
		ifree(ai3);
		ifree(ai2);
		ifree(ai1);
	}
	MT_freeAry(); // ���
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
/* (��)ymdhns
	INT *ai = idate_reYmdhns(1970, 12, 10, 0, 0, 0);
	MBS *s1 = 0;
		s1 = idate_format_ymdhns("%g%y-%m-%d(%a), %c/%C", *(ai + 0), *(ai + 1), *(ai + 2), *(ai + 3), *(ai + 4), *(ai + 5));
		P82(s1);
	ifree(s1);
	ifree(ai);
*/
/* (��)diff
	INT *ai = idate_diff(1970, 12, 10, 0, 0, 0, 2016, 10, 6, 0, 0, 0);
	MBS *s1 = idate_format_diff("%g%y-%m-%d, %W�T+%w��, %S�b", *(ai + 0), *(ai + 1), *(ai + 2), *(ai + 3), *(ai + 4), *(ai + 5), *(ai + 6), *(ai + 7));
		P82(s1);
	ifree(s1);
	ifree(ai);
*/
// v2020-08-08
MBS
*idate_format_diff(
	MBS *format, //
	INT i_sign,  // �����^-1="-", 0<="+"
	INT i_y,     // �N
	INT i_m,     // ��
	INT i_d,     // ��
	INT i_h,     // ��
	INT i_n,     // ��
	INT i_s,     // �b
	INT i_days   // �ʎY���^idate_diff()�Ŏg�p
)
{
	if(!format)
	{
		return "";
	}
	MBS *rtn = icalloc_MBS(imi_len(format) + (8 * iji_searchCnt(format, "%")));
	MBS *pEnd = rtn;
	MBS s1[32] = "";
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
					pEnd = imp_cpy(pEnd, idate_cjd_mWday(cjd));
					break;

				case 'A': // �j����
					sprintf(s1, "%d", idate_cjd_iWday(cjd));
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'c': // �N���̒ʎZ��
					sprintf(s1, "%d", idate_cjd_yeardays(cjd));
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'C': // CJD�ʎZ��
					sprintf(s1, CJD_FORMAT, cjd);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'J': // JD�ʎZ��
					sprintf(s1, CJD_FORMAT, jd);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'e': // �N���̒ʎZ�T
					sprintf(s1, "%d", idate_cjd_yearweeks(cjd));
					pEnd = imp_cpy(pEnd, s1);
					break;

				// Diff
				case 'Y': // �ʎZ�N
					sprintf(s1, "%d", i_y);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'M': // �ʎZ��
					sprintf(s1, "%d", (i_y * 12) + i_m);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'D': // �ʎZ��
					sprintf(s1, "%d", i_days);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'H': // �ʎZ��
					sprintf(s1, "%I64d", ((INT64)i_days * 24) + i_h);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'N': // �ʎZ��
					sprintf(s1, "%I64d", ((INT64)i_days * 24 * 60) + (i_h * 60) + i_n);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'S': // �ʎZ�b
					sprintf(s1, "%I64d", ((INT64)i_days * 24 * 60 * 60) + (i_h * 60 * 60) + (i_n * 60) + i_s);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'W': // �ʎZ�T
					sprintf(s1, "%d", (INT)(i_days / 7));
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'w': // �ʎZ�T�̗]��
					sprintf(s1, "%d", (i_days % 7));
					pEnd = imp_cpy(pEnd, s1);
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
					sprintf(s1, "%04d", i_y);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'm': // ��
					sprintf(s1, "%02d", i_m);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'd': // ��
					sprintf(s1, "%02d", i_d);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'h': // ��
					sprintf(s1, "%02d", i_h);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 'n': // ��
					sprintf(s1, "%02d", i_n);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case 's': // �b
					sprintf(s1, "%02d", i_s);
					pEnd = imp_cpy(pEnd, s1);
					break;

				case '%':
					*pEnd = '%';
					++pEnd;
					break;

				default:
					--format; // else �ɏ�����U��
					break;
			}
		}
		else if(*format == '\\')
		{
			switch(*(format + 1))
			{
				case('a'): *pEnd = '\a';          break;
				case('n'): *pEnd = '\n';          break;
				case('t'): *pEnd = '\t';          break;
				default:   *pEnd = *(format + 1); break;
			}
			++format;
			++pEnd;
		}
		else
		{
			*pEnd = *format;
			++pEnd;
		}
		++format;
	}
	return rtn;
}
/* (��)
	MBS *p1 = "1970/12/10 12:25:00";
	INT *ai1 = idate_MBS_to_iAryYmdhns(p1);
	P82(idate_format_iAryToA(IDATE_FORMAT_STD, ai1));
*/
// v2015-12-24
MBS
*idate_format_iAryToA(
	MBS *format, //
	INT *ymdhns  // {y, m, d, h, n, s}
)
{
	INT *ai1 = ymdhns;
	MBS *rtn = idate_format_ymdhns(format, *(ai1 + 0), *(ai1 + 1), *(ai1 + 2), *(ai1 + 3), *(ai1 + 4), *(ai1 + 5));
	return rtn;
}
// v2014-11-23
MBS
*idate_format_cjdToA(
	MBS *format,
	DOUBLE cjd
)
{
	INT *ai1 = idate_cjd_to_iAryYmdhns(cjd);
	MBS *rtn = idate_format_ymdhns(format, *(ai1 + 0), *(ai1 + 1), *(ai1 + 2), *(ai1 + 3), *(ai1 + 4), *(ai1 + 5));
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
	P82(p1);
	ifree(p1);
	ifree(ai1);
*/
// v2021-01-31
MBS
*idate_replace_format_ymdhns(
	MBS *pM,        // �ϊ��Ώە�����
	MBS *quoteBgn,  // �͕��� 1���� (��)"["
	MBS *quoteEnd,  // �͕��� 1���� (��)"]"
	MBS *add_quote, // �o�͕����ɒǉ�����quote (��)"'"
	CONST INT i_y,  // �x�[�X�N
	CONST INT i_m,  // �x�[�X��
	CONST INT i_d,  // �x�[�X��
	CONST INT i_h,  // �x�[�X��
	CONST INT i_n,  // �x�[�X��
	CONST INT i_s   // �x�[�X�b
)
{
	if(!pM)
	{
		return NULL;
	}
	MBS *p1 = 0, *p2 = 0, *p3 = 0, *p4 = 0, *p5 = 0;
	MBS *pEnd = 0;
	INT i1 = iji_searchCnt(pM, quoteBgn);
	MBS *rtn = icalloc_MBS(imi_len(pM) + (32 * i1));

	// quoteBgn ���Ȃ��Ƃ��Ap�̃N���[����Ԃ�
	if(!i1)
	{
		imp_cpy(rtn, pM);
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
	pEnd = rtn;
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
				imp_pcpy(ts1, p1, p2); // ��͗p������

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
					i1 = inum_atoi(p3); // �������琔���𒊏o
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
							*(ai + 3) = *(ai + 4) = *(ai + 5) = 0;
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
						*(ai + 0), *(ai + 1), *(ai + 2), *(ai + 3), *(ai + 4), *(ai + 5)
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
						pEnd = imp_cpy(pEnd, pAddQuote);
						pEnd = imp_cpy(pEnd, p5);
						pEnd = imp_cpy(pEnd, pAddQuote);
					ifree(p5);
					ifree(ai);
				}
				p2 += (iQuote2); // quoteEnd.len
				p1 = p2;
			}
		}
		else
		{
			pEnd = imp_pcpy(pEnd, p2, p2 + 1);
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
// v2013-03-21
INT
*idate_now_to_iAryYmdhns(
	BOOL area // TRUE=LOCAL, FALSE=SYSTEM
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
	INT *ai = icalloc_INT(7);
		*(ai + 0) = (INT)st.wYear;
		*(ai + 1) = (INT)st.wMonth;
		*(ai + 2) = (INT)st.wDay;
		*(ai + 3) = (INT)st.wHour;
		*(ai + 4) = (INT)st.wMinute;
		*(ai + 5) = (INT)st.wSecond;
		*(ai + 6) = (INT)st.wMilliseconds;
	return ai;
}
//------------------
// ������cjd��Ԃ�
//------------------
/* (��)
	idate_nowToCjd(0); // System(-9h)
	idate_nowToCjd(1); // Local
*/
// v2013-03-21
DOUBLE
idate_nowToCjd(
	BOOL area // TRUE=LOCAL, FALSE=SYSTEM
)
{
	INT *ai = idate_now_to_iAryYmdhns(area);
	INT y = *(ai + 0), m = *(ai + 1), d = *(ai + 2), h = *(ai + 3), n = *(ai + 4), s = *(ai + 5);
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
	printf("%f�x\n", rtnGeoIBLto10A(24, 26, 58.495200));
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
	printf("%f�x\n", rtnGeoIBLto10B(242658.495200));
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
	printf("%d�x%d��%f�b\n", geo.deg, geo.min, geo.sec);
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
	printf("%fkm %f�x\n", geo.dist, geo.angle);
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
