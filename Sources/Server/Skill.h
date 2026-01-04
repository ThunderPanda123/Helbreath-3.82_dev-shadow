// Skill.h: interface for the CSkill class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>



#define DEF_SKILLEFFECTTYPE_GET			    1		// ��´�. �����⳪ ������ 
#define DEF_SKILLEFFECTTYPE_PRETEND			2		// ...�� ü �ϴ� 
#define DEF_SKILLEFFECTTYPE_TAMING			3		//  ����̱� �迭	


class CSkill
{
public:
	inline CSkill()
	{
		ZeroMemory(m_cName, sizeof(m_cName));
	}

	inline virtual ~CSkill()
	{
	}

	char m_cName[21];

	short m_sType;
	short m_sValue1, m_sValue2, m_sValue3, m_sValue4, m_sValue5, m_sValue6;

};
