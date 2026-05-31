// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "MeleeAttackNotify.generated.h"

/**
 * ANS_MeleeAttack — 광선검 공격 판정 구간 AnimNotifyState
 *
 * 몽타주에서 공격 판정 구간(예: N1 6F~15F)에 배치하면
 * NotifyTick마다 AOutlawCharacter::PerformMeleeTrace()를 호출합니다.
 *
 * [에디터 사용법]
 * 1. 몽타주 에디터 → Notifies 트랙에 이 노티파이 스테이트 추가
 * 2. 공격 판정 시작 프레임 ~ 종료 프레임 구간으로 크기 조절
 * 3. 별도 파라미터 설정 불필요 — 히트박스 데이터는 BP_OutlawCharacter에서 관리
 */
UCLASS(DisplayName = "Melee Attack Trace")
class STARWARS_API UANS_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("MeleeAttack");
	}
};
