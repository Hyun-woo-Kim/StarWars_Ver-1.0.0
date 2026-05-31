// Copyright Epic Games, Inc. All Rights Reserved.

#include "MeleeAttackNotify.h"
#include "OutlawCharacter.h"

void UANS_MeleeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (AOutlawCharacter* Character = Cast<AOutlawCharacter>(MeshComp->GetOwner()))
	{
		Character->StartMeleeTrace();
	}
}

void UANS_MeleeAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (AOutlawCharacter* Character = Cast<AOutlawCharacter>(MeshComp->GetOwner()))
	{
		Character->PerformMeleeTrace();
	}
}

void UANS_MeleeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (AOutlawCharacter* Character = Cast<AOutlawCharacter>(MeshComp->GetOwner()))
	{
		Character->EndMeleeTrace();
	}
}
