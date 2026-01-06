// Item.h: interface for the CItem class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include "CommonTypes.h"

#define DEF_MAXITEMEQUIPPOS		15
#define DEF_EQUIPPOS_NONE		0
#define DEF_EQUIPPOS_HEAD		1
#define DEF_EQUIPPOS_BODY		2
#define DEF_EQUIPPOS_ARMS		3
#define DEF_EQUIPPOS_PANTS		4
#define DEF_EQUIPPOS_BOOTS		5
#define DEF_EQUIPPOS_NECK		6
#define DEF_EQUIPPOS_LHAND		7
#define DEF_EQUIPPOS_RHAND		8
#define DEF_EQUIPPOS_TWOHAND	9
#define DEF_EQUIPPOS_RFINGER	10
#define DEF_EQUIPPOS_LFINGER	11
#define DEF_EQUIPPOS_BACK		12
#define DEF_EQUIPPOS_FULLBODY	13

#define DEF_ITEMTYPE_NONE			0
#define DEF_ITEMTYPE_EQUIP			1
#define DEF_ITEMTYPE_APPLY			2
#define DEF_ITEMTYPE_USE_DEPLETE	3
#define DEF_ITEMTYPE_INSTALL		4
#define DEF_ITEMTYPE_CONSUME		5
#define DEF_ITEMTYPE_ARROW			6
#define DEF_ITEMTYPE_EAT			7
#define DEF_ITEMTYPE_USE_SKILL		8
#define DEF_ITEMTYPE_USE_PERM		9
#define DEF_ITEMTYPE_USE_SKILL_ENABLEDIALOGBOX	10
#define DEF_ITEMTYPE_USE_DEPLETE_DEST			11
#define DEF_ITEMTYPE_MATERIAL					12

class CItem
{
public:
	inline CItem()
	{
		std::memset(m_cName, 0, sizeof(m_cName));
		m_sSprite = 0;
		m_sSpriteFrame = 0;
		m_dwAttribute = 0;
		m_sItemSpecEffectValue1 = 0;
		m_sItemSpecEffectValue2 = 0;
		m_sItemSpecEffectValue3 = 0;
	}

	inline virtual ~CItem()
	{
	}
	
	char  m_cName[21];

	short m_sIDnum;					// �������� ���� ��ȣ
	char  m_cItemType;
	char  m_cEquipPos;
	short m_sItemEffectType;
	short m_sItemEffectValue1, m_sItemEffectValue2, m_sItemEffectValue3;
	short m_sItemEffectValue4, m_sItemEffectValue5, m_sItemEffectValue6;
	uint16_t  m_wMaxLifeSpan;
	short m_sSpecialEffect;

	//short m_sSM_HitRatio, m_sL_HitRatio;
	//v1.432 ���߷� ���� ��� ���Ѵ�. ��� Ư�� �ɷ� ��ġ�� ����.
	short m_sSpecialEffectValue1, m_sSpecialEffectValue2;

	short m_sX, m_sY;

	short m_sSprite;
	short m_sSpriteFrame;

	char  m_cApprValue;
	char  m_cSpeed;

	uint32_t m_wPrice;
	uint16_t  m_wWeight;
	short m_sLevelLimit;
	char  m_cGenderLimit;

	short m_sRelatedSkill;

	char  m_cCategory;
	bool  m_bIsForSale;

	uint32_t m_dwCount;
	short m_sTouchEffectType;
	short m_sTouchEffectValue1, m_sTouchEffectValue2, m_sTouchEffectValue3;
	char  m_cItemColor; // v1.4 ���� ������ ������ ����Ǿ���. 
	short m_sItemSpecEffectValue1, m_sItemSpecEffectValue2, m_sItemSpecEffectValue3;
	uint16_t  m_wCurLifeSpan;
	uint32_t m_dwAttribute;				// aaaa bbbb cccc dddd eeee ffff xxxx xxx1 
	// 1: Custom-Made Item flag 
	// a: Item �Ӽ� ���� 
	// b: Item �Ӽ� ����
	// c: Ư�� ������ ���� Flag 
	// d: Ư�� ������ ���� ���� 
	// e: Ư�� ������ �߰� ���� Flag
	// f: Ư�� ������ �߰� ���� ����
};
