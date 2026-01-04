// Client.h: interface for the CClient class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "XSocket.h"
#include "Item.h"
#include "GuildsMan.h"
#include "Magic.h"
#include "GlobalDef.h"
#include "MobCounter.h"
#include <fstream>
#include <vector>
#include <string>
using namespace std;

#define DEF_CLIENTSOCKETBLOCKLIMIT	15

#define DEF_MSGBUFFERSIZE	30000
#define DEF_MAXITEMS		50
#define DEF_MAXBANKITEMS	200
#define DEF_MAXGUILDSMAN	128 // �ִ� ���� �� 

#define	DEF_MAXMAGICTYPE	100	// �����Ϸ��� �α׼������뵵 �ٲپ��? �Ѵ�.
#define DEF_MAXSKILLTYPE	60

#define DEF_MAXPARTYMEMBERS	100

#define DEF_SPECABLTYTIMESEC	1200

class CClient  
{
public:

	int m_iAngelicStr, m_iAngelicInt, m_iAngelicDex, m_iAngelicMag;
		
	char m_cVar;
	int m_iRecentWalkTime;
	int m_iRecentRunTime;
	short m_sV1;
	char m_cHeroArmourBonus;

	bool bCreateNewParty();

	// Hack Checkers
	DWORD m_dwMagicFreqTime, m_dwMoveFreqTime, m_dwAttackFreqTime;
	bool m_bIsMoveBlocked, m_bMagicItem;
	DWORD dwClientTime;
	bool m_bMagicConfirm;
	int m_iSpellCount;
	bool m_bMagicPauseTime;
	//int m_iUninteruptibleCheck;
	//char m_cConnectionCheck;

	bool m_bIsClientConnected;

	CClient(HWND hWnd);
	virtual ~CClient();

	char m_cCharName[11];
	char m_cAccountName[11];
	char m_cAccountPassword[11];

	bool  m_bIsInitComplete;
	bool  m_bIsMsgSendAvailable;

	char  m_cMapName[11];
	char  m_cMapIndex;
	short m_sX, m_sY;
	
	char  m_cGuildName[21];		// �����? �̸� 
	char  m_cLocation[11];      //  <- ���̸��� �״��? ����ȴ�?. �Ҽ� ���� 
	int   m_iGuildRank;			// -1�̸� ���ǹ�. 0�̸� ���? ������. �����? ��峻������? ���� 
	int   m_iGuildGUID;
	
	char  m_cDir;
	short m_sType;				// ���� Ÿ�� <- �������� �ÿ� ���Ѵ�. 
	short m_sOriginalType;		// �������� Ÿ��
	short m_sAppr1;
	short m_sAppr2;
	short m_sAppr3;
	short m_sAppr4;
	int   m_iApprColor;			// v1.4 ���� �÷����̺�
	int   m_iStatus;

	DWORD m_dwTime, m_dwHPTime, m_dwMPTime, m_dwSPTime, m_dwAutoSaveTime, m_dwHungerTime, m_dwWarmEffectTime;
	// Player Ư��ġ 

	char m_cSex, m_cSkin, m_cHairStyle, m_cHairColor, m_cUnderwear;

	int  m_iHP;						// Hit Point
	int  m_iHPstock;
	int  m_iMP;
	int  m_iSP;
	DWORD  m_iExp;
	DWORD m_iNextLevelExp;
	bool m_bIsKilled;

	int  m_iDefenseRatio;		// Defense Ratio
	int  m_iHitRatio;			// Hit Ratio

	// v1.432 �������? �ʴ´�.
	//int  m_iHitRatio_ItemEffect_SM; // ������ �������� ���� HitRatio ���氪
	//int  m_iHitRatio_ItemEffect_L;

	int  m_iDamageAbsorption_Armor[DEF_MAXITEMEQUIPPOS];		// ���� �������� ���� Damage ���� ȿ��
	int  m_iDamageAbsorption_Shield;	// Parrying ���? �������� Damage ���� ȿ�� 

	int  m_iLevel;
	int  m_iStr, m_iInt, m_iVit, m_iDex, m_iMag, m_iCharisma;
	//char m_cLU_Str, m_cLU_Int, m_cLU_Vit, m_cLU_Dex, m_cLU_Mag, m_cLU_Char;   // �������ÿ� �Ҵ�Ǿ�? �ö󰡴� Ư��ġ��.
	int  m_iLuck; 
	int  m_iLU_Pool;
	char m_cAura;
	//MOG var - 3.51
	int m_iGizonItemUpgradeLeft;

	int m_iAddTransMana, m_iAddChargeCritical;

	string getvalue(string val, char* fileName);

	class CMobCounter* m_pMobKillCount[100];

	void read_mobs_data();
	void save_mobs_data();

	DWORD m_iRewardGold;
	int  m_iEnemyKillCount, m_iPKCount;
	int  m_iCurWeightLoad;		// ���� �� ����ǰ ���� 

	char m_cSide;				// �÷��̾��� �� 
	
	bool m_bInhibition;

	//50Cent - Repair All
	short totalItemRepair;
	struct
	{
		char index;
		short price;
	} m_stRepairAll[DEF_MAXITEMS];

	char m_cAttackDiceThrow_SM;	// ����ġ �ֻ��� ������ ȸ�� 
	char m_cAttackDiceRange_SM;	// ����ġ �ֻ��� ���� 
	char m_cAttackDiceThrow_L;	// ����ġ �ֻ��� ������ ȸ�� 
	char m_cAttackDiceRange_L;	// ����ġ �ֻ��� ���� 
	char m_cAttackBonus_SM;		// ���� ���ʽ�
	char m_cAttackBonus_L;		// ���� ���ʽ�

	class CItem * m_pItemList[DEF_MAXITEMS];
	POINT m_ItemPosList[DEF_MAXITEMS];
	class CItem * m_pItemInBankList[DEF_MAXBANKITEMS];
	
	bool  m_bIsItemEquipped[DEF_MAXITEMS];
	short m_sItemEquipmentStatus[DEF_MAXITEMEQUIPPOS];
	char  m_cArrowIndex;		// �÷��̾ Ȱ�� ����Ҷ�? ȭ�� ������ �ε���. �ʱⰪ�� -1(�Ҵ� �ȵ�)

	char           m_cMagicMastery[DEF_MAXMAGICTYPE];
	unsigned char  m_cSkillMastery[DEF_MAXSKILLTYPE]; // v1.4

	int   m_iSkillSSN[DEF_MAXSKILLTYPE];
	bool  m_bSkillUsingStatus[DEF_MAXSKILLTYPE];
	int   m_iSkillUsingTimeID[DEF_MAXSKILLTYPE]; //v1.12

	char  m_cMagicEffectStatus[DEF_MAXMAGICEFFECTS];

	int   m_iWhisperPlayerIndex;
	char  m_cProfile[256];

	int   m_iHungerStatus;		// �����? ����Ʈ. �̰� 0�̵Ǹ� ���¹̳ʰ� ������ ������ ü�µ� �����̻� ���� �ʽ��ϴ�. 

	DWORD m_dwWarBeginTime;		// ������ ����? ������ ��ϵǴ�? �ð�. 
	bool  m_bIsWarLocation;		// ���� ������ �ִ����� ǥ�� 

	bool  m_bIsPoisoned;		// �ߵ��Ǿ������� ���� 
	int   m_iPoisonLevel;       // ���� ���� 
	DWORD m_dwPoisonTime;		// �ߵ� �ð�.
	
	int   m_iPenaltyBlockYear, m_iPenaltyBlockMonth, m_iPenaltyBlockDay; // v1.4

	//v1.4311-3 �߰� ���� ���� ������ ��ȣ�� ������ ������ �ð�
	int   m_iFightzoneNumber , m_iReserveTime, m_iFightZoneTicketNumber ; 

	class XSocket * m_pXSock;

	int   m_iAdminUserLevel;     // ������ ����. 0�̸� ��ȿ. ��ȣ�� �ö� ���� ������ Ŀ����.
	int   m_iRating;

	int   m_iTimeLeft_ShutUp;	 // �� ���� 0�� �ƴϸ� ä�ø޽����� ���޵��� �ʴ´�.
	int   m_iTimeLeft_Rating;	 // �ٸ� �÷��̾��� ������ �ű��? ���� ���� �ð�. 0�̸� ���? ����  
	int   m_iTimeLeft_ForceRecall;  // ���� ���ݵǱ� ���� �����ִ� �ð�ƽ 
	int   m_iTimeLeft_FirmStaminar; // ���¹̳ʰ� �޾� �������� �ʴ� �ð� �� 

	bool isForceSet;   //hbest
	time_t m_iForceStart;

	bool  m_bIsOnServerChange;     // �� ���� Ȱ��ȭ �Ǿ� ������ ������ ������ ���� �� ī������ ���� �ʴ´�.

	DWORD   m_iExpStock;			 // �׿��ִ� ����ġ 
	DWORD m_dwExpStockTime;		 // ExpStock ���? �ð�.

	DWORD   m_iAutoExpAmount;		 // Auto-Exp �ð� ���� ���� ����ġ 
	DWORD m_dwAutoExpTime;		 // Auto-Exp ���? �ð�.

	DWORD m_dwRecentAttackTime;  // ���� �ֱٿ� �����ߴ� �ð� 

	int   m_iAllocatedFish;		 // �� ���� 0�� �ƴϸ� �̺�Ʈ ���ø����? �̾߱��?. 
	int   m_iFishChance;		 // ���� ���� ���� 
	
	char  m_cIPaddress[21];		 // Ŭ���̾�Ʈ�� IP address
	bool  m_bIsSafeAttackMode;

	bool  m_bIsOnWaitingProcess; // �ڷ���Ʈ�� ó���� ��ٸ���? ���¶��? 
	
	int   m_iSuperAttackLeft;	 // v1.2 �ʻ��? ���? ���� Ƚ�� 
	int   m_iSuperAttackCount;   // v1.2 �ʻ��? ���? ���� ī��Ʈ. �� ī��Ʈ�� �� ���� �ʻ��? ���? Ƚ���� �þ��. 

	short m_sUsingWeaponSkill;	 // v1.2 ���� ����ϴ�? ������ ��ų �ε��� 

	int   m_iManaSaveRatio;		 // v1.2 ���� ���� ����Ʈ 
	
	bool  m_bIsLuckyEffect;		 // v1.2 ���? ȿ�� 
	int   m_iSideEffect_MaxHPdown; // v1.4 �ִ� HP ���� ȿ�� 

	int   m_iComboAttackCount;   // v1.3 ��Ÿ ���� ī��Ʈ 
	int   m_iDownSkillIndex;	 // v1.3 �ٸ� ��ų�� �ö󰥶� ���� ��ų �ε��� 

	int   m_iMagicDamageSaveItemIndex; // v1.3 ���� ���� �����? ���� ������ �ε��� (������ -1)

	short m_sCharIDnum1, m_sCharIDnum2, m_sCharIDnum3; // v1.3 �� ĳ���Ͱ� ���� ������!

	int   m_iAbuseCount;		// ��ŷ ������ �ľǿ� 
	
	bool  m_bIsBWMonitor;		// BadWord ������ΰ�??

	//bool  m_bIsExchangeMode;		// ���� ������ ��ȯ ����ΰ�?? 
	//int   m_iExchangeH;				// ��ȯ�� �����? �ε��� 
	//char  m_cExchangeName[11];		// ��ȯ�� �����? �̸� 
	//char  m_cExchangeItemName[21];	// ��ȯ�ϰ��� �ϴ� ������ �̸� 
	//char  m_cExchangeItemIndex;  // ��ȯ�� ������ �ε��� 
	//int   m_iExchangeItemAmount; // ��ȯ�� ������ ���� 
	//bool  m_bIsExchangeConfirm;  // ��ȯ Ȯ�� 

	bool  m_bIsExchangeMode;			// Is In Exchange Mode? 
	int   m_iExchangeH;					// Client ID to Exchanging with 
	char  m_cExchangeName[11];			// Name of Client to Exchanging with 
	char  m_cExchangeItemName[4][21];	// Name of Item to exchange 

	char  m_cExchangeItemIndex[4];		// ItemID to Exchange
	int   m_iExchangeItemAmount[4];		// Ammount to exchange with

	bool  m_bIsExchangeConfirm;			// Has the user hit confirm? 
	int	  iExchangeCount;				//Keeps track of items which are on list

	int   m_iQuest;				 // ���� �Ҵ��? Quest 
	int   m_iQuestID;			 // �Ҵ����? Quest�� ID�� 
	int   m_iAskedQuest;		 // ��� ����Ʈ 
	int   m_iCurQuestCount;		 // ���� ����Ʈ ���� 
	
	int   m_iQuestRewardType;	 // ����Ʈ �ذ��? ��ǰ ���� -> �������� ID���̴�.
	int   m_iQuestRewardAmount;	 // ��ǰ ���� 

	int   m_iContribution;		 // ���ÿ� ���� ���嵵. 

	bool  m_bQuestMatchFlag_Loc; // ����Ʈ ���� ������ �Ǵ��ϱ� ����.
	bool  m_bIsQuestCompleted;   // ����Ʈ�� �Ϸ�Ǿ��°�?? 

	int   m_iCustomItemValue_Attack;
	int   m_iCustomItemValue_Defense;

	int   m_iMinAP_SM;			// Custom-Item�� ȿ���� ���� �ּ� AP
	int   m_iMinAP_L;

	int   m_iMaxAP_SM;			// Custom-Item�� ȿ���� ���� �ִ� AP
	int   m_iMaxAP_L;

	bool  m_bIsNeutral;			// v1.5 �߸����θ� �Ǻ��ϱ� ���� �÷���. ó�� �ӵ��� ���̱� �����̴�.
	bool  m_bIsObserverMode;	// v1.5 ������ �������? �Ǻ� 

	int   m_iSpecialEventID;	// Ư�� ���? ����Ȯ�ο� �÷��� 

	int   m_iSpecialWeaponEffectType;	// ���? ������ ȿ�� ����
	int   m_iSpecialWeaponEffectValue;	// ���? ������ ȿ�� ��
	// ���? ������ ȿ�� ����: 
	// 0-None 1-�ʻ�������߰�? 2-�ߵ�ȿ�� 3-������  
	// 5-��ø�� 6-������ 7-������ 8-��ȭ�� 9-���빮����

	// v1.42
	int   m_iAddHP, m_iAddSP, m_iAddMP; 
	int   m_iAddAR, m_iAddPR, m_iAddDR;
	int   m_iAddMR, m_iAddAbsPD, m_iAddAbsMD; 
	int   m_iAddCD, m_iAddExp, m_iAddGold;		// ���� ���� �����? ������. �������ʹ� ���������� ���ȴ�.

	int   m_iAddResistMagic;					// v1.2 �߰� ���� ���� 
	int   m_iAddPhysicalDamage;					// v1.2 ���� �����? �߰� ����Ʈ 
	int   m_iAddMagicalDamage;	

	int   m_iAddAbsAir;							// �Ӽ��� �����? ����
	int   m_iAddAbsEarth;
	int   m_iAddAbsFire;
	int   m_iAddAbsWater;
	
	int   m_iLastDamage;

	int   m_iMoveMsgRecvCount, m_iAttackMsgRecvCount, m_iRunMsgRecvCount, m_iSkillMsgRecvCount;
	DWORD m_dwMoveLAT, m_dwRunLAT, m_dwAttackLAT;

	int   m_iSpecialAbilityTime;				// Ư�� �ɷ��� ����ϱ�? ���ؼ��� �� ���� 0�� �Ǿ��? �Ѵ�. 
	bool  m_bIsSpecialAbilityEnabled;			// Ư�� �ɷ��� Ȱ��ȭ �� �����ΰ�?
	DWORD m_dwSpecialAbilityStartTime;			// Ư�� �ɷ��� ����ϱ�? ������ �ð�
	int   m_iSpecialAbilityLastSec;				// Ư�� �ɷ� ���� �ð�.

	int   m_iSpecialAbilityType;				// �Ҵ��? Ư�� �ɷ� ����
												// ������
												// 0:�ɷ� ����  1:���ݽ� �� HP 50% ����  2:�õ� ȿ��  3: ���� ȿ��  4: ���� ��ų  5:���� �����? ��ŭ�� HP�� ��´�?.
												// �����?
												// 50: ���� ���� 0�� ����. 51:�ش� ���� �����? ��ȿȭ  52: ��5�� ���� �����? ��ȿȭ
	int   m_iSpecialAbilityEquipPos;			// ����? ���? Ư��ȿ���� ����Ǵ�? ������ �ǹ���.
	bool  m_bIsAdminCommandEnabled;
	int   m_iAlterItemDropIndex;				// ������ ���? �������� ������ �ε��� 

	int   m_iWarContribution;					// ���� ���嵵 

	DWORD m_dwSpeedHackCheckTime;				// �ӵ����� �˻� ��ƾ 
	int   m_iSpeedHackCheckExp;		
	DWORD m_dwLogoutHackCheck;

	DWORD m_dwInitCCTimeRcv;
	DWORD m_dwInitCCTime;

	char  m_cLockedMapName[11];					// ���� �� �̸�
	int   m_iLockedMapTime;						// �� ���� 0 �̻��̸� ���� �ڷ���Ʈ �ص� ���� ������ ����.

	int   m_iCrusadeDuty;						// ũ�缼�̵忡�� ���� ����: 1-�뺴. 2-�Ǽ���. 3-���ְ�
	DWORD m_dwCrusadeGUID;						// ũ�缼�̵� GUID
	DWORD m_dwHeldenianGUID;
	bool m_bInRecallImpossibleMap;

	// �� ��Ʈ���Ĵ� ���� ������ �����ϴ� ���̴�. �ѹ��� ���� �� �� ���� ������ �������� ���� ������ �����Ѵ�.
	struct {
		char cType;
		char cSide;
		short sX, sY;
	} m_stCrusadeStructureInfo[DEF_MAXCRUSADESTRUCTURES];
	int m_iCSIsendPoint;

	char m_cSendingMapName[11];
	bool m_bIsSendingMapStatus;

	// ���ְ��� �Ǽ��� �� �ִ� ����Ʈ. �Ϲ� �÷��̾��� �ڽ��� �ൿ�� ���� �������̴�.
	int  m_iConstructionPoint;

	char m_cConstructMapName[11];
	int  m_iConstructLocX, m_iConstructLocY;
	
	// 2.06
	bool m_bIsPlayerCivil;
	bool m_bIsAttackModeChange;

	// New 06/05/2004
	// Party Stuff
	int m_iPartyID;
	int m_iPartyStatus;
	int m_iReqJoinPartyClientH;
	char m_cReqJoinPartyName[12];

	int   m_iPartyRank;										// Party�������� ��ġ. -1�̸� ���ǹ�. 1�̸� ��Ƽ ������. 12�� ���? 
	int   m_iPartyMemberCount;								// ��Ƽ �ο� ���ѿ� 
	int   m_iPartyGUID;										// v1.42 Party GUID
	struct {
	int  iIndex;
	char cName[11];
	} m_stPartyMemberName[DEF_MAXPARTYMEMBERS];

	// New 07/05/2004
	DWORD m_dwLastActionTime;
	int m_iDeadPenaltyTime;

	// New 16/05/2004
	char m_cWhisperPlayerName[11];
	bool m_bIsAdminOrderGoto;
	bool m_bIsInsideWarehouse;
	bool m_bIsInsideWizardTower;
	bool m_bIsInsideOwnTown;
	bool m_bIsCheckingWhisperPlayer;
	bool m_bIsOwnLocation;
	bool m_pIsProcessingAllowed;

	// Updated 10/11/2004 - 24/05/2004
	char m_cHeroArmorBonus;

	// New 25/05/2004
	bool m_bIsBeingResurrected;

	DWORD m_dwFightzoneDeadTime;
	char m_cSaveCount;

};
